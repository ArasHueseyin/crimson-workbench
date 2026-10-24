#pragma once
#include "world_probe.h"
#include <bit>
#include <cmath>

namespace crimson::mac_items {
struct WeatherRead {
    std::uintptr_t resource{},constant{},atmosphere{};
    float precipitation{},cloudiness{},humidity{},wind{},fog{},cloud{},aerosol{},phase{};
    bool ready{},manual{};
};
template<class Read>
WeatherRead read_weather(const WorldProbe& context,std::intptr_t slide,Read read) {
    WeatherRead out;
    if(!context.weather_read)return out;
    auto field=[&](std::uintptr_t p,std::uintptr_t offset,std::uintptr_t& value) {
        return p&&p<=std::numeric_limits<std::uintptr_t>::max()-offset&&read(p+offset,value);
    };
    std::uintptr_t interface{},type{},owner{},manual{};
    if(!field(context.weather,0x60,interface)||interface<0x28)return out;
    out.resource=interface-0x28;
    if(!field(out.resource,0,type)||type!=0x1081ca110ULL+slide
        ||!field(out.resource,0x40,interface)||interface<0x28)return out;
    out.constant=interface-0x28;
    if(!field(out.constant,0,type)||type!=0x1081ca690ULL+slide
        ||!field(out.constant,0x10,owner)||owner!=out.resource
        ||!field(out.resource,0x48,interface)||interface<0x28)return out;
    out.atmosphere=interface-0x28;
    if(!field(out.atmosphere,0,type)||type!=0x1081c9e40ULL+slide
        ||!field(out.atmosphere,0x10,owner)||owner!=out.resource
        ||!field(context.weather,0x31,manual)||(manual&0xff)>1)return out;
    auto scalar=[&](std::uintptr_t p,std::uintptr_t offset,float& value) {
        std::uintptr_t word{};
        if(!field(p,offset,word))return false;
        value=std::bit_cast<float>(static_cast<std::uint32_t>(word));
        return std::isfinite(value);
    };
    out.manual=(manual&0xff)!=0;
    out.ready=scalar(out.constant,0x154,out.precipitation)&&scalar(out.constant,0x158,out.cloudiness)
        &&scalar(out.constant,0x15c,out.humidity)&&scalar(out.constant,0x160,out.wind)
        &&scalar(out.constant,0x1c0,out.phase)&&scalar(out.atmosphere,0xc4,out.fog)
        &&scalar(out.atmosphere,0xd0,out.cloud)&&scalar(out.atmosphere,0xb0,out.aerosol);
    return out;
}
}
