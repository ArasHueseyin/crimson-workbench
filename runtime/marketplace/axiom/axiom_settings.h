#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace crimson::marketplace::axiom {
// Shared with desktop config: CWA1 followed by one little-endian u32 percent.
inline bool valid_percent(std::uint32_t percent) {
    return percent == 100 || (percent >= 200 && percent <= 500);
}
inline std::uint32_t setting(const unsigned char* bytes, std::size_t count) {
    if (!bytes || count != 8 || std::memcmp(bytes, "CWA1", 4)) return 100;
    const auto percent = std::uint32_t{bytes[4]} | (std::uint32_t{bytes[5]} << 8) |
        (std::uint32_t{bytes[6]} << 16) | (std::uint32_t{bytes[7]} << 24);
    return valid_percent(percent) ? percent : 100;
}
} // namespace crimson::marketplace::axiom
