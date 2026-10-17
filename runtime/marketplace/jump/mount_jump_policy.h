#pragma once
#include <Windows.h>
#include "mount_jump_scope.h"

namespace crimson::marketplace::jump {
using Event = void*(__fastcall*)(void*, void*, const void*, std::uintptr_t, const void*);
struct NativeRead {
    template<class T> T at(Address address) {
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
        return value;
    }
    void copy(Address address, void* output, std::size_t count) {
        std::memcpy(output, reinterpret_cast<const void*>(address), count);
    }
};
inline bool guarded_prepare(Address base, const void* event, const void* actor, Prepared& out) {
    __try {
        NativeRead read;
        return prepare(read, base, reinterpret_cast<Address>(event), reinterpret_cast<Address>(actor), out);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline void* dispatch(Event original, Address base, void* self, void* result,
                      const void* event, std::uintptr_t time, const void* actor) {
    Prepared prepared;
    const bool change = guarded_prepare(base, event, actor, prepared);
    return original(self, result, change ? prepared.reference.data() : event, time, actor);
}
} // namespace crimson::marketplace::jump
