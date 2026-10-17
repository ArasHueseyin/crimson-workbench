#pragma once
#include <Windows.h>
#include "glide_scope.h"
namespace crimson::marketplace::glide {
using Speed=std::uintptr_t(__fastcall*)(void*,const void*,void*);
struct Memory {
    template<class T> T at(Address address) {
        T value{};const auto src=reinterpret_cast<const volatile unsigned char*>(address);
        auto dst=reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i=0;i<sizeof(value);++i) dst[i]=src[i];
        return value;
    }
    bool compare(Address address,std::uint32_t before,std::uint32_t after) {
        __try {
            return static_cast<std::uint32_t>(InterlockedCompareExchange(
                reinterpret_cast<volatile LONG*>(address),static_cast<LONG>(after),static_cast<LONG>(before)))==before;
        } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
    }
};
template<class Classify=KnownAnimations>
inline bool guarded_capture(Address base,const void* method,Scope& out,Classify classify={}) {
    __try {Memory m;return capture(m,base,reinterpret_cast<Address>(method),out,classify);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline bool guarded_dt(const void* params,float& out) {
    __try {Memory m;out=m.at<float>(reinterpret_cast<Address>(params)+0x6c);return std::isfinite(out) && out>0;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
template<class Classify=KnownAnimations>
inline bool guarded_publish(Address base,const void* method,const void* params,void* output,
                            const Scope& before,float dt,std::uint32_t percent,Classify classify={}) {
    __try {
        Memory m;Scope checked{};Velocity v{};
        return m.at<float>(reinterpret_cast<Address>(params)+0x6c)==dt &&
            capture(m,base,reinterpret_cast<Address>(method),checked,classify) && checked==before &&
            prepare(m,reinterpret_cast<Address>(output),percent,v) &&
            capture(m,base,reinterpret_cast<Address>(method),checked,classify) && checked==before &&
            publish(m,reinterpret_cast<Address>(output),v);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
template<class Classify=KnownAnimations>
inline std::uintptr_t dispatch(Speed original,Address base,Address caller,bool enabled,std::uint32_t percent,
                               void* method,const void* params,void* output,Classify classify={}) {
    static thread_local bool active=false;
    Scope before{};float dt{};
    if (active || !enabled || percent<=100 || percent>500 || !speed_call(base,caller) ||
        !method || !params || !output || !guarded_dt(params,dt) || !guarded_capture(base,method,before,classify))
        return original(method,params,output);
    struct Invocation {bool& value;Invocation(bool& flag):value(flag) {value=true;} ~Invocation() {value=false;}} invocation(active);
    const auto result=original(method,params,output);
    guarded_publish(base,method,params,output,before,dt,percent,classify);
    return result;
}
} // namespace crimson::marketplace::glide
