#include "repair_action.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace crimson::repair::action;
namespace {
unsigned checks = 0;
void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
template<class T, std::size_t N>
void put(std::array<std::uint8_t, N>& bytes, std::size_t offset, T value) {
    if (offset > N || sizeof(T) > N - offset) throw std::runtime_error("Fixture bounds");
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}
template<class T, std::size_t N>
T get(const std::array<std::uint8_t, N>& bytes, std::size_t offset) {
    T value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}
Item item(std::uint64_t uid, std::uint16_t key, std::uint16_t current, Area area = Area::carried) {
    Item result;
    result.identity = {uid, area, 1, static_cast<std::uint16_t>(uid)};
    auto& raw = result.authority.bytes;
    raw.fill(0xa7);
    put(raw, 0, uid);
    put(raw, 8, key);
    put(raw, 0x10, std::int64_t(1));
    put(raw, 0x40, current);
    put(raw, 0x60, UINT64_MAX); // poison pointer: must never be dereferenced
    put(raw, 0x68, std::uint32_t(0));
    raw[0x70] = 0;
    result.presentation = result.authority;
    // Unrelated client/server fields need not be equal and must be preserved.
    result.presentation.bytes[0x38] = 0xc1;
    return result;
}
Socket socket(std::uint16_t key, std::uint16_t durability, std::uint8_t index) {
    Socket result;
    put(result.bytes, 0, key);
    put(result.bytes, 2, durability);
    result.bytes[4] = index;
    result.bytes[5] = 5;
    return result;
}
void with_sockets(Item& target, std::vector<Socket> sockets, std::uint32_t active) {
    for (auto* image : {&target.authority, &target.presentation}) {
        image->sockets = sockets;
        image->sockets.resize(active);
        put(image->bytes, 0x68, active);
        image->bytes[0x70] = static_cast<std::uint8_t>(sockets.size());
    }
}
Snapshot fixture() {
    Snapshot result{{5, 77, 1}, {{10, 100}, {20, 30}, {30, absent}, {40, 0}}, {}};
    result.items.push_back(item(1, 10, 35));
    result.items.push_back(item(2, 20, 0, Area::equipped));
    result.items.push_back(item(3, 30, 17, Area::equipped));
    result.items.push_back(item(4, 40, 0));
    // Repair a zero-durability socket on an indestructible main item, leave
    // empty/unlimited records unchanged; logical slot 3 has no stored record.
    with_sockets(result.items[2], {socket(10, 0, 0), socket(absent, 123, 1),
        socket(30, 17, 2), socket(20, 1, 3)}, 3);
    return result;
}
Request request(const Snapshot& source, Scope scope = Scope::all) { return {source.session, scope, {}}; }
void only_endurance(const Image& before, const Image& after) {
    for (std::size_t i = 0; i < item_size; ++i)
        if (i != 0x40 && i != 0x41) require(before.bytes[i] == after.bytes[i], "Non-endurance item byte changed");
    require(before.sockets.size() == after.sockets.size(), "Socket allocation changed");
    for (std::size_t s = 0; s < before.sockets.size(); ++s)
        for (std::size_t b = 0; b < socket_size; ++b)
            if (b != 2 && b != 3)
                require(before.sockets[s].bytes[b] == after.sockets[s].bytes[b], "Socket identity/flags changed");
}
void equipment_notices() {
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        auto source = fixture();
        auto& equipped = source.items[1];
        if (scenario == 1) {
            put(equipped.authority.bytes, 0x40, std::uint16_t(15));
            equipped.presentation = equipped.authority;
        }
        if (scenario == 2) source.definitions[1] = Definition{20, 30, absent, true};
        if (scenario >= 3) {
            with_sockets(equipped, {socket(10, 0, 0)}, 1);
            const auto current = scenario == 3 ? absent : std::uint16_t(0);
            put(equipped.authority.bytes, 0x40, current);
            equipped.presentation = equipped.authority;
            if (scenario == 4) source.definitions[1] = Definition{20, absent};
            if (scenario == 5) source.definitions[1] = Definition{20, 0};
        }
        Plan plan;
        require(prepare(request(source), source, plan) == Code::prepared, "Equipment event plan");
        const auto before = source;
        EquipmentNotice event;
        require(equipment_notice(plan, equipped.identity, event) == Code::prepared &&
            event.session == plan.session() && event.item == equipped.identity,
            "Equipment notice retains exact request session and slot/UID");
        require(event.was_broken == (scenario == 0 || scenario == 3 || scenario == 5 || scenario == 6) &&
            event.is_broken == (scenario == 3 || scenario == 5),
            "Engine broken state uses signed endurance and active maximum, including no-wear and socket-only repair");
        require(source == before, "Event planning does not mutate snapshots");
        auto wrong = equipped.identity; ++wrong.instance;
        require(equipment_notice(plan, wrong, event) == Code::missing_item && event == EquipmentNotice{},
            "Wrong UID clears old notification metadata");
        wrong = equipped.identity; ++wrong.slot;
        require(equipment_notice(plan, wrong, event) == Code::missing_item, "Same UID in wrong slot rejected");
        require(equipment_notice(plan, source.items[0].identity, event) == Code::invalid_request,
            "Carried repair cannot borrow equipment event path");
        require(equipment_notice(Plan{}, equipped.identity, event) == Code::invalid_request && event == EquipmentNotice{},
            "Unprepared plan cannot produce event metadata");
    }
}
void happy_path() {
    auto source = fixture();
    std::reverse(source.definitions.begin(), source.definitions.end());
    const auto before = source;
    Plan plan;
    require(prepare(request(source), source, plan) == Code::prepared, "Prepare all");
    require(plan.count() == Count{3, 2, 1}, "Count items and fields once, not per realm");
    require(source == before, "Preparation must never mutate its snapshot");
    for (const auto& change : plan.changes()) {
        only_endurance(change.before.authority, change.after.authority);
        only_endurance(change.before.presentation, change.after.presentation);
    }
    require(apply_to_owned_snapshot(plan, source) == Code::applied, "Apply all");
    require(get<std::uint16_t>(source.items[0].authority.bytes, 0x40) == 100, "Main repaired");
    require(get<std::uint16_t>(source.items[1].authority.bytes, 0x40) == 30, "Existing broken item repaired");
    require(get<std::uint16_t>(source.items[2].authority.bytes, 0x40) == 17, "Unlimited main preserved");
    require(get<std::uint16_t>(source.items[2].authority.sockets[0].bytes, 2) == 100, "Broken socket repaired");
    require(source.items[2].authority.sockets[1] == before.items[2].authority.sockets[1], "Empty socket preserved");
    require(source.items[2].authority.sockets[2] == before.items[2].authority.sockets[2], "Unlimited socket preserved");
    require(source.items[2].authority.sockets.size() == 3 && source.items[2].authority.bytes[0x70] == 4,
        "Unstored logical slots never become materialized records");
    require(source.items[3] == before.items[3], "No-durability item preserved");
    require(source.items[2].authority.sockets == source.items[2].presentation.sockets, "Both mirrors repaired");
    const auto repaired = source;
    require(apply_to_owned_snapshot(plan, source) == Code::stale && source == repaired, "No plan replay");
    require(prepare(request(source), source, plan) == Code::nothing_to_repair, "Second action is a no-op");
    require(plan.count() == Count{} && apply_to_owned_snapshot(plan, source) == Code::nothing_to_repair,
        "No-op plan");
}
void selections() {
    for (const auto [scope, count] : {std::pair{Scope::carried, Count{1, 1, 0}},
        std::pair{Scope::equipped, Count{2, 1, 1}}}) {
        auto source = fixture();
        Plan plan;
        require(prepare(request(source, scope), source, plan) == Code::prepared && plan.count() == count,
            "Scope selection");
        require(apply_to_owned_snapshot(plan, source) == Code::applied, "Scoped apply");
        require(get<std::uint16_t>(source.items[scope == Scope::carried ? 1 : 0].authority.bytes, 0x40) ==
            (scope == Scope::carried ? 0 : 35), "Unselected item unchanged");
    }
    auto source = fixture();
    auto req = request(source, Scope::one);
    req.item = source.items[2].identity;
    Plan plan;
    require(prepare(req, source, plan) == Code::prepared && plan.count() == Count{1, 0, 1}, "Single selection");
    ++req.item.instance;
    require(prepare(req, source, plan) == Code::missing_item && !plan.prepared(), "Missing item clears old plan");
    source.items.clear();
    require(prepare(request(source), source, plan) == Code::nothing_to_repair, "Empty inventory");
    req = request(source);
    req.scope = static_cast<Scope>(99);
    require(prepare(req, source, plan) == Code::invalid_request, "Unknown scope");
}
void invalid_snapshots() {
    struct Case { const char* label; Code expected; std::function<void(Snapshot&)> change; };
    const std::vector<Case> cases{
        {"Duplicate UID", Code::identity_mismatch, [](auto& s) { s.items[3].identity.instance = 1; }},
        {"Duplicate slot", Code::identity_mismatch, [](auto& s) { s.items[3].identity.slot = 1; }},
        {"Missing definition", Code::missing_definition, [](auto& s) { s.definitions.erase(s.definitions.begin()); }},
        {"Duplicate definition", Code::invalid_snapshot, [](auto& s) { s.definitions.push_back(s.definitions[0]); }},
        {"Signed maximum", Code::unsupported_maximum, [](auto& s) { s.definitions[0].maximum = 32768; }},
        {"Missing authority", Code::identity_mismatch, [](auto& s) { put(s.items[0].authority.bytes, 0, UINT64_MAX); }},
        {"Wrong presentation", Code::identity_mismatch, [](auto& s) { put(s.items[0].presentation.bytes, 0, std::uint64_t(90)); }},
        {"Consumed item", Code::identity_mismatch, [](auto& s) { put(s.items[0].authority.bytes, 0x10, std::int64_t(0)); }},
        {"Negative quantity", Code::identity_mismatch, [](auto& s) { put(s.items[0].authority.bytes, 0x10, std::int64_t(-1)); }},
        {"Empty item", Code::identity_mismatch, [](auto& s) { put(s.items[0].authority.bytes, 8, absent); }},
        {"Overfull current", Code::invalid_snapshot, [](auto& s) { for (auto* x : {&s.items[0].authority, &s.items[0].presentation}) put(x->bytes, 0x40, std::uint16_t(101)); }},
        {"Mirror durability drift", Code::mirrors_disagree, [](auto& s) { put(s.items[0].presentation.bytes, 0x40, std::uint16_t(34)); }},
        {"Mirror type drift", Code::mirrors_disagree, [](auto& s) { put(s.items[0].presentation.bytes, 8, std::uint16_t(20)); }},
        {"Mirror quantity drift", Code::mirrors_disagree, [](auto& s) { put(s.items[0].presentation.bytes, 0x10, std::int64_t(2)); }},
        {"Socket count exceeds capacity", Code::invalid_snapshot, [](auto& s) { put(s.items[2].authority.bytes, 0x68, std::uint32_t(5)); }},
        {"Truncated sockets", Code::invalid_snapshot, [](auto& s) { s.items[2].authority.sockets.pop_back(); }},
        {"Duplicate socket index", Code::invalid_snapshot, [](auto& s) { s.items[2].authority.sockets[2].bytes[4] = 0; }},
        {"Locked filled socket", Code::invalid_snapshot, [](auto& s) { s.items[2].authority.sockets[0].bytes[4] = 255; }},
        {"Socket mirror drift", Code::mirrors_disagree, [](auto& s) { s.items[2].presentation.sockets[0].bytes[2] = 1; }},
        {"Too many items", Code::invalid_snapshot, [](auto& s) { s.items.resize(max_items + 1); }},
    };
    for (const auto& scenario : cases) {
        auto source = fixture();
        Plan plan;
        require(prepare(request(source), source, plan) == Code::prepared, "Prime earlier plan");
        scenario.change(source);
        const auto before = source;
        require(prepare(request(source), source, plan) == scenario.expected, scenario.label);
        require(source == before && !plan.prepared() && plan.changes().empty(), "Refusal preserved snapshot/cleared plan");
    }
    auto source = fixture();
    Plan plan;
    auto req = request(source);
    ++req.session.generation;
    require(prepare(req, source, plan) == Code::stale, "Load invalidates queued identity");
    req.session.player = 0;
    require(prepare(req, source, plan) == Code::invalid_request, "Missing player");
    for (std::uint16_t maximum : {std::uint16_t(1), std::uint16_t(32767), absent}) {
        source = {{5, 77, 1}, {{10, maximum}}, {item(1, 10, 0)}};
        require(prepare(request(source), source, plan) ==
            (maximum == absent ? Code::nothing_to_repair : Code::prepared), "Maximum boundary");
        apply_to_owned_snapshot(plan, source);
        require(get<std::uint16_t>(source.items[0].authority.bytes, 0x40) ==
            (maximum == absent ? 0 : maximum), "Boundary result");
    }
    source = {{5, 77, 1}, {{10, 100}}, {item(1, 10, absent)}};
    require(prepare(request(source), source, plan) == Code::nothing_to_repair, "Instance sentinel preserved");
}
void stale_batch() {
    const std::vector<std::function<void(Snapshot&)>> edits{
        [](auto& s) { ++s.session.generation; },
        [](auto& s) { ++s.session.player; },
        [](auto& s) { ++s.session.catalog_revision; },
        [](auto& s) { ++s.definitions[0].maximum; },
        [](auto& s) { ++s.items[2].authority.bytes[0x38]; },
        [](auto& s) { ++s.items[2].presentation.bytes[0x38]; },
        [](auto& s) { ++s.items[2].authority.sockets[0].bytes[5]; },
        [](auto& s) { ++s.items[2].identity.slot; },
        [](auto& s) { s.items.erase(s.items.begin() + 2); },
    };
    for (const auto& edit : edits) {
        auto source = fixture();
        Plan plan;
        require(prepare(request(source), source, plan) == Code::prepared, "Prepare stale test");
        edit(source);
        const auto before = source;
        require(apply_to_owned_snapshot(plan, source) == Code::stale, "Late batch change refused");
        require(source == before, "No earlier item partially repaired on stale batch");
    }
    auto source = fixture();
    Plan plan;
    require(apply_to_owned_snapshot(plan, source) == Code::invalid_request, "Unprepared plan");
    require(prepare(request(source), source, plan) == Code::prepared, "Prepare reordered test");
    std::reverse(source.items.begin(), source.items.end());
    require(apply_to_owned_snapshot(plan, source) == Code::applied, "Identity, not array position, addresses item");
}
void no_wear_combination() {
    auto source = fixture();
    for (auto& def : source.definitions) {
        if (def.maximum != 0 && def.maximum != absent) {
            def.active_maximum = absent;
            def.no_wear_override = true;
        }
    }
    Plan plan;
    require(prepare(request(source), source, plan) == Code::prepared, "Repair with admitted no-wear override");
    require(plan.count() == Count{3, 2, 1} && apply_to_owned_snapshot(plan, source) == Code::applied,
        "No-wear still repairs finite roots and sockets to vanilla maxima");
    require(get<std::uint16_t>(source.items[0].authority.bytes, 0x40) == 100 &&
        get<std::uint16_t>(source.items[2].authority.sockets[0].bytes, 2) == 100,
        "Never write the no-wear sentinel as a repair amount");
    source = fixture();
    source.definitions[0].active_maximum = absent;
    require(prepare(request(source), source, plan) == Code::invalid_snapshot, "Unexplained live catalog override refused");
    source.definitions[0].no_wear_override = true;
    source.definitions[0].active_maximum = 500;
    require(prepare(request(source), source, plan) == Code::invalid_snapshot, "No-wear flag does not allow arbitrary maxima");
}
class Store final : public Transaction {
public:
    Snapshot data = fixture();
    unsigned captures = 0, commits = 0, notifications = 0;
    bool reject = false, uncertain = false, capture_throw = false, asynchronous = false;
    std::function<void()> on_commit;
    Snapshot capture() override {
        ++captures;
        if (capture_throw) throw std::runtime_error("Host cannot acquire inventory");
        return data;
    }
    Commit commit(const Plan& plan) noexcept override {
        ++commits;
        if (on_commit) on_commit();
        if (reject) return Commit::rejected;
        try {
            if (apply_to_owned_snapshot(plan, data) != Code::applied) return Commit::rejected;
            if (uncertain) return Commit::unknown;
            if (asynchronous) return Commit::pending;
            ++notifications; // fixture notification only; no game callback exists
            return Commit::applied;
        } catch (...) { return Commit::unknown; }
    }
};
void queue_tests() {
    Store store;
    Queue q(std::this_thread::get_id());
    const auto req = request(store.data);
    const auto now = std::chrono::steady_clock::now();
    const auto before = store.data;
    auto out = q.submit(req, now);
    require(out.code == Code::queued && out.request_id == 1, "Queue command");
    require(store.data == before, "Input does not mutate data");
    require(q.submit(req, now).code == Code::busy, "Coalesce repeated input");
    Outcome worker;
    std::thread wrong([&] { worker = q.execute(store, now); });
    wrong.join();
    require(worker.code == Code::wrong_thread && store.captures == 0, "No host callback from wrong thread");
    store.on_commit = [&] {
        require(q.submit(req).code == Code::busy, "No submission during commit");
        require(q.cancel().code == Code::busy, "No cancel during commit");
        require(q.execute(store).code == Code::busy, "Reentrant dispatch refused");
    };
    out = q.execute(store, now);
    require(out.code == Code::applied && out.count == Count{3, 2, 1} && out.request_id == 1, "Dispatch action");
    require(store.commits == 1 && store.notifications == 1, "One transaction and notification");
    require(q.execute(store, now).code == Code::no_request, "At most once execution");
    q.submit(req, now);
    require(q.execute(store, now).code == Code::nothing_to_repair && store.commits == 1, "No transaction for no-op");
    q.submit(req, now);
    require(q.cancel().code == Code::cancelled && q.execute(store, now).code == Code::no_request, "Cancel");
    q.submit(req, now);
    const auto captures = store.captures;
    require(q.execute(store, now + std::chrono::seconds(5)).code == Code::expired &&
        store.captures == captures, "Loading/menu pause expires command before host access");
    q.submit(req, now);
    ++store.data.session.generation;
    require(q.execute(store, now).code == Code::stale && store.commits == 1, "Command cannot cross world load");
    store = Store{};
    q.submit(req, now);
    store.reject = true;
    require(q.execute(store, now).code == Code::host_rejected && store.data == before, "Host rejects without writes");
    require(q.execute(store, now).code == Code::no_request, "Never auto-retry rejection");
    q.submit(req, now);
    store.capture_throw = true;
    require(q.execute(store, now).code == Code::host_rejected, "Capture exception releases queue");
    store.capture_throw = false;
    store.reject = false;
    store.uncertain = true;
    q.submit(req, now);
    require(q.execute(store, now).code == Code::outcome_unknown, "Uncertain mutation reported");
    require(q.submit(req, now).code == Code::fault_latched && q.execute(store, now).code == Code::fault_latched,
        "Uncertain engine result permanently disables this queue");
}
void concurrent_input() {
    Store store;
    Queue q(std::this_thread::get_id());
    std::atomic<unsigned> queued{0}, busy{0};
    std::vector<std::thread> workers;
    for (int i = 0; i < 16; ++i) workers.emplace_back([&] {
        const auto out = q.submit(request(store.data));
        if (out.code == Code::queued) ++queued;
        if (out.code == Code::busy) ++busy;
    });
    for (auto& worker : workers) worker.join();
    require(queued == 1 && busy == 15, "Concurrent input queues only one action");
    require(q.execute(store).code == Code::applied && store.commits == 1, "Concurrent input executes once");
}
void asynchronous_completion() {
    const auto now = std::chrono::steady_clock::now();
    for (int scenario = 0; scenario < 10; ++scenario) {
        Store store;
        store.asynchronous = true;
        Queue q(std::this_thread::get_id());
        const auto req = request(store.data);
        const auto id = q.submit(req, now).request_id;
        const auto result = q.execute(store, now);
        require(result.code == Code::awaiting_confirmation && result.request_id == id && result.count == Count{},
            "Dispatch acceptance is not reported as a completed repair");
        require(q.submit(req, now).code == Code::busy && q.cancel().code == Code::busy,
            "No second action or pretend cancellation while completion is pending");
        require(q.execute(store, now).code == Code::awaiting_confirmation && store.commits == 1,
            "Pending transaction is never replayed");
        require(q.poll(req.session, now).code == Code::awaiting_confirmation, "Pending tick");
        require(q.confirm(id + 1, req.session, store.data, true, now).code == Code::confirmation_mismatch,
            "Unrelated receipts cannot complete the command");
        auto old_session = req.session;
        --old_session.generation;
        require(q.confirm(id, old_session, store.data, true, now).code == Code::confirmation_mismatch,
            "Reused numeric ID in an old session cannot complete a new command");
        Outcome wrong;
        std::thread worker([&] { wrong = q.confirm(id, req.session, store.data, true, now); });
        worker.join();
        require(wrong.code == Code::wrong_thread, "Only dispatcher accepts completion");
        if (scenario == 0) {
            std::reverse(store.data.items.begin(), store.data.items.end());
            const auto out = q.confirm(id, req.session, store.data, true, now);
            require(out.code == Code::applied && out.count == Count{3, 2, 1}, "Confirmed fields and events complete action");
            require(q.confirm(id, req.session, store.data, true, now).code == Code::no_request, "Duplicate acknowledgement ignored");
            require(q.submit(req, now).code == Code::queued, "Confirmed success permits next command");
            continue;
        }
        if (scenario == 1) ++store.data.items[0].presentation.bytes[0x40];
        if (scenario == 2) ++store.data.items[2].authority.sockets[0].bytes[2];
        if (scenario == 3) ++store.data.session.generation;
        if (scenario == 4) ++store.data.definitions[0].maximum;
        if (scenario == 5) store.data.items.erase(store.data.items.begin());
        if (scenario == 6) ++store.data.items[0].authority.bytes[0x38];
        const auto observed = store.data;
        const auto out = scenario == 8 ? q.poll(req.session, now + std::chrono::seconds(5)) :
            scenario == 9 ? q.execute(store, now + std::chrono::seconds(5)) :
            q.confirm(id, req.session, store.data, scenario != 7, now);
        require(out.code == Code::outcome_unknown && out.count == Count{}, "Partial, late or failed completion is uncertain");
        require(store.data == observed && store.commits == 1, "No speculative rollback or retry");
        require(q.submit(req, now).code == Code::fault_latched && q.confirm(id, req.session, store.data, true, now).code ==
            Code::fault_latched && q.poll(req.session, now).code == Code::fault_latched, "Fault cannot be cleared by later success");
    }
    Store store;
    Queue q(std::this_thread::get_id());
    const auto req = request(store.data);
    q.submit(req, now);
    auto changed = req.session;
    ++changed.player;
    require(q.poll(changed, now).code == Code::stale && store.commits == 0, "World switch discards unstarted work");
    q.submit(req, now);
    require(q.poll(req.session, now + std::chrono::seconds(5)).code == Code::expired, "Tick expires unstarted work");
    store.asynchronous = true;
    q.submit(req, now); q.execute(store, now);
    require(q.poll(changed, now).code == Code::outcome_unknown, "World switch after acceptance faults the command");
    Plan plan;
    require(verify_applied(plan, store.data) == Code::invalid_request, "Unprepared confirmation refused");
}
}
int main() {
    try {
        happy_path(); selections(); invalid_snapshots(); stale_batch(); no_wear_combination(); queue_tests(); concurrent_input();
        asynchronous_completion(); equipment_notices();
        std::cout << "{\"ok\":true,\"checks\":" << checks
            << ",\"own_repair_action\":true,\"backend\":\"owned_fixture_images\","
            << "\"game_adapter\":false,\"game_process_access\":false,\"save_access\":false}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Repair action test failed: " << error.what() << '\n';
        return 1;
    }
}
