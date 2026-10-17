#pragma once
#include <cstdint>
namespace crimson::marketplace::glide {
// Read-only fingerprints of complete original 156-byte lower-glide records.
// No start, landing, braking-start or dash-ready record is accepted.
struct Animation {std::uint64_t fingerprint; std::uint8_t events;};
inline constexpr Animation kAnimations[]={
    {0x7ea15373faf11adfull,6}, // original record 6
    {0xb88a29af5d0073c9ull,3}, // original record 7
    {0x14f2948e973b2a55ull,6}, // original record 8
    {0x485bae3b9032f4edull,8}, // original record 9
    {0xe1313d2fba9ff120ull,5}, // original record 10
    {0x650ef84d5f660bffull,8}, // original record 11
    {0xda97d93f53a5c0c9ull,16}, // original record 14
    {0x53f0cabb4aa2f022ull,16}, // original record 15
    {0xc35456a1d07b21d8ull,6}, // original record 16
    {0xa6ddf60749f906e2ull,3}, // original record 17
    {0x9c01183904ce2936ull,6}, // original record 18
    {0xfe02c3f72f8e82e5ull,8}, // original record 19
    {0x585bd6cd0978937cull,5}, // original record 20
    {0xbdc3907dcb70490full,8}, // original record 21
    {0xfe02c3f72f8e82e5ull,8}, // original record 22
    {0x585bd6cd0978937cull,5}, // original record 23
    {0xbdc3907dcb70490full,8}, // original record 24
    {0xfe02c3f72f8e82e5ull,8}, // original record 25
    {0x816195b3a793188bull,11}, // original record 32
    {0x3aed8e36dc7cc78cull,7}, // original record 33
    {0xf6086fd7ecdb4d7dull,10}, // original record 34
    {0x6f8684285684c16cull,16}, // original record 40
    {0x7d8d66ab74c9e684ull,17}, // original record 41
    {0x336c9cd439d11c4eull,18}, // original record 42
    {0x2c7582f2eba711e6ull,18}, // original record 43
    {0xc266343d722ed4a0ull,10}, // original record 44
    {0x7b28edd33d0eca8eull,17}, // original record 45
    {0x4595dc7b657945dcull,10}, // original record 47
    {0xd62e93660efe13faull,25}, // original record 48
    {0xb7b6f8f951c64046ull,10}, // original record 49
    {0xa4d384d0d497bd9eull,54}, // original record 50
    {0xd548f377568aea6cull,69}, // original record 51
    {0xf166e2d308c8c808ull,54}, // original record 52
    {0xc616208ac49bf733ull,15}, // original record 53
    {0x3bc7e64788ce5b56ull,18}, // original record 54
    {0xdffc96e5efda86b0ull,20}, // original record 55
    {0x816195b3a793188bull,11}, // original record 56
    {0xe800d6c2be9358ffull,15}, // original record 57
    {0xf6086fd7ecdb4d7dull,10}, // original record 58
};
inline constexpr std::uint32_t kEventBytes=24580, kConditionBytes=824, kFinderCount=181;
} // namespace crimson::marketplace::glide
