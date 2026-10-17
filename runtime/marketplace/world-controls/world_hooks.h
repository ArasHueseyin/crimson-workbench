#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace crimson::marketplace::world_controls {
template<class Backend> class WorldHookTransaction {
    Backend& backend_;std::atomic<bool>& ready_;
    std::array<std::uintptr_t,3> targets_{},trampolines_{};
    unsigned created_{};bool attempted_{},initialized_{},activation_attempted_{};
public:
    WorldHookTransaction(Backend& backend,std::atomic<bool>& ready):backend_(backend),ready_(ready){}
    template<class Verify,class Publish>
    bool install(std::uintptr_t base,const std::array<std::uintptr_t,3>& callbacks,Verify verify,Publish publish) {
        if(attempted_||!base)return false;attempted_=true;ready_.store(false,std::memory_order_release);
        for(const auto callback:callbacks)if(!callback)return false;
        targets_={base+0x3dc4730,base+0x3dc50e0,base+0x9adf20};
        if(!verify()||!backend_.pin()||!backend_.initialize())return false;initialized_=true;
        for(;created_<targets_.size();) {
            if(!backend_.create(targets_[created_],callbacks[created_],trampolines_[created_])){cleanup();return false;}
            ++created_;
        }
        for(const auto trampoline:trampolines_)if(!trampoline){cleanup();return false;}
        if(!publish(trampolines_)){cleanup();return false;}
        activation_attempted_=true;
        for(const auto target:targets_)if(!backend_.enable(target)){cleanup();return false;}
        ready_.store(true,std::memory_order_release);return true;
    }
    bool cleanup() noexcept {
        ready_.store(false,std::memory_order_release);
        // Never release any trampoline if a disable reports an unknown state.
        for(unsigned i=0;i<created_;++i)if(!backend_.disable(targets_[i]))return false;
        if(activation_attempted_)return true;
        for(unsigned i=0;i<created_;++i)if(!backend_.remove(targets_[i]))return false;
        created_=0;
        if(initialized_&&!backend_.uninitialize())return false;initialized_=false;return true;
    }
};
struct WeatherEntryGuard {
    bool& flag;bool entered;
    explicit WeatherEntryGuard(bool& value) noexcept:flag(value),entered(!value){if(entered)flag=true;}
    ~WeatherEntryGuard(){if(entered)flag=false;}
    WeatherEntryGuard(const WeatherEntryGuard&)=delete;
    WeatherEntryGuard& operator=(const WeatherEntryGuard&)=delete;
};
} // namespace crimson::marketplace::world_controls
