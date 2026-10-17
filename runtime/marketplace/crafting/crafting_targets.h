#pragma once
#include "crafting_context.h"
namespace crimson::marketplace::crafting {
inline constexpr char kExecutableHash[] = "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";
inline constexpr Address kRecipeManager = 0x6d6e448;
inline constexpr Address kRecipeVtable = 0x59c0620;
inline constexpr Address kKnowledgeManager = 0x6d69a98;
inline constexpr std::uint32_t kRecipeCount = 18576;
inline constexpr std::uint32_t kEmptyQuantityMaximum = 40;
struct SiteTarget { Address hook, continuation; };
inline constexpr std::array<SiteTarget,kSiteCount> kTargets{{
    {0x2429dec,0x2429e17}, {0x213b793,0x213b812}, {0x1074846,0x1074858},
    {0xe27114,0xe27153}, {0x107d7c2,0x107d801},
}};
struct Fingerprint { Address rva; std::uint32_t length; const char* sha256; };
// Complete affected functions plus prerequisite/transaction witnesses. These
// are fingerprints, not copied game implementation or distribution payloads.
inline constexpr std::array<Fingerprint,23> kFingerprints{{
    {0xe27040,3781,"553d4633856410b7d243ea58c668c40dfcc5d7d0628393811a28c6583d7245ec"},
    {0x107d6e0,3797,"feb67435ffffbb198f428a32fceb57fddc62837f73c70fa19f2388742fa49c6b"},
    {0x1074800,886,"4b07f70dc2f9d240a141f2c0ae7929a36ba92ece0592951626d4ee57aeb070b1"},
    {0x213b640,3906,"e29d7a56a74589114742391176193d61e56c8380c96796a04d94d3cda772f509"},
    {0x2429700,2213,"ff45b209cbd291703c52aefe870ef5f47843dfde12bc7006aa7a1e011ce4577a"},
    {0x2429370,905,"e1ae01dff6384bf26dfcdef7958247945e82b30a4914b3e0c0107188ac51f84a"},
    {0x2b6c050,4657,"0e5edbecca6e04bc0866881890025bef326c5461752400cd692f20a447e9f976"},
    {0x2b6d620,17605,"1254adda5440b82f4895e53b968130a7c5e60e0639d6d705c654791db6e3d984"},
    {0x2b71b10,1258,"3b9b22fdd9ed581ba2af09f2125671dab7926c330f2c2cb9dcb8601d03a3e39a"},
    {0x27b0540,280,"e53915bc21f46c085696f49305d90e99b2fa066c5fd9ac1b3bd79005c800d3d0"},
    {0x49fec0,878,"6a01e86c4621dc86a1d9b3737695eed9082a74468a7ccd945138ac1f1cf32da1"},
    {0x10761e0,2164,"153ce9ed5d8042e7c02a4d0b8b71851db3ca05e652c6998a527bba57402501c3"},
    {0x38ab60,296,"35e8889ccc6a7507443bbfa57b4bdbb81dba83dc6196fba368bab5dbbad71972"},
    {0x151e6e4,142,"adc2bb712dd7c565b0054ea125aaef0ccb636db7cbe22b42e613202c9aa16d52"},
    {0x4a21f0,296,"5d4d4eecb39f40920051a00ac777b5a47f093681b8055b9eaf0734030f27faa3"},
    {0x106ec30,2169,"a2c06148e28e16814248240f07a5a6fe05b3a8db22d025d8434d0380be357425"},
    {0x107c7d0,3855,"257452db36bbee0aaa508e6d56e835d336f43480f4f88222fbb6f833fbac8de4"},
    {0x1071c60,2200,"84a15203700e7e6ab6a17c80fb8ce7f1017e472a809eccd69217a1d0c931dc7a"},
    {0x2428070,220,"bfd81540a1c5a366c7bef67c984cda465513d523c495a89c9cefa5d09b58f709"},
    {0x4a6e40,687,"5767721a0bc4bb20a50f84a1f92766141917bc8fba821b26026156a7bc125b28"},
    {0xf5420a0,185,"bf74e7c798ab6a058a48138a029e151a9f64e9fd0e373be339a815d9e76515ef"},
    {0x15119f0,302,"ba5799bfd9e41767511d68267082cc47c87ec9cfcf9269916b4ebba257fe47f5"},
    {0x1524600,263,"88f61a4e4c2eb4672b23b299fe36a2b6b040096253db27625e9a3772a01156c5"},
}};
} // namespace crimson::marketplace::crafting
