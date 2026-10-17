#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "glide_data.h"
#include "glide_targets.h"
namespace crimson::marketplace::glide {
using Address=std::uintptr_t;
struct Scope {
    Address root{},manager{},actor{},metadata{},components{},possessor{},control{},state{},controller{},method{};
    Address selector{},layer{},animation{},fixed{},package{},view{},instances{},conditions{},buffer{};
    std::uint64_t fingerprint{};
    std::uint32_t identity{},component_count{};
    std::uint8_t kind{},mode{},events{};
    bool operator==(const Scope&) const=default;
};
inline bool speed_call(Address base,Address caller) {
    if (!base || caller<base) return false;
    for (auto value:kReturns) if (caller-base==value) return true;
    return false;
}
struct KnownAnimations {
    constexpr bool operator()(std::uint64_t fingerprint,std::uint8_t events) const {
        for (const auto& value:kAnimations)
            if (value.events==events && value.fingerprint==fingerprint) return true;
        return false;
    }
};
// Production always uses KnownAnimations. The explicit dependency also lets
// owned tests exercise the full ownership pipeline with fabricated records.
template<class Read,class Classify=KnownAnimations>
bool capture(Read& read,Address base,Address method,Scope& out,Classify classify={}) {
    if (!base || !method || read.template at<Address>(method)!=base+0x55cab28) return false;
    Scope s{};s.method=method;
    s.control=read.template at<Address>(method+0x18);
    s.actor=read.template at<Address>(method+0x20);
    s.root=read.template at<Address>(base+0x6d69190);
    if (!s.root || !s.actor || !s.control) return false;
    s.manager=read.template at<Address>(s.root+0x30);
    if (!s.manager || read.template at<Address>(s.manager)!=base+0x55b62c8 ||
        read.template at<Address>(s.manager+0x50)!=s.actor ||
        read.template at<Address>(s.actor)!=base+0x5585c40 ||
        read.template at<std::uint8_t>(s.actor+0x96)!=1) return false;
    s.identity=read.template at<std::uint32_t>(s.actor+0x60);
    s.metadata=read.template at<Address>(s.actor+0x88);
    s.components=read.template at<Address>(s.actor+0x68);
    s.component_count=read.template at<std::uint32_t>(s.actor+0x70);
    s.possessor=read.template at<Address>(s.actor+0xa0);
    if (!s.identity || !s.metadata || !s.components || s.component_count<9 ||
        s.component_count>57 || !s.possessor ||
        read.template at<Address>(s.possessor+0xd0)!=s.actor) return false;
    s.kind=read.template at<std::uint8_t>(s.metadata+1);
    if ((s.kind!=1 && s.kind!=4) || read.template at<Address>(s.components+0x40)!=s.control ||
        read.template at<Address>(s.control)!=base+0x55b5460 ||
        read.template at<Address>(s.control+8)!=s.actor) return false;
    s.state=read.template at<Address>(s.control+0xc8);
    s.controller=read.template at<Address>(s.control+0x140);
    if (!s.state || !s.controller || read.template at<Address>(s.state+0x60)!=s.control ||
        read.template at<Address>(s.state+0x68)!=s.actor) return false;
    s.mode=read.template at<std::uint8_t>(s.state+0xf0);
    if (s.mode>3 || read.template at<Address>(s.state+0xf8+Address{s.mode}*8)!=method) return false;
    // ab96f0 selects this very layer, rather than any lingering lower animation.
    s.selector=read.template at<Address>(s.control+0x30);
    if (!s.selector || read.template at<std::uint8_t>(s.selector+0x1b)!=0) return false;
    s.layer=read.template at<Address>(s.control+0x88);
    if (!s.layer) return false;
    s.animation=read.template at<Address>(s.layer+0x18);
    if (!s.animation) return false;
    s.fixed=read.template at<Address>(s.animation+0x38);
    s.package=read.template at<Address>(s.animation+0x10);
    s.view=read.template at<Address>(s.animation+0x20);
    s.conditions=read.template at<Address>(s.animation+0x30);
    if (!s.fixed || s.fixed>UINTPTR_MAX-156 || !s.package || !s.view || !s.conditions ||
        read.template at<std::uint32_t>(s.package+0x1a8)!=kEventBytes ||
        read.template at<std::uint32_t>(s.package+0x118)!=kConditionBytes ||
        read.template at<std::uint32_t>(s.package+0x120)!=kFinderCount) return false;
    s.events=read.template at<std::uint8_t>(s.view+8);
    s.instances=read.template at<Address>(s.view);
    s.buffer=read.template at<Address>(s.package+0x1a0);
    if (!s.instances || !s.buffer) return false;
    s.fingerprint=14695981039346656037ull;
    for (unsigned i=0;i<156;++i)
        s.fingerprint=(s.fingerprint^read.template at<unsigned char>(s.fixed+i))*1099511628211ull;
    if (!classify(s.fingerprint,s.events)) return false;
    out=s;
    return read.template at<Address>(method)==base+0x55cab28 &&
        read.template at<Address>(method+0x18)==s.control &&
        read.template at<Address>(method+0x20)==s.actor &&
        read.template at<Address>(base+0x6d69190)==s.root &&
        read.template at<Address>(s.root+0x30)==s.manager &&
        read.template at<Address>(s.manager+0x50)==s.actor &&
        read.template at<std::uint32_t>(s.actor+0x60)==s.identity &&
        read.template at<std::uint8_t>(s.actor+0x96)==1 &&
        read.template at<Address>(s.control+0xc8)==s.state &&
        read.template at<Address>(s.state+0xf8+Address{s.mode}*8)==method &&
        read.template at<Address>(s.control+0x88)==s.layer &&
        read.template at<Address>(s.layer+0x18)==s.animation &&
        read.template at<Address>(s.animation+0x38)==s.fixed;
}
struct Velocity {
    std::array<std::uint32_t,3> before{},after{};
};
template<class Read> bool prepare(Read& read,Address output,std::uint32_t percent,Velocity& out) {
    if (percent<=100 || percent>500 || !output || output>UINTPTR_MAX-0x54 || (output&3)) return false;
    for (unsigned i=0;i<3;++i) {
        out.before[i]=read.template at<std::uint32_t>(output+0x48+4*i);
        float value{};std::memcpy(&value,&out.before[i],4);
        if (!std::isfinite(value)) return false;
        if (i!=1) value*=static_cast<float>(percent)/100.0f;
        if (!std::isfinite(value)) return false;
        std::memcpy(&out.after[i],&value,4);
    }
    return true;
}
// Both components belong to this native invocation's output, not a persistent
// tuning table. Compare before writing and undo only a still-owned first write
// if the second is refused. Foreign changes are never overwritten by rollback.
template<class ReadWrite> bool publish(ReadWrite& memory,Address output,const Velocity& v) {
    if (memory.template at<std::uint32_t>(output+0x4c)!=v.before[1] ||
        !memory.compare(output+0x48,v.before[0],v.after[0])) return false;
    if (memory.compare(output+0x50,v.before[2],v.after[2])) return true;
    memory.compare(output+0x48,v.after[0],v.before[0]);return false;
}
} // namespace crimson::marketplace::glide
