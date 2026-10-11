#pragma once
#include "blackstar_policy.h"

using CharacterResolver=void*(__fastcall*)(const std::uint16_t*);
using MountedActor=void*(__fastcall*)(void*,void*);
using ReleaseActorRef=void(__fastcall*)(void*);
using MercenaryLookup=void*(__fastcall*)(void*,std::uint64_t);
struct FlightServices{CharacterResolver character;MountedActor mounted;ReleaseActorRef release;MercenaryLookup mercenary;};

// Character route copied from the game's actor lookup (17eb150); indices are u16,
// unlike the static CharacterInfo key. Non-character actor kinds fail closed.
inline std::uint32_t actor_character_key(void* actor,CharacterResolver resolve){
    if(!actor)return 0;
    auto metadata=read_at<void*>(actor,0x88);
    if(!metadata)return 0;
    const auto kind=read_at<std::uint8_t>(metadata,1);
    if(kind!=1&&kind!=3&&kind!=4&&kind!=5&&kind!=6&&kind!=8)return 0;
    auto components=read_at<void*>(actor,0x68);
    if(!components)return 0;
    auto character=read_at<void*>(components,0x20);
    if(!character)return 0;
    auto row=resolve(reinterpret_cast<const std::uint16_t*>(static_cast<unsigned char*>(character)+0x30));
    return row?read_at<std::uint32_t>(row,0):0;
}
inline bool context_on_blackstar(void* context,const FlightServices& services){
    if(!context)return false;
    auto actor=read_at<void*>(context,0x828);
    if(!actor)return false;
    if(actor_character_key(actor,services.character)==kBlackstarKey)return true;
    unsigned char reference[0x20]{};
    services.mounted(actor,reference);
    bool eligible=false;
    // Match the game's native height query: a returned mount reference must be
    // released even if inspecting its character record fails.
    __try{
        if(read_at<std::uint8_t>(reference,0x10)==1)
            eligible=actor_character_key(read_at<void*>(reference,8),services.character)==kBlackstarKey;
    }__finally{services.release(reference);}
    return eligible;
}
inline bool initial_blackstar(void* component,std::uint64_t uid,const FlightServices& services){
    if(!component||!uid)return false;
    auto instance=services.mercenary(static_cast<unsigned char*>(component)+0x18,uid);
    if(!instance)return false;
    auto row=services.character(reinterpret_cast<const std::uint16_t*>(static_cast<unsigned char*>(instance)+0x20));
    return row&&read_at<std::uint32_t>(row,0)==kBlackstarKey;
}
inline bool town_parameters(void* condition){
    return condition&&read_at<std::uint8_t>(condition,0x18)==2&&read_at<float>(condition,0x1c)==20.0f;
}
inline void* platform_row(void* original,void* copy,std::uintptr_t caller,std::uintptr_t expected){
    if(!original||!blackstar_summon||caller!=expected||read_at<std::uint8_t>(original,0x169)!=1)return original;
    std::memcpy(copy,original,0x16a);
    write_at<std::uint8_t>(copy,0x169,0);
    return copy;
}
