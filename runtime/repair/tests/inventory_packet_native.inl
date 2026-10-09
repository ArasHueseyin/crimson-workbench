// Original carried-repair packet serializer and stream selection, with private
// recipients, buffers and transport. The pooled branch is tested only when the
// fixture allocation fails; no positive pooled transport or live delivery claim.
unsigned inventory_packet_cases = 0;
using InventoryPacket = void (*)(void*, void*, const std::uint16_t*, const std::uint16_t*, const std::uint16_t*);
struct InventoryPacketProbe;
thread_local InventoryPacketProbe* inventory_packet_probe = nullptr;
struct InventoryPacketProbe {
    std::array<std::uint8_t, 0x300> tls{}, context{};
    std::array<std::uint8_t, 0x18> descriptor{}, writer{};
    std::array<std::array<std::uint8_t, 0x18>, 2> remote{}, channels{};
    std::array<void*, 2> channel_list{}, recipients{};
    std::array<std::uintptr_t, 3> descriptor_vtable{};
    std::array<std::uintptr_t, 2> writer_vtable{};
    std::array<std::uint8_t, 0x68> actor{};
    std::array<std::uint8_t, 16> payload{}, header{};
    std::array<std::uint8_t, 0x38> pool{};
    std::array<std::uint32_t, 4> widths{};
    std::uint16_t container = 0x4567, slot = 0x89ab, target = 0xcdef;
    unsigned scenario = 0, fills = 0, compares = 0, begins = 0, writes = 0, finishes = 0;
    unsigned pooled = 0, allocations = 0, logs = 0, length = 0;
    bool valid = true, write_failed = false;
    void* chosen = nullptr;
    InventoryPacket serialize = nullptr;
    InventoryPacketProbe(Arena& arena, const std::array<std::uint8_t, 0x200>& base_tls, unsigned which)
        : scenario(which), serialize(arena.function<InventoryPacket>(0x29ec2e0)) {
        std::copy(base_tls.begin(), base_tls.end(), tls.begin());
        put(tls, 0x250, context.data());
        descriptor_vtable[1] = reinterpret_cast<std::uintptr_t>(&fill);
        descriptor_vtable[2] = reinterpret_cast<std::uintptr_t>(&equal);
        writer_vtable[1] = reinterpret_cast<std::uintptr_t>(&append);
        put(descriptor, 0, descriptor_vtable.data()); descriptor[8] = 7;
        put(writer, 0, writer_vtable.data());
        for (std::size_t i = 0; i < 2; ++i) {
            remote[i][8] = 7; put(channels[i], 0, remote[i].data());
            channel_list[i] = channels[i].data(); recipients[i] = &remote[i];
        }
        put(context, 0x298, channel_list.data()); put(context, 0x2a0, std::uint32_t(2)); context[0x2ac] = 1;
        put(context, 0x2b0, recipients.data()); put(context, 0x2b8, std::uint32_t(99));
        put(actor, 0x60, std::uint32_t(0x10203040));
        payload.fill(0xa7); header.fill(0xc3);
        if (scenario == 2) { container = 0; slot = 0; target = 0; }
        if (scenario == 3) { container = UINT16_MAX; slot = UINT16_MAX; target = UINT16_MAX; }
        if (scenario == 7) context[0x2ac] = 0;
        if (scenario == 8 || scenario == 9) remote[0][8] = 8;
        if (scenario == 9) remote[1][8] = 8;
        put(pool, 0x30, this);
        arena.pointer(0x6d696a0, reinterpret_cast<std::uintptr_t>(pool.data()));
        arena.pointer(0x6cffd00, reinterpret_cast<std::uintptr_t>(tls.data()));
    }
    static void fill(void* descriptor, void* list) noexcept {
        auto& p = *inventory_packet_probe; ++p.fills;
        p.valid &= descriptor == p.descriptor.data() && list == p.context.data() + 0x2b0 &&
            get<std::uint32_t>(p.context, 0x2b8) == 0;
        put(p.context, 0x2b8, std::uint32_t(p.scenario == 4 ? 0 : p.scenario == 11 ? 2 : 1));
    }
    static bool equal(void* descriptor, void* candidate) noexcept {
        auto& p = *inventory_packet_probe; ++p.compares;
        p.valid &= descriptor == p.descriptor.data() &&
            (candidate == p.remote[0].data() || candidate == p.remote[1].data());
        return p.scenario != 10 && !(p.scenario == 12 && candidate == p.remote[0].data());
    }
    static void* begin(void* channel, void** header) noexcept {
        auto& p = *inventory_packet_probe; ++p.begins; p.chosen = channel;
        p.valid &= channel == p.channels[p.scenario == 8 || p.scenario == 12 ? 1 : 0].data();
        *header = p.scenario == 5 ? nullptr : p.header.data();
        return p.writer.data();
    }
    static void append(void* writer, const void* bytes, std::uint32_t size) noexcept {
        auto& p = *inventory_packet_probe;
        p.valid &= writer == p.writer.data() && p.writes < p.widths.size() &&
            size == (p.writes ? 2u : 4u) && size <= 10 - p.length;
        if (!p.valid) return;
        p.widths[p.writes++] = size;
        if (p.scenario == 6) { p.write_failed = true; return; }
        std::memcpy(p.payload.data() + p.length, bytes, size); p.length += size;
    }
    static void finish(void* channel) noexcept {
        auto& p = *inventory_packet_probe; ++p.finishes;
        p.valid &= channel == p.chosen && get<std::uint16_t>(p.header, 0) == 0xc02 && p.header[2] == 0xff;
    }
    static void pool_prepare() noexcept { ++inventory_packet_probe->pooled; }
    static void* no_pool_buffer(void* receipt, std::uint32_t size, void*, void* pool) noexcept {
        auto& p = *inventory_packet_probe; ++p.allocations;
        p.valid &= size == 15 && pool == &p && item_read<std::uintptr_t>(receipt, 0) == 0 &&
            item_read<std::uint32_t>(receipt, 8) == 0;
        return nullptr;
    }
    static void log(std::uint8_t severity, ...) noexcept {
        auto& p = *inventory_packet_probe; ++p.logs; p.valid &= severity == 1;
    }
    bool complete() const {
        return valid && !write_failed && begins == 1 && writes == 4 && finishes == 1 &&
            length == 10 && widths == std::array<std::uint32_t, 4>{4, 2, 2, 2} &&
            get<std::uint16_t>(header, 0) == 0xc02 && header[2] == 0xff;
    }
};
DWORD invoke_inventory_packet(InventoryPacketProbe& p, void* actor) {
    __try { p.serialize(actor, p.descriptor.data(), &p.container, &p.slot, &p.target); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void install_inventory_packet(Arena& arena) {
    arena.redirect(0x2961d70, reinterpret_cast<std::uintptr_t>(&InventoryPacketProbe::begin));
    arena.redirect(0x2961e20, reinterpret_cast<std::uintptr_t>(&InventoryPacketProbe::finish));
    arena.redirect(0x29621b0, reinterpret_cast<std::uintptr_t>(&InventoryPacketProbe::pool_prepare));
    arena.redirect(0x2771ac0, reinterpret_cast<std::uintptr_t>(&InventoryPacketProbe::no_pool_buffer));
    arena.redirect(0x35d8d0, reinterpret_cast<std::uintptr_t>(&InventoryPacketProbe::log));
    arena.pointer(0x6cd8eb0, 0); // Private logging token read on unavailable packet buffer.
}
struct InventoryPacketRoute {
    InventoryRoute base;
    InventoryPacketProbe& packet;
    unsigned scenario;
    DWORD exception = 0;
    static reader::Frame frame(void* p) noexcept { return InventoryRoute::frame(&static_cast<InventoryPacketRoute*>(p)->base); }
    static bool admit(void* p, const transaction::Scene& scene, const action::Change& change) noexcept {
        return InventoryRoute::admit(&static_cast<InventoryPacketRoute*>(p)->base, scene, change);
    }
    static bool mark(void* p, const transaction::Scene& scene, const transaction::Notice& notice) noexcept {
        return InventoryRoute::mark(&static_cast<InventoryPacketRoute*>(p)->base, scene, notice);
    }
    static bool notify(void* p, const transaction::Scene&, const transaction::Notice& notice) noexcept {
        auto& r = *static_cast<InventoryPacketRoute*>(p); auto& b = r.base; auto& packet = r.packet;
        ++b.submitted;
        if (!b.probe.unlocked()) return false;
        packet.container = b.probe.wire_container; packet.slot = b.probe.slot;
        packet.target = item_read<std::uint16_t>(notice.presentation_after, 0x40);
        auto* actor = b.probe.owners.actors[1].bytes.data();
        ++native_calls; r.exception = invoke_inventory_packet(packet, r.scenario == 9 ? nullptr : actor);
        if (r.exception || !packet.complete()) return false;
        // Private bounded decoder: the original outer packet reader is not used.
        // It would discard the inner acknowledgement's error, so no delivery or
        // request-correlation receipt is inferred merely from its return code.
        if (get<std::uint32_t>(packet.payload, 0) != get<std::uint32_t>(b.probe.owners.actors[0].bytes, 0x60)) return false;
        if (r.scenario == 5) return true; // Transport accepted, delivery absent.
        b.probe.wire_container = get<std::uint16_t>(packet.payload, 4);
        b.probe.slot = get<std::uint16_t>(packet.payload, 6);
        b.probe.target = get<std::uint16_t>(packet.payload, 8);
        b.probe.previous = get<std::uint16_t>(b.probe.client.item, 0x40);
        ++native_calls; b.exception = invoke_inventory_ack(b.probe, &b.result);
        if (r.scenario == 6) { ++native_calls; b.exception |= invoke_inventory_ack(b.probe, &b.result); }
        return !b.exception && !b.result;
    }
    static bool finish(void* p, const transaction::Scene& scene) noexcept {
        return InventoryRoute::finish(&static_cast<InventoryPacketRoute*>(p)->base, scene);
    }
};
void inventory_packet_transactions(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    install_item_allocator(arena); event_item_methods = item_binding(arena); ItemHeap heap;
    arena.restore(pe, 0xa11ca0);
    arena.redirect(0x388d50, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::resolve));
    arena.redirect(0x240b540, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::format));
    arena.redirect(0x9a2030, reinterpret_cast<std::uintptr_t>(&InventoryEventProbe::panel));
    arena.pointer(0x6cdb98c, 911); arena.pointer(0x6cf6c24, 912);
    std::array<std::uint8_t, action::item_size> empty{}; put(empty, 8, action::absent);
    arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty.data()));
    put(character_definition, 0xbe, action::absent);
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        ++inventory_packet_cases;
        Fixture f; fixture = &f; tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = static_cast<std::uint8_t>(scenario % 2);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventories;
        for (std::size_t role = 0; role < 2; ++role) {
            inventories[role].initialize(f.actors[role]);
            if (scenario == 1) put(inventories[role].item, 0x40, std::uint16_t(100));
        }
        InventoryEventProbe probe(f, inventories[0], arena); inventory_event_probe = &probe;
        if (scenario == 4) probe.mapped = action::absent;
        InventoryPacketProbe packet(arena, tls, scenario == 3 ? 4u : scenario == 8 ? 5u : 0u);
        inventory_packet_probe = &packet;
        CaptureMemory memory(f, inventories);
        const std::array definitions{action::Definition{10, 100, scenario == 2 ? action::absent : std::uint16_t(100), scenario == 2}, action::Definition{20, 30}};
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client), s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
            lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
        reader::Capture before;
        require(reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, before) == reader::Code::captured,
            "Packet-backed carried plan captures both protected native sources");
        const action::Request request{before.snapshot.session, action::Scope::carried, {}};
        action::Plan plan; require(action::prepare(request, before.snapshot, plan) == action::Code::prepared, "Packet-backed carried native plan");
        InventoryPacketRoute route{{memory, probe, scenario == 7 ? 4u : 0u}, packet, scenario};
        transaction::Environment environment{&route, InventoryPacketRoute::frame, {}, InventoryPacketRoute::finish};
        environment.routes[0] = {&route, InventoryPacketRoute::admit, InventoryPacketRoute::mark, InventoryPacketRoute::notify};
        transaction::Adapter adapter(memory, memory.frame, definitions, sources, bindings,
            {event_construct, event_assign, event_destroy}, environment);
        action::Queue queue(std::this_thread::get_id()); const auto now = std::chrono::steady_clock::now();
        const auto submitted = queue.submit(request, now), executed = queue.execute(adapter, now);
        const bool rejected = scenario == 3 || scenario == 4 || scenario == 8 || scenario == 9;
        require(submitted.code == action::Code::queued && !route.exception && !route.base.exception && probe.valid && packet.valid &&
            route.base.admitted == 1 && route.base.marked == 1 && route.base.submitted == 1,
            "Original packet serializer connects prepared repair to original client ack through private transport");
        require(executed.code == (rejected ? action::Code::outcome_unknown : action::Code::awaiting_confirmation),
            "Missing packet or actor mismatch or inner ack error cannot become repair success");
        reader::Capture after;
        const auto read = reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, after);
        require(read == (scenario == 7 ? reader::Code::identity_mismatch : reader::Code::captured),
            "Packet omits UID; fresh protected capture must still reject replaced items");
        const bool receipts = packet.complete() && route.base.result == 0 && probe.messages == 1 && route.base.finishes == 1;
        const auto outcome = rejected ? executed : queue.confirm(submitted.request_id, request.session, after.snapshot, receipts, now);
        require(outcome.code == (scenario < 3 ? action::Code::applied : action::Code::outcome_unknown),
            "Fresh fields plus complete private receipts required; absent or duplicated delivery rejected");
        if (scenario < 3) require(action::verify_applied(plan, after.snapshot) == action::Code::applied,
            "Native ten-byte acknowledgement preserves socket repairs and unrelated item state");
        else require(queue.submit(request, now).code == action::Code::fault_latched, "Packet failure after stores cannot replay repair");
        require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls && f.valid &&
            !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Packet transaction leaves no native copies, references or owner locks");
    }
    inventory_event_probe = nullptr; fixture = nullptr;
}
void inventory_packet_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    install_inventory_packet(arena);
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 13; ++scenario) {
        ++inventory_packet_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        InventoryPacketProbe probe(arena, tls, scenario); inventory_packet_probe = &probe;
        const auto actor_before = probe.actor;
        const auto descriptor_before = probe.descriptor;
        ++native_calls;
        require(!invoke_inventory_packet(probe, scenario == 1 ? nullptr : probe.actor.data()), "Original carried packet serializer executes privately");
        const bool pool = scenario == 7 || scenario == 9 || scenario == 10;
        const bool output = scenario != 4 && scenario != 5 && !pool;
        require(probe.valid && probe.fills == 1 && probe.pooled == unsigned(pool) && probe.allocations == unsigned(pool),
            "Original recipient and channel selection preserve fixture ABI");
        require(probe.compares == (scenario == 4 || scenario == 7 || scenario == 9 ? 0u : scenario == 10 || scenario == 12 ? 2u : 1u),
            "Native channel selection checks type before descriptor equality");
        require(probe.logs == unsigned(pool || scenario == 5) && probe.begins == unsigned(!pool && scenario != 4) &&
            probe.finishes == unsigned(output) && probe.writes == (output ? 4u : 0u),
            "No recipients or unavailable buffers cannot produce a complete packet");
        require(probe.complete() == (output && scenario != 6), "Void serializer and finalization alone do not prove successful writes");
        auto expected = std::array<std::uint8_t, 16>{}; expected.fill(0xa7);
        if (output && scenario != 6) {
            put(expected, 0, scenario == 1 ? 0u : get<std::uint32_t>(probe.actor, 0x60));
            put(expected, 4, probe.container); put(expected, 6, probe.slot); put(expected, 8, probe.target);
        }
        auto expected_header = std::array<std::uint8_t, 16>{}; expected_header.fill(0xc3);
        if (output) { put(expected_header, 0, std::uint16_t(0xc02)); expected_header[2] = 0xff; }
        require(probe.payload == expected && probe.header == expected_header && probe.actor == actor_before &&
            probe.descriptor == descriptor_before, "Exact ten-byte payload, three header bytes, and untouched guards/sources");
    }
    inventory_packet_transactions(pe, arena, tls);
    arena.pointer(0x6cffd00, reinterpret_cast<std::uintptr_t>(tls.data()));
    inventory_packet_probe = nullptr;
    pe.verify_unchanged();
}
