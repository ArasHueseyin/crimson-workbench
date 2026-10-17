#pragma once
#include "crafting_context.h"
#include "crafting_targets.h"

namespace crimson::marketplace::crafting {
inline constexpr Address kLookupRva=0x4a21f0;
inline constexpr std::size_t kHookCount=kSiteCount+1;
// Backend-injected lifecycle. All signatures are verified before any hook resource is created;
// every trampoline is published before the single queued activation step.
template<class Backend>
class HookTransaction {
    struct Slot { Address target{}, trampoline{}; bool created{}; };
    Backend& backend_;
    std::array<Slot,kHookCount> slots_{};
    bool initialized_{};
    bool active_{};
    bool activation_attempted_{};
public:
    explicit HookTransaction(Backend& implementation) : backend_(implementation) {}
    HookTransaction(const HookTransaction&) = delete;
    HookTransaction& operator=(const HookTransaction&) = delete;
    ~HookTransaction() { cleanup(); }
    template<class Verify, class Publish>
    bool install(Address base, const std::array<Address,kSiteCount>& entries, Address lookup_entry,
                 Verify verify, Publish publish) {
        if (initialized_ || active_ || !base) return false;
        for (std::size_t i=0;i<kHookCount;++i)
            if (!(i<kSiteCount ? entries[i] : lookup_entry) ||
                !verify(base+(i<kSiteCount ? kTargets[i].hook : kLookupRva),i)) return false;
        if (!backend_.initialize()) return false;
        initialized_=true;
        BridgeConfiguration configuration;
        for (std::size_t i=0;i<kHookCount;++i) {
            auto& slot=slots_[i]; slot.target=base+(i<kSiteCount ? kTargets[i].hook : kLookupRva);
            if (!backend_.create(slot.target,i<kSiteCount ? entries[i] : lookup_entry,slot.trampoline)) { cleanup(); return false; }
            slot.created=true;
            if (!slot.trampoline) { cleanup(); return false; }
            if(i<kSiteCount) configuration.passthrough[i]=slot.trampoline;
        }
        if (!publish(configuration,slots_.back().trampoline)) { cleanup(); return false; }
        for (auto& slot:slots_)
            if (!backend_.queue_enable(slot.target)) { cleanup(); return false; }
        activation_attempted_=true;
        if (!backend_.apply()) { cleanup(); return false; }
        active_=true;
        return true;
    }
    bool cleanup() noexcept {
        // ApplyQueued can fail after partially enabling hooks. Disable every
        // created target before removing ANY trampoline. Keep resources owned
        // when disabling fails; freeing a still-reachable trampoline is unsafe.
        bool disabled=true;
        for (auto& slot:slots_) if (slot.created && !backend_.disable(slot.target)) disabled=false;
        if (!disabled) return false;
        active_=false;
        // Disabling prevents new entries, but an already-entered passthrough
        // callback may still need its trampoline. No quiescence is proven here.
        // The pinned ASI retains resources after ANY activation attempt.
        if (activation_attempted_) return false;
        bool removed=true;
        for (std::size_t i=kHookCount;i>0;--i) {
            auto& slot=slots_[i-1];
            if (!slot.created) continue;
            if (backend_.remove(slot.target)) slot.created=false;
            else removed=false;
        }
        if (!removed) return false;
        if (initialized_ && !backend_.uninitialize()) return false;
        initialized_=false;
        return true;
    }
    bool active() const noexcept { return active_; }
};
} // namespace crimson::marketplace::crafting
