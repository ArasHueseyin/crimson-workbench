#include "repair_reader.h"
#include "reader_fixture.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace rd = crimson::repair::reader;
namespace act = crimson::repair::action;
namespace {
using namespace repair_test_fixture;
namespace ref = crimson::repair::reference;
unsigned checks = 0;
void require(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
void happy_path() {
    Fixture f;
    const auto before = f.memory.blocks;
    rd::Capture out;
    require(f.capture(out) == rd::Code::captured, "Resolve and capture both realms");
    require(out.presentation.actor == f.client.actor && out.authority.actor == f.server.actor, "Actor identities");
    require(out.snapshot.items.size() == 2, "Only carried/equipped existing items");
    require(out.snapshot.session == act::Session{7, f.handle, 9}, "Session binds full character handle");
    require(out.bytes_read < 16384 && out.reads < 150, "Bounded selective traversal including re-read");
    act::Plan plan;
    require(act::prepare({out.snapshot.session, act::Scope::all, {}}, out.snapshot, plan) == act::Code::prepared &&
        plan.count() == act::Count{2, 2, 1}, "Reader feeds own repair action");
    require(act::apply_to_owned_snapshot(plan, out.snapshot) == act::Code::applied, "Repair only detached images");
    require(f.memory.blocks == before, "Complete pipeline never writes source memory");
    for (auto* actor : {&f.client, &f.server}) {
        const auto excluded = f.memory.allocate(12);
        f.memory.put(excluded, 0, std::uint16_t(10));
        f.memory.put(actor->bag, 0x20, excluded);
        f.memory.put(actor->bag, 0x28, std::uint32_t(1));
    }
    require(f.capture(out) == rd::Code::captured && out.snapshot.items.size() == 1,
        "Engine-filtered inventory types excluded in both realms");
    f.memory.put(f.client.descriptor, 1, std::uint8_t(4));
    f.memory.put(f.server.descriptor, 1, std::uint8_t(4));
    require(f.capture(out) == rd::Code::captured, "Anchored alternate protagonist class");
}
void refusals() {
    struct Case { const char* name; rd::Code expected; std::function<void(Fixture&)> change; };
    const std::vector<Case> cases{
        {"Title screen", rd::Code::no_player, [](auto& f) { f.memory.put(f.client_manager, 0x50, std::uintptr_t(0)); }},
        {"No authority loaded", rd::Code::no_authority, [](auto& f) { f.memory.put(f.server_manager, 0x9c, std::uint32_t(0)); }},
        {"Different generation, same low key", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.server.actor, 0x60, f.handle + 0x100000); }},
        {"No possessor backref", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.server.possessor, 0xd0, f.client.actor); }},
        {"Foreign holder owner", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.server.holder, 8, f.client.actor); }},
        {"NPC class", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.client.descriptor, 1, std::uint8_t(3)); }},
        {"Same actor in both realms", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.node, 8, f.client.actor); }},
        {"Duplicate registry key", rd::Code::ambiguous, [](auto& f) { f.memory.put(f.hash, 0, std::uint32_t(2)); f.memory.put(f.hash, 16, f.handle & 0xfffffu); }},
        {"Registry bucket overflow", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.hash, 0, std::uint32_t(32)); }},
        {"Registry index overflow", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.hash, 12, std::uint32_t(65536)); }},
        {"Wrong registry node key", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.node, 4, std::uint32_t(123)); }},
        {"Absent registry key", rd::Code::no_authority, [](auto& f) { f.memory.put(f.hash, 8, std::uint32_t(123)); }},
        {"Unreadable item data", rd::Code::unreadable, [](auto& f) { f.memory.put(f.client.bag, 0, UINTPTR_MAX); }},
        {"Old inventory layout", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.client.bag, 0xc, std::uint16_t(32768)); }},
        {"Oversized bag", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.client.bag, 0xc, std::uint16_t(2049)); }},
        {"Missing Character bag", rd::Code::no_player, [](auto& f) { f.memory.put(f.client.bag, 0x10, std::uint16_t(9)); }},
        {"Moved item", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.client.slots, 0xc8, std::uint16_t(4)); }},
        {"Different item at same position", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.client.bag_items, 0, std::uint64_t(111)); }},
        {"Consumed in only one realm", rd::Code::identity_mismatch, [](auto& f) { f.memory.put(f.client.bag_items, 0x10, std::int64_t(0)); }},
        {"Duplicate item instance", rd::Code::ambiguous, [](auto& f) { f.memory.put(f.client.slots, 0, std::uint64_t(11)); }},
        {"Changed durability mirror", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.client.bag_items, 0x40, std::uint16_t(36)); }},
        {"Too many sockets", rd::Code::invalid_layout, [](auto& f) { f.memory.put(f.client.bag_items, 0x70, std::uint8_t(65)); }},
        {"Missing socket bytes", rd::Code::unreadable, [](auto& f) { f.memory.blocks[f.client.sockets].resize(6); }},
        {"Missing catalog entry", rd::Code::invalid_layout, [](auto& f) { f.definitions.pop_back(); }},
    };
    for (const auto& c : cases) {
        Fixture f;
        rd::Capture out;
        require(f.capture(out) == rd::Code::captured, "Prime successful capture");
        c.change(f);
        const auto before = f.memory.blocks;
        require(f.capture(out) == c.expected, c.name);
        require(out.snapshot.items.empty() && out.authority.actor == 0 && out.presentation.actor == 0,
            "Failure clears prior output and addresses");
        require(f.memory.blocks == before, "Failure is read-only");
    }
    for (int which = 0; which < 5; ++which) {
        Fixture f;
        f.memory.change_on_verify = std::array<std::uintptr_t, 5>{f.client_manager + 0x50,
            f.server.actor + 0x60, f.client.bag_items, f.server.sockets, f.hash}[which];
        if (which == 1) f.memory.change_after = 3; // registry and character both check the full handle
        rd::Capture out;
        require(f.capture(out) == rd::Code::changed_during_read && out.snapshot.items.empty(),
            "Full read set detects concurrent pointer, item, socket and registry changes");
    }
    Fixture f;
    rd::Capture out;
    f.frame.world_epoch = 0;
    require(f.capture(out) == rd::Code::invalid_frame, "No inferred world epoch");
}
void lookups() {
    Fixture f;
    std::uintptr_t address = UINTPTR_MAX;
    require(rd::inventory_slot(f.memory, f.client.holder, 2, 0, address) == rd::Code::captured &&
        address == f.client.bag_items, "Slot lookup");
    f.item(f.client.bag_items + 2 * act::item_size, 12, 20, 1, 10);
    require(rd::inventory_slot(f.memory, f.client.holder, 2, 2, address) == rd::Code::captured &&
        address == f.client.bag_items + 2 * act::item_size, "Correct contiguous stride and +0xc bound");
    for (const auto slot : {std::int16_t(-1), std::int16_t(1), std::int16_t(3)}) {
        address = UINTPTR_MAX;
        require(rd::inventory_slot(f.memory, f.client.holder, 2, slot, address) == rd::Code::captured &&
            !address, "Empty/out-of-bounds slot");
    }
    require(rd::inventory_slot(f.memory, f.client.holder, 99, 0, address) == rd::Code::captured && !address,
        "Absent container");
}
void registry_and_limits() {
    Fixture f;
    // The high generation bits do not participate in bucket selection. Do not
    // accidentally treat map used-count as the backing array's highest index.
    f.handle = 0x60112346;
    f.memory.put(f.client.actor, 0x60, f.handle);
    f.memory.put(f.server.actor, 0x60, f.handle);
    const auto key = f.handle & 0xfffffu;
    const auto buckets = f.memory.allocate(3 * 256);
    const auto entries = f.memory.allocate(4 * sizeof(std::uintptr_t));
    const auto selected = buckets + (key % 3) * std::size_t(256);
    f.memory.put(selected, 0, std::uint32_t(31));
    f.memory.put(selected, 248, key);
    f.memory.put(selected, 252, std::uint32_t(3));
    f.memory.put(entries, 24, f.node);
    f.memory.put(f.node, 4, key);
    f.memory.put(f.server_manager, 0x98, std::uint32_t(3));
    f.memory.put(f.server_manager, 0xa8, buckets);
    f.memory.put(f.server_manager, 0xb0, entries);
    rd::Capture out;
    require(f.capture(out) == rd::Code::captured && out.snapshot.session.player == f.handle,
        "Modulo lookup, full handle, final bucket entry and nonzero node index");
    for (auto* actor : {&f.client, &f.server}) {
        const auto items = f.memory.allocate(act::max_items * act::item_size);
        actor->bag_items = items;
        f.memory.put(actor->bag, 0, items);
        f.memory.put(actor->bag, 0xc, static_cast<std::uint16_t>(act::max_items));
        for (std::size_t i = 0; i < act::max_items; ++i)
            f.item(items + i * act::item_size, i + 1000, 10, 1, 35);
        f.item(actor->slots, UINT64_MAX, act::absent, 0, 0);
    }
    const auto before = f.memory.blocks;
    require(f.capture(out) == rd::Code::captured && out.snapshot.items.size() == act::max_items,
        "Exactly 2048 existing items across both realms");
    require(out.bytes_read < 2 * 1024 * 1024 && out.reads < 10000 && f.memory.blocks == before,
        "Large capture remains bounded and read-only");
    for (auto* actor : {&f.client, &f.server}) f.item(actor->slots, 22, 20, 1, 0);
    require(f.capture(out) == rd::Code::budget_exceeded && out.snapshot.items.empty(),
        "Total inventory plus equipment budget rejects whole capture");
}
void local_memory() {
    rd::LocalMemory memory;
    std::array<std::uint8_t, 16> original{}, copied{};
    original.fill(42);
    require(memory.copy(reinterpret_cast<std::uintptr_t>(original.data()), copied) && original == copied,
        "Read caller-owned local memory");
    auto* region = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!region) throw std::runtime_error("Allocate local fixture");
    DWORD old;
    if (!VirtualProtect(region + 4096, 4096, PAGE_NOACCESS, &old)) throw std::runtime_error("Protect local fixture");
    const bool inaccessible = memory.copy(reinterpret_cast<std::uintptr_t>(region + 4096), copied);
    const bool crosses = memory.copy(reinterpret_cast<std::uintptr_t>(region + 4090), copied);
    const bool null = memory.copy(0, copied);
    const bool overflow = memory.copy(UINTPTR_MAX - 2, copied);
    VirtualFree(region, 0, MEM_RELEASE);
    require(!inaccessible && !crosses && !null && !overflow, "Protected/cross-page/null/overflow reads refused");
}
void locked_captures() {
    for (int scenario = 0; scenario < 13; ++scenario) {
        Fixture f;
        std::array<OwnerLock, 2> locks;
        rd::PinnedOwners owners;
        owners.actors = {f.client.actor, f.server.actor};
        owners.expected_handle = f.handle;
        for (std::size_t i = 0; i < locks.size(); ++i) {
            owners.locks[i] = {&locks[i], try_owner_lock, release_owner_lock};
            f.memory.put(owners.actors[i], 8, reinterpret_cast<std::uintptr_t>(&locks[i]));
        }
        f.memory.guarded_address = f.server.bag_items;
        f.memory.owner_depth_a = &locks[0].depth;
        f.memory.owner_depth_b = &locks[1].depth;
        rd::Code expected = rd::Code::captured;
        if (scenario == 1) { locks[1].blocked = true; expected = rd::Code::lock_busy; }
        if (scenario == 2) {
            f.memory.put(f.client.actor, 8, reinterpret_cast<std::uintptr_t>(&locks[1]));
            expected = rd::Code::invalid_lock_binding;
        }
        if (scenario == 3) { f.memory.put(f.client.bag, 0xc, std::uint16_t(0xffff)); expected = rd::Code::invalid_layout; }
        if (scenario == 4) {
            locks[1].fixture = &f; locks[1].change_owner = f.server.actor;
            expected = rd::Code::invalid_lock_binding;
        }
        if (scenario == 5) {
            std::swap(owners.actors[0], owners.actors[1]); std::swap(owners.locks[0], owners.locks[1]);
            expected = rd::Code::identity_mismatch;
        }
        if (scenario == 6) { f.frame.world_epoch = 0; expected = rd::Code::invalid_frame; }
        if (scenario == 7) {
            f.memory.put(f.client_manager, 0x50, f.server.actor); expected = rd::Code::identity_mismatch;
        }
        if (scenario == 8) {
            owners.locks[1] = owners.locks[0];
            f.memory.put(f.server.actor, 8, reinterpret_cast<std::uintptr_t>(&locks[0]));
            f.memory.owner_depth_b = &locks[0].depth;
        }
        if (scenario == 9) { owners.expected_handle = 0; expected = rd::Code::invalid_lock_binding; }
        if (scenario == 10) { f.memory.put(f.client.actor, 0x4a, std::uint8_t(0)); expected = rd::Code::owner_unavailable; }
        if (scenario == 11) { f.memory.put(f.server.actor, 0x4b, std::uint8_t(1)); expected = rd::Code::owner_unavailable; }
        if (scenario == 12) { owners.expected_handle ^= 0x100000u; expected = rd::Code::owner_unavailable; }
        rd::Capture output;
        output.authority.actor = UINTPTR_MAX; // failures must clear stale output
        const auto before = f.memory.blocks;
        require(rd::capture_with_locks(f.memory, f.frame, f.definitions, owners, output) == expected,
            "Locked capture validates identity, ownership and frame");
        require(!locks[0].depth && !locks[1].depth, "Every capture path releases its locks");
        if (expected == rd::Code::captured) {
            require(output.snapshot.items.size() == 2 && f.memory.reads[f.server.bag_items] >= 2,
                "Item capture and verification both run under owner locks");
            require(locks[0].releases == 1 && locks[1].releases == unsigned(scenario != 8),
                "One release per distinct owner lock");
        } else require(output.snapshot.items.empty() && !output.authority.actor, "No stale output after failed locked capture");
        if (scenario == 2 || scenario == 6 || scenario == 9)
            require(!locks[0].attempts && !locks[1].attempts, "Invalid input never calls native locks");
        if (scenario != 4) require(f.memory.blocks == before, "Locked capture does not modify source items");
    }
}

