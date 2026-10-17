#pragma once
#include <atomic>
#include <limits>
#include "photo_native.h"
namespace crimson::marketplace::photo {
class Policy {
    std::atomic_flag busy_=ATOMIC_FLAG_INIT;
    struct Guard {std::atomic_flag& flag;~Guard(){flag.clear(std::memory_order_release);}};
    enum class Await {none,enter,hide_ready,hide,show,leave};
    Reply reply_{};Request request_{};std::uint64_t token_{},until_{};
    Await awaiting_{};bool pending_{},owned_{},hide_on_enter_{},available_{};
    void result(State state,Reason reason=Reason::none) {reply_.state=state;reply_.reason=reason;reply_.owned=owned_;}
    void invalidate() {
        token_=0;owned_=false;pending_=false;awaiting_=Await::none;available_=false;
        reply_.native_photo=reply_.hud_hidden=reply_.scene_frozen=0;
        result(State::waiting,Reason::unavailable);
    }
public:
    explicit Policy(std::uint64_t session){reply_.session=session;}
    Reply handle(const Request& r,std::uint64_t now) {
        if(busy_.test_and_set(std::memory_order_acquire)) {Reply value{};value.state=State::rejected;value.reason=Reason::busy;return value;}
        Guard guard{busy_};
        auto rejected=[&](Reason reason){auto value=reply_;value.command=r.command;value.state=State::rejected;value.reason=reason;return value;};
        if(!valid(r))return rejected(Reason::malformed);
        if(r.session!=reply_.session&&(r.operation!=Operation::status||r.session))return rejected(Reason::session);
        if(r.operation==Operation::status) {
            auto value=reply_;
            if(!token_||now<value.observed_ms||now-value.observed_ms>1000){value.state=State::waiting;value.reason=Reason::stale;}
            return value;
        }
        if(r.command==reply_.command) {
            if(std::memcmp(&r,&request_,sizeof(r)))return rejected(Reason::order);
            return reply_;
        }
        if(r.command<reply_.command)return rejected(Reason::order);
        if(r.epoch!=reply_.epoch)return rejected(Reason::epoch);
        if(!available_||!token_||now<reply_.observed_ms||now-reply_.observed_ms>1000||r.not_after_ms<=now||r.not_after_ms-now>3000)return rejected(Reason::stale);
        if(pending_||awaiting_!=Await::none)return rejected(Reason::busy);
        request_=r;pending_=true;reply_.command=r.command;result(State::pending);return reply_;
    }
    template<class Bridge> void observe(Bridge& bridge,void* view,std::uint64_t now) {
        if(busy_.test_and_set(std::memory_order_acquire))return;
        Guard guard{busy_};Frame frame{};
        if(!bridge.snapshot(view,frame)) {
            // Missing transition data is not evidence of a new context. Keep
            // the pointer-free receipt and never replay an issued call.
            available_=false;
            if(pending_&&now>request_.not_after_ms){pending_=false;result(State::rejected,Reason::stale);}
            else if(awaiting_!=Await::none&&now>until_){awaiting_=Await::none;result(State::uncertain,Reason::timeout);}
            else result(token_?State::uncertain:State::waiting,Reason::unavailable);
            return;
        }
        available_=true;
        if(frame.token!=token_) {
            const bool changed=token_!=0||reply_.epoch!=0;
            if(reply_.epoch==std::numeric_limits<std::uint64_t>::max()){invalidate();return;}
            ++reply_.epoch;token_=frame.token;owned_=false;pending_=false;awaiting_=Await::none;
            result(State::ready,changed?Reason::context_changed:Reason::none);
        }
        reply_.observed_ms=now;reply_.native_photo=frame.photo;reply_.hud_hidden=frame.hidden;reply_.scene_frozen=frame.frozen;
        // Expiry must precede every success observation. A late frame may
        // update current native status, but cannot acknowledge the command,
        // adopt a late camera, or trigger its deferred hide-on-enter action.
        if(awaiting_!=Await::none&&now>until_){awaiting_=Await::none;result(State::uncertain,Reason::timeout);}
        if(awaiting_!=Await::none) {
            if(awaiting_==Await::enter&&frame.photo&&frame.frozen) {
                owned_=true;awaiting_=Await::none;result(State::active);
                if(hide_on_enter_&&!frame.hidden) {
                    awaiting_=frame.controls?Await::hide:Await::hide_ready;result(State::native_queued);
                    if(frame.controls)bridge.hide(frame);return;
                }
            } else if(awaiting_==Await::leave&&!frame.photo) {
                owned_=false;awaiting_=Await::none;result(State::left);
            } else if(frame.controls&&((awaiting_==Await::hide&&frame.hidden)||(awaiting_==Await::show&&!frame.hidden))) {
                const auto state=awaiting_==Await::hide?State::hud_hidden:State::hud_visible;
                awaiting_=Await::none;result(state);
            }
            if(awaiting_==Await::hide_ready&&frame.controls&&now<=until_) {
                awaiting_=Await::hide;result(State::native_queued);bridge.hide(frame);return;
            }
        }
        if(owned_&&!frame.photo&&awaiting_!=Await::enter) {
            owned_=false;awaiting_=Await::none;
            // Actual closure still drops ownership; a timed-out receipt must
            // remain uncertain instead of becoming a late success response.
            if(reply_.state==State::uncertain&&reply_.reason==Reason::timeout)reply_.owned=0;
            else result(State::left);
        }
        if(!pending_)return;
        if(now>request_.not_after_ms){pending_=false;result(State::rejected,Reason::stale);return;}
        if(request_.operation!=Operation::enter&&owned_&&frame.photo&&!frame.controls){result(State::pending,Reason::unavailable);return;}
        pending_=false;
        until_=request_.not_after_ms;
        if(request_.operation==Operation::enter) {
            if(frame.photo){result(State::rejected,Reason::foreign_photo);return;}
            if(frame.camera_busy){result(State::rejected,Reason::camera_busy);return;}
            hide_on_enter_=request_.hide_on_enter!=0;awaiting_=Await::enter;
            result(State::native_queued);bridge.enter(frame);return;
        }
        if(!owned_||!frame.photo){result(State::rejected,Reason::not_owned);return;}
        switch(request_.operation) {
        case Operation::hide_hud:
            if(frame.hidden){result(State::hud_hidden);return;}
            awaiting_=Await::hide;result(State::native_queued);bridge.hide(frame);break;
        case Operation::show_hud:
            if(!frame.hidden){result(State::hud_visible);return;}
            awaiting_=Await::show;result(State::native_queued);bridge.show(frame);break;
        case Operation::leave:
            awaiting_=Await::leave;result(State::native_queued);bridge.leave(frame);break;
        default:result(State::rejected,Reason::malformed);break;
        }
    }
};
template<class Bridge> inline void dispatch(Bridge& bridge,Policy& policy,FrameUpdate original,bool enabled,Address base,Address caller,std::uint64_t now,void* view,std::uint64_t(*clock)()=nullptr) {
    if(!original)return;
    static thread_local bool in_callback=false;
    if(in_callback){original(view);return;}
    struct Scope {bool& value;explicit Scope(bool& v):value(v){value=true;}~Scope(){value=false;}} scope(in_callback);
    original(view);
    if(enabled&&leased_caller(base,caller))policy.observe(bridge,view,clock?clock():now);
}
}
