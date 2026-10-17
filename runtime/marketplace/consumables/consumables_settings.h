#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "consumables_catalog.h"

namespace crimson::marketplace::consumables {
struct Settings {
    std::array<std::uint32_t, kItems.size()> keys{};
    std::uint32_t count{};
    bool contains(std::uint32_t key) const {
        if (count>keys.size()) return false;
        for (std::uint32_t i=0; i<count; ++i) if (keys[i] == key) return true;
        return false;
    }
};
inline Settings setting(const unsigned char* bytes, std::size_t length) {
    Settings result{};
    if (!bytes || length < 8 || std::memcmp(bytes, "CWC1", 4)) return result;
    std::uint32_t count{}; std::memcpy(&count, bytes+4, 4);
    if (count > kItems.size() || length != 8 + std::size_t{count}*4) return result;
    for (std::uint32_t i=0; i<count; ++i) {
        std::uint32_t key{}; std::memcpy(&key, bytes+8+i*4, 4);
        if (!supported(key) || (i && key <= result.keys[i-1])) return {};
        result.keys[i] = key;
    }
    result.count = count;
    return result;
}
} // namespace crimson::marketplace::consumables
