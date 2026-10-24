#include "../src/world_probe.h"
#include <cassert>
#include <map>
using namespace crimson::mac_items;
int main() {
    std::map<std::uintptr_t,std::uintptr_t> memory{
        {0x1000,0x107e2fe10},{0x1108,0x2000},{0x21a0,0x3000},{0x3000,0x107e33228},{0x3008,0x1000},
        {0x3500,0x4000},{0x4000,0x107e30b10},{0x1140,0x5000},{0x5100,7},{0x4220,7},
        {0x3028,1},{0x44b8,0x6000},{0x6004,1},{0x3548,0},{0x421c,0},
        {0x108a6ab00,1},{0x108a6ab08,0},{0x108a6ab10,0},{0x108a6ab18,100},
        {0x108ce55f0,0x7000},{0x7040,0x8000},{0x8008,0x9000},{0x9f68,0xa000},
        {0xa000,0x1081c3b98},{0xa0d0,0x9000},
    };
    auto read=[&](std::uintptr_t a,std::uintptr_t& value){auto i=memory.find(a);if(i==memory.end())return false;value=i->second;return true;};
    auto value=inspect_world_probe(0x1000,0,read);
    assert(value.context&&value.clock_read&&value.weather_read&&value.world==7&&value.stage==1);
    assert(value.weather_parent_offset==0x40&&value.weather_child_offset==8);
    memory[0x7038]=0x8000;memory[0x8030]=0x9000;memory[0x7040]=0;
    assert(!inspect_world_probe(0x1000,0,read).weather_read);
    memory[0x7040]=0x8000;
    memory[0xa0d0]=0;
    assert(!inspect_world_probe(0x1000,0,read).weather_read);
    memory[0x4220]=8;
    assert(!inspect_world_probe(0x1000,0,read).context);
    memory[0x4220]=7;memory[0x3008]=0x9999;
    assert(!inspect_world_probe(0x1000,0,read).context);
}
