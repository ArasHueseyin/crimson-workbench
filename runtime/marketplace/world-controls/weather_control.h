#pragma once
#include "weather_native.h"

namespace crimson::marketplace::world_controls {
struct WeatherClientObservation {
    std::uint32_t world{},mask{};std::uint16_t stage{0xffff};std::uint64_t fingerprint{};
};
// The pinned client-phase dispatcher at 0xa0df38 accepts seven slots [0,6].
// Normal ClientPhase_LivePlay uses slot 5, not one of three buffer indices.
inline constexpr std::size_t kClientPhaseSlots=7;
inline WeatherNativeReason read_weather_client(const WeatherBindings& b,void* borrowed_field,std::uintptr_t caller,std::uint64_t salt,WeatherClientObservation& out) noexcept {
    __try {
        if(!b.verified||!b.service)return WeatherNativeReason::unsupported_build;
        if(caller!=b.base+0xa8aed9)return WeatherNativeReason::client_field;
        const auto field=static_cast<std::uint8_t*>(borrowed_field);
        if(!field||*reinterpret_cast<std::uintptr_t*>(field)!=b.client_field_vtable)return WeatherNativeReason::client_field;
        auto buffers=*reinterpret_cast<std::uint8_t**>(field+0x210);if(!buffers)return WeatherNativeReason::client_field;
        const auto phase=std::size_t{buffers[0x48]};if(phase>=kClientPhaseSlots)return WeatherNativeReason::client_field;
        auto active=*reinterpret_cast<std::uint8_t**>(buffers+0x10+phase*8);
        if(!active||!active[9])return WeatherNativeReason::client_field;
        auto service=static_cast<std::uint8_t*>(*b.service);if(!service)return WeatherNativeReason::current_manager;
        auto holder=*reinterpret_cast<std::uint8_t**>(service+0x38);if(!holder)return WeatherNativeReason::current_manager;
        auto manager=*reinterpret_cast<std::uint8_t**>(holder+0x38);
        if(!manager||*reinterpret_cast<std::uintptr_t*>(manager)!=b.manager_vtable)return WeatherNativeReason::current_manager;
        auto result=*reinterpret_cast<std::uint8_t**>(manager+0x60);if(!result)return WeatherNativeReason::result_owner;
        out.world=*reinterpret_cast<std::uint32_t*>(field+0x180);out.stage=*reinterpret_cast<std::uint16_t*>(field+0x2b2);out.mask=*reinterpret_cast<std::uint32_t*>(field+0x178);
        out.fingerprint=weather_fingerprint(reinterpret_cast<std::uintptr_t>(manager),reinterpret_cast<std::uintptr_t>(result-0x28),salt);
        return out.stage==0xffff?WeatherNativeReason::client_field:WeatherNativeReason::ready;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return WeatherNativeReason::native_fault;}
}
class WeatherControl {
    Request pending_{};WeatherSnapshot original_{},held_{};WeatherParameters selected_{};
    std::uint64_t fingerprint_{},generation_{},client_ms_{},client_fingerprint_{},await_id_{},await_deadline_{};
    std::uint32_t client_world_{};std::uint16_t client_stage_{0xffff};
    bool pending_valid_{},owned_{},frozen_{},selection_{},original_manual_{},await_auto_{},latched_{},release_requested_{};
    void finish(Reply& s,ControlCode code,ControlReason reason,std::uint64_t id=0) noexcept {
        s.code=code;s.reason=reason;s.id=id;if(id){s.weather_receipt=id;s.weather_result=code;s.weather_result_reason=reason;}
    }
    void latch(Reply& s,WeatherNativeReason reason,std::uint64_t id=0) noexcept {
        if(!id)id=pending_valid_?pending_.id:await_id_;
        latched_=true;pending_valid_=false;s.flags&=~weather_available;s.flags|=weather_uncertain;s.weather_reason=static_cast<std::uint32_t>(reason);
        finish(s,reason==WeatherNativeReason::native_fault?ControlCode::fault_latched:ControlCode::uncertain,
            reason==WeatherNativeReason::native_fault?ControlReason::native_fault:ControlReason::publication,id);
    }
    bool successful(Reply& s,WeatherNativeReason result,std::uint64_t id=0) noexcept {
        s.weather_reason=static_cast<std::uint32_t>(result);
        if(result==WeatherNativeReason::published)return true;latch(s,result,id);return false;
    }
    void relinquish(Reply& s,ControlReason reason) noexcept {
        if(pending_valid_){finish(s,ControlCode::rejected,reason,pending_.id);pending_valid_=false;}
        owned_=false;frozen_=false;selection_=false;await_auto_=false;release_requested_=false;s.flags&=~weather_frozen;finish(s,ControlCode::released,reason,await_id_);await_id_=0;
    }
    bool still_owned(const WeatherSnapshot& current,const WeatherParameters& params) const noexcept {
        if(selection_)return params.precipitation==selected_.precipitation&&params.cloudiness==selected_.cloudiness&&params.humidity==selected_.humidity&&params.wind_speed==selected_.wind_speed&&
            current.fog==selected_.height_fog&&current.cloud==(selected_.cloudiness>0?selected_.cloudiness:0)&&current.aerosol==0;
        if(!frozen_)return true;
        if(current.aerosol!=held_.aerosol||current.fog!=held_.fog||current.cloud!=held_.cloud)return false;
        for(std::size_t i=0;i<current.scalar.size();++i) {
            // Native manual precipitation derives 4 outputs each main tick;
            // both producer paths update 5 climate outputs. Those changes
            // are expected and are precisely what the freeze restores.
            if((i>=0x34&&i<0x44)||(i>=0x6c&&i<0x80))continue;
            if(current.scalar[i]!=held_.scalar[i])return false;
        }
        return true;
    }
public:
    bool latched() const noexcept{return latched_;}
    bool busy() const noexcept{return pending_valid_||await_auto_||await_id_;}
    bool fresh(std::uint64_t now) const noexcept{return client_ms_&&now>=client_ms_&&now-client_ms_<=1000;}
    void enqueue(const Request& r) noexcept{pending_=r;pending_valid_=true;}
    void expire(Reply& s,std::uint64_t now) noexcept {
        if(pending_valid_&&now>pending_.not_after_ms){finish(s,ControlCode::expired,ControlReason::deadline,pending_.id);pending_valid_=false;}
        if(await_id_&&await_deadline_&&now>await_deadline_) {
            finish(s,ControlCode::expired,ControlReason::deadline,await_id_);await_id_=0;await_deadline_=0;release_requested_=true;
        }
    }
    void world_changed(Reply& s) noexcept {
        s.flags&=~weather_available;release_requested_=true;
        if(pending_valid_){finish(s,ControlCode::world_changed,ControlReason::generation,pending_.id);pending_valid_=false;}
        if(await_id_){finish(s,ControlCode::world_changed,ControlReason::generation,await_id_);await_id_=0;await_deadline_=0;}
    }
    void client(Reply& s,const WeatherClientObservation& client,std::uint64_t now) noexcept {
        if(latched_)return;
        client_ms_=now;client_fingerprint_=client.fingerprint;client_world_=client.world;client_stage_=client.stage;
        if(s.world!=client.world||s.stage!=client.stage){s.flags&=~weather_available;release_requested_=true;return;}
        if(await_id_&&!await_auto_&&owned_&&client.fingerprint==fingerprint_) {
            const auto expected=selected_.precipitation>0?(selected_.phase==PrecipitationPhase::snow?2u:1u):32u;
            if(!selection_||((client.mask&0x23u)==expected)) {
                finish(s,ControlCode::published,ControlReason::none,await_id_);await_id_=0;await_deadline_=0;
            }
        }
    }
    void producer(Reply& s,const WeatherBindings& b,WeatherFrame& frame,std::uint64_t now,bool primary,bool task_fresh) noexcept {
        if(latched_)return;
        WeatherSnapshot current{};WeatherParameters params{};const auto read=read_weather(frame,current,params);
        if(read!=WeatherNativeReason::ready){latch(s,read);return;}
        s.weather_reason=static_cast<std::uint32_t>(read);
        const bool context=task_fresh&&fresh(now)&&client_world_==s.world&&client_stage_==s.stage&&client_fingerprint_==frame.fingerprint;
        if(context)s.flags|=weather_available;else s.flags&=~weather_available;
        if(owned_&&frame.fingerprint!=fingerprint_) {
            // No stale manager pointer is retained or dereferenced. The new
            // manager keeps its native policy. The old manager's lifetime ends
            // with its world; callbacks on it are rejected by the global check.
            relinquish(s,ControlReason::generation);
        }
        if(owned_&&!frame.manual) {relinquish(s,ControlReason::foreign_change);return;}
        if(owned_&&!still_owned(current,params)){relinquish(s,ControlReason::foreign_change);return;}
        if(owned_&&(release_requested_||!task_fresh||generation_!=s.generation)) {
            if(original_manual_&&!successful(s,restore_weather(frame,original_)))return;
            if(!successful(s,manual_weather(b,frame,original_manual_)))return;
            if(original_manual_){relinquish(s,ControlReason::generation);return;}
            owned_=false;frozen_=false;selection_=false;await_auto_=true;release_requested_=false;s.flags&=~weather_frozen;return;
        }
        if(await_auto_) {
            // Observe an actual ordinary producer execution with manual=false,
            // rather than claiming that writing the flag proved continuation.
            if(primary&&!frame.manual){await_auto_=false;finish(s,ControlCode::released,ControlReason::none,await_id_);await_id_=0;}
            return;
        }
        if(owned_) {
            if(frozen_){if(!successful(s,restore_weather(frame,held_)))return;}
            else if(selection_){if(!successful(s,apply_weather(b,frame,selected_)))return;}
        }
        if(!pending_valid_){WeatherSnapshot ignored{};if(read_weather(frame,ignored,s.weather)!=WeatherNativeReason::ready)latch(s,WeatherNativeReason::native_fault);return;}
        const auto request=pending_;pending_valid_=false;
        if(request.generation!=s.generation){finish(s,ControlCode::world_changed,ControlReason::generation,request.id);return;}
        if(now>request.not_after_ms){finish(s,ControlCode::expired,ControlReason::deadline,request.id);return;}
        if(!context){finish(s,ControlCode::rejected,ControlReason::context,request.id);return;}
        if(request.operation==ControlOperation::release_weather||(request.operation==ControlOperation::freeze_weather&&!request.enabled)) {
            if(!owned_){finish(s,ControlCode::released,ControlReason::not_frozen,request.id);return;}
            if(request.operation==ControlOperation::freeze_weather&&selection_) {
                frozen_=false;s.flags&=~weather_frozen;finish(s,ControlCode::published,ControlReason::none,request.id);return;
            }
            await_id_=request.id;await_deadline_=request.not_after_ms;release_requested_=true;
            // Perform release on this owning callback, with no queued pointer.
            if(original_manual_&&!successful(s,restore_weather(frame,original_),request.id))return;
            if(!successful(s,manual_weather(b,frame,original_manual_),request.id))return;
            owned_=false;frozen_=false;selection_=false;release_requested_=false;s.flags&=~weather_frozen;
            if(original_manual_){finish(s,ControlCode::released,ControlReason::none,request.id);await_id_=0;}else await_auto_=true;
            return;
        }
        if(!owned_) {
            release_requested_=false;
            original_=current;original_manual_=frame.manual;generation_=s.generation;fingerprint_=frame.fingerprint;
            if(!successful(s,manual_weather(b,frame,true),request.id))return;owned_=true;
        }
        if(request.operation==ControlOperation::choose_weather) {
            if(!successful(s,apply_weather(b,frame,request.weather),request.id))return;
            selected_=request.weather;selection_=true;
            WeatherParameters ignored{};
            if(read_weather(frame,held_,ignored)!=WeatherNativeReason::ready){latch(s,WeatherNativeReason::native_fault,request.id);return;}
            s.weather=selected_;
        } else {
            WeatherParameters ignored{};
            if(read_weather(frame,held_,ignored)!=WeatherNativeReason::ready){latch(s,WeatherNativeReason::native_fault,request.id);return;}
            frozen_=true;s.flags|=weather_frozen;s.weather=ignored;selected_=ignored;
        }
        await_id_=request.id;await_deadline_=request.not_after_ms;
        // Completion waits for the native leased client-field publication.
        s.code=ControlCode::queued;s.reason=ControlReason::none;s.id=request.id;
    }
    void fault(Reply& s,WeatherNativeReason reason) noexcept {latch(s,reason);}
};
} // namespace crimson::marketplace::world_controls
