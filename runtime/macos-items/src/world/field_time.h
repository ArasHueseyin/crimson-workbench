#pragma once
#include "time_units.h"
#include <cstddef>
#include <type_traits>

namespace crimson::marketplace::world_controls {
// Value layout shared by the Windows protocol and pinned Mac build. No engine pointers.
struct FieldTime {
    std::uint32_t day{},hour{},minute{},second{},millisecond{};
    std::uint8_t weekday{},reserved{};
    std::uint16_t rate{};
    std::uint64_t anchor{};
    constexpr bool operator==(const FieldTime&) const = default;
};
static_assert(sizeof(FieldTime)==0x20 && alignof(FieldTime)==8);
static_assert(offsetof(FieldTime,weekday)==0x14 && offsetof(FieldTime,rate)==0x16 && offsetof(FieldTime,anchor)==0x18);
static_assert(std::is_trivially_copyable_v<FieldTime> && std::is_standard_layout_v<FieldTime>);
inline constexpr ClockFields fields(const FieldTime& time) {
    return {time.day,time.hour,time.minute,time.second,time.millisecond,time.weekday};
}
inline constexpr FieldTime field_time(const ClockFields& time,std::uint16_t rate,std::uint64_t anchor) {
    return {time.day,time.hour,time.minute,time.second,time.millisecond,time.weekday,0,rate,anchor};
}
// Mirrors native clock conversion, including its future-anchor branch. The native
// arithmetic is elapsed*rate (no denominator), so zero is a supported stop.
// Reject overflow instead of allowing the native unchecked multiplication.
inline constexpr std::optional<FieldTime> at_native_time(const FieldTime& time,std::uint64_t now) {
    const auto raw=encode_clock(fields(time));if(!raw)return {};
    const auto elapsed=now>time.anchor?now-time.anchor:0;
    const auto limit=std::numeric_limits<std::uint64_t>::max();
    if(time.rate && elapsed>(limit-*raw)/time.rate)return {};
    const auto current=decode_clock(*raw+elapsed*time.rate);if(!current)return {};
    return field_time(*current,time.rate,now>time.anchor?now:time.anchor);
}
struct TimeOfDay {
    std::uint32_t hour{},minute{},second{},millisecond{};
};
// A choice changes the displayed time on the current native day. A new anchor
// prevents the elapsed interval from being counted again after publication.
inline constexpr std::optional<FieldTime> choose_time(const FieldTime& current,TimeOfDay choice,std::uint64_t now) {
    auto time=at_native_time(current,now);if(!time)return {};
    auto selected=fields(*time);
    selected.hour=choice.hour;selected.minute=choice.minute;selected.second=choice.second;selected.millisecond=choice.millisecond;
    const auto raw=encode_clock(selected);if(!raw)return {};
    const auto canonical=decode_clock(*raw);if(!canonical)return {};
    return field_time(*canonical,current.rate,now);
}
inline constexpr std::uint32_t kMaximumAdvanceHours=10'000;
// Advance whole DISPLAYED hours, including day/week rollover, rather than
// adding linear raw units through the compressed native nighttime intervals.
inline constexpr std::optional<FieldTime> advance_time(const FieldTime& current,std::uint32_t hours,std::uint64_t now) {
    if(!hours||hours>kMaximumAdvanceHours)return {};
    const auto time=at_native_time(current,now);if(!time)return {};
    auto selected=fields(*time);
    const auto total_hours=std::uint64_t{selected.hour}+hours;
    const auto day=std::uint64_t{selected.day}+total_hours/24;
    if(day>std::numeric_limits<std::uint32_t>::max())return {};
    selected.day=static_cast<std::uint32_t>(day);selected.hour=static_cast<std::uint32_t>(total_hours%24);
    const auto raw=encode_clock(selected);if(!raw)return {};
    const auto canonical=decode_clock(*raw);if(!canonical)return {};
    return field_time(*canonical,current.rate,now);
}
inline constexpr std::optional<FieldTime> freeze_time(const FieldTime& current,std::uint64_t now) {
    auto time=at_native_time(current,now);if(!time)return {};
    time->rate=0;time->anchor=now;return time;
}
// Resume at the frozen displayed time, rather than catching up the frozen
// interval or resetting the native day. The caller owns the captured rate.
inline constexpr std::optional<FieldTime> resume_time(const FieldTime& frozen,std::uint16_t saved_rate,std::uint64_t now) {
    if(frozen.rate!=0)return {};
    auto time=at_native_time(frozen,now);if(!time)return {};
    time->rate=saved_rate;time->anchor=now;return time;
}
} // namespace crimson::marketplace::world_controls
