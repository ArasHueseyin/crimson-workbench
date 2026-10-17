#pragma once
#include <array>
#include <atomic>
#include "consumables_targets.h"

namespace crimson::marketplace::consumables {
// The owning module is pinned for the process lifetime. Explicit cleanup is
// used on install failure; destruction must never free reachable trampolines.
template<class Backend> class HookTransaction {
    struct Slot { std::uintptr_t target{},trampoline{};bool created{}; };
    Backend& backend_;
    std::atomic<bool>& policy_enabled_;
    std::array<Slot,3> slots_{};
    bool attempted_{},initialized_{},activation_attempted_{};
public:
    HookTransaction(Backend& implementation,std::atomic<bool>& enabled) : backend_(implementation),policy_enabled_(enabled) {}
    HookTransaction(const HookTransaction&)=delete;
    HookTransaction& operator=(const HookTransaction&)=delete;
    template<class Verify,class Publish>
    bool install(std::uintptr_t base,const std::array<std::uintptr_t,3>& entries,Verify verify,Publish publish) {
        if (attempted_ || !base) return false;
        attempted_=true;policy_enabled_.store(false,std::memory_order_release);
        constexpr std::uintptr_t offsets[]{kUseRva,kLookupRva,kPopRva};
        for (std::size_t i=0;i<3;++i) if (!entries[i] || !verify(base+offsets[i],i)) return false;
        if (!backend_.pin() || !backend_.initialize()) return false;
        initialized_=true;
        std::array<std::uintptr_t,3> trampolines{};
        for (std::size_t i=0;i<3;++i) {
            auto& slot=slots_[i];slot.target=base+offsets[i];
            if (!backend_.create(slot.target,entries[i],slot.trampoline)) {cleanup();return false;}
            slot.created=true;
            if (!slot.trampoline) {cleanup();return false;}
            trampolines[i]=slot.trampoline;
        }
        if (!publish(trampolines)) {cleanup();return false;}
        for (auto& slot:slots_) if (!backend_.queue_enable(slot.target)) {cleanup();return false;}
        activation_attempted_=true;
        if (!backend_.apply()) {cleanup();return false;}
        policy_enabled_.store(true,std::memory_order_release);return true;
    }
    bool cleanup() noexcept {
        // ApplyQueued may have enabled a subset before failing. Callbacks stay
        // in passthrough until full activation and retain their published
        // original pointers if any target cannot safely be disabled.
        policy_enabled_.store(false,std::memory_order_release);
        bool disabled=true;
        for (auto& slot:slots_) if (slot.created && !backend_.disable(slot.target)) disabled=false;
        if (!disabled) return false;
        // A thread may already have reached a passthrough callback during a
        // partially successful activation. Disabling a hook is not proof that
        // this callback has returned. Keep published trampolines for the pinned
        // module's process lifetime after any activation attempt.
        if (activation_attempted_) return true;
        bool removed=true;
        for (std::size_t i=slots_.size();i>0;--i) {
            auto& slot=slots_[i-1];
            if (slot.created) {
                if (backend_.remove(slot.target)) slot.created=false;
                else removed=false;
            }
        }
        if (!removed) return false;
        if (initialized_ && !backend_.uninitialize()) return false;
        initialized_=false;return true;
    }
    bool active() const noexcept {return policy_enabled_.load(std::memory_order_acquire);}
};
} // namespace crimson::marketplace::consumables
