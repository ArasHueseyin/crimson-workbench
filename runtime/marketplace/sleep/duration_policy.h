#pragma once
#include <cstdint>
#include <cstring>
#include <limits>

struct GameCalendar {
    std::uint32_t day, hour, minute, second, millisecond;
    std::uint8_t weekday, padding;
    std::uint16_t speed;
    std::uint64_t timestamp;
};
static_assert(sizeof(GameCalendar)==32);

inline bool is_twelve_hour_sleep(const unsigned char* event) {
    // Current build's reflected event fields: mode, fix, cooldown, day, hour, minute.
    return event[0x38]==1 && event[0x39]==0 && event[0x3a]==1 &&
           event[0x3b]==0 && event[0x3c]==12 && event[0x3d]==0;
}
inline bool extend_sleep_target(GameCalendar& calendar) {
    if (calendar.hour>=24 || calendar.minute>=60 || calendar.second>=60 || calendar.millisecond>=1000)
        return false;
    const auto hours=calendar.hour+18;
    const auto days=hours/24;
    if (calendar.day>std::numeric_limits<std::uint32_t>::max()-days) return false;
    calendar.day+=days;
    calendar.hour=hours%24;
    calendar.weekday=static_cast<std::uint8_t>(calendar.day%7);
    return true;
}

struct SleepScopeState { bool eligible=false, consumed=false; };
inline thread_local SleepScopeState sleep_scope;
struct SleepScope {
    SleepScopeState saved;
    explicit SleepScope(bool eligible): saved(sleep_scope) {sleep_scope={eligible,false};}
    ~SleepScope() {sleep_scope=saved;}
};
inline bool extend_scoped_target(GameCalendar& calendar, bool expected_caller) {
    if (!expected_caller || !sleep_scope.eligible || sleep_scope.consumed) return false;
    if (!extend_sleep_target(calendar)) return false;
    sleep_scope.consumed=true;
    return true;
}
