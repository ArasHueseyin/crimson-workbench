#include <Windows.h>
#include <cstring>
#include "../teleport/teleport_pipe.h"
namespace tp=crimson::marketplace::teleport;
namespace {
// A transport/codec fixture with owned values only. No engine bindings,
// process lookup, real bookmark storage or gameplay calls are present.
constexpr std::uint64_t session=0xfedcba9876543210ULL,epoch=0x20000000000001ULL;
tp::Reply current{};
tp::Reply handle(const tp::Request& request,std::uint64_t now){
    current.session=session;current.epoch=epoch;current.observed_ms=current.markers_observed_ms=now;
    auto rejected=[&](tp::Reason reason){auto reply=current;reply.command=request.command;reply.state=tp::State::rejected;reply.reason=reason;return reply;};
    if(!tp::valid_request(request))return rejected(tp::Reason::malformed);
    if(request.operation==tp::Operation::status)return current;
    if(request.session!=session)return rejected(tp::Reason::session);
    if(request.epoch!=epoch)return rejected(tp::Reason::stale_context);
    current.command=request.command;current.reason=tp::Reason::none;
    if(request.operation==tp::Operation::save_current){
        current.bookmarks[request.slot]={current.current,request.name,1,0};++current.bookmarks_revision;current.state=tp::State::captured;
    }else if(request.operation==tp::Operation::remove_saved){
        current.bookmarks[request.slot]={};++current.bookmarks_revision;current.state=tp::State::removed;
    }else if(request.operation==tp::Operation::travel_saved){
        if(!current.bookmarks[request.slot].occupied)return rejected(tp::Reason::slot);
        current.current=current.bookmarks[request.slot].position;current.state=tp::State::position_observed;
    }else if(request.operation==tp::Operation::travel_marker){
        const tp::MarkerValue* selected{};
        for(std::uint32_t i=0;i<current.marker_count;++i)if(current.markers[i].id==request.marker&&static_cast<std::uint32_t>(current.markers[i].source)==request.marker_source)selected=&current.markers[i];
        if(!selected)return rejected(tp::Reason::marker_missing);
        current.current.position=selected->position;current.current.position.y=42.0f;current.state=tp::State::position_observed;
    }
    return current;
}
}
int main(){
    current.state=tp::State::ready;current.reason=tp::Reason::none;current.features=7;current.identity=123;current.world=456;current.storage_status=1;
    current.current={1,17,0,{10,20,30},0.5f,1,0};
    current.bookmarks[0].position=current.current;current.bookmarks[0].occupied=1;
    constexpr char name[]="Camp caf\xc3\xa9";std::memcpy(current.bookmarks[0].name.data(),name,sizeof(name));
    current.bookmarks_revision=0x20000000000003ULL;current.marker_count=2;current.marker_status=tp::MarkerCode::ready;
    current.markers[0]={0,{100,0,200},tp::MarkerSource::map_pin,tp::MarkerLayer::mainland,1,2};
    current.markers[1]={0,{300,0,400},tp::MarkerSource::waypoint,tp::MarkerLayer::mainland,3,4};
    tp::serve_teleport_pipe(handle);return 1;
}
