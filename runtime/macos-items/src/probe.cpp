#include "context.h"
#include "data_hook.h"
#include "world_probe.h"
#include "world_read.h"
#include "weather_read.h"
#include "world/time_native.h"
#include "world/world_transport.h"
#include "teleport/transport.h"
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
using WeatherJob=std::uintptr_t(*)(void*);
WeatherJob original_weather_job{};
std::uintptr_t actor_vtable{};
std::intptr_t image_slide{};
Queue* queue{};
WorldControl* world_control{};
TeleportControl* teleport_control{};
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

WorldReadBindings world_read_bindings() {
    return {
        reinterpret_cast<decltype(WorldReadBindings::acquire)>(0x100863b14ULL+image_slide),
        reinterpret_cast<decltype(WorldReadBindings::release)>(0x100e79bf4ULL+image_slide),
        reinterpret_cast<decltype(WorldReadBindings::time)>(0x100e94448ULL+image_slide),
        reinterpret_cast<decltype(WorldReadBindings::position)>(0x100ec7748ULL+image_slide),
        reinterpret_cast<decltype(WorldReadBindings::now)>(0x100e350e0ULL+image_slide),
        0x107e74b68ULL+image_slide};
}

void observe_time(void* task,void* context,std::uintptr_t holder,std::uint32_t key) {
    if(!world_control)return;
    static std::mutex observer_mutex;
    std::unique_lock observer(observer_mutex,std::try_to_lock);
    if(!observer.owns_lock())return;
    const auto now=now_ms();
    if(!world_control->needs_observation(now))return;
    std::uintptr_t actor{};
    if(!read_word(reinterpret_cast<std::uintptr_t>(context),actor))return;
    const auto view=inspect_world_probe(actor,image_slide,read_word);
    const auto fixed=time_stage_fixed(view.stage,image_slide,read_word);
    const auto bindings=world_read_bindings();
    const auto native=read_native_world(view,actor,bindings);
    std::optional<TimeContext> frame;
    const auto raw=read_time_value(0x108a6ab00ULL+image_slide,read_word);
    if(native.ready&&fixed&&!*fixed&&raw)frame=TimeContext{key,view.world,view.stage,*raw,native.time,native.native_now};
    world_control->observe(frame,now,[&](const TimeContext& current,const MacFieldTime& value){
        std::uint32_t selected{};
        if(verified_holder(task,context,selected)!=holder||selected!=key)return TimePublication::rejected;
        const TimeWriteBindings write{bindings,
            reinterpret_cast<decltype(TimeWriteBindings::publish)>(0x100eaba18ULL+image_slide),
            reinterpret_cast<decltype(TimeWriteBindings::send)>(0x10099ed7cULL+image_slide),
            0x107e4f878ULL+image_slide,image_slide};
        const auto result=publish_native_time(view,actor,current.raw,value,write,read_word);
        std::fprintf(stderr,"CW_MAC_TIME: publication=%u day=%u time=%u:%u rate=%u; no automatic retry.\n",
            static_cast<unsigned>(result),value.day,value.hour,value.minute,value.rate);
        return result;
    });
    const auto status=world_control->handle({},now,key);
    static std::uint64_t witnessed{};
    if(status.time_receipt&&status.time_receipt!=witnessed
        &&(status.time_result==world::ControlCode::published||status.time_result==world::ControlCode::released)) {
        const auto server=read_time_value(0x108a6ab00ULL+image_slide,read_word);
        const auto client=read_time_value(0x108a6aae0ULL+image_slide,read_word);
        if(server&&client&&world::fields(*server)==world::fields(*client)&&server->rate==client->rate) {
            witnessed=status.time_receipt;
            std::fprintf(stderr,"CW_MAC_TIME_WITNESS: receipt=%llu client/server day=%u time=%u:%u:%u rate=%u.\n",
                static_cast<unsigned long long>(witnessed),client->day,client->hour,client->minute,client->second,client->rate);
        }
    }
}

