#pragma once
#include "control_protocol.h"
#include "../weather_read.h"
#include <array>

namespace crimson::mac_items {
namespace world = crimson::marketplace::world_controls;
struct WeatherSettings {
    float precipitation{},cloudiness{},humidity{},wind{},fog{},cloud{},aerosol{},phase{};
    world::WeatherParameters parameters() const {
        return {precipitation,cloudiness,humidity,wind,fog,phase>0?world::PrecipitationPhase::rain:world::PrecipitationPhase::snow};
    }
    bool same_control(const WeatherSettings& other) const {
        // The ordinary producer recomputes temperature/phase; its derived rain/snow is checked separately.
        return precipitation==other.precipitation&&cloudiness==other.cloudiness&&humidity==other.humidity
            &&wind==other.wind&&fog==other.fog&&cloud==other.cloud&&aerosol==other.aerosol;
    }
};
inline WeatherSettings settings_of(const WeatherRead& v) {
    return {v.precipitation,v.cloudiness,v.humidity,v.wind,v.fog,v.cloud,v.aerosol,v.phase};
}
inline WeatherSettings selected_weather(const world::WeatherParameters& p) {
    return {p.precipitation,p.cloudiness,p.humidity,p.wind_speed,p.height_fog,
        p.cloudiness>0?p.cloudiness:0,0,p.phase==world::PrecipitationPhase::rain?1.f:-1.f};
}
struct WeatherFrame {
    WorldProbe context;
    WeatherRead view;
    WeatherSettings settings;
    std::uint64_t fingerprint{};
    float rain{},snow{};
    bool ready{};
};
inline std::uint64_t weather_fingerprint(std::uintptr_t manager,std::uintptr_t resource,std::uint64_t salt) {
    auto value=std::uint64_t{manager}^std::rotl(std::uint64_t{resource},17)^salt;
    value^=value>>30;value*=0xbf58476d1ce4e5b9ULL;value^=value>>27;return value?value:1;
}
template<class Read>
WeatherFrame open_weather_frame(std::uintptr_t job,std::intptr_t slide,std::uint64_t salt,Read read) {
    WeatherFrame frame;
    std::uintptr_t type{},owner{},rain{},snow{};
    if(!job||job>std::numeric_limits<std::uintptr_t>::max()-0x90
        ||!read(job,type)||type!=0x108288ab0ULL+slide||!read(job+0x90,owner))return frame;
    frame.context=inspect_active_weather(slide,read);
    if(!frame.context.weather_read||owner!=frame.context.weather_world)return frame;
    frame.view=read_weather(frame.context,slide,read);
    if(!frame.view.ready||!read(frame.view.constant+0x194,rain)||!read(frame.view.constant+0x190,snow))return frame;
    frame.rain=std::bit_cast<float>(static_cast<std::uint32_t>(rain));
    frame.snow=std::bit_cast<float>(static_cast<std::uint32_t>(snow));
    frame.settings=settings_of(frame.view);
    frame.fingerprint=weather_fingerprint(frame.context.weather,frame.view.resource,salt);
    frame.ready=std::isfinite(frame.rain)&&std::isfinite(frame.snow);return frame;
}
enum class WeatherPublication { published,rejected,uncertain };
using WeatherBool=void(*)(void*,const std::uint8_t*);
using WeatherFloat=void(*)(void*,const float*);
struct WeatherBindings {
    WeatherBool manual{};
    WeatherFloat precipitation{},cloudiness{},humidity{},wind{},fog{},cloud{};
    std::intptr_t slide{};
};
template<class Read>
WeatherRead current_weather_frame(const WeatherFrame& frame,const WeatherBindings& b,Read read) {
    const auto context=inspect_active_weather(b.slide,read);
    const auto current=read_weather(context,b.slide,read);
    if(current.ready&&context.weather==frame.context.weather&&context.weather_world==frame.context.weather_world
        &&current.resource==frame.view.resource&&current.constant==frame.view.constant&&current.atmosphere==frame.view.atmosphere)return current;
    return {};
}
template<class Read>
bool weather_frame_matches(const WeatherFrame& frame,const WeatherBindings& b,Read read) {
    const auto current=current_weather_frame(frame,b,read);
    return current.ready&&current.manual==frame.view.manual&&settings_of(current).same_control(frame.settings);
}
template<class Read>
WeatherPublication manual_weather(const WeatherFrame& frame,const WeatherBindings& b,bool enabled,Read read) {
    if(!b.manual||!weather_frame_matches(frame,b,read))return WeatherPublication::rejected;
    const std::uint8_t value=enabled;
    b.manual(reinterpret_cast<void*>(frame.context.weather),&value);
    const auto observed=current_weather_frame(frame,b,read);
    return observed.ready&&observed.manual==enabled?WeatherPublication::published:WeatherPublication::uncertain;
}
template<class Read>
WeatherPublication apply_weather(const WeatherFrame& frame,const WeatherBindings& b,const WeatherSettings& settings,bool acquire,Read read) {
    if(!b.manual||!b.precipitation||!b.cloudiness||!b.humidity||!b.wind||!b.fog||!b.cloud
        ||(acquire?frame.view.manual:!frame.view.manual)||!weather_frame_matches(frame,b,read))return WeatherPublication::rejected;
    for(const auto value:std::array{settings.precipitation,settings.cloudiness,settings.humidity,settings.wind,
        settings.fog,settings.cloud,settings.aerosol,settings.phase})if(!std::isfinite(value))return WeatherPublication::rejected;
    if(acquire){const auto manual=manual_weather(frame,b,true,read);if(manual!=WeatherPublication::published)return manual;}
    auto constant=reinterpret_cast<void*>(frame.view.constant),atmosphere=reinterpret_cast<void*>(frame.view.atmosphere);
    b.precipitation(constant,&settings.precipitation);b.cloudiness(constant,&settings.cloudiness);
    b.humidity(constant,&settings.humidity);b.wind(constant,&settings.wind);
    b.fog(atmosphere,&settings.fog);b.cloud(atmosphere,&settings.cloud);
    *reinterpret_cast<float*>(frame.view.atmosphere+0xb0)=settings.aerosol;
    *reinterpret_cast<float*>(frame.view.constant+0x1c0)=settings.phase;
    const auto observed=current_weather_frame(frame,b,read);
    const auto values=settings_of(observed);
    return observed.ready&&observed.manual&&values.same_control(settings)&&values.phase==settings.phase
        ?WeatherPublication::published:WeatherPublication::uncertain;
}
template<class Read>
WeatherPublication hold_weather_phase(const WeatherFrame& frame,const WeatherBindings& b,float phase,Read read) {
    if(!std::isfinite(phase)||!weather_frame_matches(frame,b,read))return WeatherPublication::rejected;
    *reinterpret_cast<float*>(frame.view.constant+0x1c0)=phase;
    return WeatherPublication::published;
}
}
