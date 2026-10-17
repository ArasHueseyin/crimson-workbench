#pragma once
#include "teleport_native.h"
#include "marker_native.h"
#include "teleport_protocol.h"
#include <mutex>
#include <optional>

namespace crimson::marketplace::teleport {
// The worker handles only this pointer-free policy and IPC. Every engine call
// is made synchronously by observe on LiveItems' borrowed native task thread.
class TeleportPolicy {
    mutable std::mutex mutex_;
    Reply reply_{};std::optional<Request> pending_;Request last_request_{};
    std::uint64_t last_command_{},pending_at_{},last_observed_{},queued_at_{},last_marker_attempt_{};
    std::uint32_t queued_identity_{},queued_world_{};
    SavedPosition queued_destination_{};
    bool faulted_{},awaiting_position_{};
    static constexpr std::uint64_t kFreshMs=3000,kWaitMs=15000,kArrivalMs=60000;
    void clear_markers(MarkerCode reason) noexcept {
        reply_.marker_count=0;reply_.markers={};reply_.markers_observed_ms=0;reply_.marker_status=reason;
    }
    void uncertain(Reason reason) noexcept {reply_.state=State::uncertain;reply_.reason=reason;pending_.reset();awaiting_position_=false;}
    void latch_fault() noexcept {faulted_=true;clear_markers(MarkerCode::native_fault);uncertain(Reason::native_fault);reply_.state=State::faulted;}
    void expire(std::uint64_t now) noexcept {
        if(reply_.markers_observed_ms&&(now<reply_.markers_observed_ms||now-reply_.markers_observed_ms>kFreshMs))clear_markers(MarkerCode::context);
        if(pending_&&(now<pending_at_||now-pending_at_>kWaitMs||now>pending_->not_after_ms))uncertain(Reason::timeout);
        if(awaiting_position_&&(now<queued_at_||now-queued_at_>kArrivalMs))uncertain(Reason::timeout);
    }
    void result(State state,Reason reason) noexcept {reply_.state=state;reply_.reason=reason;}
    static Reason marker_reason(MarkerCode code) noexcept {
        switch(code){
        case MarkerCode::ready:return Reason::none;
        case MarkerCode::missing:return Reason::marker_missing;
        case MarkerCode::layer:return Reason::marker_layer;
        case MarkerCode::geometry_unavailable:return Reason::marker_geometry;
        case MarkerCode::placement_unavailable:case MarkerCode::placement_shifted:return Reason::marker_placement;
        case MarkerCode::world:return Reason::marker_world;
        case MarkerCode::component:case MarkerCode::list:return Reason::marker_list;
        case MarkerCode::native_fault:return Reason::native_fault;
        case MarkerCode::context:return Reason::marker_changed;
        default:return Reason::unavailable;
        }
    }
    struct FrameScope {
        TeleportPolicy& owner;const NativeBindings& bindings;NativeFrame& frame;bool finished{};
        void finish(){
            finished=true;
            try{if(!close_frame(bindings,frame)||frame.faulted)owner.latch_fault();}
            catch(...){owner.latch_fault();throw;}
        }
        ~FrameScope() noexcept {
            if(finished)return;
            // A C++ exception before Begin leaves a known native borrow to
            // release. Begin/hardware uncertainty must never trigger a guessed
            // release (which itself can enqueue the deferred field move).
            if(frame.acquired&&!frame.attempted&&!frame.faulted){
                try{close_frame(bindings,frame);}catch(...){frame.faulted=true;}
            }
            owner.latch_fault(); // Preserve the original C++ exception.
        }
    };
public:
    explicit TeleportPolicy(std::uint64_t session){reply_.session=session;}
    std::array<Bookmark,kSavedSlots> saved() const {std::lock_guard lock(mutex_);return reply_.bookmarks;}
    bool restore(const std::array<Bookmark,kSavedSlots>& values) {
        std::lock_guard lock(mutex_);if(reply_.epoch||last_command_||pending_)return false;
        for(const auto& value:values){
            if(value.reserved||value.occupied>1)return false;
            if(value.occupied&&(!valid_saved_position(value.position)||!valid_name(value.name)))return false;
            if(!value.occupied&&value.name!=std::array<char,48>{})return false;
        }
        reply_.bookmarks=values;return true;
    }
    Reply handle(const Request& request,std::uint64_t now) {
        std::lock_guard lock(mutex_);expire(now);
        auto rejected=[&](Reason reason){auto reply=reply_;reply.state=State::rejected;reply.reason=reason;return reply;};
        if(!valid_request(request))return rejected(Reason::malformed);
        if(request.operation==Operation::status)return reply_;
        if(request.session!=reply_.session||!reply_.session)return rejected(Reason::session);
        if(request.command==reply_.command){
            if(std::memcmp(&request,&last_request_,sizeof(request)))return rejected(Reason::command_order);
            return reply_; // Repeated IPC never repeats a command.
        }
        if(request.command<=last_command_)return rejected(Reason::command_order);
        if(faulted_)return rejected(Reason::native_fault);
        if(pending_||awaiting_position_)return rejected(Reason::busy);
        if(!reply_.epoch||request.epoch!=reply_.epoch||now<last_observed_||now-last_observed_>kFreshMs||now>request.not_after_ms||request.not_after_ms-now>kFreshMs)return rejected(Reason::stale_context);
        if(request.operation==Operation::travel_marker&&!(reply_.features&Feature::travel_marker))return rejected(Reason::marker_height_or_world_unproven);
        if((request.operation==Operation::travel_saved||request.operation==Operation::remove_saved)&&!reply_.bookmarks[request.slot].occupied)return rejected(Reason::slot);
        last_command_=request.command;reply_.command=request.command;last_request_=request;
        if(request.operation==Operation::remove_saved){reply_.bookmarks[request.slot]={};++reply_.bookmarks_revision;result(State::removed,Reason::none);return reply_;}
        pending_=request;pending_at_=now;result(State::pending,Reason::none);return reply_;
    }
    void observe(const NativeBindings& bindings,const crimson::live_items::TaskObservationV1& task,const MarkerBindings* markers=nullptr) {
        std::lock_guard lock(mutex_);
        if(faulted_)return;
        expire(task.monotonic_ms);
        NativeFrame frame{};FrameScope scope{*this,bindings,frame};const auto code=open_frame(bindings,task,frame);
        reply_.features=Feature::save_current|Feature::travel_saved;
        if(markers&&marker_bindings_ready(bindings,*markers))reply_.features|=Feature::travel_marker;
        reply_.native_reason=static_cast<std::uint32_t>(code);reply_.native_error=frame.native_error;
        if(code==NativeCode::ready){
            const bool changed=reply_.identity!=frame.identity||reply_.world!=frame.world||reply_.current.stage!=frame.current.stage||reply_.current.field_definition!=frame.current.field_definition;
            if(changed){
                clear_markers(MarkerCode::context);last_marker_attempt_=0;
                ++reply_.epoch;if(!reply_.epoch)++reply_.epoch;
                if(pending_||awaiting_position_)uncertain(Reason::context_changed);
            }
            reply_.identity=frame.identity;reply_.world=frame.world;reply_.current=frame.current;last_observed_=task.monotonic_ms;reply_.observed_ms=last_observed_;
            if(markers&&marker_bindings_ready(bindings,*markers)){
                // Copy the lists at most twice per second. A travel command
                // always captures and validates its own fresh native rows.
                if(!last_marker_attempt_||task.monotonic_ms<last_marker_attempt_||task.monotonic_ms-last_marker_attempt_>=500){
                    last_marker_attempt_=task.monotonic_ms;MarkerSnapshot snapshot{};
                    const auto captured=capture_markers(bindings,*markers,frame,snapshot);
                    if(captured==MarkerCode::ready){reply_.marker_count=snapshot.count;reply_.markers=snapshot.values;reply_.markers_observed_ms=task.monotonic_ms;reply_.marker_status=MarkerCode::ready;}
                    else clear_markers(captured);
                }
            } else clear_markers(MarkerCode::unsupported);
            if(reply_.state==State::waiting)result(State::ready,Reason::none);
            if(awaiting_position_){
                if(frame.identity!=queued_identity_||frame.world!=queued_world_){uncertain(Reason::context_changed);}
                else if(frame.current.field_definition==queued_destination_.field_definition&&frame.current.stage==queued_destination_.stage){
                    // A separate observation, never an assertion of a client
                    // render/Ack or a licence to retry. Native spawning can
                    // select a different destination because of quest state.
                    const auto& a=frame.current.position;const auto& b=queued_destination_.position;
                    const double dx=double(a.x)-b.x,dy=double(a.y)-b.y,dz=double(a.z)-b.z;
                    if(dx*dx+dy*dy+dz*dz<=0.25){awaiting_position_=false;result(State::position_observed,Reason::none);}
                }
            }
            if(pending_&&!frame.faulted){
                const auto request=*pending_;pending_.reset();
                if(request.epoch!=reply_.epoch)result(State::rejected,Reason::context_changed);
                else if(request.operation==Operation::save_current){
                    reply_.bookmarks[request.slot]={frame.current,request.name,1,0};++reply_.bookmarks_revision;result(State::captured,Reason::none);
                } else {
                    SavedPosition destination{};NativeCode moved=NativeCode::unsupported;
                    if(request.operation==Operation::travel_marker){
                        if(!markers){result(State::rejected,Reason::unavailable);scope.finish();return;}
                        const auto marker_move=begin_marker_move(bindings,*markers,frame,request.marker,static_cast<MarkerSource>(request.marker_source));
                        if(marker_move.marker!=MarkerCode::ready){
                            clear_markers(marker_move.marker);result(State::rejected,marker_reason(marker_move.marker));scope.finish();return;
                        }
                        destination=marker_move.destination;moved=marker_move.movement;
                    } else {destination=reply_.bookmarks[request.slot].position;moved=begin_saved_move(bindings,frame,destination);}
                    reply_.native_reason=static_cast<std::uint32_t>(moved);reply_.native_error=frame.native_error;
                    if(moved==NativeCode::accepted){
                        clear_markers(MarkerCode::context);
                        queued_identity_=frame.identity;queued_world_=frame.world;queued_destination_=destination;queued_at_=task.monotonic_ms;
                        awaiting_position_=true;result(State::native_queued,Reason::none);
                    } else if(frame.attempted)uncertain(moved==NativeCode::publish_mismatch?Reason::publish_mismatch:moved==NativeCode::native_fault?Reason::native_fault:Reason::native_error);
                    else result(State::rejected,moved==NativeCode::busy?Reason::busy:Reason::unavailable);
                }
            }
        } else clear_markers(frame.faulted?MarkerCode::native_fault:MarkerCode::context);
        scope.finish();
    }
};
} // namespace crimson::marketplace::teleport
