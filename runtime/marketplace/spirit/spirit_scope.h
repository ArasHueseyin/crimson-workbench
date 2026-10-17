#pragma once
#include "../stamina/stamina_scope.h"

namespace crimson::marketplace::spirit {
using stamina::Address;
inline constexpr auto kSpiritKey = stamina::kSpiritKey;

template<class Read>
bool selected_spirit(Read& read, Address base, Address entry) {
    return stamina::selected_resource(read, base, entry, kSpiritKey);
}

template<class Read>
std::int64_t protected_target(Read& read, Address base, Address entry, std::int64_t target) {
    return stamina::protected_resource_target(read, base, entry, target, kSpiritKey);
}
} // namespace crimson::marketplace::spirit
