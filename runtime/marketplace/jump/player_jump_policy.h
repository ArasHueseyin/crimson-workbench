#pragma once
#include <Windows.h>
#include <cstring>
#include "player_jump_scope.h"

namespace crimson::marketplace::player_jump {
using Consume = bool(__fastcall*)(void*, std::uintptr_t, std::uint8_t, std::uint8_t);
struct Memory {
    template<class T> T at(Address address) {
        T value; std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value)); return value;
    }
    void put(Address address, float value) { std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(value)); }
};
inline bool guarded_adjust(Address base, void* state, std::uint32_t percent, Adjustment& out) {
    __try { Memory memory; return adjust(memory, base, reinterpret_cast<Address>(state), percent, out); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline void guarded_restore(Address base, void* state, const Adjustment& previous) {
    __try { Memory memory; restore(memory, base, reinterpret_cast<Address>(state), previous); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}
inline bool dispatch(Consume original, Address base, std::uint32_t percent,
                     void* state, std::uintptr_t arg2, std::uint8_t arg3, std::uint8_t arg4) {
    // Native callbacks can enter the consumer recursively. Never scale a pending
    // height again while the outer invocation is using it.
    static thread_local bool active = false;
    if (active) return original(state, arg2, arg3, arg4);
    Adjustment change;
    if (!guarded_adjust(base, state, percent, change)) return original(state, arg2, arg3, arg4);
    struct Invocation { bool& active; Invocation(bool& value) : active(value) { active = true; } ~Invocation() { active = false; } } invocation(active);
    const bool result = original(state, arg2, arg3, arg4);
    if (!result) guarded_restore(base, state, change);
    return result;
}
} // namespace crimson::marketplace::player_jump