void observe_teleport(void* task,void* context,std::uintptr_t holder,std::uint32_t key) {
    if(!teleport_control)return;
    static std::mutex mutex;
    std::unique_lock lock(mutex,std::try_to_lock);if(!lock.owns_lock())return;
    static std::uint64_t observed{};const auto now=now_ms();if(now-observed<100)return;observed=now;
    std::uintptr_t actor{};if(!read_word(reinterpret_cast<std::uintptr_t>(context),actor))return;
    const TeleportBindings bindings{world_read_bindings(),
        reinterpret_cast<decltype(TeleportBindings::lock)>(0x1000a4bdcULL+image_slide),
        reinterpret_cast<decltype(TeleportBindings::unlock)>(0x1000a4c68ULL+image_slide),
        reinterpret_cast<decltype(TeleportBindings::yaw)>(0x100ec7a9cULL+image_slide),
        reinterpret_cast<decltype(TeleportBindings::eligibility)>(0x100eaaff8ULL+image_slide),
        reinterpret_cast<decltype(TeleportBindings::begin)>(0x10086018cULL+image_slide),image_slide};
    TeleportFrame frame;const auto code=capture_teleport(actor,bindings,read_word,frame);
    static bool reported{};
    if(!reported&&code==TeleportNative::ready){reported=true;std::fprintf(stderr,
        "CW_MAC_TELEPORT: field=%u stage=%u character=%u position=%.3f/%.3f/%.3f yaw=%f; waiting for explicit app requests.\n",
        frame.current.field_definition,frame.current.stage,frame.character,frame.current.position.x,frame.current.position.y,frame.current.position.z,frame.current.yaw);}
    teleport_control->observe(frame,code,key,now_ms(),[&](const teleport::SavedPosition& destination,std::int32_t& error,bool& attempted,std::uint64_t deadline){
        std::uint32_t selected{};if(verified_holder(task,context,selected)!=holder||selected!=key)return TeleportNative::actor;
        const auto result=begin_teleport(frame,destination,bindings,read_word,error,attempted,[&]{
            std::uint32_t current{};return now_ms()<=deadline&&verified_holder(task,context,current)==holder&&current==key;
        });
        std::fprintf(stderr,"CW_MAC_TELEPORT: result=%u attempted=%u error=%d; no automatic retry.\n",static_cast<unsigned>(result),attempted,error);
        return result;
    });
}

void observe_world(std::uintptr_t context,std::uint32_t key) {
    static const bool world_probe = [] {const auto flag=std::getenv("CW_MAC_WORLD_OBSERVE");return flag&&std::strcmp(flag,"1")==0;}();
    if(!world_probe)return;
    static std::mutex mutex;
    std::unique_lock lock(mutex,std::try_to_lock);
    if(!lock.owns_lock())return;
    static std::uint64_t world_observed{};
    static unsigned world_samples{};
    if(world_samples<24&&now_ms()-world_observed>=1000) {
        world_observed=now_ms();++world_samples;
        std::uintptr_t actor{};read_word(context,actor);
        const auto view=inspect_world_probe(actor,image_slide,read_word);
        const auto bindings=world_read_bindings();
        const auto native=read_native_world(view,actor,bindings);
        const auto weather=read_weather(view,image_slide,read_word);
        std::fprintf(stderr,"CW_MAC_WORLD_PROBE: context=%u clock=%u identity=%u world=%u stage=%u phase=%u paused=%u transform=%llx field=%llx time=%llx/%llx/%llx/%llx weather=%u manager=%llx parent_offset=%x child_offset=%x.\n",
            view.context,view.clock_read,key,view.world,view.stage,view.transition,view.field_paused,
            static_cast<unsigned long long>(view.transform),static_cast<unsigned long long>(view.field),
            static_cast<unsigned long long>(view.clock[0]),static_cast<unsigned long long>(view.clock[1]),
            static_cast<unsigned long long>(view.clock[2]),static_cast<unsigned long long>(view.clock[3]),
            view.weather_read,static_cast<unsigned long long>(view.weather),view.weather_parent_offset,view.weather_child_offset);
        std::fprintf(stderr,"CW_MAC_WORLD_READ: ready=%u day=%u time=%02u:%02u:%02u.%03u rate=%u anchor=%llu now=%llu position=%.3f/%.3f/%.3f.\n",
            native.ready,native.time.day,native.time.hour,native.time.minute,native.time.second,native.time.millisecond,native.time.rate,
            static_cast<unsigned long long>(native.time.anchor),static_cast<unsigned long long>(native.native_now),
            native.position.x,native.position.y,native.position.z);
        std::fprintf(stderr,"CW_MAC_WEATHER_READ: ready=%u reason=%u manual=%u precipitation=%f cloudiness=%f humidity=%f wind=%f fog=%f cloud=%f aerosol=%f phase=%f.\n",
            weather.ready,weather.reason,weather.manual,weather.precipitation,weather.cloudiness,weather.humidity,weather.wind,weather.fog,weather.cloud,weather.aerosol,weather.phase);
        std::fflush(stderr);
    }
}

