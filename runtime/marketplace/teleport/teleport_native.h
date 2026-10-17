#pragma once
#include "native_layout.h"
#include "../../live-items/src/task_observer.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <intrin.h>
#include <cstring>

namespace crimson::marketplace::teleport {
struct FieldLease {
    std::uintptr_t vtable{};void* field{};
    std::uint8_t valid{},kind{};std::array<std::byte,6> padding{};
    std::uint64_t reference_data{};void* field_token{};void* transform_owner{};
};
static_assert(sizeof(FieldLease)==0x30 && offsetof(FieldLease,transform_owner)==0x28);
using AcquireField=FieldLease*(__fastcall*)(void*,FieldLease*);
using ReleaseField=void(__fastcall*)(void*);
using ReleaseReference=void(__fastcall*)(FieldLease*);
using GetStage=std::uint16_t*(__fastcall*)(void*,std::uint16_t*);
using GetStageInfo=const std::byte*(__fastcall*)(const std::uint16_t*);
using GetPosition=CompressedWorldPosition*(__fastcall*)(void*,CompressedWorldPosition*);
using GetYaw=float(__fastcall*)(void*);
using ReadLock=void**(__fastcall*)(void**,void*);
using Unlock=void(__fastcall*)(void*,bool);
using ValidateMove=int*(__fastcall*)(void*,int*);
using BeginFieldMove=int*(__fastcall*)(void*,int*,const NativeFieldMoveValue*);
using ReadRealm=std::uint8_t(*)();
using SelectedIdentity=std::uint32_t(*)(std::uintptr_t);
struct NativeBindings {
    // A module factory may populate these only after full file hash, image
    // identity, entry pins and the current-build virtual slots are verified.
    bool verified{};std::uintptr_t base{},actor_type{},transform_type{},field_type{};
    AcquireField acquire{};ReleaseField release{};ReleaseReference release_reference{};
    GetStage stage{};GetStageInfo stage_info{};GetPosition position{};GetYaw yaw{};
    ReadLock read_lock{};ValidateMove validate{};BeginFieldMove begin{};ReadRealm realm{};SelectedIdentity selected{};
};
enum class NativeCode : std::uint32_t {
    ready,unsupported,task,actor,transform,field,world,stage,position,busy,native_rejection,native_fault,accepted,publish_mismatch
};
struct NativeFrame {
    FieldLease lease{};void* actor{};void* transform{};
    std::uint32_t identity{},world{};std::uint16_t character{};
    SavedPosition current{};int native_error{};
    bool acquired{},ready{},attempted{},faulted{};
};
// MSVC implements C++ exceptions through SEH as well. Preserve their normal
// unwinding; only other native SEH faults become fail-closed status values.
inline int native_fault_filter(unsigned long code) noexcept {
    return code==0xe06d7363UL?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;
}
inline std::uint8_t native_realm() noexcept {
    __try {auto tls=reinterpret_cast<std::uint8_t**>(__readgsqword(0x58));return tls&&tls[0]?tls[0][0x1ec]:255;}
    __except(native_fault_filter(GetExceptionCode())){return 255;}
}
inline void acquire_snapshot_lock(const NativeBindings& b,void** lock,void* node,NativeFrame& frame) {
    try {b.read_lock(lock,node);}
    catch(...){frame.faulted=true;throw;} // Unknown partial lock acquisition.
}
inline void release_snapshot_lock(void* lock,NativeFrame& frame) {
    try {
        auto table=*static_cast<void***>(lock);
        reinterpret_cast<Unlock>(table[0x20/8])(lock,true);
    } catch(...){frame.faulted=true;throw;} // Never retry uncertain cleanup.
}
inline bool capture_transform_value(const NativeBindings& b,std::uint8_t* actor,std::uint8_t* transform,std::array<std::byte,0x110>& value,NativeFrame& frame) {
    void* lock{};bool acquired=false;
    __try {
        __try {
            acquire_snapshot_lock(b,&lock,*reinterpret_cast<void**>(actor+8),frame);acquired=true;
            // 4f8220 and 17e9170 consume only the value members through +10f.
            // Copy them while locked; perform their parent/SRT arithmetic on
            // this private value after unlocking, not on a borrowed component.
            for(std::size_t i=0;i<value.size();++i)value[i]=static_cast<std::byte>(*reinterpret_cast<volatile std::uint8_t*>(transform+i));
        } __finally {
            if(acquired){
                release_snapshot_lock(lock,frame);
            }
        }
        return true;
    } __except(native_fault_filter(GetExceptionCode())){return false;}
}
inline void acquire_native_field(const NativeBindings& b,void* transform,NativeFrame& frame) {
    try {b.acquire(transform,&frame.lease);}
    catch(...){frame.faulted=true;throw;} // No completed acquisition to release.
    frame.acquired=true;
}
inline NativeCode open_frame(const NativeBindings& b,const crimson::live_items::TaskObservationV1& task,NativeFrame& f) {
    if(f.acquired||f.faulted)return NativeCode::native_fault;
    f={};
    __try {
        if(!b.verified||!b.base||!b.acquire||!b.release||!b.release_reference||!b.stage||!b.stage_info||!b.position||!b.yaw||!b.read_lock||!b.validate||!b.begin||!b.realm||!b.selected)return NativeCode::unsupported;
        if(task.size!=sizeof(task)||task.abi!=crimson::live_items::kTaskObserverAbiV1||task.base!=b.base||!task.identity||task.reserved||!task.context||!task.native_error||*task.native_error||b.realm()!=1||
            (task.return_address!=b.base+0x2774a0d&&task.return_address!=b.base+0x27742ae))return NativeCode::task;
        if(b.selected(b.base)!=task.identity)return NativeCode::actor;
        auto actor=*static_cast<std::uint8_t**>(task.context);
        if(!actor||*reinterpret_cast<std::uintptr_t*>(actor)!=b.actor_type||*reinterpret_cast<std::uint32_t*>(actor+0x60)!=task.identity||actor[0x96]!=1)return NativeCode::actor;
        auto metadata=*reinterpret_cast<std::uint8_t**>(actor+0x88);
        auto components=*reinterpret_cast<std::uint8_t**>(actor+0x68);
        const auto count=*reinterpret_cast<std::uint32_t*>(actor+0x70);
        auto possessor=*reinterpret_cast<std::uint8_t**>(actor+0xa0);
        if(!metadata||(metadata[1]!=1&&metadata[1]!=4)||!components||count<53||count>57||!possessor||*reinterpret_cast<void**>(possessor+0xd0)!=actor)return NativeCode::actor;
        auto transform=*reinterpret_cast<std::uint8_t**>(components+0x1a0);
        if(!transform||*reinterpret_cast<std::uintptr_t*>(transform)!=b.transform_type||*reinterpret_cast<void**>(transform+8)!=actor||transform[0x3dc])return NativeCode::transform;
        auto table=*reinterpret_cast<std::uintptr_t**>(transform);
        if(table[0x140/8]!=reinterpret_cast<std::uintptr_t>(b.acquire)||table[0x148/8]!=reinterpret_cast<std::uintptr_t>(b.release))return NativeCode::transform;
        auto character=*reinterpret_cast<std::uint8_t**>(components+0x20);
        if(!character||*reinterpret_cast<void**>(character+8)!=actor)return NativeCode::actor;
        f.character=*reinterpret_cast<std::uint16_t*>(character+0x30);
        if(f.character==0xffff)return NativeCode::actor;
        f.actor=actor;f.transform=transform;f.identity=task.identity;
        acquire_native_field(b,transform,f);
        if(!f.lease.valid||!f.lease.field)return NativeCode::field;
        if(f.lease.transform_owner!=transform){f.faulted=true;return NativeCode::native_fault;}
        auto field=static_cast<std::uint8_t*>(f.lease.field);
        if(*reinterpret_cast<std::uintptr_t*>(field)!=b.field_type)return NativeCode::field;
        f.world=*reinterpret_cast<std::uint32_t*>(field+0x180);
        if(f.world!=*reinterpret_cast<std::uint32_t*>(possessor+0x60))return NativeCode::world;
        b.stage(field,&f.current.stage);
        if(f.current.stage==0xffff||f.current.stage!=*reinterpret_cast<std::uint16_t*>(transform+0x28))return NativeCode::stage;
        const auto stage=b.stage_info(&f.current.stage);if(!stage)return NativeCode::stage;
        f.current.field_definition=*reinterpret_cast<const std::uint32_t*>(stage);
        int result{};b.validate(transform,&result);f.native_error=result;
        if(result)return NativeCode::native_rejection;
        // The current actor is already borrowed by LiveItems. The native field
        // lease additionally prevents a field transition until this frame ends.
        // Their input values must be captured in the actor's native read lock.
        std::array<std::byte,0x110> transform_value{};
        if(!capture_transform_value(b,actor,transform,transform_value,f)){f.faulted=true;return NativeCode::native_fault;}
        CompressedWorldPosition compressed{};b.position(transform_value.data(),&compressed);
        const auto decoded=decode_world_position(compressed);f.current.yaw=b.yaw(transform_value.data());
        if(!decoded)return NativeCode::position;
        f.current.position=*decoded;
        if(!valid_saved_position(f.current))return NativeCode::position;
        // No address from this frame is valid beyond close_frame.
        f.ready=true;return NativeCode::ready;
    } __except(native_fault_filter(GetExceptionCode())){f.faulted=true;return NativeCode::native_fault;}
}
inline void invoke_native_begin(const NativeBindings& b,NativeFrame& frame,void* context,int* error,const NativeFieldMoveValue* value) {
    try {b.begin(context,error,value);}
    catch(...){frame.faulted=true;throw;} // Mutation may already have occurred.
}
inline NativeCode begin_saved_move(const NativeBindings& b,NativeFrame& f,const SavedPosition& destination) {
    if(!b.verified||!f.ready||!f.acquired||f.faulted||f.attempted||!f.lease.valid||!valid_saved_position(destination))return NativeCode::position;
    // Until the field lookup's persistent identity is proven independently,
    // only positions captured in this same native stage/field are eligible.
    // This still uses native field streaming for distant places in that field.
    if(destination.field_definition!=f.current.field_definition||destination.stage!=f.current.stage)return NativeCode::stage;
    __try {
        auto* actor=static_cast<std::uint8_t*>(f.actor);
        auto* transform=static_cast<std::uint8_t*>(f.transform);
        auto* field=static_cast<std::uint8_t*>(f.lease.field);
        if(b.realm()!=1||b.selected(b.base)!=f.identity||!actor||!transform||!field||
            *reinterpret_cast<std::uintptr_t*>(actor)!=b.actor_type||*reinterpret_cast<std::uint32_t*>(actor+0x60)!=f.identity||actor[0x96]!=1||
            *reinterpret_cast<void**>(transform+8)!=actor||*reinterpret_cast<std::uintptr_t*>(transform)!=b.transform_type||transform[0x3dc]||
            f.lease.transform_owner!=transform||*reinterpret_cast<std::uintptr_t*>(field)!=b.field_type||*reinterpret_cast<std::uint32_t*>(field+0x180)!=f.world)return NativeCode::actor;
        auto* components=*reinterpret_cast<std::uint8_t**>(actor+0x68);
        auto* possessor=*reinterpret_cast<std::uint8_t**>(actor+0xa0);
        if(!components||*reinterpret_cast<void**>(components+0x1a0)!=transform||!possessor||*reinterpret_cast<void**>(possessor+0xd0)!=actor||*reinterpret_cast<std::uint32_t*>(possessor+0x60)!=f.world)return NativeCode::actor;
        auto* character=*reinterpret_cast<std::uint8_t**>(components+0x20);
        if(!character||*reinterpret_cast<void**>(character+8)!=actor||*reinterpret_cast<std::uint16_t*>(character+0x30)!=f.character)return NativeCode::actor;
        std::uint16_t stage=0xffff;b.stage(field,&stage);
        if(stage!=f.current.stage||stage!=*reinterpret_cast<std::uint16_t*>(transform+0x28))return NativeCode::stage;
        auto* context=transform+0x4f0;
        // Native acquire increments this counter. Begin must defer the field
        // transition while we still inspect its accepted descriptor. Do not
        // bypass an already active transition or another pending native value.
        if(context[0x58]||*reinterpret_cast<const std::int16_t*>(context+0x5a)<=0||*reinterpret_cast<const std::uint32_t*>(context+0x60))return NativeCode::busy;
        int error{};b.validate(transform,&error);f.native_error=error;if(error)return NativeCode::native_rejection;
        NativeFieldMoveValue value{};value.field_definition=destination.field_definition;value.character_definition=f.character;
        value.position=destination.position;value.yaw=destination.yaw;value.flag_18=1;
        // One mutation attempt. The native function owns deep-copying its
        // typed descriptor and starting its field transition after release.
        f.ready=false;f.attempted=true;error=-1;invoke_native_begin(b,f,context,&error,&value);f.native_error=error;
        if(error)return NativeCode::native_rejection;
        const auto& stored=*reinterpret_cast<const NativeFieldMoveValue*>(context+0x60);
        if(context[0x58]!=1||stored.field_definition!=value.field_definition||stored.character_definition!=value.character_definition||
            stored.position!=value.position||stored.yaw!=value.yaw||stored.flag_18!=1||stored.flag_19||stored.flag_1a||stored.flag_1b||stored.flag_1c||stored.key_20||
            stored.native_characters||stored.native_character_count||stored.native_character_capacity)return NativeCode::publish_mismatch;
        // This acknowledges the queued native value only. It does not claim
        // that the client has loaded the destination or the character arrived.
        return NativeCode::accepted;
    } __except(native_fault_filter(GetExceptionCode())){f.faulted=true;return NativeCode::native_fault;}
}
inline void release_native_field(const NativeBindings& b,NativeFrame& frame) {
    try {
        if(frame.lease.valid)b.release(frame.lease.transform_owner);
        b.release_reference(&frame.lease);
    } catch(...){frame.faulted=true;throw;} // No speculative second release.
}
// Callers must create an unwind guard before open_frame. On a C++ exception,
// it may close a fully acquired !attempted && !faulted frame and then preserve
// the exception. After an uncertain acquire/Begin/cleanup, latch and do not
// retry engine operations. A secondary unwind-cleanup exception must not hide
// the primary exception or be represented as successful cleanup.
inline bool close_frame(const NativeBindings& b,NativeFrame& f) {
    if(!f.acquired)return !f.faulted;
    if(f.faulted)return false;
    __try {
        release_native_field(b,f);f.acquired=false;f.ready=false;f.actor=nullptr;f.transform=nullptr;f.lease={};return true;
    } __except(native_fault_filter(GetExceptionCode())){f.faulted=true;return false;}
}
} // namespace crimson::marketplace::teleport
