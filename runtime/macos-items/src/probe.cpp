#include "context.h"
#include "inventory.h"
#include "experiment.h"
#include "prepare.h"
#include <CommonCrypto/CommonDigest.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace {
constexpr char expected_hash[] = "fc0727d176ad7434e9fcea5b358cdb6b7c78ebb1f73975b76efb100c1c129a74";
constexpr unsigned char expected_uuid[] = {0x0a,0x2e,0xa5,0x7a,0x71,0x9e,0x36,0xb4,0x8a,0xb1,0xed,0xa6,0x09,0xac,0x00,0x4e};
using Dispatch = std::uint32_t(*)(void*, void*);
Dispatch original{};
std::uintptr_t actor_vtable{};
std::intptr_t image_slide{};
std::atomic<bool> logged_dispatch{}, logged_actor{}, logged_holder{};
std::atomic<std::int64_t> next_observation{};
std::atomic<unsigned> observation_count{};
bool experiment_enabled{};
std::atomic<bool> experiment_attempted{};

bool read_word(std::uintptr_t address, std::uintptr_t& value) {
    mach_vm_size_t size{};
    return mach_vm_read_overwrite(mach_task_self(), address, sizeof(value),
        reinterpret_cast<mach_vm_address_t>(&value), &size) == KERN_SUCCESS && size == sizeof(value);
}

void attempt_experiment(void* task, void* context) {
    if (!experiment_enabled || experiment_attempted.load()) return;
    std::uintptr_t flags{}, actor{}, user{}, vtable{}, controlled{}, component{}, holder{};
    if (!read_word(reinterpret_cast<std::uintptr_t>(task)+0x20, flags) || (flags & 0x100)
        || !read_word(reinterpret_cast<std::uintptr_t>(context), actor)
        || !crimson::mac_items::inspect_local_actor(actor, image_slide, read_word).keys_match
        || !read_word(actor+0x140, user) || !read_word(user, vtable)
        || vtable != 0x107e30310ULL+image_slide
        || !read_word(user+0x220, controlled) || controlled != actor
        || !read_word(actor+0x108, component) || !read_word(component+0xb8, holder)) return;
    using ServerMode = bool(*)();
    if (!reinterpret_cast<ServerMode>(0x100e32104ULL+image_slide)()) return;
    std::uintptr_t manager{}, definitions{}, count{}, definition{}, key{};
    if (!read_word(0x108590c20ULL+image_slide, manager) || !read_word(manager+8, count)
        || (count & 0xffffffff) <= 1244 || !read_word(manager+0x58, definitions)) return;
    for (const auto entry : {std::pair{1243U, 710001U}, std::pair{1244U, 1000081U}}) {
        if (!read_word(definitions+entry.first*8, definition) || !read_word(definition, key)
            || static_cast<std::uint32_t>(key) != entry.second) return;
    }
    if (experiment_attempted.exchange(true)) return;
    std::fputs("CW_MAC_ITEMS: one-Timber experiment latched; this session cannot retry.\n", stderr);
    try {
        const crimson::mac_items::ItemBindings bindings{
            reinterpret_cast<decltype(crimson::mac_items::ItemBindings::construct)>(0x10199c500ULL+image_slide),
            reinterpret_cast<decltype(crimson::mac_items::ItemBindings::initialize)>(0x101c87774ULL+image_slide),
            reinterpret_cast<decltype(crimson::mac_items::ItemBindings::serialize)>(0x1019a27a4ULL+image_slide),
            reinterpret_cast<decltype(crimson::mac_items::ItemBindings::destroy)>(0x1019a04e4ULL+image_slide)};
        auto prepare = [&](void* init) { return crimson::mac_items::prepare_timber(bindings, 1243, init); };
        auto observe = [&] {
            const auto timber = crimson::mac_items::count_items(holder, 1243, read_word, 0);
            const auto fine = crimson::mac_items::count_items(holder, 1244, read_word, 0);
            const auto other_timber = crimson::mac_items::count_items(holder, 1243, read_word, 5);
            const auto other_fine = crimson::mac_items::count_items(holder, 1244, read_word, 5);
            return crimson::mac_items::InventoryBaseline{timber.quantity, fine.quantity,
                other_timber.quantity, other_fine.quantity,
                timber.valid && fine.valid && other_timber.valid && other_fine.valid};
        };
        auto grant = [&](const void* init) -> std::uint32_t {
            // Recheck local selection after construction and before the native transaction.
            if (!crimson::mac_items::inspect_local_actor(actor, image_slide, read_word).keys_match) return UINT32_MAX;
            std::fputs("CW_MAC_ITEMS: submitting exactly one Timber through the native transaction.\n", stderr);
            std::fflush(stderr);
            fsync(STDERR_FILENO);
            using Grant = std::uint32_t(*)(void*, const void*);
            return reinterpret_cast<Grant>(0x100b7da10ULL+image_slide)(reinterpret_cast<void*>(holder), init);
        };
        const auto result = crimson::mac_items::one_timber_experiment(prepare, grant, observe);
        const char* label = result == crimson::mac_items::ExperimentResult::verified ? "verified-delta"
            : result == crimson::mac_items::ExperimentResult::rejected ? "rejected-before-grant" : "uncertain";
        std::fprintf(stderr, "CW_MAC_ITEMS: one-Timber result=%s; no retries.\n", label);
    } catch (...) {
        std::fputs("CW_MAC_ITEMS: native exception; experiment uncertain; no retries.\n", stderr);
    }
    std::fflush(stderr);
}

