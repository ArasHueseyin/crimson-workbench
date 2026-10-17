#pragma once
#include "position.h"
#include "marker_layout.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace crimson::marketplace::teleport {
inline constexpr std::uint32_t kMagic=0x54505743;
inline constexpr std::uint16_t kProtocol=1;
inline constexpr std::size_t kSavedSlots=16;
enum class Operation : std::uint32_t {status,save_current,remove_saved,travel_saved,travel_marker};
enum class State : std::uint32_t {waiting,ready,pending,captured,removed,native_queued,position_observed,rejected,uncertain,faulted};
enum class Reason : std::uint32_t {none,malformed,session,stale_context,command_order,busy,slot,name,unavailable,native_error,native_fault,publish_mismatch,timeout,context_changed,marker_height_or_world_unproven,
    marker_missing,marker_layer,marker_geometry,marker_placement,marker_changed,marker_world,marker_list};
enum Feature : std::uint32_t {save_current=1,travel_saved=2,travel_marker=4};
struct Request {
    std::uint32_t magic{kMagic};std::uint16_t protocol{kProtocol},bytes{sizeof(Request)};
    std::uint64_t session{},command{},epoch{},not_after_ms{};
    Operation operation{};std::uint32_t slot{};
    std::uint64_t marker{};
    // UTF-8 display name, maximum 47 bytes. No path or coordinates accepted.
    std::array<char,48> name{};
    std::uint32_t marker_source{},reserved{};
};
static_assert(sizeof(Request)==112&&offsetof(Request,name)==56&&offsetof(Request,marker_source)==104&&offsetof(Request,reserved)==108);
struct Bookmark {
    SavedPosition position{};std::array<char,48> name{};std::uint32_t occupied{},reserved{};
};
static_assert(sizeof(Bookmark)==88);
struct Reply {
    std::uint32_t magic{kMagic};std::uint16_t protocol{kProtocol},bytes{sizeof(Reply)};
    std::uint64_t session{},command{},epoch{},observed_ms{};
    State state{State::waiting};Reason reason{Reason::unavailable};
    std::uint32_t features{Feature::save_current|Feature::travel_saved},identity{},world{},native_reason{};
    std::int32_t native_error{};std::uint32_t storage_status{}; // 0 unknown, 1 stored, 2 unavailable
    std::uint64_t bookmarks_revision{};
    SavedPosition current{};
    std::array<Bookmark,kSavedSlots> bookmarks{};
    std::uint32_t marker_count{};MarkerCode marker_status{MarkerCode::unsupported};
    std::uint64_t markers_observed_ms{};
    std::array<MarkerValue,kMaximumMapPins+kMaximumWaypoints> markers{};
};
static_assert(sizeof(Reply)==14016&&offsetof(Reply,marker_count)==1520&&offsetof(Reply,marker_status)==1524&&
    offsetof(Reply,markers_observed_ms)==1528&&offsetof(Reply,markers)==1536&&std::is_trivially_copyable_v<Request>&&std::is_trivially_copyable_v<Reply>);
inline bool valid_name(const std::array<char,48>& name) noexcept {
    std::size_t length=0;
    while(length<name.size()&&name[length])++length;
    if(!length||length==name.size())return false;
    for(auto i=length;i<name.size();++i)if(name[i])return false;
    for(std::size_t i=0;i<length;){
        const auto lead=static_cast<std::uint8_t>(name[i++]);
        std::uint32_t code=lead,minimum=0;unsigned following=0;
        if(lead<0x80){}
        else if(lead>=0xc2&&lead<=0xdf){code=lead&0x1fu;minimum=0x80;following=1;}
        else if(lead>=0xe0&&lead<=0xef){code=lead&0x0fu;minimum=0x800;following=2;}
        else if(lead>=0xf0&&lead<=0xf4){code=lead&7u;minimum=0x10000;following=3;}
        else return false;
        if(length-i<following)return false;
        while(following--){
            const auto next=static_cast<std::uint8_t>(name[i++]);
            if((next&0xc0u)!=0x80u)return false;
            code=(code<<6)|(next&0x3fu);
        }
        if(code<minimum||code>0x10ffff||(code>=0xd800&&code<=0xdfff)||code<=0x1f||(code>=0x7f&&code<=0x9f))return false;
    }
    return true;
}
inline bool valid_request(const Request& r) noexcept {
    if(r.magic!=kMagic||r.protocol!=kProtocol||r.bytes!=sizeof(Request)||r.reserved||static_cast<std::uint32_t>(r.operation)>static_cast<std::uint32_t>(Operation::travel_marker))return false;
    if(r.operation==Operation::status)return !r.command&&!r.epoch&&!r.not_after_ms&&!r.slot&&!r.marker&&!r.marker_source&&r.name==std::array<char,48>{};
    if(!r.command||!r.epoch||!r.not_after_ms)return false;
    if(r.operation==Operation::travel_marker)return r.marker!=UINT64_MAX&&r.marker_source<=static_cast<std::uint32_t>(MarkerSource::waypoint)&&!r.slot&&r.name==std::array<char,48>{};
    if(r.slot>=kSavedSlots||r.marker||r.marker_source)return false;
    return r.operation==Operation::save_current ? valid_name(r.name) : r.name==std::array<char,48>{};
}
} // namespace crimson::marketplace::teleport
