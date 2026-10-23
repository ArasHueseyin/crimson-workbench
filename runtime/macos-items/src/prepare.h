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

inline void* prepare_timber(ItemBindings bindings, std::uint16_t type, void* init) {
    alignas(16) unsigned char value[0xc8]{};
    if (bindings.construct(value, &type, 1) != value) return nullptr;
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
        && value_field<std::int64_t>(value, 0x10) == 1
        // This first experiment accepts only a plain material with no active inline lists.
        && value_field<std::uint32_t>(value, 0x68) <= 5
        && value_field<std::uint8_t>(value, 0x70) == 0
        && value_field<std::uint32_t>(value, 0x80) == 0
        && value_field<std::uint32_t>(value, 0xb0) == 0;
    if (valid && bindings.initialize(init) == init)
        bindings.serialize(value, 0xffff, init);
    else {
        return nullptr;
    }
    return init;
}
}
