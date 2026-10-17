#pragma once
#include "marker_layout.h"
#include "teleport_native.h"
#include <cmath>
#include <cstring>
#include <limits>

namespace crimson::marketplace::teleport {
using CreateNavigationContext=NavigationContext*(__fastcall*)(NavigationContext*,void*,void*,std::uint32_t,std::uint16_t,float);
using CopyCollisionFilter=void*(__fastcall*)(void*,const void*);
using DestroyCollisionFilter=void(__fastcall*)(void*);
using CastGroundRay=bool(__fastcall*)(NavigationContext*,GroundRayQuery*,GroundRayResult*);
using CreatePlacementQuery=PlacementQuery*(__fastcall*)(PlacementQuery*,const WorldPosition*);
// The fourth argument is a nullable native result pointer, not a bool: the
// original reads/passes the full R9 value. Its normal server caller supplies 0.
using FindPlacement=bool(__fastcall*)(NavigationContext*,PlacementQuery*,PlacementResult*,void*);
using ReleaseNativeValue=void(__fastcall*)(void*);
using FreeNativeAllocation=void(*)(std::uintptr_t,void*);
using IsIdentitySrt=bool(__fastcall*)(const void*);
using ComposeSrtPosition=CompressedWorldPosition*(__fastcall*)(const void*,CompressedWorldPosition*,const CompressedWorldPosition*);
struct MarkerBindings {
    bool verified{};
    std::uintptr_t base{},misc_type{},navigation_type{};
    const float* layer_height{};
    const void* default_filter{};
    const void* ground_filter{};
    CreateNavigationContext context{};
    CopyCollisionFilter copy_filter{};
    DestroyCollisionFilter destroy_filter{};
    CastGroundRay ray{};
    CreatePlacementQuery placement_query{};
    FindPlacement placement{};
    ReleaseNativeValue release_reference{},release_locked_reference{};
    FreeNativeAllocation free_allocation{};
    IsIdentitySrt identity_srt{};
    ComposeSrtPosition compose_position{};
};
template<class T> inline T marker_load(const std::byte* at) noexcept {
    T value{};std::memcpy(&value,at,sizeof(value));return value;
}
template<class T> inline void marker_store(std::byte* at,const T& value) noexcept {
    std::memcpy(at,&value,sizeof(value));
}
inline bool marker_bindings_ready(const NativeBindings& b,const MarkerBindings& m) noexcept {
    return b.verified&&m.verified&&b.base==m.base&&m.misc_type&&m.navigation_type&&m.layer_height&&m.default_filter&&m.ground_filter&&
        m.context&&m.copy_filter&&m.destroy_filter&&m.ray&&m.placement_query&&m.placement&&
        m.release_reference&&m.release_locked_reference&&m.free_allocation&&m.identity_srt&&m.compose_position;
}
inline MarkerLayer marker_layer(float value,float threshold) noexcept {
    // d91ef0 is the producer: the ordinary map cursor explicitly supplies Y=0;
    // the alternate map supplies the configured layer threshold + 1. Neither
    // value is a terrain elevation. Icon-derived/other heights stay unknown.
    if(value==0.0f)return MarkerLayer::mainland;
    if(value==threshold+1.0f)return MarkerLayer::alternate;
    return MarkerLayer::unknown;
}
inline bool read_marker_rows(const std::byte* misc,std::size_t offset,std::uint32_t limit,
                             MarkerSource source,float threshold,MarkerSnapshot& out) noexcept {
    const auto* rows=marker_load<const std::byte*>(misc+offset);
    const auto count=marker_load<std::uint32_t>(misc+offset+8);
    const auto capacity=marker_load<std::uint32_t>(misc+offset+12);
    // Native vector growth may reserve beyond its gameplay count limit.
    if(count>limit||capacity<count||capacity>4096||(!rows&&count))return false;
    for(std::uint32_t i=0;i<count;++i){
        const auto* row=rows+static_cast<std::size_t>(i)*0x18;
        MarkerValue value{};value.id=marker_load<std::uint64_t>(row);
        value.position=marker_load<WorldPosition>(row+8);
        value.shape=marker_load<std::uint8_t>(row+0x14);
        value.color=marker_load<std::uint8_t>(row+0x15);
        if(value.id==std::numeric_limits<std::uint64_t>::max()||
           !std::isfinite(value.position.x)||!std::isfinite(value.position.y)||!std::isfinite(value.position.z)||
           value.shape>=14||value.color>=15)return false;
        value.source=source;value.layer=marker_layer(value.position.y,threshold);
        for(std::uint32_t j=0;j<out.count;++j)
            if(out.values[j].source==source&&out.values[j].id==value.id)return false;
        if(out.count>=out.values.size())return false;
        out.values[out.count++]=value;
    }
    return true;
}
inline MarkerCode capture_markers(const NativeBindings& b,const MarkerBindings& m,
                                 NativeFrame& f,MarkerSnapshot& out) {
    out={};
    if(!marker_bindings_ready(b,m))return MarkerCode::unsupported;
    if(!f.ready||!f.acquired||f.faulted||f.attempted)return MarkerCode::context;
    if(f.current.field_definition!=1)return MarkerCode::world;
    void* lock{};bool acquired=false,valid=false;
    __try {
        const auto threshold=*m.layer_height;
        if(!std::isfinite(threshold)||threshold<1.0f||threshold>4096.0f)return MarkerCode::layer;
        if(b.realm()!=1||b.selected(b.base)!=f.identity)return MarkerCode::context;
        auto* actor=static_cast<std::byte*>(f.actor);
        __try {
            b.read_lock(&lock,marker_load<void*>(actor+8));acquired=true;
            if(marker_load<std::uintptr_t>(actor)!=b.actor_type||marker_load<std::uint32_t>(actor+0x60)!=f.identity)
                valid=false;
            else {
                const auto* components=marker_load<const std::byte*>(actor+0x68);
                const auto* misc=components?marker_load<const std::byte*>(components+0x168):nullptr;
                if(!misc||marker_load<std::uintptr_t>(misc)!=m.misc_type||marker_load<void*>(misc+8)!=f.actor)
                    valid=false;
                else valid=read_marker_rows(misc,0xc8,static_cast<std::uint32_t>(kMaximumMapPins),MarkerSource::map_pin,threshold,out)&&
                           read_marker_rows(misc,0xd8,static_cast<std::uint32_t>(kMaximumWaypoints),MarkerSource::waypoint,threshold,out);
            }
        } __finally {
            if(acquired){auto table=*static_cast<void***>(lock);reinterpret_cast<Unlock>(table[0x20/8])(lock,true);}
        }
        if(!valid){out={};return MarkerCode::list;}
        if(b.realm()!=1||b.selected(b.base)!=f.identity){out={};return MarkerCode::context;}
        out.identity=f.identity;out.world=f.world;out.field_definition=f.current.field_definition;out.field_index=f.current.stage;out.layer_height=threshold;
        return MarkerCode::ready;
    } __except(native_fault_filter(GetExceptionCode())){f.faulted=true;out={};return MarkerCode::native_fault;}
}
inline bool marker_navigation_ready(const NativeBindings& b,const MarkerBindings& m,NativeFrame& f) {
    void* lock{};bool acquired=false,valid=false;
    __try {
        __try {
            auto* actor=static_cast<std::byte*>(f.actor);
            b.read_lock(&lock,marker_load<void*>(actor+8));acquired=true;
            const auto count=marker_load<std::uint32_t>(actor+0x70);
            const auto* components=marker_load<const std::byte*>(actor+0x68);
            // Navigation is component index 0x35, constructed/registered at
            // 2a7e323..2a7e35e. Count 53 only covers transform (index 0x34).
            const auto* navigation=components&&count>=54&&count<=57?marker_load<const std::byte*>(components+0x1a8):nullptr;
            valid=navigation&&marker_load<std::uintptr_t>(navigation)==m.navigation_type&&
                marker_load<void*>(navigation+8)==f.actor&&marker_load<void*>(navigation+0x28);
        } __finally {
            if(acquired){auto table=*static_cast<void***>(lock);reinterpret_cast<Unlock>(table[0x20/8])(lock,true);}
        }
        return valid;
    } __except(native_fault_filter(GetExceptionCode())){f.faulted=true;return false;}
}
inline void close_navigation_context(const MarkerBindings& m,NavigationContext& context) {
    // Exact normal destruction in 21a1060/29181b0: a plain reference at +28,
    // followed by the two locked references. +0 is borrowed, not released.
    m.release_reference(context.bytes.data()+0x28);
    m.release_locked_reference(context.bytes.data()+0x10);m.release_reference(context.bytes.data()+0x10);
    m.release_locked_reference(context.bytes.data()+8);m.release_reference(context.bytes.data()+8);
}
struct GroundRayResources {
    const MarkerBindings& bindings;NativeFrame& frame;
    GroundRayQuery query{};GroundRayResult hit{};
    bool filter_ready{},ray_returned{};
    void finish(){
        if(frame.faulted)return;
        try{
            if(ray_returned&&hit.crossings){auto p=hit.crossings;hit.crossings=nullptr;bindings.free_allocation(bindings.base,p);}
            if(filter_ready){filter_ready=false;bindings.destroy_filter(query.bytes.data()+0x40);}
        }catch(...){frame.faulted=true;throw;}
    }
    ~GroundRayResources() noexcept {
        if(frame.faulted)return;
        try{finish();}catch(...){frame.faulted=true;}
    }
};
inline bool cast_marker_ground(const MarkerBindings& m,NativeFrame& frame,NavigationContext& context,
                               const WorldPosition& start,float distance,float& hit_y) {
    GroundRayResources resources{m,frame};auto& query=resources.query;auto& hit=resources.hit;
    // Same query construction as the selected ServerTransform down-ray at
    // 21a1060, with only its origin and bounded length changed.
    m.copy_filter(query.bytes.data()+0x40,m.default_filter);resources.filter_ready=true;
    marker_store(query.bytes.data(),start);
    marker_store(query.bytes.data()+0xc,WorldPosition{0.0f,-1.0f,0.0f});
    marker_store(query.bytes.data()+0x18,distance);
    marker_store(query.bytes.data()+0x1c,std::uint32_t{3});
    marker_store(query.bytes.data()+0x20,m.ground_filter);
    marker_store(query.bytes.data()+0x28,std::uint16_t{1});
    const auto found=m.ray(&context,&query,&hit);resources.ray_returned=true;
    const auto measured=hit.distance;
    // The result owns a native vector of value-only crossing records. This is
    // the original allocator's same-thread free, never CRT free/delete.
    resources.finish();
    if(!found||!std::isfinite(measured)||measured<=0.0f||measured>distance)return false;
    hit_y=start.y-measured;
    return std::isfinite(hit_y);
}
inline bool placement_position(const MarkerBindings& m,const PlacementResult& result,WorldPosition& out) {
    CompressedWorldPosition value{};
    const auto* bytes=result.bytes.data();
    if(m.identity_srt(bytes+0x18))value=marker_load<CompressedWorldPosition>(bytes);
    else m.compose_position(bytes+0x18,&value,reinterpret_cast<const CompressedWorldPosition*>(bytes));
    const auto decoded=decode_world_position(value);
    if(!decoded)return false;
    out=*decoded;return true;
}
struct MarkerQueryResources {
    const MarkerBindings& bindings;NativeFrame& frame;
    NavigationContext context{};PlacementQuery query{};PlacementResult result{};
    bool context_ready{},query_ready{};
    void finish(){
        if(frame.faulted)return;
        try{
            if(query_ready){query_ready=false;bindings.destroy_filter(query.bytes.data()+0x78);}
            if(context_ready){context_ready=false;close_navigation_context(bindings,context);}
        }catch(...){frame.faulted=true;throw;}
    }
    ~MarkerQueryResources() noexcept {
        if(frame.faulted)return;
        try{finish();}catch(...){frame.faulted=true;}
    }
};
inline MarkerCode resolve_marker_core(const NativeBindings& b,const MarkerBindings& m,NativeFrame& f,
                                      std::uint64_t id,MarkerSource source,SavedPosition& destination,
                                      MarkerQueryResources& resources) {
    destination={};MarkerSnapshot snapshot{};
    const auto captured=capture_markers(b,m,f,snapshot);
    if(captured!=MarkerCode::ready)return captured;
    MarkerValue selected{};bool found=false;
    for(std::uint32_t i=0;i<snapshot.count;++i)
        if(snapshot.values[i].id==id&&snapshot.values[i].source==source){selected=snapshot.values[i];found=true;break;}
    if(!found)return MarkerCode::missing;
    if(selected.layer!=MarkerLayer::mainland)return MarkerCode::layer;
    if(!marker_navigation_ready(b,m,f))return f.faulted?MarkerCode::native_fault:MarkerCode::component;
    auto& context=resources.context;auto& query=resources.query;auto& result=resources.result;
    MarkerCode code=MarkerCode::geometry_unavailable;
        // Native creation takes references to this actor's current navigation
        // world. Mode 3 is the native transform's ground/ceiling ray mode.
        m.context(&context,f.actor,nullptr,3,0xffff,0.0f);resources.context_ready=true;
        if(!marker_load<std::uint8_t>(context.bytes.data()+0x12c)||
           marker_load<std::uint32_t>(context.bytes.data()+0x10c)!=3)code=MarkerCode::geometry_unavailable;
        else {
            const auto threshold=*m.layer_height;
            if(threshold!=snapshot.layer_height){resources.finish();return MarkerCode::context;}
            WorldPosition start=selected.position;
            start.y=std::nextafter(threshold,-std::numeric_limits<float>::infinity());
            float height{};
            // This is a search bound, never an invented destination altitude.
            // A miss/unloaded region causes rejection before any movement.
            if(cast_marker_ground(m,f,context,start,4096.0f,height)){
                WorldPosition candidate{start.x,height,start.z};
                m.placement_query(&query,&candidate);resources.query_ready=true;
                // Only the native ground classification is accepted. In
                // particular mode 2's unconditional placement bypass is not
                // usable, and no broad XZ search or fallback is performed.
                marker_store(query.bytes.data()+0x180,std::uint32_t{1});
                if(!m.placement(&context,&query,&result,nullptr))code=MarkerCode::placement_unavailable;
                else {
                    WorldPosition placed{};
                    if(!placement_position(m,result,placed))code=MarkerCode::placement_unavailable;
                    else if(std::fabs(placed.x-candidate.x)>0.5f||std::fabs(placed.z-candidate.z)>0.5f||
                            std::fabs(placed.y-candidate.y)>2.0f||placed.y>=threshold)code=MarkerCode::placement_shifted;
                    else {
                        auto check=placed;check.y+=0.5f;float floor{};
                        if(!cast_marker_ground(m,f,context,check,2.0f,floor)||std::fabs(placed.y-floor)>0.5f)
                            code=MarkerCode::placement_unavailable;
                        else {destination=f.current;destination.position=placed;code=MarkerCode::ready;}
                    }
                }
            }
        }
        resources.finish();
        if(code==MarkerCode::ready){
            MarkerSnapshot fresh{};
            if(capture_markers(b,m,f,fresh)!=MarkerCode::ready||fresh.layer_height!=snapshot.layer_height||
               fresh.world!=snapshot.world||fresh.field_definition!=snapshot.field_definition||fresh.field_index!=snapshot.field_index)
                code=f.faulted?MarkerCode::native_fault:MarkerCode::context;
            else {
                bool matches=false;
                for(std::uint32_t i=0;i<fresh.count;++i)
                    if(fresh.values[i].id==selected.id&&fresh.values[i].source==selected.source){
                        matches=fresh.values[i].position==selected.position&&fresh.values[i].layer==selected.layer;break;
                    }
                if(!matches)code=MarkerCode::context;
            }
        }
        if(code!=MarkerCode::ready)destination={};
        return code;
}
inline MarkerCode resolve_marker_hardware(const NativeBindings& b,const MarkerBindings& m,NativeFrame& f,
                                          std::uint64_t id,MarkerSource source,SavedPosition& destination,
                                          MarkerQueryResources& resources) {
    __try{return resolve_marker_core(b,m,f,id,source,destination,resources);}
    __except(native_fault_filter(GetExceptionCode())){
        // After an uncertain native call, neither cleanup nor movement is
        // guessed. The owning policy latches this frame and stops game calls.
        f.faulted=true;destination={};return MarkerCode::native_fault;
    }
}
inline MarkerCode resolve_marker(const NativeBindings& b,const MarkerBindings& m,NativeFrame& f,
                                 std::uint64_t id,MarkerSource source,SavedPosition& destination) {
    MarkerQueryResources resources{m,f};
    return resolve_marker_hardware(b,m,f,id,source,destination,resources);
}
struct MarkerMoveResult {
    MarkerCode marker{MarkerCode::unsupported};
    NativeCode movement{NativeCode::unsupported};
    SavedPosition destination{};
};
inline MarkerMoveResult begin_marker_move(const NativeBindings& b,const MarkerBindings& m,NativeFrame& f,
                                         std::uint64_t id,MarkerSource source) {
    MarkerMoveResult result{};
    result.marker=resolve_marker(b,m,f,id,source,result.destination);
    if(result.marker==MarkerCode::ready)result.movement=begin_saved_move(b,f,result.destination);
    return result;
}
} // namespace crimson::marketplace::teleport
