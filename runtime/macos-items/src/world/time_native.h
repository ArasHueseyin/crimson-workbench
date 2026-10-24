#pragma once
#include "time_control.h"
#include "../world_read.h"
#include <cstring>

namespace crimson::mac_items {
template<class Read>
std::optional<MacFieldTime> read_time_value(std::uintptr_t address,Read read) {
    if(!address||address>std::numeric_limits<std::uintptr_t>::max()-24)return {};
    std::uintptr_t words[4]{};
    for(unsigned i=0;i<4;++i)if(!read(address+i*8,words[i]))return {};
    MacFieldTime value;std::memcpy(&value,words,sizeof(value));return value;
}
template<class Read>
std::optional<bool> time_stage_fixed(std::uint16_t stage,std::intptr_t slide,Read read) {
    std::uintptr_t imported{},table{},count{},slots{},info{},flag{};
    if(stage==0xffff||!read(0x107d859e0ULL+slide,imported)||!imported||!read(imported,table)||!table
        ||table>std::numeric_limits<std::uintptr_t>::max()-0x58||!read(table+8,count)||stage>=static_cast<std::uint32_t>(count)
        ||!read(table+0x58,slots)||!slots||slots>std::numeric_limits<std::uintptr_t>::max()-stage*8
        ||!read(slots+stage*8,info)||!info||info>std::numeric_limits<std::uintptr_t>::max()-0x6a||!read(info+0x6a,flag))return {};
    if((flag&0xff)>1)return {};
    return (flag&0xff)!=0;
}
struct MacNearTarget {
    std::uintptr_t vtable{};
    std::uint8_t enabled{1};
    std::array<std::byte,7> padding{};
    std::uintptr_t actor{};
};
static_assert(sizeof(MacNearTarget)==24&&offsetof(MacNearTarget,actor)==16);
struct TimeWriteBindings {
    WorldReadBindings read;
    void (*publish)(void*,const MacFieldTime*);
    void (*send)(void*,const MacNearTarget*,const MacFieldTime*);
    std::uintptr_t target_vtable{};
    std::intptr_t slide{};
};
template<class Read>
TimePublication publish_native_time(const WorldProbe& expected,std::uintptr_t actor,const MacFieldTime& raw,
    const MacFieldTime& value,const TimeWriteBindings& b,Read read) {
    if(!b.publish||!b.send||!b.target_vtable||!b.read.acquire||!b.read.release||!world::encode_clock(world::fields(value)))return TimePublication::rejected;
    const auto current=inspect_world_probe(actor,b.slide,read);
    const auto fixed=time_stage_fixed(current.stage,b.slide,read);
    if(!current.context||current.transition||current.field_paused||!fixed||*fixed
        ||current.transform!=expected.transform||current.field!=expected.field
        ||current.world!=expected.world||current.stage!=expected.stage)return TimePublication::rejected;
    auto lease=b.read.acquire(reinterpret_cast<void*>(current.transform));
    struct Release { MacFieldLease& lease;const WorldReadBindings& b;~Release(){b.release(&lease);} } release{lease,b.read};
    if(lease.vtable!=b.read.lease_vtable||lease.valid!=1||lease.field!=current.field||lease.owner!=current.transform)return TimePublication::rejected;
    const auto clock=read_time_value(0x108a6ab00ULL+b.slide,read);
    if(!clock||*clock!=raw)return TimePublication::rejected;
    b.publish(reinterpret_cast<void*>(current.transform),&value);
    const auto global=read_time_value(0x108a6ab00ULL+b.slide,read);
    const auto stored=read_time_value(current.transform+0x450,read);
    if(!global||!stored||*global!=value||*stored!=value)return TimePublication::uncertain;
    // Matches constructor 100c2f364; destructor 100c2f104 only changes the stack vtable.
    const MacNearTarget target{b.target_vtable,1,{},actor};
    b.send(reinterpret_cast<void*>(actor),&target,&value);
    return TimePublication::published;
}
}
