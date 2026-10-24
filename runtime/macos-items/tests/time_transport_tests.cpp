#include "../src/world/time_transport.h"
#include <cassert>
using namespace crimson::mac_items;
int main(){
    TimeControl control(7);unsigned writes{};
    TimeContext frame{42,8,0,{50,12,0,0,0,1,0,12,100},{50,12,0,0,0,1,0,12,100},100};
    control.observe(frame,now_ms(),[&](const TimeContext&,const world::FieldTime&){++writes;return TimePublication::published;});
    int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
    std::thread server([&]{serve_time_peer(pair[1],control,[]{return 42U;});close(pair[1]);});
    world::Request request;request.operation=world::ControlOperation::freeze_time;request.enabled=1;
    request.id=1;request.session=7;request.generation=1;request.not_after_ms=now_ms()+5000;
    assert(write(pair[0],&request,11)==11);
    assert(write(pair[0],reinterpret_cast<char*>(&request)+11,sizeof(request)-11)==sizeof(request)-11);
    world::Reply reply;
    assert(receive_all(pair[0],&reply,sizeof(reply)));
    assert(reply.bytes==176&&reply.code==world::ControlCode::queued&&reply.time_receipt==1&&writes==0);
    close(pair[0]);server.join();
    control.observe(frame,now_ms(),[&](const TimeContext&,const world::FieldTime&){++writes;return TimePublication::published;});
    assert(writes==1);
}
