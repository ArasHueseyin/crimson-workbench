#pragma once
#include <cstdint>
namespace crimson::live_items {
using InventoryDispatch=int*(__fastcall*)(void*,int*,void*,void*,void*,bool,std::uint64_t,void*,std::uint16_t,bool,std::uint16_t,bool,bool);
using AfterInventory=void(*)(void*,int*,void*);
// Build2976's task wrapper has exactly three arguments. Both pinned callers
// hold a native actor reference until the wrapper (including this callback)
// returns. The callback borrows that context; it never queues an engine pointer.
using TaskDispatch=int*(__fastcall*)(int*,void*,void*);
using AfterTask=void(*)(void*,int*,std::uintptr_t);
inline int* forward_task(TaskDispatch original,AfterTask after,int* error,void* task,void* context,std::uintptr_t caller){
    auto result=original(error,task,context);
    after(context,error,caller);return result;
}
inline int* forward_inventory(InventoryDispatch original,AfterInventory after,void* context,int* error,void* owner,void* label,void* requests,bool a6,std::uint64_t a7,void* a8,std::uint16_t a9,bool a10,std::uint16_t a11,bool a12,bool a13){
    auto result=original(context,error,owner,label,requests,a6,a7,a8,a9,a10,a11,a12,a13);
    after(context,error,owner);return result;
}
}
