#pragma once
#include "field_time.h"
#include "../../live-items/src/task_observer.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <intrin.h>
#include <cstring>

namespace crimson::marketplace::world_controls {
// Exact native FieldReference produced by ServerTransformSync virtual+0x140.
// It protects a field transition separately from the observer's actor lease.
struct FieldLease {
    std::uintptr_t vtable{};
    void* field{};
    std::uint8_t valid{},kind{};
    std::uint8_t padding[6]{};
    std::uint64_t reference_data{};
    void* field_token{};
    void* transform_owner{};
};
static_assert(sizeof(FieldLease)==0x30 && offsetof(FieldLease,valid)==0x10 && offsetof(FieldLease,transform_owner)==0x28);
struct NearSendTarget {
    std::uintptr_t vtable{};
    std::uint8_t enabled{1};
    std::uint8_t padding[7]{};
    void* actor{};
};
static_assert(sizeof(NearSendTarget)==0x18 && offsetof(NearSendTarget,actor)==0x10);
enum class ClockNativeReason : std::uint32_t {
    ready,unsupported_build,task_context,actor_identity,transform_owner,field_unavailable,
    world_changed,stage_unavailable,stage_fixed,field_clock_stopped,invalid_clock,native_fault,publish_mismatch,published
};
using AcquireField=FieldLease*(__fastcall*)(void*,FieldLease*);
using ReleaseField=void(__fastcall*)(void*);
using ReleaseFieldReference=void(__fastcall*)(FieldLease*);
using GetStageKey=std::uint16_t*(__fastcall*)(void*,std::uint16_t*);
// Historical "stage" names below refer to the native FieldInfo index.
// 4a2010 identifies its table as "fieldinfo" at 5971d78; 4a5580 allocates
// 0x78-byte rows through 1505ab0. This is not the separate StageInfo table.
using GetStageInfo=const std::uint8_t*(__fastcall*)(const std::uint16_t*);
using GetFieldTime=FieldTime*(__fastcall*)(void*,FieldTime*);
using NativeNow=std::uint64_t(__fastcall*)();
using PublishFieldTime=void(__fastcall*)(void*,const FieldTime*);
using SendFieldTime=void(__fastcall*)(void*,const NearSendTarget*,const FieldTime*);
using ReadRealm=std::uint8_t(*)();
struct ClockBindings {
    // Construct only after full EXE identity and bounded routine pins pass.
    bool verified{};
    std::uintptr_t base{};
    std::uintptr_t actor_vtable{},transform_vtable{},field_vtable{},near_target_vtable{};
    const FieldTime* server_clock{};
    AcquireField acquire{};ReleaseField release{};ReleaseFieldReference release_reference{};
    GetStageKey stage_key{};GetStageInfo stage_info{};GetFieldTime current{};NativeNow now{};
    PublishFieldTime publish{};SendFieldTime send{};ReadRealm realm{};
};
// Borrowed only until close_clock_frame. Never place this object in a queue or
// retain it in policy state. Its engine pointers expire on this callback.
struct ClockFrame {
    FieldLease lease{};
    void* actor{};
    void* transform{};
    std::uint32_t identity{},world{};
    std::uint16_t stage{0xffff};
    FieldTime time{};
    FieldTime raw{};
    std::uint64_t native_now{};
    bool acquired{},context_valid{},ready{},faulted{},attempted{};
};
inline std::uint8_t native_realm() noexcept {
    __try {
        auto tls=reinterpret_cast<std::uint8_t**>(__readgsqword(0x58));
        return tls&&tls[0]?tls[0][0x1ec]:255;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return 255;}
}
inline ClockNativeReason open_clock_frame(const ClockBindings& b,const crimson::live_items::TaskObservationV1& task,ClockFrame& frame) noexcept {
    if(frame.acquired||frame.faulted)return ClockNativeReason::native_fault;
    frame={};
    __try {
        if(!b.verified||!b.acquire||!b.release||!b.release_reference||!b.stage_key||!b.stage_info||!b.current||!b.now||!b.publish||!b.send||!b.realm||!b.server_clock)
            return ClockNativeReason::unsupported_build;
        if(task.size!=sizeof(task)||task.abi!=crimson::live_items::kTaskObserverAbiV1||task.base!=b.base||!task.context||!task.native_error||*task.native_error||!task.identity||b.realm()!=1)
            return ClockNativeReason::task_context;
        if(task.return_address!=task.base+0x2774a0d && task.return_address!=task.base+0x27742ae)return ClockNativeReason::task_context;
        auto actor=*static_cast<std::uint8_t**>(task.context);
        if(!actor||*reinterpret_cast<std::uintptr_t*>(actor)!=b.actor_vtable||*reinterpret_cast<std::uint32_t*>(actor+0x60)!=task.identity||!actor[0x96])
            return ClockNativeReason::actor_identity;
        auto components=*reinterpret_cast<std::uint8_t**>(actor+0x68);
        const auto component_count=*reinterpret_cast<std::uint32_t*>(actor+0x70);
        auto possessor=*reinterpret_cast<std::uint8_t**>(actor+0xa0);
        if(component_count<53||component_count>57||!components||!possessor||*reinterpret_cast<void**>(possessor+0xd0)!=actor)return ClockNativeReason::actor_identity;
        auto transform=*reinterpret_cast<std::uint8_t**>(components+0x1a0);
        if(!transform||*reinterpret_cast<std::uintptr_t*>(transform)!=b.transform_vtable||*reinterpret_cast<void**>(transform+8)!=actor||transform[0x3dc])
            return ClockNativeReason::transform_owner;
        auto table=*reinterpret_cast<std::uintptr_t**>(transform);
        if(table[0x140/8]!=reinterpret_cast<std::uintptr_t>(b.acquire)||table[0x148/8]!=reinterpret_cast<std::uintptr_t>(b.release))
            return ClockNativeReason::transform_owner;
        frame.actor=actor;frame.transform=transform;frame.identity=task.identity;
        b.acquire(transform,&frame.lease);frame.acquired=true;
        if(!frame.lease.valid||!frame.lease.field)return ClockNativeReason::field_unavailable;
        auto field=static_cast<std::uint8_t*>(frame.lease.field);
        if(frame.lease.transform_owner!=transform) {frame.faulted=true;return ClockNativeReason::native_fault;}
        if(*reinterpret_cast<std::uintptr_t*>(field)!=b.field_vtable)return ClockNativeReason::transform_owner;
        frame.world=*reinterpret_cast<std::uint32_t*>(field+0x180);
        if(frame.world!=*reinterpret_cast<std::uint32_t*>(possessor+0x60))return ClockNativeReason::world_changed;
        b.stage_key(field,&frame.stage);
        if(frame.stage==0xffff||frame.stage!=*reinterpret_cast<std::uint16_t*>(transform+0x28))return ClockNativeReason::stage_unavailable;
        auto stage=b.stage_info(&frame.stage);if(!stage)return ClockNativeReason::stage_unavailable;
        frame.raw=*b.server_clock;frame.native_now=b.now();
        if(!encode_clock(fields(frame.raw)))return ClockNativeReason::invalid_clock;
        frame.context_valid=true;
        if(stage[0x6a])return ClockNativeReason::stage_fixed;
        if(field[0x17c])return ClockNativeReason::field_clock_stopped;
        b.current(field,&frame.time);frame.native_now=b.now();
        if(!encode_clock(fields(frame.time)))return ClockNativeReason::invalid_clock;
        frame.ready=true;
        return ClockNativeReason::ready;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return ClockNativeReason::native_fault;}
}
inline bool close_clock_frame(const ClockBindings& b,ClockFrame& frame) noexcept {
    if(!frame.acquired)return !frame.faulted;
    // After a native fault the acquisition's partial state is unknown. Do not
    // guess at reference cleanup or retry a command with possible side effects.
    if(frame.faulted)return false;
    __try {
        if(frame.lease.valid)b.release(frame.lease.transform_owner);
        b.release_reference(&frame.lease);frame.acquired=false;frame.ready=false;frame.context_valid=false;
        frame.actor=nullptr;frame.transform=nullptr;frame.lease={};return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return false;}
}
inline ClockNativeReason publish_clock_frame(const ClockBindings& b,ClockFrame& frame,const FieldTime& value) noexcept {
    if(!b.verified||!frame.ready||!frame.acquired||frame.faulted||frame.attempted||!frame.lease.valid||!encode_clock(fields(value)))return ClockNativeReason::invalid_clock;
    // A frame grants at most one mutation attempt, including uncertain writes.
    frame.ready=false;frame.attempted=true;
    __try {
        // This is the narrow native publication used by SetFieldTimeReq. The
        // full request also changes other gameplay state and is not called.
        b.publish(frame.transform,&value);
        const auto stored=reinterpret_cast<const FieldTime*>(static_cast<std::uint8_t*>(frame.transform)+0x450);
        if(std::memcmp(stored,&value,sizeof(value))||std::memcmp(b.server_clock,&value,sizeof(value)))return ClockNativeReason::publish_mismatch;
        const NearSendTarget target{b.near_target_vtable,1,{},frame.actor};
        b.send(frame.actor,&target,&value);
        frame.time=value;
        frame.raw=value;
        // Published means server readback matched and the native Ack sender
        // was entered. Client receipt / in-game acceptance is a later witness.
        return ClockNativeReason::published;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return ClockNativeReason::native_fault;}
}
// Releases only an exactly owned global zero-rate value. Stage-fixed and native
// paused fields may ignore the global, but must not leave our zero rate behind
// for the next normal field. No stage flag or native pause flag is changed.
inline ClockNativeReason release_owned_clock(const ClockBindings& b,ClockFrame& frame,const FieldTime& owned,std::uint16_t saved_rate) noexcept {
    if(!frame.context_valid||!frame.acquired||frame.faulted||frame.attempted||owned.rate||!saved_rate||frame.raw!=owned)
        return ClockNativeReason::invalid_clock;
    const auto value=resume_time(owned,saved_rate,frame.native_now);
    if(!value)return ClockNativeReason::invalid_clock;
    frame.ready=true;return publish_clock_frame(b,frame,*value);
}
inline ClockBindings clock_bindings_2976(std::uintptr_t base,bool pins_verified) {
    ClockBindings b{};if(!base||!pins_verified)return b;
    b.verified=true;b.base=base;b.actor_vtable=base+0x5b1e2b0;b.transform_vtable=base+0x5b2cf60;b.field_vtable=base+0x5b29a60;b.near_target_vtable=base+0x5b25418;
    b.server_clock=reinterpret_cast<const FieldTime*>(base+0x6a6d238);
    b.acquire=reinterpret_cast<AcquireField>(base+0x2bc4280);b.release=reinterpret_cast<ReleaseField>(base+0x2bc42c0);
    b.release_reference=reinterpret_cast<ReleaseFieldReference>(base+0x1434880);
    b.stage_key=reinterpret_cast<GetStageKey>(base+0x2af7d10);b.stage_info=reinterpret_cast<GetStageInfo>(base+0x4a2010);
    b.current=reinterpret_cast<GetFieldTime>(base+0x20e55a0);b.now=reinterpret_cast<NativeNow>(base+0x1416b80);
    b.publish=reinterpret_cast<PublishFieldTime>(base+0x21a1cc0);b.send=reinterpret_cast<SendFieldTime>(base+0x299aed0);b.realm=native_realm;
    return b;
}
} // namespace crimson::marketplace::world_controls
