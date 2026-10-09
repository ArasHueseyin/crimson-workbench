// Original inventory repair acknowledgement on private objects. Its absolute
// main-word store is not a UID check, a socket notification or persistence.
// Container-key lookup, name formatting, UI and server delivery are fixtures.
unsigned inventory_event_cases = 0;
using InventoryAck = void* (*)(void*, std::uint32_t*, std::uint16_t, std::uint16_t, std::uint16_t);
struct InventoryEventProbe;
thread_local InventoryEventProbe* inventory_event_probe = nullptr;
struct InventoryEventProbe {
    Fixture& owners;
    InventoryFixture& client;
    alignas(8) std::array<std::uint8_t, 0x10> component{};
    std::array<std::uint8_t, 0x88> context{};
    std::array<std::uint8_t, 0x2d8> panels{};
    std::uint16_t mapped = reader::carried_container, wire_container = 102, slot = 0, target = 100, previous = 35;
    unsigned resolves = 0, formats = 0, messages = 0;
    bool valid = true;
    void* empty_string = nullptr;
    InventoryAck acknowledge;
    InventoryEventProbe(Fixture& f, InventoryFixture& v, Arena& arena)
        : owners(f), client(v), acknowledge(arena.function<InventoryAck>(0xa11ca0)) {
        put(component, 8, owners.actors[0].bytes.data());
        put(context, 0x80, panels.data()); put(panels, 0x2d0, this);
        empty_string = arena.function<void*>(0x6a65940);
        arena.pointer(0x6d691b0, reinterpret_cast<std::uintptr_t>(context.data()));
    }
    bool owned() const {
        for (const auto& actor : owners.actors)
            if (get<std::uint32_t>(actor.bytes, 0x10) != 1) return false;
        return true;
    }
    bool unlocked() const {
        if (!owned()) return false;
        for (auto& actor : owners.actors) {
            if (get<std::uint32_t>(actor.owner_lock, 0x2c)) return false;
            auto* lock = reinterpret_cast<SRWLOCK*>(actor.owner_lock.data() + 0x10);
            if (!TryAcquireSRWLockExclusive(lock)) return false;
            ReleaseSRWLockExclusive(lock);
        }
        return true;
    }
    static void* resolve(void* out, std::uint16_t input) noexcept {
        auto& p = *inventory_event_probe; ++p.resolves;
        p.valid &= input == p.wire_container && p.unlocked();
        std::memcpy(out, &p.mapped, 2); return out;
    }
    static void format(const void* item, void* text) noexcept {
        auto& p = *inventory_event_probe; ++p.formats;
        p.valid &= item == p.client.item.data() && p.owned() &&
            get<std::uint32_t>(p.owners.actors[0].owner_lock, 0x2c) == 1 &&
            !get<std::uint32_t>(p.owners.actors[1].owner_lock, 0x2c);
        // Empty fixture name stays in the original function's stack buffer.
        *static_cast<char*>(text) = '\0';
    }
    static void panel(void* receiver, std::uint32_t kind, std::uint16_t slot,
        void* const* text, std::uint16_t delta) noexcept {
        auto& p = *inventory_event_probe; ++p.messages;
        p.valid &= receiver == &p && kind == UINT32_MAX && slot == p.slot &&
            *text == p.empty_string && delta == std::uint16_t(p.target - p.previous) &&
            get<std::uint16_t>(p.client.item, 0x40) == p.target && p.unlocked();
    }
};
DWORD invoke_inventory_ack(InventoryEventProbe& p, std::uint32_t* result) {
    __try { p.acknowledge(p.component.data(), result, p.wire_container, p.slot, p.target); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
struct InventoryRoute {
    CaptureMemory& memory;
    InventoryEventProbe& probe;
    unsigned scenario = 0, admitted = 0, marked = 0, submitted = 0, finishes = 0;
    std::uint32_t result = UINT32_MAX;
    DWORD exception = 0;
    static reader::Frame frame(void* p) noexcept { return static_cast<InventoryRoute*>(p)->memory.frame; }
    static bool admit(void* p, const transaction::Scene& scene, const action::Change& change) noexcept {
        auto& r = *static_cast<InventoryRoute*>(p); ++r.admitted;
        return scene.frame == r.memory.frame && change.before.identity == action::Identity{11, action::Area::carried, 2, 0} &&
            scene.presentation.actor == reinterpret_cast<std::uintptr_t>(r.probe.owners.actors[0].bytes.data());
    }
    static bool mark(void* p, const transaction::Scene&, const transaction::Notice& notice) noexcept {
        auto& r = *static_cast<InventoryRoute*>(p); ++r.marked;
        if (!r.probe.owned() || !notice.authority_after || !notice.presentation_after ||
            notice.authority_after == notice.presentation_after) return false;
        for (const auto& actor : r.probe.owners.actors)
            if (get<std::uint32_t>(actor.owner_lock, 0x2c) != 1) return false;
        // Deliberate mutation after the writer tests fresh UID confirmation.
        if (r.scenario == 4) put(r.probe.client.item, 0, std::uint64_t(999));
        return true; // Server inventory marking/persistence is a fixture here.
    }
    static bool notify(void* p, const transaction::Scene&, const transaction::Notice& notice) noexcept {
        auto& r = *static_cast<InventoryRoute*>(p); ++r.submitted;
        if (!r.probe.unlocked()) return false;
        r.probe.target = item_read<std::uint16_t>(notice.presentation_after, 0x40);
        // The production field writer already set the absolute client word.
        // This native ack consequently emits delta 0; socket fields stay alone.
        r.probe.previous = get<std::uint16_t>(r.probe.client.item, 0x40);
        if (r.scenario == 5) return true; // Simulated missing transport delivery.
        ++native_calls; r.exception = invoke_inventory_ack(r.probe, &r.result);
        if (r.scenario == 7) { ++native_calls; r.exception |= invoke_inventory_ack(r.probe, &r.result); }
        return !r.exception && !r.result;
    }
    static bool finish(void* p, const transaction::Scene&) noexcept {
        auto& r = *static_cast<InventoryRoute*>(p); ++r.finishes;
        return r.scenario != 6 && r.probe.valid && r.probe.unlocked(); // Fixture persistence result only.
    }
};
void inventory_events_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    require(hash(pe.rva(0xbbee00, 0x379)) == "5ac29646aa893ceb1983e76c63425142ca493bfe464a398f663bfb5840a3505d",
        "Complete original repair-inventory packet handler anchor");
    pe.function(0xbbee00, 0x379);
    require(item_read<std::uint64_t>(pe.rva(0x5609728 + 0x10, 8).data(), 0) == 0x140bbee00ull &&
        pe.rva(0xbbf030, 7) == std::vector<std::uint8_t>{0x48,0x8b,0x89,0xd8,0,0,0},
        "RTTI packet handler resolves the repair component at actor parts +0xd8");
    arena.redirect(0x388d50, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::resolve));
    arena.redirect(0x240b540, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::format));
    arena.redirect(0x9a2030, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::panel));
    arena.pointer(0x6cdb98c, 911); // Invalid mapped inventory key.
    arena.pointer(0x6cf6c24, 912); // Missing/invalid item at slot.
    std::array<std::uint8_t, action::item_size> empty{};
    put(empty, 8, action::absent); arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty.data()));
    put(character_definition, 0xbe, action::absent);
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 14; ++scenario) {
        ++inventory_event_cases;
        Fixture f; fixture = &f; tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventories;
        for (std::size_t role = 0; role < 2; ++role) inventories[role].initialize(f.actors[role]);
        auto& client = inventories[0];
        InventoryEventProbe probe(f, client, arena); inventory_event_probe = &probe;
        if (scenario == 1) probe.previous = 0;
        if (scenario == 2) probe.previous = 100;
        put(client.item, 0x40, probe.previous);
        if (scenario == 3) probe.target = 0;
        if (scenario == 4) probe.target = action::absent;
        if (scenario == 5) probe.mapped = action::absent;
        if (scenario == 6) probe.mapped = 99;
        if (scenario == 7) probe.slot = action::absent;
        if (scenario == 8) probe.slot = 1;
        if (scenario == 9) put(client.item, 8, action::absent);
        if (scenario == 10 || scenario == 11) put(client.item, 0x10, std::int64_t(scenario == 10 ? 0 : -1));
        std::array<std::uint8_t, 12> exclusion{}; put(exclusion, 0, std::uint16_t(10));
        if (scenario == 12) { put(client.bag, 0x20, exclusion.data()); put(client.bag, 0x28, std::uint32_t(1)); }
        if (scenario == 13) put(client.item, 0, std::uint64_t(999));
        const auto before = client, server_before = inventories[1];
        auto expected = before;
        const bool accepted = scenario < 5 || scenario == 13;
        if (accepted) put(expected.item, 0x40, probe.target);
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client), s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        {
            reference::Pair refs; require(refs.acquire(sources) == reference::Code::acquired,
                "Inventory acknowledgement keeps native owner references");
            std::uint32_t result = UINT32_MAX; ++native_calls;
            const auto fault = invoke_inventory_ack(probe, &result);
            require(!fault && result == (accepted ? 0u : scenario == 5 ? 911u : 912u),
                ("Original inventory ack result, case " + std::to_string(scenario) + ", fault " + std::to_string(fault) + ", result " + std::to_string(result)).c_str());
            require(probe.valid && probe.resolves == 1 && probe.formats == unsigned(accepted) && probe.messages == unsigned(accepted),
                "Original ack formats under client lock then reports the exact delta after unlocking");
            require(client == expected && inventories[1] == server_before && probe.unlocked(),
                "Only absolute client main word changes; sockets, UID, metadata and server state preserved");
        }
        require(f.valid && !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Native acknowledgement owner/lock cleanup balances");
    }
    install_item_allocator(arena); event_item_methods = item_binding(arena); ItemHeap heap;
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++inventory_event_cases;
        Fixture f; fixture = &f; tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = static_cast<std::uint8_t>(scenario % 2);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventories;
        for (std::size_t role = 0; role < 2; ++role) {
            inventories[role].initialize(f.actors[role]);
            if (scenario == 1) put(inventories[role].item, 0x40, std::uint16_t(100));
        }
        InventoryEventProbe probe(f, inventories[0], arena); inventory_event_probe = &probe;
        if (scenario == 3) probe.mapped = action::absent;
        CaptureMemory memory(f, inventories);
        const std::array definitions{action::Definition{10, 100, scenario == 2 ? action::absent : std::uint16_t(100), scenario == 2}, action::Definition{20, 30}};
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client), s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
            lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
        reader::Capture before;
        require(reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, before) == reader::Code::captured,
            "Carried event plan captures original owned native sources");
        const action::Request request{before.snapshot.session, action::Scope::carried, {}};
        action::Plan plan; require(action::prepare(request, before.snapshot, plan) == action::Code::prepared, "Carried native plan");
        InventoryRoute route{memory, probe, scenario};
        transaction::Environment environment{&route, InventoryRoute::frame, {}, InventoryRoute::finish};
        environment.routes[0] = {&route, InventoryRoute::admit, InventoryRoute::mark, InventoryRoute::notify};
        transaction::Adapter adapter(memory, memory.frame, definitions, sources, bindings,
            {event_construct, event_assign, event_destroy}, environment);
        action::Queue queue(std::this_thread::get_id()); const auto now = std::chrono::steady_clock::now();
        const auto submitted = queue.submit(request, now), executed = queue.execute(adapter, now);
        require(submitted.code == action::Code::queued && !route.exception && probe.valid &&
            route.admitted == 1 && route.marked == 1 && route.submitted == 1,
            "Carried transaction uses prepared native copies and original client acknowledgement");
        require(executed.code == (scenario == 3 || scenario == 6 ? action::Code::outcome_unknown : action::Code::awaiting_confirmation),
            "Native client errors and finalizer failure cannot become repair success");
        reader::Capture after;
        const auto read = reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, after);
        require(read == (scenario == 4 ? reader::Code::identity_mismatch : reader::Code::captured),
            "Fresh source capture rejects UID replacement that original ack does not validate");
        const bool receipts = route.result == 0 && probe.messages == 1 && route.finishes == 1;
        const auto outcome = scenario == 3 || scenario == 6 ? executed : queue.confirm(submitted.request_id, request.session, after.snapshot, receipts, now);
        require(outcome.code == (scenario < 3 ? action::Code::applied : action::Code::outcome_unknown),
            "Only fresh fields and complete fixture receipts count; missing/duplicate UI never prove success");
        if (scenario < 3) require(action::verify_applied(plan, after.snapshot) == action::Code::applied,
            "Original carried ack preserves repaired sockets and every unrelated byte");
        else require(queue.submit(request, now).code == action::Code::fault_latched, "Carried failure prevents replay");
        require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls && f.valid &&
            !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Carried transaction releases native copies, references and locks");
    }
    inventory_event_probe = nullptr; fixture = nullptr;
}
