#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace crimson::mac_items {
template<class T>
T value_field(const void* value, unsigned offset) {
    T result;
    std::memcpy(&result, static_cast<const unsigned char*>(value)+offset, sizeof(result));
    return result;
}

struct ItemBindings {
    void* (*construct)(void*, const std::uint16_t*, std::int64_t);
    void* (*initialize)(void*);
    void (*serialize)(const void*, std::uint16_t, void*);
    void* (*destroy)(void*);
};

inline bool supported_value(const void* value) {
    const auto active = value_field<std::uint8_t>(value, 0x70);
    const auto backing = value_field<std::uint32_t>(value, 0x68);
    const auto extra = value_field<std::uint32_t>(value, 0x80);
    const auto properties = value_field<std::uint32_t>(value, 0xb0);
    return active <= 5 && backing <= 5 && active <= backing
        && backing <= value_field<std::uint32_t>(value, 0x6c)
        && (!backing || value_field<std::uintptr_t>(value, 0x60))
        && extra <= 12 && extra <= value_field<std::uint32_t>(value, 0x84)
        && (!extra || value_field<std::uintptr_t>(value, 0x78))
        && properties <= 3 && properties <= value_field<std::uint32_t>(value, 0xb4)
        && (!properties || value_field<std::uintptr_t>(value, 0xa8));
}

inline void* prepare_item(ItemBindings bindings, std::uint16_t type, std::int64_t quantity, void* init) {
    alignas(16) unsigned char value[0xc8]{};
    if (quantity < 1 || quantity > 10000 || bindings.construct(value, &type, quantity) != value) return nullptr;
    struct Cleanup {
        ItemBindings bindings;
        void* value;
        ~Cleanup() { bindings.destroy(value); }
    } cleanup{bindings, value};
    std::fprintf(stderr, "CW_MAC_ITEMS: prepared value type=%u quantity=%lld backing=%u active=%u extra=%u properties=%u.\n",
        value_field<std::uint16_t>(value, 8), static_cast<long long>(value_field<std::int64_t>(value, 0x10)),
        value_field<std::uint32_t>(value, 0x68), value_field<std::uint8_t>(value, 0x70),
        value_field<std::uint32_t>(value, 0x80), value_field<std::uint32_t>(value, 0xb0));
    const bool valid = value_field<std::int64_t>(value, 0) == -1
        && value_field<std::uint16_t>(value, 8) == type
        && value_field<std::int64_t>(value, 0x10) == quantity && supported_value(value);
    if (valid && bindings.initialize(init) == init)
        bindings.serialize(value, 0xffff, init);
    else {
        return nullptr;
    }
    return init;
}
}