void inventory_observation(void* context) {
    std::uintptr_t actor{}, component{}, holder{}, user{}, vtable{}, selected{}, selected_key{}, actor_key{};
    if (!read_word(reinterpret_cast<std::uintptr_t>(context), actor)
        || !read_word(actor+0x108, component) || !read_word(component+0xb8, holder)) return;
    if (read_word(actor+0x140, user) && user && read_word(user, vtable)
        && read_word(user+0x218, selected) && read_word(user+0x100, selected_key)
        && read_word(actor+0x130, actor_key)) {
        std::fprintf(stderr, "CW_MAC_ITEMS: user vtable=%llx registered_actor_matches=%d owner_key_matches=%d.\n",
            static_cast<unsigned long long>(vtable-image_slide), selected==actor,
            static_cast<std::uint32_t>(selected_key)==static_cast<std::uint32_t>(actor_key));
    }
    const auto local = crimson::mac_items::inspect_local_actor(actor, image_slide, read_word);
    std::fprintf(stderr, "CW_MAC_ITEMS: candidate local manager_vtable=%llx user_vtable=%llx actor_vtable=%llx client_key=%u server_key=%u matches=%d.\n",
        static_cast<unsigned long long>(local.manager_vtable ? local.manager_vtable-image_slide : 0),
        static_cast<unsigned long long>(local.user_vtable ? local.user_vtable-image_slide : 0),
        static_cast<unsigned long long>(local.actor_vtable ? local.actor_vtable-image_slide : 0),
        local.client_key, local.server_key, local.keys_match);
    std::uintptr_t manager{}, definitions{}, count{};
    if (!read_word(0x108590c20ULL+image_slide, manager) || !manager
        || !read_word(manager+8, count) || !read_word(manager+0x58, definitions)) return;
    count &= 0xffffffff;
    if (!count || count > 65535) return;
    for (unsigned i = 0; i < count; ++i) {
        std::uintptr_t definition{}, key{};
        if (!read_word(definitions+i*8, definition)) return;
        if (!definition || !read_word(definition, key)) continue;
        key &= 0xffffffff;
        if (key != 710001 && key != 1000081) continue;
        const auto amount = crimson::mac_items::count_items(holder, static_cast<std::uint16_t>(i), read_word);
        std::fprintf(stderr, "CW_MAC_ITEMS: candidate item=%llu type=%u count_valid=%d quantity=%lld.\n",
            static_cast<unsigned long long>(key), i, amount.valid, static_cast<long long>(amount.quantity));
        std::uintptr_t bucket_count{};
        if (!read_word(holder+0x20, bucket_count)) return;
        bucket_count &= 0xffffffff;
        if (bucket_count > 128) return;
        for (unsigned bucket = 0; bucket < bucket_count; ++bucket) {
            const auto part = crimson::mac_items::count_items(holder, static_cast<std::uint16_t>(i), read_word, bucket);
            if (part.quantity || !part.valid)
                std::fprintf(stderr, "CW_MAC_ITEMS: candidate item=%llu bucket=%u count_valid=%d quantity=%lld.\n",
                    static_cast<unsigned long long>(key), bucket, part.valid, static_cast<long long>(part.quantity));
        }
    }
}

std::uint32_t hook(void* task, void* context) {
    const auto result = original(task, context);
    if (!logged_dispatch.exchange(true)) std::fputs("CW_MAC_ITEMS: native movement dispatch observed; read-only probe.\n", stderr);
    if (result != 0) return result;
    const auto observation = crimson::mac_items::inspect_context(
        reinterpret_cast<std::uintptr_t>(context), actor_vtable, read_word);
    if (observation.actor_matches && !logged_actor.exchange(true))
        std::fputs("CW_MAC_ITEMS: server actor type confirmed.\n", stderr);
    if (observation.holder_matches && !logged_holder.exchange(true)) {
        std::fputs("CW_MAC_ITEMS: candidate inventory holder round-trip confirmed; selected player and grant are NOT verified.\n", stderr);
    }
    if (observation.holder_matches && observation_count.load() < 12) {
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto next = next_observation.load();
        if (now >= next && next_observation.compare_exchange_strong(next, now+5000000000LL)) {
            std::fprintf(stderr, "CW_MAC_ITEMS: observation=%u.\n", observation_count.fetch_add(1)+1);
            inventory_observation(context);
        }
    }
    if (observation.holder_matches) attempt_experiment(task, context);
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
    const auto experiment = std::getenv("CW_MAC_ITEMS_TEST_ONCE");
    experiment_enabled = experiment && std::strcmp(experiment, "one-timber") == 0;
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
    std::fputs(experiment_enabled
        ? "CW_MAC_ITEMS: exact Mac build verified; one-Timber diagnostic armed for this process only. No grant endpoint.\n"
        : "CW_MAC_ITEMS: exact Mac build verified; read-only movement observer installed. No grant endpoint.\n", stderr);
}
}
