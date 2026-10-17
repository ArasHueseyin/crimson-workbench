#pragma once
#include "field_time.h"
#include <bit>
#include <cmath>

namespace crimson::marketplace::world_controls {
inline constexpr std::uint32_t kControlMagic=0x31575743; // CWW1, little endian
inline constexpr std::uint32_t kControlVersion=1;
// These controls expose native scalar parameters, not invented resource names.
// Bounds are a conservative subset of native reflection/consumer ranges.
enum class PrecipitationPhase : std::uint32_t { rain=0,snow=1 };
struct WeatherParameters {
    float precipitation{}; // [0,1], native WeatherConstant data+0x12c
    float cloudiness{};    // [-1,1], data+0x130
    float humidity{};      // [0,1], data+0x134
    float wind_speed{};    // [0,60], data+0x138; native output clamps at 60
    float height_fog{};    // [0,10], AtmosphereConstant data+0x9c
    PrecipitationPhase phase{PrecipitationPhase::rain};
};
static_assert(sizeof(WeatherParameters)==24);
enum class ControlOperation : std::uint32_t {
    status=0,choose_time=1,freeze_time=2,release_time=3,
    choose_weather=4,freeze_weather=5,release_weather=6,advance_time=7
};
inline constexpr bool is_weather_operation(ControlOperation operation) noexcept {
    return operation>=ControlOperation::choose_weather&&operation<=ControlOperation::release_weather;
}
enum class ControlCode : std::uint32_t {
    unavailable=0,ready=1,queued=2,published=3,rejected=4,expired=5,
    world_changed=6,uncertain=7,fault_latched=8,released=9
};
enum class ControlReason : std::uint32_t {
    none=0,protocol=1,session=2,generation=3,deadline=4,replay=5,busy=6,
    context=7,stage_fixed=8,native_stopped=9,foreign_change=10,native_fault=11,
    publication=12,invalid_value=13,unsupported=14,not_frozen=15
};
enum ControlFlags : std::uint32_t {
    time_available=1u<<0,weather_available=1u<<1,time_frozen=1u<<2,
    weather_frozen=1u<<3,time_uncertain=1u<<4,weather_uncertain=1u<<5
};
struct Request {
    std::uint32_t magic{kControlMagic},version{kControlVersion},bytes{sizeof(Request)};
    ControlOperation operation{ControlOperation::status};
    std::uint64_t id{},session{},generation{},not_after_ms{};
    // choose_time: displayed time of day. advance_time: hour is the whole-hour
    // delta (word48); minute/second/millisecond words52/56/60 must be zero.
    // This deliberately reuses the existing member, without a type-punned union.
    TimeOfDay time{};
    WeatherParameters weather{};
    std::uint32_t enabled{},reserved{};
};
struct Reply {
    std::uint32_t magic{kControlMagic},version{kControlVersion},bytes{sizeof(Reply)};
    ControlCode code{ControlCode::unavailable};
    std::uint64_t id{},session{},generation{},monotonic_ms{};
    ControlReason reason{ControlReason::none};
    std::uint32_t flags{},identity{},world{},stage{0xffff},clock_reason{},weather_reason{},reserved{};
    FieldTime time{};
    WeatherParameters weather{};
    std::uint64_t time_receipt{},weather_receipt{},reserved_tail{};
    ControlCode time_result{ControlCode::unavailable};ControlReason time_result_reason{ControlReason::none};
    ControlCode weather_result{ControlCode::unavailable};ControlReason weather_result_reason{ControlReason::none};
};
static_assert(sizeof(Request)==96 && sizeof(Reply)==176);
static_assert(offsetof(Request,id)==16 && offsetof(Request,time)==48 && offsetof(Request,weather)==64 && offsetof(Request,enabled)==88);
static_assert(offsetof(Reply,reason)==48 && offsetof(Reply,time)==80 && offsetof(Reply,weather)==112 && offsetof(Reply,time_receipt)==136);
static_assert(offsetof(Reply,time_result)==160 && offsetof(Reply,weather_result)==168);
static_assert(std::is_trivially_copyable_v<Request> && std::is_standard_layout_v<Request>);
static_assert(std::is_trivially_copyable_v<Reply> && std::is_standard_layout_v<Reply>);
using ControlHandler=Reply(*)(const Request&,std::uint64_t now);
inline constexpr bool valid_advance_payload(const Request& request) noexcept {
    return request.time.hour&&request.time.hour<=kMaximumAdvanceHours&&
        !request.time.minute&&!request.time.second&&!request.time.millisecond&&!request.enabled&&
        !std::bit_cast<std::uint32_t>(request.weather.precipitation)&&!std::bit_cast<std::uint32_t>(request.weather.cloudiness)&&
        !std::bit_cast<std::uint32_t>(request.weather.humidity)&&!std::bit_cast<std::uint32_t>(request.weather.wind_speed)&&
        !std::bit_cast<std::uint32_t>(request.weather.height_fog)&&request.weather.phase==PrecipitationPhase::rain;
}
inline bool valid_weather(const WeatherParameters& p) noexcept {
    return std::isfinite(p.precipitation)&&p.precipitation>=0&&p.precipitation<=1&&
        std::isfinite(p.cloudiness)&&p.cloudiness>=-1&&p.cloudiness<=1&&
        std::isfinite(p.humidity)&&p.humidity>=0&&p.humidity<=1&&
        std::isfinite(p.wind_speed)&&p.wind_speed>=0&&p.wind_speed<=60&&
        std::isfinite(p.height_fog)&&p.height_fog>=0&&p.height_fog<=10&&
        (p.phase==PrecipitationPhase::rain||p.phase==PrecipitationPhase::snow);
}
} // namespace crimson::marketplace::world_controls
