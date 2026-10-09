#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <thread>

namespace crimson::repair::reference {
// Native PaPtr receipt ABI for this pinned build. Never copy an acquired receipt
// to manufacture ownership. Only verified engine acquisition may populate it.
struct alignas(8) Receipt {
    std::array<std::uint8_t, 0x20> bytes{};
    Receipt() = default;
    Receipt(const Receipt&) = delete;
    Receipt& operator=(const Receipt&) = delete;
    Receipt(Receipt&&) = delete;
    Receipt& operator=(Receipt&&) = delete;
    std::uintptr_t actor() const noexcept;
};
static_assert(sizeof(Receipt) == 0x20);
struct Source {
    void* context = nullptr;
    std::uint32_t handle = 0; // complete handle, not the registry's masked key
    void (*acquire)(void*, std::uint32_t, Receipt&) noexcept = nullptr;
    void (*release)(void*, Receipt&) noexcept = nullptr;
};
enum class Code { acquired, unavailable, invalid_source, already_acquired, released, empty, wrong_thread };

// Host callbacks must acquire from a protected registry/current-player anchor,
// not dereference a borrowed actor pointer. They must return initialized PaPtr
// receipts, including failed acquisitions, and use the matching native release.
// All calls require the same engine thread AND unchanged TLS reference mode.
// This is ownership/cleanup, not a loader or a live source resolver.
class Pair {
    std::thread::id thread_ = std::this_thread::get_id();
    std::array<Receipt, 2> receipts_{};
    std::array<Source, 2> sources_{};
    std::size_t count_ = 0;
public:
    Pair() = default;
    Pair(const Pair&) = delete;
    Pair& operator=(const Pair&) = delete;
    Pair(Pair&&) = delete;
    Pair& operator=(Pair&&) = delete;
    ~Pair();
    Code acquire(std::span<const Source>) noexcept;
    Code release() noexcept;
    // Only usable on the acquisition thread after the complete pair succeeded.
    std::uintptr_t actor(std::size_t) const noexcept;
};
}
