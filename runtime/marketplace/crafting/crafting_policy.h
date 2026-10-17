#pragma once
#include <Windows.h>
#include <cstring>
#include "crafting_scope.h"

namespace crimson::marketplace::crafting {
template<class Read>
Edit decide(Read& read, Address base, Site site, const Context& context) {
    if (!base || static_cast<std::size_t>(site) >= kSiteCount) return {};
    const auto continuation = base+kTargets[static_cast<std::size_t>(site)].continuation;
    Recipe selected;
    if (site == Site::Database) {
        if (!address(context.rbx,2) || !recipe(read,base,read.template at<std::uint16_t>(context.rbx),selected) ||
            selected.row != context.rax || !stable(read,base,selected)) return {};
        return {continuation};
    }
    if (site == Site::Client) {
        if (context.rcx != 0 || context.rax != 0 || !address(context.rbp,0xac8) || context.rbp < 0x10080 ||
            read.template at<std::uint32_t>(context.rbp+0x38) != 0 ||
            read.template at<std::uint32_t>(context.rbp-0x78) != 0 ||
            !recipe(read,base,read.template at<std::uint16_t>(context.rbp+0xa80),selected) || !stable(read,base,selected)) return {};
        // Continue before native 2429370. Conditions, knowledge and request
        // matching are neither replaced nor reported as successful here.
        return {continuation};
    }
    if (site == Site::KnowledgeA || site == Site::KnowledgeB) {
        if ((context.rbx & 0xffff) != 0xffff || !address(context.r14,16) ||
            read.template at<std::uint32_t>(context.r14+8) == 0) return {};
        const auto handles = read.template at<Address>(context.r14);
        if (!address(handles,2) || !recipe(read,base,read.template at<std::uint16_t>(handles),selected) ||
            selected.row != context.rax || selected.need_knowledge != 0xffff || !stable(read,base,selected)) return {};
        const auto fallback = knowledge_fallback(selected.known->key);
        if (!fallback) return {};
        ResolvedKnowledge display;
        const auto handle = knowledge(read,base,fallback->knowledge,display) ? display.handle : std::uint16_t{0xffff};
        // A temporarily unavailable display definition keeps its invalid
        // display handle and skips the unsafe empty-first-material dereference.
        // The actual recipe knowledge and native prerequisite validator stay intact.
        if (!stable(read,base,selected)) return {};
        return {continuation,true,handle};
    }
    // Native quantity routine has already acquired a live actor reference at
    // [RBP-80]. Bind it to the current selected player and its crafting component.
    if (site != Site::Quantity || context.r13 != 0 || !address(context.r15,0x194) ||
        context.rbp < 0x10080 || read.template at<std::uint32_t>(context.r15+0x190) != 0) return {};
    constexpr Address root_rva=0x6d69190, manager_vtable=0x55b62c8, actor_vtable=0x5585c40;
    const auto root = read.template at<Address>(base+root_rva);
    if (!address(root,0x38)) return {};
    const auto manager = read.template at<Address>(root+0x30);
    if (!address(manager,0x58) || read.template at<Address>(manager) != base+manager_vtable) return {};
    const auto actor = read.template at<Address>(manager+0x50);
    if (!address(actor,0xa8) || actor != read.template at<Address>(context.rbp-0x80) ||
        read.template at<Address>(actor) != base+actor_vtable || read.template at<std::uint8_t>(actor+0x96) != 1) return {};
    const auto identity = read.template at<std::uint32_t>(actor+0x60);
    const auto components = read.template at<Address>(actor+0x68);
    const auto count = read.template at<std::uint32_t>(actor+0x70);
    const auto metadata = read.template at<Address>(actor+0x88);
    const auto possessor = read.template at<Address>(actor+0xa0);
    if (!identity || !address(components,0x170) || count < 46 || count > 57 || !address(metadata,2) ||
        !address(possessor,0xd8) || read.template at<Address>(possessor+0xd0) != actor) return {};
    const auto kind = read.template at<std::uint8_t>(metadata+1);
    if (kind != 1 && kind != 4) return {};
    const auto crafting = read.template at<Address>(components+0x168);
    if (!address(crafting,0x34e) || read.template at<Address>(crafting+8) != actor ||
        read.template at<std::uint32_t>(crafting+0x2f8) != 0 ||
        read.template at<std::uint32_t>(crafting+0x308) != 0 ||
        !recipe(read,base,read.template at<std::uint16_t>(crafting+0x2e8),selected)) return {};
    if (!stable(read,base,selected) || read.template at<Address>(base+root_rva) != root ||
        read.template at<Address>(root+0x30) != manager || read.template at<Address>(manager+0x50) != actor ||
        read.template at<Address>(manager) != base+manager_vtable || read.template at<Address>(actor) != base+actor_vtable ||
        read.template at<Address>(context.rbp-0x80) != actor || read.template at<std::uint32_t>(actor+0x60) != identity ||
        read.template at<std::uint8_t>(actor+0x96) != 1 || read.template at<Address>(actor+0x68) != components ||
        read.template at<std::uint32_t>(actor+0x70) != count || read.template at<Address>(actor+0x88) != metadata ||
        read.template at<std::uint8_t>(metadata+1) != kind || read.template at<Address>(actor+0xa0) != possessor ||
        read.template at<Address>(possessor+0xd0) != actor || read.template at<Address>(components+0x168) != crafting ||
        read.template at<Address>(crafting+8) != actor || read.template at<std::uint16_t>(crafting+0x2e8) != selected.handle ||
        read.template at<std::uint32_t>(crafting+0x2f8) != 0 || read.template at<std::uint32_t>(crafting+0x308) != 0 ||
        read.template at<std::uint32_t>(context.r15+0x190) != 0) return {};
    return {continuation,false,0,true,kEmptyQuantityMaximum};
}
struct Memory {
    template<class T> T at(Address at) const {
        static_assert(std::is_trivially_copyable_v<T>);
        if constexpr (std::is_integral_v<T>) return *reinterpret_cast<const volatile T*>(at);
        else {
            T value;
            const auto source=reinterpret_cast<const volatile unsigned char*>(at);
            const auto destination=reinterpret_cast<unsigned char*>(&value);
            for(std::size_t i=0;i<sizeof(value);++i)destination[i]=source[i];
            return value;
        }
    }
};
inline Edit guarded_decide(Address base, Site site, const Context& context) noexcept {
    __try { Memory memory; return decide(memory,base,site,context); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
}
} // namespace crimson::marketplace::crafting
