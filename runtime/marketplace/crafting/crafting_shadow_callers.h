#pragma once
#include "crafting_context.h"
#include <algorithm>
namespace crimson::marketplace::crafting {
// Instruction-verified post-call addresses from ten reviewed native UI, client
// validation and server-transaction functions. Database initialization is excluded.
inline constexpr std::array<Address,50> kShadowCallers{{
    0xe2710a,0xe27122,0xe2712f,0xe2713a,0xe272f2,0xe27326,
    0xe2758b,0x106ed30,0x106ee5d,0x106f17e,0x1071f82,0x10721ae,
    0x1072246,0x107225c,0x10748e3,0x10768fc,0x107c8cb,0x107c8ff,
    0x107ca42,0x107d7be,0x107d7d0,0x107d7dd,0x107d7e8,0x107d9a2,
    0x107d9d6,0x107dc3b,0x213b821,0x213b8c4,0x213b94f,0x213bbd3,
    0x213c49c,0x2b6c370,0x2b6cb27,0x2b6cf4b,0x2b6cf80,0x2b6cfa3,
    0x2b6cfcd,0x2b6d03b,0x2b6d763,0x2b6d77e,0x2b6d7dc,0x2b6d94d,
    0x2b6df8b,0x2b6dfe2,0x2b6e16c,0x2b6e1f6,0x2b6e532,0x2b6eb31,
    0x2b6ee10,0x2b707c7,
}};
inline bool shadow_caller(Address base, Address returned) noexcept {
    return returned>=base && std::binary_search(kShadowCallers.begin(),kShadowCallers.end(),returned-base);
}
} // namespace crimson::marketplace::crafting
