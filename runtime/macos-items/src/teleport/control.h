#pragma once
#include "native.h"
#include "teleport_protocol.h"
#include <mutex>

namespace crimson::mac_items {
class TeleportControl {
    std::mutex mutex_;
    teleport::Reply status_;
    teleport::Request last_;
    std::optional<teleport::Request> pending_;
    teleport::SavedPosition destination_;
    std::uint64_t observed_{},queued_{};
    std::uint32_t field_definition_{};std::uint16_t stage_{0xffff};
    bool awaiting_{},faulted_{};
    void result(teleport::State state,teleport::Reason reason) {status_.state=state;status_.reason=reason;}
    void expire(std::uint64_t now) {
        if(pending_&&now>pending_->not_after_ms){pending_.reset();result(teleport::State::rejected,teleport::Reason::timeout);}
        if(awaiting_&&(now<queued_||now-queued_>60000)){awaiting_=false;result(teleport::State::uncertain,teleport::Reason::timeout);}
    }
public:
    explicit TeleportControl(std::uint64_t session){status_.session=session;}
    void fault(){std::lock_guard lock(mutex_);faulted_=true;pending_.reset();awaiting_=false;status_.features=0;result(teleport::State::faulted,teleport::Reason::native_fault);}
    teleport::Reply handle(const teleport::Request& request,std::uint64_t now,std::uint32_t selected) {
        using namespace teleport;std::lock_guard lock(mutex_);expire(now);
        auto rejected=[&](Reason reason){auto reply=status_;reply.state=State::rejected;reply.reason=reason;return reply;};
        if(!valid_request(request))return rejected(Reason::malformed);
        if(request.operation==Operation::status){auto reply=status_;if(!observed_||now<observed_||now-observed_>1000||selected!=reply.identity){reply.features=0;reply.current={};}return reply;}
        if(request.session!=status_.session)return rejected(Reason::session);
        if(request.command==status_.command)return std::memcmp(&request,&last_,sizeof(request))?rejected(Reason::command_order):status_;
        if(request.command<status_.command)return rejected(Reason::command_order);
        if(faulted_)return rejected(Reason::native_fault);
        if(pending_||awaiting_)return rejected(Reason::busy);
        if(!observed_||now<observed_||now-observed_>1000||request.epoch!=status_.epoch
            ||selected!=status_.identity||!status_.features||now>request.not_after_ms||request.not_after_ms-now>3000)return rejected(Reason::stale_context);
        if(request.operation==Operation::travel_marker)return rejected(Reason::marker_height_or_world_unproven);
        if(request.operation!=Operation::save_current&&!status_.bookmarks[request.slot].occupied)return rejected(Reason::slot);
        status_.command=request.command;last_=request;
        if(request.operation==Operation::remove_saved){status_.bookmarks[request.slot]={};++status_.bookmarks_revision;result(State::removed,Reason::none);}
        else {pending_=request;result(State::pending,Reason::none);}
        return status_;
    }
    template<class Move>
    void observe(const TeleportFrame& frame,TeleportNative code,std::uint32_t selected,std::uint64_t now,Move move) {
        using namespace teleport;std::lock_guard lock(mutex_);if(faulted_)return;expire(now);
        status_.native_reason=static_cast<unsigned>(code);status_.native_error=frame.error;
        if(code!=TeleportNative::ready||!frame.ready||!selected){status_.features=0;observed_=0;status_.current={};return;}
        const bool changed=selected!=status_.identity||frame.context.world!=status_.world
            ||frame.current.field_definition!=field_definition_||frame.current.stage!=stage_;
        // A transient loading observation clears current, but must not invalidate an accepted same-field move.
        const bool identity_changed=status_.identity&&(selected!=status_.identity||frame.context.world!=status_.world);
        if(changed){++status_.epoch;if(!status_.epoch)++status_.epoch;if(pending_){pending_.reset();result(State::rejected,Reason::context_changed);}}
        if(awaiting_&&identity_changed){awaiting_=false;result(State::uncertain,Reason::context_changed);}
        status_.identity=selected;status_.world=frame.context.world;status_.current=frame.current;
        field_definition_=frame.current.field_definition;stage_=frame.current.stage;
        observed_=now;status_.observed_ms=now;status_.features=Feature::save_current|Feature::travel_saved;
        if(status_.state==State::waiting)result(State::ready,Reason::none);
        if(awaiting_&&frame.current.stage==destination_.stage&&frame.current.field_definition==destination_.field_definition){
            const auto& a=frame.current.position;const auto& b=destination_.position;
            const double x=double(a.x)-b.x,y=double(a.y)-b.y,z=double(a.z)-b.z;
            if(x*x+y*y+z*z<=.25){awaiting_=false;result(State::position_observed,Reason::none);}
        }
        if(!pending_)return;
        const auto request=*pending_;pending_.reset();
        if(request.operation==Operation::save_current){status_.bookmarks[request.slot]={frame.current,request.name,1,0};++status_.bookmarks_revision;result(State::captured,Reason::none);return;}
        bool attempted{};std::int32_t error{};destination_=status_.bookmarks[request.slot].position;
        const auto moved=move(destination_,error,attempted,request.not_after_ms);status_.native_error=error;status_.native_reason=static_cast<unsigned>(moved);
        if(moved==TeleportNative::accepted){awaiting_=true;queued_=now;result(State::native_queued,Reason::none);}
        else result(attempted?State::uncertain:State::rejected,moved==TeleportNative::publish_mismatch?Reason::publish_mismatch:Reason::native_error);
    }
};
}
