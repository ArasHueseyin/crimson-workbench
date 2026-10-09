#include "reader_fixture.h"
#include <iostream>
#include <thread>

using namespace repair_test_fixture;
namespace wr = crimson::repair::writer;
namespace {
unsigned checks = 0, scenarios = 0;
void require(bool ok, const char* message) { ++checks; if (!ok) throw std::runtime_error(message); }
struct Owned {
    Fixture f;
    std::array<OwnerLock, 2> locks;
    std::array<ReferenceSource, 2> contexts;
    std::array<ref::Source, 2> sources;
    std::array<crimson::repair::lease::Binding, 2> bindings;
    rd::HeldCapture held;
    Owned() {
        f.frame.build = rd::Frame::Build::steam_25455892;
        const std::array actors{f.client.actor, f.server.actor};
        for (std::size_t i = 0; i < 2; ++i) {
            contexts[i].actor = actors[i]; contexts[i].handle = f.handle; contexts[i].locks = &locks;
            sources[i] = {&contexts[i], f.handle, acquire_reference, release_reference};
            bindings[i] = {&locks[i], try_owner_lock, release_owner_lock};
            locks[i].reference_depth = &contexts[i].depth;
            f.memory.put(actors[i], 8, reinterpret_cast<std::uintptr_t>(&locks[i]));
        }
        f.memory.owner_depth_a = &locks[0].depth; f.memory.owner_depth_b = &locks[1].depth;
        f.memory.guarded_address = f.server.bag_items;
    }
    act::Plan plan(act::Scope scope = act::Scope::all) {
        require(held.open(f.memory, f.frame, f.definitions, sources, bindings) == rd::Code::captured, "Writer owns a complete capture");
        const auto& snapshot = held.view()->snapshot;
        act::Plan result;
        const auto status = act::prepare({snapshot.session, scope, {11, act::Area::carried, 2, 0}}, snapshot, result);
        require(status == act::Code::prepared || status == act::Code::nothing_to_repair, "Writer receives a valid immutable plan");
        return result;
    }
    void close() {
        held.close();
        require(!locks[0].depth && !locks[1].depth && !contexts[0].depth && !contexts[1].depth &&
            contexts[0].order_ok && contexts[1].order_ok && contexts[0].cleanups == 1 && contexts[1].cleanups == 1,
            "Field phase releases both locks before the references exactly once");
    }
};
void success() {
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++scenarios; Owned o;
        const auto scope = scenario == 1 ? act::Scope::carried : scenario == 2 ? act::Scope::equipped :
            scenario == 3 ? act::Scope::one : act::Scope::all;
        if (scenario == 4) o.f.definitions[0] = {10, 100, act::absent, true};
        if (scenario == 5) for (const auto& a : {o.f.client, o.f.server}) {
            o.f.memory.put(a.bag_items, 0x40, act::absent);
            o.f.memory.put(a.slots, 0x40, std::uint16_t(30));
        }
        if (scenario == 6) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.sockets, 2, std::uint16_t(30));
        if (scenario == 7) for (const auto& a : {o.f.client, o.f.server}) {
            o.f.memory.put(a.bag_items, 0x40, std::uint16_t(100));
            o.f.memory.put(a.slots, 0x40, std::uint16_t(30));
            o.f.memory.put(a.sockets, 2, std::uint16_t(30));
        }
        const auto plan = o.plan(scope);
        auto expected = o.held.view()->snapshot;
        const auto applied = act::apply_to_owned_snapshot(plan, expected);
        const auto before = o.f.memory.blocks;
        // Independent byte oracle for the six known fields in this fixture.
        const auto patch = [&](std::uintptr_t address, std::size_t offset, std::uint16_t value) {
            o.f.memory.put(address, offset, value);
        };
        for (const auto& a : {o.f.client, o.f.server}) {
            if (scope != act::Scope::equipped) {
                if (scenario != 5) patch(a.bag_items, 0x40, 100);
                patch(a.sockets, 2, 30);
            }
            if (scope == act::Scope::all || scope == act::Scope::equipped) patch(a.slots, 0x40, 30);
        }
        const auto oracle = o.f.memory.blocks;
        o.f.memory.blocks = before;
        const auto result = wr::Batch::apply(o.held, o.f.frame, plan);
        require(result.code == (applied == act::Code::nothing_to_repair ? wr::Code::nothing_to_write : wr::Code::fields_written),
            "Full, carried, equipped, selected, NoWear and no-op field batches");
        require(result.written_words == (plan.count().main_fields + plan.count().socket_fields) * 2 &&
            result.attempted_words == result.written_words && o.f.memory.write_calls == result.written_words,
            "Only changed words are stored, once per realm");
        require(o.f.memory.blocks == oracle && o.held.view()->snapshot == expected,
            "Every source byte matches the independent oracle; UID/quantity/empty slots/metadata remain intact");
        require(o.locks[0].depth == 1 && o.locks[1].depth == 1 && o.contexts[0].depth == 1 && o.contexts[1].depth == 1,
            "Field success retains ownership for subsequent copy/dirty/event stages");
        const auto writes = o.f.memory.write_calls;
        require(wr::Batch::apply(o.held, o.f.frame, plan).code == wr::Code::already_attempted && o.f.memory.write_calls == writes,
            "No field replay inside one capture, including a no-op");
        o.close();
    }
}
void rejection() {
    for (unsigned scenario = 0; scenario < 14; ++scenario) {
        ++scenarios; Owned o;
        if (scenario == 1) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.bag_items, 0x80, std::uint32_t(1));
        if (scenario == 2) o.f.memory.put(o.f.client.bag, 0, o.f.server.bag_items);
        if (scenario == 3) o.f.memory.put(o.f.client.bag_items, 0x60, o.f.server.sockets);
        if (scenario == 4 || scenario == 5) for (const auto& a : {o.f.client, o.f.server}) {
            const auto pointer = scenario == 4 ? a.bag_items + 0x48 : a.actor + 0x20;
            const auto sockets = o.f.memory.blocks.at(a.sockets);
            for (std::size_t i = 0; i < sockets.size(); ++i) o.f.memory.put(pointer, i, sockets[i]);
            o.f.memory.put(a.bag_items, 0x60, pointer);
        }
        if (scenario == 6) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.bag_items, 0xb8, a.slots + 0x40);
        if (scenario == 7) for (const auto& a : {o.f.client, o.f.server}) {
            o.f.memory.put(a.bag_items, 0x78, a.slots + 0x40);
            o.f.memory.put(a.bag_items, 0x84, std::uint32_t(1));
        }
        if (scenario == 13) for (const auto& a : {o.f.client, o.f.server}) {
            o.f.memory.put(a.bag_items, 0x78, a.slots + 0x40 - 12);
            o.f.memory.put(a.bag_items, 0x84, std::uint32_t(1));
        }
        auto plan = o.plan();
        auto frame = o.f.frame;
        wr::Reason reason = scenario < 2 ? wr::Reason::invalid_geometry : wr::Reason::aliased_storage;
        if (scenario == 0) {
            for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.bag_items, 0x6c, std::uint32_t(1));
            reason = wr::Reason::read_failed;
        }
        if (scenario == 8) { o.f.memory.deny_word = o.f.server.sockets + 2; reason = wr::Reason::read_only; }
        if (scenario == 9) {
            for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.slots, 0x40, std::uint16_t(7));
            reason = wr::Reason::invalid_plan;
        }
        if (scenario == 10) { ++frame.world_epoch; reason = wr::Reason::read_failed; }
        if (scenario == 11) { o.f.memory.put(o.f.client_manager, 0x50, o.f.server.actor); reason = wr::Reason::read_failed; }
        if (scenario == 12) { plan = {}; reason = wr::Reason::invalid_plan; }
        const auto before = o.f.memory.blocks;
        const auto result = wr::Batch::apply(o.held, frame, plan);
        require(result.code == wr::Code::rejected && result.reason == reason && !result.attempted_words && !o.f.memory.write_calls,
            ("Preflight rejects the entire batch before any write, case " + std::to_string(scenario)).c_str());
        require(o.f.memory.blocks == before, "Rejected batch makes no changes to earlier valid items");
        o.close();
    }
}
void changed_during_admission() {
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        ++scenarios; Owned o; const auto plan = o.plan();
        o.f.memory.probe_callback = [&](unsigned call) {
            if (call != 2) return;
            if (scenario == 0) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.slots, 0x40, std::uint16_t(7));
            if (scenario == 1) {
                const auto copy = o.f.memory.blocks.at(o.f.server.bag_items);
                const auto replacement = o.f.memory.allocate(copy.size()); o.f.memory.blocks.at(replacement) = copy;
                o.f.memory.put(o.f.server.bag, 0, replacement);
            }
            if (scenario == 2) o.f.memory.put(o.f.server.bag_items + act::item_size, 0x20, std::uint8_t(7));
        };
        const auto result = wr::Batch::apply(o.held, o.f.frame, plan);
        require(result.code == wr::Code::rejected && result.reason == wr::Reason::source_changed && !o.f.memory.write_calls,
            "A full second read catches changed later values, relocated same-byte arrays and empty-slot metadata");
        o.close();
    }
}
void uncertain_writes() {
    for (unsigned scenario = 0; scenario < 6; ++scenario) {
        ++scenarios; Owned o; const auto plan = o.plan();
        if (scenario < 3) { o.f.memory.fail_write = scenario == 1 ? 3 : 1; o.f.memory.store_then_fail = scenario == 2; }
        if (scenario == 3) o.f.memory.ignore_store = true;
        if (scenario == 4) o.f.memory.write_callback = [&](unsigned n) {
            if (n == 1) o.f.memory.put(o.f.server.bag_items + act::item_size, 0x20, std::uint8_t(7));
        };
        if (scenario == 5) o.f.memory.write_callback = [&](unsigned n) {
            if (n == 6) o.f.memory.put(o.f.client_manager, 0x50, o.f.server.actor);
        };
        const auto result = wr::Batch::apply(o.held, o.f.frame, plan);
        require(result.code == wr::Code::outcome_unknown && result.attempted_words >= 1 &&
            result.reason == (scenario < 3 ? wr::Reason::write_failed : wr::Reason::verification_failed),
            "A failed/lying/partial store or changed read set cannot report rejection or success");
        if (scenario < 3) require(!o.held.view(), "A failed word store hides the now-stale capture");
        const auto writes = o.f.memory.write_calls;
        const auto again = wr::Batch::apply(o.held, o.f.frame, plan);
        require((again.code == wr::Code::already_attempted || again.code == wr::Code::invalid_state) && writes == o.f.memory.write_calls,
            "Uncertain outcome never triggers rollback or automatic field replay");
        if (scenario == 0) require(!result.written_words, "Even a first failed attempt is conservatively uncertain");
        if (scenario == 1) require(result.written_words == 2, "Partial batch reports only confirmed word stores");
        o.close();
    }
}
void lifecycle() {
    ++scenarios; Owned o; const auto plan = o.plan();
    wr::Outcome foreign;
    std::thread worker([&] { foreign = wr::Batch::apply(o.held, o.f.frame, plan); }); worker.join();
    require(foreign.code == wr::Code::wrong_thread && !o.f.memory.probe_calls && !o.f.memory.write_calls,
        "Foreign thread never probes or writes any field");
    require(wr::Batch::apply(o.held, o.f.frame, plan).code == wr::Code::fields_written, "Wrong-thread attempt does not consume owning-thread work");
    o.held.release_locks();
    require(wr::Batch::apply(o.held, o.f.frame, plan).code == wr::Code::invalid_state, "No writes after releasing the owner locks");
    o.close();
    ++scenarios; Owned read_only;
    struct OnlyRead final : rd::Memory {
        const rd::Memory& underlying;
        explicit OnlyRead(const rd::Memory& m) : underlying(m) {}
        bool copy(std::uintptr_t a, std::span<std::uint8_t> b) const noexcept override { return underlying.copy(a, b); }
    } reader(read_only.f.memory);
    require(read_only.held.open(reader, read_only.f.frame, read_only.f.definitions, read_only.sources, read_only.bindings) == rd::Code::captured,
        "Read-only capture fixture");
    const auto denied = wr::Batch::apply(read_only.held, read_only.f.frame, plan);
    require(denied.code == wr::Code::rejected && denied.reason == wr::Reason::read_only && !read_only.f.memory.write_calls,
        "Detached/read-only Memory never silently gains write access");
    read_only.close();
}
void local_pages() {
    ++scenarios; wr::LocalAccess access;
    auto* pages = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(pages != nullptr, "Own-process field fixture allocated");
    const auto address = reinterpret_cast<std::uintptr_t>(pages);
    const std::uint16_t old = 35; std::memcpy(pages, &old, 2);
    require(access.write_word(address, 35, 100) && !access.write_word(address, 35, 101), "Local writer stores an aligned matching word only");
    DWORD previous;
    require(VirtualProtect(pages + 4096, 4096, PAGE_READONLY, &previous) != 0, "Own second page read-only");
    require(!access.writable(address + 4090, 8) && !access.write_word(address + 4096, 0, 1) &&
        !access.write_word(address + 1, 0, 1) && !access.writable(UINTPTR_MAX - 1, 4) && !access.writable(0, 2),
        "Read-only, crossing, unaligned, overflow and null targets rejected");
    require(VirtualProtect(pages + 4096, 4096, PAGE_EXECUTE_READ, &previous) != 0, "Own second page executable/read-only");
    require(!access.writable(address + 4096, 2), "Executable memory is never admitted as item storage");
    require(VirtualFree(pages, 0, MEM_RELEASE) != 0, "Own pages released");
}
void maximum_batch() {
    ++scenarios; Owned o;
    for (auto* a : {&o.f.client, &o.f.server}) {
        a->bag_items = o.f.memory.allocate(act::max_items * act::item_size);
        o.f.memory.put(a->bag, 0, a->bag_items);
        o.f.memory.put(a->bag, 0xc, static_cast<std::uint16_t>(act::max_items));
        for (std::size_t i = 0; i < act::max_items; ++i)
            o.f.item(a->bag_items + i * act::item_size, 1000 + i, 10, 1, 35);
        o.f.item(a->slots, UINT64_MAX, act::absent, 0, 0);
    }
    o.f.memory.guarded_address = o.f.server.bag_items;
    const auto plan = o.plan();
    require(plan.count().items == act::max_items, "Maximum-budget plan contains every item");
    const auto result = wr::Batch::apply(o.held, o.f.frame, plan);
    require(result.code == wr::Code::fields_written && result.written_words == act::max_items * 2 &&
        act::verify_applied(plan, o.held.view()->snapshot) == act::Code::applied,
        "The full implementation budget writes and verifies both realms in one batch");
    o.close();
}
}
int main() {
    try {
        success(); rejection(); changed_during_admission(); uncertain_writes(); lifecycle(); local_pages(); maximum_batch();
        std::cout << "{\"ok\":true,\"checks\":" << checks << ",\"scenarios\":" << scenarios
            << ",\"field_writer\":true,\"source\":\"owned_fixture_memory\",\"engine_commit\":false,\"game_process_access\":false}\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "Field writer: " << e.what() << '\n'; return 1; }
}
