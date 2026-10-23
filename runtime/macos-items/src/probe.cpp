#include "context.h"
#include "execute.h"
#include "mount.h"
#include "transport.h"
#include <CommonCrypto/CommonDigest.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <atomic>
#include <cstdlib>
#include <new>

namespace {
constexpr char expected_hash[] = "fc0727d176ad7434e9fcea5b358cdb6b7c78ebb1f73975b76efb100c1c129a74";
constexpr unsigned char expected_uuid[] = {0x0a,0x2e,0xa5,0x7a,0x71,0x9e,0x36,0xb4,0x8a,0xb1,0xed,0xa6,0x09,0xac,0x00,0x4e};
using Dispatch = std::uint32_t(*)(void*, void*);
using namespace crimson::mac_items;
using namespace crimson::live_items;
Dispatch original{};
std::uintptr_t actor_vtable{};
std::intptr_t image_slide{};
Queue* queue{};
std::atomic<bool> enabled{};

bool read_word(std::uintptr_t address, std::uintptr_t& value) {
    mach_vm_size_t size{};
    return address && mach_vm_read_overwrite(mach_task_self(), address, sizeof(value),
        reinterpret_cast<mach_vm_address_t>(&value), &size) == KERN_SUCCESS && size == sizeof(value);
}

std::uintptr_t verified_holder(void* task, void* context, std::uint32_t& key) {
    std::uintptr_t flags{}, actor{}, user{}, vtable{}, controlled{}, component{}, holder{}, owner_key{}, user_key{};
    if (!task || !context || !read_word(reinterpret_cast<std::uintptr_t>(task)+0x20, flags) || (flags & 0x100)
        || !inspect_context(reinterpret_cast<std::uintptr_t>(context), actor_vtable, read_word).holder_matches
        || !read_word(reinterpret_cast<std::uintptr_t>(context), actor)) return 0;
    const auto local = inspect_local_actor(actor, image_slide, read_word);
    if (!local.keys_match || !read_word(actor+0x140, user) || !read_word(user, vtable)
        || vtable != 0x107e30310ULL+image_slide
        || !read_word(user+0x220, controlled) || controlled != actor
        || !read_word(actor+0x130, owner_key) || !read_word(user+0x100, user_key)
        || static_cast<std::uint32_t>(owner_key) != static_cast<std::uint32_t>(user_key)
        || !read_word(actor+0x108, component) || !read_word(component+0xb8, holder)) return 0;
    using ServerMode = bool(*)();
    if (!reinterpret_cast<ServerMode>(0x100e32104ULL+image_slide)()) return 0;
    key = local.client_key;
    return holder;
}

std::uint32_t hook(void* task, void* context) {
    const auto result = original(task, context);
    if (result != 0 || !enabled.load(std::memory_order_acquire)) return result;
    try {
        std::uint32_t key{};
        const auto holder = verified_holder(task, context, key);
        if (!holder) return result;
        queue->observe(now_ms(), key);
        const auto request = queue->take(now_ms(), key);
        if (!request) return result;
        Execution outcome{State::uncertain, Error::native_fault};
        try {
            if (request->operation == Operation::mount_grant) {
                std::uintptr_t actor{}, component{}, manager{};
                if (request->quantity != 1 || !read_word(reinterpret_cast<std::uintptr_t>(context),actor)
                    || !read_word(actor+0x108,component) || !read_word(component+0x110,manager)) {
                    outcome = {State::rejected,Error::context};
                } else {
                    auto grant_mount = [](std::uintptr_t owner,std::uint16_t type) {
                        using Grant = std::uint32_t(*)(void*,std::uint16_t,std::uint16_t,bool);
                        const auto code = reinterpret_cast<Grant>(0x100bcc1f0ULL+image_slide)(
                            reinterpret_cast<void*>(owner),type,0xffff,false);
                        std::fprintf(stderr,"CW_MAC_MOUNTS: native grant type=%u result=%u.\n",type,code);
                        return code;
                    };
                    auto recheck_mount = [&] {
                        std::uint32_t current{};
                        std::uintptr_t current_actor{}, current_component{}, current_manager{};
                        return verified_holder(task,context,current)==holder && current==key
                            && read_word(reinterpret_cast<std::uintptr_t>(context),current_actor) && current_actor==actor
                            && read_word(actor+0x108,current_component) && current_component==component
                            && read_word(component+0x110,current_manager) && current_manager==manager;
                    };
                    outcome=execute_mount(grant_mount,recheck_mount,actor,manager,
                        request->key,image_slide,read_word);
                }
            } else {
            const ItemBindings bindings{
                reinterpret_cast<decltype(ItemBindings::construct)>(0x10199c500ULL+image_slide),
                reinterpret_cast<decltype(ItemBindings::initialize)>(0x101c87774ULL+image_slide),
                reinterpret_cast<decltype(ItemBindings::serialize)>(0x1019a27a4ULL+image_slide),
                reinterpret_cast<decltype(ItemBindings::destroy)>(0x1019a04e4ULL+image_slide)};
            auto grant = [](std::uintptr_t inventory, const void* init) {
                using Grant = std::uint32_t(*)(void*, const void*);
                return reinterpret_cast<Grant>(0x100b7da10ULL+image_slide)(reinterpret_cast<void*>(inventory), init);
            };
            auto recheck = [&] {
                std::uint32_t current{};
                return verified_holder(task, context, current) == holder && current == key;
            };
            outcome = execute_item(bindings, grant, recheck, holder, request->key, request->quantity, image_slide, read_word);
            }
        } catch (...) { /* The receipt stays uncertain after any native exception. */ }
        queue->finish(*request, outcome.state, outcome.error);
        std::fprintf(stderr, "CW_MAC_ITEMS: key=%u quantity=%u state=%u error=%u; no automatic retry.\n",
            request->key, request->quantity, static_cast<unsigned>(outcome.state), static_cast<unsigned>(outcome.error));
        std::fflush(stderr);
    } catch (...) { enabled.store(false, std::memory_order_release); }
    return result;
}

bool executable_matches() {
    char path[4096];
    std::uint32_t length = sizeof(path);
    if (_NSGetExecutablePath(path, &length) != 0) return false;
    const int file = open(path, O_RDONLY | O_CLOEXEC);
    if (file == -1) return false;
    CC_SHA256_CTX hash;
    CC_SHA256_Init(&hash);
    unsigned char buffer[65536], digest[CC_SHA256_DIGEST_LENGTH];
    ssize_t count;
    while ((count = read(file, buffer, sizeof(buffer))) > 0)
        CC_SHA256_Update(&hash, buffer, static_cast<CC_LONG>(count));
    close(file);
    if (count < 0) return false;
    CC_SHA256_Final(digest, &hash);
    char hex[65];
    for (std::size_t i = 0; i < sizeof(digest); ++i) std::snprintf(hex + 2*i, 3, "%02x", digest[i]);
    if (std::strcmp(hex, expected_hash) != 0) std::fprintf(stderr, "CW_MAC_ITEMS: executable digest %s.\n", hex);
    return std::strcmp(hex, expected_hash) == 0;
}

__attribute__((constructor)) void initialize() {
    std::uint32_t image = 0;
    const mach_header_64* header = nullptr;
    for (; image < _dyld_image_count(); ++image) {
        const auto candidate = _dyld_get_image_header(image);
        if (candidate && candidate->filetype == MH_EXECUTE) {
            header = reinterpret_cast<const mach_header_64*>(candidate);
            break;
        }
    }
    if (!header || header->magic != MH_MAGIC_64 || header->cputype != CPU_TYPE_ARM64) return;
    bool uuid_matches = false;
    const auto begin = reinterpret_cast<const unsigned char*>(header + 1);
    const auto end = begin + header->sizeofcmds;
    auto next = begin;
    for (std::uint32_t i = 0; i < header->ncmds; ++i) {
        if (end - next < static_cast<std::ptrdiff_t>(sizeof(load_command))) return;
        const auto command = reinterpret_cast<const load_command*>(next);
        if (command->cmdsize < sizeof(load_command) || command->cmdsize > end - next) return;
        if (command->cmd == LC_UUID && command->cmdsize == sizeof(uuid_command)) {
            const auto uuid = reinterpret_cast<const uuid_command*>(next)->uuid;
            std::fprintf(stderr, "CW_MAC_ITEMS: main image pid=%d UUID=", getpid());
            for (unsigned char byte : reinterpret_cast<const uuid_command*>(next)->uuid) std::fprintf(stderr, "%02x", byte);
            std::fputc('\n', stderr);
            uuid_matches = std::memcmp(uuid, expected_uuid, sizeof(expected_uuid)) == 0;
        }
        next += command->cmdsize;
    }
    if (!uuid_matches) std::fputs("CW_MAC_ITEMS: UUID differs.\n", stderr);
    if (!uuid_matches || !executable_matches()) {
        std::fputs("CW_MAC_ITEMS: unsupported executable; no hook installed.\n", stderr);
        return;
    }
    const auto slide = _dyld_get_image_vmaddr_slide(image);
    image_slide = slide;
    auto slot = reinterpret_cast<std::uintptr_t*>(0x107e3e3f8ULL + slide);
    const auto target = 0x10098d510ULL + slide;
    std::uintptr_t current{};
    if (!read_word(reinterpret_cast<std::uintptr_t>(slot), current) || current != target) {
        std::fputs("CW_MAC_ITEMS: movement dispatch differs; no hook installed.\n", stderr);
        return;
    }
    original = reinterpret_cast<Dispatch>(current);
    actor_vtable = 0x107e2fe10ULL + slide;
    std::uint64_t epoch{};
    arc4random_buf(&epoch, sizeof(epoch));
    epoch = (epoch & ((1ULL<<53)-1)) | 1;
    queue = new (std::nothrow) Queue(static_cast<std::uint32_t>(getpid()), epoch);
    if (!queue) return;
    const auto page = reinterpret_cast<std::uintptr_t>(slot) & ~(static_cast<std::uintptr_t>(vm_page_size)-1);
    mach_vm_address_t region = page;
    mach_vm_size_t region_size{};
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    const auto query = mach_vm_region(mach_task_self(), &region, &region_size,
        VM_REGION_BASIC_INFO_64, reinterpret_cast<vm_region_info_t>(&info), &count, &object);
    if (object != MACH_PORT_NULL) mach_port_deallocate(mach_task_self(), object);
    if (query != KERN_SUCCESS || region > page || page - region >= region_size
        || !(info.protection & VM_PROT_READ) || (info.protection & VM_PROT_EXECUTE)) {
        std::fputs("CW_MAC_ITEMS: dispatch mapping cannot be verified; no hook installed.\n", stderr);
        return;
    }
    if (mach_vm_protect(mach_task_self(), page, vm_page_size, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS) {
        std::fputs("CW_MAC_ITEMS: dispatch page is protected; no hook installed.\n", stderr);
        return;
    }
    std::atomic_ref<std::uintptr_t>(*slot).store(reinterpret_cast<std::uintptr_t>(&hook), std::memory_order_release);
    if (mach_vm_protect(mach_task_self(), page, vm_page_size, false, info.protection) != KERN_SUCCESS) {
        std::atomic_ref<std::uintptr_t>(*slot).store(current, std::memory_order_release);
        mach_vm_protect(mach_task_self(), page, vm_page_size, false, info.protection);
        std::fputs("CW_MAC_ITEMS: protection restoration failed; observer removed.\n", stderr);
        return;
    }
    enabled.store(true, std::memory_order_release);
    if (!start_transport(queue, [] {
        const auto local = inspect_local_actor(0, image_slide, read_word);
        return local.selection_valid ? local.client_key : 0;
    })) {
        enabled.store(false, std::memory_order_release);
        std::fputs("CW_MAC_ITEMS: local connection unavailable; grants disabled.\n", stderr);
        return;
    }
    std::fputs("CW_MAC_ITEMS: exact Mac build verified; waiting for explicit app requests.\n", stderr);
}
}
