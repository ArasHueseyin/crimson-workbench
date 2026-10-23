#pragma once
#include <bit>
#include <cstdint>
#include <limits>

namespace crimson::mac_items {
struct ItemCount { bool valid{}; std::int64_t quantity{}; };

// Bucket 0 was calibrated against Kliff's inventory; totals include other allocated storage.
template<class Read>
ItemCount count_items(std::uintptr_t holder, std::uint16_t type, Read read,
    std::uint32_t bucket_index = UINT32_MAX) {
    auto field = [&](std::uintptr_t object, std::uintptr_t offset, std::uintptr_t& value) {
        return object && object <= std::numeric_limits<std::uintptr_t>::max() - offset
            && read(object + offset, value);
    };
    std::uintptr_t buckets{}, count{};
    if (!field(holder, 0x18, buckets) || !field(holder, 0x20, count)) return {};
    count &= 0xffffffff;
    if (!count || count > 128) return {};
    if (bucket_index != UINT32_MAX && bucket_index >= count) return {};
    std::int64_t total = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (bucket_index != UINT32_MAX && i != bucket_index) continue;
        std::uintptr_t bucket{}, slots{}, size{};
        if (!field(buckets, i*8, bucket)) return {};
        if (!bucket) continue;
        if (!field(bucket, 0, slots) || !field(bucket, 8, size)) return {};
        size &= 0xffffffff;
        if (size > 16384 || (size && !slots)) return {};
        for (unsigned n = 0; n < size; ++n) {
            std::uintptr_t key{}, quantity{};
            if (!field(slots, n*0xc8+8, key) || !field(slots, n*0xc8+0x10, quantity)) return {};
            if ((key & 0xffff) != type) continue;
            const auto amount = std::bit_cast<std::int64_t>(quantity);
            if (amount < 0 || amount > 1000000000 || total > 1000000000-amount) return {};
            total += amount;
        }
    }
    return {true, total};
}
}
