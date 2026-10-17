#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace crimson::marketplace::stamina {
using Address = std::uintptr_t;
inline constexpr Address kClientRoot = 0x6d69190;
inline constexpr Address kServerRoot = 0x6d696a0;
inline constexpr Address kStatusInfoManager = 0x6d69ae8;
inline constexpr Address kClientManager = 0x55b62c8;
inline constexpr Address kServerManager = 0x5b25988;
inline constexpr Address kClientActor = 0x5585c40;
inline constexpr Address kServerActor = 0x5b1e2b0;
inline constexpr Address kClientStatus = 0x558d868;
inline constexpr Address kServerStatus = 0x5b203f0;
inline constexpr std::size_t kEntrySize = 0x90;
inline constexpr std::uint32_t kStaminaKey = 1000026;
inline constexpr std::uint32_t kSpiritKey = 1000027;

struct Player {
    Address actor{}, components{}, possessor{}, status{}, vital{}, entries{};
    std::uint32_t identity{}, count{}, component_count{};
};

// Read only the current selected player's two native representations. The
// callback's entry is borrowed for this invocation; no address is cached.
template<class Read>
bool player(Read& read, Address base, Address actor, Address actor_type,
            Address status_type, std::uint32_t identity, Player& out) {
    if (!actor || read.template at<Address>(actor) != base + actor_type ||
        read.template at<std::uint32_t>(actor + 0x60) != identity ||
        read.template at<std::uint8_t>(actor + 0x96) != 1) return false;
    const auto metadata = read.template at<Address>(actor + 0x88);
    if (!metadata) return false;
    const auto selector = read.template at<std::uint8_t>(metadata + 1);
    if (selector != 1 && selector != 4) return false;
    const auto components = read.template at<Address>(actor + 0x68);
    const auto component_count = read.template at<std::uint32_t>(actor + 0x70);
    const auto possessor = read.template at<Address>(actor + 0xa0);
    if (!components || component_count < 24 || component_count > 57 || !possessor ||
        read.template at<Address>(possessor + 0xd0) != actor) return false;
    const auto holder = read.template at<Address>(components + 0xb8);
    const auto status = read.template at<Address>(components + 0x20);
    if (!holder || read.template at<Address>(holder + 8) != actor || !status ||
        read.template at<Address>(status) != base + status_type ||
        read.template at<Address>(status + 8) != actor) return false;
    const auto vital = read.template at<Address>(status + 0x18);
    if (!vital || read.template at<Address>(vital) != status) return false;
    const auto entries = read.template at<Address>(vital + 0x58);
    const auto count = read.template at<std::uint32_t>(vital + 0x60);
    if (!entries || count == 0 || count > 4096) return false;
    out = {actor, components, possessor, status, vital, entries, identity, count, component_count};
    return true;
}

template<class Read>
bool stable(Read& read, const Player& value) {
    return read.template at<std::uint32_t>(value.actor + 0x60) == value.identity &&
        read.template at<std::uint8_t>(value.actor + 0x96) == 1 &&
        read.template at<Address>(value.actor + 0x68) == value.components &&
        read.template at<std::uint32_t>(value.actor + 0x70) == value.component_count &&
        read.template at<Address>(value.actor + 0xa0) == value.possessor &&
        read.template at<Address>(value.possessor + 0xd0) == value.actor &&
        read.template at<Address>(value.components + 0x20) == value.status &&
        read.template at<Address>(value.status + 8) == value.actor &&
        read.template at<Address>(value.status + 0x18) == value.vital &&
        read.template at<Address>(value.vital) == value.status &&
        read.template at<Address>(value.vital + 0x58) == value.entries &&
        read.template at<std::uint32_t>(value.vital + 0x60) == value.count;
}

inline bool contains(const Player& value, Address entry) {
    return entry >= value.entries && (entry - value.entries) % kEntrySize == 0 &&
        (entry - value.entries) / kEntrySize < value.count;
}

