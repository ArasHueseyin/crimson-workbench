#pragma once
#include <Windows.h>
#include <cstring>
#include <type_traits>
#include "mount_protection_scope.h"
namespace crimson::marketplace::mount_protection {
using Delta = std::uintptr_t(__fastcall*)(void*,std::uint16_t,std::uint64_t,std::int64_t,void*);
struct Memory {
    template<class T>T at(Address address) {
        static_assert(std::is_integral_v<T>);
        // Each validation pass must issue fresh native reads.
        return *reinterpret_cast<const volatile T*>(address);
    }
};
inline std::int64_t guarded_delta(Address base,void* owner,std::uint16_t index,std::int64_t delta) {
    __try {Memory memory;return protected_delta(memory,base,reinterpret_cast<Address>(owner),index,delta);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return delta;}
}
inline std::uintptr_t dispatch(Delta original,Address base,void* owner,std::uint16_t index,
                               std::uint64_t time,std::int64_t delta,void* output) {
    return original(owner,index,time,guarded_delta(base,owner,index,delta),output);
}
} // namespace crimson::marketplace::mount_protection
