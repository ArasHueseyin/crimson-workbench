#include "reader_fixture.h"
#include "repair_transaction.h"
#include <iostream>
#include <set>

using namespace repair_test_fixture;
namespace tx = crimson::repair::transaction;
namespace it = crimson::repair::item;
namespace {
unsigned checks = 0, scenarios = 0;
void require(bool ok, const char* message) { ++checks; if (!ok) throw std::runtime_error(message); }
template<class T> T read(const void* p, std::size_t at) {
    T out; std::memcpy(&out, static_cast<const std::uint8_t*>(p) + at, sizeof(out)); return out;
}
template<class T> void put(void* p, std::size_t at, T v) { std::memcpy(static_cast<std::uint8_t*>(p) + at, &v, sizeof(v)); }
struct Probe {
    unsigned constructs = 0, copies = 0, destroys = 0, admits = 0, marks = 0, notices = 0, finishes = 0;
    unsigned fail_construct = 0, fail_copy = 0, corrupt_copy = 0, alias_copy = 0, fail_admit = 0, fail_mark = 0, fail_notice = 0;
    bool ok = true;
    bool fail_finish = false;
    std::set<void*> buffers;
    std::function<void(unsigned)> copy_hook;
};
thread_local Probe* probe = nullptr;
void* construct(void* p) noexcept {
    ++probe->constructs; std::memset(p, 0, act::item_size);
    return probe->constructs == probe->fail_construct ? nullptr : p;
}
void* assign(void* p, const void* source) noexcept {
    auto& q = *probe; ++q.copies;
    std::memcpy(p, source, act::item_size);
    const auto count = read<std::uint32_t>(source, 0x68);
    void* sockets = nullptr;
    if (count) {
        sockets = std::malloc(count * act::socket_size);
        if (!sockets) std::terminate();
        std::memcpy(sockets, read<const void*>(source, 0x60), count * act::socket_size);
        q.buffers.insert(sockets);
    }
    put(p, 0x60, sockets); put(p, 0x6c, count);
    if (q.copies == q.alias_copy) {
        if (sockets) { q.buffers.erase(sockets); std::free(sockets); }
        put(p, 0x60, read<const void*>(source, 0x60));
    }
    if (q.copies == q.corrupt_copy) put(p, 0x18, read<std::uint64_t>(source, 0x18) + 1);
    if (q.copy_hook) q.copy_hook(q.copies);
    return q.copies == q.fail_copy ? nullptr : p;
}
void destroy(void* p) noexcept {
    ++probe->destroys;
    auto* sockets = read<void*>(p, 0x60);
    if (probe->buffers.erase(sockets)) std::free(sockets);
}
const it::Binding lifecycle{construct, assign, destroy};
struct Owned {
    Fixture f{true};
    std::array<OwnerLock, 2> locks;
    std::array<ReferenceSource, 2> contexts;
    std::array<ref::Source, 2> sources;
    std::array<crimson::repair::lease::Binding, 2> bindings;
    Probe p;
    rd::Frame current;
    std::function<void(unsigned)> admit_hook, mark_hook, notice_hook, finish_hook;
    unsigned expected_items = 2;
    Owned() {
        probe = &p; f.frame.build = rd::Frame::Build::steam_25455892; current = f.frame;
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
        f.memory.write_callback = [&](unsigned) {
            p.ok &= p.copies == expected_items * 2 && p.admits == expected_items && !p.marks && !p.notices &&
                !p.destroys && locks[0].depth == 1 && locks[1].depth == 1;
        };
    }
    ~Owned() { probe = nullptr; }
    static rd::Frame frame(void* self) noexcept { return static_cast<Owned*>(self)->current; }
    static bool admit(void* self, const tx::Scene& scene, const act::Change&) noexcept {
        auto& o = *static_cast<Owned*>(self); auto& p = o.p; ++p.admits;
        p.ok &= scene.frame == o.f.frame && scene.authority.actor == o.f.server.actor &&
            scene.presentation.actor == o.f.client.actor && o.locks[0].depth == 1 && o.locks[1].depth == 1 &&
            !o.f.memory.write_calls && !p.copies;
        if (o.admit_hook) o.admit_hook(p.admits);
        return p.admits != p.fail_admit;
    }
    bool notice_valid(const tx::Notice& notice) const {
        for (const auto& [copy, after] : {std::pair{notice.authority_after, &notice.change.after.authority},
            std::pair{notice.presentation_after, &notice.change.after.presentation}}) {
            if (!copy || read<std::uint64_t>(copy, 0) != notice.change.before.identity.instance ||
                read<std::uint16_t>(copy, 0x40) != read<std::uint16_t>(after->bytes.data(), 0x40)) return false;
            const auto n = after->sockets.size();
            if (read<std::uint32_t>(copy, 0x68) != n || (n && std::memcmp(read<const void*>(copy, 0x60), after->sockets.data(), n * act::socket_size))) return false;
        }
        if (notice.change.before.identity.area == act::Area::equipped &&
            (notice.equipment.item != notice.change.before.identity || !notice.equipment.was_broken || notice.equipment.is_broken)) return false;
        return true;
    }
    static bool mark(void* self, const tx::Scene&, const tx::Notice& notice) noexcept {
        auto& o = *static_cast<Owned*>(self); auto& p = o.p; ++p.marks;
        p.ok &= p.copies == o.expected_items * 2 && !p.destroys && !p.notices && o.notice_valid(notice) &&
            o.f.memory.write_calls != 0 && o.locks[0].depth == 1 && o.locks[1].depth == 1 &&
            o.contexts[0].depth == 1 && o.contexts[1].depth == 1;
        if (o.mark_hook) o.mark_hook(p.marks);
        return p.marks != p.fail_mark;
    }
    static bool notify(void* self, const tx::Scene&, const tx::Notice& notice) noexcept {
        auto& o = *static_cast<Owned*>(self); auto& p = o.p; ++p.notices;
        p.ok &= p.marks == o.expected_items && !p.destroys && o.notice_valid(notice) &&
            !o.locks[0].depth && !o.locks[1].depth && o.contexts[0].depth == 1 && o.contexts[1].depth == 1;
        if (o.notice_hook) o.notice_hook(p.notices);
        return p.notices != p.fail_notice;
    }
    static bool finish(void* self, const tx::Scene&) noexcept {
        auto& o = *static_cast<Owned*>(self); auto& p = o.p; ++p.finishes;
        p.ok &= p.notices == o.expected_items && !p.destroys && !o.locks[0].depth && !o.locks[1].depth &&
            o.contexts[0].depth == 1 && o.contexts[1].depth == 1;
        if (o.finish_hook) o.finish_hook(p.finishes);
        return !p.fail_finish;
    }
    tx::Environment environment() { return {this, frame, {{{this, admit, mark, notify}, {this, admit, mark, notify}}}, finish}; }
    act::Plan plan(tx::Adapter& adapter, act::Scope scope = act::Scope::all) {
        const auto snapshot = adapter.capture();
        require(snapshot.items.size() == 2, "Capture retains both owners for planning");
        act::Plan plan;
        require(act::prepare({snapshot.session, scope, {11, act::Area::carried, 2, 0}}, snapshot, plan) == act::Code::prepared,
            "Immutable plan prepared from owned sources");
        expected_items = static_cast<unsigned>(plan.changes().size());
        return plan;
    }
    void cleaned() {
        require(p.ok && p.buffers.empty() && p.destroys == p.constructs, "Lifecycle order, copies and releases remain balanced");
        require(!locks[0].depth && !locks[1].depth && !contexts[0].depth && !contexts[1].depth &&
            contexts[0].order_ok && contexts[1].order_ok, "Every path releases locks before actor references");
    }
};
void success() {
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        ++scenarios; Owned o;
        const auto scope = scenario == 1 ? act::Scope::carried : scenario == 2 ? act::Scope::equipped :
            scenario == 3 ? act::Scope::one : act::Scope::all;
        if (scenario == 4) o.f.definitions[0] = {10, 100, act::absent, true};
        for (const auto& a : {o.f.client, o.f.server}) {
            if (scenario >= 5) o.f.memory.put(a.bag_items, 0x70, std::uint8_t(4));
            if (scenario == 6 || scenario == 7) {
                o.f.memory.put(a.bag_items, 0x68, std::uint32_t(0));
                o.f.memory.forbidden.emplace_back(a.sockets, 12);
            }
            if (scenario == 6) { o.f.memory.put(a.bag_items, 0x60, std::uintptr_t(0)); o.f.memory.put(a.bag_items, 0x6c, std::uint32_t(0)); }
            if (scenario == 8 || scenario == 9) {
                o.f.memory.put(a.bag_items, 0x68, std::uint32_t(1));
                if (scenario == 8) o.f.memory.put(a.bag_items, 0x6c, std::uint32_t(1));
                o.f.memory.forbidden.emplace_back(a.sockets + 6, 6);
            }
        }
        tx::Adapter adapter(o.f.memory, o.f.frame, o.f.definitions, o.sources, o.bindings, lifecycle, o.environment());
        const auto plan = o.plan(adapter, scope);
        require(adapter.commit(plan) == act::Commit::pending && adapter.report().stage == tx::Stage::submitted,
            "Complete transaction submits both realms and area routes, never premature success");
        require(adapter.report().prepared_items == plan.count().items && adapter.report().marked_items == plan.count().items &&
            adapter.report().submitted_items == plan.count().items && o.p.admits == plan.count().items &&
            adapter.report().finalized && o.p.finishes == 1,
            "Each requested item prepared, marked and submitted exactly once");
        o.cleaned();
        rd::Capture observed;
        require(rd::capture_with_references(o.f.memory, o.f.frame, o.f.definitions, o.sources, o.bindings, observed) == rd::Code::captured &&
            act::verify_applied(plan, observed.snapshot) == act::Code::applied && !o.f.memory.forbidden_reads,
            "Fresh acquisition proves complete after-images without reading unstored slots or unused allocation tail");
        const auto writes = o.f.memory.write_calls;
        require(adapter.commit(plan) == act::Commit::unknown && adapter.capture().items.empty() && o.f.memory.write_calls == writes,
            "A submitted transaction cannot be captured or committed again");
    }
}
void refused() {
    for (unsigned scenario = 0; scenario < 20; ++scenario) {
        ++scenarios; Owned o; auto env = o.environment(); auto binding = lifecycle;
        if (scenario == 0) env.routes[0] = {};
        if (scenario == 1) env.routes[1].notify_unlocked = nullptr;
        if (scenario == 2) o.p.fail_admit = 2;
        if (scenario == 3) o.p.fail_construct = 3;
        if (scenario == 4) o.p.fail_copy = 3;
        if (scenario == 5) o.p.corrupt_copy = 2;
        if (scenario == 6) o.p.alias_copy = 1;
        if (scenario == 7) o.admit_hook = [&](unsigned) { ++o.current.world_epoch; };
        if (scenario == 8) o.p.copy_hook = [&](unsigned n) { if (n == 4) ++o.current.catalog_revision; };
        if (scenario == 9) o.f.memory.deny_word = o.f.server.slots + 0x40;
        if (scenario == 10) binding.destroy = nullptr;
        if (scenario == 19) env.finish_unlocked = nullptr;
        if (scenario >= 15 && scenario < 19) for (const auto& a : {o.f.client, o.f.server}) {
            if (scenario == 15 || scenario == 16) {
                o.f.memory.put(a.bag_items, 0x78, std::uintptr_t(0x10000));
                o.f.memory.put(a.bag_items, 0x80, std::uint32_t(scenario == 15));
                o.f.memory.put(a.bag_items, 0x84, std::uint32_t(scenario == 15 ? 1 : UINT32_MAX));
            }
            if (scenario == 17) o.f.memory.put(a.bag_items, 0xb8, std::uintptr_t(0x10000));
            if (scenario == 18) o.f.memory.put(a.bag_items, 0xb0, std::uint32_t(1));
        }
        tx::Adapter adapter(o.f.memory, o.f.frame, o.f.definitions, o.sources, o.bindings, binding, env);
        auto plan = o.plan(adapter);
        if (scenario == 11) plan = {};
        if (scenario == 12) ++o.current.world_epoch;
        if (scenario == 13) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.bag_items, 0x6c, std::uint32_t(1));
        if (scenario == 14) for (const auto& a : {o.f.client, o.f.server}) o.f.memory.put(a.slots, 0x40, std::uint16_t(7));
        const auto before = o.f.memory.blocks;
        require(adapter.commit(plan) == act::Commit::rejected && adapter.report().stage == tx::Stage::refused &&
            !o.f.memory.write_calls && !o.p.marks && !o.p.notices && !o.p.finishes && o.f.memory.blocks == before,
            ("Whole batch rejected before all source stores, case " + std::to_string(scenario)).c_str());
        if (scenario >= 15 && scenario < 19) require(!o.p.constructs && !o.p.copies && adapter.report().copy == it::Code::invalid_source,
            "Unmapped auxiliary data, oversized allocations and invalid owning headers fail before native lifecycle calls");
        o.cleaned();
    }
}
void unknown() {
    for (unsigned scenario = 0; scenario < 9; ++scenario) {
        ++scenarios; Owned o;
        if (scenario <= 2) { o.f.memory.fail_write = scenario == 1 ? 3 : 1; o.f.memory.store_then_fail = scenario == 2; }
        if (scenario == 3) o.p.fail_mark = 2;
        if (scenario == 4) o.p.fail_notice = 2;
        if (scenario == 5) o.mark_hook = [&](unsigned n) { if (n == 1) ++o.current.world_epoch; };
        if (scenario == 6) o.notice_hook = [&](unsigned n) { if (n == 1) ++o.current.catalog_revision; };
        if (scenario == 7) o.p.fail_finish = true;
        if (scenario == 8) o.finish_hook = [&](unsigned) { ++o.current.world_epoch; };
        tx::Adapter adapter(o.f.memory, o.f.frame, o.f.definitions, o.sources, o.bindings, lifecycle, o.environment());
        const auto plan = o.plan(adapter);
        require(adapter.commit(plan) == act::Commit::unknown && adapter.report().stage == tx::Stage::uncertain,
            "Every post-store failure or changed epoch is uncertain, never rejection or success");
        if (scenario < 3) require(!o.p.marks && !o.p.notices, "Failed field writes cannot emit events");
        require(!adapter.report().finalized && o.p.finishes == unsigned(scenario >= 7),
            "Finalization follows all notices once; failure or epoch change remains unconfirmed");
        const auto writes = o.f.memory.write_calls;
        require(adapter.commit(plan) == act::Commit::unknown && o.f.memory.write_calls == writes, "Unknown outcome never retries or rolls back");
        o.cleaned();
    }
}
void threads_and_queue() {
    ++scenarios; Owned o;
    tx::Adapter adapter(o.f.memory, o.f.frame, o.f.definitions, o.sources, o.bindings, lifecycle, o.environment());
    const auto plan = o.plan(adapter);
    bool refused = false;
    std::thread other([&] { refused = adapter.capture().items.empty() && adapter.commit(plan) == act::Commit::rejected; }); other.join();
    require(refused && !o.f.memory.write_calls && o.locks[0].depth == 1 && !o.p.copies, "Foreign thread cannot consume the transaction or access native methods");
    require(adapter.commit(plan) == act::Commit::pending, "Bound thread retains its one valid attempt"); o.cleaned();
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        ++scenarios; Owned q;
        tx::Adapter transaction(q.f.memory, q.f.frame, q.f.definitions, q.sources, q.bindings, lifecycle, q.environment());
        act::Queue queue(std::this_thread::get_id());
        const act::Session session{q.f.frame.world_epoch, q.f.handle, q.f.frame.catalog_revision};
        const auto now = std::chrono::steady_clock::now();
        const auto ticket = queue.submit({session, act::Scope::all, {}}, now);
        require(ticket.code == act::Code::queued && queue.execute(transaction, now).code == act::Code::awaiting_confirmation,
            "Queue keeps successfully submitted engine work awaiting independent confirmation");
        rd::Capture fresh;
        require(rd::capture_with_references(q.f.memory, q.f.frame, q.f.definitions, q.sources, q.bindings, fresh) == rd::Code::captured, "Confirmation uses a new ownership scope");
        if (scenario == 2) ++fresh.snapshot.session.generation;
        const auto result = queue.confirm(ticket.request_id, session, fresh.snapshot, scenario != 1, now);
        require(result.code == (scenario == 0 ? act::Code::applied : act::Code::outcome_unknown),
            "Only fresh values, matching session and separately confirmed notices finish the request");
        if (scenario) require(queue.submit({session, act::Scope::all, {}}, now).code == act::Code::fault_latched,
            "Missing event proof or a changed session permanently faults the queue");
        q.cleaned();
    }
}
}
int main() {
    try {
        success(); refused(); unknown(); threads_and_queue();
        std::cout << "{\"ok\":true,\"checks\":" << checks << ",\"scenarios\":" << scenarios
            << ",\"binding_source\":\"fixture_callbacks\",\"game_process_access\":false,\"live_installable\":false}\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "Repair transaction test failed: " << e.what() << '\n'; return 1; }
}
