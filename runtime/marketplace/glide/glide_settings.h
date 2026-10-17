#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace crimson::marketplace::glide {
inline bool valid_percent(std::uint32_t value) {return value>=100 && value<=500;}
inline std::uint32_t setting(const unsigned char* bytes,std::size_t count) {
    if (!bytes || count!=8 || std::memcmp(bytes,"CWG1",4)) return 100;
    const auto value=std::uint32_t{bytes[4]}|(std::uint32_t{bytes[5]}<<8)|
        (std::uint32_t{bytes[6]}<<16)|(std::uint32_t{bytes[7]}<<24);
    return valid_percent(value)?value:100;
}
} // namespace crimson::marketplace::glide
