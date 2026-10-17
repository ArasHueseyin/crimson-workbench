#include <Windows.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>
#include "../glide/glide_policy.h"
#include "../glide/glide_hooks.h"
#include "../glide/glide_settings.h"
#ifdef CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
#include CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
namespace fixture_data=crimson::marketplace::glide::fixture;
#else
#include "glide_synthetic_fixture.h"
namespace fixture_data=crimson::marketplace::glide::synthetic_fixture;
#endif
using namespace crimson::marketplace::glide;
namespace {
#ifdef CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
using FixtureAnimations=KnownAnimations;
#else
using FixtureAnimations=synthetic_fixture::SyntheticAnimations;
#endif
unsigned invocations{};
std::uintptr_t fixture_dispatch(Speed original,Address base,Address caller,bool enabled,std::uint32_t percent,
                                void* method,const void* params,void* output) {
    return dispatch(original,base,caller,enabled,percent,method,params,output,FixtureAnimations{});
}
template<class T,std::size_t N> void put(std::array<unsigned char,N>& a,std::size_t offset,T value) {
    assert(offset+sizeof(value)<=N);std::memcpy(a.data()+offset,&value,sizeof(value));
}
template<std::size_t N> Address address(std::array<unsigned char,N>& a) {return reinterpret_cast<Address>(a.data());}
struct Fixture {
    Address root_slot{},base{};
    std::array<unsigned char,64> root{},metadata{},selector{},layer{},view{},conditions{};
    std::array<unsigned char,256> manager{},actor{},possessor{},components{},animation{},instances{};
    std::array<unsigned char,1024> control{},state{},method{};
    std::array<unsigned char,512> controller{},package{},params{},output{};
    std::array<unsigned char,156> fixed{};
    std::array<unsigned char,kEventBytes> buffer{};
    Fixture() {
        base=reinterpret_cast<Address>(&root_slot)-0x6d69190;root_slot=address(root);
        put(root,0x30,address(manager));put(manager,0,base+0x55b62c8);put(manager,0x50,address(actor));
        put(actor,0,base+0x5585c40);put(actor,0x60,std::uint32_t{0x12345678});put(actor,0x96,std::uint8_t{1});
        put(actor,0x88,address(metadata));put(metadata,1,std::uint8_t{1});put(actor,0x68,address(components));
        put(actor,0x70,std::uint32_t{9});put(actor,0xa0,address(possessor));put(possessor,0xd0,address(actor));
        put(components,0x40,address(control));put(control,0,base+0x55b5460);put(control,8,address(actor));
        put(control,0xc8,address(state));put(control,0x140,address(controller));put(state,0x60,address(control));
        put(state,0x68,address(actor));put(state,0xf0,std::uint8_t{0});
        for (unsigned i=0;i<4;++i) put(state,0xf8+8*i,address(method));
        put(method,0,base+0x55cab28);put(method,0x18,address(control));put(method,0x20,address(actor));
        put(control,0x30,address(selector));put(selector,0x1b,std::uint8_t{0});put(control,0x88,address(layer));
        put(layer,0x18,address(animation));put(animation,0x38,address(fixed));put(animation,0x10,address(package));
        put(animation,0x20,address(view));put(animation,0x30,address(conditions));put(view,0,address(instances));
        put(view,8,fixture_data::kSustainedEvents);put(package,0x1a0,address(buffer));
        put(package,0x1a8,kEventBytes);put(package,0x118,kConditionBytes);put(package,0x120,kFinderCount);
        fixed=fixture_data::kSustained;put(params,0x6c,0.5f);
        put(params,0xf0,10.0f);put(params,0xf4,20.0f);put(params,0xf8,30.0f);
        put(method,0xd8,1.0f);put(method,0xdc,2.0f);put(method,0xe0,3.0f);put(method,0xe4,4.0f);
    }
};
constexpr Address result_value=0xfedcba9876543210ull;
void* expected_method{};const void* expected_params{};void* expected_output{};
unsigned native_calls{};std::function<void()> native_effect;
bool recurse{};Fixture* current{};std::uint32_t current_percent{};
std::array<float,3> original_velocity{2.0f,-3.0f,4.0f};
std::uintptr_t __fastcall native_speed(void* method,const void* params,void* output) {
    ++native_calls;assert(method==expected_method && params==expected_params && output==expected_output);
    if (output) std::memcpy(static_cast<unsigned char*>(output)+0x48,original_velocity.data(),12);
    if (native_effect) native_effect();
    if (recurse) {
        recurse=false;
        assert(fixture_dispatch(native_speed,current->base,current->base+kReturns[0],true,current_percent,
            method,params,output)==result_value);
    }
    return result_value;
}
void expect_arguments(void* method,const void* params,void* output) {
    expected_method=method;expected_params=params;expected_output=output;
}
void expect_velocity(const Fixture& f,float multiplier) {
    float actual[3]{};std::memcpy(actual,f.output.data()+0x48,12);
    assert(actual[0]==original_velocity[0]*multiplier && actual[2]==original_velocity[2]*multiplier);
    assert(std::memcmp(&actual[1],&original_velocity[1],4)==0);
}
void invoke(Fixture& f,std::uint32_t percent,float expected,Address caller=0,bool enabled=true) {
    const auto input=f.params;const auto record=f.fixed;const auto package=f.package;const auto previous=f.output;
    native_calls=0;current=&f;current_percent=percent;
    expect_arguments(f.method.data(),f.params.data(),f.output.data());
    assert(fixture_dispatch(native_speed,f.base,caller?caller:f.base+kReturns[0],enabled,percent,
        f.method.data(),f.params.data(),f.output.data())==result_value);
    assert(native_calls==1);expect_velocity(f,expected);
    assert(f.params==input && f.fixed==record && f.package==package);
    for (std::size_t i=0;i<f.output.size();++i)
        if (i<0x48 || i>=0x54) assert(f.output[i]==previous[i]);
    ++invocations;
}
void settings_tests() {
    unsigned char bytes[9]={'C','W','G','1',0,0,0,0,0};
    for (std::uint32_t value=100;value<=500;++value) {
        std::memcpy(bytes+4,&value,4);assert(setting(bytes,8)==value);
    }
    for (auto value:{0u,99u,501u,UINT32_MAX}) {
        std::memcpy(bytes+4,&value,4);assert(setting(bytes,8)==100);
    }
    for (std::size_t size=0;size<=9;++size) if (size!=8) assert(setting(bytes,size)==100);
    assert(setting(nullptr,8)==100);bytes[0]='X';assert(setting(bytes,8)==100);
}
void dispatch_tests() {
    Fixture f;
    for (auto kind:std::array<unsigned char,2>{1,4}) for (unsigned char mode=0;mode<4;++mode)
        for (auto caller:kReturns) for (auto percent:{101u,125u,200u,300u,500u}) {
            put(f.metadata,1,kind);put(f.state,0xf0,mode);
            invoke(f,percent,static_cast<float>(percent)/100.0f,f.base+caller);
        }
    for (auto value:{0u,99u,100u,501u,UINT32_MAX}) invoke(f,value,1);
    invoke(f,500,1,0,false);invoke(f,500,1,f.base+0xab8774);
    // Repeated calls consume fresh native steering speed. Never scale yesterday's output again.
    for (unsigned i=0;i<25;++i) invoke(f,500,5);
    original_velocity={-4.5f,-0.0f,-7.0f};invoke(f,200,2);
    original_velocity={0,-3,0};invoke(f,500,5);
    original_velocity={2,-3,4};
}
void rejected_scope_tests() {
    Fixture f;
    std::vector<std::pair<Address,std::uint64_t>> bad={
        {address(f.method),f.base+0x55cab30},{address(f.method)+0x18,0},{address(f.method)+0x20,0},
        {reinterpret_cast<Address>(&f.root_slot),0},{address(f.root)+0x30,0},
        {address(f.manager),f.base+1},{address(f.manager)+0x50,address(f.actor)+8},
        {address(f.actor),f.base+1},{address(f.actor)+0x60,0},{address(f.actor)+0x96,0},
        {address(f.actor)+0x88,0},{address(f.metadata)+1,2},{address(f.actor)+0x68,0},
        {address(f.actor)+0x70,8},{address(f.actor)+0x70,58},{address(f.actor)+0xa0,0},
        {address(f.possessor)+0xd0,0},{address(f.components)+0x40,0},
        {address(f.control),f.base+1},{address(f.control)+8,0},{address(f.control)+0xc8,0},
        {address(f.control)+0x140,0},{address(f.state)+0x60,0},{address(f.state)+0x68,0},
        {address(f.state)+0xf0,4},{address(f.state)+0xf8,0},{address(f.control)+0x30,0},
        {address(f.selector)+0x1b,1},{address(f.control)+0x88,0},{address(f.layer)+0x18,0},
        {address(f.animation)+0x38,0},{address(f.animation)+0x10,0},{address(f.animation)+0x20,0},
        {address(f.animation)+0x30,0},{address(f.view),0},{address(f.view)+8,1},
        {address(f.package)+0x1a0,0},{address(f.package)+0x1a8,kEventBytes+4},
        {address(f.package)+0x118,kConditionBytes+1},{address(f.package)+0x120,kFinderCount+1}};
    for (const auto& [location,value]:bad) {
        std::uint64_t previous{};std::memcpy(&previous,reinterpret_cast<void*>(location),8);
        std::memcpy(reinterpret_cast<void*>(location),&value,8);invoke(f,500,1);
        std::memcpy(reinterpret_cast<void*>(location),&previous,8);
    }
    // Every byte contributes to the classification, including fabricated records.
    for (std::size_t i=0;i<f.fixed.size();++i) {f.fixed[i]^=1;invoke(f,500,1);f.fixed[i]^=1;}
    f.fixed=fixture_data::kLanding;put(f.view,8,fixture_data::kLandingEvents);invoke(f,500,1);
    f.fixed=fixture_data::kSustained;put(f.view,8,fixture_data::kSustainedEvents);
    for (auto value:{0.0f,-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        put(f.params,0x6c,value);invoke(f,500,1);
    }
    put(f.params,0x6c,0.5f);
    const auto source=original_velocity;
    for (unsigned component=0;component<3;++component) {
        original_velocity[component]=std::numeric_limits<float>::infinity();invoke(f,500,1);
        original_velocity=source;
    }
    original_velocity[0]=std::numeric_limits<float>::max();invoke(f,500,1);original_velocity=source;
}
void stale_and_recursive_tests() {
    Fixture f;native_effect=[&] {put(f.actor,0x60,std::uint32_t{123});};
    invoke(f,500,1);native_effect={};put(f.actor,0x60,std::uint32_t{0x12345678});
    native_effect=[&] {put(f.selector,0x1b,std::uint8_t{1});};
    invoke(f,500,1);native_effect={};put(f.selector,0x1b,std::uint8_t{0});
    native_effect=[&] {f.fixed=fixture_data::kLanding;put(f.view,8,fixture_data::kLandingEvents);};
    // These two snapshots intentionally change across native execution.
    native_calls=0;expect_arguments(f.method.data(),f.params.data(),f.output.data());
    assert(fixture_dispatch(native_speed,f.base,f.base+kReturns[0],true,500,
        f.method.data(),f.params.data(),f.output.data())==result_value);
    expect_velocity(f,1);assert(native_calls==1);native_effect={};
    f.fixed=fixture_data::kSustained;put(f.view,8,fixture_data::kSustainedEvents);
    native_effect=[&] {put(f.params,0x6c,0.25f);};
    native_calls=0;expect_arguments(f.method.data(),f.params.data(),f.output.data());
    fixture_dispatch(native_speed,f.base,f.base+kReturns[1],true,500,f.method.data(),f.params.data(),f.output.data());
    expect_velocity(f,1);assert(native_calls==1);native_effect={};put(f.params,0x6c,0.5f);
    current=&f;current_percent=200;recurse=true;native_calls=0;
    assert(fixture_dispatch(native_speed,f.base,f.base+kReturns[0],true,200,
        f.method.data(),f.params.data(),f.output.data())==result_value);
    assert(native_calls==2);expect_velocity(f,2);
    // Bad borrowed addresses are caught in the guard; original args and full RAX survive.
    expected_method=reinterpret_cast<void*>(1);native_calls=0;
    assert(fixture_dispatch(native_speed,f.base,f.base+kReturns[0],true,200,
        expected_method,f.params.data(),f.output.data())==result_value);
    assert(native_calls==1);expect_velocity(f,1);
}
struct Refusal {
    Memory m;Address fail{},foreign{};unsigned attempts{};
    template<class T> T at(Address p) {return m.at<T>(p);}
    bool compare(Address p,std::uint32_t before,std::uint32_t after) {
        ++attempts;
        if (p==fail) {if (foreign) {const float value=777;std::memcpy(reinterpret_cast<void*>(foreign),&value,4);}return false;}
        return m.compare(p,before,after);
    }
};
void compare_publish_tests() {
    Fixture f;Memory memory;std::memcpy(f.output.data()+0x48,original_velocity.data(),12);
    Velocity v{};assert(prepare(memory,address(f.output),500,v));
    assert(publish(memory,address(f.output),v));expect_velocity(f,5);
    std::memcpy(f.output.data()+0x48,original_velocity.data(),12);
    Refusal second;second.fail=address(f.output)+0x50;
    assert(!publish(second,address(f.output),v));expect_velocity(f,1);assert(second.attempts==3);
    Refusal first;first.fail=address(f.output)+0x48;
    assert(!publish(first,address(f.output),v));expect_velocity(f,1);assert(first.attempts==1);
    Refusal foreign;foreign.fail=address(f.output)+0x50;foreign.foreign=address(f.output)+0x48;
    assert(!publish(foreign,address(f.output),v));assert(memory.at<float>(address(f.output)+0x48)==777);
    assert(memory.at<float>(address(f.output)+0x4c)==-3 && memory.at<float>(address(f.output)+0x50)==4);
    std::memcpy(f.output.data()+0x48,original_velocity.data(),12);put(f.output,0x4c,999.0f);
    assert(!publish(memory,address(f.output),v));assert(memory.at<float>(address(f.output)+0x48)==2);
}
void classification_tests() {
    for (const auto& animation:kAnimations)
        assert(KnownAnimations{}(animation.fingerprint,animation.events));
    assert(!KnownAnimations{}(0,0));
#ifndef CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
    // A public fixture can never become an accepted production fingerprint.
    assert(!KnownAnimations{}(synthetic_fixture::fingerprint(fixture_data::kSustained),fixture_data::kSustainedEvents));
    Fixture f;Memory memory;Scope scope{};
    assert(!capture(memory,f.base,address(f.method),scope));
    assert(capture(memory,f.base,address(f.method),scope,FixtureAnimations{}));
#endif
}
void synthetic_integration_test() {
    // Our C++ consumer checks the dispatch output contract. It is deliberately
    // not a copy of the engine's integration instructions.
    Fixture f;current=&f;expect_arguments(f.method.data(),f.params.data(),f.output.data());
    for (auto percent:{100u,125u,200u,500u}) for (unsigned repeat=0;repeat<4;++repeat) {
        const auto input=f.params;current_percent=percent;native_calls=0;
        fixture_dispatch(native_speed,f.base,f.base+kReturns[1],true,percent,f.method.data(),f.params.data(),f.output.data());
        assert(native_calls==1);float position[3]{},velocity[3]{};
        std::memcpy(position,f.params.data()+0xf0,12);std::memcpy(velocity,f.output.data()+0x48,12);
        float dt{};std::memcpy(&dt,f.params.data()+0x6c,4);
        for (unsigned axis=0;axis<3;++axis) position[axis]+=dt*velocity[axis];
        const float factor=static_cast<float>(percent)/100;
        assert(position[0]==10+0.5f*2*factor && position[1]==20+0.5f*(-3) && position[2]==30+0.5f*4*factor);
        expect_velocity(f,factor);assert(f.params==input);++invocations;
    }
}
#ifdef CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
std::uintptr_t __fastcall integrated_dispatch(void* method,const void* params,void* output) {
    return fixture_dispatch(native_speed,current->base,current->base+kReturns[1],true,current_percent,method,params,output);
}
void copied_native_integration_test() {
    const auto block=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(block);std::memcpy(block,fixture_data::kIntegration.data(),fixture_data::kIntegration.size());
    const std::int32_t call=0x120-(0x24+5);std::memcpy(block+0x25,&call,4);
    const std::int32_t constant=0x140-(0x45+8);std::memcpy(block+0x49,&constant,4);
    block[0x120]=0x48;block[0x121]=0xb8;
    const auto callback=reinterpret_cast<Address>(&integrated_dispatch);std::memcpy(block+0x122,&callback,8);
    block[0x12a]=0xff;block[0x12b]=0xe0;const float cell=1000;std::memcpy(block+0x140,&cell,4);
    DWORD old{};assert(VirtualProtect(block,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),block,4096);
    using Integrate=void(__fastcall*)(void*,const void*,void*);const auto integrate=reinterpret_cast<Integrate>(block);
    Fixture f;current=&f;expect_arguments(f.method.data(),f.params.data(),f.output.data());
    for (auto percent:{100u,125u,200u,500u}) for (unsigned repeat=0;repeat<4;++repeat) {
        const auto input=f.params;current_percent=percent;native_calls=0;
        integrate(f.method.data(),f.params.data(),f.output.data());assert(native_calls==1);
        float xyz[3]{};std::memcpy(xyz,f.output.data(),12);const float factor=static_cast<float>(percent)/100;
        assert(xyz[0]==10+0.5f*2*factor && xyz[1]==20+0.5f*(-3) && xyz[2]==30+0.5f*4*factor);
        expect_velocity(f,factor);assert(f.params==input);
        assert(std::memcmp(f.output.data()+0x10,f.method.data()+0xd8,16)==0);++invocations;
    }
    assert(VirtualFree(block,0,MEM_RELEASE));
}
#endif
struct Backend {
    bool pin_ok=true,init_ok=true,create_ok=true,enable_ok=true,disable_ok=true,remove_ok=true;
    unsigned pins{},inits{},creates{},enables{},disables{},removes{},uninits{};
    bool pin(){++pins;return pin_ok;}bool initialize(){++inits;return init_ok;}
    bool create(Address,Address,Address& out){++creates;out=0x9876;return create_ok;}
    bool enable(Address){++enables;return enable_ok;}bool disable(Address){++disables;return disable_ok;}
    bool remove(Address){++removes;return remove_ok;}bool uninitialize(){++uninits;return true;}
};
void hooks_tests() {
    for (unsigned failure=0;failure<7;++failure) {
        Backend b;std::atomic<bool> enabled=true;HookTransaction<Backend> tx(b,enabled);unsigned published{};
        b.pin_ok=failure!=1;b.init_ok=failure!=2;b.create_ok=failure!=3;b.enable_ok=failure!=5;
        const bool ok=tx.install(0x140000000,0x1234,[&]{return failure!=0;},[&](Address a){++published;assert(a==0x9876);return failure!=4;});
        assert(ok==(failure==6));assert(enabled==ok);
        assert(!tx.install(0x140000000,0x1234,[]{return true;},[](Address){return true;}));
        if (failure==5 || failure==6) {assert(!b.removes && !b.uninits && published==1);tx.cleanup();assert(!b.removes && !b.uninits);}
        if (failure==4) assert(b.removes==1 && b.uninits==1);
    }
}
} // namespace
int main() {
    settings_tests();classification_tests();dispatch_tests();rejected_scope_tests();stale_and_recursive_tests();
    compare_publish_tests();synthetic_integration_test();hooks_tests();
#ifdef CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER
    copied_native_integration_test();
    std::puts("Private original-image integration fixture also passed.");
#endif
    std::printf("Glide speed: %u owned invocations, synthetic consumer, selected actor/layer guards, settings and hook lifetime passed.\n",invocations);
}
