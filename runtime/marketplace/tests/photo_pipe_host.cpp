#include <Windows.h>
#include "../photo-mode/photo_pipe.h"

namespace photo=crimson::marketplace::photo;
namespace {
// This executable owns all values. It never loads or calls the game.
constexpr std::uint64_t session=0xfedcba9876543210ULL,epoch=0x20000000000001ULL;
photo::Reply current{};
photo::Reply handle(const photo::Request& request,std::uint64_t now) {
    current.session=session;current.epoch=epoch;current.observed_ms=now;
    auto rejected=[&](photo::Reason reason){auto reply=current;reply.command=request.command;reply.state=photo::State::rejected;reply.reason=reason;return reply;};
    if(!photo::valid(request))return rejected(photo::Reason::malformed);
    if(request.operation==photo::Operation::status)return current;
    if(request.session!=session)return rejected(photo::Reason::session);
    if(request.epoch!=epoch)return rejected(photo::Reason::epoch);
    current.command=request.command;current.reason=photo::Reason::none;
    if(request.operation==photo::Operation::enter) {
        current.native_photo=current.owned=current.scene_frozen=1;
        current.hud_hidden=request.hide_on_enter;current.state=photo::State::active;
    }else if(request.operation==photo::Operation::leave) {
        current.native_photo=current.owned=current.hud_hidden=current.scene_frozen=0;
        current.state=photo::State::left;
    }else {
        if(!current.owned)return rejected(photo::Reason::not_owned);
        current.hud_hidden=request.operation==photo::Operation::hide_hud;
        current.state=current.hud_hidden?photo::State::hud_hidden:photo::State::hud_visible;
    }
    return current;
}
}
int main(){current.state=photo::State::ready;current.reason=photo::Reason::none;photo::serve_photo_pipe(handle);return 1;}
