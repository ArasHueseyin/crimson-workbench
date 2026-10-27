#pragma once
#include "protocol.h"
#include "inventory.h"
#include "prepare.h"
#include <limits>
#include <optional>

namespace crimson::mac_items {
struct NativeItem { std::uint16_t type{}, destination_type{}; std::uint32_t destination_key{}; bool stackable{}; };
template<class Read>
std::optional<NativeItem> resolve_item(std::uint32_t key, std::intptr_t slide, Read read) {
    auto field = [&](std::uintptr_t object, std::uintptr_t offset, std::uintptr_t& value) {
        return object && object <= std::numeric_limits<std::uintptr_t>::max()-offset && read(object+offset, value);
    };
    std::uintptr_t manager{}, count{}, definitions{};
    if (!read(0x108590c20ULL+slide, manager) || !field(manager, 8, count)
        || !field(manager, 0x58, definitions)) return {};
    count &= 0xffffffff;
    if (!count || count > 65535) return {};
    for (unsigned i = 0; i < count; ++i) {
        std::uintptr_t definition{}, external{}, destination{}, stackable{};
        if (!field(definitions, i*8, definition)) return {};
        if (!definition) continue;
        if (!field(definition, 0, external)) return {};
        if (static_cast<std::uint32_t>(external) != key) continue;
        if (!field(definition, 0x428, destination) || !field(definition, 0x3f0, stackable)) return {};
        NativeItem item{static_cast<std::uint16_t>(i), static_cast<std::uint16_t>(destination), 0, (stackable & 0xff) == 1};
        if (!read(0x1085cb7a0ULL+slide, manager) || !field(manager, 8, count)
            || !field(manager, 0x58, definitions) || (count & 0xffffffff) <= item.destination_type
            || !field(definitions, item.destination_type*8, definition) || !field(definition, 0, external)) return {};
        item.destination_key = static_cast<std::uint16_t>(external);
        return item;
    }
    return {};
}

struct Execution { crimson::live_items::State state; crimson::live_items::Error error; };
template<class Read, class Grant, class Recheck>
Execution execute_item(ItemBindings bindings, Grant grant, Recheck recheck, std::uintptr_t holder,
    std::uint32_t key, std::uint32_t quantity, std::intptr_t slide, Read read) {
    using crimson::live_items::State;
    using crimson::live_items::Error;
    if (!key || !quantity || quantity > 10000) return {State::rejected, Error::invalid_request};
    const auto item = resolve_item(key, slide, read);
    if (!item) return {State::rejected, Error::unsupported_item};
    // Native InventoryKey 2 is the verified Character inventory; special storage is not a grant destination.
    if (item->destination_key != 2) return {State::rejected, Error::special_storage};
    if (!item->stackable && quantity > 100) return {State::rejected, Error::quantity_limit};
    std::uintptr_t buckets{}, first{}, inventory_type{};
    if (!read(holder+0x18, buckets) || !read(buckets, first) || !read(first+0x10, inventory_type)
        || static_cast<std::uint16_t>(inventory_type) != item->destination_type) return {State::rejected, Error::context};
    const auto before = count_items(holder, item->type, read, 0);
    const auto total_before = count_items(holder, item->type, read);
    if (!before.valid || !total_before.valid) return {State::rejected, Error::context};
    if (total_before.quantity > 1000000000-quantity) return {State::rejected, Error::quantity_limit};
    alignas(16) unsigned char init[0x1c0]{};
    if (!prepare_item(bindings, item->type, quantity, init)) return {State::rejected, Error::unsupported_value};
    if (value_field<std::int64_t>(init, 0) != -1 || value_field<std::uint32_t>(init, 8) != key
        || value_field<std::int64_t>(init, 0x10) != quantity) return {State::rejected, Error::conversion_mismatch};
    if (!recheck()) return {State::rejected, Error::context};
    if (grant(holder, init) != 0) return {State::uncertain, Error::native_error};
    const auto after = count_items(holder, item->type, read, 0);
    const auto total_after = count_items(holder, item->type, read);
    if (!after.valid || !total_after.valid || after.quantity-before.quantity != quantity
        || total_after.quantity-total_before.quantity != quantity) return {State::uncertain, Error::inventory_mismatch};
    return {State::applied, Error::none};
}
}
