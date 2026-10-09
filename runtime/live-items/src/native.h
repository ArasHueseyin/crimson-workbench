#pragma once
#include <Windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstring>
#include "protocol.h"
#include "observe.h"

namespace crimson::live_items {
using Lookup=bool(__fastcall*)(std::uint16_t*,std::uint32_t);
using Definition=unsigned char*(__fastcall*)(const std::uint16_t*);
using Construct=void*(__fastcall*)(void*,const std::uint16_t*,std::int64_t);
using Convert=void*(__fastcall*)(void*,const void*,std::uint16_t);
using Destroy=void(__fastcall*)(void*);
using Grant=int*(__fastcall*)(void*,int*,const void*);
struct Bindings { Lookup lookup{};Definition definition{};Construct construct{};Convert convert{};Destroy destroy{};Grant grant{}; };
struct Outcome { State state{State::rejected};Error error{Error::context}; };
inline bool completed_context(void* context,int* error,void* owner) noexcept {
    __try {return context&&error&&*error==0&&*reinterpret_cast<void**>(context)==owner;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
// These reads run only inside a native dispatch that holds the actor reference.
inline void* server_holder(std::uintptr_t base,void* actor) noexcept {
    __try {
        auto tls=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        if(!tls||!tls[0]||tls[0][0x1ec]!=1||!*reinterpret_cast<void**>(tls[0]+0x250))return nullptr;
        if(!actor||*reinterpret_cast<std::uintptr_t*>(actor)!=base+0x5b1e2b0)return nullptr;
        auto component=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(actor)+0x68);
        if(!component)return nullptr;
        auto holder=*reinterpret_cast<void**>(component+0xb8);
        auto observation=observe(holder);
        if(!observation.player_holder||*reinterpret_cast<void**>(static_cast<unsigned char*>(holder)+8)!=actor)return nullptr;
        return holder;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}
// Pure reads, also safe for readiness polling while the game is paused. This
// gives no permission to write: only task_player's native lease can do that.
inline std::uint32_t selected_identity(std::uintptr_t base) noexcept {
    __try {
        if(!base)return 0;
        auto root=*reinterpret_cast<unsigned char**>(base+0x6d69190);if(!root)return 0;
        auto manager=*reinterpret_cast<unsigned char**>(root+0x30);
        if(!manager||*reinterpret_cast<std::uintptr_t*>(manager)!=base+0x55b62c8)return 0;
        auto selected=*reinterpret_cast<unsigned char**>(manager+0x50);
        if(!selected||*reinterpret_cast<std::uintptr_t*>(selected)!=base+0x5585c40||!selected[0x96])return 0;
        auto metadata=*reinterpret_cast<unsigned char**>(selected+0x88);
        auto possessor=*reinterpret_cast<unsigned char**>(selected+0xa0);
        if(!metadata||metadata[1]!=1||!possessor||*reinterpret_cast<void**>(possessor+0xd0)!=selected)return 0;
        auto handle=*reinterpret_cast<std::uint32_t*>(selected+0x60);
        if(*reinterpret_cast<void**>(manager+0x50)!=selected)return 0;
        return handle;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return 0;}
}
inline void* task_player(std::uintptr_t base,void* context,int* error,std::uintptr_t caller,std::uint32_t* identity) noexcept {
    __try {
        // These are the only two verified callers. Their actor lease outlives
        // the task wrapper. A different caller is not an execution permission.
        if(caller!=base+0x2774a0d&&caller!=base+0x27742ae)return nullptr;
        if(!context||!error||*error!=0)return nullptr;
        auto owner=*reinterpret_cast<unsigned char**>(context);
        auto holder=server_holder(base,owner);if(!holder)return nullptr;
        auto possessor=*reinterpret_cast<unsigned char**>(owner+0xa0);
        if(!possessor||*reinterpret_cast<void**>(possessor+0xd0)!=owner)return nullptr;
        auto handle=*reinterpret_cast<std::uint32_t*>(owner+0x60);
        if(!handle||selected_identity(base)!=handle)return nullptr;
        *identity=handle;return holder;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}
inline bool total(void* holder,std::uint16_t type,std::int64_t* result) noexcept {
    __try {
        auto p=static_cast<unsigned char*>(holder);
        auto buckets=*reinterpret_cast<unsigned char***>(p+0x18);
        auto count=*reinterpret_cast<std::uint32_t*>(p+0x20);
        if(!buckets||!count||count>128)return false;
        std::int64_t sum=0;
        for(unsigned b=0;b<count;b++) {
            if(!buckets[b])return false;
            auto slots=*reinterpret_cast<unsigned char**>(buckets[b]);
            auto size=*reinterpret_cast<std::uint16_t*>(buckets[b]+8);
            if(size>16384||(!slots&&size))return false;
            for(unsigned i=0;i<size;i++) {
                auto slot=slots+i*0xc8;
                if(*reinterpret_cast<std::uint16_t*>(slot+8)==type) {
                    auto qty=*reinterpret_cast<std::int64_t*>(slot+0x10);
                    if(qty<0||qty>1000000000||sum>1000000000-qty)return false;sum+=qty;
                }
            }
        }
        *result=sum;return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline Outcome execute(Bindings b,void* holder,const Request& r) noexcept {
    bool constructed=false,submitted=false;
    alignas(16) unsigned char value[0xc8]{};
    alignas(16) unsigned char init[0x1c0]{};
    __try {
        std::uint16_t type=0xffff;
        if(!b.lookup(&type,r.key)||type==0xffff)return {State::rejected,Error::unsupported_item};
        auto def=b.definition(&type);
        // +0x42 is optional. Native grant explicitly accepts its 0xffff
        // sentinel (e.g. Extra-large Bag 6003); it is not an invalid key.
        if(!def||*reinterpret_cast<std::uint32_t*>(def)!=r.key)return {State::rejected,Error::definition_mismatch};
        // The native InitData serializer contains five inline socket records.
        if(def[0x244]>5||*reinterpret_cast<std::uint32_t*>(def+0x228)>5)return {State::rejected,Error::socket_limit};
        if(!def[0x3f0]&&r.quantity>100)return {State::rejected,Error::quantity_limit};
        std::int64_t before{},after{};
        if(!total(holder,type,&before))return {State::rejected,Error::context};
        b.construct(value,&type,r.quantity);constructed=true;
        auto cap=value[0x70];auto physical=*reinterpret_cast<std::uint32_t*>(value+0x68);
        if(cap>5||physical>5||physical<cap){b.destroy(value);return {State::rejected,Error::socket_limit};}
        b.convert(init,value,0xffff);
        // Conversion must preserve a NEW item, its external key and quantity.
        if(*reinterpret_cast<std::int64_t*>(init)!=-1||*reinterpret_cast<std::uint32_t*>(init+8)!=r.key||*reinterpret_cast<std::int64_t*>(init+0x10)!=r.quantity) {
            b.destroy(value);return {State::rejected,Error::conversion_mismatch};
        }
        b.destroy(value);constructed=false;
        int error=-1;submitted=true;b.grant(holder,&error,init);
        // Any failure after entering the write path has unknown side effects.
        // Never re-submit automatically, including after a pipe timeout.
        if(error!=0)return {State::uncertain,Error::native_error};
        if(!total(holder,type,&after)||after-before!=r.quantity)return {State::uncertain,Error::inventory_mismatch};
        return {State::applied,Error::none};
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // A partly constructed engine value cannot be destroyed speculatively.
        (void)constructed;(void)submitted;return {State::uncertain,Error::native_fault};
    }
}
}
