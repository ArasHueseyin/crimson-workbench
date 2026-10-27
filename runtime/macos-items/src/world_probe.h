#pragma once
#include <cstdint>
#include <limits>

namespace crimson::mac_items {
struct WorldProbe {
    std::uintptr_t transform{}, field{}, weather{}, weather_world{};
    std::uint32_t world{};
    std::uint16_t stage{0xffff}, field_stage{0xffff};
    std::uint8_t transition{}, field_paused{};
    std::uint64_t clock[4]{};
    unsigned weather_parent_offset{}, weather_child_offset{};
    bool context{}, clock_read{}, weather_read{};
};
template<class Read>
WorldProbe inspect_active_weather(std::intptr_t slide,Read read) {
    WorldProbe out;
    auto field=[&](std::uintptr_t object,std::uintptr_t offset,std::uintptr_t& value) {
        return object&&object<=std::numeric_limits<std::uintptr_t>::max()-offset&&read(object+offset,value);
    };
    std::uintptr_t service{},parent{},candidate{},manager{},type{},back{};
    // Captured on the pinned Mac build: service -> +40 -> +08 -> active world.
    if(read(0x108ce55f0ULL+slide,service)&&field(service,0x40,parent)
        &&field(parent,8,candidate)&&field(candidate,0xf68,manager)&&field(manager,0,type)
        &&type==0x1081c3b98ULL+slide&&field(manager,0xd0,back)&&back==candidate) {
        out.weather=manager;out.weather_world=candidate;out.weather_parent_offset=0x40;
        out.weather_child_offset=8;out.weather_read=true;
    }
    return out;
}
template<class Read>
WorldProbe inspect_world_probe(std::uintptr_t actor, std::intptr_t slide, Read read) {
    WorldProbe out;
    auto field=[&](std::uintptr_t object,std::uintptr_t offset,std::uintptr_t& value) {
        return object && object<=std::numeric_limits<std::uintptr_t>::max()-offset && read(object+offset,value);
    };
    std::uintptr_t table{},components{},owner{},user{},world{},stage{},definition{},state{},pause{};
    if(!field(actor,0,table)||table!=0x107e2fe10ULL+slide
        ||!field(actor,0x108,components)||!field(components,0x1a0,out.transform)
        ||!field(out.transform,0,table)||table!=0x107e33228ULL+slide
        ||!field(out.transform,8,owner)||owner!=actor
        ||!field(out.transform,0x500,out.field)||!field(out.field,0,table)||table!=0x107e30b10ULL+slide
        ||!field(actor,0x140,user)||!field(user,0x100,world)
        ||!field(out.field,0x220,state)||static_cast<std::uint32_t>(state)!=static_cast<std::uint32_t>(world)
        ||!field(out.transform,0x28,stage)||!field(out.field,0x4b8,definition)||!field(definition,4,state)
        ||static_cast<std::uint16_t>(stage)!=static_cast<std::uint16_t>(state)
        ||!field(out.transform,0x548,state)||!field(out.field,0x21c,pause)) return out;
    out.world=static_cast<std::uint32_t>(world);out.stage=static_cast<std::uint16_t>(stage);out.field_stage=out.stage;
    out.transition=static_cast<std::uint8_t>(state);out.field_paused=static_cast<std::uint8_t>(pause);out.context=true;
    out.clock_read=true;
    for(unsigned i=0;i<4;++i) {
        std::uintptr_t value{};
        if(!read(0x108a6ab00ULL+slide+8*i,value))out.clock_read=false;
        out.clock[i]=value;
    }
    const auto weather=inspect_active_weather(slide,read);
    out.weather=weather.weather;out.weather_world=weather.weather_world;
    out.weather_parent_offset=weather.weather_parent_offset;out.weather_child_offset=weather.weather_child_offset;
    out.weather_read=weather.weather_read;
    return out;
}
}
