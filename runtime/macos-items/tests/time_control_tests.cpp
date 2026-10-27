#include "../src/world/world_control.h"
#include <cassert>
using namespace crimson::mac_items;
using namespace crimson::marketplace::world_controls;
int main(){
    WorldControl control(123);
    TimeContext frame{7,8,0,{50,18,0,0,0,1,0,12,1000},{50,18,0,0,0,1,0,12,1000},1000};
    unsigned writes{};
    auto publish=[&](const TimeContext&,const FieldTime& value){++writes;frame.raw=value;frame.time=value;return TimePublication::published;};
    control.observe(frame,100,publish);
    auto status=control.handle({},100,7);
    assert((status.flags&time_available)&&status.generation==1);
    assert(!(control.handle({},100,9).flags&time_available));
    Request request;request.operation=ControlOperation::freeze_time;request.enabled=1;
    request.id=1;request.session=123;request.generation=status.generation;request.not_after_ms=1000;
    assert(control.handle(request,100,7).code==ControlCode::queued);
    assert(writes==0);
    control.observe(frame,101,publish);
    status=control.handle({},101,7);
    assert(writes==1&&(status.flags&time_frozen)&&frame.raw.rate==0);
    assert(control.handle(request,102,7).reason==ControlReason::replay);
    control.observe(frame,102,publish);assert(writes==1);
    request.id=2;request.operation=ControlOperation::release_time;request.enabled=0;
    assert(control.handle(request,102,7).code==ControlCode::queued);
    frame.native_now=5000;control.observe(frame,103,publish);
    status=control.handle({},103,7);
    assert(writes==2&&frame.raw.rate==12&&frame.raw.anchor==5000&&!(status.flags&time_frozen));
    request.id=3;request.operation=ControlOperation::choose_time;request.time={25,0,0,0};
    assert(control.handle(request,104,7).reason==ControlReason::invalid_value);
    request.time={12,0,0,0};request.not_after_ms=104;
    assert(control.handle(request,104,7).code==ControlCode::queued);
    control.observe(frame,105,publish);assert(writes==2);
    assert(control.handle({},105,7).time_result==ControlCode::expired);
    request.id=4;request.not_after_ms=1000;
    assert(control.handle(request,106,7).code==ControlCode::queued);
    ++frame.world;control.observe(frame,107,publish);
    assert(writes==2&&control.handle({},107,7).time_result==ControlCode::world_changed);
    request.id=5;request.generation=2;
    assert(control.handle(request,108,7).code==ControlCode::queued);
    control.observe(frame,109,[&](const TimeContext&,const FieldTime&){++writes;return TimePublication::uncertain;});
    assert(writes==3&&(control.handle({},109,7).flags&time_uncertain));
    control.observe(frame,110,publish);assert(writes==3);
    request.id=6;
    assert(control.handle(request,110,7).code==ControlCode::rejected);

    WorldControl foreign(456);foreign.observe(frame,200,publish);
    request={};request.id=1;request.session=456;request.generation=1;request.not_after_ms=1000;
    request.operation=ControlOperation::freeze_time;request.enabled=1;
    assert(foreign.handle(request,200,7).code==ControlCode::queued);
    foreign.observe(frame,201,publish);assert(writes==4);
    frame.raw.anchor+=1;request.id=2;request.operation=ControlOperation::release_time;
    assert(foreign.handle(request,202,7).code==ControlCode::queued);
    foreign.observe(frame,203,publish);
    assert(writes==4&&foreign.handle({},203,7).time_result_reason==ControlReason::foreign_change);
    WorldControl faulted(789);faulted.observe(frame,300,publish);
    faulted.fault();assert(faulted.handle({},300,7).code==ControlCode::fault_latched);
    assert(!faulted.needs_observation(1000));
}
