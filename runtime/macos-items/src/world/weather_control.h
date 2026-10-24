#pragma once
#include "weather_native.h"
#include <optional>

namespace crimson::mac_items {
class WeatherControl {
    std::optional<world::Request> pending_;
    WeatherSettings held_;
    std::uint64_t fingerprint_{},generation_{},observed_ms_{},await_id_{},await_deadline_{};
    bool owned_{},selection_{},frozen_{},await_auto_{},release_requested_{},latched_{};
    void finish(world::Reply& s,world::ControlCode code,world::ControlReason reason,std::uint64_t id=0) {
        s.code=code;s.reason=reason;s.id=id;
        if(id){s.weather_receipt=id;s.weather_result=code;s.weather_result_reason=reason;}
    }
    void drop(world::Reply& s,world::ControlReason reason) {
        const auto id=pending_?pending_->id:await_id_;
        finish(s,id?world::ControlCode::rejected:world::ControlCode::released,reason,id);
        pending_.reset();owned_=false;selection_=false;frozen_=false;await_auto_=false;
        release_requested_=false;await_id_=0;await_deadline_=0;s.flags&=~(world::weather_frozen|world::weather_available);
    }
    bool successful(world::Reply& s,WeatherPublication result,std::uint64_t id=0) {
        if(result==WeatherPublication::published)return true;
        if(result==WeatherPublication::rejected){
            drop(s,world::ControlReason::generation);
            if(id)finish(s,world::ControlCode::rejected,world::ControlReason::generation,id);
            return false;
        }
        fault(s,id);return false;
    }
    void awaiting(world::Reply& s,const world::Request& request) {
        await_id_=request.id;await_deadline_=request.not_after_ms;
        finish(s,world::ControlCode::queued,world::ControlReason::none,request.id);
    }
public:
    bool latched()const{return latched_;}
    bool busy()const{return pending_.has_value()||await_id_||await_auto_;}
    bool fresh(std::uint64_t now)const{return observed_ms_&&now>=observed_ms_&&now-observed_ms_<=1000;}
    void enqueue(const world::Request& request){pending_=request;}
    void fault(world::Reply& s,std::uint64_t id=0) {
        latched_=true;s.flags=(s.flags&~world::weather_available)|world::weather_uncertain;
        s.weather_reason=static_cast<unsigned>(world::ControlReason::publication);
        if(!id)id=pending_?pending_->id:await_id_;
        finish(s,world::ControlCode::uncertain,world::ControlReason::publication,id);
        pending_.reset();await_id_=0;
    }
    void expire(world::Reply& s,std::uint64_t now) {
        if(pending_&&now>pending_->not_after_ms){finish(s,world::ControlCode::expired,world::ControlReason::deadline,pending_->id);pending_.reset();}
        if(await_id_&&now>await_deadline_){
            finish(s,world::ControlCode::expired,world::ControlReason::deadline,await_id_);
            await_id_=0;await_deadline_=0;release_requested_=true;
        }
    }
    void world_changed(world::Reply& s) {
        s.flags&=~world::weather_available;release_requested_=true;
        s.weather_reason=static_cast<unsigned>(world::ControlReason::generation);
        if(pending_){finish(s,world::ControlCode::world_changed,world::ControlReason::generation,pending_->id);pending_.reset();}
        if(await_id_){finish(s,world::ControlCode::world_changed,world::ControlReason::generation,await_id_);await_id_=0;await_deadline_=0;}
    }
    template<class Apply,class Manual,class Phase>
    void producer(world::Reply& s,const WeatherFrame& frame,std::uint64_t now,bool task_fresh,Apply apply,Manual manual,Phase phase) {
        using namespace world;
        if(latched_)return;
        expire(s,now);
        if(!frame.ready){s.flags&=~weather_available;s.weather_reason=static_cast<unsigned>(ControlReason::context);return;}
        observed_ms_=now;
        s.weather_reason=static_cast<unsigned>(!task_fresh?ControlReason::context:
            frame.view.manual&&!owned_?ControlReason::foreign_change:ControlReason::none);
        if(task_fresh&&(!frame.view.manual||owned_))s.flags|=weather_available;else s.flags&=~weather_available;
        if(owned_&&frame.fingerprint!=fingerprint_){drop(s,ControlReason::generation);return;}
        if(owned_&&(!frame.view.manual||!frame.settings.same_control(held_))){drop(s,ControlReason::foreign_change);return;}
        if(owned_&&(release_requested_||!task_fresh||generation_!=s.generation)) {
            if(!successful(s,manual(false)))return;
            owned_=false;selection_=false;frozen_=false;await_auto_=true;release_requested_=false;s.flags&=~weather_frozen;
            return;
        }
        // This callback precedes the producer. The next callback witnesses the previous ordinary execution.
        if(await_auto_) {
            if(frame.fingerprint!=fingerprint_){drop(s,ControlReason::generation);return;}
            if(frame.view.manual){drop(s,ControlReason::foreign_change);return;}
            await_auto_=false;finish(s,ControlCode::released,ControlReason::none,await_id_);
            await_id_=0;await_deadline_=0;s.weather=frame.settings.parameters();return;
        }
        if(owned_&&await_id_) {
            const bool rain=held_.phase>0;
            if(frame.rain!=(rain?held_.precipitation:0)||frame.snow!=(rain?0:held_.precipitation)){fault(s);return;}
            finish(s,ControlCode::published,ControlReason::none,await_id_);await_id_=0;await_deadline_=0;
        }
        if(owned_) {
            if(!successful(s,phase(held_.phase)))return;
            s.weather=held_.parameters();
        } else s.weather=frame.settings.parameters();
        if(!pending_)return;
        const auto request=*pending_;pending_.reset();
        if(request.generation!=s.generation){finish(s,ControlCode::world_changed,ControlReason::generation,request.id);return;}
        if(!task_fresh||!fresh(now)||(!owned_&&frame.view.manual)){finish(s,ControlCode::rejected,ControlReason::context,request.id);return;}
        const bool release=request.operation==ControlOperation::release_weather
            ||(request.operation==ControlOperation::freeze_weather&&!request.enabled);
        if(release) {
            if(!owned_){finish(s,ControlCode::released,ControlReason::not_frozen,request.id);return;}
            if(request.operation==ControlOperation::freeze_weather&&selection_) {
                frozen_=false;s.flags&=~weather_frozen;finish(s,ControlCode::published,ControlReason::none,request.id);return;
            }
            if(!successful(s,manual(false),request.id))return;
            owned_=false;selection_=false;frozen_=false;await_auto_=true;s.flags&=~weather_frozen;
            awaiting(s,request);return;
        }
        if(request.operation==ControlOperation::freeze_weather&&owned_&&frozen_) {
            finish(s,ControlCode::published,ControlReason::none,request.id);return;
        }
        const auto settings=request.operation==ControlOperation::choose_weather?selected_weather(request.weather)
            :owned_?held_:frame.settings;
        const auto result=apply(settings,!owned_);
        if(result==WeatherPublication::rejected){finish(s,ControlCode::rejected,ControlReason::context,request.id);return;}
        if(!successful(s,result,request.id))return;
        fingerprint_=frame.fingerprint;generation_=s.generation;release_requested_=false;owned_=true;held_=settings;
        if(request.operation==ControlOperation::choose_weather)selection_=true;
        else{frozen_=true;s.flags|=weather_frozen;}
        s.weather=held_.parameters();awaiting(s,request);
    }
};
}
