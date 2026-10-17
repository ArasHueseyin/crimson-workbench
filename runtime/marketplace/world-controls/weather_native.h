#pragma once
#include "control_protocol.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <cstring>

namespace crimson::marketplace::world_controls {
enum class WeatherNativeReason : std::uint32_t {
    ready,unsupported_build,producer_context,current_manager,result_owner,invalid_value,
    native_fault,publish_mismatch,published,client_field
};
using SetWeatherBool=void(__fastcall*)(void*,const std::uint8_t*);
using SetWeatherFloat=void(__fastcall*)(void*,const float*);
using DerivePrecipitation=void(__fastcall*)(void*);
struct WeatherBindings {
    bool verified{};std::uintptr_t base{},manager_vtable{},resource_vtable{},constant_vtable{},atmosphere_vtable{},client_field_vtable{},world_vtable{};
    void* const* service{};
    SetWeatherBool manual{};
    SetWeatherFloat precipitation{},cloudiness{},humidity{},wind{},fog{},cloud_density{};
    DerivePrecipitation derive{};
};
// WeatherConstant primary+0x154..0x1d3 contains scalar values / byte flags only.
// Constructor 0x13ca2801..28d0 initializes this tail; texture references end
// before it. Padding after primary+184/+1a4 is excluded from read and restore.
struct WeatherSnapshot {
    std::array<std::uint8_t,0x80> scalar{};
    float aerosol{},fog{},cloud{};
    bool operator==(const WeatherSnapshot&) const=default;
};
struct WeatherFrame {
    std::uint8_t* manager{};std::uint8_t* resource{};
    std::uint8_t* constant{};std::uint8_t* atmosphere{};
    std::uint64_t fingerprint{};bool manual{},ready{},faulted{};
};
inline std::uint64_t weather_fingerprint(std::uintptr_t manager,std::uintptr_t resource,std::uint64_t salt) noexcept {
    // Scalar comparison fingerprint only; this is not a cryptographic claim.
    // It is never dereferenced. Fresh pointers are validated on every call.
    auto value=static_cast<std::uint64_t>(manager)^((static_cast<std::uint64_t>(resource)<<17)|(static_cast<std::uint64_t>(resource)>>47))^salt;
    value^=value>>30;value*=0xbf58476d1ce4e5b9ull;value^=value>>27;value*=0x94d049bb133111ebull;return value^(value>>31);
}
inline WeatherNativeReason open_weather_frame(const WeatherBindings& b,void* manager,std::uintptr_t caller,std::uint64_t salt,WeatherFrame& out) noexcept {
    out={};
    __try {
        if(!b.verified||!b.service||!b.manual||!b.precipitation||!b.cloudiness||!b.humidity||!b.wind||!b.fog||!b.cloud_density||!b.derive)return WeatherNativeReason::unsupported_build;
        if(caller!=b.base+0x3b1073c&&caller!=b.base+0x3b4c180)return WeatherNativeReason::producer_context;
        auto value=static_cast<std::uint8_t*>(manager);
        if(!value||*reinterpret_cast<std::uintptr_t*>(value)!=b.manager_vtable)return WeatherNativeReason::producer_context;
        auto service=static_cast<std::uint8_t*>(*b.service);if(!service)return WeatherNativeReason::current_manager;
        auto holder=*reinterpret_cast<std::uint8_t**>(service+0x38);
        if(!holder||*reinterpret_cast<void**>(holder+0x38)!=manager)return WeatherNativeReason::current_manager;
        // Manager+e8 is the constructor's PAWorld argument, not a guessed
        // camera or an inferred critical-section owner.
        auto world=*reinterpret_cast<std::uint8_t**>(value+0xe8);
        if(!world||*reinterpret_cast<std::uintptr_t*>(world)!=b.world_vtable||*reinterpret_cast<void**>(world+0xf68)!=manager)return WeatherNativeReason::current_manager;
        auto result=*reinterpret_cast<std::uint8_t**>(value+0x60);if(!result)return WeatherNativeReason::result_owner;
        auto resource=result-0x28;
        if(*reinterpret_cast<std::uintptr_t*>(resource)!=b.resource_vtable)return WeatherNativeReason::result_owner;
        auto weather=*reinterpret_cast<std::uint8_t**>(resource+0x40);
        auto atmosphere=*reinterpret_cast<std::uint8_t**>(resource+0x48);
        if(!weather||!atmosphere)return WeatherNativeReason::result_owner;
        if(*reinterpret_cast<std::uintptr_t*>(weather-0x28)!=b.constant_vtable||*reinterpret_cast<std::uintptr_t*>(atmosphere-0x28)!=b.atmosphere_vtable||
            *reinterpret_cast<void**>(weather-0x18)!=resource||*reinterpret_cast<void**>(atmosphere-0x18)!=resource)return WeatherNativeReason::result_owner;
        if(value[0x31]>1)return WeatherNativeReason::invalid_value;
        out.manager=value;out.resource=resource;out.constant=weather;out.atmosphere=atmosphere;out.manual=value[0x31]!=0;
        out.fingerprint=weather_fingerprint(reinterpret_cast<std::uintptr_t>(manager),reinterpret_cast<std::uintptr_t>(resource),salt);out.ready=true;
        return WeatherNativeReason::ready;
    } __except(EXCEPTION_EXECUTE_HANDLER) {out.faulted=true;return WeatherNativeReason::native_fault;}
}
inline WeatherNativeReason read_weather(const WeatherFrame& frame,WeatherSnapshot& snapshot,WeatherParameters& params) noexcept {
    if(!frame.ready||frame.faulted)return WeatherNativeReason::producer_context;
    __try {
        snapshot={};
        std::memcpy(snapshot.scalar.data(),frame.constant+0x12c,0x31);
        std::memcpy(snapshot.scalar.data()+0x34,frame.constant+0x160,0x1d);
        std::memcpy(snapshot.scalar.data()+0x54,frame.constant+0x180,0x2c);
        snapshot.aerosol=*reinterpret_cast<float*>(frame.atmosphere+0x88);snapshot.fog=*reinterpret_cast<float*>(frame.atmosphere+0x9c);snapshot.cloud=*reinterpret_cast<float*>(frame.atmosphere+0xa8);
        if(!std::isfinite(snapshot.aerosol)||!std::isfinite(snapshot.fog)||!std::isfinite(snapshot.cloud))return WeatherNativeReason::invalid_value;
        for(std::size_t i=0;i<snapshot.scalar.size();i+=sizeof(float)) {
            // +30 is one byte flag, +44..50 are packed byte flags; their
            // adjacent padding is neither a float nor part of the snapshot.
            if(i==0x30||(i>=0x44&&i<0x54))continue;
            float value{};std::memcpy(&value,snapshot.scalar.data()+i,sizeof(value));
            if(!std::isfinite(value))return WeatherNativeReason::invalid_value;
        }
        params.precipitation=*reinterpret_cast<float*>(frame.constant+0x12c);params.cloudiness=*reinterpret_cast<float*>(frame.constant+0x130);
        params.humidity=*reinterpret_cast<float*>(frame.constant+0x134);params.wind_speed=*reinterpret_cast<float*>(frame.constant+0x138);params.height_fog=snapshot.fog;
        params.phase=*reinterpret_cast<float*>(frame.constant+0x198)>0?PrecipitationPhase::rain:PrecipitationPhase::snow;
        return WeatherNativeReason::ready;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return WeatherNativeReason::native_fault;}
}
inline WeatherNativeReason manual_weather(const WeatherBindings& b,WeatherFrame& frame,bool enabled) noexcept {
    if(!frame.ready||frame.faulted)return WeatherNativeReason::producer_context;
    __try {
        const auto value=static_cast<std::uint8_t>(enabled);b.manual(frame.manager,&value);
        if(frame.manager[0x31]!=value)return WeatherNativeReason::publish_mismatch;
        frame.manual=enabled;return WeatherNativeReason::published;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return WeatherNativeReason::native_fault;}
}
inline WeatherNativeReason apply_weather(const WeatherBindings& b,WeatherFrame& frame,const WeatherParameters& p) noexcept {
    if(!frame.ready||frame.faulted||!valid_weather(p))return WeatherNativeReason::invalid_value;
    __try {
        auto primary=frame.constant-0x28;auto atmosphere=frame.atmosphere-0x28;
        const auto cloud=p.cloudiness>0?p.cloudiness:0;
        b.precipitation(primary,&p.precipitation);b.cloudiness(primary,&p.cloudiness);b.humidity(primary,&p.humidity);b.wind(primary,&p.wind_speed);
        b.fog(atmosphere,&p.height_fog);b.cloud_density(atmosphere,&cloud);
        // Native fog output is max(heightFog, scaled/clamped aerosol). Zeroing
        // only the owned aerosol scalar makes the chosen fog value effective.
        *reinterpret_cast<float*>(frame.atmosphere+0x88)=0;
        *reinterpret_cast<float*>(frame.constant+0x198)=p.phase==PrecipitationPhase::rain?1.0f:-1.0f;
        b.derive(frame.resource);
        WeatherSnapshot observed{};WeatherParameters read{};
        if(read_weather(frame,observed,read)!=WeatherNativeReason::ready||std::memcmp(&read,&p,sizeof(p))||observed.cloud!=cloud||observed.aerosol!=0)
            return WeatherNativeReason::publish_mismatch;
        const auto rain=*reinterpret_cast<float*>(frame.constant+0x16c),snow=*reinterpret_cast<float*>(frame.constant+0x168);
        if(rain!=(p.phase==PrecipitationPhase::rain?p.precipitation:0)||snow!=(p.phase==PrecipitationPhase::snow?p.precipitation:0))return WeatherNativeReason::publish_mismatch;
        return WeatherNativeReason::published;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return WeatherNativeReason::native_fault;}
}
inline WeatherNativeReason restore_weather(WeatherFrame& frame,const WeatherSnapshot& snapshot) noexcept {
    if(!frame.ready||frame.faulted)return WeatherNativeReason::producer_context;
    __try {
        std::memcpy(frame.constant+0x12c,snapshot.scalar.data(),0x31);
        std::memcpy(frame.constant+0x160,snapshot.scalar.data()+0x34,0x1d);
        std::memcpy(frame.constant+0x180,snapshot.scalar.data()+0x54,0x2c);
        *reinterpret_cast<float*>(frame.atmosphere+0x88)=snapshot.aerosol;*reinterpret_cast<float*>(frame.atmosphere+0x9c)=snapshot.fog;*reinterpret_cast<float*>(frame.atmosphere+0xa8)=snapshot.cloud;
        WeatherSnapshot observed{};WeatherParameters ignored{};
        if(read_weather(frame,observed,ignored)!=WeatherNativeReason::ready||observed!=snapshot)return WeatherNativeReason::publish_mismatch;
        return WeatherNativeReason::published;
    } __except(EXCEPTION_EXECUTE_HANDLER) {frame.faulted=true;return WeatherNativeReason::native_fault;}
}
inline WeatherBindings weather_bindings_2976(std::uintptr_t base,bool verified) noexcept {
    WeatherBindings b{};if(!base||!verified)return b;
    b.verified=true;b.base=base;b.manager_vtable=base+0x5d51268;b.resource_vtable=base+0x5ce9df0;b.constant_vtable=base+0x5ce8e58;b.atmosphere_vtable=base+0x5cea328;b.client_field_vtable=base+0x55bcea0;b.world_vtable=base+0x5d20718;
    b.service=reinterpret_cast<void* const*>(base+0x6d69198);b.manual=reinterpret_cast<SetWeatherBool>(base+0x3db7460);
    b.precipitation=reinterpret_cast<SetWeatherFloat>(base+0x37f1ba0);b.cloudiness=reinterpret_cast<SetWeatherFloat>(base+0x37f2570);
    b.humidity=reinterpret_cast<SetWeatherFloat>(base+0x37f2f20);b.wind=reinterpret_cast<SetWeatherFloat>(base+0x37f38e0);
    b.fog=reinterpret_cast<SetWeatherFloat>(base+0x3809230);b.cloud_density=reinterpret_cast<SetWeatherFloat>(base+0x380af40);b.derive=reinterpret_cast<DerivePrecipitation>(base+0x382a1a0);return b;
}
} // namespace crimson::marketplace::world_controls
