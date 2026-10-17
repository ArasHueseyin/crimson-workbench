#include "../dismount/mount_dismount_policy.h"
#include "../dismount/mount_dismount_hooks.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
namespace {
using namespace crimson::marketplace;
using namespace crimson::marketplace::dismount;
using mount_protection::kClientVehicle;
using mount_protection::kServerVehicle;
using mount_protection::kHealthKey;
using mount_protection::kHealthKind;
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
struct BaseFixture {
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
    BaseFixture() {
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
    ~BaseFixture() {assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE));}
    BaseFixture(const BaseFixture&)=delete;BaseFixture& operator=(const BaseFixture&)=delete;
    void seat(std::uint8_t value) {
        for(auto* rider:{&c,&s})put(rider->vehicle_data.data(),0x14,value);
        for(auto* mount:{&mc,&ms}) {for(unsigned i=0;i<16;++i)put(mount->vehicle.data(),0x48+std::size_t{i}*4,std::uint32_t{0});put(mount->vehicle.data(),0x48+std::size_t{value}*4,rider_id);}
    }
};

struct Fixture:BaseFixture {
    std::array<unsigned char,0x60> characters{};
    std::array<Address,40> character_rows{};
    std::array<unsigned char,0x20> definition{};
    std::array<unsigned char,0x160> package{};
    std::array<std::uint32_t,1142> buffer{};
    std::array<std::uint16_t,1385> finder{};
    std::array<unsigned char,635*52> branches{};
    Address reference{};
    const Program* spec{};
    float distance{};
    explicit Fixture(std::uint32_t key=1002316) {
        using namespace crimson::marketplace::stamina;
        put(reinterpret_cast<void*>(base+mount_protection::kCharacterInfoManager),0,characters.data());
        put(characters.data(),8,std::uint32_t{40});put(characters.data(),0x58,character_rows.data());
        character_rows[7]=reinterpret_cast<Address>(definition.data());put(definition.data(),0,key);
        for(auto* entity:{&mc,&ms})put(entity->status.data(),0x30,std::uint16_t{7});
        program_for(key);
    }
    void program_for(std::uint32_t key) {
        spec=key==1002316?&kLionProgram:&kRokadeProgram;
        buffer.fill(0);finder.fill(0);branches.fill(0);
        put(package.data(),0x110,buffer.data());put(package.data(),0x118,spec->bytes);
        put(package.data(),0x120,spec->finders);put(package.data(),0x128,finder.data());
        put(package.data(),0x130,spec->branches);put(package.data(),0x138,branches.data());
        const auto branch=branches.data()+spec->branch*52;
        put(branch,20,spec->target);put(branch,36,spec->root);put(branch,42,std::uint8_t{7});
        for(std::size_t i=0;i<spec->nodes.size();++i) {
            const auto& n=spec->nodes[i];finder[spec->root+i]=n.word;
            for(std::size_t j=0;j<n.count;++j)buffer[n.word+j]=n.data[j];
        }
        reference=reinterpret_cast<Address>(buffer.data()+spec->leaf);
    }
};
using FloatCompare=bool(__fastcall*)(float,float,std::uint32_t);
FloatCompare native_compare{};
Fixture* expected{};bool modified{},throw_native{};unsigned calls{};void* expected_control{};
std::uint8_t __fastcall original(void* control,const void* reference,void* shared,void* context,
    std::uint8_t mode,void* flag,void* output) {
    assert(control==expected_control && shared==expected->package.data()+0x20 && context==expected);
    assert(mode==0xa5 && flag==&calls && output==expected->output.data());
    const auto leaf=NativeRead{}.at<Address>(reinterpret_cast<Address>(reference));
    const auto threshold=NativeRead{}.at<float>(leaf+4);
    if(modified) {
        assert(reference!=&expected->reference && leaf!=expected->reference && threshold==25.0f);
        assert(NativeRead{}.at<std::uint32_t>(leaf)==404 && NativeRead{}.at<std::uint32_t>(leaf+8)==4);
        assert(NativeRead{}.at<std::uint64_t>(leaf+12)==0);
    } else assert(reference==&expected->reference);
    ++calls;put(output,0,std::uint32_t{0xdeadbeef});
    if (throw_native)throw std::runtime_error("owned original exception");
    // Native 242db10 operator 4 is strict greater-than. Handler 404 inverts
    // that bool into the engine's zero=true / one=false condition status.
    return static_cast<std::uint8_t>(!native_compare(expected->distance,threshold,NativeRead{}.at<std::uint32_t>(leaf+8)));
}
void invoke(Fixture& f,bool change,bool server=false,bool enabled=true) {
    expected=&f;modified=change;expected_control=server?static_cast<void*>(f.ms.control.data()):f.mc.control.data();
    const auto buffer=f.buffer;const auto finder=f.finder;const auto branches=f.branches;const auto reference=f.reference;
    const float limit=change?25.0f:NativeRead{}.at<float>(reference+4);
    const auto opcode=NativeRead{}.at<std::uint32_t>(reference+8);
    assert(dispatch(original,f.base,expected_control,&f.reference,f.package.data()+0x20,&f,0xa5,&calls,f.output.data(),enabled)==!native_compare(f.distance,limit,opcode));
    assert(f.buffer==buffer && f.finder==finder && f.branches==branches && f.reference==reference);
}
struct ChangingRead:NativeRead {
    Address watched{};unsigned reads{};
    template<class T>T at(Address at) {
        const auto result=NativeRead::at<T>(at);
        if(at==watched && ++reads==2)return T{};
        return result;
    }
};
struct ChangingCopy:NativeRead {
    Address watched{};
    void copy(Address at,void* output,std::size_t count) {
        NativeRead::copy(at,output,count);
        if (at==watched && count==20)put(output,4,6.0f);
    }
};
struct Backend {
    bool fail_pin{},fail_init{},fail_create{},null_trampoline{},fail_enable{},fail_disable{},fail_remove{},fail_uninit{};
    unsigned pins{},initializations{},creates{},enables{},disables{},removes{},uninitializations{};
    bool pin() {++pins;return !fail_pin;}
    bool initialize() {++initializations;return !fail_init;}
    bool create(Address,Address,Address& trampoline) {++creates;trampoline=null_trampoline ? 0 : 0x12345;return !fail_create;}
    bool enable(Address) {++enables;return !fail_enable;}
    bool disable(Address) {++disables;return !fail_disable;}
    bool remove(Address) {++removes;return !fail_remove;}
    bool uninitialize() {++uninitializations;return !fail_uninit;}
};
void lifecycle() {
    for (unsigned failure=0;failure<9;++failure) {
        Backend backend;std::atomic<bool> enabled=false;Address published{};
        backend.fail_pin=failure==0;backend.fail_init=failure==1;backend.fail_create=failure==2;
        backend.null_trampoline=failure==3;backend.fail_enable=failure==5 || failure==6;
        backend.fail_disable=failure==6;backend.fail_remove=failure==7;backend.fail_uninit=failure==8;
        HookTransaction<Backend> hooks(backend,enabled);
        const bool ok=hooks.install(0x10000000,0x123,[]{return true;},[&](Address value) {
            published=value;return failure!=4 && failure!=7 && failure!=8;
        });
        assert(!ok && !enabled);
        assert(!hooks.install(0x10000000,0x123,[]{return true;},[](Address){return true;}));
        if (failure==5 || failure==6) {
            assert(published==0x12345 && backend.enables==1 && backend.disables==1 &&
                backend.removes==0 && backend.uninitializations==0);
        }
        if (failure==7) assert(backend.removes==1 && backend.uninitializations==0);
        if (failure==8) assert(backend.removes==1 && backend.uninitializations==1);
    }
    Backend backend;std::atomic<bool> enabled=false;Address published{};
    HookTransaction<Backend> hooks(backend,enabled);
    assert(hooks.install(0x10000000,0x123,[]{return true;},[&](Address p){published=p;return true;}));
    assert(enabled && published==0x12345 && hooks.cleanup() && !enabled);
    assert(backend.removes==0 && backend.uninitializations==0); // In-flight original remains valid.
}
} // namespace
int main() {
    // Execute only a copied, self-contained native float comparator in owned
    // fixture memory. Its valid 0..5 cases have no external calls; none of the
    // game's process, original image addresses or reference helpers execute.
    auto* compare_code=VirtualAlloc(nullptr,sizeof(kCode3),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    assert(compare_code);std::memcpy(compare_code,kCode3,sizeof(kCode3));
    DWORD protection{};assert(VirtualProtect(compare_code,sizeof(kCode3),PAGE_EXECUTE_READ,&protection));
    assert(FlushInstructionCache(GetCurrentProcess(),compare_code,sizeof(kCode3)));
    native_compare=reinterpret_cast<FloatCompare>(compare_code);
    assert(!native_compare(5.f,5.f,4) && native_compare(5.01f,5.f,4));
    assert(!native_compare(25.f,25.f,4) && native_compare(25.01f,25.f,4));
    lifecycle();
    for(auto key:{1002316u,31378u}) {
        Fixture f(key);
        for(bool server:{false,true})for(float height:{0.f,4.99f,5.f,5.01f,24.99f,25.f,25.01f,100.f}) {
            f.distance=height;invoke(f,true,server);invoke(f,true,server);
        }
        for(auto seat:{std::uint8_t{1},std::uint8_t{15}}) {f.seat(seat);invoke(f,true);}
        f.seat(0);
        invoke(f,false,false,false);
        for(auto other:{0u,1u,4u,6u,31377u,1002317u}) {put(f.definition.data(),0,other);invoke(f,false);}
        put(f.definition.data(),0,key);
        f.program_for(key==1002316?31378:1002316);invoke(f,false);
    }
    for(unsigned test=0;test<30;++test) {
        Fixture f;
        switch(test) {
        case 0:put(f.package.data(),0x118,std::uint32_t{1101});break;
        case 1:put(f.package.data(),0x120,std::uint32_t{168});break;
        case 2:put(f.package.data(),0x130,std::uint32_t{95});break;
        case 3:put(f.branches.data()+f.spec->branch*52,20,std::uint32_t{0});break;
        case 4:put(f.branches.data()+f.spec->branch*52,36,std::uint16_t{28});break;
        case 5:put(f.branches.data()+f.spec->branch*52,42,std::uint8_t{6});break;
        case 6:f.finder[f.spec->root+5]++;break;
        case 7:f.buffer[12+2]=38;break;
        case 8:f.buffer[f.spec->leaf+1]=0;break;
        case 9:f.buffer[f.spec->leaf+1]=0x7f800000;break;
        case 10:f.buffer[f.spec->leaf+2]=5;break;
        case 11:f.reference+=4;break;
        case 12:put(f.mc.actor.data(),0x96,std::uint8_t{0});break;
        case 13:put(f.ms.actor.data(),0x96,std::uint8_t{0});break;
        case 14:put(f.mc.control.data(),8,Address{});break;
        case 15:put(f.ms.vehicle.data(),0x48,std::uint32_t{0});break;
        case 16:put(f.mc.vehicle.data(),0xc8,std::uint8_t{2});break;
        case 17:put(f.mc.vehicle.data(),0x4c,BaseFixture::rider_id);break;
        case 18:put(f.s.vehicle_data.data(),0x14,std::uint8_t{1});break;
        case 19:put(f.c.vehicle_data.data(),0x10,std::uint32_t{0});break;
        case 20:put(f.mc.status.data(),0x30,std::uint16_t{40});break;
        case 21:put(f.cm.data(),0x50,Address{});break;
        case 22:put(f.package.data(),0x128,Address{1});break;
        case 23:put(f.package.data(),0x110,Address{1});break;
        case 24:put(f.mc.state.data(),0x68,Address{});break;
        case 25:f.finder[0]=static_cast<std::uint16_t>(f.spec->leaf);break;
        case 26:put(f.branches.data(),36,std::uint16_t{32});put(f.branches.data(),42,std::uint8_t{1});break;
        case 27:put(f.package.data(),0x128,UINTPTR_MAX-1);break;
        case 28:put(f.package.data(),0x138,UINTPTR_MAX-1);break;
        case 29:put(f.package.data(),0x110,UINTPTR_MAX-1);break;
        }
        invoke(f,false);
    }
    {
        Fixture f;ChangingRead read;Prepared result{};
        read.watched=reinterpret_cast<Address>(&f.reference);
        assert(!prepare(read,f.base,reinterpret_cast<Address>(f.mc.control.data()),reinterpret_cast<Address>(&f.reference),reinterpret_cast<Address>(f.package.data()+0x20),result));
    }
    {
        Fixture f;ChangingCopy read;Prepared result{};
        read.watched=f.reference;
        assert(!prepare(read,f.base,reinterpret_cast<Address>(f.mc.control.data()),reinterpret_cast<Address>(&f.reference),reinterpret_cast<Address>(f.package.data()+0x20),result));
    }
    {
        Fixture f;throw_native=true;bool caught=false;const auto before=calls;
        try {invoke(f,true);}catch(const std::runtime_error&) {caught=true;}
        assert(caught && calls==before+1);throw_native=false;
    }
    assert(VirtualFree(compare_code,0,MEM_RELEASE));
    std::printf("PASS: %u isolated dispatch checks; both selected mounts and representations, exact native programs, 5/25 boundaries, seats, changes, invalid reads, original arguments/status and immutable inputs. No game calls.\n",calls);
}
