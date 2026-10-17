#pragma once
#include <Windows.h>
#include "force_palm_height_scope.h"
namespace crimson::marketplace::force_palm_height {
using Event=void*(__fastcall*)(void*,void*,const void*,std::uintptr_t,const void*);
struct Memory {
    template<class T> T at(Address address) {
        T value{};const auto src=reinterpret_cast<const volatile unsigned char*>(address);
        const auto dst=reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i=0;i<sizeof(value);++i) dst[i]=src[i];
        return value;
    }
    void copy(Address address,void* output,std::size_t count) {
        const auto src=reinterpret_cast<const volatile unsigned char*>(address);
        const auto dst=static_cast<unsigned char*>(output);
        for (std::size_t i=0;i<count;++i) dst[i]=src[i];
    }
};
template<class Records=KnownPalmRecords>
inline bool guarded_prepare(Address base,std::uint32_t percent,const void* event,const void* actor,Prepared& out,Records records={}) {
    __try {Memory memory;return prepare(memory,base,percent,reinterpret_cast<Address>(event),reinterpret_cast<Address>(actor),out,records);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
template<class Records=KnownPalmRecords>
inline void* dispatch(Event original,Address base,bool enabled,std::uint32_t percent,
    void* self,void* result,const void* event,std::uintptr_t time,const void* actor,Records records={}) {
    Prepared prepared{};
    const bool change=enabled && guarded_prepare(base,percent,event,actor,prepared,records);
    // The native synchronous event copies these 26 bytes, does its original
    // status/eligibility checks and writes its own pending jump. Nothing is
    // retained and the shared source chart is never changed.
    return original(self,result,change?prepared.reference.data():event,time,actor);
}
} // namespace crimson::marketplace::force_palm_height
