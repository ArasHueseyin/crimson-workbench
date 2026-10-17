#pragma once
#include <Windows.h>
#include <cstring>
#include "force_palm_scope.h"

namespace crimson::marketplace::force_palm {
// 242d370 forwards all seven parameters through recursive evaluation. AL is a
// tri-state condition result: 0 match, 1 false, 2 invalid. The third argument
// is the caller's frame context; its exact contents remain native-owned.
using Evaluate = std::uint8_t(__fastcall*)(void*,void*,void*,std::uint8_t,
                                          std::uint32_t*,std::uint8_t*,void*);
struct Memory {
    template<class T> T at(Address address) {
        // Native callbacks may switch actors or reload script data. Volatile
        // byte reads force each snapshot to observe memory again, including
        // unaligned scalars in the serialized native chart buffers.
        T value;
        const auto source=reinterpret_cast<const volatile unsigned char*>(address);
        const auto destination=reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i=0;i<sizeof(value);++i) destination[i]=source[i];
        return value;
    }
};
inline bool guarded_snapshot(Address base,void* descriptor,void* control,
                             std::uint32_t* cursor,Snapshot& out) {
    __try {
        Memory memory;
        return snapshot(memory,base,reinterpret_cast<Address>(descriptor),
                        reinterpret_cast<Address>(control),reinterpret_cast<Address>(cursor),out);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline std::uint8_t dispatch(Evaluate original,Address base,bool enabled,
    void* descriptor,void* control,void* frame,std::uint8_t flag,
    std::uint32_t* cursor,std::uint8_t* negated,void* result) {
    Snapshot before;
    const bool candidate=enabled && guarded_snapshot(base,descriptor,control,cursor,before);
    // Native comparison executes exactly once, including its native diagnostics,
    // cache and reference management. Never catch or repeat a native exception.
    const auto native=original(descriptor,control,frame,flag,cursor,negated,result);
    if (!candidate || native!=1) return native;
    Snapshot after;
    return guarded_snapshot(base,descriptor,control,cursor,after) && before==after ? 0 : native;
}
} // namespace crimson::marketplace::force_palm
