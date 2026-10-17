#pragma once
#include <cmath>
#include "../stamina/stamina_scope.h"
#include "../force-palm-height/force_palm_height_scope.h"

namespace crimson::marketplace::player_jump {
using Address = std::uintptr_t;
struct Adjustment { Address actor{}, components{}, control{}; std::uint32_t identity{}; float before{}, after{}; };

template<class Read>
bool selected(Read& read, Address base, Address state, Adjustment& out) {
    using namespace stamina;
    if (!base || !state) return false;
    const auto root = read.template at<Address>(base + kClientRoot);
    if (!root) return false;
    const auto manager = read.template at<Address>(root + 0x30);
    if (!manager || read.template at<Address>(manager) != base + kClientManager) return false;
    const auto actor = read.template at<Address>(manager + 0x50);
    if (!actor || read.template at<Address>(actor) != base + kClientActor ||
        read.template at<std::uint8_t>(actor + 0x96) != 1) return false;
    const auto identity = read.template at<std::uint32_t>(actor + 0x60);
    const auto metadata = read.template at<Address>(actor + 0x88);
    if (!identity || !metadata) return false;
    const auto kind = read.template at<std::uint8_t>(metadata + 1);
    if (kind != 1 && kind != 4) return false;
    const auto components = read.template at<Address>(actor + 0x68);
    const auto count = read.template at<std::uint32_t>(actor + 0x70);
    const auto possessor = read.template at<Address>(actor + 0xa0);
    if (!components || count < 9 || count > 57 || !possessor ||
        read.template at<Address>(possessor + 0xd0) != actor) return false;
    const auto control = read.template at<Address>(components + 0x40);
    if (!control || read.template at<Address>(control + 8) != actor ||
        read.template at<Address>(control + 0xc8) != state || read.template at<Address>(state + 0x68) != actor) return false;
    out.actor = actor; out.identity = identity; out.components = components; out.control = control;
    return read.template at<Address>(base + kClientRoot) == root &&
        read.template at<Address>(root + 0x30) == manager &&
        read.template at<Address>(manager) == base + kClientManager &&
        read.template at<Address>(manager + 0x50) == actor &&
        read.template at<Address>(actor) == base + kClientActor &&
        read.template at<std::uint32_t>(actor + 0x60) == identity &&
        read.template at<std::uint8_t>(actor + 0x96) == 1 &&
        read.template at<Address>(actor + 0x88) == metadata &&
        read.template at<std::uint8_t>(metadata + 1) == kind &&
        read.template at<Address>(actor + 0x68) == components &&
        read.template at<std::uint32_t>(actor + 0x70) == count &&
        read.template at<Address>(actor + 0xa0) == possessor &&
        read.template at<Address>(possessor + 0xd0) == actor &&
        read.template at<Address>(components + 0x40) == control &&
        read.template at<Address>(control + 8) == actor &&
        read.template at<Address>(control + 0xc8) == state &&
        read.template at<Address>(state + 0x68) == actor;
}

template<class ReadWrite>
bool adjust(ReadWrite& memory, Address base, Address state, std::uint32_t percent, Adjustment& out) {
    if (percent <= 100 || percent > 300 || !selected(memory, base, state, out) ||
        force_palm_height::active_palm(memory,out.control) ||
        memory.template at<std::uint8_t>(state + 0x320) != 1 ||
        memory.template at<std::uint8_t>(state + 0x34d) != 0) return false;
    out.before = memory.template at<float>(state + 0x32c);
    out.after = out.before * (static_cast<float>(percent) / 100.0f);
    if (!std::isfinite(out.before) || out.before <= 0 || !std::isfinite(out.after) || out.after <= out.before) return false;
    // Recheck identity/ownership immediately before touching this borrowed state.
    Adjustment current;
    if (!selected(memory, base, state, current) || current.actor != out.actor || current.identity != out.identity ||
        force_palm_height::active_palm(memory,current.control) ||
        memory.template at<std::uint8_t>(state + 0x320) != 1 ||
        memory.template at<std::uint8_t>(state + 0x34d) != 0 ||
        memory.template at<float>(state + 0x32c) != out.before) return false;
    memory.put(state + 0x32c, out.after);
    return true;
}

// The original consumer normally clears the pending height on success. If it
// refuses the jump, restore only our still-owned, still-matching temporary edit.
template<class ReadWrite>
void restore(ReadWrite& memory, Address base, Address state, const Adjustment& previous) {
    (void)base;
    // Selection may have changed inside a callback. Restore this invocation's
    // borrowed owner, without following the new selection or retaining an address
    // beyond the original call. A replaced actor/control is never touched.
    if (memory.template at<Address>(state + 0x68) == previous.actor &&
        memory.template at<std::uint32_t>(previous.actor + 0x60) == previous.identity &&
        memory.template at<std::uint8_t>(previous.actor + 0x96) == 1 &&
        memory.template at<Address>(previous.actor + 0x68) == previous.components &&
        memory.template at<Address>(previous.components + 0x40) == previous.control &&
        memory.template at<Address>(previous.control + 8) == previous.actor &&
        memory.template at<Address>(previous.control + 0xc8) == state &&
        memory.template at<std::uint8_t>(state + 0x320) == 1 &&
        memory.template at<float>(state + 0x32c) == previous.after) memory.put(state + 0x32c, previous.before);
}
} // namespace crimson::marketplace::player_jump
