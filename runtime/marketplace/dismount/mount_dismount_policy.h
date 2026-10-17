#pragma once
#include <Windows.h>
#include "mount_dismount_scope.h"

namespace crimson::marketplace::dismount {
using Handler=std::uint8_t(__fastcall*)(void*,const void*,void*,void*,std::uint8_t,void*,void*);
struct NativeRead {
    template<class T> T at(Address address) {
        T value{};copy(address,&value,sizeof(value));return value;
    }
    void copy(Address address,void* output,std::size_t count) {
        const auto* input=reinterpret_cast<const volatile unsigned char*>(address);
        auto* bytes=static_cast<unsigned char*>(output);
        for (std::size_t i=0;i<count;++i) bytes[i]=input[i];
    }
};
inline bool guarded_prepare(Address base,void* control,const void* reference,void* shared,Prepared& out) {
    __try {
        NativeRead read;
        return prepare(read,base,reinterpret_cast<Address>(control),reinterpret_cast<Address>(reference),
            reinterpret_cast<Address>(shared),out);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline std::uint8_t dispatch(Handler original,Address base,void* control,const void* reference,
    void* shared,void* context,std::uint8_t mode,void* flag,void* result,bool enabled=true) {
    Prepared prepared{};
    const bool change=enabled && guarded_prepare(base,control,reference,shared,prepared);
    return original(control,change?&prepared.reference:reference,shared,context,mode,flag,result);
}
} // namespace crimson::marketplace::dismount
