#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace crimson::marketplace::force_palm_height {
inline std::uint32_t setting(const unsigned char* bytes,std::size_t count) {
    if (!bytes || count!=8 || std::memcmp(bytes,"CWF1",4)) return 100;
    const auto value=std::uint32_t{bytes[4]} | (std::uint32_t{bytes[5]}<<8) |
        (std::uint32_t{bytes[6]}<<16) | (std::uint32_t{bytes[7]}<<24);
    return value>=100 && value<=10000?value:100;
}
} // namespace crimson::marketplace::force_palm_height
