#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
namespace crimson::marketplace::photo {
inline constexpr std::uint32_t kMagic=0x48505743;
inline constexpr std::uint16_t kProtocol=1;
enum class Operation:std::uint32_t {status,enter,hide_hud,show_hud,leave};
enum class State:std::uint32_t {waiting,ready,pending,native_queued,active,hud_hidden,hud_visible,left,rejected,uncertain};
enum class Reason:std::uint32_t {none,malformed,session,epoch,stale,busy,order,unavailable,foreign_photo,camera_busy,not_owned,context_changed,timeout};
struct Request {
    std::uint32_t magic{kMagic};std::uint16_t protocol{kProtocol},bytes{sizeof(Request)};
    std::uint64_t session{},command{},epoch{},not_after_ms{};
    Operation operation{};std::uint32_t hide_on_enter{1};std::uint64_t reserved{};
};
struct Reply {
    std::uint32_t magic{kMagic};std::uint16_t protocol{kProtocol},bytes{sizeof(Reply)};
    std::uint64_t session{},command{},epoch{},observed_ms{};
    State state{State::waiting};Reason reason{Reason::unavailable};
    std::uint32_t native_photo{},owned{},hud_hidden{},scene_frozen{};
};
static_assert(sizeof(Request)==56&&sizeof(Reply)==64&&std::is_trivially_copyable_v<Request>&&std::is_trivially_copyable_v<Reply>);
inline bool valid(const Request& r) noexcept {
    if(r.magic!=kMagic||r.protocol!=kProtocol||r.bytes!=sizeof(Request)||r.reserved||r.hide_on_enter>1||
        static_cast<std::uint32_t>(r.operation)>static_cast<std::uint32_t>(Operation::leave))return false;
    if(r.operation==Operation::status)return !r.command&&!r.epoch&&!r.not_after_ms;
    return r.command&&r.epoch&&r.not_after_ms&&(r.operation==Operation::enter||r.hide_on_enter==1);
}
}
