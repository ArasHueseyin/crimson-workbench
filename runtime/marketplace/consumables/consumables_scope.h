#pragma once
#include <array>
#include <cstdint>
#include "consumables_settings.h"
#include "../stamina/stamina_scope.h"

namespace crimson::marketplace::consumables {
using Address = std::uintptr_t;
inline constexpr Address kItemManager = 0x6d69aa8;
inline constexpr Address kUseManager = 0x6d6f6c8;
inline constexpr Address kInventoryType = 0x5b2b760;
inline constexpr Address kSkillType = 0x59bf6b0;
inline constexpr Address kProjectileType = 0x59bf740;
inline constexpr Address kReserveType = 0x59bf3c0;
inline constexpr Address kDestroyType = 0x59bf8a8;
inline constexpr Address kSkillLookupReturn = 0x2b4aa10;
inline constexpr Address kArrowPopReturn = 0x2b4a2c4;

struct Owner { Address client{}, server{}, inventory{}; std::uint32_t identity{}; };
template<class Read> bool selected(Read& read, Address base, Owner& out) {
    using namespace crimson::marketplace::stamina;
    if (!base) return false;
    const auto cr = read.template at<Address>(base+kClientRoot);
    const auto sr = read.template at<Address>(base+kServerRoot);
    if (!cr || !sr) return false;
    const auto cm = read.template at<Address>(cr+0x30);
    const auto sm = read.template at<Address>(sr+0x48);
    if (!cm || !sm || read.template at<Address>(cm) != base+kClientManager ||
        read.template at<Address>(sm) != base+kServerManager) return false;
    const auto client = read.template at<Address>(cm+0x50);
    if (!client) return false;
    const auto identity = read.template at<std::uint32_t>(client+0x60);
    Player c{}, s{};
    if (!identity || !player(read,base,client,kClientActor,kClientStatus,identity,c)) return false;
    const auto count = read.template at<std::uint32_t>(sm+0x98);
    const auto buckets = read.template at<Address>(sm+0xa8);
    const auto nodes = read.template at<Address>(sm+0xb0);
    if (!count || count > 1'000'000 || !buckets || !nodes) return false;
    const auto key = identity & 0xfffff;
    const auto bucket = buckets+(key%count)*Address{0x100};
    const auto size = read.template at<std::uint32_t>(bucket);
    if (size > 31) return false;
    Address server{}, node{}; std::uint32_t index{};
    for (std::uint32_t i=0; i<size; ++i) {
        const auto entry = bucket+8+i*Address{8};
        if (read.template at<std::uint32_t>(entry) != key) continue;
        if (server) return false;
        index = read.template at<std::uint32_t>(entry+4);
        if (index >= 1'000'000) return false;
        node = read.template at<Address>(nodes+index*Address{8});
        if (!node || read.template at<std::uint32_t>(node+4) != key) return false;
        server = read.template at<Address>(node+8);
        if (!server) return false;
    }
    if (!player(read,base,server,kServerActor,kServerStatus,identity,s)) return false;
    const auto inventory = read.template at<Address>(s.components+0xb8);
    if (read.template at<Address>(inventory) != base+kInventoryType ||
        read.template at<Address>(inventory+8) != server) return false;
    if (!stable(read,c) || !stable(read,s) ||
        read.template at<Address>(base+kClientRoot) != cr ||
        read.template at<Address>(base+kServerRoot) != sr ||
        read.template at<Address>(cr+0x30) != cm || read.template at<Address>(sr+0x48) != sm ||
        read.template at<Address>(cm+0x50) != client ||
        read.template at<std::uint32_t>(sm+0x98) != count ||
        read.template at<Address>(sm+0xa8) != buckets || read.template at<Address>(sm+0xb0) != nodes ||
        read.template at<std::uint32_t>(bucket) != size ||
        read.template at<Address>(nodes+index*Address{8}) != node ||
        read.template at<std::uint32_t>(node+4) != key || read.template at<Address>(node+8) != server ||
        read.template at<Address>(s.components+0xb8) != inventory) return false;
    out = {client,server,inventory,identity}; return true;
}

template<class Read> Address row(Read& read, Address base, Address manager_rva,
                                std::uint32_t index, std::uint32_t limit) {
    const auto manager = read.template at<Address>(base+manager_rva);
    if (!manager) return 0;
    const auto count = read.template at<std::uint32_t>(manager+8);
    const auto rows = read.template at<Address>(manager+0x58);
    if (!count || count>limit || index>=count || !rows) return 0;
    const auto value = read.template at<Address>(rows+index*Address{8});
    // ItemInfo/ItemUseInfo +11..17 are padding, unlike StatusInfo's
    // meaningful kind/ordinal fields. Bind by the registered array handle;
    // callers also verify the actual semantic key and complete use family.
    if (!value || read.template at<std::uint8_t>(value+0x10) ||
        read.template at<Address>(base+manager_rva) != manager ||
        read.template at<Address>(manager+0x58) != rows ||
        read.template at<std::uint32_t>(manager+8) != count ||
        read.template at<Address>(rows+index*Address{8}) != value) return 0;
    return value;
}

struct Slot { Address address{}; std::uint64_t uid{}; std::uint16_t definition{}; std::int64_t amount{}; };
template<class Read> bool slot(Read& read, Address inventory, std::uint16_t handle,
                               std::uint16_t index, Slot& out) {
    if (!inventory || index>=32768 || handle==0xffff) return false;
    const auto array = read.template at<Address>(inventory+0x18);
    const auto count = read.template at<std::uint32_t>(inventory+0x20);
    if (!array || !count || count>128) return false;
    Address found{};
    for (std::uint32_t i=0; i<count; ++i) {
        const auto item = read.template at<Address>(array+i*Address{8});
        if (!item) return false;
        if (read.template at<std::uint16_t>(item+0x10) != handle) continue;
        if (found) return false;
        found = item;
    }
    if (!found) return false;
    const auto capacity = read.template at<std::int16_t>(found+0xc);
    const auto data = read.template at<Address>(found);
    if (capacity<=0 || index>=capacity || !data) return false;
    const auto stack_address = data+index*Address{0xc8};
    const auto uid = read.template at<std::uint64_t>(stack_address);
    const auto definition = read.template at<std::uint16_t>(stack_address+8);
    const auto amount = read.template at<std::int64_t>(stack_address+0x10);
    if (!uid || definition==0xffff || amount<=0 ||
        read.template at<Address>(inventory+0x18) != array ||
        read.template at<std::uint32_t>(inventory+0x20) != count ||
        read.template at<Address>(found) != data ||
        read.template at<std::int16_t>(found+0xc) != capacity) return false;
    out = {stack_address,uid,definition,amount}; return true;
}

struct UseRef { std::uint32_t index{}; Address info{}, data{}; std::uint8_t type{}, cost{}, frame{}; };
struct Proof {
    Address base{}, inventory{}, actor{}, count_pointer{}, source{}, definition{};
    Owner owner{}; Slot original_slot{}; const Item* item{};
    std::uint16_t inventory_handle{}, definition_handle{}, slot_index{};
    std::uint64_t requested{}, extra{};
    std::array<UseRef,kMaxUses> uses{};
    std::array<std::uint32_t,kMaxUses> requested_uses{};
    std::uint32_t use_count{};
};

template<class Read> bool definition(Read& read, Proof& proof) {
    const auto info = row(read,proof.base,kItemManager,proof.definition_handle,65535);
    if (!info || read.template at<std::uint32_t>(info) != proof.item->key ||
        read.template at<std::uint8_t>(info+0xa3) != proof.item->native_type) return false;
    const auto list = read.template at<Address>(info+0x80);
    const auto count = read.template at<std::uint32_t>(info+0x88);
    if (!list || !count || count!=proof.item->count || count>kMaxUses) return false;
    for (std::uint32_t i=0; i<count; ++i) {
        const auto index = read.template at<std::uint32_t>(list+i*Address{4});
        const auto use = row(read,proof.base,kUseManager,index,20000);
        if (!use || read.template at<std::uint32_t>(use) != proof.item->uses[i]) return false;
        const auto data = read.template at<Address>(use+0x18);
        if (proof.item->patterns[i]>=kUsePatterns.size()) return false;
        const auto& expected = kUsePatterns[proof.item->patterns[i]];
        const auto vt = use_vtable(expected.type);
        if (!data || !vt || read.template at<Address>(data) != proof.base+vt ||
            read.template at<std::uint8_t>(data+0x18) != expected.type ||
            read.template at<std::uint8_t>(data+0x19) != expected.frame ||
            read.template at<std::uint8_t>(data+0x24) != expected.cost ||
            read.template at<std::uint8_t>(data+0x25) != expected.gate25 ||
            read.template at<std::uint8_t>(data+0x26) != expected.gate26 ||
            read.template at<std::uint8_t>(data+0x27) != expected.flag27 ||
            read.template at<std::uint8_t>(data+0x28) != expected.flag28 ||
            read.template at<std::uint32_t>(data+0x20) != expected.parameter ||
            read.template at<std::uint32_t>(data+0x10) != expected.conditions ||
            (expected.conditions && !read.template at<Address>(data+8))) return false;
        // The Skill handler only borrows its original array during this call.
        // The native virtual eligibility path still evaluates these original
        // conditions before dispatch; neither conditions nor skills are copied
        // to an invented definition or modified by this policy.
        if (expected.type==0 && (!read.template at<Address>(data+0x30) ||
            read.template at<std::uint32_t>(data+0x38)!=expected.skills)) return false;
        proof.uses[i] = {index,use,data,expected.type,expected.cost,expected.frame};
    }
    proof.definition = info; return true;
}

template<class Read> bool requested_uses(Read& read, Proof& proof) {
    auto vector = read.template at<Address>(proof.source);
    if (!vector) vector = read.template at<Address>(proof.source+8);
    if (!vector) return false;
    const auto data = read.template at<Address>(vector);
    const auto count = read.template at<std::uint32_t>(vector+8);
    if (!data || !count || count>proof.item->count) return false;
    for (std::uint32_t i=0; i<count; ++i) {
        const auto handle = read.template at<std::uint32_t>(data+i*Address{4});
        bool found = false;
        for (std::uint32_t j=0; j<proof.item->count; ++j) if (proof.uses[j].index==handle) found=true;
        for (std::uint32_t j=0; j<i; ++j) if (proof.requested_uses[j]==handle) return false;
        if (!found) return false;
        proof.requested_uses[i] = handle;
    }
    proof.use_count = count; return true;
}

template<class Read> bool begin(Read& read, Address base, const Settings& settings,
    Address inventory, Address actor, std::uint16_t inventory_handle, Address count_pointer,
    std::uint16_t definition_handle, Address source, std::uint16_t slot_index,
    std::uint64_t extra, Proof& proof) {
    if (!settings.count || !inventory || !actor || !count_pointer || !source) return false;
    Owner owner{}; Slot stack{};
    if (!selected(read,base,owner) || owner.server!=actor || owner.inventory!=inventory ||
        !slot(read,inventory,inventory_handle,slot_index,stack) || stack.definition!=definition_handle) return false;
    const auto requested = read.template at<std::int64_t>(count_pointer);
    const auto info = row(read,base,kItemManager,definition_handle,65535);
    if (!info || requested<=0 || requested>stack.amount) return false;
    const auto item = supported(read.template at<std::uint32_t>(info));
    if (!item || !settings.contains(item->key)) return false;
    proof.base=base; proof.inventory=inventory; proof.actor=actor; proof.count_pointer=count_pointer;
    proof.source=source; proof.owner=owner; proof.original_slot=stack; proof.item=item;
    proof.inventory_handle=inventory_handle; proof.definition_handle=definition_handle; proof.slot_index=slot_index;
    proof.requested=static_cast<std::uint64_t>(requested); proof.extra=extra;
    return definition(read,proof) && requested_uses(read,proof);
}

template<class Read> bool fresh(Read& read, const Proof& proof) {
    if (!proof.item) return false;
    Owner owner{}; Slot stack{};
    if (!selected(read,proof.base,owner) || owner.client!=proof.owner.client ||
        owner.server!=proof.owner.server || owner.inventory!=proof.inventory || owner.identity!=proof.owner.identity ||
        !slot(read,proof.inventory,proof.inventory_handle,proof.slot_index,stack) ||
        stack.address!=proof.original_slot.address || stack.uid!=proof.original_slot.uid ||
        stack.definition!=proof.definition_handle || stack.amount<static_cast<std::int64_t>(proof.requested) ||
        read.template at<std::uint64_t>(proof.count_pointer)!=proof.requested) return false;
    Proof check = proof;
    if (!definition(read,check) || check.definition!=proof.definition || !requested_uses(read,check) ||
        check.use_count!=proof.use_count) return false;
    for (std::uint32_t i=0; i<proof.item->count; ++i)
        if (check.uses[i].index!=proof.uses[i].index || check.uses[i].info!=proof.uses[i].info ||
            check.uses[i].data!=proof.uses[i].data) return false;
    for (std::uint32_t i=0; i<proof.use_count; ++i)
        if (check.requested_uses[i]!=proof.requested_uses[i]) return false;
    return true;
}
inline const UseRef* requested(const Proof& proof, std::uint32_t index) {
    bool found = false;
    for (std::uint32_t i=0; i<proof.use_count; ++i) if (proof.requested_uses[i]==index) found=true;
    if (found && proof.item) for (std::uint32_t i=0; i<proof.item->count; ++i)
        if (proof.uses[i].index==index) return &proof.uses[i];
    return nullptr;
}
} // namespace crimson::marketplace::consumables
