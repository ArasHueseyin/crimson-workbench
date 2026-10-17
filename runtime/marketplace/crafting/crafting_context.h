#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace crimson::marketplace::crafting {
using Address = std::uintptr_t;
enum class Site : std::uint32_t { Database, Client, Quantity, KnowledgeA, KnowledgeB, Count };
inline constexpr std::size_t kSiteCount = static_cast<std::size_t>(Site::Count);

// Exact stack layout produced by crafting_bridge.asm. The last cell initially
// contains the site number, then the dispatcher replaces it with a continuation.
// No field here aliases the interrupted function's own frame or game objects.
struct Context {
    std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    std::uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax, flags;
    std::uint64_t continuation;
    Address interrupted_rsp() const noexcept { return reinterpret_cast<Address>(this) + sizeof(Context); }
};
static_assert(std::is_standard_layout_v<Context> && sizeof(Context) == 136);
static_assert(offsetof(Context, rbx) == 88 && offsetof(Context, rax) == 112);
static_assert(offsetof(Context, flags) == 120 && offsetof(Context, continuation) == 128);

struct Edit {
    Address continuation{};
    bool change_bx{};
    std::uint16_t bx{};
    bool change_edi{};
    std::uint32_t edi{};
};
inline void apply(Context& context, const Edit& edit) noexcept {
    if (!edit.continuation) return;
    if (edit.change_bx) context.rbx = (context.rbx & ~std::uint64_t{0xffff}) | edit.bx;
    if (edit.change_edi) context.rdi = edit.edi; // Native MOV EDI clears its high half.
    context.continuation = edit.continuation;
}

using Callback = void (*)(Context&, Site) noexcept;
struct BridgeConfiguration {
    std::array<Address, kSiteCount> passthrough{};
    Callback callback{};
};
struct StateSupport {
    std::uint32_t bytes{};
    std::uint64_t mask{};
    bool xsave{};
    bool avx{};
};
StateSupport state_support() noexcept;
// Publish once, while no bridge entry is reachable. No hooks are activated here.
bool configure_bridge(const BridgeConfiguration&) noexcept;
bool bridge_ready() noexcept;
std::array<Address, kSiteCount> bridge_entries() noexcept;
} // namespace crimson::marketplace::crafting
