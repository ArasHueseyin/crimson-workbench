#include <Windows.h>
#include <intrin.h>
#include <atomic>
#include "crafting_context.h"

using crimson::marketplace::crafting::Context;
extern "C" {
std::uint32_t crimson_crafting_state_bytes = 512;
std::uint32_t crimson_crafting_state_mode = 0;
std::uint32_t crimson_crafting_state_low = 0;
std::uint32_t crimson_crafting_state_high = 0;
std::uint32_t crimson_crafting_test_avx = 0;
void crimson_crafting_database_entry();
void crimson_crafting_client_entry();
void crimson_crafting_quantity_entry();
void crimson_crafting_knowledge_a_entry();
void crimson_crafting_knowledge_b_entry();
}

namespace crimson::marketplace::crafting {
namespace {
std::atomic<unsigned> readiness{};
BridgeConfiguration configuration{};
bool executable(Address address) noexcept {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &region, sizeof(region)) ||
        region.State != MEM_COMMIT || (region.Protect & PAGE_GUARD)) return false;
    const auto protection = region.Protect & 0xff;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}
}
StateSupport state_support() noexcept {
    int info[4]{};
    __cpuid(info, 0);
    const int highest = info[0];
    __cpuid(info, 1);
    constexpr unsigned xsave = 1u << 26, osxsave = 1u << 27, avx = 1u << 28;
    if (!(static_cast<unsigned>(info[2]) & xsave) || !(static_cast<unsigned>(info[2]) & osxsave))
        return {512, 0, false, false};
    if (highest < 0xd) return {};
    const bool cpu_avx = (static_cast<unsigned>(info[2]) & avx) != 0;
    const auto mask = _xgetbv(0);
    __cpuidex(info, 0xd, 0);
    const auto available = static_cast<std::uint64_t>(static_cast<unsigned>(info[0])) |
        (static_cast<std::uint64_t>(static_cast<unsigned>(info[3])) << 32);
    const auto bytes = static_cast<unsigned>(info[1]);
    // Restore all currently enabled user state, including upper AVX/AVX-512 or
    // future enabled components. Do not silently truncate it to 512/832 bytes.
    if ((mask & 3) != 3 || (mask & ~available) || bytes < 576 || bytes > 65536) return {};
    return {bytes, mask, true, cpu_avx && (mask & 6) == 6};
}
bool configure_bridge(const BridgeConfiguration& candidate) noexcept {
    for (const auto target : candidate.passthrough) if (!executable(target)) return false;
    if (candidate.callback && !executable(reinterpret_cast<Address>(candidate.callback))) return false;
    const auto support = state_support();
    if (!support.bytes) return false;
    unsigned expected = 0;
    if (!readiness.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) return false;
    configuration = candidate;
    crimson_crafting_state_bytes = support.bytes;
    crimson_crafting_state_mode = support.xsave ? 1u : 0u;
    crimson_crafting_state_low = static_cast<std::uint32_t>(support.mask);
    crimson_crafting_state_high = static_cast<std::uint32_t>(support.mask >> 32);
    crimson_crafting_test_avx = support.avx ? 1u : 0u;
    readiness.store(2, std::memory_order_release);
    return true;
}
bool bridge_ready() noexcept { return readiness.load(std::memory_order_acquire) == 2; }
std::array<Address, kSiteCount> bridge_entries() noexcept {
    return {reinterpret_cast<Address>(&crimson_crafting_database_entry),
        reinterpret_cast<Address>(&crimson_crafting_client_entry),
        reinterpret_cast<Address>(&crimson_crafting_quantity_entry),
        reinterpret_cast<Address>(&crimson_crafting_knowledge_a_entry),
        reinterpret_cast<Address>(&crimson_crafting_knowledge_b_entry)};
}
} // namespace crimson::marketplace::crafting

extern "C" void crimson_crafting_dispatch(Context* context) noexcept {
    using namespace crimson::marketplace::crafting;
    if (!bridge_ready() || context->continuation >= kSiteCount) __fastfail(FAST_FAIL_INVALID_ARG);
    const auto site = static_cast<Site>(context->continuation);
    context->continuation = configuration.passthrough[static_cast<std::size_t>(site)];
    if (configuration.callback) configuration.callback(*context, site);
}
