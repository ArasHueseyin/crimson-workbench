#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace crimson::marketplace::resources {
inline constexpr std::uint32_t kStamina = 1;
inline constexpr std::uint32_t kSpirit = 2;
inline constexpr std::uint32_t kLegacyDefault = kStamina;

// Exactly CWR1 followed by little-endian uint32 flags. Existing invalid bytes
// disable protection; only an absent file retains legacy stamina behavior.
inline std::uint32_t setting(const unsigned char* bytes, std::size_t count) {
    if (!bytes || count != 8 || std::memcmp(bytes, "CWR1", 4)) return 0;
    const auto flags = std::uint32_t{bytes[4]} | (std::uint32_t{bytes[5]} << 8) |
        (std::uint32_t{bytes[6]} << 16) | (std::uint32_t{bytes[7]} << 24);
    return flags <= (kStamina | kSpirit) ? flags : 0;
}
} // namespace crimson::marketplace::resources
