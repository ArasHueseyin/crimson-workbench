#pragma once
#include <Windows.h>
inline bool replace_vtable_slot(void** slot,void* expected,void* replacement){
    DWORD before=0;
    if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&before))return false;
    auto observed=InterlockedCompareExchangePointer(slot,replacement,expected);
    DWORD unused=0;
    if(!VirtualProtect(slot,sizeof(void*),before,&unused)){
        if(observed==expected)InterlockedCompareExchangePointer(slot,expected,replacement);
        VirtualProtect(slot,sizeof(void*),before,&unused);
        return false;
    }
    return observed==expected;
}
