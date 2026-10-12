#include "../blackstar/blackstar_request.h"
#include "../sleep/cooldown_policy.h"
#include "../sleep/duration_policy.h"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    int clears = 0;
    bool blocked = true;
    auto read = [&] { return std::uint8_t(blocked ? 0 : 1); };
    auto clear = [&] { ++clears; blocked = false; };
    assert(check_sleep(read, clear) == 1 && clears == 1);
    for (int i = 0; i < 100; ++i) assert(check_sleep(read, clear) == 1);
    assert(clears == 1); // No timer or forced extra attempt.
    assert(check_sleep([] { return std::uint8_t(2); }, clear) == 2);
    assert(check_sleep([] { return std::uint8_t(0); }, clear) == 0);

    unsigned char event[0x40]{};
    event[0x38] = event[0x3a] = 1; event[0x3c] = 12;
    assert(is_twelve_hour_sleep(event));
    for (unsigned hours : {1, 3, 6, 9}) { event[0x3c] = static_cast<unsigned char>(hours); assert(!is_twelve_hour_sleep(event)); }
    event[0x3c] = 12;
    for (unsigned day = 0; day < 12; ++day) for (unsigned hour = 0; hour < 24; ++hour) {
        GameCalendar value{day, hour, 17, 23, 456, 0, 0, 2, 987};
        SleepScope outer(true);
        auto original = value;
        assert(!extend_scoped_target(value, false));
        { SleepScope nested(false); assert(!extend_scoped_target(value, true)); }
        assert(extend_scoped_target(value, true));
        assert(value.day * 24 + value.hour == day * 24 + hour + 18);
        assert(value.minute == original.minute && value.second == original.second && value.millisecond == original.millisecond);
        assert(value.speed == original.speed && value.timestamp == original.timestamp);
        assert(!extend_scoped_target(value, true));
    }
    GameCalendar overflow{UINT32_MAX, 23, 0, 0, 0, 0, 0, 1, 0};
    assert(!extend_sleep_target(overflow));
    overflow.day = 0; overflow.hour = 24; assert(!extend_sleep_target(overflow));

    std::array<unsigned char, 0x600> row{};
    std::array<unsigned char, 0x80> level{};
    std::array<unsigned char, 20 * 40> stats{};
    write_at<std::uint32_t>(row.data(), 0, kBlackstarKey);
    write_at<GameArray>(row.data(), 0x5c0, {level.data(), 1, 1});
    write_at<std::uint32_t>(level.data(), 0, 1);
    write_at<GameArray>(level.data(), 0x60, {stats.data(), 20, 20});
    auto stamina = stats.data() + 12 * 40;
    write_at<std::int64_t>(stamina, 0, 300000); write_at<std::int64_t>(stamina, 16, 20);
    write_at<std::int64_t>(stamina, 24, 100000); write_at<std::int64_t>(stamina, 32, 65000);
    write_at<std::int64_t>(row.data(), 0x70, 3600); write_at<std::int64_t>(row.data(), 0x78, 600);
    auto row_before = row; auto stats_before = stats;
    write_at<std::uint32_t>(row.data(), 0, 123);
    assert(apply_row(row.data()) == RowResult::Other && stats == stats_before);
    row = row_before;
    write_at<std::int64_t>(stamina, 16, 21);
    assert(apply_row(row.data()) == RowResult::Rejected && row == row_before);
    stats = stats_before;
    assert(apply_row(row.data()) == RowResult::Applied);
    assert(apply_row(row.data()) == RowResult::AlreadyApplied);
    write_at<std::int64_t>(row_before.data(), 0x70, 1); write_at<std::int64_t>(row_before.data(), 0x78, 60000);
    write_at<std::int64_t>(stats_before.data(), 12 * 40 + 16, 30000000);
    assert(row == row_before && stats == stats_before); // All unrelated bytes preserved.
    assert(!blackstar_summon && !bypass_region(1, 1));
    {
        FrameSummonScope request(11, 12);
        select_frame_summon(10, kBlackstarKey); assert(!blackstar_summon);
        select_frame_summon(11, kBlackstarKey); assert(bypass_region(7, 7) && !bypass_region(7, 8));
        { SummonScope unrelated(false); assert(!blackstar_summon); }
        assert(blackstar_summon);
        select_frame_summon(12, 999); assert(!blackstar_summon);
    }
    assert(!blackstar_summon && !current_frame_summon);
    std::cout << "PASS: instant cooldown with native validity, 288 calendar cases, nested scope isolation and Blackstar-only byte edits.\n";
}
