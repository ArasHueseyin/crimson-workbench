#pragma once
#include <cstdint>
#include <limits>
#include <optional>

namespace crimson::marketplace::world_controls {
// Pure conversions only. These do not read/write game memory or implement a
// live clock command. The pinned Windows/Mac builds use nonlinear night/day encoding.
inline constexpr std::uint64_t kDayUnits=75'600'000;
inline constexpr std::uint64_t kEarlyNightEnd=5'400'000;
inline constexpr std::uint64_t kLateNightStart=70'200'000;
inline constexpr std::uint64_t kDisplayDayMilliseconds=86'400'000;
struct ClockFields {
    std::uint32_t day{},hour{},minute{},second{},millisecond{};
    std::uint8_t weekday{};
    constexpr bool operator==(const ClockFields&) const =default;
};
inline constexpr std::optional<std::uint64_t> encode_clock(const ClockFields& fields) {
    if (fields.hour>=24 || fields.minute>=60 || fields.second>=60 || fields.millisecond>=1000) return {};
    const std::uint64_t display=(((std::uint64_t{fields.hour}*60+fields.minute)*60)+fields.second)*1000+fields.millisecond;
    std::uint64_t part{};
    if (display>=10'800'000 && display<75'600'000) part=display-5'400'000;
    else part=(display+(display>=75'600'000?64'800'000:0))/2;
    return std::uint64_t{fields.day}*kDayUnits+part;
}
inline constexpr std::optional<ClockFields> decode_clock(std::uint64_t raw) {
    const auto day=raw/kDayUnits;
    if (day>std::numeric_limits<std::uint32_t>::max()) return {};
    const auto part=raw%kDayUnits;
    const auto display=part<kEarlyNightEnd?part*2:(part<kLateNightStart?part+5'400'000:part*2-64'800'000);
    return ClockFields{static_cast<std::uint32_t>(day),static_cast<std::uint32_t>(display/3'600'000),
        static_cast<std::uint32_t>((display/60'000)%60),static_cast<std::uint32_t>((display/1000)%60),
        static_cast<std::uint32_t>(display%1000),static_cast<std::uint8_t>(day%7)};
}
} // namespace crimson::marketplace::world_controls
