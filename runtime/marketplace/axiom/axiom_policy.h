#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "axiom_settings.h"

namespace crimson::marketplace::axiom {
using Address = std::uintptr_t;
inline constexpr Address kCrtState = 0x6c88690;
inline constexpr Address kFloatTuningType = 0x5715be8;
inline constexpr Address kClientRoot = 0x6d69190;
inline constexpr Address kClientManager = 0x55b62c8;
inline constexpr Address kClientActor = 0x5585c40;
struct Tuning { Address record; std::uint32_t key; float normal; };
inline constexpr std::array<Tuning, 4> kTuning{{
    {0x6c97870, 0x6dce9b68, 20.0f}, // RemoteCatch acquisition and held reach.
    {0x6c978c0, 0x8a220e76, 32.0f}, // Native special/type-5 RemoteCatch reach.
    {0x6c97960, 0xd59ce7f6, 3.0f},  // Horizontal release tolerance.
    {0x6c979b0, 0xe3381094, 3.0f},  // Vertical release tolerance.
}};
inline std::uint32_t bits(float value) {
    std::uint32_t result{}; std::memcpy(&result, &value, sizeof(result)); return result;
}
enum class Result { Normal, Waiting, Applied, Conflict, RolledBack, RollbackConflict };

// Static constructors and tuning-override loading complete before a selected
// world exists. No actor address is retained or written by this module.
template<class Memory> bool world_ready(Memory& memory, Address base) {
    std::uint32_t crt{}, identity{};
    Address root{}, manager{}, actor{}, type{}, possessor{}, backlink{};
    std::uint8_t alive{};
    if (!base || !memory.read(base + kCrtState, crt) || crt != 2 ||
        !memory.read(base + kClientRoot, root) || !root ||
        !memory.read(root + 0x30, manager) || !manager ||
        !memory.read(manager, type) || type != base + kClientManager ||
        !memory.read(manager + 0x50, actor) || !actor ||
        !memory.read(actor, type) || type != base + kClientActor ||
        !memory.read(actor + 0x60, identity) || !identity ||
        !memory.read(actor + 0x96, alive) || alive != 1 ||
        !memory.read(actor + 0xa0, possessor) || !possessor ||
        !memory.read(possessor + 0xd0, backlink) || backlink != actor) return false;
    Address again{};
    return memory.read(manager + 0x50, again) && again == actor;
}
template<class Memory> bool unchanged_record(Memory& memory, Address base, const Tuning& tuning) {
    Address type{}; std::uint32_t key{}, original{};
    return memory.read(base + tuning.record, type) && type == base + kFloatTuningType &&
        memory.read(base + tuning.record + 0x28, key) && key == tuning.key &&
        memory.read(base + tuning.record + 0x3c, original) && original == bits(tuning.normal);
}
template<class Memory> Result apply(Memory& memory, Address base, std::uint32_t percent) {
    if (!valid_percent(percent)) return Result::Conflict;
    if (percent == 100) return Result::Normal;
    if (!world_ready(memory, base)) return Result::Waiting;
    std::array<std::uint32_t, kTuning.size()> after{};
    // Inspect all records before touching any one of them. Never multiply a
    // previous value or silently combine another mod's tuning with this one.
    for (std::size_t i = 0; i < kTuning.size(); ++i) {
        std::uint32_t current{};
        if (!unchanged_record(memory, base, kTuning[i]) ||
            !memory.read(base + kTuning[i].record + 0x38, current) ||
            current != bits(kTuning[i].normal)) return Result::Conflict;
        after[i] = bits(kTuning[i].normal * (static_cast<float>(percent) / 100.0f));
    }
    if (!world_ready(memory, base)) return Result::Waiting;
    std::size_t written = 0;
    for (; written < kTuning.size(); ++written) {
        if (!unchanged_record(memory, base, kTuning[written]) ||
            !memory.compare_exchange(base + kTuning[written].record + 0x38,
                bits(kTuning[written].normal), after[written])) break;
    }
    if (written == kTuning.size()) {
        bool complete = true;
        for (std::size_t i = 0; i < kTuning.size(); ++i) {
            std::uint32_t current{};
            if (!unchanged_record(memory, base, kTuning[i]) ||
                !memory.read(base + kTuning[i].record + 0x38, current) || current != after[i]) complete = false;
        }
        if (complete) return Result::Applied;
    }
    bool restored = true;
    while (written) {
        --written;
        // A concurrent writer owns any new value: never overwrite it to make
        // a rollback appear successful.
        if (!memory.compare_exchange(base + kTuning[written].record + 0x38,
            after[written], bits(kTuning[written].normal))) restored = false;
    }
    return restored ? Result::RolledBack : Result::RollbackConflict;
}
} // namespace crimson::marketplace::axiom
