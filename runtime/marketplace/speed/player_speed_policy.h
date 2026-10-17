#pragma once
#include <Windows.h>
#include <cstring>
#include "player_speed_scope.h"
namespace crimson::marketplace::player_speed {
// Ground callers ignore RAX, but this shared native helper also has other
// callers. Preserve its full result register on every path, including fallback.
using Step = std::uintptr_t(__fastcall*)(void*, float, const float*, std::uintptr_t, void*, std::uintptr_t, std::uintptr_t);
struct Memory {
    template<class T> T at(Address address) {
        T value; std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value)); return value;
    }
};
inline bool guarded_velocity(Address base, void* controller, Address return_address,
                             std::uint32_t percent, float dt, const float* input, float (&copy)[3]) {
    __try { Memory memory; return velocity(memory, base, reinterpret_cast<Address>(controller), return_address, percent, dt, input, copy); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline std::uintptr_t dispatch(Step original, Address base, Address return_address, std::uint32_t percent,
                     void* controller, float dt, const float* velocity, std::uintptr_t flag,
                     void* output, std::uintptr_t arg6, std::uintptr_t arg7) {
    static thread_local bool active = false;
    float copy[3];
    if (active || !guarded_velocity(base, controller, return_address, percent, dt, velocity, copy)) {
        return original(controller, dt, velocity, flag, output, arg6, arg7);
    }
    struct Invocation { bool& active; Invocation(bool& value) : active(value) { active = true; } ~Invocation() { active = false; } } invocation(active);
    return original(controller, dt, copy, flag, output, arg6, arg7);
}
} // namespace crimson::marketplace::player_speed
