#include "../src/world_read.h"
#include <cassert>
#include <limits>
using namespace crimson::mac_items;
namespace {
unsigned releases{},reads{};
MacFieldLease lease{0x7000,0x4000,1,0,{},0,0,0x3000};
MacFieldTime time_value{50,18,19,18,108,1,0,3212,79339};
MacPosition position_value{12,34,56};
MacFieldLease acquire(void* p){assert(p==reinterpret_cast<void*>(0x3000));return lease;}
void release(MacFieldLease* p){assert(p->field==lease.field);++releases;}
MacFieldTime time_read(void* p){assert(p==reinterpret_cast<void*>(0x4000));++reads;return time_value;}
MacPosition position_read(void* p){assert(p==reinterpret_cast<void*>(0x1000));return position_value;}
std::uint64_t now(){return 79400;}
}
int main(){
    using namespace crimson::marketplace::world_controls;
    // Captured raw clock and native getter on the pinned Mac build, same native anchor.
    const MacFieldTime captured{50,18,11,56,232,1,140,12,53830};
    const auto calculated=at_native_time(captured,98767);
    assert(calculated&&calculated->day==50&&calculated->hour==18&&calculated->minute==20
        &&calculated->second==55&&calculated->millisecond==476&&calculated->rate==12&&calculated->anchor==98767);
    const auto frozen=freeze_time(*calculated,98767);
    assert(frozen&&frozen->rate==0);
    const auto resumed=resume_time(*frozen,12,200000);
    assert(resumed&&resumed->hour==18&&resumed->minute==20&&resumed->second==55&&resumed->rate==12);
    assert(!choose_time(captured,{24,0,0,0},98767));
    WorldProbe context;context.context=true;context.transform=0x3000;context.field=0x4000;
    WorldReadBindings bindings{acquire,release,time_read,position_read,now,0x7000};
    auto observed=read_native_world(context,0x1000,bindings);
    assert(observed.ready&&observed.time.rate==3212&&observed.position.z==56&&observed.native_now==79400);
    assert(releases==1&&reads==1);
    lease.owner=0;
    assert(!read_native_world(context,0x1000,bindings).ready);
    assert(releases==2&&reads==1);
    lease.owner=0x3000;lease.valid=0;
    assert(!read_native_world(context,0x1000,bindings).ready);
    assert(releases==3&&reads==1);
    lease.valid=1;time_value.hour=24;
    assert(!read_native_world(context,0x1000,bindings).ready);
    time_value.hour=18;position_value.x=std::numeric_limits<float>::infinity();
    assert(!read_native_world(context,0x1000,bindings).ready);
    context.transition=1;auto before=releases;
    assert(!read_native_world(context,0x1000,bindings).ready&&releases==before);
}