std::uint32_t hook(void* task, void* context) {
    const auto result = original(task, context);
    if (result != 0 || !enabled.load(std::memory_order_acquire)) return result;
    try {
        std::uint32_t key{};
        const auto holder = verified_holder(task, context, key);
        if (!holder) return result;
        try { observe_time(task,context,holder,key); }
        catch (...) { if(world_control)world_control->fault(); }
        try { observe_teleport(task,context,holder,key); }
        catch (...) { if(teleport_control)teleport_control->fault(); }
        observe_world(reinterpret_cast<std::uintptr_t>(context),key);
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

std::uintptr_t weather_hook(void* job) {
    // Run before the ordinary producer and its completion signal, while its output is exclusively writable.
    if(enabled.load(std::memory_order_acquire)&&world_control) {
        try {
            const auto frame=open_weather_frame(reinterpret_cast<std::uintptr_t>(job),image_slide,world_control->session(),read_word);
            const auto local=inspect_local_actor(0,image_slide,read_word);
            const WeatherBindings bindings{
                reinterpret_cast<WeatherBool>(0x102dc2e3cULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e78c48ULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e791f8ULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e797a8ULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e79d58ULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e876a0ULL+image_slide),
                reinterpret_cast<WeatherFloat>(0x102e887b0ULL+image_slide),image_slide};
            world_control->weather_producer(frame,now_ms(),local.selection_valid?local.client_key:0,
                [&](const WeatherSettings& settings,bool acquire){return apply_weather(frame,bindings,settings,acquire,read_word);},
                [&](bool value){return manual_weather(frame,bindings,value,read_word);},
                [&](float value){return hold_weather_phase(frame,bindings,value,read_word);});
        } catch(...){world_control->weather_fault();}
    }
    return original_weather_job(job);
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
    world_control=new (std::nothrow) WorldControl(epoch);
    teleport_control=new (std::nothrow) TeleportControl(epoch);
    if(!install_data_hook(reinterpret_cast<std::uintptr_t>(slot),target,reinterpret_cast<std::uintptr_t>(&hook))) {
        std::fputs("CW_MAC_ITEMS: dispatch hook could not be verified; no grants enabled.\n",stderr);
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
    if(world_control&&!start_world_transport(world_control,[]{
        const auto local=inspect_local_actor(0,image_slide,read_word);
        return local.selection_valid?local.client_key:0;
    }))std::fputs("CW_MAC_TIME: local connection unavailable; time commands disabled.\n",stderr);
    if(teleport_control&&!start_teleport_transport(teleport_control,[]{
        const auto local=inspect_local_actor(0,image_slide,read_word);
        return local.selection_valid?local.client_key:0;
    }))std::fputs("CW_MAC_TELEPORT: local connection unavailable; teleport commands disabled.\n",stderr);
    if(world_control) {
        original_weather_job=reinterpret_cast<WeatherJob>(0x10361ea0cULL+slide);
        if(install_data_hook(0x108288ac8ULL+slide,0x10361ea0cULL+slide,reinterpret_cast<std::uintptr_t>(&weather_hook)))
            std::fputs("CW_MAC_WEATHER: verified producer connected; waiting for explicit app requests.\n",stderr);
        else std::fputs("CW_MAC_WEATHER: producer hook unavailable; weather controls disabled.\n",stderr);
    }
}
}
