#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "../stamina/stamina_scope.h"

namespace crimson::marketplace::force_palm {
using Address = std::uintptr_t;
inline constexpr Address kControlType = 0x55b5460;
inline constexpr std::uint32_t kConditionStart = 4612, kConditionCount = 5;
inline constexpr std::uint32_t kBufferBytes = 20640, kFinderCount = 11334;
inline constexpr std::array<std::uint16_t,5> kFinderWords{0,73,0,2717,2722};
inline constexpr std::array<std::uint32_t,10> kLeafPair{
    234,0xd2603bc1,0x3f800000,0,0,237,15035,1,0,0};
inline constexpr std::array<unsigned char,52> kEntryBranch{
    0,0,0,0,0,0,0,0,0,0,0x80,0xbf,0,0,0x80,0xbf,
    0,0,0,0,0x88,0x21,0x1b,0xb8,0,0,0,0,4,0,1,0,
    8,0x9b,0x0b,0xc3,4,0x12,0x30,0x0c,0,0,5,1,0,0x28,0x28,0x1e,0,0,0,0};

struct Snapshot {
    Address root{}, manager{}, actor{}, metadata{}, components{}, possessor{};
    Address owner{}, buffer{}, finder{}, branches{};
    std::uint32_t identity{}, component_count{};
    std::uint8_t kind{};
    bool operator==(const Snapshot&) const = default;
};

// All addresses are borrowed from this invocation. No actor, chart, descriptor,
// buffer or finder address is retained across calls or native script reloads.
template<class Read>
bool snapshot(Read& read, Address base, Address descriptor, Address control,
              Address cursor, Snapshot& out) {
    using namespace stamina;
    if (!base || !descriptor || !control || !cursor ||
        read.template at<std::uint32_t>(descriptor) != kConditionStart ||
        read.template at<std::uint32_t>(descriptor+4) != kConditionCount ||
        read.template at<std::uint32_t>(cursor) != 3 ||
        read.template at<Address>(control) != base+kControlType) return false;
    out.root=read.template at<Address>(base+kClientRoot);
    if (!out.root) return false;
    out.manager=read.template at<Address>(out.root+0x30);
    if (!out.manager || read.template at<Address>(out.manager) != base+kClientManager) return false;
    out.actor=read.template at<Address>(out.manager+0x50);
    if (!out.actor || read.template at<Address>(out.actor) != base+kClientActor ||
        read.template at<Address>(control+8) != out.actor ||
        read.template at<std::uint8_t>(out.actor+0x96) != 1) return false;
    out.identity=read.template at<std::uint32_t>(out.actor+0x60);
    out.metadata=read.template at<Address>(out.actor+0x88);
    out.components=read.template at<Address>(out.actor+0x68);
    out.component_count=read.template at<std::uint32_t>(out.actor+0x70);
    out.possessor=read.template at<Address>(out.actor+0xa0);
    if (!out.identity || !out.metadata || !out.components || !out.possessor ||
        out.component_count<9 || out.component_count>57 ||
        read.template at<Address>(out.components+0x40) != control ||
        read.template at<Address>(out.possessor+0xd0) != out.actor) return false;
    out.kind=read.template at<std::uint8_t>(out.metadata+1);
    if (out.kind!=1 && out.kind!=4) return false;
    out.owner=read.template at<Address>(descriptor+8);
    // Native package loader 1f68701..1f6876a supplies these bounds. The owner
    // is package+110 in the native caller's current expression descriptor.
    if (out.owner<0x110 || read.template at<std::uint32_t>(out.owner+8)!=kBufferBytes ||
        read.template at<std::uint32_t>(out.owner+0x10)!=kFinderCount ||
        read.template at<std::uint32_t>(out.owner+0x20)!=1646) return false;
    out.buffer=read.template at<Address>(out.owner);
    out.finder=read.template at<Address>(out.owner+0x18);
    out.branches=read.template at<Address>(out.owner+0x28);
    if (!out.buffer || !out.finder || !out.branches ||
        out.buffer>UINTPTR_MAX-kBufferBytes || out.finder>UINTPTR_MAX-kFinderCount*2 ||
        out.branches>UINTPTR_MAX-1646*52) return false;
    for (std::size_t i=0;i<kFinderWords.size();++i) {
        const auto word=read.template at<std::uint16_t>(out.finder+(kConditionStart+i)*2);
        if (word!=kFinderWords[i] || word*4u>=kBufferBytes) return false;
    }
    if (read.template at<std::uint32_t>(out.buffer)!=629 ||
        read.template at<std::uint32_t>(out.buffer+73*4)!=183) return false;
    for (std::size_t i=0;i<kLeafPair.size();++i)
        if (read.template at<std::uint32_t>(out.buffer+2717*4+i*4)!=kLeafPair[i]) return false;
    for (std::size_t i=0;i<kEntryBranch.size();++i)
        if (read.template at<std::uint8_t>(out.branches+285*52+i)!=kEntryBranch[i]) return false;
    return true;
}
} // namespace crimson::marketplace::force_palm
