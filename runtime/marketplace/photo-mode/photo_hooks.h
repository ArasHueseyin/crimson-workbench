#pragma once
#include <array>
#include <atomic>
#include "photo_targets.h"
#include "photo_native.h"
namespace crimson::marketplace::photo {
template<class Backend> class HookTransaction {
    Backend& backend_;
    std::atomic<bool>& enabled_;
    std::array<Address,kHookRvas.size()> targets_{},trampolines_{};
    std::array<bool,kHookRvas.size()> created_{};
    bool attempted_{},initialized_{},activation_attempted_{};
public:
    HookTransaction(Backend& backend_value,std::atomic<bool>& enabled_value):backend_(backend_value),enabled_(enabled_value){}
    template<class Verify,class Publish>
    bool install(Address base,const std::array<Address,kHookRvas.size()>& callbacks,Verify verify,Publish publish) {
        if(attempted_||!base)return false;
        for(const auto callback:callbacks)if(!callback)return false;
        attempted_=true;enabled_.store(false,std::memory_order_release);
        if(!verify()||!backend_.pin()||!backend_.initialize())return false;
        initialized_=true;
        for(std::size_t i=0;i<kHookRvas.size();++i) {
            targets_[i]=base+kHookRvas[i];
            if(!backend_.create(targets_[i],callbacks[i],trampolines_[i])){cleanup();return false;}
            created_[i]=true;
            if(!trampolines_[i]){cleanup();return false;}
        }
        if(!publish(trampolines_)){cleanup();return false;}
        for(const auto target:targets_)if(!backend_.queue_enable(target)){cleanup();return false;}
        activation_attempted_=true;
        if(!backend_.apply()){cleanup();return false;}
        enabled_.store(true,std::memory_order_release);return true;
    }
    bool cleanup() noexcept {
        enabled_.store(false,std::memory_order_release);
        bool disabled=true;
        for(std::size_t i=kHookRvas.size();i-->0;)if(created_[i]&&!backend_.disable(targets_[i]))disabled=false;
        if(!disabled||activation_attempted_)return disabled;
        for(std::size_t i=kHookRvas.size();i-->0;)if(created_[i]) {
            if(!backend_.remove(targets_[i]))return false;
            created_[i]=false;
        }
        if(initialized_&&!backend_.uninitialize())return false;
        initialized_=false;return true;
    }
};
} // namespace crimson::marketplace::photo
