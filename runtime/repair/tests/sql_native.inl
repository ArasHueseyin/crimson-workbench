// The complete original SQL execute shim is NOT a persistence backend. Its
// bypass path returns zero without even reading its request. No database/save
// is opened here; time/log callbacks and every request/context are private.
unsigned sql_cases = 0;
using SqlExecute = int (*)(void*);
using SqlAcquire = void* (*)(void*, void*);
struct SqlProbe;
thread_local SqlProbe* sql_probe = nullptr;
struct SqlProbe {
    Arena& arena;
    alignas(8) std::array<std::uint8_t, 0x200> request{};
    alignas(8) std::array<std::uint8_t, 0x40> backend{};
    unsigned clocks = 0, logs = 0;
    bool valid = true, bypass_during_clock = false;
    explicit SqlProbe(Arena& a) : arena(a) {
        put(request, 0x10, backend.data()); put(request, 0x28, request.data() + 0x100);
        put(request, 0x48, std::uint64_t(0x12345678)); put(request, 0x50, std::int32_t(-7));
    }
    static std::uint64_t clock() noexcept {
        auto& p = *sql_probe; ++p.clocks;
        if (p.bypass_during_clock) p.arena.pointer(0x6ceee08, 1);
        return 100;
    }
    static void log(void* backend, void* alternate, const void*, const void*, unsigned line, const void*) noexcept {
        auto& p = *sql_probe; ++p.logs;
        p.valid &= backend == p.backend.data() && alternate == p.request.data() + 0xb0 && line == 0x151 &&
            get<std::uintptr_t>(p.request, 0x28) == reinterpret_cast<std::uintptr_t>(alternate);
        // The original shim must clear this after its diagnostic callback.
        put(p.request, 0x50, std::int32_t(-99)); put(p.request, 0x48, std::uint64_t(99));
    }
};
DWORD invoke_sql(SqlExecute fn, void* request, int* result) {
    __try { *result = fn(request); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void sql_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    // Earlier tests replace these dependencies. Restore the pinned full bodies
    // here without changing any byte other than the manifest's TLS sites.
    // They have no TLS substitutions themselves.
    arena.restore(pe, 0x2776d00); arena.restore(pe, 0x2c1da50);
    arena.redirect(0x1416b80, reinterpret_cast<std::uintptr_t>(&SqlProbe::clock));
    arena.redirect(0x2776e40, reinterpret_cast<std::uintptr_t>(&SqlProbe::log));
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++sql_cases; SqlProbe probe(arena); sql_probe = &probe;
        arena.pointer(0x6ceee08, scenario < 2 ? (scenario == 1 ? 255 : 1) : 0);
        if (scenario >= 3 && scenario <= 6) {
            put(probe.backend, 0x34, std::uint32_t(5));
            put(probe.backend, 0x38, std::uint64_t(scenario == 3 ? 200 : scenario == 4 ? 100 : 99));
        }
        if (scenario == 6) probe.bypass_during_clock = true;
        if (scenario == 7) put(probe.request, 0x10, std::uintptr_t(0));
        auto expected_request = probe.request;
        auto expected_backend = probe.backend;
        if (scenario >= 2) {
            put(expected_request, 0x48, std::uint64_t(4)); put(expected_request, 0x50, std::int32_t(0));
            if (scenario >= 3 && scenario <= 6) put(expected_backend, 0x38, std::uint64_t(105));
        }
        int result = -123; ++native_calls;
        const auto fault = invoke_sql(arena.function<SqlExecute>(0x2776d00), scenario == 1 ? nullptr : probe.request.data(), &result);
        require(fault == (scenario == 7 ? EXCEPTION_ACCESS_VIOLATION : 0u) &&
            result == (scenario < 2 ? 0 : scenario == 7 ? -123 : 1),
            ("Complete SQL shim result/unwind, case " + std::to_string(scenario) + ", fault " + std::to_string(fault)).c_str());
        require(probe.valid && probe.request == expected_request && probe.backend == expected_backend &&
            probe.clocks == unsigned(scenario >= 3 && scenario <= 6) && probe.logs == unsigned(scenario == 4 || scenario == 5),
            "Bypass ignores request; non-bypass only clears status, throttles diagnostics and returns one");
    }
    sql_probe = nullptr;
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        ++sql_cases;
        std::array<std::uint8_t, 0x90> manager{};
        std::array<std::uint8_t, 0x80> request{}; put(request, 0x50, std::int32_t(-77));
        const auto before = request;
        const auto pointer = scenario == 1 ? std::uintptr_t(0) : reinterpret_cast<std::uintptr_t>(request.data());
        put(manager, 0x88, pointer); const auto manager_before = manager;
        std::array<std::uint8_t, 32> out{}; out.fill(0xc7);
        const auto acquire = arena.function<SqlAcquire>(0x2c1da50);
        ++native_calls;
        require(acquire(manager.data(), out.data()) == out.data() && get<std::uintptr_t>(out, 8) == pointer &&
            get<std::uintptr_t>(out, 0) == reinterpret_cast<std::uintptr_t>(arena.function<void*>(0x5b30128)),
            "Original request acquisition wraps the exact manager-owned pointer, including null");
        if (scenario == 2) { ++native_calls; acquire(manager.data(), out.data()); }
        require(request == before && manager == manager_before &&
            std::all_of(out.begin() + 16, out.end(), [](auto b) { return b == 0xc7; }),
            "Request acquisition neither resets stale error state nor allocates independent request storage");
    }
    install_item_allocator(arena); ItemHeap heap;
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 4; ++scenario) {
        ++sql_cases;
        Fixture f; fixture = &f; tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventory;
        for (std::size_t role = 0; role < 2; ++role) inventory[role].initialize(f.actors[role]);
        put(inventory[1].equipped, 0x40, std::uint16_t(100));
        DirtyComponent dirty; std::copy(inventory[1].equipment.begin(), inventory[1].equipment.end(), dirty.bytes.begin());
        ConsumerProbe probe(f, dirty); probe.bind(arena);
        std::array<std::uint8_t, 0x90> manager{};
        std::array<std::uint8_t, 0x40> backend{};
        put(manager, 0x88, probe.request.data()); put(probe.context, 0x28, manager.data());
        put(probe.request, 0x10, backend.data());
        const std::int32_t prior_error = scenario == 2 ? -7 : scenario == 3 ? 7 : 0;
        put(probe.request, 0x50, prior_error);
        arena.restore(pe, 0x2776d00); arena.restore(pe, 0x2c1da50);
        arena.pointer(0x6ceee08, scenario == 1 ? 0 : 1);
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client), s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        {
            reference::Pair refs; require(refs.acquire(sources) == reference::Code::acquired,
                "Original request shim and consumer retain actual owner references");
            {
                lease::Group locks;
                const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
                    lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
                require(locks.try_acquire(bindings) == lease::Code::acquired, "Native slot marking holds both owners");
                ++native_calls; require(invoke_slot_changed(dirty.bytes.data(), 3) == 0, "Native fixture dirty slot mark");
            }
            const auto items_before = inventory[1].equipped;
            const auto result = consume_fixture(arena, tls, probe);
            require(!result.first && result.second == (scenario == 0 ? 0u : scenario == 2 ? 903u : 902u) &&
                probe.valid && probe.acquisitions == 0 && probe.executions == 0,
                "Consumer uses original acquisition/execute, preserving stale status errors after bypass success");
            require(probe.uids[0] == 22 && probe.endurance[0] == 100 &&
                get<std::uint64_t>(probe.request, 0x670) == 1 &&
                get<std::int32_t>(probe.request, 0x50) == prior_error &&
                probe.reports == unsigned(scenario == 1 || scenario == 3) && probe.errors == unsigned(scenario != 0),
                "Native request values and original diagnostic decisions are exact; no persistence inferred");
            require(inventory[1].equipped == items_before && probe.unlocked(), "SQL shim never changes original source items");
            dirty.check(heap, {}, true);
            dirty.cleanup(heap, mode);
            require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls,
                "Original request path balances every private dirty-table allocation");
        }
        require(f.valid && !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Original request/consumer integration releases both owners");
    }
    consumer_probe = nullptr; fixture = nullptr;
}
