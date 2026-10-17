#pragma once
#include <array>
#include <cstdint>
namespace crimson::marketplace::force_palm_height {
// FNV-1a fingerprints of complete original records. No serialized game data
// is embedded: seven animation/event-count pairs and ten exact launch sites.
struct Animation { std::uint64_t fingerprint; std::uint8_t events; };
inline constexpr Animation kAnimations[]={
    {0x4512d7236367ed47ull,33},
    {0x4512d7236367ed47ull,39},
    {0x5e19b7973debb833ull,31},
    {0xcbc60508782fc17cull,31},
    {0x54902c669ef70a1full,24},
    {0xc23b06755cde4991ull,25},
    {0x00085cafd480a4a1ull,25},
};
struct Launch {
    std::uint8_t animation,index;
    std::uint32_t word;
    std::uint64_t instance_fingerprint,data_fingerprint;
};
inline constexpr Launch kLaunches[]={
    {0,13,0x1cf45,0x8999015b32455c16ull,0x3c7ff70fb6016281ull},
    {0,15,0x1cf96,0xb4af41f553cfd423ull,0xcc82fb9f3a132eaeull},
    {1,19,0x1cf45,0x98f9259253918340ull,0x3c7ff70fb6016281ull},
    {1,21,0x1cf96,0xa54f1dbe3283acf9ull,0xcc82fb9f3a132eaeull},
    {3,28,0x1d528,0xc2fa1bc6b03b3508ull,0x49f7e430f01bd503ull},
    {4,4,0x1d52f,0x0b955f62e12aed97ull,0x54f4cfb3bb6fe431ull},
    {4,5,0x1d536,0x8459c5978cc1d31bull,0xfdb52e95b711328eull},
    {5,15,0x1d656,0x9ccf020ecbb7ab2aull,0x6d62a5b581cf4621ull},
    {5,16,0x1d65d,0x6fee49b560ead29aull,0x45a2b1db53b1255eull},
    {6,17,0x1d686,0xd8acb1d4180aa0b2ull,0x530626148eb13dc8ull},
};
struct KnownPalmRecords {
    constexpr const auto& animations() const { return kAnimations; }
    constexpr const auto& launches() const { return kLaunches; }
};
inline constexpr std::uint32_t kEventBytes=531344,kConditionBytes=11212,kFinderCount=2325;
} // namespace crimson::marketplace::force_palm_height
