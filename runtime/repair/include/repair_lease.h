#pragma once
#include <array>
#include <cstddef>
#include <span>
#include <thread>

namespace crimson::repair::lease {
// Current WindowsRWLock ABI: false means exclusive, true means shared.
// Callers MUST pin owner lifetimes, validate the loaded build/vtables and run
// on the engine dispatch thread with its initialized TLS before binding these.
// This library is not a resolver, lifetime reference, loader or transaction.
struct Binding {
    void* object = nullptr;
    bool (*try_acquire)(void*, bool) noexcept = nullptr;
    void (*release)(void*, bool) noexcept = nullptr;
    bool operator==(const Binding&) const = default;
};
enum class Code { acquired, busy, invalid_binding, already_acquired, released, empty, wrong_thread };

// At most the server/client owner locks. Never blocks: if either try fails,
// release every earlier lock before returning busy. No item writes/events here.
// Construct, acquire and destroy on the dispatch thread. Noncopyable/nonmovable
// so ownership cannot accidentally migrate to a worker or survive a frame.
class Group {
    std::thread::id thread_ = std::this_thread::get_id();
    std::array<Binding, 2> held_{};
    std::size_t size_ = 0;
public:
    Group() = default;
    Group(const Group&) = delete;
    Group& operator=(const Group&) = delete;
    Group(Group&&) = delete;
    Group& operator=(Group&&) = delete;
    ~Group();
    Code try_acquire(std::span<const Binding>) noexcept;
    Code release() noexcept;
    // Dispatch-thread inspection only; concurrent use is not supported.
    std::size_t size() const noexcept { return size_; }
};
}
