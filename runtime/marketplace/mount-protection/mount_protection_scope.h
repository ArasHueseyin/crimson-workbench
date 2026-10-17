#pragma once
#include <cstddef>
#include <cstdint>
#include "../stamina/stamina_scope.h"
namespace crimson::marketplace::mount_protection {
using Address = std::uintptr_t;
inline constexpr Address kClientVehicle = 0x55b6108;
inline constexpr Address kServerVehicle = 0x5b2fc48;
inline constexpr std::uint32_t kHealthKey = 1000000;
inline constexpr std::uint8_t kHealthKind = 1;
struct Ride {
    Address component{}, data{};
    std::uint32_t mount{};
    std::uint8_t seat{};
    bool operator==(const Ride&) const = default;
};
struct Mount {
    Address actor{}, components{}, metadata{}, control{}, state{}, status{}, vital{}, entries{}, health{}, vehicle{}, vehicle_data{};
    std::uint32_t identity{}, component_count{}, entry_count{};
    std::uint8_t kind{}, riders{};
    std::int64_t current{};
    bool operator==(const Mount&) const = default;
};
template<class Read>
Address server_actor(Read& read, Address manager, std::uint32_t identity) {
    if (!manager || !identity) return 0;
    const auto count=read.template at<std::uint32_t>(manager+0x98);
    const auto buckets=read.template at<Address>(manager+0xa8);
    const auto nodes=read.template at<Address>(manager+0xb0);
    if (!count || count>1'000'000 || !buckets || !nodes) return 0;
    const auto key=identity & 0xfffff;
    const auto bucket=buckets+(key%count)*Address{0x100};
    const auto size=read.template at<std::uint32_t>(bucket);
    if (size>31) return 0;
    Address actor{};
    for (std::uint32_t i=0;i<size;++i) {
        const auto item=bucket+8+Address{i}*8;
        if (read.template at<std::uint32_t>(item)!=key) continue;
        const auto index=read.template at<std::uint32_t>(item+4);
        if (index>=1'000'000 || actor) return 0;
        const auto node=read.template at<Address>(nodes+Address{index}*8);
        if (!node || read.template at<std::uint32_t>(node+4)!=key) return 0;
        actor=read.template at<Address>(node+8);
    }
    // The low registry key is not the full actor generation/identity.
    if (!actor || read.template at<std::uint32_t>(actor+0x60)!=identity ||
        read.template at<std::uint32_t>(manager+0x98)!=count ||
        read.template at<Address>(manager+0xa8)!=buckets ||
        read.template at<Address>(manager+0xb0)!=nodes ||
        read.template at<std::uint32_t>(bucket)!=size) return 0;
    return actor;
}
template<class Read>
bool riding(Read& read, Address base, const stamina::Player& player, Address type, Ride& out) {
    const auto component=read.template at<Address>(player.components+0x48);
    if (!component || read.template at<Address>(component)!=base+type ||
        read.template at<Address>(component+8)!=player.actor) return false;
    const auto data=read.template at<Address>(component+0x18);
    if (!data || read.template at<Address>(data)!=player.actor) return false;
    const auto mount=read.template at<std::uint32_t>(data+0x10);
    const auto seat=read.template at<std::uint8_t>(data+0x14);
    if (!mount || mount==player.identity || seat>=16) return false;
    out={component,data,mount,seat};return true;
}
inline bool character_kind(std::uint8_t value) {
    // Native CharacterInfo/VehicleInfo resolver's character branches.
    return value==1 || value==3 || value==4 || value==5 || value==6 || value==8 || value==9;
}
template<class Read>
bool mount(Read& read, Address base, Address actor, bool server, std::uint32_t identity,
           std::uint16_t hp_index, Mount& out, bool require_rider=true) {
    using namespace stamina;
    const auto actor_type=server?kServerActor:kClientActor;
    const auto status_type=server?kServerStatus:kClientStatus;
    const auto vehicle_type=server?kServerVehicle:kClientVehicle;
    if (!actor || read.template at<Address>(actor)!=base+actor_type ||
        read.template at<std::uint32_t>(actor+0x60)!=identity ||
        read.template at<std::uint8_t>(actor+0x96)!=1) return false;
    const auto metadata=read.template at<Address>(actor+0x88);
    if (!metadata) return false;
    const auto kind=read.template at<std::uint8_t>(metadata+1);
    if (!character_kind(kind)) return false;
    const auto components=read.template at<Address>(actor+0x68);
    const auto count=read.template at<std::uint32_t>(actor+0x70);
    if (!components || count<10 || count>57) return false;
    const auto status=read.template at<Address>(components+0x20);
    const auto control=read.template at<Address>(components+0x40);
    const auto vehicle=read.template at<Address>(components+0x48);
    if (!status || read.template at<Address>(status)!=base+status_type ||
        read.template at<Address>(status+8)!=actor || !control ||
        read.template at<Address>(control+8)!=actor || !vehicle ||
        read.template at<Address>(vehicle)!=base+vehicle_type ||
        read.template at<Address>(vehicle+8)!=actor ||
        (require_rider && read.template at<std::uint8_t>(vehicle+0xeb)!=1)) return false;
    const auto vehicle_data=read.template at<Address>(vehicle+0x18);
    const auto state=read.template at<Address>(control+0xc8);
    const auto vital=read.template at<Address>(status+0x18);
    if (!vehicle_data || read.template at<Address>(vehicle_data)!=actor ||
        !state || read.template at<Address>(state+0x68)!=actor ||
        !vital || read.template at<Address>(vital)!=status) return false;
    const auto entries=read.template at<Address>(vital+0x58);
    const auto entry_count=read.template at<std::uint32_t>(vital+0x60);
    const auto riders=read.template at<std::uint8_t>(vehicle+0xc8);
    if (!entries || !entry_count || entry_count>4096 || (require_rider && !riders) || riders>16) return false;
    Address health{};
    for (std::uint32_t i=0;i<entry_count;++i) {
        const auto entry=entries+Address{i}*kEntrySize;
        if (read.template at<std::uint16_t>(entry)!=hp_index) continue;
        if (health) return false;
        health=entry;
    }
    if (!health) return false;
    const auto current=read.template at<std::int64_t>(health+8);
    if (current<=0) return false; // Never resurrect an already dead mount.
    out={actor,components,metadata,control,state,status,vital,entries,health,vehicle,vehicle_data,
         identity,count,entry_count,kind,riders,current};return true;
}
template<class Read>
bool seats(Read& read, const Mount& value, const Ride& ride, std::uint32_t rider) {
    if (ride.mount!=value.identity || ride.seat>=16 ||
        read.template at<std::uint32_t>(value.vehicle+0x48+Address{ride.seat}*4)!=rider) return false;
    unsigned used=0,matching=0;
    for (unsigned i=0;i<16;++i) {
        const auto identity=read.template at<std::uint32_t>(value.vehicle+0x48+Address{i}*4);
        used+=identity!=0;matching+=identity==rider;
    }
    return matching==1 && used==value.riders;
}

inline constexpr Address kCharacterInfoManager=0x6d69a48;
inline constexpr Address kMercenaryInfoManager=0x6d69a60;
inline constexpr Address kMercenaryGroupManager=0x6d71e58;
inline constexpr Address kClientClan=0x55be900,kServerClan=0x5b2c6c8;
inline constexpr Address kClientMercenary=0x55beb00,kServerMercenary=0x5b2c988;
struct Definition {
    Address manager{},rows{},row{};
    std::uint32_t count{},key{};
    std::uint16_t index{};
    bool operator==(const Definition&)const=default;
};
template<class Read>
bool loaded_definition(Read& read,Address base,Address root,std::uint16_t index,
                       bool byte_key,Definition& out) {
    const auto manager=read.template at<Address>(base+root);
    if (!manager || index==0xffff) return false;
    const auto count=read.template at<std::uint32_t>(manager+8);
    const auto rows=read.template at<Address>(manager+0x58);
    if (!count || count>65535 || index>=count || !rows) return false;
    const auto row=read.template at<Address>(rows+Address{index}*8);
    if (!row || read.template at<std::uint8_t>(row+0x10)) return false;
    const std::uint32_t key=byte_key?read.template at<std::uint8_t>(row):read.template at<std::uint32_t>(row);
    if (!key || (byte_key && key==0xff)) return false;
    out={manager,rows,row,count,key,index};return true;
}
template<class Read>
bool vehicle_group(Read& read,Address base,std::uint16_t character,
                   Definition& ci,Definition& mi,Definition& gi) {
    if (!loaded_definition(read,base,kCharacterInfoManager,character,false,ci) ||
        !loaded_definition(read,base,kMercenaryInfoManager,
            read.template at<std::uint16_t>(ci.row+0xbe),true,mi) ||
        !loaded_definition(read,base,kMercenaryGroupManager,
            read.template at<std::uint16_t>(mi.row+0x70),true,gi)) return false;
    // Native MercenaryGroupInfo semantic key 65 is Vehicle. Pet/Domestic and
    // combat followers use separate groups. Kind 2 is MercenaryType::Vehicle.
    return gi.key==65 && read.template at<std::uint8_t>(mi.row+0x20)==2 &&
        (mi.key==65 || mi.key==78 || mi.key==79 || mi.key==80 || mi.key==81 || mi.key==82);
}
struct OwnedChoice {
    Address clan{},list{},buckets{},nodes{},node{},record{};
    std::uint64_t serial{};
    std::uint32_t count{},bucket_count{},node_count{},bucket_size{},identity{};
    std::uint16_t selected{},character{};
    Definition ci{},mi{},gi{};
    bool operator==(const OwnedChoice&)const=default;
};
template<class Read>
bool main_choice(Read& read,Address base,const stamina::Player& player,bool server,
                 std::uint64_t serial,std::uint32_t identity,std::uint16_t character,
                 std::uint16_t selected,OwnedChoice& out) {
    if (player.component_count<36 || !serial || serial==UINT64_MAX) return false;
    const auto clan=read.template at<Address>(player.components+0x110);
    if (!clan || read.template at<Address>(clan)!=base+(server?kServerClan:kClientClan) ||
        read.template at<Address>(clan+8)!=player.actor) return false;
    const auto list=read.template at<Address>(clan+0x18);
    const auto count=read.template at<std::uint32_t>(clan+0x20);
    const auto bucket_count=read.template at<std::uint32_t>(clan+0x28);
    const auto node_count=read.template at<std::uint32_t>(clan+0x2c);
    const auto buckets=read.template at<Address>(clan+0x38);
    const auto nodes=read.template at<Address>(clan+0x40);
    if (!list || !count || count>4096 || !bucket_count || bucket_count>1'000'000 ||
        !node_count || node_count>1'000'000 || !buckets || !nodes) return false;
    const auto key=static_cast<std::uint32_t>(serial);
    const auto bucket=buckets+(key%bucket_count)*Address{0x100};
    const auto size=read.template at<std::uint32_t>(bucket);
    if (size>31) return false;
    Address record{},matched_node{};
    // Native 2146170 hashes the low 32 bits, but compares the complete 64-bit
    // MercenaryNo at node+8 before returning the record pointer at node+10.
    for (std::uint32_t n=0;n<size;++n) {
        const auto item=bucket+8+Address{n}*8;
        if (read.template at<std::uint32_t>(item)!=key) continue;
        const auto index=read.template at<std::uint32_t>(item+4);
        if (index>=node_count) return false;
        const auto node=read.template at<Address>(nodes+Address{index}*8);
        if (!node) return false;
        if (read.template at<std::uint64_t>(node+8)!=serial) continue;
        if (matched_node) return false;
        record=read.template at<Address>(node+0x10);matched_node=node;
    }
    if (!record || read.template at<std::uint64_t>(record+0x28)!=serial ||
        read.template at<std::uint32_t>(record+0x50)!=identity ||
        read.template at<std::uint16_t>(record+0x20)!=character ||
        read.template at<std::uint16_t>(record+0x148)!=selected) return false;
    Definition chosen_ci{},chosen_mi{},chosen_gi{};
    if (!vehicle_group(read,base,character,chosen_ci,chosen_mi,chosen_gi)) return false;
    unsigned choices=0;
    for (std::uint32_t n=0;n<count;++n) {
        const auto candidate=read.template at<Address>(list+Address{n}*8);
        if (!candidate) return false;
        if (read.template at<std::uint16_t>(candidate+0x148)!=selected) continue;
        Definition ci{},mi{},gi{};
        if (!loaded_definition(read,base,kCharacterInfoManager,
                read.template at<std::uint16_t>(candidate+0x20),false,ci) ||
            !loaded_definition(read,base,kMercenaryInfoManager,
                read.template at<std::uint16_t>(ci.row+0xbe),true,mi) ||
            !loaded_definition(read,base,kMercenaryGroupManager,
                read.template at<std::uint16_t>(mi.row+0x70),true,gi)) return false;
        if (gi.key!=65) continue;
        if (candidate!=record || ++choices!=1) return false;
    }
    if (choices!=1) return false;
    out={clan,list,buckets,nodes,matched_node,record,serial,count,bucket_count,node_count,size,
         identity,selected,character,chosen_ci,chosen_mi,chosen_gi};return true;
}
struct Mercenary {
    Address component{};std::uint64_t serial{};
    bool operator==(const Mercenary&)const=default;
};
template<class Read>
bool mercenary_owner(Read& read,Address base,const Mount& mount,bool server,
                     std::uint32_t owner,Mercenary& out) {
    if (mount.component_count<36) return false;
    const auto component=read.template at<Address>(mount.components+0x118);
    if (!component || read.template at<Address>(component)!=base+(server?kServerMercenary:kClientMercenary) ||
        read.template at<Address>(component+8)!=mount.actor ||
        read.template at<std::uint32_t>(component+0x18)!=owner) return false;
    const auto serial=read.template at<std::uint64_t>(component+0x20);
    if (!serial || serial==UINT64_MAX) return false;
    out={component,serial};return true;
}
struct ActivePair {
    Mount candidate{},authoritative{};Mercenary cm{},sm{};OwnedChoice client{},server{};
    Definition selected{};
    bool operator==(const ActivePair&)const=default;
};
template<class Read>
bool active_owned(Read& read,Address base,Address manager,Address client,Address server,
                  std::uint32_t rider,Address owner,std::uint16_t hp_index,ActivePair& out) {
    using namespace stamina;
    Player c{},s{};
    if (!player(read,base,client,kClientActor,kClientStatus,rider,c) ||
        !player(read,base,server,kServerActor,kServerStatus,rider,s)) return false;
    const auto selected=read.template at<std::uint16_t>(c.status+0x30);
    if (selected==0xffff || read.template at<std::uint16_t>(s.status+0x30)!=selected) return false;
    const auto status=read.template at<Address>(owner);
    if (!status) return false;
    const auto actor=read.template at<Address>(status+8);
    if (!actor || actor==client || actor==server) return false;
    const auto identity=read.template at<std::uint32_t>(actor+0x60);
    const auto authoritative=server_actor(read,manager,identity);
    const bool is_server=read.template at<Address>(actor)==base+kServerActor;
    ActivePair value{};
    if ((is_server && actor!=authoritative) ||
        !loaded_definition(read,base,kCharacterInfoManager,selected,false,value.selected) ||
        !mount(read,base,actor,is_server,identity,hp_index,value.candidate,false) ||
        value.candidate.vital!=owner ||
        !mount(read,base,authoritative,true,identity,hp_index,value.authoritative,false) ||
        !mercenary_owner(read,base,value.candidate,is_server,rider,value.cm) ||
        !mercenary_owner(read,base,value.authoritative,true,rider,value.sm) ||
        value.cm.serial!=value.sm.serial) return false;
    const auto character=read.template at<std::uint16_t>(value.candidate.status+0x30);
    if (character==0xffff || read.template at<std::uint16_t>(value.authoritative.status+0x30)!=character ||
        !main_choice(read,base,c,false,value.cm.serial,identity,character,selected,value.client) ||
        !main_choice(read,base,s,true,value.sm.serial,identity,character,selected,value.server) ||
        !stable(read,c) || !stable(read,s) || server_actor(read,manager,rider)!=server ||
        server_actor(read,manager,identity)!=authoritative) return false;
    out=value;return true;
}
template<class Read>
std::int64_t protected_delta(Read& read, Address base, Address owner,
                             std::uint16_t index, std::int64_t delta) {
    using namespace stamina;
    if (!base || !owner || delta>=0) return delta;
    const auto definitions=read.template at<Address>(base+kStatusInfoManager);
    if (!definitions) return delta;
    const auto definition_count=read.template at<std::uint32_t>(definitions+8);
    const auto rows=read.template at<Address>(definitions+0x58);
    if (!rows || !definition_count || definition_count>4096 || index>=definition_count) return delta;
    const auto definition=read.template at<Address>(rows+Address{index}*8);
    if (!definition || read.template at<std::uint32_t>(definition)!=kHealthKey ||
        read.template at<std::uint32_t>(definition+0x14)!=index ||
        read.template at<std::uint8_t>(definition+0x11)!=kHealthKind) return delta;
    const auto client_root=read.template at<Address>(base+kClientRoot);
    const auto server_root=read.template at<Address>(base+kServerRoot);
    if (!client_root || !server_root) return delta;
    const auto cm=read.template at<Address>(client_root+0x30);
    const auto sm=read.template at<Address>(server_root+0x48);
    if (!cm || !sm || read.template at<Address>(cm)!=base+kClientManager ||
        read.template at<Address>(sm)!=base+kServerManager) return delta;
    const auto client=read.template at<Address>(cm+0x50);
    if (!client) return delta;
    const auto identity=read.template at<std::uint32_t>(client+0x60);
    const auto server=server_actor(read,sm,identity);
    Player c{},s{};
    if (!player(read,base,client,kClientActor,kClientStatus,identity,c) ||
        !player(read,base,server,kServerActor,kServerStatus,identity,s)) return delta;
    ActivePair active{},again{};
    if (active_owned(read,base,sm,client,server,identity,owner,index,active) &&
        active_owned(read,base,sm,client,server,identity,owner,index,again) && active==again &&
        read.template at<Address>(base+kClientRoot)==client_root &&
        read.template at<Address>(base+kServerRoot)==server_root &&
        read.template at<Address>(client_root+0x30)==cm &&
        read.template at<Address>(server_root+0x48)==sm &&
        read.template at<Address>(cm)==base+kClientManager &&
        read.template at<Address>(sm)==base+kServerManager &&
        read.template at<Address>(cm+0x50)==client &&
        read.template at<Address>(base+kStatusInfoManager)==definitions &&
        read.template at<std::uint32_t>(definitions+8)==definition_count &&
        read.template at<Address>(definitions+0x58)==rows &&
        read.template at<Address>(rows+Address{index}*8)==definition &&
        read.template at<std::uint32_t>(definition)==kHealthKey &&
        read.template at<std::uint32_t>(definition+0x14)==index &&
        read.template at<std::uint8_t>(definition+0x11)==kHealthKind) return 0;
    Ride cr{},sr{};
    if (!riding(read,base,c,kClientVehicle,cr) || !riding(read,base,s,kServerVehicle,sr) ||
        cr.mount!=sr.mount || cr.seat!=sr.seat) return delta;
    const auto status=read.template at<Address>(owner);
    if (!status) return delta;
    const auto actor=read.template at<Address>(status+8);
    if (!actor || actor==client || actor==server) return delta;
    const bool is_server=read.template at<Address>(actor)==base+kServerActor;
    Mount candidate{},authoritative{};
    const auto server_mount=server_actor(read,sm,cr.mount);
    if (!mount(read,base,server_mount,true,cr.mount,index,authoritative) ||
        !seats(read,authoritative,sr,identity) ||
        !mount(read,base,actor,is_server,cr.mount,index,candidate) || candidate.vital!=owner ||
        (is_server && candidate.actor!=server_mount) ||
        !seats(read,candidate,is_server?sr:cr,identity)) return delta;
    // Rebuild every borrowed relationship immediately before suppressing damage.
    Player cc{},ss{};Ride ccr{},ssr{};Mount check{},server_check{};
    if (!player(read,base,client,kClientActor,kClientStatus,identity,cc) ||
        !player(read,base,server,kServerActor,kServerStatus,identity,ss) ||
        !riding(read,base,cc,kClientVehicle,ccr) || ccr!=cr ||
        !riding(read,base,ss,kServerVehicle,ssr) || ssr!=sr ||
        !mount(read,base,actor,is_server,cr.mount,index,check) || check!=candidate ||
        !mount(read,base,server_mount,true,cr.mount,index,server_check) || server_check!=authoritative ||
        !seats(read,check,is_server?ssr:ccr,identity) || !seats(read,server_check,ssr,identity) ||
        server_actor(read,sm,identity)!=server || server_actor(read,sm,cr.mount)!=server_mount ||
        read.template at<Address>(base+kClientRoot)!=client_root ||
        read.template at<Address>(base+kServerRoot)!=server_root ||
        read.template at<Address>(client_root+0x30)!=cm ||
        read.template at<Address>(server_root+0x48)!=sm ||
        read.template at<Address>(cm)!=base+kClientManager ||
        read.template at<Address>(sm)!=base+kServerManager ||
        read.template at<Address>(cm+0x50)!=client ||
        read.template at<Address>(base+kStatusInfoManager)!=definitions ||
        read.template at<std::uint32_t>(definitions+8)!=definition_count ||
        read.template at<Address>(definitions+0x58)!=rows ||
        read.template at<Address>(rows+Address{index}*8)!=definition ||
        read.template at<std::uint32_t>(definition)!=kHealthKey ||
        read.template at<std::uint32_t>(definition+0x14)!=index ||
        read.template at<std::uint8_t>(definition+0x11)!=kHealthKind) return delta;
    return 0;
}
} // namespace crimson::marketplace::mount_protection
