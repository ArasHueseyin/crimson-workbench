#include "../src/teleport/control.h"
#include <cassert>
#include <map>
using namespace crimson::mac_items;
static std::map<std::uintptr_t,std::uintptr_t> memory;
static unsigned acquired{},released{},locked{},unlocked{},begins{};
static std::int32_t native_error{};
static MacFieldLease acquire(void*){++acquired;return {0x77,0x4000,1,0,{},0,0,0x3000};}
static void release(MacFieldLease*){++released;}
static MacActorGuard lock_actor(void* actor){++locked;return {0x888,1,{},reinterpret_cast<std::uintptr_t>(actor)};}
static void unlock_actor(MacActorGuard*){++unlocked;}
static MacPosition position(void*){return {10,20,30};}
static float yaw(void*){return .5f;}
static void eligibility(void*,std::int32_t* error,std::uint8_t* blocked){*error=native_error;*blocked=0;}
static std::uint32_t begin(void*,const teleport::NativeFieldMoveValue* value){
    ++begins;std::array<std::uintptr_t,7> words{};std::memcpy(words.data(),value,sizeof(*value));
    for(unsigned i=0;i<7;++i)memory[0x3550+i*8]=words[i];memory[0x3548]=1;return 0;
}
int main(){
    memory={{0x1000,0x107e2fe10},{0x1108,0x2000},{0x21a0,0x3000},{0x3000,0x107e33228},{0x3008,0x1000},
        {0x3500,0x4000},{0x4000,0x107e30b10},{0x1140,0x5000},{0x5100,7},{0x4220,7},
        {0x3028,1},{0x44b8,0x6000},{0x6004,1},{0x3548,0},{0x421c,0},
        {0x2020,0x7000},{0x7008,0x1000},{0x7030,12},{0x354a,1},{0x3550,0},
        {0x107d859e0,0x8000},{0x8000,0x9000},{0x9008,2},{0x9058,0xa000},{0xa008,0xb000},{0xb000,99}};
    auto read=[](std::uintptr_t a,std::uintptr_t& value){auto it=memory.find(a);if(it==memory.end())return false;value=it->second;return true;};
    TeleportBindings b{{acquire,release,nullptr,position,nullptr,0x77},lock_actor,unlock_actor,yaw,eligibility,begin,0};
    TeleportFrame frame;assert(capture_teleport(0x1000,b,read,frame)==TeleportNative::ready);
    assert(frame.current.field_definition==99&&frame.current.position.x==10&&frame.character==12);
    assert(acquired==released&&locked==unlocked);
    memory[0x7008]=0x9999;TeleportFrame invalid;
    assert(capture_teleport(0x1000,b,read,invalid)==TeleportNative::actor&&begins==0);
    memory[0x7008]=0x1000;
    assert(acquired==released&&locked==unlocked);
    std::int32_t error{};bool attempted{};auto destination=frame.current;destination.stage=2;
    assert(begin_teleport(frame,destination,b,read,error,attempted,[]{return true;})==TeleportNative::stage&&!attempted&&begins==0);
    destination=frame.current;native_error=4;
    assert(begin_teleport(frame,destination,b,read,error,attempted,[]{return true;})==TeleportNative::native_rejection&&!attempted&&begins==0);
    native_error=0;
    assert(begin_teleport(frame,destination,b,read,error,attempted,[]{return false;})==TeleportNative::task&&!attempted&&begins==0);
    assert(begin_teleport(frame,destination,b,read,error,attempted,[]{return true;})==TeleportNative::accepted&&attempted&&begins==1);
    assert(acquired==released&&locked==unlocked);
    using namespace teleport;
    TeleportControl control(42);unsigned moves{};
    auto move=[&](const SavedPosition&,std::int32_t&,bool& tried,std::uint64_t){++moves;tried=true;return TeleportNative::accepted;};
    control.observe(frame,TeleportNative::ready,9,100,move);
    auto status=control.handle({},100,9);assert(status.features==3&&status.epoch==1);
    Request request;request.session=42;request.command=1;request.epoch=status.epoch;request.not_after_ms=1000;request.operation=Operation::save_current;
    std::memcpy(request.name.data(),"Camp",5);
    assert(control.handle(request,101,9).state==State::pending);
    control.observe(frame,TeleportNative::ready,9,102,move);status=control.handle({},102,9);
    assert(status.state==State::captured&&status.bookmarks[0].occupied&&moves==0);
    request.command=2;request.name={};request.operation=Operation::travel_saved;
    assert(control.handle(request,103,8).reason==Reason::stale_context);
    assert(control.handle(request,103,9).state==State::pending);
    control.observe(frame,TeleportNative::ready,9,104,move);
    assert(control.handle(request,105,9).state==State::native_queued&&moves==1);
    control.observe({},TeleportNative::field,9,106,move);
    assert(control.handle({},106,9).features==0);
    control.observe(frame,TeleportNative::ready,9,107,move);
    assert(control.handle({},107,9).state==State::position_observed&&moves==1);
    assert(control.handle({},107,9).epoch==1);
    assert(control.handle(request,108,9).state==State::position_observed&&moves==1);
    assert(control.handle({},1200,9).features==0);
    request.command=3;request.not_after_ms=5000;
    assert(control.handle(request,1200,9).reason==Reason::stale_context);
    control.fault();assert(control.handle({},1201,9).state==State::faulted);
}
