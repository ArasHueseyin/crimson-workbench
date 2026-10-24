#pragma once
#include "world_probe.h"
#include <array>
#include <cmath>
#include <cstddef>

namespace crimson::mac_items {
struct MacFieldTime {
    std::uint32_t day{},hour{},minute{},second{},millisecond{};
    std::uint8_t weekday{},reserved{};
    std::uint16_t rate{};
    std::uint64_t anchor{};
};
struct MacFieldLease {
    std::uintptr_t vtable{},field{};
    std::uint8_t valid{},kind{};
    std::array<std::byte,6> padding{};
    std::uintptr_t data{},token{},owner{};
};
struct MacPosition { float x{},y{},z{}; };
static_assert(sizeof(MacFieldTime)==32&&offsetof(MacFieldTime,anchor)==24);
static_assert(sizeof(MacFieldLease)==48&&offsetof(MacFieldLease,owner)==40);
static_assert(sizeof(MacPosition)==12);
struct WorldReadBindings {
    MacFieldLease (*acquire)(void*);
    void (*release)(MacFieldLease*);
    MacFieldTime (*time)(void*);
    MacPosition (*position)(void*);
    std::uint64_t (*now)();
    std::uintptr_t lease_vtable{};
};
struct WorldRead {
    MacFieldTime time{};
    MacPosition position{};
    std::uint64_t native_now{};
    bool ready{};
};
inline WorldRead read_native_world(const WorldProbe& context,std::uintptr_t actor,const WorldReadBindings& b) {
    WorldRead out;
    if(!context.context||context.transition||context.field_paused||!actor
        ||!b.acquire||!b.release||!b.time||!b.position||!b.now) return out;
    // Both aggregate results use Darwin arm64 return conventions: lease/time in X8, position in S0-S2.
    auto lease=b.acquire(reinterpret_cast<void*>(context.transform));
    struct Release { MacFieldLease& lease;const WorldReadBindings& b;~Release(){b.release(&lease);} } release{lease,b};
    if(lease.vtable!=b.lease_vtable||lease.valid!=1||lease.field!=context.field||lease.owner!=context.transform) return out;
    out.time=b.time(reinterpret_cast<void*>(lease.field));
    out.position=b.position(reinterpret_cast<void*>(actor));
    out.native_now=b.now();
    out.ready=out.time.hour<24&&out.time.minute<60&&out.time.second<60&&out.time.millisecond<1000
        &&out.time.weekday<7&&std::isfinite(out.position.x)&&std::isfinite(out.position.y)&&std::isfinite(out.position.z);
    return out;
}
}
