#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include "force_palm_height_data.h"
#include "../stamina/stamina_scope.h"

namespace crimson::marketplace::force_palm_height {
using Address=std::uintptr_t;
struct Actor {
    Address actor{},components{},control{},state{};
    std::uint32_t identity{};
    bool operator==(const Actor&) const=default;
};
struct Palm {
    Address layer{},animation{},fixed{},package{},instances{},conditions{},buffer{};
    std::size_t index{};
    bool operator==(const Palm&) const=default;
};
template<class Read> bool selected(Read& read,Address base,Address actor,Actor& out) {
    using namespace stamina;
    if (!base || !actor) return false;
    const auto root=read.template at<Address>(base+kClientRoot);
    if (!root) return false;
    const auto manager=read.template at<Address>(root+0x30);
    if (!manager || read.template at<Address>(manager)!=base+kClientManager ||
        read.template at<Address>(manager+0x50)!=actor ||
        read.template at<Address>(actor)!=base+kClientActor ||
        read.template at<std::uint8_t>(actor+0x96)!=1) return false;
    const auto identity=read.template at<std::uint32_t>(actor+0x60);
    const auto metadata=read.template at<Address>(actor+0x88);
    const auto components=read.template at<Address>(actor+0x68);
    const auto count=read.template at<std::uint32_t>(actor+0x70);
    const auto possessor=read.template at<Address>(actor+0xa0);
    if (!identity || !metadata || !components || count<9 || count>57 || !possessor ||
        read.template at<Address>(possessor+0xd0)!=actor) return false;
    const auto kind=read.template at<std::uint8_t>(metadata+1);
    if (kind!=1 && kind!=4) return false;
    const auto control=read.template at<Address>(components+0x40);
    if (!control || read.template at<Address>(control)!=base+0x55b5460 ||
        read.template at<Address>(control+8)!=actor) return false;
    const auto state=read.template at<Address>(control+0xc8);
    if (!state || read.template at<Address>(state+0x68)!=actor) return false;
    out={actor,components,control,state,identity};
    return read.template at<Address>(base+kClientRoot)==root &&
        read.template at<Address>(root+0x30)==manager &&
        read.template at<Address>(manager)==base+kClientManager &&
        read.template at<Address>(manager+0x50)==actor &&
        read.template at<Address>(actor)==base+kClientActor &&
        read.template at<std::uint8_t>(actor+0x96)==1 &&
        read.template at<std::uint32_t>(actor+0x60)==identity &&
        read.template at<Address>(actor+0x88)==metadata &&
        read.template at<std::uint8_t>(metadata+1)==kind &&
        read.template at<Address>(actor+0x68)==components &&
        read.template at<std::uint32_t>(actor+0x70)==count &&
        read.template at<Address>(actor+0xa0)==possessor &&
        read.template at<Address>(possessor+0xd0)==actor &&
        read.template at<Address>(components+0x40)==control &&
        read.template at<Address>(control)==base+0x55b5460 &&
        read.template at<Address>(control+8)==actor &&
        read.template at<Address>(control+0xc8)==state &&
        read.template at<Address>(state+0x68)==actor;
}
template<std::size_t N,class Read> bool fingerprint(Read& read,Address pointer,std::uint64_t& out) {
    static_assert(N==156 || N==20 || N==26);
    if (!pointer || pointer>UINTPTR_MAX-N) return false;
    std::uint64_t value=14695981039346656037ull;
    for (std::size_t i=0;i<N;++i)
        value=(value^read.template at<unsigned char>(pointer+i))*1099511628211ull;
    out=value;return true;
}
template<std::size_t N,class Read> bool equals(Read& read,Address pointer,std::uint64_t expected) {
    std::uint64_t actual{};return fingerprint<N>(read,pointer,actual) && actual==expected;
}
template<std::size_t N> constexpr std::uint64_t fingerprint(const std::array<unsigned char,N>& bytes) {
    std::uint64_t value=14695981039346656037ull;
    for (auto byte:bytes) value=(value^byte)*1099511628211ull;
    return value;
}
// Current layer ownership, full original fixed-record fingerprints and package
// bounds are read afresh. Null/incomplete/reloaded layers never qualify.
template<class Read,class Records=KnownPalmRecords>
bool palm_layer(Read& read,Address control,unsigned layer_index,Palm& out,Records records={}) {
    if (!control || layer_index>1) return false;
    const auto layer=read.template at<Address>(control+0x88+layer_index*8);
    if (!layer) return false;
    const auto animation=read.template at<Address>(layer+0x18);
    if (!animation) return false;
    const auto fixed=read.template at<Address>(animation+0x38);
    const auto instance_view=read.template at<Address>(animation+0x20);
    if (!instance_view) return false;
    const auto event_count=read.template at<std::uint8_t>(instance_view+8);
    std::uint64_t fixed_fingerprint{};
    if (!fingerprint<156>(read,fixed,fixed_fingerprint)) return false;
    const auto& animations=records.animations();
    std::size_t index=std::size(animations);
    for (std::size_t i=0;i<std::size(animations);++i)
        if (event_count==animations[i].events && fixed_fingerprint==animations[i].fingerprint) {index=i;break;}
    if (index==std::size(animations)) return false;
    const auto package=read.template at<Address>(animation+0x10);
    const auto conditions=read.template at<Address>(animation+0x30);
    if (!package || !instance_view || !conditions ||
        read.template at<std::uint8_t>(instance_view+8)!=animations[index].events ||
        read.template at<std::uint32_t>(package+0x1a8)!=kEventBytes ||
        read.template at<std::uint32_t>(package+0x118)!=kConditionBytes ||
        read.template at<std::uint32_t>(package+0x120)!=kFinderCount) return false;
    const auto instances=read.template at<Address>(instance_view);
    const auto buffer=read.template at<Address>(package+0x1a0);
    if (!instances || instances>UINTPTR_MAX-20u*animations[index].events ||
        !buffer || buffer>UINTPTR_MAX-kEventBytes) return false;
    out={layer,animation,fixed,package,instances,conditions,buffer,index};
    return read.template at<Address>(control+0x88+layer_index*8)==layer &&
        read.template at<Address>(layer+0x18)==animation &&
        read.template at<Address>(animation+0x10)==package &&
        read.template at<Address>(animation+0x20)==instance_view &&
        read.template at<Address>(animation+0x30)==conditions &&
        read.template at<Address>(animation+0x38)==fixed &&
        read.template at<Address>(instance_view)==instances &&
        read.template at<Address>(package+0x1a0)==buffer;
}
// Shared read-only classification prevents the ordinary-jump consumer from
// applying its separate multiplier during an audited aerial Palm animation.
template<class Read,class Records=KnownPalmRecords> bool active_palm(Read& read,Address control,Records records={}) {
    Palm value{};
    return palm_layer(read,control,0,value,records) || palm_layer(read,control,1,value,records);
}
struct Prepared {
    std::array<unsigned char,80> reference{};
    std::array<unsigned char,26> data{};
};
template<class Read,class Records=KnownPalmRecords> bool prepare(Read& read,Address base,std::uint32_t percent,
    Address event,Address actor_reference,Prepared& out,Records records={}) {
    if (percent<=100 || percent>10000 || !event || !actor_reference) return false;
    const auto actor=read.template at<Address>(actor_reference);
    Actor owner{};
    if (!selected(read,base,actor,owner)) return false;
    const auto instance=read.template at<Address>(event);
    const auto data=read.template at<Address>(event+8);
    if (!instance || !data) return false;
    for (unsigned layer=0;layer<2;++layer) {
        Palm palm{};
        if (!palm_layer(read,owner.control,layer,palm,records) ||
            read.template at<Address>(event+0x10)!=palm.conditions ||
            read.template at<Address>(event+0x18)!=palm.package+0x20) continue;
        for (const auto& launch:records.launches()) {
            if (launch.animation!=palm.index ||
                instance!=palm.instances+launch.index*Address{20} ||
                data!=palm.buffer+launch.word*Address{4} ||
                !equals<20>(read,instance,launch.instance_fingerprint) || !equals<26>(read,data,launch.data_fingerprint)) continue;
            read.copy(event,out.reference.data(),out.reference.size());
            read.copy(data,out.data.data(),out.data.size());
            if (fingerprint(out.data)!=launch.data_fingerprint) return false;
            float height{};std::memcpy(&height,out.data.data()+12,sizeof(height));
            const float higher=height*(static_cast<float>(percent)/100.0f);
            if (!std::isfinite(height) || height<=0 || !std::isfinite(higher) || higher<=height) return false;
            Actor check{};Palm current{};
            if (read.template at<Address>(actor_reference)!=actor ||
                !selected(read,base,actor,check) || check!=owner ||
                !palm_layer(read,owner.control,layer,current,records) || current!=palm ||
                read.template at<Address>(event)!=instance ||
                read.template at<Address>(event+8)!=data ||
                read.template at<Address>(event+0x10)!=palm.conditions ||
                read.template at<Address>(event+0x18)!=palm.package+0x20 ||
                !equals<20>(read,instance,launch.instance_fingerprint) || !equals<26>(read,data,launch.data_fingerprint)) return false;
            std::memcpy(out.data.data()+12,&higher,sizeof(higher));
            const auto local=reinterpret_cast<Address>(out.data.data());
            std::memcpy(out.reference.data()+8,&local,sizeof(local));
            return true;
        }
    }
    return false;
}
} // namespace crimson::marketplace::force_palm_height
