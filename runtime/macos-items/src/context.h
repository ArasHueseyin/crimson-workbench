#pragma once
#include <cstdint>
#include <limits>

namespace crimson::mac_items {
struct Observation { bool actor_matches{}, holder_matches{}; };
struct LocalActorObservation {
    std::uintptr_t manager_vtable{}, user_vtable{}, actor_vtable{};
    std::uint32_t client_key{}, server_key{};
    bool selection_valid{}, keys_match{};
};

// Only numeric identity leaves this snapshot; no engine pointer is retained.
template<class Read>
LocalActorObservation inspect_local_actor(std::uintptr_t server_actor, std::intptr_t slide, Read read) {
    LocalActorObservation result;
    std::uintptr_t root{}, manager{}, user{}, actor{}, key{}, check{};
    auto field = [&](std::uintptr_t object, std::uintptr_t offset, std::uintptr_t& value) {
        return object && object <= std::numeric_limits<std::uintptr_t>::max() - offset
            && read(object + offset, value);
    };
    if (!read(0x108a6a938ULL+slide, root) || !field(root, 0x10, manager)
        || !field(manager, 0, result.manager_vtable) || result.manager_vtable != 0x10833e4e0ULL+slide
        || !field(manager, 0x100, user) || !field(user, 0, result.user_vtable)
        || result.user_vtable != 0x10833e2c0ULL+slide
        || !field(user, 0x220, actor) || !field(actor, 0, result.actor_vtable)
        || result.actor_vtable != 0x10833cd88ULL+slide || !field(actor, 0x100, key)) return result;
    result.client_key = static_cast<std::uint32_t>(key);
    if (!field(user, 0x220, check) || check != actor
        || !field(manager, 0x100, check) || check != user
        || !field(root, 0x10, check) || check != manager
        || !read(0x108a6a938ULL+slide, check) || check != root) return result;
    result.selection_valid = result.client_key != 0;
    if (!field(server_actor, 0x100, key)) return result;
    result.server_key = static_cast<std::uint32_t>(key);
    result.keys_match = result.client_key && result.client_key == result.server_key;
    return result;
}

// No engine pointer survives the callback. This observation never permits a grant.
template<class Read>
Observation inspect_context(std::uintptr_t context, std::uintptr_t actor_vtable, Read read) {
    std::uintptr_t actor{}, vtable{}, component{}, holder{}, owner{};
    auto field = [&](std::uintptr_t object, std::uintptr_t offset, std::uintptr_t& value) {
        return object && object <= std::numeric_limits<std::uintptr_t>::max() - offset
            && read(object + offset, value);
    };
    if (!field(context, 0, actor) || !field(actor, 0, vtable) || vtable != actor_vtable) return {};
    const bool matches = field(actor, 0x108, component) && field(component, 0xb8, holder)
        && field(holder, 8, owner) && owner == actor;
    return {true, matches};
}
}
