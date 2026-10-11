#pragma once
#include <cstdint>

// Keep the game's validity checks. Clear only its sleep deadline and repeat
// the same check immediately; unrelated native failures still fail.
template<class Read, class Clear>
std::uint8_t check_sleep(Read read, Clear clear) {
    auto result = read();
    if (result != 0) return result;
    clear();
    return read();
}
