#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include "../jump/mount_jump_scope.h"
#include "../mount-protection/mount_protection_scope.h"

namespace crimson::marketplace::dismount {
using Address = std::uintptr_t;
inline constexpr float kThresholdMultiplier = 5.0f;
inline constexpr std::array<std::uint32_t,5> kHeightLeaf{404,0x40a00000,4,0,0};
struct Node {
    std::uint16_t word{};
    std::uint8_t count{};
    std::array<std::uint32_t,5> data{};
};
struct Program {
    std::uint32_t key{}, bytes{}, finders{}, branches{}, branch{}, target{}, leaf{};
    std::uint16_t root{};
    std::array<Node,7> nodes{};
};
// These are semantic tapes from the two ridden packages, not global opcode
// filters. 404 reads accumulated downward displacement; 213/352 are unchanged.
inline constexpr Program kLionProgram{1002316,1100,169,96,5,0xe196052f,45,27,{{
    {0,1,{629}}, {44,1,{183}}, {0,1,{629}},
    {12,5,{289,0,39,2,0}}, {0,1,{629}},
    {45,5,{404,0x40a00000,4,0,0}}, {50,5,{213,0x40a00000,4,0,0}}
}}};
inline constexpr Program kRokadeProgram{31378,4568,1385,635,609,0x9ec06df8,1122,1372,{{
    {0,1,{629}}, {428,5,{186,0x3e800000,4,0,0}}, {0,1,{629}},
    {1122,5,{404,0x40a00000,4,0,0}}, {0,1,{629}},
    {1127,5,{213,0x40a00000,4,0,0}}, {1132,5,{352,0x40a00000,4,0,0}}
}}};
struct Prepared {
    Address reference{};
    std::array<std::uint32_t,5> node{};
};
struct Mount {
    Address actor{},control{},vehicle{},data{};
    std::uint32_t identity{},key{};
    std::uint8_t riders{};
    mount_protection::Definition definition{};
    bool operator==(const Mount&) const = default;
};
template<class Read>
bool current_mount(Read& read,Address base,Address actor,bool server,Mount& out) {
    using namespace stamina;
    if (!actor || read.template at<Address>(actor)!=base+(server?kServerActor:kClientActor) ||
        !jump::mount(read,base,actor)) return false;
    const auto components=read.template at<Address>(actor+0x68);
    const auto status=read.template at<Address>(components+0x20);
    const auto control=read.template at<Address>(components+0x40);
    const auto vehicle=read.template at<Address>(components+0x48);
    if (read.template at<std::uint32_t>(actor+0x70)<10 || !vehicle ||
        read.template at<Address>(vehicle)!=base+(server?mount_protection::kServerVehicle:mount_protection::kClientVehicle) ||
        read.template at<Address>(vehicle+8)!=actor || read.template at<std::uint8_t>(vehicle+0xeb)!=1) return false;
    const auto data=read.template at<Address>(vehicle+0x18);
    const auto riders=read.template at<std::uint8_t>(vehicle+0xc8);
    mount_protection::Definition definition{};
    if (!data || read.template at<Address>(data)!=actor || !riders || riders>16 ||
        !mount_protection::loaded_definition(read,base,jump::kCharacterInfoManager,
            read.template at<std::uint16_t>(status+0x30),false,definition) ||
        (definition.key!=jump::kLion && definition.key!=jump::kRokade)) return false;
    out={actor,control,vehicle,data,read.template at<std::uint32_t>(actor+0x60),definition.key,riders,definition};
    return true;
}
template<class Read>
bool occupied(Read& read,const Mount& mount,const mount_protection::Ride& ride,std::uint32_t rider) {
    if (mount.identity!=ride.mount || ride.seat>=16 ||
        read.template at<std::uint32_t>(mount.vehicle+0x48+Address{ride.seat}*4)!=rider) return false;
    unsigned used{},matches{};
    for (unsigned i=0;i<16;++i) {
        const auto identity=read.template at<std::uint32_t>(mount.vehicle+0x48+Address{i}*4);
        used+=identity!=0; matches+=identity==rider;
    }
    return used==mount.riders && matches==1;
}
struct Scope {
    Address client_root{},server_root{},cm{},sm{},client{},server{};
    Mount candidate{},authoritative{};
    mount_protection::Ride cr{},sr{};
    bool operator==(const Scope&) const = default;
};
template<class Read>
bool selected_scope(Read& read,Address base,Address control,Scope& out) {
    using namespace stamina;
    if (!base || !control) return false;
    Scope value{};
    value.client_root=read.template at<Address>(base+kClientRoot);
    value.server_root=read.template at<Address>(base+kServerRoot);
    if (!value.client_root || !value.server_root) return false;
    value.cm=read.template at<Address>(value.client_root+0x30);
    value.sm=read.template at<Address>(value.server_root+0x48);
    if (!value.cm || !value.sm || read.template at<Address>(value.cm)!=base+kClientManager ||
        read.template at<Address>(value.sm)!=base+kServerManager) return false;
    value.client=read.template at<Address>(value.cm+0x50);
    if (!value.client) return false;
    const auto rider=read.template at<std::uint32_t>(value.client+0x60);
    value.server=mount_protection::server_actor(read,value.sm,rider);
    Player c{},s{};
    if (!player(read,base,value.client,kClientActor,kClientStatus,rider,c) ||
        !player(read,base,value.server,kServerActor,kServerStatus,rider,s) ||
        !mount_protection::riding(read,base,c,mount_protection::kClientVehicle,value.cr) ||
        !mount_protection::riding(read,base,s,mount_protection::kServerVehicle,value.sr) ||
        value.cr.mount!=value.sr.mount || value.cr.seat!=value.sr.seat) return false;
    const auto actor=read.template at<Address>(control+8);
    if (!actor) return false;
    const bool server=read.template at<Address>(actor)==base+kServerActor;
    const auto authoritative=mount_protection::server_actor(read,value.sm,value.sr.mount);
    if (!current_mount(read,base,actor,server,value.candidate) || value.candidate.control!=control ||
        (server && actor!=authoritative) ||
        !current_mount(read,base,authoritative,true,value.authoritative) ||
        value.candidate.key!=value.authoritative.key ||
        !occupied(read,value.candidate,server?value.sr:value.cr,rider) ||
        !occupied(read,value.authoritative,value.sr,rider) ||
        !stable(read,c) || !stable(read,s)) return false;
    out=value;return true;
}
struct Owner {
    Address package{},buffer{},finder{},branches{},node{};
    bool operator==(const Owner&) const = default;
};
template<class Read>
bool program(Read& read,Address reference,Address shared,const Program& spec,Owner& out) {
    // 232e5d5..232e5f6 derives both arguments from the same branch owner.
    if (!reference || reference>UINTPTR_MAX-sizeof(Address) || shared<0x20 ||
        spec.branch>=spec.branches || spec.leaf+5>spec.bytes/4 ||
        std::size_t{spec.root}+spec.nodes.size()>spec.finders) return false;
    const Address package=shared-0x20;
    if (package>UINTPTR_MAX-0x140) return false;
    const auto buffer=read.template at<Address>(package+0x110);
    const auto finder=read.template at<Address>(package+0x128);
    const auto branches=read.template at<Address>(package+0x138);
    const auto node=read.template at<Address>(reference);
    if (!buffer || !finder || !branches || buffer>UINTPTR_MAX-Address{spec.bytes} ||
        finder>UINTPTR_MAX-Address{spec.finders}*2 ||
        branches>UINTPTR_MAX-Address{spec.branches}*52 ||
        node!=buffer+Address{spec.leaf}*4 ||
        read.template at<std::uint32_t>(package+0x118)!=spec.bytes ||
        read.template at<std::uint32_t>(package+0x120)!=spec.finders ||
        read.template at<std::uint32_t>(package+0x130)!=spec.branches) return false;
    const auto branch=branches+Address{spec.branch}*52;
    if (read.template at<std::uint32_t>(branch+20)!=spec.target ||
        read.template at<std::uint16_t>(branch+36)!=spec.root ||
        read.template at<std::uint8_t>(branch+42)!=7) return false;
    for (std::size_t i=0;i<spec.nodes.size();++i) {
        const auto& required=spec.nodes[i];
        if (read.template at<std::uint16_t>(finder+(spec.root+i)*2)!=required.word) return false;
        for (std::size_t j=0;j<required.count;++j)
            if (read.template at<std::uint32_t>(buffer+Address{required.word}*4+j*4)!=required.data[j]) return false;
    }
    std::uint32_t leaf_finder=UINT32_MAX;
    for (std::uint32_t i=0;i<spec.finders;++i) {
        if (read.template at<std::uint16_t>(finder+Address{i}*2)!=spec.leaf) continue;
        if (leaf_finder!=UINT32_MAX) return false;
        leaf_finder=i;
    }
    if (leaf_finder==UINT32_MAX) return false;
    // One finder reference and one containing branch expression: a deduplicated
    // leaf shared with another movement program must never qualify.
    unsigned owners{};
    for (std::uint32_t i=0;i<spec.branches;++i) {
        const auto row=branches+Address{i}*52;
        for (unsigned which=0;which<2;++which) {
            const auto start=read.template at<std::uint16_t>(row+36+which*2);
            const auto count=read.template at<std::uint8_t>(row+42+which);
            if (!count || leaf_finder<start || leaf_finder>=std::uint32_t{start}+count) continue;
            if (i!=spec.branch || which!=0 || ++owners!=1) return false;
        }
    }
    if (owners!=1) return false;
    out={package,buffer,finder,branches,node};return true;
}
template<class Read>
bool prepare(Read& read,Address base,Address control,Address reference,Address shared,Prepared& out) {
    Scope first{},second{}; Owner owner{},again{};
    if (!selected_scope(read,base,control,first)) return false;
    const auto& spec=first.candidate.key==jump::kLion?kLionProgram:kRokadeProgram;
    if (!program(read,reference,shared,spec,owner)) return false;
    read.copy(owner.node,out.node.data(),sizeof(out.node));
    if (out.node!=kHeightLeaf) return false;
    float original{};std::memcpy(&original,&out.node[1],sizeof(original));
    const float higher=original*kThresholdMultiplier;
    if (!std::isfinite(original) || original<=0 || !std::isfinite(higher) || higher<=original ||
        !selected_scope(read,base,control,second) || first!=second ||
        !program(read,reference,shared,spec,again) || owner!=again) return false;
    std::memcpy(&out.node[1],&higher,sizeof(higher));
    out.reference=reinterpret_cast<Address>(out.node.data());
    return true;
}
} // namespace crimson::marketplace::dismount
