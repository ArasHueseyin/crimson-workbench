#pragma once
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "fall_scope.h"

namespace crimson::marketplace::fall {
using Resolver=void*(__fastcall*)(const std::uint16_t*);
using Fall=void(__fastcall*)(void*,void*,float,void*,void*);
template<class T> inline T read_at(const void* p,std::size_t at){T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
inline bool protected_key(std::uint32_t key){return std::binary_search(kProtectedKeys.begin(),kProtectedKeys.end(),key);}

// The owner route and u16 CharacterInfo handle are taken from the fall caller
// e52bb60. Actor kind validation matches the game's actor lookup at 17eb150.
inline std::uint32_t component_key(void* component,Resolver resolver){
    if(!component||!resolver)return 0;
    auto actor=read_at<void*>(component,8);if(!actor)return 0;
    auto metadata=read_at<void*>(actor,0x88);if(!metadata)return 0;
    const auto kind=read_at<std::uint8_t>(metadata,1);
    if(kind!=1&&kind!=3&&kind!=4&&kind!=5&&kind!=6&&kind!=8)return 0;
    auto components=read_at<void*>(actor,0x68);if(!components)return 0;
    auto status=read_at<void*>(components,0x20);if(!status)return 0;
    auto row=resolver(reinterpret_cast<const std::uint16_t*>(static_cast<unsigned char*>(status)+0x30));
    return row?read_at<std::uint32_t>(row,0):0;
}
inline std::uint32_t guarded_key(void* component,Resolver resolver){
    __try{return component_key(component,resolver);}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
inline bool dispatch_fall(Fall original,Resolver resolver,void* self,void* source,float height,void* event,void* detail){
    if(protected_key(guarded_key(self,resolver)))return true;
    original(self,source,height,event,detail);return false;
}

} // namespace crimson::marketplace::fall
