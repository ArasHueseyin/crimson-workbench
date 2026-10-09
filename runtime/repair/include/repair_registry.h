#pragma once
#include "repair_reference.h"

namespace crimson::repair::registry {
// Current examined registry ABI. The lookup itself holds its registry lock and
// acquires an actor reference before returning a PaPtr receipt. It can block:
// invoke only on the verified engine dispatch thread, never input/render code.
using Lookup = void* (*)(void*, reference::Receipt*, std::uint32_t) noexcept;
using Destroy = void (*)(reference::Receipt*) noexcept;

// Adapter, not a live resolver. The host must validate the loaded build and both
// methods, keep the manager alive, and preserve thread/TLS mode throughout the
// reference scope. A borrowed actor address is NOT a valid manager binding.
// This object must outlive every reference acquired from source(). No global
// release function: each source retains the destroy method of its own binding.
class Source {
    void* manager_;
    Lookup lookup_;
    Destroy destroy_;
    static void acquire(void*, std::uint32_t, reference::Receipt&) noexcept;
    static void release(void*, reference::Receipt&) noexcept;
public:
    Source(void* manager, Lookup lookup, Destroy destroy) noexcept
        : manager_(manager), lookup_(lookup), destroy_(destroy) {}
    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;
    Source(Source&&) = delete;
    Source& operator=(Source&&) = delete;
    reference::Source source(std::uint32_t full_handle) noexcept;
};
}
