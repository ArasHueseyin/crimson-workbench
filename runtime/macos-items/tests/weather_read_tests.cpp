#include "../src/weather_read.h"
#include <cassert>
#include <map>
using namespace crimson::mac_items;
int main(){
    std::map<std::uintptr_t,std::uintptr_t> memory{
        {0x1031,0},{0x1060,0x2028},{0x2000,0x1081ca110},{0x2040,0x3028},{0x2048,0x4028},
        {0x3000,0x1081ca690},{0x3010,0x2000},{0x4000,0x1081c9e40},{0x4010,0x2000},
    };
    for(auto offset:{0x154,0x158,0x15c,0x160,0x1c0})memory[0x3000+offset]=std::bit_cast<std::uint32_t>(0.5f);
    for(auto offset:{0xc4,0xd0,0xb0})memory[0x4000+offset]=std::bit_cast<std::uint32_t>(0.25f);
    auto read=[&](std::uintptr_t a,std::uintptr_t& value){auto i=memory.find(a);if(i==memory.end())return false;value=i->second;return true;};
    WorldProbe context;context.weather_read=true;context.weather=0x1000;
    auto out=read_weather(context,0,read);
    assert(out.ready&&!out.manual&&out.precipitation==0.5f&&out.fog==0.25f);
    memory[0x3010]=0;
    assert(!read_weather(context,0,read).ready);
    memory[0x3010]=0x2000;memory[0x1031]=2;
    assert(!read_weather(context,0,read).ready);
    memory[0x1031]=1;memory[0x3154]=0x7f800000;
    assert(!read_weather(context,0,read).ready);
    memory[0x3154]=0;memory[0x4000]=0;
    assert(!read_weather(context,0,read).ready);
}
