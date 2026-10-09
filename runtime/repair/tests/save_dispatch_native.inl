// Complete original save dispatcher. All backend methods and diagnostics are
// private callbacks: no filesystem, save access, platform API or live context.
unsigned save_dispatch_cases = 0;
using SaveDispatch = bool (*)(void*, void*);
struct SaveDispatchProbe;
thread_local SaveDispatchProbe* save_dispatch_probe = nullptr;
struct SaveDispatchProbe {
    Arena& arena;
    alignas(8) std::array<std::uint8_t, 0x40> context{};
    alignas(8) std::array<std::uint8_t, 0x160> backend{};
    alignas(8) std::array<std::uint8_t, 0x10> platform{};
    std::array<std::uintptr_t, 21> platform_methods{};
    alignas(8) std::array<std::uint8_t, 0x6f0> record{};
    std::array<std::uintptr_t, 10> methods{};
    std::array<unsigned, 4> order{};
    unsigned calls = 0, fail_stage = 0, logs = 0, reports = 0;
    std::uint8_t prior_saved = 0;
    bool valid = true;
    bool quota_ok = true;
    std::uint64_t quota_total = 100, quota_available = 1;
    std::uint32_t platform_files = 9999;
    unsigned quota_calls = 0, file_count_calls = 0;
    static constexpr char external_name[] = "private-external-slot";
    static constexpr std::array<std::uint32_t, 4> report_rvas{0x5983b98, 0x5983ae8, 0x5983b38, 0x5983a78};
    explicit SaveDispatchProbe(Arena& a) : arena(a) {
        record.fill(0xc7); std::fill(record.begin(), record.begin() + 0x6d0, std::uint8_t(0));
        constexpr char name[] = "private-inline-slot";
        std::copy_n(name, sizeof(name), record.begin() + 0x28);
        methods[4] = reinterpret_cast<std::uintptr_t>(&load);
        methods[5] = reinterpret_cast<std::uintptr_t>(&save);
        methods[6] = reinterpret_cast<std::uintptr_t>(&remove);
        methods[9] = reinterpret_cast<std::uintptr_t>(&validate);
        put(backend, 0, methods.data()); put(context, 0x10, backend.data());
        put(context, 0x18, std::uint64_t(0x11223344));
        put(backend, 0x148, platform.data()); put(platform, 0, platform_methods.data());
        platform_methods[20] = reinterpret_cast<std::uintptr_t>(&quota);
        platform_methods[18] = reinterpret_cast<std::uintptr_t>(&file_count);
    }
    bool step(unsigned stage, void* self, const void* argument) noexcept {
        valid &= self == backend.data() && calls < order.size() && calls + 1 == stage;
        valid &= record[0x698] == 1 && record[0x69b] == prior_saved;
        valid &= argument == (stage == 2 ? context.data() + 0x18 : record.data());
        if (calls < order.size()) order[calls] = stage;
        ++calls;
        return fail_stage != stage;
    }
    static bool load(void* self, const void* data) noexcept { return save_dispatch_probe->step(1, self, data); }
    static bool validate(void* self, const void* data) noexcept { return save_dispatch_probe->step(2, self, data); }
    static bool save(void* self, const void* data) noexcept { return save_dispatch_probe->step(3, self, data); }
    static bool remove(void* self, const void* data) noexcept { return save_dispatch_probe->step(4, self, data); }
    static bool quota(void* self, std::uint64_t* total, std::uint64_t* available) noexcept {
        auto& p = *save_dispatch_probe; ++p.quota_calls;
        p.valid &= self == p.platform.data() && total && available && total != available && p.calls == 1 &&
            p.record[0x698] == 1 && p.record[0x69b] == p.prior_saved;
        p.order[p.calls++] = 2;
        *total = p.quota_total; *available = p.quota_available;
        return p.quota_ok;
    }
    static std::uint32_t file_count(void* self) noexcept {
        auto& p = *save_dispatch_probe; ++p.file_count_calls;
        p.valid &= self == p.platform.data() && p.calls == 2 && p.quota_calls == 1 &&
            p.record[0x698] == 1 && p.record[0x69b] == p.prior_saved;
        return p.platform_files;
    }
    // Only the low-byte enable argument is used; formatting and remaining
    // diagnostic parameters belong to the game and are not executed here.
    static void log(std::uint8_t enabled) noexcept {
        auto& p = *save_dispatch_probe; ++p.logs;
        p.valid &= enabled == 1 && p.fail_stage && p.calls == p.fail_stage && !p.record[0x69b];
    }
    static void report(void* context, const void* message, const void* record) noexcept {
        auto& p = *save_dispatch_probe; ++p.reports;
        p.valid &= context == p.context.data() && record == p.record.data() &&
            p.fail_stage && p.calls == p.fail_stage && p.record[0x698] == 1 && !p.record[0x69b];
        if (p.fail_stage >= 1 && p.fail_stage <= 4)
            p.valid &= message == p.arena.function<void*>(report_rvas[p.fail_stage - 1]);
    }
    void bind() {
        save_dispatch_probe = this;
        arena.redirect(0x35de50, reinterpret_cast<std::uintptr_t>(&log));
        arena.redirect(0x2356220, reinterpret_cast<std::uintptr_t>(&report));
    }
};
DWORD invoke_save_dispatch(SaveDispatch fn, void* context, void* record, bool* result) {
    __try { *result = fn(context, record); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void save_dispatch_native_tests(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    const auto dispatch = arena.function<SaveDispatch>(0x23556b0);
    for (unsigned gates = 0; gates < 8; ++gates) for (unsigned old_status = 0; old_status < 2; ++old_status) {
        ++save_dispatch_cases;
        SaveDispatchProbe p(arena); p.bind(); p.prior_saved = static_cast<std::uint8_t>(old_status);
        p.record[0x698] = static_cast<std::uint8_t>(gates & 1);
        p.record[0x699] = static_cast<std::uint8_t>((gates >> 1) & 1);
        p.record[0x69c] = static_cast<std::uint8_t>((gates >> 2) & 1);
        p.record[0x69b] = p.prior_saved;
        const bool active = gates == 1;
        const auto context_before = p.context;
        const auto backend_before = p.backend;
        auto expected_record = p.record;
        if (active) { expected_record[0x698] = 0; expected_record[0x69b] = 1; }
        bool result = false; ++native_calls;
        require(!invoke_save_dispatch(dispatch, active ? p.context.data() : nullptr, p.record.data(), &result) && result,
            "Original dispatcher succeeds both after backend execution and after every skip-gate combination");
        require(p.valid && p.calls == (active ? 4u : 0u) && !p.logs && !p.reports &&
            p.record == expected_record && p.context == context_before && p.backend == backend_before,
            "Only a completed callback sequence changes dirty/saved bytes; skipped work retains stale status without context access");
        if (active) {
            // A repeated call after successful completion sees clean state and
            // never invokes the backend a second time. No automatic retry.
            result = false; ++native_calls;
            require(!invoke_save_dispatch(dispatch, nullptr, p.record.data(), &result) && result && p.calls == 4 &&
                p.record == expected_record && p.valid, "Completed record is skipped on repeat without reading any backend");
        }
    }
    for (unsigned stage = 1; stage <= 4; ++stage) for (unsigned external = 0; external < 2; ++external) {
        ++save_dispatch_cases;
        SaveDispatchProbe p(arena); p.bind(); p.fail_stage = stage; p.prior_saved = 1;
        p.record[0x698] = 1; p.record[0x69b] = 1;
        if (external) put(p.record, 0xb0, SaveDispatchProbe::external_name);
        const auto context_before = p.context;
        const auto backend_before = p.backend;
        auto expected_record = p.record; expected_record[0x69b] = 0;
        bool result = true; ++native_calls;
        require(!invoke_save_dispatch(dispatch, p.context.data(), p.record.data(), &result) && !result,
            "Every backend failure stops the original dispatcher and returns false");
        require(p.valid && p.calls == stage && p.logs == 1 && p.reports == 1 &&
            p.record == expected_record && p.context == context_before && p.backend == backend_before,
            "Failure clears stale success but preserves dirty state and every other byte, including inline/external diagnostic names");
        for (unsigned i = 0; i < stage; ++i)
            require(p.order[i] == i + 1, "Backend methods before failure are invoked exactly once and in order");
    }
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++save_dispatch_cases;
        SaveDispatchProbe p(arena); p.bind(); p.prior_saved = 1;
        p.record[0x698] = 1; p.record[0x69b] = 1;
        p.methods[9] = reinterpret_cast<std::uintptr_t>(arena.function<void*>(0x235adb0));
        if (scenario == 0) p.quota_ok = false;
        if (scenario == 1) p.quota_available = 0;
        if (scenario == 3) p.platform_files = 10000;
        if (scenario == 4) p.platform_files = UINT32_MAX;
        if (scenario == 5) p.quota_total = 0;
        if (scenario == 6) p.platform_files = 0;
        if (scenario == 7) p.quota_available = UINT64_MAX;
        const bool allowed = scenario == 2 || scenario >= 5;
        p.fail_stage = allowed ? 0 : 2;
        const auto backend_before = p.backend;
        const auto platform_before = p.platform;
        auto expected_record = p.record;
        expected_record[0x698] = allowed ? 0 : 1; expected_record[0x69b] = allowed ? 1 : 0;
        bool result = false; ++native_calls;
        require(!invoke_save_dispatch(dispatch, p.context.data(), p.record.data(), &result) && result == allowed,
            "Original Steam validation helper is integrated with the original save dispatcher on private platform data");
        require(p.valid && p.quota_calls == 1 && p.file_count_calls == unsigned(scenario > 1) &&
            p.calls == (allowed ? 4u : 2u) && p.logs == unsigned(!allowed) && p.reports == unsigned(!allowed) &&
            p.record == expected_record && p.backend == backend_before && p.platform == platform_before,
            "Steam validation requires a successful query, positive available bytes and fewer than 10000 files; no file writes are involved");
    }
    {
        ++save_dispatch_cases;
        SaveDispatchProbe p(arena); p.bind(); p.record[0x698] = 1;
        put(p.context, 0x10, std::uintptr_t(0));
        const auto record_before = p.record;
        bool result = false; ++native_calls;
        require(invoke_save_dispatch(dispatch, p.context.data(), p.record.data(), &result) == EXCEPTION_ACCESS_VIOLATION &&
            !result && p.record == record_before && !p.calls && !p.logs && !p.reports,
            "Active work needs a valid backend; original stack unwind returns its private access fault without a success receipt");
    }
    install_item_allocator(arena); ItemHeap heap;
    using MarkSaveEntry = void (*)(void*, void*, std::uint8_t);
    const auto mark = arena.function<MarkSaveEntry>(0x235b5d0);
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned kind = 0; kind < 3; ++kind) {
        ++save_dispatch_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        SaveDispatchProbe p(arena); p.bind();
        std::array<std::array<std::uint8_t, 8>, 8> entries{};
        for (unsigned i = 0; i < entries.size(); ++i) entries[i].fill(static_cast<std::uint8_t>(i + 1));
        const auto entries_before = entries;
        auto expected_record = p.record;
        const auto allocs = heap.alloc_calls, frees = heap.free_calls;
        const auto at = 0x6a0 + kind * 16;
        for (unsigned i = 0; i < 9; ++i) {
            // Repeated pointer is deliberate: native marking does not deduplicate.
            ++native_calls; mark(p.record.data(), entries[i % 8].data(), static_cast<std::uint8_t>(kind));
            const auto* queued = get<const std::uintptr_t*>(p.record, at);
            require(get<std::uint32_t>(p.record, at + 8) == i + 1 &&
                get<std::uint32_t>(p.record, at + 12) >= i + 1 && heap.covers(queued, (i + 1) * 8) &&
                p.record[0x698] == 1 && heap.valid && heap.guards(),
                "Native save-entry marking grows the selected queue and marks its owner dirty");
            for (unsigned j = 0; j <= i; ++j)
                require(queued[j] == reinterpret_cast<std::uintptr_t>(entries[j % 8].data()),
                    "Native save queue preserves pointer order, including duplicates");
        }
        expected_record[0x698] = 1;
        std::copy_n(p.record.begin() + at, 16, expected_record.begin() + at);
        require(p.record == expected_record && entries == entries_before,
            "Save marking changes only the selected queue header and dirty byte, never queued objects or saved status");
        heap.free(get<void*>(p.record, at), mode); // Fixture owns the containing object and queue allocation.
        require(!heap.live() && heap.valid && heap.guards() &&
            heap.alloc_calls[1 - mode] == allocs[1 - mode] && heap.free_calls[1 - mode] == frees[1 - mode] &&
            heap.alloc_calls[mode] - allocs[mode] == heap.free_calls[mode] - frees[mode],
            "Native queue growth and fixture teardown balance every private allocation in each TLS mode");
    }
    save_dispatch_probe = nullptr;
}
