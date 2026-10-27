#include "../src/mount.h"
#include <cassert>
#include <map>
using namespace crimson::mac_items;
using namespace crimson::live_items;
int main() {
    Queue queue(10,20);
    queue.observe(1,7);
    Request request;
    request.operation=Operation::mount_grant;
    request.id[0]=1; request.key=30108; request.quantity=1; request.epoch=20;
    assert(queue.handle(request,2).state==State::queued);
    auto reused=request; reused.operation=Operation::grant;
    assert(queue.handle(reused,3).state==State::rejected);
    const auto selected=queue.take(4,7);
    assert(selected&&selected->operation==Operation::mount_grant);
    queue.finish(request,State::applied,Error::none);
    assert(queue.handle(request,5).state==State::applied);
    auto invalid=request; invalid.id[0]=2; invalid.quantity=2;
    assert(queue.handle(invalid,6).state==State::rejected);
    std::map<std::uintptr_t,std::uintptr_t> memory{
        {0x1085a7b90,0x1000},{0x1008,1},{0x1058,0x2000},{0x2000,0x3000},
        {0x3000,30108},{0x3068,4},{0x4008,0x5000},
        {0x4018,0x6000},{0x4020,0},
    };
    auto read = [&](std::uintptr_t address,std::uintptr_t& value) {
        auto p=memory.find(address);if(p==memory.end())return false;value=p->second;return true;
    };
    unsigned calls=0;
    auto grant = [&](std::uintptr_t manager,std::uint16_t type) {
        assert(manager==0x4000&&type==0);++calls;
        memory[0x4020]=1;memory[0x6000]=0x7000;
        memory[0x7020]=type;memory[0x7028]=101;
        return 0U;
    };
    auto run = [&](bool context=true) {return execute_mount(grant,[=]{return context;},0x5000,0x4000,30108,0,read);};
    assert(run(false).state==State::rejected&&calls==0);
    memory[0x4008]=0x9000;
    assert(run().state==State::rejected&&calls==0);
    memory[0x4008]=0x5000;
    assert(run().state==State::applied&&calls==1);
    assert(run().error==Error::mount_owned&&calls==1);
    memory[0x4020]=4097;
    assert(run().state==State::rejected&&calls==1);
    memory[0x4020]=0;
    auto unchanged=[](std::uintptr_t,std::uint16_t){return 0U;};
    assert(execute_mount(unchanged,[]{return true;},0x5000,0x4000,30108,0,read).state==State::uncertain);
    auto native_error=[](std::uintptr_t,std::uint16_t){return 7U;};
    assert(execute_mount(native_error,[]{return true;},0x5000,0x4000,30108,0,read).state==State::uncertain);
}
