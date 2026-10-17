#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace crimson::marketplace::player_jump {
// Shared with cd_core::config::player_jump: exactly CWJ1 + little-endian u32.
inline std::uint32_t setting(const unsigned char* bytes, std::size_t count) {
    if (!bytes || count != 8 || std::memcmp(bytes, "CWJ1", 4)) return 100;
    const auto percent = std::uint32_t{bytes[4]} | (std::uint32_t{bytes[5]} << 8) |
        (std::uint32_t{bytes[6]} << 16) | (std::uint32_t{bytes[7]} << 24);
    return percent >= 100 && percent <= 300 ? percent : 100;
}
} // namespace crimson::marketplace::player_jump
