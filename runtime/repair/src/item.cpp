#include "repair_item.h"
#include <exception>
#include <limits>
#include <cstring>
#include <algorithm>

namespace crimson::repair::item {
Value::~Value() {
    if (constructed_ && release() == Code::wrong_thread) std::terminate();
}
Code Value::construct() noexcept {
    if (thread_ != std::this_thread::get_id()) return Code::wrong_thread;
    if (constructed_) return Code::already_constructed;
    if (!binding_.construct || !binding_.assign || !binding_.destroy) return Code::invalid_binding;
    const auto result = binding_.construct(storage_.data());
    // Even a malformed return cannot suppress cleanup of the constructed item.
    constructed_ = true;
    usable_ = result == storage_.data();
    return usable_ ? Code::constructed : Code::native_failure;
}
Code Value::assign(const void* source) noexcept {
    if (thread_ != std::this_thread::get_id()) return Code::wrong_thread;
    if (!constructed_) return Code::empty;
    if (!usable_) return Code::native_failure;
    const auto address = reinterpret_cast<std::uintptr_t>(source);
    const auto own = reinterpret_cast<std::uintptr_t>(storage_.data());
    if (!address || address % alignof(std::uint64_t) ||
        address > std::numeric_limits<std::uintptr_t>::max() - action::item_size ||
        (address != own && address < own + action::item_size && own < address + action::item_size))
        return Code::invalid_source;
    // Validate the owning vector headers before the native assignment clears
    // its destination or allocates from source capacity. The caller still owns
    // source readability/lifetime; these checks cannot validate an arbitrary pointer.
    const auto read = [source]<class T>(std::size_t at) {
        T value; std::memcpy(&value, static_cast<const std::uint8_t*>(source) + at, sizeof(T)); return value;
    };
    for (const auto at : {std::size_t(0x60), std::size_t(0x78), std::size_t(0xa8)}) {
        const auto count = read.operator()<std::uint32_t>(at + 8);
        const auto capacity = read.operator()<std::uint32_t>(at + 12);
        const auto pointer = read.operator()<std::uintptr_t>(at);
        if (count > capacity || (capacity && !pointer) || (pointer && pointer % 2)) return Code::invalid_source;
    }
    const auto sockets = read.operator()<std::uint32_t>(0x68);
    const auto limit = read.operator()<std::uint8_t>(0x70);
    if (sockets > limit) return Code::invalid_source;
    usable_ = binding_.assign(storage_.data(), source) == storage_.data();
    return usable_ ? Code::copied : Code::native_failure;
}
Code Value::prepare_repair(const void* source, const action::Plan& plan, action::Identity identity, Realm realm) noexcept {
    if (thread_ != std::this_thread::get_id()) return Code::wrong_thread;
    if (!constructed_) return Code::empty;
    if (!usable_) return Code::native_failure;
    if (!plan.prepared() || (realm != Realm::authority && realm != Realm::presentation)) return Code::plan_mismatch;
    const auto found = std::find_if(plan.changes().begin(), plan.changes().end(), [&](const auto& change) {
        return change.before.identity == identity;
    });
    if (found == plan.changes().end()) return Code::plan_mismatch;
    const auto& before = realm == Realm::authority ? found->before.authority : found->before.presentation;
    const auto& after = realm == Realm::authority ? found->after.authority : found->after.presentation;
    const auto address = reinterpret_cast<std::uintptr_t>(source);
    if (address < 65536 || address % 8 || address > UINTPTR_MAX - action::item_size || source == data()) return Code::invalid_source;
    const auto field = []<class T>(const void* p, std::size_t at) {
        T out; std::memcpy(&out, static_cast<const std::uint8_t*>(p) + at, sizeof(out)); return out;
    };
    const auto count = field.operator()<std::uint32_t>(before.bytes.data(), 0x68);
    if (count != before.sockets.size() || count != after.sockets.size() ||
        std::memcmp(source, before.bytes.data(), action::item_size)) return Code::plan_mismatch;
    const auto* original_sockets = field.operator()<const std::uint8_t*>(source, 0x60);
    const auto sockets_address = reinterpret_cast<std::uintptr_t>(original_sockets);
    if (count > field.operator()<std::uint32_t>(source, 0x6c) || (count &&
        (sockets_address < 65536 || sockets_address % 2 || sockets_address > UINTPTR_MAX - count * action::socket_size)))
        return Code::invalid_source;
    if (count && (!original_sockets || std::memcmp(original_sockets, before.sockets.data(), count * action::socket_size)))
        return Code::plan_mismatch;
    const auto status = assign(source);
    if (status != Code::copied) return status;
    auto* copy = reinterpret_cast<std::uint8_t*>(storage_.data());
    auto* sockets = field.operator()<std::uint8_t*>(copy, 0x60);
    // Native assignment preserves meaningful scalar bytes but may normalize
    // padding, capacities and owning pointers. Compare its semantic copy first.
    bool matches = copy[0x70] == before.bytes[0x70] && field.operator()<std::uint32_t>(copy, 0x68) == count &&
        field.operator()<std::uint32_t>(copy, 0x6c) >= count;
    for (const auto& [at, length] : std::array<std::pair<std::size_t, std::size_t>, 5>{{
        {0, 12}, {0x10, 0x32}, {0x48, 0x12}, {0x88, 10}, {0x98, 10}}})
        matches &= std::memcmp(copy + at, before.bytes.data() + at, length) == 0;
    for (const auto at : {std::size_t(0x60), std::size_t(0x78), std::size_t(0xa8), std::size_t(0xb8), std::size_t(0xc0)}) {
        const auto pointer = field.operator()<std::uintptr_t>(copy, at);
        if (pointer && pointer == field.operator()<std::uintptr_t>(source, at)) matches = false;
    }
    if (count) {
        const auto p = reinterpret_cast<std::uintptr_t>(sockets);
        if (!p || p % 2 || p > UINTPTR_MAX - count * action::socket_size || sockets == original_sockets) matches = false;
        else matches &= std::memcmp(sockets, before.sockets.data(), count * action::socket_size) == 0;
    }
    if (!matches) { usable_ = false; return Code::native_failure; }
    std::memcpy(copy + 0x40, after.bytes.data() + 0x40, 2);
    for (std::size_t i = 0; i < count; ++i)
        std::memcpy(sockets + i * action::socket_size + 2, after.sockets[i].bytes.data() + 2, 2);
    return Code::prepared;
}
Code Value::release() noexcept {
    if (thread_ != std::this_thread::get_id()) return Code::wrong_thread;
    if (!constructed_) return Code::empty;
    binding_.destroy(storage_.data());
    constructed_ = false;
    usable_ = false;
    storage_.fill(0);
    return Code::released;
}
const void* Value::data() const noexcept {
    return constructed_ && usable_ && thread_ == std::this_thread::get_id() ? storage_.data() : nullptr;
}
}
