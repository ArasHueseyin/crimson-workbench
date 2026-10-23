#pragma once
#include "execute.h"
#include <algorithm>

namespace crimson::mac_items {
struct StableEntry {
    std::uint64_t id{};
    std::uint16_t type{};
    bool operator==(const StableEntry&) const = default;
};
template<class Read>
std::optional<std::vector<StableEntry>> read_stable(std::uintptr_t manager, Read read) {
    std::uintptr_t entries{}, count{};
    if (!manager || !read(manager+0x18,entries) || !read(manager+0x20,count)) return {};
    count &= 0xffffffff;
    if (count>4096 || (count&&!entries)) return {};
    std::vector<StableEntry> out;
    for (std::uintptr_t i=0;i<count;++i) {
        std::uintptr_t record{}, type{}, id{};
        if (!read(entries+i*8,record)) return {};
        if (!record) continue;
        if (!read(record+0x20,type) || !read(record+0x28,id) || !id || id==UINT64_MAX) return {};
        if (std::any_of(out.begin(),out.end(),[&](const auto& entry){return entry.id==id;})) return {};
        out.push_back({id,static_cast<std::uint16_t>(type)});
    }
    return out;
}
template<class Read>
std::optional<std::uint16_t> resolve_mount(std::uint32_t key,std::intptr_t slide,Read read) {
    std::uintptr_t manager{}, count{}, entries{};
    if (!read(0x1085a7b90ULL+slide,manager) || !manager || !read(manager+8,count)
        || !read(manager+0x58,entries) || !entries) return {};
    count &= 0xffffffff;
    if (!count || count>65535) return {};
    for (std::uintptr_t i=0;i<count;++i) {
        std::uintptr_t record{}, external{}, category{};
        if (!read(entries+i*8,record)) return {};
        if (!record) continue;
        if (!read(record,external)) return {};
        if (static_cast<std::uint32_t>(external)!=key) continue;
        if (!read(record+0x68,category) || (category&0xff)<4 || (category&0xff)>6) return {};
        return static_cast<std::uint16_t>(i);
    }
    return {};
}
template<class Grant,class Recheck,class Read>
Execution execute_mount(Grant grant,Recheck recheck,std::uintptr_t actor,std::uintptr_t manager,
    std::uint32_t key,std::intptr_t slide,Read read) {
    using crimson::live_items::State;
    using crimson::live_items::Error;
    std::uintptr_t owner{};
    const auto type=resolve_mount(key,slide,read);
    if (!type) return {State::rejected,Error::unsupported_item};
    if (!actor || !manager || !read(manager+8,owner) || owner!=actor) return {State::rejected,Error::context};
    const auto before=read_stable(manager,read);
    if (!before || !recheck()) return {State::rejected,Error::context};
    if (std::any_of(before->begin(),before->end(),[&](const auto& entry){return entry.type==*type;}))
        return {State::rejected,Error::mount_owned};
    const auto result=grant(manager,*type);
    const auto after=read_stable(manager,read);
    if (result || !after || !recheck() || after->size()!=before->size()+1)
        return {State::uncertain,Error::inventory_mismatch};
    for (const auto& entry:*before)
        if (std::find(after->begin(),after->end(),entry)==after->end())
            return {State::uncertain,Error::inventory_mismatch};
    unsigned added=0;
    for (const auto& entry:*after)
        if (std::find(before->begin(),before->end(),entry)==before->end()) {
            if (entry.type!=*type) return {State::uncertain,Error::inventory_mismatch};
            ++added;
        }
    return added==1?Execution{State::applied,Error::none}:Execution{State::uncertain,Error::inventory_mismatch};
}
}
