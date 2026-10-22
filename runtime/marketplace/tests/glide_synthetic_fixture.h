#pragma once
#include <array>
#include <cstdint>
namespace crimson::marketplace::glide::synthetic_fixture {
// Deliberately fabricated test records; these are not bytes from game assets.
constexpr std::array<unsigned char,156> record(unsigned char seed) {
    std::array<unsigned char,156> bytes{};
    for (std::size_t i=0;i<bytes.size();++i)
        bytes[i]=static_cast<unsigned char>(seed+37*i+(i%7)*11);
    return bytes;
}
constexpr std::uint64_t fingerprint(const std::array<unsigned char,156>& bytes) {
    std::uint64_t result=14695981039346656037ull;
    for (auto byte:bytes) result=(result^byte)*1099511628211ull;
    return result;
}
inline constexpr auto kSustained=record(0x47);
inline constexpr auto kLanding=record(0xa3);
inline constexpr auto kUpperSustained=record(0x61);
inline constexpr auto kUpperLanding=record(0xc7);
inline constexpr std::uint8_t kSustainedEvents=6,kLandingEvents=17;
inline constexpr std::uint8_t kUpperSustainedEvents=7,kUpperLandingEvents=14;
struct SyntheticAnimations {
    constexpr bool operator()(std::uint64_t value,std::uint8_t events,std::uint8_t layer=1) const {
        // The upper wing animation is ordinary gliding, not the native boost.
        if (layer==0) return false;
        return layer==1 && value==fingerprint(kSustained) && events==kSustainedEvents;
    }
};
} // namespace crimson::marketplace::glide::synthetic_fixture
