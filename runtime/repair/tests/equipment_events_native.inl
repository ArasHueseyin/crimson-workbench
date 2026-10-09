// Private 2949 equipment-event integration. Original server notifier and client
// acknowledgement; transport, effect processor, child list and UI are fixtures.
// Production transaction prepares both native after-copies before source stores,
// marks under locks, dispatches after unlocking and consumes the dirty table.
// Game host, carried-item routes and actual persistence remain fixture dependencies.
unsigned event_cases = 0;
using ServerNotice = void* (*)(void*, std::uint32_t*, const void*, std::uint16_t,
    std::int16_t, bool, const void*);
using ClientNotice = void* (*)(void*, std::uint32_t*, std::uint16_t, std::int16_t, const void*);
struct EventProbe {
    Fixture* owners = nullptr;
    void* client_component = nullptr;
    const void* server_component = nullptr;
    const void* after_item = nullptr;
    void* child_actor = nullptr;
    ClientNotice acknowledge = nullptr;
    unsigned scenario = 0, packets = 0, effects = 0, child_events = 0, child_lists = 0;
    unsigned panels = 0, sockets = 0, appends = 0, frees = 0, effect_kind = 0;
    std::uint32_t client_error = UINT32_MAX;
    std::array<std::uint8_t, 12> removed{};
    bool abi = true;
};
thread_local EventProbe* event_probe = nullptr;
template<class T> T event_read(const void* p, std::size_t at) noexcept {
    T result;
    std::memcpy(&result, static_cast<const std::uint8_t*>(p) + at, sizeof(T));
    return result;
}
bool event_ownership() noexcept {
    bool ok = event_probe->server_component && event_read<std::uint32_t>(event_probe->server_component, 0x1e8) == 1;
    for (auto& a : event_probe->owners->actors) {
        ok &= get<std::uint32_t>(a.bytes, 0x10) == 1 && !get<std::uint32_t>(a.owner_lock, 8) &&
            !get<std::uint32_t>(a.owner_lock, 0x2c);
        auto* lock = reinterpret_cast<SRWLOCK*>(a.owner_lock.data() + 0x10);
        const auto available = TryAcquireSRWLockExclusive(lock);
        if (available) ReleaseSRWLockExclusive(lock);
        ok &= available != 0;
    }
    return ok;
}
std::uint64_t event_time() noexcept { return 123; }
void event_effect(void* receiver, const void* item, std::uint8_t kind, std::uint16_t slot) noexcept {
    auto& p = *event_probe;
    ++p.effects; p.effect_kind = kind;
    p.abi &= receiver == &p && item == p.after_item && kind == 3 && slot == 3 && event_ownership();
}
void event_children(void* receiver, const std::uint16_t* slot, void* list) noexcept {
    auto& p = *event_probe;
    p.abi &= receiver == &p && *slot == 3 && event_read<std::uint32_t>(list, 8) == 0 &&
        event_read<std::uint32_t>(list, 12) == 10;
    auto* data = event_read<std::uint8_t*>(list, 0);
    std::memset(data, 0, 32);
    std::memcpy(data + 8, &p.child_actor, 8);
    const std::uint32_t count = 1;
    std::memcpy(static_cast<std::uint8_t*>(list) + 8, &count, 4);
}
void event_children_dispose(void* list) noexcept {
    auto& p = *event_probe;
    ++p.child_lists;
    p.abi &= event_read<std::uint32_t>(list, 8) == (p.child_actor ? 1u : 0u);
    // Native notifier's stack buffer contains fixture references only.
}
void* event_child(void*, std::uint32_t* out, const void* event) noexcept {
    auto& p = *event_probe;
    ++p.child_events;
    p.abi &= event_read<std::uint32_t>(event, 0) == 77 &&
        event_read<std::uint32_t>(event, 0x10) == 0x12346 &&
        event_read<std::uint32_t>(event, 0x14) == 0x12346 &&
        event_read<std::uint64_t>(event, 0x18) == 223 && event_ownership();
    *out = 0; return out;
}
void event_append(void* list, const void* socket) noexcept {
    auto& p = *event_probe;
    const auto count = event_read<std::uint32_t>(list, 8);
    if (count >= 2) { p.abi = false; return; }
    std::memcpy(p.removed.data() + count * 6, socket, 6);
    auto* data = p.removed.data();
    std::memcpy(list, &data, 8);
    const auto next = count + 1;
    std::memcpy(static_cast<std::uint8_t*>(list) + 8, &next, 4);
    ++p.appends;
}
void event_free(void* pointer, unsigned mode) noexcept {
    if (item_heap && item_heap->owns(pointer)) { item_heap->free(pointer, mode); return; }
    ++event_probe->frees;
    event_probe->abi &= pointer == event_probe->removed.data();
}
void event_free0(void* pointer) noexcept { event_free(pointer, 0); }
void event_free1(void* pointer) noexcept { event_free(pointer, 1); }
void* event_item_key(void* out, std::uint32_t key) noexcept {
    const auto small = static_cast<std::uint16_t>(key);
    std::memcpy(out, &small, 2); return out;
}
void event_socket_ui(void*, std::uint16_t item, std::uint16_t socket) noexcept {
    ++event_probe->sockets;
    event_probe->abi &= item == 10 && socket == 20 && event_ownership();
}
void event_panel(void* receiver) noexcept {
    ++event_probe->panels;
    event_probe->abi &= receiver == event_probe && event_ownership();
}
void event_packet(void* owner, const void* recipient, const std::uint16_t* slot,
    const std::int16_t* delta, const void* removed) noexcept {
    auto& p = *event_probe;
    ++p.packets;
    p.abi &= owner == p.owners->actors[1].bytes.data() && event_read<void*>(recipient, 0x10) == owner &&
        event_read<std::uint8_t>(recipient, 8) == 1 && *slot == 3 && *delta == 0 &&
        event_read<std::uint32_t>(removed, 8) == 0 && event_ownership();
    if (p.scenario == 6) return; // Missing delivery is not a completed repair.
    std::array<std::uint8_t, 16> incorrect{};
    if (p.scenario == 7) { put(incorrect, 0, p.removed.data()); put(incorrect, 8, std::uint32_t(1)); }
    const auto* report = p.scenario == 7 ? incorrect.data() : removed;
    ++native_calls;
    p.acknowledge(p.client_component, &p.client_error, *slot, *delta, report);
    if (p.scenario == 13) { // Duplicate delivery must not become a success receipt.
        ++native_calls;
        p.acknowledge(p.client_component, &p.client_error, *slot, *delta, report);
    }
}
DWORD invoke_event(ServerNotice fn, void* component, std::uint32_t* out,
    const void* item, bool broken, const void* removed) {
    __try { fn(component, out, item, 3, 0, broken, removed); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
thread_local item::Binding event_item_methods;
void* event_construct(void* p) noexcept { ++native_calls; return event_item_methods.construct(p); }
void* event_assign(void* p, const void* source) noexcept { ++native_calls; return event_item_methods.assign(p, source); }
void event_destroy(void* p) noexcept { ++native_calls; event_item_methods.destroy(p); }
struct EventRoute {
    CaptureMemory* memory;
    EventProbe* probe;
    InventoryFixture* client;
    ServerNotice server_notice;
    unsigned admitted = 0, marked = 0, submitted = 0;
    std::uint32_t server_error = UINT32_MAX;
    DWORD exception = 0;
    Arena* arena = nullptr;
    const std::array<std::uint8_t, 0x200>* tls = nullptr;
    ConsumerProbe* consumer = nullptr;
    std::pair<DWORD, std::uint32_t> persistence{0, UINT32_MAX};
    static reader::Frame frame(void* p) noexcept { return static_cast<EventRoute*>(p)->memory->frame; }
    static bool admit(void* p, const transaction::Scene& scene, const action::Change& change) noexcept {
        auto& r = *static_cast<EventRoute*>(p); ++r.admitted;
        return scene.frame == r.memory->frame && change.before.identity == action::Identity{22, action::Area::equipped, 0, 3} &&
            reinterpret_cast<const void*>(scene.authority.equipment) == r.probe->server_component;
    }
    static bool mark(void* p, const transaction::Scene& scene, const transaction::Notice& notice) noexcept {
        auto& r = *static_cast<EventRoute*>(p); ++r.marked;
        for (const auto& a : r.probe->owners->actors)
            if (get<std::uint32_t>(a.owner_lock, 0x2c) != 1 || get<std::uint32_t>(a.bytes, 0x10) != 1) return false;
        if (!notice.authority_after || !notice.presentation_after || notice.authority_after == notice.presentation_after ||
            event_read<std::uint16_t>(notice.authority_after, 0x40) != event_read<std::uint16_t>(notice.change.after.authority.bytes.data(), 0x40) ||
            event_read<std::uint16_t>(notice.presentation_after, 0x40) != event_read<std::uint16_t>(notice.change.after.presentation.bytes.data(), 0x40)) return false;
        ++native_calls;
        if (invoke_slot_changed(reinterpret_cast<void*>(scene.authority.equipment), notice.change.before.identity.slot)) return false;
        // Deliberate post-store fixture corruption tests independent confirmation.
        // The production writer has already repaired BOTH client and server.
        if (r.probe->scenario == 8) put(r.client->equipped, 0xc8, std::uint16_t(99));
        if (r.probe->scenario == 9 || r.probe->scenario == 10) {
            std::copy_n(notice.change.before.presentation.bytes.begin() + 0x40, 2, r.client->equipped.begin() + 0x40);
            for (std::size_t i = 0; i < notice.change.before.presentation.sockets.size(); ++i)
                std::copy_n(notice.change.before.presentation.sockets[i].bytes.begin() + 2, 2, r.client->sockets.begin() + i * 6 + 2);
        }
        return true;
    }
    static bool notify(void* p, const transaction::Scene& scene, const transaction::Notice& notice) noexcept {
        auto& r = *static_cast<EventRoute*>(p); ++r.submitted;
        r.probe->after_item = notice.authority_after;
        if (!event_ownership()) return false;
        std::array<std::uint8_t, 16> removed{}, removed_view{}; put(removed_view, 8, removed.data());
        ++native_calls;
        r.exception = invoke_event(r.server_notice, reinterpret_cast<void*>(scene.authority.equipment),
            &r.server_error, notice.authority_after, notice.equipment.was_broken, removed_view.data());
        return !r.exception && !r.server_error; // Delivery receipt is deliberately a separate obligation.
    }
    static bool finish(void* p, const transaction::Scene&) noexcept {
        auto& r = *static_cast<EventRoute*>(p);
        r.persistence = consume_fixture(*r.arena, *r.tls, *r.consumer);
        return !r.persistence.first && !r.persistence.second && r.consumer->valid;
    }
};
void equipment_events_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    require(IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE) != 0, "Original notifier requires enabled AVX");
    install_item_allocator(arena);
    event_item_methods = item_binding(arena);
    ItemHeap heap;
    // Both original virtual entries and the wrapper call order are pinned by
    // the EXE/function hashes; the dirty-slot path remains a separate obligation.
    require(event_read<std::uint64_t>(pe.rva(0x5b28f80 + 0x190, 8).data(), 0) == 0x142adfda0ull,
        "Current concrete server equipment notification vtable");
    for (const auto [rva, callback] : std::array<std::pair<std::uint32_t, std::uintptr_t>, 10>{{
        {0x1416c10, reinterpret_cast<std::uintptr_t>(&event_time)},
        {0x17b1d30, reinterpret_cast<std::uintptr_t>(&event_effect)},
        {0x20bf240, reinterpret_cast<std::uintptr_t>(&event_children)},
        {0x38a1a0, reinterpret_cast<std::uintptr_t>(&event_children_dispose)},
        {0x29ead70, reinterpret_cast<std::uintptr_t>(&event_packet)},
        {0x10d2b40, reinterpret_cast<std::uintptr_t>(&event_append)},
        {0x47f026c, reinterpret_cast<std::uintptr_t>(&event_free0)},
        {0x47f036c, reinterpret_cast<std::uintptr_t>(&event_free1)},
        {0x38aac0, reinterpret_cast<std::uintptr_t>(&event_item_key)},
        {0x4324a0, reinterpret_cast<std::uintptr_t>(&event_socket_ui)}}}) arena.redirect(rva, callback);
    arena.redirect(0x423990, reinterpret_cast<std::uintptr_t>(&event_panel));
    arena.pointer(0x6d69458, 100); // Fixture world clock offset and event number.
    arena.pointer(0x6cf94d0, (std::uint64_t(77) << 32) | 76); // broken / repaired IDs
    arena.pointer(0x6ce88d8, 0x3f800000); arena.pointer(0x6ce8888, 0);
    arena.write(0x5545e98, pe.rva(0x5545e98, 12), PAGE_READONLY);
    arena.pointer(0x6cf7248, 902); arena.pointer(0x6cf7d4c, 1);
    std::array<std::uint8_t, action::item_size> empty{};
    put(empty, 8, action::absent); arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty.data()));
    const auto now = std::chrono::steady_clock::now();
    for (unsigned scenario = 0; scenario < 18; ++scenario) {
        ++event_cases;
        definition_query_count = 0;
        Fixture f; fixture = &f;
        tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = std::uint8_t(scenario == 10);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventories;
        EventProbe probe; event_probe = &probe; probe.owners = &f; probe.scenario = scenario;
        probe.acknowledge = arena.function<ClientNotice>(0x98fb70);
        std::array<std::uint8_t, 0x178> server_parts{};
        std::array<std::uint8_t, 0x90> child{};
        std::array<std::uint8_t, 0x38> child_parts{};
        std::array<std::uint8_t, 2> child_type{0,7};
        std::array<std::uintptr_t, 0x748 / 8> child_methods{};
        auto* child_vtable = child_methods.data();
        if (scenario == 2) {
            probe.child_actor = child.data();
            put(child, 0x68, child_parts.data()); put(child, 0x88, child_type.data());
            put(child_parts, 0x30, &child_vtable);
            child_methods[0x740 / 8] = reinterpret_cast<std::uintptr_t>(&event_child);
            put(server_parts, 0x170, &probe);
        }
        const auto maximum = scenario == 4 ? action::absent : std::uint16_t(100);
        const auto active = scenario == 3 ? action::absent : maximum;
        put(main_definition, 0, std::uint32_t(10)); put(main_definition, 0x400, active);
        put(main_definition, 0x250, std::uint32_t(scenario == 14 ? 0 : 1));
        put(socket_definition, 0x400, std::uint16_t(30));
        for (std::size_t role = 0; role < 2; ++role) {
            auto& v = inventories[role]; v.initialize(f.actors[role]);
            // Each native Item owns its own vectors. Do not alias carried and
            // equipped socket buffers in this event fixture.
            put(v.item, 0x60, std::uintptr_t(0)); put(v.item, 0x68, std::uint32_t(0));
            put(v.item, 0x6c, std::uint32_t(0)); v.item[0x70] = 0;
            put(v.equipped, 8, std::uint16_t(10));
            put(v.equipped, 0x40, std::uint16_t(scenario == 1 ? 35 : scenario == 4 ? 17 : scenario == 5 ? 100 : 0));
            put(v.equipped, 0x60, v.sockets.data()); put(v.equipped, 0x68, std::uint32_t(2));
            put(v.equipped, 0x6c, std::uint32_t(2)); v.equipped[0x70] = 2;
            if (scenario == 10) put(v.sockets, 2, std::uint16_t(0));
        }
        auto& client = inventories[0]; auto& server = inventories[1];
        DirtyComponent dirty;
        std::copy(server.equipment.begin(), server.equipment.end(), dirty.bytes.begin());
        probe.server_component = dirty.bytes.data();
        put(server_parts, 0x20, server.status.data()); put(server.status, 0x18, &probe);
        put(server_parts, 0x38, dirty.bytes.data());
        put(server_parts, 0xb8, server.holder.data());
        put(f.actors[1].bytes, 0x68, server_parts.data());
        probe.client_component = client.equipment.data(); probe.after_item = server.equipped.data();
        std::array<std::uint8_t, 0x88> context{};
        std::array<std::uint8_t, 0x5d0> ui{};
        put(context, 0x80, ui.data()); put(ui, 0x4e8, &probe); put(ui, 0x5c8, &probe);
        arena.pointer(0x6d691b0, reinterpret_cast<std::uintptr_t>(context.data()));
        CaptureMemory memory(f, inventories); memory.allow(server_parts); memory.allow(dirty.bytes);
        const std::array definitions{action::Definition{10, maximum, active, scenario == 3}, action::Definition{20, 30}};
        const action::Identity identity{22, action::Area::equipped, 0, 3};
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client);
        registry::Source s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
            lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
        {
            reader::Capture before;
            require(reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, before) == reader::Code::captured,
                "Event plan uses fresh production capture with native owners and locks");
            action::Plan plan;
            const action::Request request{before.snapshot.session, action::Scope::equipped, {}};
            require(action::prepare(request, before.snapshot, plan) == action::Code::prepared, "Native event repair plan");
            action::EquipmentNotice notice;
            require(action::equipment_notice(plan, identity, notice) == action::Code::prepared && !notice.is_broken,
                "Original notification receives saved before-state from production planner");
            EventRoute route{&memory, &probe, &client, arena.function<ServerNotice>(0x2adfda0)};
            ConsumerProbe consumer(f, dirty); consumer.bind(arena);
            if (scenario == 15) consumer.capacity = 0;
            if (scenario == 16) consumer.execute_result = 1;
            if (scenario == 17) { consumer.execute_result = -1; put(consumer.request, 0x50, std::int32_t(-1)); }
            route.arena = &arena; route.tls = &tls; route.consumer = &consumer;
            transaction::Environment environment{&route, EventRoute::frame, {}, EventRoute::finish};
            environment.routes[1] = {&route, EventRoute::admit, EventRoute::mark, EventRoute::notify};
            transaction::Adapter adapter(memory, memory.frame, definitions, sources, bindings,
                {event_construct, event_assign, event_destroy}, environment);
            action::Queue queue(std::this_thread::get_id());
            const auto submitted = queue.submit(request, now);
            const auto executed = queue.execute(adapter, now);
            require(submitted.code == action::Code::queued && executed.code == (scenario >= 15 ? action::Code::outcome_unknown : action::Code::awaiting_confirmation) &&
                adapter.report().stage == (scenario >= 15 ? transaction::Stage::uncertain : transaction::Stage::submitted) && adapter.report().prepared_items == 1 &&
                route.admitted == 1 && route.marked == 1 && route.submitted == 1,
                "Production transaction prepares native copies, writes both realms, marks and submits before awaiting receipts");
            require(route.exception == 0, "Original server event and client acknowledgement complete on private objects");
            require(!route.persistence.first && route.persistence.second == (scenario == 15 ? 901u : scenario == 16 ? 902u : scenario == 17 ? 903u : 0u) &&
                consumer.valid && consumer.acquisitions == 1 && consumer.executions == unsigned(scenario != 15) &&
                consumer.uids[0] == 22 && consumer.endurance[0] == item_read<std::uint16_t>(server.equipped.data(), 0x40) &&
                adapter.report().finalized == (scenario < 15), "Batch finalization consumes actual server mark and preserves persistence failures after UI acknowledgement");
            const bool transitioned = notice.was_broken;
            require(route.server_error == 0 && probe.packets == 1 && probe.abi, "Native server dispatch ABI, ownership and return value");
            require(probe.effects == unsigned(transitioned && scenario != 14) &&
                probe.child_lists == unsigned(transitioned) && probe.child_events == unsigned(scenario == 2),
                ("Original repaired transition/child event, scenario " + std::to_string(scenario)).c_str());
            require(probe.client_error == (scenario == 6 ? UINT32_MAX : scenario == 7 || scenario == 10 ? 902u : 0u),
                "Current acknowledgement failure/missing delivery is preserved");
            const auto panels = scenario == 6 || scenario == 7 || scenario == 8 || scenario == 10 ? 0u : scenario == 13 ? 2u : 1u;
            require(probe.panels == panels && !probe.sockets && probe.appends == unsigned(scenario == 10) &&
                probe.frees == unsigned(scenario == 10), "Ack validates removals before panel refresh; native collector may mutate first");
            reader::Capture refreshed;
            const auto read_result = reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, refreshed);
            const auto expected_read = scenario == 8 ? reader::Code::identity_mismatch :
                scenario == 9 || scenario == 10 ? reader::Code::invalid_layout : reader::Code::captured;
            require(read_result == expected_read,
                ("Fresh ownership confirms source data or refuses changed slot identity, scenario " + std::to_string(scenario) +
                    ", code " + std::to_string(static_cast<unsigned>(read_result))).c_str());
            auto observed = refreshed.snapshot;
            if (scenario == 12) ++observed.session.generation;
            const bool receipts = route.server_error == 0 && probe.client_error == 0 && probe.panels == 1 && scenario != 11;
            const auto outcome = scenario >= 15 ? executed : scenario == 6 ? queue.poll(request.session, now + std::chrono::seconds(5)) :
                queue.confirm(submitted.request_id, request.session, observed, receipts, now);
            const bool success = scenario < 6 || scenario == 14;
            require(outcome.code == (success ? action::Code::applied : action::Code::outcome_unknown) &&
                outcome.count == (success ? plan.count() : action::Count{}),
                "Only matching fields plus complete callback receipts count as fixture repair success");
            if (!success) require(queue.submit(request, now).code == action::Code::fault_latched,
                "Event/field failure permanently prevents automatic repair replay");
            if (success) require(action::verify_applied(plan, observed) == action::Code::applied,
                "Native notification leaves all repaired values and unrelated bytes intact");
            require(heap.live() == 1 && heap.valid, "Native after-copies and dirty payloads released; only reusable node storage remains");
            dirty.check(heap, {}, true);
            dirty.cleanup(heap, tls[0x1fd]);
            require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls,
                "Fixture teardown releases dirty entries separately from native item destruction");
        }
        require(f.valid && !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Native actor references released after all fixture event callbacks");
        require(layout_callbacks_ok, "Native item definition queries remain inside the fixture catalog");
    }
    event_probe = nullptr; fixture = nullptr;
}
