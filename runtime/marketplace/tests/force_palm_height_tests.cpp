#include "../force-palm-height/force_palm_height_policy.h"
#include "../force-palm-height/force_palm_height_settings.h"
#include "../force-palm-height/force_palm_height_targets.h"
#include "../force-palm-height/force_palm_height_hooks.h"
#include "../jump/player_jump_policy.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
#define force_palm_height force_palm_height_original_fixture
#include CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
#undef force_palm_height
namespace fixture_data=crimson::marketplace::force_palm_height_original_fixture;
#else
#include "force_palm_height_synthetic_fixture.h"
namespace fixture_data=crimson::marketplace::force_palm_height::synthetic_fixture;
#endif

namespace {
using namespace crimson::marketplace::force_palm_height;
using FixtureLaunch=fixture_data::Launch;
struct FixtureRecords {
    const auto& animations() const {
        static constexpr auto values=[] {
            std::array<Animation,std::size(fixture_data::kAnimations)> result{};
            for (std::size_t i=0;i<result.size();++i)
                result[i]={fingerprint(fixture_data::kAnimations[i].fixed),fixture_data::kAnimations[i].events};
            return result;
        }();return values;
    }
    const auto& launches() const {
        static constexpr auto values=[] {
            std::array<Launch,std::size(fixture_data::kLaunches)> result{};
            for (std::size_t i=0;i<result.size();++i) {
                const auto& source=fixture_data::kLaunches[i];
                result[i]={source.animation,source.index,source.word,fingerprint(source.instance),fingerprint(source.data)};
            }
            return result;
        }();return values;
    }
};
template<class T> void put(void* at,std::size_t offset,T value) {std::memcpy(static_cast<unsigned char*>(at)+offset,&value,sizeof(value));}
template<class T> T get(const void* at,std::size_t offset) {T value{};std::memcpy(&value,static_cast<const unsigned char*>(at)+offset,sizeof(value));return value;}
struct Fixture {
    Address base{};
    std::array<unsigned char,0x38> root{};
    std::array<unsigned char,0x58> manager{};
    std::array<unsigned char,0xb0> actor{};
    std::array<unsigned char,2> metadata{};
    std::array<unsigned char,0x48> components{};
    std::array<unsigned char,0xd8> possessor{};
    std::array<unsigned char,0xd0> control{};
    std::array<unsigned char,0x380> state{};
    std::array<unsigned char,0x20> layer{};
    std::array<unsigned char,0x40> animation{};
    std::array<unsigned char,156> fixed{};
    std::array<unsigned char,0x1b0> package{};
    std::array<unsigned char,0x20> conditions{};
    std::array<unsigned char,0x10> instance_view{};
    std::array<unsigned char,800> instances{};
    std::array<unsigned char,80> event{};
    std::vector<unsigned char> buffer=std::vector<unsigned char>(kEventBytes);
    Address actor_reference{};
    const FixtureLaunch* launch{};
    Fixture() {
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d6a000,MEM_RESERVE,PAGE_NOACCESS));
        assert(base && VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        put(reinterpret_cast<void*>(base+crimson::marketplace::stamina::kClientRoot),0,root.data());
        put(root.data(),0x30,manager.data());
        put(manager.data(),0,base+crimson::marketplace::stamina::kClientManager);
        put(manager.data(),0x50,actor.data());
        put(actor.data(),0,base+crimson::marketplace::stamina::kClientActor);
        put(actor.data(),0x60,std::uint32_t{0xa0011234});
        put(actor.data(),0x68,components.data());put(actor.data(),0x70,std::uint32_t{24});
        put(actor.data(),0x88,metadata.data());metadata[1]=1;actor[0x96]=1;
        put(actor.data(),0xa0,possessor.data());put(possessor.data(),0xd0,actor.data());
        put(components.data(),0x40,control.data());put(control.data(),0,base+0x55b5460);
        put(control.data(),8,actor.data());put(control.data(),0xc8,state.data());
        put(state.data(),0x68,actor.data());
        actor_reference=reinterpret_cast<Address>(actor.data());
        select(fixture_data::kLaunches[0]);
    }
    void select(const FixtureLaunch& value) {
        launch=&value;fixed=fixture_data::kAnimations[value.animation].fixed;
        put(control.data(),0x90,layer.data());put(layer.data(),0x18,animation.data());
        put(animation.data(),0x10,package.data());put(animation.data(),0x20,instance_view.data());
        put(animation.data(),0x30,conditions.data());put(animation.data(),0x38,fixed.data());
        put(instance_view.data(),0,instances.data());instance_view[8]=fixture_data::kAnimations[value.animation].events;
        put(package.data(),0x1a8,kEventBytes);put(package.data(),0x118,kConditionBytes);
        put(package.data(),0x120,kFinderCount);put(package.data(),0x1a0,buffer.data());
        std::memcpy(instances.data()+value.index*20,value.instance.data(),20);
        std::memcpy(buffer.data()+value.word*4,value.data.data(),26);
        put(event.data(),0,instances.data()+value.index*20);put(event.data(),8,buffer.data()+value.word*4);
        put(event.data(),0x10,conditions.data());put(event.data(),0x18,package.data()+0x20);
        for (std::size_t i=0x20;i<event.size();++i) event[i]=static_cast<unsigned char>(i);
    }
    ~Fixture() {assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE));}
    Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
};
Fixture* fixture_ptr{};float expected{};bool changed{},throw_native{},apply_ordinary{};
unsigned calls{},ordinary_calls{};
bool __fastcall ordinary(void* state,std::uintptr_t a,std::uint8_t b,std::uint8_t c) {
    assert(state==fixture_ptr->state.data() && a==123 && b==4 && c==5);
    assert(get<float>(state,0x32c)==expected);++ordinary_calls;
    return true;
}
void* __fastcall original(void* self,void* result,const void* event,std::uintptr_t time,const void* actor) {
    assert(self==reinterpret_cast<void*>(17) && result==reinterpret_cast<void*>(19));
    assert(time==0xfedcba9876543210 && actor==&fixture_ptr->actor_reference);++calls;
    if ((event!=fixture_ptr->event.data())!=changed)
        std::fprintf(stderr,"Unexpected adaptation at call %u, animation %u, instance %u, expected %s.\n",calls,
            fixture_ptr->launch->animation,fixture_ptr->launch->index,changed?"changed":"native");
    assert((event!=fixture_ptr->event.data())==changed);
    const auto data=get<const unsigned char*>(event,8);
    assert(get<float>(data,12)==expected);
    for (std::size_t i=0;i<26;++i) if (i<12 || i>=16) assert(data[i]==fixture_ptr->launch->data[i]);
    for (std::size_t i=0;i<80;++i) if (i<8 || i>=16)
        assert(static_cast<const unsigned char*>(event)[i]==fixture_ptr->event[i]);
    if (throw_native) throw std::runtime_error("native error remains native");
    if (apply_ordinary) {
        fixture_ptr->state[0x320]=1;put(fixture_ptr->state.data(),0x32c,get<float>(data,12));
        assert(crimson::marketplace::player_jump::dispatch(ordinary,fixture_ptr->base,300,fixture_ptr->state.data(),123,4,5));
    }
    return result;
}
void invoke(Fixture& fixture,std::uint32_t percent,bool adapt,bool enabled=true) {
    fixture_ptr=&fixture;changed=adapt;
    expected=get<float>(fixture.launch->data.data(),12)*(adapt?static_cast<float>(percent)/100.0f:1.0f);
    const auto before=fixture.buffer;
    const auto refs=fixture.event;
    assert(dispatch(original,fixture.base,enabled,percent,reinterpret_cast<void*>(17),reinterpret_cast<void*>(19),
        fixture.event.data(),0xfedcba9876543210,&fixture.actor_reference,FixtureRecords{})==reinterpret_cast<void*>(19));
    assert(fixture.buffer==before && fixture.event==refs);
}
void* __fastcall invalid(void*,void*,const void* event,std::uintptr_t,const void*) {
    assert(event==reinterpret_cast<void*>(1));++calls;return reinterpret_cast<void*>(37);
}
struct Changing : Memory {
    Fixture& fixture;unsigned reads{};
    explicit Changing(Fixture& value):fixture(value) {}
    template<class T> T at(Address address) {
        const T result=Memory::at<T>(address);
        if (address==reinterpret_cast<Address>(fixture.actor.data())+0x60 && ++reads==3)
            put(fixture.manager.data(),0x50,Address{});
        return result;
    }
};
struct Backend {
    int fail{};unsigned stage{};bool published{},retained{},removed{};
    bool next() {return ++stage!=static_cast<unsigned>(fail);}
    bool pin(){return next();} bool initialize(){return next();}
    bool create(Address target,Address callback,Address& trampoline) {
        assert(target==0x100000+kEventRva && callback==0x200000);trampoline=0x300000;return next();
    }
    bool enable(Address){assert(published);retained=true;return next();}
    bool disable(Address){return true;} bool remove(Address){removed=true;return true;} bool uninitialize(){return true;}
};
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
// Exact pinned sqrt(2*abs(gravity)*height) instruction slice. This executes only
// an owned copy, with the sole RIP data reference relocated to an owned mask.
float copied_native_height(float height,float gravity) {
    std::array<unsigned char,128> bytes{};
    const unsigned char prefix[]={0x57,0x48,0x89,0xcf,0xc5,0xf8,0x28,0xd9};
    std::memcpy(bytes.data(),prefix,sizeof(prefix));
    static_assert(sizeof(kPin7)==34);
    std::memcpy(bytes.data()+sizeof(prefix),kPin7,sizeof(kPin7));
    bytes[42]=0x5f;bytes[43]=0xc3; // pop rdi; ret on the finite native path.
    bytes[44]=0x0f;bytes[45]=0x0b; // negative-native fallback traps rather than calling the engine.
    const std::uint32_t mask=0x7fffffff;
    for (std::size_t i=0;i<4;++i) put(bytes.data(),64+i*4,mask);
    put(bytes.data(),sizeof(prefix)+4,std::int32_t{64-static_cast<int>(sizeof(prefix))-8});
    void* copy=VirtualAlloc(nullptr,bytes.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(copy);
    std::memcpy(copy,bytes.data(),bytes.size());DWORD old{};assert(VirtualProtect(copy,bytes.size(),PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),copy,bytes.size()));
    std::array<unsigned char,0x330> state{};put(state.data(),0x32c,height);
    using Convert=float(__fastcall*)(void*,float);
    const float result=reinterpret_cast<Convert>(copy)(state.data(),gravity);
    assert(VirtualFree(copy,0,MEM_RELEASE));return result;
}
#endif
void classification_tests() {
    const auto& animations=FixtureRecords{}.animations();const auto& launches=FixtureRecords{}.launches();
    static_assert(std::size(fixture_data::kAnimations)==7 && std::size(fixture_data::kLaunches)==10);
    for (std::size_t i=0;i<animations.size();++i) {
        assert(animations[i].events==kAnimations[i].events);
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
        assert(animations[i].fingerprint==kAnimations[i].fingerprint);
#else
        assert(animations[i].fingerprint!=kAnimations[i].fingerprint);
#endif
    }
    for (std::size_t i=0;i<launches.size();++i) {
        const auto& a=launches[i];const auto& b=kLaunches[i];
        assert(a.animation==b.animation && a.index==b.index && a.word==b.word);
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
        assert(a.instance_fingerprint==b.instance_fingerprint && a.data_fingerprint==b.data_fingerprint);
#else
        assert(a.instance_fingerprint!=b.instance_fingerprint && a.data_fingerprint!=b.data_fingerprint);
#endif
    }
    Fixture f;Memory memory;Palm scope{};
    assert(palm_layer(memory,reinterpret_cast<Address>(f.control.data()),1,scope,FixtureRecords{}));
#ifndef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
    assert(!palm_layer(memory,reinterpret_cast<Address>(f.control.data()),1,scope));
#endif
    std::uint64_t value{};
    assert(!fingerprint<156>(memory,0,value) && !fingerprint<156>(memory,UINTPTR_MAX-155,value));
    assert(!equals<20>(memory,UINTPTR_MAX-19,0) && !equals<26>(memory,UINTPTR_MAX-25,0));
    for (std::size_t i=0;i<animations.size();++i) {
        f.fixed=fixture_data::kAnimations[i].fixed;f.instance_view[8]=animations[i].events;
        assert(palm_layer(memory,reinterpret_cast<Address>(f.control.data()),1,scope,FixtureRecords{}) && scope.index==i);
        ++f.instance_view[8];
        assert(!palm_layer(memory,reinterpret_cast<Address>(f.control.data()),1,scope,FixtureRecords{}));
        --f.instance_view[8];
        for (std::size_t byte=0;byte<f.fixed.size();++byte) {
            f.fixed[byte]^=1;
            assert(!palm_layer(memory,reinterpret_cast<Address>(f.control.data()),1,scope,FixtureRecords{}));
            f.fixed[byte]^=1;
        }
    }
}
void guard_page_tests() {
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(pages);
    DWORD old{};assert(VirtualProtect(pages+4096,4096,PAGE_NOACCESS,&old));
    const Address boundary=reinterpret_cast<Address>(pages+4096);
    for (unsigned kind=0;kind<3;++kind) {
        Fixture f;
        if (kind==0) put(f.animation.data(),0x38,boundary-155);
        if (kind==1) {
            put(f.instance_view.data(),0,boundary-19-f.launch->index*Address{20});
            put(f.event.data(),0,boundary-19);
        }
        if (kind==2) {
            put(f.package.data(),0x1a0,boundary-25-f.launch->word*Address{4});
            put(f.event.data(),8,boundary-25);
        }
        Prepared prepared{};
        assert(!guarded_prepare(f.base,10000,f.event.data(),&f.actor_reference,prepared,FixtureRecords{}));
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
}
int main() {
    classification_tests();guard_page_tests();
    for (std::uint32_t value:{0u,99u,100u,101u,140u,9999u,10000u,10001u,0xffffffffu}) {
        std::array<unsigned char,8> bytes{'C','W','F','1'};put(bytes.data(),4,value);
        assert(setting(bytes.data(),8)==(value>=100 && value<=10000?value:100));
        assert(setting(bytes.data(),7)==100 && setting(bytes.data(),9)==100);
        bytes[0]='X';assert(setting(bytes.data(),8)==100);
    }
    assert(setting(nullptr,8)==100);
    for (const auto& launch:fixture_data::kLaunches) for (auto percent:{101u,140u,250u,10000u}) {
        Fixture f;f.select(launch);for (unsigned repeat=0;repeat<5;++repeat) invoke(f,percent,true);
    }
    for (auto percent:{0u,99u,100u,10001u,0xffffffffu}) {Fixture f;invoke(f,percent,false);}
    {Fixture f;invoke(f,10000,false,false);}
    for (unsigned kind:{1u,4u}) {Fixture f;f.metadata[1]=static_cast<unsigned char>(kind);invoke(f,10000,true);}
    for (unsigned fail=0;fail<25;++fail) {
        Fixture f;
        switch(fail) {
        case 0:put(f.manager.data(),0x50,Address{});break;
        case 1:f.metadata[1]=6;break;
        case 2:put(f.actor.data(),0,Address{});break;
        case 3:f.actor[0x96]=0;break;
        case 4:put(f.actor.data(),0x60,std::uint32_t{});break;
        case 5:put(f.actor.data(),0x70,std::uint32_t{8});break;
        case 6:put(f.actor.data(),0x70,std::uint32_t{58});break;
        case 7:put(f.possessor.data(),0xd0,Address{});break;
        case 8:put(f.control.data(),0,Address{});break;
        case 9:put(f.control.data(),8,Address{});break;
        case 10:put(f.control.data(),0xc8,Address{});break;
        case 11:put(f.state.data(),0x68,Address{});break;
        case 12:put(f.control.data(),0x90,Address{});break;
        case 13:put(f.layer.data(),0x18,Address{});break;
        case 14:put(f.animation.data(),0x10,Address{});break;
        case 15:put(f.animation.data(),0x20,Address{});break;
        case 16:put(f.animation.data(),0x30,Address{});break;
        case 17:put(f.package.data(),0x1a8,kEventBytes-1);break;
        case 18:put(f.package.data(),0x118,kConditionBytes-1);break;
        case 19:put(f.package.data(),0x120,kFinderCount-1);break;
        case 20:--f.instance_view[8];break;
        case 21:put(f.event.data(),0,f.instances.data()+20);break;
        case 22:put(f.event.data(),0x10,Address{});break;
        case 23:put(f.event.data(),0x18,Address{});break;
        case 24:put(f.package.data(),0x1a0,Address{UINTPTR_MAX-10});break;
        }
        invoke(f,10000,false);
    }
    for (const auto& launch:fixture_data::kLaunches) {
        Fixture f;f.select(launch);
        for (std::size_t byte=0;byte<20;++byte) {
            f.instances[launch.index*20+byte]^=1;invoke(f,10000,false);f.instances[launch.index*20+byte]^=1;
        }
        for (std::size_t byte=0;byte<26;++byte) {
            f.buffer[launch.word*4+byte]^=1;
            // Native passthrough must preserve an intentionally modified source.
            Prepared prepared{};assert(!guarded_prepare(f.base,10000,f.event.data(),&f.actor_reference,prepared,FixtureRecords{}));
            f.buffer[launch.word*4+byte]^=1;
        }
    }
    {Fixture f;Changing memory(f);Prepared prepared{};assert(!prepare(memory,f.base,10000,reinterpret_cast<Address>(f.event.data()),reinterpret_cast<Address>(&f.actor_reference),prepared,FixtureRecords{}));}
    {Fixture f;Prepared prepared{};assert(!guarded_prepare(f.base,10000,reinterpret_cast<void*>(1),&f.actor_reference,prepared,FixtureRecords{}));}
    {
        // A fabricated setup-like event cannot masquerade as an approved launch.
        Fixture f;
        std::array<unsigned char,20> setup_instance{};setup_instance.fill(0x71);
        std::array<unsigned char,26> setup_data{};setup_data.fill(0x29);put(setup_data.data(),12,0.01f);
        std::memcpy(f.instances.data()+20,setup_instance.data(),setup_instance.size());
        std::memcpy(f.buffer.data()+0x1cf03*4,setup_data.data(),setup_data.size());
        put(f.event.data(),0,f.instances.data()+20);put(f.event.data(),8,f.buffer.data()+0x1cf03*4);
        Prepared prepared{};assert(!guarded_prepare(f.base,10000,f.event.data(),&f.actor_reference,prepared,FixtureRecords{}));
        assert(get<float>(f.buffer.data()+0x1cf03*4,12)==0.01f);
    }
    assert(dispatch(invalid,0,true,10000,nullptr,nullptr,reinterpret_cast<void*>(1),0,nullptr)==reinterpret_cast<void*>(37));
    {Fixture f;throw_native=true;try {invoke(f,10000,true);assert(false);}catch(const std::runtime_error&){}throw_native=false;}
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
    // Only the private original records can establish production classification
    // at the ordinary-jump integration boundary.
    for (const auto& launch:fixture_data::kLaunches) {Fixture f;f.select(launch);apply_ordinary=true;invoke(f,10000,true);apply_ordinary=false;}
    assert(ordinary_calls==std::size(fixture_data::kLaunches));
#endif
    // With ordinary animation data, the separate ordinary slider still scales.
    {Fixture f;f.fixed.fill(0);f.state[0x320]=1;put(f.state.data(),0x32c,2.5f);fixture_ptr=&f;expected=7.5f;
        assert(crimson::marketplace::player_jump::dispatch(ordinary,f.base,300,f.state.data(),123,4,5));}
    for (int failure=1;failure<=4;++failure) {
        Backend backend;backend.fail=failure;std::atomic<bool> enabled{};HookTransaction hooks(backend,enabled);
        assert(!hooks.install(0x100000,0x200000,[]{return true;},[&](Address t){assert(t==0x300000);backend.published=true;return true;}));
        assert(!enabled && (!backend.retained || !backend.removed));
    }
    {Backend backend;std::atomic<bool> enabled{};HookTransaction hooks(backend,enabled);
        assert(hooks.install(0x100000,0x200000,[]{return true;},[&](Address){backend.published=true;return true;}));
        assert(enabled && hooks.cleanup() && !enabled && !backend.removed);}
#ifdef CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER
    for (float height:{3.5f,4.5f,5.0f,5.5f,6.0f,7.0f,10.0f}) {
        const float native=copied_native_height(height,-9.81f);
        const float boosted=copied_native_height(height*100.0f,-9.81f);
        assert(std::fabs(native-std::sqrt(2.0f*9.81f*height))<0.0001f);
        assert(std::fabs(boosted/native-10.0f)<0.0001f);
    }
    std::puts("Private original records, ordinary-jump coexistence and copied-native physics also passed.");
#endif
    std::printf("PASS: owned Palm scope, 1x..100x height, forwarding, mutation/guard-page/race fallbacks and hook lifetime (%u event calls).\n",calls);
}