void reference_captures() {
    for (int scenario = 0; scenario < 28; ++scenario) {
        Fixture f;
        std::array<OwnerLock, 2> locks;
        std::array<ReferenceSource, 2> contexts;
        std::array<ref::Source, 2> sources;
        std::array<crimson::repair::lease::Binding, 2> bindings;
        const std::array actors{f.client.actor, f.server.actor};
        for (std::size_t i = 0; i < 2; ++i) {
            contexts[i].actor = actors[i]; contexts[i].handle = f.handle; contexts[i].locks = &locks;
            sources[i] = {&contexts[i], f.handle, acquire_reference, release_reference};
            bindings[i] = {&locks[i], try_owner_lock, release_owner_lock};
            locks[i].reference_depth = &contexts[i].depth;
            f.memory.put(actors[i], 8, reinterpret_cast<std::uintptr_t>(&locks[i]));
            f.memory.put(actors[i], 0x4a, std::uint8_t(1));
        }
        rd::Code expected = rd::Code::captured;
        if (scenario == 1) { contexts[0].available = false; expected = rd::Code::owner_unavailable; }
        if (scenario == 2) { contexts[1].available = false; expected = rd::Code::owner_unavailable; }
        if (scenario == 3) { locks[1].blocked = true; expected = rd::Code::lock_busy; }
        if (scenario == 4) { f.memory.put(f.server.actor, 0x4a, std::uint8_t(0)); expected = rd::Code::owner_unavailable; }
        if (scenario == 5) { f.memory.put(f.client.actor, 0x4b, std::uint8_t(1)); expected = rd::Code::owner_unavailable; }
        if (scenario == 6) {
            f.memory.change_on_verify = f.server.actor + 0x4a; // destruction begins during capture
            expected = rd::Code::owner_unavailable;
        }
        if (scenario == 7) { f.memory.put(f.server.actor, 0x60, f.handle | 0x100000u); expected = rd::Code::owner_unavailable; }
        if (scenario == 8) { sources[1].handle ^= 0x100000u; expected = rd::Code::invalid_reference_source; }
        if (scenario == 9) { sources[1].release = nullptr; expected = rd::Code::invalid_reference_source; }
        if (scenario == 10) { f.frame.world_epoch = 0; expected = rd::Code::invalid_frame; }
        if (scenario == 11) { contexts[1].actor = contexts[0].actor; expected = rd::Code::owner_unavailable; }
        // A held reference is sufficient to keep an unregistered actor alive.
        // The reader must not search any unprotected registry, even on errors.
        for (const auto address : {f.frame.image_base + rd::server_context_rva,
            f.server_context, f.server_manager, f.hash, f.nodes, f.node})
            f.memory.forbidden.emplace_back(address, f.memory.blocks.at(address).size());
        if (scenario == 12) {
            f.memory.put(f.node, 8, std::uintptr_t(0x300000000));
            f.memory.forbidden.emplace_back(0x300000000, 0x1000);
        }
        if (scenario == 13) {
            f.memory.put(f.client_manager, 0x50, std::uintptr_t(0x300000000));
            f.memory.forbidden.emplace_back(0x300000000, 0x1000);
            expected = rd::Code::identity_mismatch;
        }
        if (scenario == 14) {
            f.memory.change_on_verify = f.client_manager + 0x50;
            expected = rd::Code::changed_during_read;
        }
        if (scenario == 15) {
            f.memory.change_on_verify = f.frame.image_base + rd::client_context_rva;
            expected = rd::Code::changed_during_read;
        }
        if (scenario == 16) {
            f.memory.change_on_verify = f.client_context + 0x30;
            expected = rd::Code::changed_during_read;
        }
        if (scenario == 17) {
            f.memory.change_on_verify = f.server.actor + 0x60;
            expected = rd::Code::identity_mismatch;
        }
        if (scenario == 18) {
            f.memory.put(f.client_manager, 0x50, std::uintptr_t(0));
            expected = rd::Code::no_player;
        }
        if (scenario == 19) {
            f.memory.put(f.server.possessor, 0xd0, f.client.actor);
            expected = rd::Code::identity_mismatch;
        }
        if (scenario == 20) {
            f.memory.change_on_verify = f.server.bag_items;
            expected = rd::Code::changed_during_read;
        }
        if (scenario == 21) {
            f.memory.blocks.erase(f.server.sockets);
            expected = rd::Code::unreadable;
        }
        if (scenario == 22) {
            f.memory.change_on_verify = f.server.actor + 8;
            f.memory.change_after = 3; // the final binding check
            expected = rd::Code::invalid_lock_binding;
        }
        if (scenario == 23) {
            for (const auto actor : actors) f.memory.put(actor, 0x60, f.handle | 0x100000u);
            expected = rd::Code::owner_unavailable;
        }
        if (scenario == 24) f.frame.build = rd::Frame::Build::steam_25455892;
        if (scenario == 25 || scenario == 26) {
            f.frame.build = scenario == 25 ? rd::Frame::Build::unverified : static_cast<rd::Frame::Build>(99);
            expected = rd::Code::invalid_frame;
        }
        if (scenario == 27) {
            for (std::size_t i = 0; i < 2; ++i) {
                sources[i].handle = contexts[i].handle = 0x40012345u;
                f.memory.put(actors[i], 0x60, sources[i].handle);
            }
            expected = rd::Code::invalid_lock_binding;
        }
        f.memory.guarded_address = f.server.bag_items;
        f.memory.owner_depth_a = &locks[0].depth;
        f.memory.owner_depth_b = &locks[1].depth;
        rd::Capture result;
        result.authority.actor = UINTPTR_MAX;
        const auto before = f.memory.blocks;
        require(rd::capture_with_references(f.memory, f.frame, f.definitions, sources, bindings, result) == expected,
            "Referenced capture requires live complete owner identities");
        require(!contexts[0].depth && !contexts[1].depth && !locks[0].depth && !locks[1].depth,
            "Every success/failure returns all references and locks");
        require(contexts[0].order_ok && contexts[1].order_ok, "Release locks before any lifetime reference");
        require(contexts[0].attempts == contexts[0].cleanups && contexts[1].attempts == contexts[1].cleanups,
            "Failed native receipts also receive cleanup exactly once");
        if ((scenario >= 8 && scenario <= 10) || scenario == 25 || scenario == 26)
            require(!contexts[0].attempts && !contexts[1].attempts, "All sources/frame validated before acquisition");
        if (scenario == 1 || scenario == 2 || scenario == 11 || scenario == 27)
            require(!locks[0].attempts && !locks[1].attempts, "Incomplete or aliased owner pair never locks");
        if (expected == rd::Code::captured) require(result.snapshot.items.size() == 2, "Referenced item snapshot complete");
        else require(result.snapshot.items.empty() && !result.authority.actor, "Failed referenced capture clears output");
        require(f.memory.blocks == before, "Reference capture never writes item/actor source memory");
        require(!f.memory.forbidden_reads, "Pinned capture never reads a registry or a replacement unowned actor");
    }
    std::array<OwnerLock, 2> locks;
    ReferenceSource client{0x100000, 7}, server{0x200000, 7};
    client.locks = &locks; server.locks = &locks;
    const std::array sources{ref::Source{&client, 7, acquire_reference, release_reference},
        ref::Source{&server, 7, acquire_reference, release_reference}};
    ref::Pair pair;
    static_assert(!std::is_copy_constructible_v<ref::Pair> && !std::is_move_constructible_v<ref::Pair>);
    static_assert(!std::is_copy_constructible_v<ref::Receipt> && !std::is_move_constructible_v<ref::Receipt>);
    require(pair.acquire({}) == ref::Code::invalid_source, "Exactly two reference sources required");
    require(pair.acquire(sources) == ref::Code::acquired && pair.actor(0) == client.actor && pair.actor(1) == server.actor,
        "Pair exposes acquired owners only");
    require(!pair.actor(2) && pair.acquire(sources) == ref::Code::already_acquired, "No duplicate acquisition");
    ref::Code acquired{}, released{};
    std::uintptr_t wrong_thread_actor = 1;
    std::thread worker([&] { acquired = pair.acquire(sources); released = pair.release(); wrong_thread_actor = pair.actor(0); });
    worker.join();
    require(acquired == ref::Code::wrong_thread && released == ref::Code::wrong_thread && !wrong_thread_actor &&
        client.depth == 1 && server.depth == 1, "No ownership use or release on foreign thread");
    require(pair.release() == ref::Code::released && !pair.actor(0) && !client.depth && !server.depth,
        "Owning thread releases and clears references");
    require(pair.release() == ref::Code::empty, "No double reference release");
    try {
        ref::Pair exception_pair;
        require(exception_pair.acquire(sources) == ref::Code::acquired, "Exception fixture holds references");
        throw std::runtime_error("fixture unwind");
    } catch (const std::runtime_error&) {}
    require(!client.depth && !server.depth && client.cleanups == 2 && server.cleanups == 2,
        "C++ exception unwinds both native reference owners");
}
void retained_captures() {
    static_assert(!std::is_copy_constructible_v<rd::HeldCapture> && !std::is_move_constructible_v<rd::HeldCapture>);
    for (unsigned scenario = 0; scenario < 14; ++scenario) {
        Fixture f;
        std::array<OwnerLock, 2> locks;
        std::array<ReferenceSource, 2> contexts;
        std::array<ref::Source, 2> sources;
        std::array<crimson::repair::lease::Binding, 2> bindings;
        const std::array actors{f.client.actor, f.server.actor};
        for (std::size_t i = 0; i < 2; ++i) {
            contexts[i].actor = actors[i]; contexts[i].handle = f.handle; contexts[i].locks = &locks;
            sources[i] = {&contexts[i], f.handle, acquire_reference, release_reference};
            bindings[i] = {&locks[i], try_owner_lock, release_owner_lock};
            locks[i].reference_depth = &contexts[i].depth;
            f.memory.put(actors[i], 8, reinterpret_cast<std::uintptr_t>(&locks[i]));
        }
        f.memory.guarded_address = f.server.bag_items;
        f.memory.owner_depth_a = &locks[0].depth; f.memory.owner_depth_b = &locks[1].depth;
        for (const auto address : {f.frame.image_base + rd::server_context_rva, f.server_context,
            f.server_manager, f.hash, f.nodes, f.node})
            f.memory.forbidden.emplace_back(address, f.memory.blocks.at(address).size());
        bool unwound = false;
        try {
            rd::HeldCapture held;
            require(!held.view() && held.refresh(f.frame) == rd::Code::invalid_state,
                "Unopened scope exposes no evidence and cannot refresh");
            require(held.open(f.memory, f.frame, f.definitions, sources, bindings) == rd::Code::captured,
                "Retained capture opens one referenced/locked owner pair");
            require(held.view() && held.view()->snapshot.items.size() == 2 && locks[0].depth == 1 &&
                locks[1].depth == 1 && contexts[0].depth == 1 && contexts[1].depth == 1,
                "References and locks survive return from capture for the complete synchronous operation");
            const auto before = held.view()->snapshot;
            require(held.open(f.memory, f.frame, f.definitions, sources, bindings) == rd::Code::invalid_state &&
                contexts[0].attempts == 1 && contexts[1].attempts == 1, "Retained scope never reacquires or replaces owners");
            if (scenario == 0) {
                require(held.refresh(f.frame) == rd::Code::captured && held.view()->snapshot == before &&
                    locks[0].attempts == 1 && locks[1].attempts == 1, "Fresh full capture uses the same uninterrupted locks");
                require(held.release_locks() == rd::Code::released && !held.view() && !locks[0].depth && !locks[1].depth &&
                    contexts[0].depth == 1 && contexts[1].depth == 1, "Notification phase retains references after releasing both locks");
                require(held.release_locks() == rd::Code::invalid_state && held.refresh(f.frame) == rd::Code::invalid_state,
                    "Unlocked notification phase cannot reopen the read/write phase");
                require(held.close() == rd::Code::released && held.close() == rd::Code::released,
                    "Explicit close is idempotent");
            } else if (scenario == 1) {
                rd::Code refresh{}, unlock{}, close{}, open{}; const rd::Capture* foreign = reinterpret_cast<const rd::Capture*>(1);
                std::thread worker([&] {
                    foreign = held.view(); refresh = held.refresh(f.frame); unlock = held.release_locks(); close = held.close();
                    open = held.open(f.memory, f.frame, f.definitions, sources, bindings);
                }); worker.join();
                require(!foreign && refresh == rd::Code::wrong_thread && unlock == rd::Code::wrong_thread &&
                    close == rd::Code::wrong_thread && open == rd::Code::wrong_thread && held.view()->snapshot == before &&
                    contexts[0].depth == 1 && locks[0].depth == 1, "Wrong-thread operations preserve ownership without using it");
            } else if (scenario == 2) {
                throw std::runtime_error("retained fixture unwind");
            } else if (scenario == 3) {
                act::Plan plan;
                require(act::prepare({before.session, act::Scope::all, {}}, before, plan) == act::Code::prepared,
                    "Plan covers both carried and equipped items inside a retained capture");
                // Deliberate private-fixture mutation simulates a stale plan.
                f.memory.put(f.server.slots, 0x40, std::uint16_t(7));
                f.memory.put(f.client.slots, 0x40, std::uint16_t(7));
                require(held.refresh(f.frame) == rd::Code::captured && held.view()->snapshot != before,
                    "Refresh replaces old evidence with newly observed values");
                auto candidate = held.view()->snapshot;
                const auto unchanged = candidate;
                require(act::apply_to_owned_snapshot(plan, candidate) == act::Code::stale && candidate == unchanged,
                    "Full batch preflight rejects a later changed item before altering any earlier candidate");
            } else {
                auto frame = f.frame;
                rd::Code expected = rd::Code::invalid_frame;
                if (scenario == 4) ++frame.world_epoch;
                if (scenario == 5) ++frame.catalog_revision;
                if (scenario == 6) frame.build = rd::Frame::Build::unverified;
                if (scenario == 7) ++frame.image_base;
                if (scenario == 8) { f.memory.put(f.server.actor, 0x4b, std::uint8_t(1)); expected = rd::Code::owner_unavailable; }
                if (scenario == 9) { f.memory.put(f.client_manager, 0x50, f.server.actor); expected = rd::Code::identity_mismatch; }
                if (scenario == 10) { f.memory.put(f.server.actor, 8, std::uintptr_t(0)); expected = rd::Code::invalid_lock_binding; }
                if (scenario == 11) { f.memory.put(f.server.actor, 0x60, f.handle ^ 0x100000u); expected = rd::Code::owner_unavailable; }
                if (scenario == 12) { f.memory.blocks.erase(f.server.sockets); expected = rd::Code::unreadable; }
                if (scenario == 13) { f.memory.put(f.client.bag, 0xc, std::uint16_t(0xffff)); expected = rd::Code::invalid_layout; }
                require(held.refresh(frame) == expected && !held.view() && !contexts[0].depth && !contexts[1].depth &&
                    !locks[0].depth && !locks[1].depth, "Failed refresh clears stale evidence and releases the complete ownership scope");
                require(held.open(f.memory, f.frame, f.definitions, sources, bindings) == rd::Code::invalid_state,
                    "A failed scope cannot be reused for a different session");
            }
        } catch (const std::runtime_error& error) {
            if (scenario != 2 || std::string_view(error.what()) != "retained fixture unwind") throw;
            unwound = true;
        }
        require(unwound == (scenario == 2), "Only explicit exception fixture unwinds");
        require(!locks[0].depth && !locks[1].depth && !contexts[0].depth && !contexts[1].depth &&
            contexts[0].cleanups == 1 && contexts[1].cleanups == 1 && contexts[0].order_ok && contexts[1].order_ok,
            "Explicit/implicit/error/exception cleanup releases locks before references, exactly once");
        require(!f.memory.forbidden_reads, "Retained refresh never follows an unowned replacement or registry");
    }
}
}
int main() {
    try {
        happy_path(); refusals(); lookups(); registry_and_limits(); local_memory(); locked_captures(); reference_captures(); retained_captures();
        std::cout << "{\"ok\":true,\"checks\":" << checks
            << ",\"reader\":\"pinned_pair_explicit_build\",\"source\":\"own_fixture_memory\","
            << "\"game_process_access\":false,\"game_writes\":false,\"engine_commit\":false}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Repair reader test failed: " << e.what() << '\n';
        return 1;
    }
}
