#pragma once
#include <cmath>
#include "../jump/player_jump_scope.h"
namespace crimson::marketplace::player_speed {
using Address = std::uintptr_t;
inline constexpr Address kControlVtable = 0x55b5460;
inline constexpr Address kNormalMethodVtable = 0x55cab28;
inline constexpr Address kGroundReturns[] = {0x36a3d0c, 0x36a4222, 0x36a473e, 0x36a4b66};
inline bool ground_call(Address base, Address return_address) {
    if (!base || return_address < base) return false;
    for (auto value : kGroundReturns) if (return_address - base == value) return true;
    return false;
}
template<class Read>
bool selected_ground(Read& read, Address base, Address controller, Address return_address) {
    using namespace stamina;
    if (!controller || !ground_call(base, return_address)) return false;
    const auto root = read.template at<Address>(base + kClientRoot);
    if (!root) return false;
    const auto manager = read.template at<Address>(root + 0x30);
    if (!manager || read.template at<Address>(manager) != base + kClientManager) return false;
    const auto actor = read.template at<Address>(manager + 0x50);
    if (!actor) return false;
    const auto components = read.template at<Address>(actor + 0x68);
    const auto count = read.template at<std::uint32_t>(actor + 0x70);
    if (!components || count < 9 || count > 57) return false;
    const auto control = read.template at<Address>(components + 0x40);
    if (!control || read.template at<Address>(control) != base + kControlVtable) return false;
    const auto state = read.template at<Address>(control + 0xc8);
    player_jump::Adjustment owner{};
    if (!player_jump::selected(read, base, state, owner) || owner.actor != actor ||
        owner.components != components || owner.control != control ||
        read.template at<Address>(state + 0x60) != control ||
        read.template at<Address>(control + 0x140) != controller ||
        read.template at<std::uint8_t>(controller + 0x2ed) != 0) return false;
    const auto mode = read.template at<std::uint8_t>(state + 0xf0);
    // The original registers NormalMovementMethod for modes 0..3. Other
    // methods include scripted, climbing, vehicle and mount movement.
    if (mode > 3) return false;
    const auto method = read.template at<Address>(state + 0xf8 + Address{mode} * 8);
    if (!method || read.template at<Address>(method) != base + kNormalMethodVtable ||
        read.template at<Address>(method + 0x18) != control ||
        read.template at<Address>(method + 0x20) != actor) return false;
    player_jump::Adjustment current{};
    return player_jump::selected(read, base, state, current) &&
        current.actor == owner.actor && current.identity == owner.identity &&
        current.components == owner.components && current.control == owner.control &&
        read.template at<Address>(base + kClientRoot) == root &&
        read.template at<Address>(root + 0x30) == manager &&
        read.template at<Address>(manager + 0x50) == actor &&
        read.template at<Address>(control) == base + kControlVtable &&
        read.template at<Address>(state + 0x60) == control &&
        read.template at<Address>(control + 0x140) == controller &&
        read.template at<std::uint8_t>(controller + 0x2ed) == 0 &&
        read.template at<std::uint8_t>(state + 0xf0) == mode &&
        read.template at<Address>(state + 0xf8 + Address{mode} * 8) == method &&
        read.template at<Address>(method) == base + kNormalMethodVtable &&
        read.template at<Address>(method + 0x18) == control &&
        read.template at<Address>(method + 0x20) == actor;
}
template<class Read>
bool velocity(Read& read, Address base, Address controller, Address return_address,
              std::uint32_t percent, float dt, const float* input, float (&copy)[3]) {
    if (percent <= 100 || percent > 300 || !std::isfinite(dt) || dt <= 0 || !input ||
        !selected_ground(read, base, controller, return_address)) return false;
    const auto address = reinterpret_cast<Address>(input);
    for (unsigned i = 0; i < 3; ++i) {
        copy[i] = read.template at<float>(address + i * sizeof(float));
        if (!std::isfinite(copy[i])) return false;
    }
    const float factor = static_cast<float>(percent) / 100.0f;
    copy[0] *= factor;
    copy[2] *= factor;
    if (!std::isfinite(copy[0]) || !std::isfinite(copy[2])) return false;
    // Native input is borrowed and read-only. Only the call-local copy is
    // changed; original gravity, output collision data and time step survive.
    return selected_ground(read, base, controller, return_address);
}
} // namespace crimson::marketplace::player_speed
