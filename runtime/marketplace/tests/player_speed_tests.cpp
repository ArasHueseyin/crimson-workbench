#include "../speed/player_speed_policy.h"
#include "../speed/player_speed_settings.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
namespace {
using namespace crimson::marketplace::player_speed;
template<class T> void put(void* value, std::size_t offset, T data) { std::memcpy(static_cast<unsigned char*>(value) + offset, &data, sizeof(data)); }
template<class T> T get(const void* value, std::size_t offset) { T data; std::memcpy(&data, static_cast<const unsigned char*>(value) + offset, sizeof(data)); return data; }
struct Fixture {
    Address base{};
    std::array<unsigned char,0x38> root{};
    std::array<unsigned char,0x58> manager{};
    std::array<unsigned char,0xb0> actor{};
    std::array<unsigned char,2> metadata{};
    std::array<unsigned char,0x48> components{};
    std::array<unsigned char,0xd8> possessor{};
    std::array<unsigned char,0x148> control{};
    std::array<unsigned char,0x5c0> state{};
    std::array<unsigned char,0x2f0> controller{};
    std::array<unsigned char,0x28> method{};
    std::array<float,3> input{2.0f,-55.0f,-3.0f};
    Fixture() {
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d6a000,MEM_RESERVE,PAGE_NOACCESS));
        assert(base && VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        put(reinterpret_cast<void*>(base+crimson::marketplace::stamina::kClientRoot),0,root.data());
        put(root.data(),0x30,manager.data());
        put(manager.data(),0,base+crimson::marketplace::stamina::kClientManager);
        put(manager.data(),0x50,actor.data());
        put(actor.data(),0,base+crimson::marketplace::stamina::kClientActor);
        put(actor.data(),0x60,std::uint32_t{0xa0101234});
        put(actor.data(),0x68,components.data());
        put(actor.data(),0x70,std::uint32_t{24});
        put(actor.data(),0x88,metadata.data());metadata[1]=1;
        put(actor.data(),0x96,std::uint8_t{1});
        put(actor.data(),0xa0,possessor.data());put(possessor.data(),0xd0,actor.data());
        put(components.data(),0x40,control.data());
        put(control.data(),0,base+kControlVtable);put(control.data(),8,actor.data());
        put(control.data(),0xc8,state.data());put(control.data(),0x140,controller.data());
        put(state.data(),0x60,control.data());put(state.data(),0x68,actor.data());
        for (unsigned i=0;i<4;++i)put(state.data(),0xf8+i*8,method.data());
        put(method.data(),0,base+kNormalMethodVtable);put(method.data(),0x18,control.data());put(method.data(),0x20,actor.data());
    }
    ~Fixture() { assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE)); }
    Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
};
Fixture* fixture_ptr{};
float expected_x=3.0f,expected_z=-4.5f;
float expected_dt=0.016f;
bool modified=true,recurse=false;
unsigned calls=0;
std::uintptr_t return_value=0xfedcba9876543210;
std::uintptr_t __fastcall step(void* controller,float dt,const float* input,std::uintptr_t flag,void* output,std::uintptr_t a6,std::uintptr_t a7) {
    assert(controller==fixture_ptr->controller.data() && (dt==expected_dt || (std::isnan(dt)&&std::isnan(expected_dt))));
    assert(flag==0xfedcba98765432a5 && output==fixture_ptr->state.data() && a6==0xabcdef01234567a6 && a7==0x123456789abcdef7);
    assert((std::isnan(expected_x)&&std::isnan(input[0])) || input[0]==expected_x);
    assert(input[1]==fixture_ptr->input[1]);assert(input[2]==expected_z);
    assert(modified ? input!=fixture_ptr->input.data() : input==fixture_ptr->input.data());
    ++calls;
    if(recurse) {
        recurse=false;
        assert(dispatch(step,fixture_ptr->base,fixture_ptr->base+kGroundReturns[0],150,controller,dt,input,flag,output,a6,a7)==return_value);
    }
    put(output,0x400,std::uint32_t{0xdeadbeef});
    return return_value;
}
void invoke(Fixture& f,unsigned percent=150,Address return_offset=kGroundReturns[0],float dt=0.016f) {
    fixture_ptr=&f;expected_dt=dt;
    const auto original=f.input;
    assert(dispatch(step,f.base,f.base+return_offset,percent,f.controller.data(),dt,f.input.data(),0xfedcba98765432a5,f.state.data(),0xabcdef01234567a6,0x123456789abcdef7)==return_value);
    assert(!std::memcmp(f.input.data(),original.data(),sizeof(original)));
    assert(get<std::uint32_t>(f.state.data(),0x400)==0xdeadbeef);
}
std::uintptr_t __fastcall invalid(void* controller,float dt,const float* velocity,std::uintptr_t flag,void* output,std::uintptr_t a6,std::uintptr_t a7) {
    assert(controller==reinterpret_cast<void*>(1) && dt==0.1f && velocity==reinterpret_cast<float*>(1) && flag==2 && output==reinterpret_cast<void*>(3) && a6==4 && a7==5);++calls;
    return 0xabcdef9876543210;
}
struct ChangingMemory : Memory {
    Fixture& f; unsigned reads=0;
    explicit ChangingMemory(Fixture& value):f(value){}
    template<class T>T at(Address a) {
        T value=Memory::at<T>(a);
        if(a==reinterpret_cast<Address>(f.actor.data())+0x60 && ++reads==3)put(f.control.data(),0x140,Address{0});
        return value;
    }
};
}
int main() {
    const unsigned char config[]{'C','W','S','1',150,0,0,0};assert(setting(config,8)==150);
    assert(setting(nullptr,8)==100 && setting(config,7)==100 && setting(config,9)==100);
    const unsigned char wrong_magic[]{'C','W','J','1',150,0,0,0};assert(setting(wrong_magic,8)==100);
    for(auto percent:{0u,99u,100u,101u,150u,300u,301u,0xffffffffu}) {
        std::array<unsigned char,8> bytes{'C','W','S','1',0,0,0,0};put(bytes.data(),4,percent);
        assert(setting(bytes.data(),8)==(percent>=100 && percent<=300?percent:100));
    }
    for(auto kind:{1u,4u})for(auto mode:{0u,1u,2u,3u})for(auto return_offset:kGroundReturns) {
        Fixture f;f.metadata[1]=static_cast<unsigned char>(kind);put(f.state.data(),0xf0,static_cast<std::uint8_t>(mode));
        modified=true;expected_x=3;expected_z=-4.5f;invoke(f,150,return_offset);
    }
    {Fixture f;recurse=true;modified=true;expected_x=3;expected_z=-4.5f;invoke(f);}
    for(auto percent:{100u,99u,301u,0xffffffffu}) {Fixture f;modified=false;expected_x=2;expected_z=-3;invoke(f,percent);}
    {Fixture f;modified=true;expected_x=6;expected_z=-9;invoke(f,300);}
    for(unsigned invalid_case=0;invalid_case<26;++invalid_case) {
        Fixture f;
        switch(invalid_case) {
        case 0:put(f.manager.data(),0x50,Address{0});break;
        case 1:f.metadata[1]=6;break; // Mount actor.
        case 2:put(f.actor.data(),0,Address{0});break;
        case 3:put(f.actor.data(),0x60,std::uint32_t{0});break;
        case 4:put(f.actor.data(),0x96,std::uint8_t{0});break;
        case 5:put(f.actor.data(),0x70,std::uint32_t{8});break;
        case 6:put(f.actor.data(),0x70,std::uint32_t{58});break;
        case 7:put(f.possessor.data(),0xd0,Address{0});break;
        case 8:put(f.control.data(),8,Address{0});break;
        case 9:put(f.control.data(),0xc8,Address{0});break;
        case 10:put(f.state.data(),0x68,Address{0});break;
        case 11:put(f.actor.data(),0x88,Address{0});break;
        case 12:put(f.root.data(),0x30,Address{0});break;
        case 13:put(f.control.data(),0,Address{0});break;
        case 14:put(f.control.data(),0x140,Address{0});break;
        case 15:put(f.state.data(),0x60,Address{0});break;
        case 16:put(f.controller.data(),0x2ed,std::uint8_t{1});break; // Air mover.
        case 17:put(f.controller.data(),0x2ed,std::uint8_t{2});break;
        case 18:put(f.state.data(),0xf0,std::uint8_t{4});break;
        case 19:put(f.state.data(),0xf8,Address{0});break;
        case 20:put(f.method.data(),0,Address{0});break;
        case 21:put(f.method.data(),0x18,Address{0});break;
        case 22:put(f.method.data(),0x20,Address{0});break;
        case 23:put(f.manager.data(),0,Address{0});break;
        case 24:put(f.actor.data(),0xa0,Address{0});break;
        case 25:put(f.state.data(),0xf0,std::uint8_t{255});break;
        }
        modified=false;expected_x=2;expected_z=-3;invoke(f);
    }
    for(auto offset:{Address{0},Address{0x36a4eeb},Address{0x36a3d0b},Address{0x36a3d0d}}) {
        Fixture f;modified=false;expected_x=2;expected_z=-3;invoke(f,150,offset);
    }
    for(float dt:{0.0f,-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        Fixture f;modified=false;expected_x=2;expected_z=-3;invoke(f,150,kGroundReturns[0],dt);
    }
    for(float value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::max()}) {
        Fixture f;f.input[0]=value;modified=false;expected_x=value;expected_z=-3;invoke(f);
    }
    {Fixture f;ChangingMemory memory(f);float copy[3];assert(!velocity(memory,f.base,reinterpret_cast<Address>(f.controller.data()),f.base+kGroundReturns[0],150,0.016f,f.input.data(),copy));}
    assert(dispatch(invalid,1,1,150,reinterpret_cast<void*>(1),0.1f,reinterpret_cast<float*>(1),2,reinterpret_cast<void*>(3),4,5)==0xabcdef9876543210);
    for (auto result : {std::uintptr_t{0},std::uintptr_t{0xffffffffffffffff},std::uintptr_t{0x8000000000000001}}) {
        return_value=result;
        {Fixture f;modified=true;expected_x=3;expected_z=-4.5f;invoke(f);}
        {Fixture f;modified=false;expected_x=2;expected_z=-3;invoke(f,100);}
    }
    std::printf("PASS: player speed record, full native argument/result forwarding, copied horizontal drive, selected-owner/ground/method guards, reentrancy and invalid memory (%u calls).\n",calls);
}
