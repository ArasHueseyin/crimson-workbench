#include "../src/world/time_native.h"
#include <cassert>
#include <map>
using namespace crimson::mac_items;
namespace {
std::map<std::uintptr_t,std::uintptr_t> memory;
unsigned acquired{},released{},published{},sent{};
bool mismatch{};
void store(std::uintptr_t at,const MacFieldTime& value){
    std::uintptr_t words[4];std::memcpy(words,&value,sizeof(value));
    for(unsigned i=0;i<4;++i)memory[at+i*8]=words[i];
}
bool read(std::uintptr_t at,std::uintptr_t& value){auto i=memory.find(at);if(i==memory.end())return false;value=i->second;return true;}
MacFieldLease acquire(void* p){assert(p==reinterpret_cast<void*>(0x3000));++acquired;return {0x107e74b68,0x4000,1,0,{},0,0,0x3000};}
void release(MacFieldLease*){++released;}
void publish(void* p,const MacFieldTime* value){
    assert(p==reinterpret_cast<void*>(0x3000));++published;store(0x3450,*value);
    if(!mismatch)store(0x108a6ab00,*value);
}
void send(void* actor,const MacNearTarget* target,const MacFieldTime*){
    assert(actor==reinterpret_cast<void*>(0x1000)&&target->actor==0x1000&&target->enabled==1);
    assert(acquired==released+1);++sent;
}
}
int main(){
    memory={{0x1000,0x107e2fe10},{0x1108,0x2000},{0x21a0,0x3000},{0x3000,0x107e33228},{0x3008,0x1000},
        {0x3500,0x4000},{0x4000,0x107e30b10},{0x1140,0x5000},{0x5100,7},{0x4220,7},
        {0x3028,1},{0x44b8,0x6000},{0x6004,1},{0x3548,0},{0x421c,0},
        {0x107d859e0,0x7000},{0x7000,0x8000},{0x8008,2},{0x8058,0x9000},{0x9008,0xa000},{0xa06a,0}};
    MacFieldTime raw{50,12,0,0,0,1,0,12,1000};store(0x108a6ab00,raw);
    const auto context=inspect_world_probe(0x1000,0,read);
    assert(context.context&&time_stage_fixed(1,0,read)==false);
    const auto value=*world::freeze_time(raw,1000);
    TimeWriteBindings bindings{{acquire,release,nullptr,nullptr,nullptr,0x107e74b68},publish,send,0x107e4f878,0};
    assert(publish_native_time(context,0x1000,raw,value,bindings,read)==TimePublication::published);
    assert(acquired==1&&released==1&&published==1&&sent==1);
    assert(publish_native_time(context,0x1000,raw,value,bindings,read)==TimePublication::rejected);
    assert(acquired==2&&released==2&&published==1&&sent==1);
    store(0x108a6ab00,raw);mismatch=true;
    assert(publish_native_time(context,0x1000,raw,value,bindings,read)==TimePublication::uncertain);
    assert(acquired==3&&released==3&&published==2&&sent==1);
    memory[0xa06a]=1;
    assert(publish_native_time(context,0x1000,raw,value,bindings,read)==TimePublication::rejected);
    assert(acquired==3&&published==2);
    memory[0xa06a]=2;assert(!time_stage_fixed(1,0,read));
    memory.erase(0xa06a);assert(!time_stage_fixed(1,0,read));
    assert(!read_time_value(std::numeric_limits<std::uintptr_t>::max(),read));
}
