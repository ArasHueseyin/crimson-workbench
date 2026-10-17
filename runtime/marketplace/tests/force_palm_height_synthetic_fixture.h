#pragma once
#include <array>
#include <bit>
#include "../force-palm-height/force_palm_height_data.h"
namespace crimson::marketplace::force_palm_height::synthetic_fixture {
// Owned artificial records. Only the public site/index/count metadata is shared
// with production; no serialized animation or event bytes come from the game.
struct Animation { std::array<unsigned char,156> fixed; std::uint8_t events; };
struct Launch {
    std::uint8_t animation,index;std::uint32_t word;
    std::array<unsigned char,20> instance;std::array<unsigned char,26> data;
};
inline constexpr auto kAnimations=[] {
    std::array<Animation,std::size(force_palm_height::kAnimations)> values{};
    for (std::size_t i=0;i<values.size();++i) {
        values[i].events=force_palm_height::kAnimations[i].events;
        for (std::size_t j=0;j<values[i].fixed.size();++j)
            values[i].fixed[j]=static_cast<unsigned char>(0x51+43*i+17*j+(j%5)*3);
    }
    return values;
}();
inline constexpr auto kLaunches=[] {
    std::array<Launch,std::size(force_palm_height::kLaunches)> values{};
    for (std::size_t i=0;i<values.size();++i) {
        const auto& site=force_palm_height::kLaunches[i];auto& value=values[i];
        value.animation=site.animation;value.index=site.index;value.word=site.word;
        for (std::size_t j=0;j<value.instance.size();++j)
            value.instance[j]=static_cast<unsigned char>(0x35+17*i+19*j);
        for (std::size_t j=0;j<value.data.size();++j)
            value.data[j]=static_cast<unsigned char>(0x81+31*i+7*j);
        const auto height=std::bit_cast<std::uint32_t>(1.25f+static_cast<float>(i)*0.5f);
        for (unsigned j=0;j<4;++j)value.data[12+j]=static_cast<unsigned char>(height>>(8*j));
    }
    return values;
}();
} // namespace crimson::marketplace::force_palm_height::synthetic_fixture
