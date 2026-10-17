#include <Windows.h>
#include "../world-controls/control_pipe.h"

namespace wc=crimson::marketplace::world_controls;
namespace {
// This executable owns the pipe and all values. No game memory or native game
// functions are used. Deliberately exceed JavaScript's exact integer range.
constexpr std::uint64_t session=0xfedcba9876543210ULL,generation=0x20000000000001ULL;
wc::Reply handle(const wc::Request& request,std::uint64_t now) {
    wc::Reply reply{};
    reply.session=session;reply.generation=generation;reply.monotonic_ms=now;
    reply.flags=wc::time_available|wc::weather_available;
    reply.identity=123;reply.world=456;reply.stage=7;
    reply.time={987,18,45,23,456,0,0,12,0x123456789abcdef0ULL};
    reply.weather={0.25f,-0.5f,0.75f,45.0f,2.0f,wc::PrecipitationPhase::snow};
    reply.code=wc::ControlCode::ready;
    if(request.magic!=wc::kControlMagic||request.version!=wc::kControlVersion||request.bytes!=sizeof(request)||request.reserved||
        static_cast<std::uint32_t>(request.operation)>7) {
        reply.code=wc::ControlCode::rejected;reply.reason=wc::ControlReason::protocol;
    } else if(request.operation!=wc::ControlOperation::status) {
        reply.id=request.id;
        if(request.session!=session||request.generation!=generation||!request.id) {
            reply.code=wc::ControlCode::rejected;reply.reason=wc::ControlReason::session;
        } else {
            reply.code=wc::ControlCode::published;
            if(request.operation==wc::ControlOperation::choose_weather) {
                reply.weather=request.weather;reply.weather_receipt=request.id;reply.weather_result=wc::ControlCode::published;
            } else if(request.operation==wc::ControlOperation::advance_time) {
                const auto hour=reply.time.hour+request.time.hour;
                reply.time.day+=hour/24;reply.time.hour=hour%24;
                reply.time.weekday=static_cast<std::uint8_t>((reply.time.weekday+hour/24)%7);
                reply.time_receipt=request.id;reply.time_result=wc::ControlCode::published;
            } else {
                reply.time.hour=request.time.hour;reply.time.minute=request.time.minute;
                reply.time.second=request.time.second;reply.time.millisecond=request.time.millisecond;
                reply.time_receipt=request.id;reply.time_result=wc::ControlCode::published;
            }
        }
    }
    return reply;
}
}
int main(){wc::serve_control_pipe(handle);return 1;}
