#pragma once
#include <cstdint>
namespace crimson::marketplace::glide {
// Read-only fingerprints of complete original 156-byte glide records.
// Only sustained native fast-blend glide records qualify. Normal wing glide,
// start/landing/braking transitions and directional dodges remain unchanged.
struct Animation {std::uint64_t fingerprint; std::uint8_t events;};
inline constexpr Animation kAnimations[]={
    {0x816195b3a793188bull,11}, // original lower record 32
    {0x3aed8e36dc7cc78cull,7}, // original lower record 33
    {0xf6086fd7ecdb4d7dull,10}, // original lower record 34
    {0x816195b3a793188bull,11}, // original lower record 56
    {0xf6086fd7ecdb4d7dull,10}, // original lower record 58
};
// Exclusion evidence for regression tests; none of these records qualify.
inline constexpr Animation kUnboostedAnimations[]={
    {0x36eb2f2bf4021cf8ull,27}, // original lower record 0
    {0x6f7fec20b1a620a5ull,17}, // original lower record 1
    {0x210f330a72ece8efull,19}, // original lower record 2
    {0x5ea076a418849454ull,27}, // original lower record 3
    {0xea67a10fa5119e08ull,17}, // original lower record 4
    {0xbc0950834d625113ull,19}, // original lower record 5
    {0x7ea15373faf11adfull,6}, // original lower record 6
    {0xb88a29af5d0073c9ull,3}, // original lower record 7
    {0x14f2948e973b2a55ull,6}, // original lower record 8
    {0x485bae3b9032f4edull,8}, // original lower record 9
    {0xe1313d2fba9ff120ull,5}, // original lower record 10
    {0x650ef84d5f660bffull,8}, // original lower record 11
    {0x3841891efaa9e422ull,19}, // original lower record 12
    {0x203ea8df2baa4bc1ull,19}, // original lower record 13
    {0xda97d93f53a5c0c9ull,16}, // original lower record 14
    {0x53f0cabb4aa2f022ull,16}, // original lower record 15
    {0xc35456a1d07b21d8ull,6}, // original lower record 16
    {0xa6ddf60749f906e2ull,3}, // original lower record 17
    {0x9c01183904ce2936ull,6}, // original lower record 18
    {0xfe02c3f72f8e82e5ull,8}, // original lower record 19
    {0x585bd6cd0978937cull,5}, // original lower record 20
    {0xbdc3907dcb70490full,8}, // original lower record 21
    {0xfe02c3f72f8e82e5ull,8}, // original lower record 22
    {0x585bd6cd0978937cull,5}, // original lower record 23
    {0xbdc3907dcb70490full,8}, // original lower record 24
    {0xfe02c3f72f8e82e5ull,8}, // original lower record 25
    {0x35ff41a5146abfaaull,10}, // original lower record 26
    {0xd5b72e4768194761ull,5}, // original lower record 27
    {0x0f7c9d80c06a26aaull,6}, // original lower record 28
    {0x630816b35012ecdaull,16}, // original lower record 29
    {0x1aa39c41a1fe0d0cull,12}, // original lower record 30
    {0x61334f5c377c3792ull,13}, // original lower record 31
    {0xfd80841ade35882aull,17}, // original lower record 35
    {0x3c9d6b3f15daa5a4ull,12}, // original lower record 36
    {0xe87cdfd0fda3291aull,12}, // original lower record 37
    {0xfa9fb2c81a243132ull,6}, // original lower record 38
    {0xe3275b9ff2963846ull,11}, // original lower record 39
    {0x6f8684285684c16cull,16}, // original lower record 40
    {0x7d8d66ab74c9e684ull,17}, // original lower record 41
    {0x336c9cd439d11c4eull,18}, // original lower record 42
    {0x2c7582f2eba711e6ull,18}, // original lower record 43
    {0xc266343d722ed4a0ull,10}, // original lower record 44
    {0x7b28edd33d0eca8eull,17}, // original lower record 45
    {0xb08e8bdea622ebe0ull,5}, // original lower record 46
    {0x4595dc7b657945dcull,10}, // original lower record 47
    {0xd62e93660efe13faull,25}, // original lower record 48
    {0xb7b6f8f951c64046ull,10}, // original lower record 49
    {0xa4d384d0d497bd9eull,54}, // original lower record 50
    {0xd548f377568aea6cull,69}, // original lower record 51
    {0xf166e2d308c8c808ull,54}, // original lower record 52
    {0xc616208ac49bf733ull,15}, // original lower record 53
    {0x3bc7e64788ce5b56ull,18}, // original lower record 54
    {0xdffc96e5efda86b0ull,20}, // original lower record 55
    {0xe800d6c2be9358ffull,15}, // original lower record 57
};
inline constexpr std::uint32_t kEventBytes=24580, kConditionBytes=824, kFinderCount=181;
// The native movement selector uses upper layer 0 or lower layer 1.
// Bind each fingerprint to its original package and selected layer.
// Ordinary upper-body wing glide is exclusion evidence only.
inline constexpr Animation kUpperAnimations[]={
    {0x11ef84f8dfb77a68ull,6}, // original upper record 0
    {0x5dede4baa437fcebull,4}, // original upper record 1
    {0x515d5f13cd853f70ull,3}, // original upper record 2
    {0x11ef84f8dfb77a68ull,6}, // original upper record 3
    {0x5dede4baa437fcebull,4}, // original upper record 4
    {0x515d5f13cd853f70ull,3}, // original upper record 5
    {0x11ef84f8dfb77a68ull,7}, // original upper record 6
    {0x5dede4baa437fcebull,5}, // original upper record 7
    {0x515d5f13cd853f70ull,4}, // original upper record 8
    {0x2b1e0fd5733a5dfcull,10}, // original upper record 56
    {0xa39997f03be8ae87ull,7}, // original upper record 57
    {0x591e89991b8af49cull,9}, // original upper record 58
    {0xa12cfa43d44bdabaull,14}, // original upper record 68
    {0x2261bae428063e69ull,10}, // original upper record 69
    {0x04ac301949fa07b2ull,11}, // original upper record 70
};
inline constexpr std::uint32_t kUpperEventBytes=14956, kUpperConditionBytes=680, kUpperFinderCount=107;
inline bool package_matches(std::uint8_t layer,std::uint32_t events,std::uint32_t conditions,std::uint32_t finders) {
    if (layer==0) return events==kUpperEventBytes && conditions==kUpperConditionBytes && finders==kUpperFinderCount;
    return layer==1 && events==kEventBytes && conditions==kConditionBytes && finders==kFinderCount;
}
} // namespace crimson::marketplace::glide
