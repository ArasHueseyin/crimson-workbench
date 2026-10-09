#pragma once
#include <Windows.h>
#include <intrin.h>
#include <cstdint>

namespace crimson::live_items {
struct Observation { bool player_holder{}; bool readable{}; unsigned realm{255}; DWORD thread{}; };
// Native dispatch owns the argument's lifetime. This function retains no pointer.
inline Observation observe(void* holder) noexcept {
    Observation o{};o.thread=GetCurrentThreadId();
    __try {
        if (!holder) return o;
        auto owner=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(holder)+8);
        if (!owner) return o;
        auto metadata=*reinterpret_cast<unsigned char**>(owner+0x88);
        auto component=*reinterpret_cast<unsigned char**>(owner+0x68);
        o.readable=metadata&&component;
        if(o.readable) o.player_holder=metadata[1]==1&&owner[0x96]!=0&&*reinterpret_cast<void**>(component+0xb8)==holder;
        auto tls=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        if(tls&&tls[0]) o.realm=tls[0][0x1ec];
    } __except(EXCEPTION_EXECUTE_HANDLER) { o.readable=false;o.player_holder=false; }
    return o;
}
using NativeDispatch=void* (__fastcall*)(void*,int*);
using Observer=void (*)(Observation);
inline void* forward(NativeDispatch original, Observer record, void* holder,int* error) {
    record(observe(holder));
    // Forward every call exactly once, including unknown/non-player objects.
    return original(holder,error);
}
}
