#pragma once
#include "repair_action.h"

namespace crimson::repair::item {
// Actual engine item lifecycle, not a raw memcpy of an Item's owning pointers.
// The host must bind a verified build's functions on its initialized engine
// thread. Binding non-null callbacks is NOT build/thread/lifetime admission.
struct Binding {
    void* (*construct)(void*) noexcept = nullptr;
    void* (*assign)(void*, const void*) noexcept = nullptr;
    void (*destroy)(void*) noexcept = nullptr;
};
enum class Code { constructed, copied, released, empty, invalid_binding,
    invalid_source, wrong_thread, already_constructed, native_failure, prepared, plan_mismatch };
enum class Realm { authority, presentation };

class Value {
    const Binding binding_;
    const std::thread::id thread_ = std::this_thread::get_id();
    // Word storage provides native alignment without relying on packed structs.
    std::array<std::uint64_t, action::item_size / sizeof(std::uint64_t)> storage_{};
    bool constructed_ = false;
    bool usable_ = false;
public:
    explicit Value(Binding binding) noexcept : binding_(binding) {}
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) = delete;
    Value& operator=(Value&&) = delete;
    ~Value();
    Code construct() noexcept;
    // Source must be a valid native Item, including its nested allocations,
    // pinned and protected for the full call. Detached action::Image bytes are
    // NOT such an Item. No pointer resolution or source access permission here.
    Code assign(const void* source) noexcept;
    // Construct an independent native AFTER value before any source stores.
    // Source must remain owned/locked/readable throughout this call. Its raw
    // image and initialized sockets must exactly match the selected plan entry.
    // Only the owned copy's endurance words change; unmaterialized logical
    // socket slots are never accessed. Does not send events or modify source.
    Code prepare_repair(const void* source, const action::Plan&, action::Identity,
        Realm = Realm::authority) noexcept;
    Code release() noexcept;
    // The copy remains valid after source owner locks are released. Native
    // notification must finish before release/destruction, on the same thread.
    const void* data() const noexcept;
};
}
