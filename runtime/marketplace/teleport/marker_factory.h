#pragma once
#include "marker_native.h"

namespace crimson::marketplace::teleport {
inline void free_marker_crossings(std::uintptr_t base,void* allocation) {
    // Exact normal native vector destruction (21a1060/1f04bd0). This flag is
    // the engine allocator's per-thread tracking choice, not a realm switch.
    auto tls=reinterpret_cast<std::uint8_t**>(__readgsqword(0x58));
    if(!tls||!tls[0])RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    const auto target=base+(tls[0][0x1fd]?0x47f03bc:0x47f02bc);
    reinterpret_cast<void(__fastcall*)(void*)>(target)(allocation);
}
// Called only after the full original EXE hash, current image identity, and
// both saved-path and marker function/table pins pass in the module factory.
inline MarkerBindings supported_marker_bindings(const NativeBindings& b,bool marker_pins_verified) {
    MarkerBindings m{};
    if(!b.verified||!b.base||!marker_pins_verified)return m;
    const auto base=b.base;m.verified=true;m.base=base;m.misc_type=base+0x5b1d318;m.navigation_type=base+0x5b1f2a8;
    m.layer_height=reinterpret_cast<const float*>(base+0x6cde048);
    m.default_filter=reinterpret_cast<const void*>(base+0x6cecaa0);
    m.ground_filter=reinterpret_cast<const void*>(base+0x6cec7d0);
    m.context=reinterpret_cast<CreateNavigationContext>(base+0x1eebd20);
    m.copy_filter=reinterpret_cast<CopyCollisionFilter>(base+0x39ae00);
    m.destroy_filter=reinterpret_cast<DestroyCollisionFilter>(base+0x360470);
    m.ray=reinterpret_cast<CastGroundRay>(base+0x1f04bd0);
    m.placement_query=reinterpret_cast<CreatePlacementQuery>(base+0x1fe25c0);
    m.placement=reinterpret_cast<FindPlacement>(base+0x1efe790);
    m.release_reference=reinterpret_cast<ReleaseNativeValue>(base+0x38ad90);
    m.release_locked_reference=reinterpret_cast<ReleaseNativeValue>(base+0x20416b0);
    m.free_allocation=free_marker_crossings;
    m.identity_srt=reinterpret_cast<IsIdentitySrt>(base+0x3933a0);
    m.compose_position=reinterpret_cast<ComposeSrtPosition>(base+0x50ea80);
    return m;
}
} // namespace crimson::marketplace::teleport
