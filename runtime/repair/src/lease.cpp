#include "repair_lease.h"
#include <algorithm>
#include <exception>
#include <functional>

namespace crimson::repair::lease {
Group::~Group() {
    // Releasing an SRW lock on another thread violates the native contract.
    // Treat such host misuse as a programming error rather than unlock foreign
    // state. Public release() reports wrong_thread and keeps ownership intact.
    if (size_ && release() == Code::wrong_thread) std::terminate();
}
Code Group::try_acquire(std::span<const Binding> bindings) noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (size_) return Code::already_acquired;
    if (bindings.empty() || bindings.size() > held_.size()) return Code::invalid_binding;
    std::array<Binding, 2> ordered{};
    std::size_t count = 0;
    for (const auto& b : bindings) {
        if (!b.object || !b.try_acquire || !b.release) return Code::invalid_binding;
        if (count && ordered[0].object == b.object) {
            if (ordered[0] != b) return Code::invalid_binding;
            continue; // both realms may share one synchronization object
        }
        ordered[count++] = b;
    }
    if (count == 2 && std::less<void*>{}(ordered[1].object, ordered[0].object))
        std::swap(ordered[0], ordered[1]);
    for (std::size_t i = 0; i < count; ++i) {
        if (!ordered[i].try_acquire(ordered[i].object, false)) {
            release();
            return Code::busy;
        }
        held_[size_++] = ordered[i];
    }
    return Code::acquired;
}
Code Group::release() noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (!size_) return Code::empty;
    while (size_) {
        auto& b = held_[--size_];
        b.release(b.object, false);
        b = {};
    }
    return Code::released;
}
}
