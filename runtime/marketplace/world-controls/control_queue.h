#pragma once
#include "control_protocol.h"
#include "clock_native.h"
#include "weather_control.h"

namespace crimson::marketplace::world_controls {
// The queue owns values only. Native pointers live solely in a callback-local
// ClockFrame, and every accepted command is consumed once before native calls.
class ControlQueue {
    SRWLOCK lock_=SRWLOCK_INIT;
    Reply status_{};
    Request pending_{};
    std::uint64_t high_id_{},observed_ms_{};
    FieldTime owned_freeze_{};
    std::uint16_t saved_rate_{};
    bool pending_valid_{},owns_freeze_{},latched_{},have_world_{};
    WeatherControl weather_{};
    void finish(ControlCode code,ControlReason reason,std::uint64_t id=0) noexcept {
        status_.code=code;status_.reason=reason;
        status_.id=id;
        if(id){status_.time_receipt=id;status_.time_result=code;status_.time_result_reason=reason;}
    }
    void latch(ControlCode code,ControlReason reason,std::uint64_t id=0) noexcept {
        if(!id&&pending_valid_)id=pending_.id;
        latched_=true;status_.flags&=~time_available;status_.flags|=time_uncertain;
        pending_valid_=false;finish(code,reason,id);
    }
    void outcome(ClockNativeReason result,std::uint64_t id) noexcept {
        status_.clock_reason=static_cast<std::uint32_t>(result);
        if(result==ClockNativeReason::published)finish(ControlCode::published,ControlReason::none,id);
        else latch(result==ClockNativeReason::native_fault?ControlCode::fault_latched:ControlCode::uncertain,
            result==ClockNativeReason::native_fault?ControlReason::native_fault:ControlReason::publication,id);
    }
    void observe_locked(const ClockBindings& b,const crimson::live_items::TaskObservationV1& task) noexcept {
        if(latched_)return; // Includes native acquisition: no automatic retry.
        status_.monotonic_ms=task.monotonic_ms;
        ClockFrame frame{};const auto opened=open_clock_frame(b,task,frame);
        status_.clock_reason=static_cast<std::uint32_t>(opened);
        const bool changed=frame.context_valid&&have_world_&&
            (status_.identity!=frame.identity||status_.world!=frame.world||status_.stage!=frame.stage);
        const bool expired_world=have_world_&&task.monotonic_ms>observed_ms_&&task.monotonic_ms-observed_ms_>10'000;
        bool pending_cancelled=false;
        if(frame.context_valid) {
            if(changed||expired_world)weather_.world_changed(status_);
            if(!have_world_||changed||expired_world){++status_.generation;if(!status_.generation)++status_.generation;}
            have_world_=true;observed_ms_=task.monotonic_ms;
            status_.identity=frame.identity;status_.world=frame.world;status_.stage=frame.stage;
            // Exact equality includes anchor/day/rate: an unrelated zero rate
            // is never treated as our freeze or overwritten with our saved rate.
            if(owns_freeze_&&frame.raw!=owned_freeze_) {
                owns_freeze_=false;status_.flags&=~time_frozen;
                if(pending_valid_){finish(ControlCode::rejected,ControlReason::foreign_change,pending_.id);pending_valid_=false;}
                else finish(ControlCode::released,ControlReason::foreign_change);
            }
            if((changed||expired_world)&&pending_valid_) {
                finish(ControlCode::world_changed,ControlReason::generation,pending_.id);pending_valid_=false;pending_cancelled=true;
            }
            if(opened!=ClockNativeReason::ready&&pending_valid_) {
                finish(ControlCode::rejected,opened==ClockNativeReason::stage_fixed?ControlReason::stage_fixed:ControlReason::native_stopped,pending_.id);
                pending_valid_=false;pending_cancelled=true;
            }
            if(owns_freeze_&&(changed||expired_world||opened!=ClockNativeReason::ready)) {
                status_.flags&=~time_available;
                const auto result=release_owned_clock(b,frame,owned_freeze_,saved_rate_);
                status_.clock_reason=static_cast<std::uint32_t>(result);
                if(result!=ClockNativeReason::published)outcome(result,0);
                else {owns_freeze_=false;status_.flags&=~time_frozen;status_.time=frame.time;if(!pending_cancelled)finish(ControlCode::released,ControlReason::generation);}
                // One mutation per observation; let the next callback advertise
                // the new field instead of using a pre-publication snapshot.
                if(!close_clock_frame(b,frame))latch(ControlCode::fault_latched,ControlReason::native_fault);
                return;
            }
        }
        if(opened!=ClockNativeReason::ready) {
            status_.flags&=~time_available;
            if(opened==ClockNativeReason::native_fault)latch(ControlCode::fault_latched,ControlReason::native_fault);
            else if(!pending_valid_&&!pending_cancelled) {
                const auto reason=opened==ClockNativeReason::stage_fixed?ControlReason::stage_fixed:
                    opened==ClockNativeReason::field_clock_stopped?ControlReason::native_stopped:ControlReason::context;
                finish(ControlCode::unavailable,reason);
            }
            if(!close_clock_frame(b,frame))latch(ControlCode::fault_latched,ControlReason::native_fault);
            return;
        }
        status_.flags|=time_available;status_.time=frame.time;
        if(status_.code==ControlCode::unavailable)finish(ControlCode::ready,ControlReason::none);
        std::uint64_t attempted_id{};
        if(pending_valid_) {
            const auto request=pending_;pending_valid_=false; // Consume before mutation.
            attempted_id=request.id;
            if(request.generation!=status_.generation)finish(ControlCode::world_changed,ControlReason::generation,request.id);
            else if(task.monotonic_ms>request.not_after_ms)finish(ControlCode::expired,ControlReason::deadline,request.id);
            else if(request.operation==ControlOperation::choose_time||request.operation==ControlOperation::advance_time) {
                const auto value=request.operation==ControlOperation::advance_time?
                    advance_time(frame.time,request.time.hour,frame.native_now):choose_time(frame.time,request.time,frame.native_now);
                if(!value)finish(ControlCode::rejected,ControlReason::invalid_value,request.id);
                else {
                    const auto result=publish_clock_frame(b,frame,*value);outcome(result,request.id);
                    if(result==ClockNativeReason::published){status_.time=*value;if(owns_freeze_)owned_freeze_=*value;}
                }
            } else if(request.operation==ControlOperation::freeze_time&&request.enabled) {
                if(owns_freeze_)finish(ControlCode::published,ControlReason::none,request.id);
                else if(frame.time.rate==0)finish(ControlCode::rejected,ControlReason::native_stopped,request.id);
                else {
                    const auto value=freeze_time(frame.time,frame.native_now);
                    if(!value)finish(ControlCode::rejected,ControlReason::invalid_value,request.id);
                    else {
                        const auto rate=frame.time.rate;const auto result=publish_clock_frame(b,frame,*value);outcome(result,request.id);
                        if(result==ClockNativeReason::published){saved_rate_=rate;owned_freeze_=*value;owns_freeze_=true;status_.flags|=time_frozen;status_.time=*value;}
                    }
                }
            } else {
                if(!owns_freeze_)finish(ControlCode::released,ControlReason::not_frozen,request.id);
                else {
                    const auto result=release_owned_clock(b,frame,owned_freeze_,saved_rate_);outcome(result,request.id);
                    if(result==ClockNativeReason::published){owns_freeze_=false;status_.flags&=~time_frozen;status_.time=frame.time;finish(ControlCode::released,ControlReason::none,request.id);}
                }
            }
        }
        if(!close_clock_frame(b,frame))latch(ControlCode::fault_latched,ControlReason::native_fault,attempted_id);
    }
public:
    explicit ControlQueue(std::uint64_t session) noexcept {status_.session=session;}
    Reply handle(const Request& request,std::uint64_t now) noexcept {
        AcquireSRWLockExclusive(&lock_);
        if(pending_valid_&&now>pending_.not_after_ms){finish(ControlCode::expired,ControlReason::deadline,pending_.id);pending_valid_=false;}
        weather_.expire(status_,now);
        auto result=status_;result.monotonic_ms=now;
        if(!have_world_||now<observed_ms_||now-observed_ms_>10'000)
            result.flags&=~(time_available|weather_available);
        if(!weather_.fresh(now))result.flags&=~weather_available;
        const bool weather_operation=is_weather_operation(request.operation);
        ControlReason reject=ControlReason::none;
        if(request.magic!=kControlMagic||request.version!=kControlVersion||request.bytes!=sizeof(Request)||request.reserved||
            static_cast<std::uint32_t>(request.operation)>static_cast<std::uint32_t>(ControlOperation::advance_time))reject=ControlReason::protocol;
        else if(request.operation==ControlOperation::status){ReleaseSRWLockExclusive(&lock_);return result;}
        else if((!weather_operation&&latched_)||(weather_operation&&weather_.latched())){ReleaseSRWLockExclusive(&lock_);return result;}
        else if(!request.session||request.session!=status_.session)reject=ControlReason::session;
        else if(!have_world_||request.generation!=status_.generation)reject=ControlReason::generation;
        else if(!request.id||request.id<=high_id_)reject=ControlReason::replay;
        else if(request.not_after_ms<now||request.not_after_ms-now>10'000||now<observed_ms_||now-observed_ms_>10'000)reject=ControlReason::deadline;
        else if(!(result.flags&(weather_operation?weather_available:time_available)))reject=ControlReason::context;
        else if(weather_operation?weather_.busy():pending_valid_)reject=ControlReason::busy;
        else if(request.enabled>1)reject=ControlReason::invalid_value;
        else if(request.operation==ControlOperation::choose_weather&&!valid_weather(request.weather))reject=ControlReason::invalid_value;
        else if(request.operation==ControlOperation::choose_time&&(request.time.hour>=24||request.time.minute>=60||request.time.second>=60||request.time.millisecond>=1000))reject=ControlReason::invalid_value;
        else if(request.operation==ControlOperation::advance_time&&!valid_advance_payload(request))reject=ControlReason::invalid_value;
        if(reject!=ControlReason::none){result.code=ControlCode::rejected;result.reason=reject;result.id=request.id;}
        else {
            high_id_=request.id;
            if(weather_operation){weather_.enqueue(request);status_.weather_receipt=request.id;status_.weather_result=ControlCode::queued;status_.weather_result_reason=ControlReason::none;}
            else {pending_=request;pending_valid_=true;status_.time_receipt=request.id;status_.time_result=ControlCode::queued;status_.time_result_reason=ControlReason::none;}
            finish(ControlCode::queued,ControlReason::none);status_.id=request.id;result=status_;result.monotonic_ms=now;
        }
        ReleaseSRWLockExclusive(&lock_);return result;
    }
    void observe(const ClockBindings& b,const crimson::live_items::TaskObservationV1& task) noexcept {
        if(!TryAcquireSRWLockExclusive(&lock_))return;
        observe_locked(b,task);ReleaseSRWLockExclusive(&lock_);
    }
    void weather_producer(const WeatherBindings& b,void* manager,std::uintptr_t caller,std::uint64_t now) noexcept {
        AcquireSRWLockExclusive(&lock_);
        if(!weather_.latched()) {
            WeatherFrame frame{};const auto opened=open_weather_frame(b,manager,caller,status_.session,frame);
            if(opened==WeatherNativeReason::ready)weather_.producer(status_,b,frame,now,caller==b.base+0x3b1073c,
                have_world_&&now>=observed_ms_&&now-observed_ms_<=10'000);
            else if(opened==WeatherNativeReason::native_fault)weather_.fault(status_,opened);
            else {status_.weather_reason=static_cast<std::uint32_t>(opened);status_.flags&=~weather_available;}
        }
        ReleaseSRWLockExclusive(&lock_);
    }
    void weather_client(const WeatherBindings& b,void* field,std::uintptr_t caller,std::uint64_t now) noexcept {
        AcquireSRWLockExclusive(&lock_);
        if(!weather_.latched()) {
            WeatherClientObservation client{};const auto read=read_weather_client(b,field,caller,status_.session,client);
            if(read==WeatherNativeReason::ready)weather_.client(status_,client,now);
            else if(read==WeatherNativeReason::native_fault)weather_.fault(status_,read);
            else status_.flags&=~weather_available;
        }
        ReleaseSRWLockExclusive(&lock_);
    }
};
} // namespace crimson::marketplace::world_controls
