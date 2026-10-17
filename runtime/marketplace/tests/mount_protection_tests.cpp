#include "../mount-protection/mount_protection_policy.h"
#include "../mount-protection/mount_protection_lifecycle.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
namespace {
using namespace crimson::marketplace::mount_protection;
template<class T>void put(void* address,std::size_t offset,T value) {std::memcpy(static_cast<unsigned char*>(address)+offset,&value,sizeof(value));}
struct Entity {
    std::array<unsigned char,0xb0> actor{};
    std::array<unsigned char,2> metadata{};
    std::array<unsigned char,0x120> components{};
    std::array<unsigned char,0xd8> possessor{};
    std::array<unsigned char,0xd0> control{};
    std::array<unsigned char,0x70> state{};
    std::array<unsigned char,0x40> status{};
    std::array<unsigned char,0x68> vital{};
    std::array<unsigned char,0x90*3> entries{};
    std::array<unsigned char,16> holder{};
    std::array<unsigned char,0xf0> vehicle{};
    std::array<unsigned char,0x20> vehicle_data{};
    void init(Address base,std::uint32_t identity,bool server) {
        using namespace crimson::marketplace::stamina;
        put(actor.data(),0,base+(server?kServerActor:kClientActor));put(actor.data(),0x60,identity);
        put(actor.data(),0x68,components.data());put(actor.data(),0x70,std::uint32_t{24});put(actor.data(),0x88,metadata.data());metadata[1]=1;
        put(actor.data(),0x96,std::uint8_t{1});put(actor.data(),0xa0,possessor.data());put(possessor.data(),0xd0,actor.data());
        put(components.data(),0x20,status.data());put(components.data(),0x40,control.data());put(components.data(),0x48,vehicle.data());put(components.data(),0xb8,holder.data());
        put(holder.data(),8,actor.data());put(control.data(),8,actor.data());put(control.data(),0xc8,state.data());put(state.data(),0x68,actor.data());
        put(status.data(),0,base+(server?kServerStatus:kClientStatus));put(status.data(),8,actor.data());put(status.data(),0x18,vital.data());
        put(vital.data(),0,status.data());put(vital.data(),0x58,entries.data());put(vital.data(),0x60,std::uint32_t{3});
        for(unsigned n=0;n<3;++n) {put(entries.data(),n*0x90,static_cast<std::uint16_t>(n==0?0:21+n));put(entries.data(),n*0x90+8,std::int64_t{50});}
        put(vehicle.data(),0,base+(server?kServerVehicle:kClientVehicle));put(vehicle.data(),8,actor.data());put(vehicle.data(),0x18,vehicle_data.data());
        put(vehicle_data.data(),0,actor.data());
    }
};
struct Fixture {
    static constexpr std::uint32_t rider_id=0xa0101234,mount_id=0xa0105678;
    Address base{};
    std::array<unsigned char,0x50> client_root{},server_root{};
    std::array<unsigned char,0xb8> cm{},sm{};
    std::array<unsigned char,0x100> bucket{};
    std::array<Address,2> nodes{};
    std::array<unsigned char,16> rider_node{},mount_node{};
    Entity c,s,mc,ms;
    std::array<unsigned char,0x60> definitions{};
    std::array<Address,84> rows{};
    std::array<unsigned char,0x20> hp{},stamina{};
    std::array<unsigned char,0x60> output{};
    Fixture() {
        using namespace crimson::marketplace::stamina;
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d73000,MEM_RESERVE,PAGE_NOACCESS));
        assert(base && VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        assert(VirtualAlloc(reinterpret_cast<void*>(base+0x6d71000),4096,MEM_COMMIT,PAGE_READWRITE));
        c.init(base,rider_id,false);s.init(base,rider_id,true);mc.init(base,mount_id,false);ms.init(base,mount_id,true);
        put(reinterpret_cast<void*>(base+kClientRoot),0,client_root.data());put(reinterpret_cast<void*>(base+kServerRoot),0,server_root.data());
        put(client_root.data(),0x30,cm.data());put(server_root.data(),0x48,sm.data());put(cm.data(),0,base+kClientManager);put(sm.data(),0,base+kServerManager);put(cm.data(),0x50,c.actor.data());
        put(sm.data(),0x98,std::uint32_t{1});put(sm.data(),0xa8,bucket.data());put(sm.data(),0xb0,nodes.data());
        put(bucket.data(),0,std::uint32_t{2});put(bucket.data(),8,rider_id&0xfffff);put(bucket.data(),12,std::uint32_t{0});put(bucket.data(),16,mount_id&0xfffff);put(bucket.data(),20,std::uint32_t{1});
        nodes={reinterpret_cast<Address>(rider_node.data()),reinterpret_cast<Address>(mount_node.data())};
        put(rider_node.data(),4,rider_id&0xfffff);put(rider_node.data(),8,s.actor.data());put(mount_node.data(),4,mount_id&0xfffff);put(mount_node.data(),8,ms.actor.data());
        put(reinterpret_cast<void*>(base+kStatusInfoManager),0,definitions.data());put(definitions.data(),8,std::uint32_t{84});put(definitions.data(),0x58,rows.data());
        rows[0]=reinterpret_cast<Address>(hp.data());rows[22]=reinterpret_cast<Address>(stamina.data());put(hp.data(),0,kHealthKey);put(hp.data(),0x11,kHealthKind);put(hp.data(),0x14,std::uint32_t{0});
        put(stamina.data(),0,std::uint32_t{1000026});put(stamina.data(),0x11,std::uint8_t{2});put(stamina.data(),0x14,std::uint32_t{22});
        for(auto* rider:{&c,&s}) {put(rider->vehicle_data.data(),0x10,mount_id);put(rider->vehicle_data.data(),0x14,std::uint8_t{0});}
        for(auto* mount:{&mc,&ms}) {put(mount->vehicle.data(),0xeb,std::uint8_t{1});put(mount->vehicle.data(),0xc8,std::uint8_t{1});put(mount->vehicle.data(),0x48,rider_id);}
    }
    ~Fixture() {assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE));}
    Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
    void seat(std::uint8_t value) {
        for(auto* rider:{&c,&s})put(rider->vehicle_data.data(),0x14,value);
        for(auto* mount:{&mc,&ms}) {put(mount->vehicle.data(),0x48,std::uint32_t{0});put(mount->vehicle.data(),0x48+std::size_t{value}*4,rider_id);}
    }
};
struct ActiveFixture:Fixture {
    static constexpr std::uint64_t serial=0x1234567890abcdef;
    std::array<unsigned char,0x60> characters{},mercenaries{},groups{};
    std::array<Address,8> character_rows{};
    std::array<Address,1> mercenary_rows{};
    std::array<Address,6> group_rows{};
    std::array<unsigned char,0xc0> player_info{},mount_info{};
    std::array<unsigned char,0x78> mercenary_info{};
    std::array<unsigned char,0x20> group_info{};
    std::array<std::array<unsigned char,0x48>,2> clans{};
    std::array<std::array<unsigned char,0x30>,2> mercenary{};
    std::array<std::array<Address,2>,2> records{},serial_nodes{};
    std::array<std::array<unsigned char,0x150>,2> active_record{},inactive_record{};
    std::array<std::array<unsigned char,0x100>,2> serial_buckets{};
    std::array<std::array<unsigned char,0x18>,2> serial_node{},collision_node{};
    ActiveFixture() {
        put(reinterpret_cast<void*>(base+kCharacterInfoManager),0,characters.data());
        put(reinterpret_cast<void*>(base+kMercenaryInfoManager),0,mercenaries.data());
        put(reinterpret_cast<void*>(base+kMercenaryGroupManager),0,groups.data());
        character_rows={reinterpret_cast<Address>(player_info.data()),reinterpret_cast<Address>(mount_info.data())};
        mercenary_rows={reinterpret_cast<Address>(mercenary_info.data())};group_rows={reinterpret_cast<Address>(group_info.data())};
        put(characters.data(),8,std::uint32_t{2});put(characters.data(),0x58,character_rows.data());
        put(mercenaries.data(),8,std::uint32_t{1});put(mercenaries.data(),0x58,mercenary_rows.data());
        put(groups.data(),8,std::uint32_t{1});put(groups.data(),0x58,group_rows.data());
        put(player_info.data(),0,std::uint32_t{10001});put(mount_info.data(),0,std::uint32_t{7250});
        put(mount_info.data(),0xbe,std::uint16_t{0});put(mercenary_info.data(),0,std::uint8_t{81});
        put(mercenary_info.data(),0x20,std::uint8_t{2});put(mercenary_info.data(),0x70,std::uint16_t{0});
        put(group_info.data(),0,std::uint8_t{65});
        Entity* players[]={&c,&s};Entity* mounts[]={&mc,&ms};
        for(unsigned n=0;n<2;++n) {
            auto& player=*players[n];auto& mount=*mounts[n];
            put(player.actor.data(),0x70,std::uint32_t{36});put(mount.actor.data(),0x70,std::uint32_t{36});
            put(player.status.data(),0x30,std::uint16_t{0});put(mount.status.data(),0x30,std::uint16_t{1});
            put(player.vehicle_data.data(),0x10,std::uint32_t{0});put(player.vehicle_data.data(),0x14,std::uint8_t{255});
            put(mount.vehicle.data(),0xc8,std::uint8_t{0});put(mount.vehicle.data(),0x48,std::uint32_t{0});
            put(player.components.data(),0x110,clans[n].data());put(clans[n].data(),0,base+(n?kServerClan:kClientClan));
            put(clans[n].data(),8,player.actor.data());put(clans[n].data(),0x18,records[n].data());
            put(clans[n].data(),0x20,std::uint32_t{1});put(clans[n].data(),0x28,std::uint32_t{1});put(clans[n].data(),0x2c,std::uint32_t{1});
            put(clans[n].data(),0x38,serial_buckets[n].data());put(clans[n].data(),0x40,serial_nodes[n].data());
            records[n][0]=reinterpret_cast<Address>(active_record[n].data());records[n][1]=reinterpret_cast<Address>(inactive_record[n].data());
            serial_nodes[n][0]=reinterpret_cast<Address>(serial_node[n].data());serial_nodes[n][1]=reinterpret_cast<Address>(collision_node[n].data());
            put(serial_buckets[n].data(),0,std::uint32_t{1});put(serial_buckets[n].data(),8,static_cast<std::uint32_t>(serial));put(serial_buckets[n].data(),12,std::uint32_t{0});
            put(serial_node[n].data(),8,serial);put(serial_node[n].data(),0x10,active_record[n].data());
            put(active_record[n].data(),0x20,std::uint16_t{1});put(active_record[n].data(),0x28,serial);
            put(active_record[n].data(),0x50,mount_id);put(active_record[n].data(),0x148,std::uint16_t{0});
            put(inactive_record[n].data(),0x20,std::uint16_t{1});put(inactive_record[n].data(),0x148,std::uint16_t{0xffff});
            put(mount.components.data(),0x118,mercenary[n].data());put(mercenary[n].data(),0,base+(n?kServerMercenary:kClientMercenary));
            put(mercenary[n].data(),8,mount.actor.data());put(mercenary[n].data(),0x18,rider_id);put(mercenary[n].data(),0x20,serial);
        }
    }
};
Fixture* fixture{};void* expected_owner{};std::uint16_t expected_index{};std::int64_t expected_delta{};unsigned calls=0;
constexpr std::uintptr_t result=0xfedcba98765432a5;
std::uintptr_t __fastcall original(void* owner,std::uint16_t index,std::uint64_t time,std::int64_t delta,void* output) {
    assert(owner==expected_owner && index==expected_index && time==0x1122334455667788 && delta==expected_delta && output==fixture->output.data());
    put(output,0,std::uint32_t{0xdeadbeef});++calls;return result;
}
void invoke(Fixture& f,Entity& candidate,std::int64_t input,std::int64_t expected,std::uint16_t index=0) {
    fixture=&f;expected_owner=candidate.vital.data();expected_index=index;expected_delta=expected;
    assert(dispatch(original,f.base,expected_owner,index,0x1122334455667788,input,f.output.data())==result);
    assert(Memory{}.at<std::uint32_t>(reinterpret_cast<Address>(f.output.data()))==0xdeadbeef);
    assert(Memory{}.at<std::int64_t>(reinterpret_cast<Address>(candidate.entries.data())+8)==50 || expected==input);
}
std::uintptr_t __fastcall invalid(void* owner,std::uint16_t index,std::uint64_t time,std::int64_t delta,void* output) {
    assert(owner==reinterpret_cast<void*>(1) && index==2 && time==3 && delta==-4 && output==reinterpret_cast<void*>(5));++calls;return result;
}
struct ChangingRead:Memory {
    Fixture& f;unsigned reads=0;
    explicit ChangingRead(Fixture& value):f(value){}
    template<class T>T at(Address address) {
        const auto value=Memory::at<T>(address);
        if(address==reinterpret_cast<Address>(f.c.actor.data())+0x60 && ++reads==3)put(f.c.vehicle_data.data(),0x10,std::uint32_t{0});
        return value;
    }
};
struct UnselectingRead:Memory {
    ActiveFixture& f;unsigned reads=0;
    explicit UnselectingRead(ActiveFixture& value):f(value){}
    template<class T>T at(Address address) {
        const auto value=Memory::at<T>(address);
        if(address==reinterpret_cast<Address>(f.active_record[0].data())+0x148 && ++reads==3)
            put(f.active_record[0].data(),0x148,std::uint16_t{0xffff});
        return value;
    }
};
struct LifecycleFixture {
    unsigned fail{},step{};bool pinned{},published{},entered{},retained{};
    bool pin_module() {assert(++step==1);return pinned=fail!=1;}
    bool initialize() {assert(pinned && ++step==2);return fail!=2;}
    bool create() {assert(pinned && ++step==3);return published=fail!=3;}
    bool enable() {
        assert(pinned && published && ++step==4);
        entered=true;retained=true;
        return fail!=4;
    }
};
}
int main() {
    for(unsigned fail=0;fail<=4;++fail) {
        LifecycleFixture native{fail};
        const auto installed=install_hook(native);
        const auto expected=fail==0?InstallResult::ready:fail==1?InstallResult::pin_failed:
            fail==2?InstallResult::initialize_failed:fail==3?InstallResult::create_failed:InstallResult::activation_failed;
        assert(installed==expected && native.step==(fail?fail:4));
        assert(native.published==(fail==0 || fail==4));
        assert(native.entered==native.retained && (!native.entered || native.pinned));
    }
    for(unsigned seat:{0u,1u,15u})for(unsigned kind:{1u,3u,4u,5u,6u,8u,9u}) {
        Fixture f;f.seat(static_cast<std::uint8_t>(seat));f.mc.metadata[1]=f.ms.metadata[1]=static_cast<unsigned char>(kind);
        for(auto value:{std::int64_t{-1},std::int64_t{-1000},std::numeric_limits<std::int64_t>::min()}) {invoke(f,f.mc,value,0);invoke(f,f.ms,value,0);}
    }
    for(auto value:{std::int64_t{0},std::int64_t{1},std::numeric_limits<std::int64_t>::max()}) {Fixture f;invoke(f,f.mc,value,value);invoke(f,f.ms,value,value);}
    {Fixture f;invoke(f,f.c,-999,-999);invoke(f,f.s,-999,-999);invoke(f,f.mc,-999,-999,22);invoke(f,f.ms,-999,-999,22);}
    {Fixture f;for(auto* mount:{&f.mc,&f.ms}) {put(mount->vehicle.data(),0x4c,std::uint32_t{0x12345678});put(mount->vehicle.data(),0xc8,std::uint8_t{2});}invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    {Fixture f;for(auto* rider:{&f.c,&f.s})put(rider->vehicle_data.data(),0x10,std::uint32_t{0});invoke(f,f.mc,-999,-999);invoke(f,f.ms,-999,-999);}
    for(auto kind:{std::uint8_t{0},std::uint8_t{2},std::uint8_t{255}}) {Fixture f;put(f.hp.data(),0x11,kind);invoke(f,f.mc,-999,-999);invoke(f,f.ms,-999,-999);}
    {Fixture f;f.rows[6]=f.rows[0];f.rows[0]=0;put(f.hp.data(),0x14,std::uint32_t{6});put(f.mc.entries.data(),0,std::uint16_t{6});put(f.ms.entries.data(),0,std::uint16_t{6});invoke(f,f.mc,-1000,0,6);invoke(f,f.ms,-1000,0,6);}
    for(unsigned invalid_case=0;invalid_case<37;++invalid_case) {
        Fixture f;
        switch(invalid_case) {
        case 0:put(f.cm.data(),0x50,Address{0});break;
        case 1:put(f.c.actor.data(),0x96,std::uint8_t{0});break;
        case 2:put(f.s.actor.data(),0x60,Fixture::rider_id^0x100000);break;
        case 3:put(f.mc.actor.data(),0x60,Fixture::mount_id^0x100000);break;
        case 4:put(f.ms.actor.data(),0x60,Fixture::mount_id^0x100000);break;
        case 5:put(f.c.possessor.data(),0xd0,Address{0});break;
        case 6:put(f.c.vehicle_data.data(),0x10,std::uint32_t{0});break; // Dismounted/inactive.
        case 7:put(f.s.vehicle_data.data(),0x10,Fixture::mount_id^1);break;
        case 8:put(f.c.vehicle_data.data(),0x14,std::uint8_t{16});break;
        case 9:put(f.s.vehicle_data.data(),0x14,std::uint8_t{1});break;
        case 10:put(f.c.vehicle_data.data(),0,Address{0});break;
        case 11:put(f.s.vehicle.data(),8,Address{0});break;
        case 12:put(f.c.vehicle.data(),0,Address{0});break;
        case 13:put(f.mc.vehicle.data(),0x48,Fixture::rider_id^0x100000);break;
        case 14:put(f.ms.vehicle.data(),0x48,Fixture::rider_id^0x100000);break;
        case 15:put(f.mc.vehicle.data(),0x4c,Fixture::rider_id);break; // Duplicated rider.
        case 16:put(f.ms.vehicle.data(),0xc8,std::uint8_t{0});break;
        case 17:put(f.mc.vehicle.data(),0xc8,std::uint8_t{17});break;
        case 18:put(f.ms.vehicle.data(),0xeb,std::uint8_t{0});break;
        case 19:put(f.mc.vehicle.data(),0x18,Address{0});break;
        case 20:put(f.ms.vehicle_data.data(),0,Address{0});break;
        case 21:put(f.mc.entries.data(),8,std::int64_t{0});break;
        case 22:put(f.ms.entries.data(),8,std::int64_t{0});break;
        case 23:put(f.mc.actor.data(),0x96,std::uint8_t{0});break;
        case 24:put(f.ms.actor.data(),0x96,std::uint8_t{0});break;
        case 25:f.mc.metadata[1]=7;break;
        case 26:put(f.mc.status.data(),8,Address{0});break;
        case 27:put(f.mc.control.data(),8,Address{0});break;
        case 28:put(f.mc.state.data(),0x68,Address{0});break;
        case 29:put(f.mc.vital.data(),0x60,std::uint32_t{4097});break;
        case 30:put(f.mc.entries.data(),0x90,std::uint16_t{0});break;
        case 31:put(f.hp.data(),0,std::uint32_t{1000001});break; // Fatal is not HP.
        case 32:put(f.hp.data(),0x11,std::uint8_t{2});break; // Stamina's kind is not HP's kind.
        case 33:put(f.hp.data(),0x14,std::uint32_t{1});break;
        case 34:put(f.mount_node.data(),4,std::uint32_t{0});break;
        case 35:put(f.bucket.data(),24,Fixture::mount_id&0xfffff);put(f.bucket.data(),28,std::uint32_t{1});put(f.bucket.data(),0,std::uint32_t{3});break;
        case 36:put(f.mc.components.data(),0x48,f.c.vehicle.data());break;
        }
        invoke(f,f.mc,-1000,-1000);
    }
    {Fixture f;ChangingRead memory(f);assert(protected_delta(memory,f.base,reinterpret_cast<Address>(f.mc.vital.data()),0,-1000)==-1000);}
    {ActiveFixture f;invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);invoke(f,f.mc,1,1);invoke(f,f.ms,-999,-999,22);}
    {ActiveFixture f;put(f.mc.vehicle.data(),0xeb,std::uint8_t{0});put(f.ms.vehicle.data(),0xeb,std::uint8_t{0});invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    {ActiveFixture f;f.character_rows[5]=f.character_rows[1];f.character_rows[1]=0;put(f.characters.data(),8,std::uint32_t{6});f.group_rows[5]=f.group_rows[0];f.group_rows[0]=0;put(f.groups.data(),8,std::uint32_t{6});put(f.mercenary_info.data(),0x70,std::uint16_t{5});for(auto* mount:{&f.mc,&f.ms})put(mount->status.data(),0x30,std::uint16_t{5});for(auto& record:f.active_record)put(record.data(),0x20,std::uint16_t{5});invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    {ActiveFixture f;f.character_rows[0]=0;invoke(f,f.mc,-999,-999);invoke(f,f.ms,-999,-999);}
    {ActiveFixture f;Entity stale;stale.init(f.base,Fixture::mount_id,true);put(stale.actor.data(),0x70,std::uint32_t{36});put(stale.status.data(),0x30,std::uint16_t{1});std::array<unsigned char,0x30> mercenary{};put(mercenary.data(),0,f.base+kServerMercenary);put(mercenary.data(),8,stale.actor.data());put(mercenary.data(),0x18,Fixture::rider_id);put(mercenary.data(),0x20,ActiveFixture::serial);put(stale.components.data(),0x118,mercenary.data());invoke(f,stale,-999,-999);}
    for(auto key:{65u,78u,79u,80u,81u,82u}) {ActiveFixture f;put(f.mercenary_info.data(),0,static_cast<std::uint8_t>(key));invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    {ActiveFixture f;for(unsigned n=0;n<2;++n)put(f.clans[n].data(),0x20,std::uint32_t{2});invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    {ActiveFixture f;for(unsigned n=0;n<2;++n) {put(f.clans[n].data(),0x2c,std::uint32_t{2});put(f.serial_buckets[n].data(),0,std::uint32_t{2});put(f.serial_buckets[n].data(),16,static_cast<std::uint32_t>(ActiveFixture::serial));put(f.serial_buckets[n].data(),20,std::uint32_t{1});put(f.collision_node[n].data(),8,ActiveFixture::serial^0x100000000ULL);}invoke(f,f.mc,-999,0);invoke(f,f.ms,-999,0);}
    for(unsigned invalid_case=0;invalid_case<33;++invalid_case) {
        ActiveFixture f;
        switch(invalid_case) {
        case 0:put(f.active_record[0].data(),0x148,std::uint16_t{0xffff});break;
        case 1:put(f.active_record[1].data(),0x148,std::uint16_t{0xffff});break;
        case 2:put(f.active_record[0].data(),0x50,std::uint32_t{0});break;
        case 3:put(f.active_record[1].data(),0x50,Fixture::mount_id^0x100000);break;
        case 4:put(f.active_record[0].data(),0x28,ActiveFixture::serial^0x100000000ULL);break;
        case 5:put(f.serial_node[1].data(),8,ActiveFixture::serial^0x100000000ULL);break;
        case 6:put(f.mercenary[1].data(),0x18,Fixture::rider_id^0x100000);break;
        case 7:put(f.mercenary[0].data(),8,Address{0});break;
        case 8:put(f.mercenary[1].data(),0x20,UINT64_MAX);break;
        case 9:put(f.clans[0].data(),8,Address{0});break;
        case 10:put(f.clans[1].data(),0,Address{0});break;
        case 11:put(f.clans[0].data(),0x20,std::uint32_t{0});break;
        case 12:put(f.clans[1].data(),0x28,std::uint32_t{0});break;
        case 13:put(f.clans[0].data(),0x2c,std::uint32_t{0});break;
        case 14:put(f.serial_buckets[1].data(),0,std::uint32_t{32});break;
        case 15:put(f.serial_buckets[0].data(),12,std::uint32_t{1});break;
        case 16:f.records[1][0]=reinterpret_cast<Address>(f.inactive_record[1].data());break;
        case 17:put(f.clans[0].data(),0x20,std::uint32_t{2});put(f.inactive_record[0].data(),0x148,std::uint16_t{0});break;
        case 18:put(f.s.status.data(),0x30,std::uint16_t{1});break;
        case 19:put(f.active_record[1].data(),0x20,std::uint16_t{0});break;
        case 20:put(f.ms.status.data(),0x30,std::uint16_t{0xffff});break;
        case 21:put(f.mount_info.data(),0x10,std::uint8_t{1});break;
        case 22:put(f.mount_info.data(),0xbe,std::uint16_t{0xffff});break;
        case 23:put(f.mercenary_info.data(),0,std::uint8_t{67});break;
        case 24:put(f.mercenary_info.data(),0x20,std::uint8_t{4});break;
        case 25:put(f.mercenary_info.data(),0x70,std::uint16_t{0xffff});break;
        case 26:put(f.group_info.data(),0,std::uint8_t{67});break;
        case 27:put(f.group_info.data(),0x10,std::uint8_t{1});break;
        case 28:put(f.ms.actor.data(),0x96,std::uint8_t{0});break;
        case 29:put(f.ms.entries.data(),8,std::int64_t{0});break;
        case 30:put(f.c.actor.data(),0x70,std::uint32_t{24});break;
        case 31:put(f.clans[0].data(),0x2c,std::uint32_t{2});put(f.serial_buckets[0].data(),0,std::uint32_t{2});put(f.serial_buckets[0].data(),16,static_cast<std::uint32_t>(ActiveFixture::serial));put(f.serial_buckets[0].data(),20,std::uint32_t{1});put(f.collision_node[0].data(),8,ActiveFixture::serial);put(f.collision_node[0].data(),0x10,f.active_record[0].data());break;
        case 32:put(f.clans[0].data(),0x2c,std::uint32_t{2});put(f.serial_buckets[0].data(),0,std::uint32_t{2});put(f.serial_buckets[0].data(),16,static_cast<std::uint32_t>(ActiveFixture::serial));put(f.serial_buckets[0].data(),20,std::uint32_t{1});put(f.serial_node[0].data(),0x10,Address{0});put(f.collision_node[0].data(),8,ActiveFixture::serial);put(f.collision_node[0].data(),0x10,f.active_record[0].data());break;
        }
        invoke(f,f.mc,-999,-999);
        // Client component state is only consulted for client damage; the
        // authoritative server path independently verifies both owner clans.
        if(invalid_case!=7)invoke(f,f.ms,-999,-999);
    }
    {ActiveFixture f;UnselectingRead memory(f);assert(protected_delta(memory,f.base,reinterpret_cast<Address>(f.mc.vital.data()),0,-999)==-999);}
    assert(dispatch(invalid,1,reinterpret_cast<void*>(1),2,3,-4,reinterpret_cast<void*>(5))==result);
    std::printf("PASS: ridden and selected summoned mount HP protection, fresh main-slot/serial/owner identity, server generation, semantic HP, unchanged heals/stamina/player/NPC paths and full native forwarding (%u calls).\n",calls);
}
