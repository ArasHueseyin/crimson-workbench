#pragma once
#include "field_move.h"
#include "../world_read.h"
#include <cstring>

namespace crimson::mac_items {
namespace teleport=crimson::marketplace::teleport;
// Pinned actor guard constructor/destructor, including its native reference bookkeeping.
struct MacActorGuard {std::uintptr_t lock{};std::uint8_t owned{};std::array<std::byte,7> padding{};std::uintptr_t actor{};};
static_assert(sizeof(MacActorGuard)==24);
struct TeleportBindings {
    WorldReadBindings read;
    MacActorGuard (*lock)(void*);
    void (*unlock)(MacActorGuard*);
    float (*yaw)(void*);
    void (*eligibility)(void*,std::int32_t*,std::uint8_t*);
    std::uint32_t (*begin)(void*,const teleport::NativeFieldMoveValue*);
    std::intptr_t slide{};
};
enum class TeleportNative : std::uint32_t {ready,unsupported,task,actor,transform,field,world,stage,position,busy,native_rejection,native_fault,accepted,publish_mismatch};
struct TeleportFrame {
    WorldProbe context;
    teleport::SavedPosition current;
    std::uintptr_t actor{};
    std::uint16_t character{0xffff};
    std::int32_t error{};
    bool ready{};
};
template<class Read>
std::optional<std::uint32_t> teleport_field_definition(std::uint16_t stage,std::intptr_t slide,Read read) {
    std::uintptr_t imported{},table{},count{},slots{},info{},key{};
    if(stage==0xffff||!read(0x107d859e0ULL+slide,imported)||!imported||!read(imported,table)||!table
        ||!read(table+8,count)||stage>=static_cast<std::uint32_t>(count)||!read(table+0x58,slots)||!slots
        ||!read(slots+stage*8,info)||!info||!read(info,key)||!static_cast<std::uint32_t>(key))return {};
    return static_cast<std::uint32_t>(key);
}
template<class Read>
TeleportNative capture_teleport(std::uintptr_t actor,const TeleportBindings& b,Read read,TeleportFrame& out) {
    if(!b.read.acquire||!b.read.release||!b.read.position||!b.lock||!b.unlock||!b.yaw||!b.eligibility)return TeleportNative::unsupported;
    out={};out.actor=actor;out.context=inspect_world_probe(actor,b.slide,read);
    const auto& context=out.context;
    if(!context.context||context.transition||context.field_paused)return TeleportNative::field;
    auto lease=b.read.acquire(reinterpret_cast<void*>(context.transform));
    struct Release {MacFieldLease& lease;const WorldReadBindings& b;~Release(){b.release(&lease);}} release{lease,b.read};
    if(lease.vtable!=b.read.lease_vtable||lease.valid!=1||lease.field!=context.field||lease.owner!=context.transform)return TeleportNative::field;
    const auto key=teleport_field_definition(context.stage,b.slide,read);if(!key)return TeleportNative::stage;
    std::uintptr_t components{},character{},owner{},definition{};
    if(!read(actor+0x108,components)||!components||!read(components+0x20,character)||!character
        ||!read(character+8,owner)||owner!=actor||!read(character+0x30,definition)||(definition&0xffff)==0xffff)return TeleportNative::actor;
    out.character=definition&0xffff;
    std::uint8_t blocked{};b.eligibility(reinterpret_cast<void*>(context.transform),&out.error,&blocked);
    if(out.error||blocked)return TeleportNative::native_rejection;
    {
        auto guard=b.lock(reinterpret_cast<void*>(actor));
        struct Unlock {MacActorGuard& guard;const TeleportBindings& b;~Unlock(){b.unlock(&guard);}} unlock{guard,b};
        if(!guard.lock||guard.owned!=1||guard.actor!=actor)return TeleportNative::actor;
        const auto position=b.read.position(reinterpret_cast<void*>(actor));
        out.current={*key,context.stage,0,{position.x,position.y,position.z},b.yaw(reinterpret_cast<void*>(actor)),1,0};
    }
    if(!teleport::valid_saved_position(out.current))return TeleportNative::position;
    out.ready=true;return TeleportNative::ready;
}
template<class Read,class Allow>
TeleportNative begin_teleport(const TeleportFrame& expected,const teleport::SavedPosition& destination,const TeleportBindings& b,Read read,std::int32_t& error,bool& attempted,Allow allow) {
    attempted=false;
    if(!b.begin||!expected.ready||!teleport::valid_saved_position(destination))return TeleportNative::position;
    if(destination.stage!=expected.current.stage||destination.field_definition!=expected.current.field_definition)return TeleportNative::stage;
    TeleportFrame current;
    const auto code=capture_teleport(expected.actor,b,read,current);error=current.error;
    if(code!=TeleportNative::ready)return code;
    if(current.context.transform!=expected.context.transform||current.context.field!=expected.context.field
        ||current.context.world!=expected.context.world||current.current.field_definition!=expected.current.field_definition
        ||current.current.stage!=expected.current.stage||current.character!=expected.character)return TeleportNative::field;
    auto lease=b.read.acquire(reinterpret_cast<void*>(current.context.transform));
    struct Release {MacFieldLease& lease;const WorldReadBindings& b;~Release(){b.release(&lease);}} release{lease,b.read};
    if(lease.vtable!=b.read.lease_vtable||lease.valid!=1||lease.field!=current.context.field||lease.owner!=current.context.transform)return TeleportNative::field;
    const auto recheck=inspect_world_probe(expected.actor,b.slide,read);
    if(!recheck.context||recheck.transition||recheck.field_paused||recheck.transform!=current.context.transform
        ||recheck.field!=current.context.field||recheck.stage!=current.current.stage||recheck.world!=current.context.world)return TeleportNative::field;
    const auto context=current.context.transform+0x4f0;
    std::uintptr_t phase{},borrows{},pending{};
    if(!read(context+0x58,phase)||(phase&0xff)||!read(context+0x5a,borrows)||!(borrows&0xffff)
        ||!read(context+0x60,pending)||static_cast<std::uint32_t>(pending))return TeleportNative::busy;
    teleport::NativeFieldMoveValue value;value.field_definition=destination.field_definition;value.character_definition=current.character;
    value.position=destination.position;value.yaw=destination.yaw;value.flag_18=1;
    if(!allow())return TeleportNative::task;
    attempted=true;
    error=static_cast<std::int32_t>(b.begin(reinterpret_cast<void*>(context),&value));
    if(error)return TeleportNative::native_rejection;
    teleport::NativeFieldMoveValue stored;std::array<std::uintptr_t,7> words{};
    for(unsigned i=0;i<words.size();++i)if(!read(context+0x60+i*8,words[i]))return TeleportNative::publish_mismatch;
    std::memcpy(&stored,words.data(),sizeof(stored));
    if(!read(context+0x58,phase)||(phase&0xff)!=1||stored.field_definition!=value.field_definition
        ||stored.character_definition!=value.character_definition||stored.position!=value.position||stored.yaw!=value.yaw
        ||stored.flag_18!=1||stored.flag_19||stored.flag_1a||stored.flag_1b||stored.flag_1c||stored.key_20
        ||stored.native_characters||stored.native_character_count||stored.native_character_capacity)return TeleportNative::publish_mismatch;
    return TeleportNative::accepted;
}
}
