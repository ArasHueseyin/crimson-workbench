#pragma once
#include <Windows.h>
#include <cstring>
#include <type_traits>
#include "stamina_scope.h"
#include "../spirit/spirit_scope.h"
#include "../spirit/player_resource_settings.h"

namespace crimson::marketplace::stamina {
using Commit = std::uint64_t(__fastcall*)(void*, std::uint64_t, std::int64_t, std::uint16_t);
struct NativeRead {
    template<class T> T at(Address address) {
        static_assert(std::is_integral_v<T>);
        // Fresh integral loads keep repeated ownership/selection witnesses from
        // being reused by the optimizer. Native pointers are not cached.
        return *reinterpret_cast<const volatile T*>(address);
    }
};
template<class Read>
std::int64_t configured_target(Read& read, Address base, Address entry, std::int64_t target, std::uint32_t flags) {
    if (flags > (resources::kStamina | resources::kSpirit)) return target;
    if (flags & resources::kStamina) {
        const auto changed = protected_target(read, base, entry, target);
        if (changed != target) return changed;
    }
    return flags & resources::kSpirit ? spirit::protected_target(read, base, entry, target) : target;
}
inline std::int64_t guarded_target(Address base, void* entry, std::int64_t target,
                                   std::uint32_t flags = resources::kLegacyDefault) {
    __try {
        NativeRead read;
        return configured_target(read, base, reinterpret_cast<Address>(entry), target, flags);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return target; }
}
inline std::uint64_t dispatch_commit(Commit original, Address base, void* entry,
                                    std::uint64_t time, std::int64_t target, std::uint16_t flag,
                                    std::uint32_t flags = resources::kLegacyDefault) {
    return original(entry, time, guarded_target(base, entry, target, flags), flag);
}
} // namespace crimson::marketplace::stamina