template<class Read>
bool selected_resource(Read& read, Address base, Address entry, std::uint32_t resource_key) {
    if (!base || !entry || (resource_key != kStaminaKey && resource_key != kSpiritKey)) return false;
    // Resolve the semantic key, not a guessed integer enum. In build 2976,
    // index 22 is Stamina and index 23 is Mp (spirit). A resolved ordinal is
    // never substituted for the semantic key; Fullness/Relief/Breath differ.
    const auto index = read.template at<std::uint16_t>(entry);
    const auto definitions = read.template at<Address>(base + kStatusInfoManager);
    if (!definitions) return false;
    const auto definition_count = read.template at<std::uint32_t>(definitions + 8);
    const auto rows = read.template at<Address>(definitions + 0x58);
    if (!rows || !definition_count || definition_count > 4096 || index >= definition_count) return false;
    const auto row = read.template at<Address>(rows + static_cast<Address>(index) * 8);
    if (!row || read.template at<std::uint32_t>(row) != resource_key ||
        read.template at<std::uint8_t>(row + 0x10) != 0 ||
        read.template at<std::uint32_t>(row + 0x14) != index ||
        read.template at<std::uint8_t>(row + 0x11) != 2) return false;
    const auto client_root = read.template at<Address>(base + kClientRoot);
    const auto server_root = read.template at<Address>(base + kServerRoot);
    if (!client_root || !server_root) return false;
    const auto cm = read.template at<Address>(client_root + 0x30);
    const auto sm = read.template at<Address>(server_root + 0x48);
    if (!cm || !sm || read.template at<Address>(cm) != base + kClientManager ||
        read.template at<Address>(sm) != base + kServerManager) return false;
    const auto client = read.template at<Address>(cm + 0x50);
    if (!client) return false;
    const auto identity = read.template at<std::uint32_t>(client + 0x60);
    if (!identity) return false;
    Player c{}, s{};
    if (!player(read, base, client, kClientActor, kClientStatus, identity, c)) return false;
    const auto buckets = read.template at<std::uint32_t>(sm + 0x98);
    const auto bucket_array = read.template at<Address>(sm + 0xa8);
    const auto nodes = read.template at<Address>(sm + 0xb0);
    if (!buckets || buckets > 1'000'000 || !bucket_array || !nodes) return false;
    const auto key = identity & 0xfffff;
    const auto bucket = bucket_array + (key % buckets) * Address{0x100};
    const auto bucket_count = read.template at<std::uint32_t>(bucket);
    if (bucket_count > 31) return false;
    Address server{}, selected_node{};
    std::uint32_t selected_index{};
    for (std::uint32_t i = 0; i < bucket_count; ++i) {
        const auto item = bucket + 8 + i * Address{8};
        if (read.template at<std::uint32_t>(item) != key) continue;
        const auto node_index = read.template at<std::uint32_t>(item + 4);
        if (node_index >= 1'000'000 || server) return false;
        const auto node = read.template at<Address>(nodes + node_index * Address{8});
        if (!node || read.template at<std::uint32_t>(node + 4) != key) return false;
        server = read.template at<Address>(node + 8);
        selected_node = node;
        selected_index = node_index;
    }
    if (!player(read, base, server, kServerActor, kServerStatus, identity, s) ||
        (!contains(c, entry) && !contains(s, entry))) return false;
    return stable(read, c) && stable(read, s) &&
        read.template at<Address>(base + kClientRoot) == client_root &&
        read.template at<Address>(base + kServerRoot) == server_root &&
        read.template at<Address>(client_root + 0x30) == cm &&
        read.template at<Address>(server_root + 0x48) == sm &&
        read.template at<Address>(cm + 0x50) == client &&
        read.template at<std::uint32_t>(sm + 0x98) == buckets &&
        read.template at<Address>(sm + 0xa8) == bucket_array &&
        read.template at<Address>(sm + 0xb0) == nodes &&
        read.template at<std::uint32_t>(bucket) == bucket_count &&
        read.template at<Address>(nodes + selected_index * Address{8}) == selected_node &&
        read.template at<std::uint32_t>(selected_node + 4) == key &&
        read.template at<Address>(selected_node + 8) == server &&
        read.template at<Address>(base + kStatusInfoManager) == definitions &&
        read.template at<std::uint32_t>(definitions + 8) == definition_count &&
        read.template at<Address>(definitions + 0x58) == rows &&
        read.template at<Address>(rows + static_cast<Address>(index) * 8) == row &&
        read.template at<std::uint32_t>(row) == resource_key &&
        read.template at<std::uint8_t>(row + 0x10) == 0 &&
        read.template at<std::uint8_t>(row + 0x11) == 2 &&
        read.template at<std::uint32_t>(row + 0x14) == index &&
        read.template at<std::uint16_t>(entry) == index;
}

template<class Read>
bool selected_stamina(Read& read, Address base, Address entry) {
    return selected_resource(read, base, entry, kStaminaKey);
}

template<class Read>
std::int64_t protected_resource_target(Read& read, Address base, Address entry,
                                       std::int64_t target, std::uint32_t key) {
    if (!entry) return target;
    const auto current = read.template at<std::int64_t>(entry + 8);
    if (current <= 0 || target >= current) return target;
    // Native commit clamps against +18 + +20 and +28. Preserve reductions
    // needed to restore valid bounds; never manufacture a larger resource cap.
    const auto maximum_base = read.template at<std::int64_t>(entry + 0x18);
    const auto maximum_extra = read.template at<std::int64_t>(entry + 0x20);
    const auto minimum = read.template at<std::int64_t>(entry + 0x28);
    if ((maximum_extra > 0 && maximum_base > (std::numeric_limits<std::int64_t>::max)() - maximum_extra) ||
        (maximum_extra < 0 && maximum_base < (std::numeric_limits<std::int64_t>::min)() - maximum_extra)) return target;
    const auto maximum = maximum_base + maximum_extra;
    if (minimum < 0 || maximum <= minimum || current < minimum || current > maximum ||
        !selected_resource(read, base, entry, key)) return target;
    return read.template at<std::int64_t>(entry + 8) == current &&
        read.template at<std::int64_t>(entry + 0x18) == maximum_base &&
        read.template at<std::int64_t>(entry + 0x20) == maximum_extra &&
        read.template at<std::int64_t>(entry + 0x28) == minimum ? current : target;
}

template<class Read>
std::int64_t protected_target(Read& read, Address base, Address entry, std::int64_t target) {
    return protected_resource_target(read, base, entry, target, kStaminaKey);
}
} // namespace crimson::marketplace::stamina
