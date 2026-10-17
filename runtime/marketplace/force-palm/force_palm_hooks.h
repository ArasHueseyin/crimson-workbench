#pragma once
#include <atomic>
#include <cstdint>
#include "force_palm_targets.h"

namespace crimson::marketplace::force_palm {
// A published trampoline can be executing even after DisableHook returns.
// Pin the module and retain it and that trampoline until process termination
// after any activation attempt, including an activation failure.
template<class Backend> class HookTransaction {
    Backend& backend_;
    std::atomic<bool>& enabled_;
    std::uintptr_t target_{},trampoline_{};
    bool attempted_{},initialized_{},created_{},activation_attempted_{};
public:
    HookTransaction(Backend& implementation,std::atomic<bool>& policy):backend_(implementation),enabled_(policy) {}
    template<class Verify,class Publish>
    bool install(std::uintptr_t base,std::uintptr_t callback,Verify verify,Publish publish) {
        if (attempted_ || !base || !callback) return false;
        attempted_=true;enabled_.store(false,std::memory_order_release);
        target_=base+kEvaluateRva;
        if (!verify() || !backend_.pin() || !backend_.initialize()) return false;
        initialized_=true;
        if (!backend_.create(target_,callback,trampoline_)) {cleanup();return false;}
        created_=true;
        if (!trampoline_ || !publish(trampoline_)) {cleanup();return false;}
        activation_attempted_=true;
        if (!backend_.enable(target_)) {cleanup();return false;}
        enabled_.store(true,std::memory_order_release);return true;
    }
    bool cleanup() noexcept {
        enabled_.store(false,std::memory_order_release);
        if (created_ && !backend_.disable(target_)) return false;
        if (activation_attempted_) return true;
        if (created_) {
            if (!backend_.remove(target_)) return false;
            created_=false;
        }
        if (initialized_ && !backend_.uninitialize()) return false;
        initialized_=false;return true;
    }
};
} // namespace crimson::marketplace::force_palm
