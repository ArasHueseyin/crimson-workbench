#pragma once
#include "control_protocol.h"
#include <mutex>
#include <optional>

namespace crimson::mac_items {
namespace world = crimson::marketplace::world_controls;
struct TimeContext {
    std::uint32_t identity{},world{},stage{};
    world::FieldTime raw{},time{};
    std::uint64_t native_now{};
};
enum class TimePublication { published, rejected, uncertain };

// Values only. The publisher runs once on the verified native callback, never on the socket thread.
class TimeControl {
    std::mutex mutex_;
    world::Reply status_{};
    std::optional<world::Request> pending_;
    world::FieldTime owned_{};
    std::uint64_t high_id_{},observed_ms_{},attempt_ms_{};
    std::uint16_t saved_rate_{};
    bool have_context_{},owns_freeze_{},latched_{};
    void finish(world::ControlCode code,world::ControlReason reason,std::uint64_t id=0) {
        status_.code=code;status_.reason=reason;status_.id=id;
        if(id){status_.time_receipt=id;status_.time_result=code;status_.time_result_reason=reason;}
    }
public:
    explicit TimeControl(std::uint64_t session){status_.session=session;}
    void fault() {
        std::lock_guard lock(mutex_);latched_=true;
        status_.flags=(status_.flags&~world::time_available)|world::time_uncertain;
        finish(world::ControlCode::fault_latched,world::ControlReason::native_fault,pending_?pending_->id:0);
        pending_.reset();
    }
    bool needs_observation(std::uint64_t now) {
        std::unique_lock lock(mutex_,std::try_to_lock);
        if(!lock.owns_lock()||latched_||(!pending_&&now>=attempt_ms_&&now-attempt_ms_<100))return false;
        attempt_ms_=now;return true;
    }
    world::Reply handle(const world::Request& request,std::uint64_t now,std::uint32_t selected) {
        using namespace world;
        std::lock_guard lock(mutex_);
        if(pending_&&now>pending_->not_after_ms){finish(ControlCode::expired,ControlReason::deadline,pending_->id);pending_.reset();}
        auto reply=status_;reply.monotonic_ms=now;
        const bool fresh=have_context_&&selected&&selected==status_.identity&&now>=observed_ms_&&now-observed_ms_<=10000;
        if(!fresh)reply.flags&=~time_available;
        ControlReason reason=ControlReason::none;
        if(request.magic!=kControlMagic||request.version!=kControlVersion||request.bytes!=sizeof(request)||request.reserved
            ||static_cast<unsigned>(request.operation)>7)reason=ControlReason::protocol;
        else if(request.operation==ControlOperation::status)return reply;
        else if(is_weather_operation(request.operation))reason=ControlReason::unsupported;
        else if(latched_||!fresh||!(reply.flags&time_available))reason=ControlReason::context;
        else if(!request.session||request.session!=status_.session)reason=ControlReason::session;
        else if(request.generation!=status_.generation)reason=ControlReason::generation;
        else if(!request.id||request.id<=high_id_)reason=ControlReason::replay;
        else if(request.not_after_ms<now||request.not_after_ms-now>10000)reason=ControlReason::deadline;
        else if(pending_)reason=ControlReason::busy;
        else if(request.enabled>1)reason=ControlReason::invalid_value;
        else if(request.operation==ControlOperation::choose_time&&(!encode_clock({0,request.time.hour,request.time.minute,request.time.second,request.time.millisecond,0})))reason=ControlReason::invalid_value;
        else if(request.operation==ControlOperation::advance_time&&!valid_advance_payload(request))reason=ControlReason::invalid_value;
        if(reason!=ControlReason::none){reply.code=ControlCode::rejected;reply.reason=reason;reply.id=request.id;return reply;}
        high_id_=request.id;pending_=request;finish(ControlCode::queued,ControlReason::none,request.id);
        reply=status_;reply.monotonic_ms=now;return reply;
    }
    template<class Publish>
    void observe(const std::optional<TimeContext>& context,std::uint64_t now,Publish publish) {
        using namespace world;
        std::unique_lock lock(mutex_,std::try_to_lock);
        if(!lock.owns_lock()||latched_)return;
        status_.monotonic_ms=now;
        if(!context){status_.flags&=~time_available;return;}
        const auto& frame=*context;
        if(!frame.identity||!encode_clock(fields(frame.raw))||!encode_clock(fields(frame.time))){status_.flags&=~time_available;return;}
        const bool changed=have_context_&&(status_.identity!=frame.identity||status_.world!=frame.world||status_.stage!=frame.stage
            ||now<observed_ms_||now-observed_ms_>10000);
        if(!have_context_||changed){++status_.generation;if(!status_.generation)++status_.generation;}
        if(changed&&pending_){finish(ControlCode::world_changed,ControlReason::generation,pending_->id);pending_.reset();}
        if(owns_freeze_&&frame.raw!=owned_){
            owns_freeze_=false;status_.flags&=~time_frozen;
            finish(ControlCode::released,ControlReason::foreign_change,pending_?pending_->id:0);pending_.reset();
        }
        have_context_=true;observed_ms_=now;status_.identity=frame.identity;status_.world=frame.world;status_.stage=frame.stage;
        status_.time=frame.time;status_.flags|=time_available;
        if(status_.code==ControlCode::unavailable)finish(ControlCode::ready,ControlReason::none);
        if(!pending_)return;
        const auto request=*pending_;pending_.reset();
        if(now>request.not_after_ms){finish(ControlCode::expired,ControlReason::deadline,request.id);return;}
        std::optional<FieldTime> value;
        const bool freeze=request.operation==ControlOperation::freeze_time&&request.enabled;
        const bool release=request.operation==ControlOperation::release_time||(request.operation==ControlOperation::freeze_time&&!request.enabled);
        if(request.operation==ControlOperation::choose_time)value=choose_time(frame.time,request.time,frame.native_now);
        else if(request.operation==ControlOperation::advance_time)value=advance_time(frame.time,request.time.hour,frame.native_now);
        else if(freeze){
            if(owns_freeze_){finish(ControlCode::published,ControlReason::none,request.id);return;}
            if(!frame.time.rate){finish(ControlCode::rejected,ControlReason::native_stopped,request.id);return;}
            value=freeze_time(frame.time,frame.native_now);
        } else if(release){
            if(!owns_freeze_){finish(ControlCode::released,ControlReason::not_frozen,request.id);return;}
            value=resume_time(owned_,saved_rate_,frame.native_now);
        }
        if(!value){finish(ControlCode::rejected,ControlReason::invalid_value,request.id);return;}
        TimePublication result=TimePublication::uncertain;
        try{result=publish(frame,*value);}catch(...){result=TimePublication::uncertain;}
        if(result==TimePublication::rejected){finish(ControlCode::rejected,ControlReason::context,request.id);return;}
        if(result!=TimePublication::published){
            latched_=true;status_.flags=(status_.flags&~time_available)|time_uncertain;
            finish(ControlCode::uncertain,ControlReason::publication,request.id);return;
        }
        status_.time=*value;
        if(freeze){saved_rate_=frame.time.rate;owns_freeze_=true;status_.flags|=time_frozen;}
        if(owns_freeze_)owned_=*value;
        if(release){owns_freeze_=false;status_.flags&=~time_frozen;}
        finish(release?ControlCode::released:ControlCode::published,ControlReason::none,request.id);
    }
};
}
