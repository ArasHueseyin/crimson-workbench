// Original equipment persistence consumer; the database is a bounded fixture.
// No save or database is opened. A cleared dirty set is not a success receipt.
unsigned consumer_cases = 0;
using ConsumeDirty = void* (*)(void*, std::uint32_t*);
struct ConsumerProbe;
thread_local ConsumerProbe* consumer_probe = nullptr;
struct ConsumerProbe {
    Fixture& owners;
    DirtyComponent& component;
    alignas(8) std::array<std::uint8_t, 0x1300> request{};
    alignas(8) std::array<std::uint8_t, 0x30> context{};
    std::array<std::uintptr_t, 6> methods{};
    std::array<std::uint64_t, 128> uids{}, uid_sizes{}, word_sizes{};
    std::array<std::uint16_t, 128> endurance{};
    std::uint64_t character = 1234567;
    std::uintptr_t old_character;
    std::uint8_t* status;
    std::uint32_t capacity = 128;
    int execute_result = 0;
    unsigned acquisitions = 0, executions = 0, reports = 0, errors = 0, broken_notices = 0;
    bool valid = true;
    ConsumerProbe(Fixture& f, DirtyComponent& c) : owners(f), component(c) {
        auto* parts = item_read<std::uint8_t*>(owners.actors[1].bytes.data(), 0x68);
        status = item_read<std::uint8_t*>(parts, 0x20);
        old_character = item_read<std::uintptr_t>(status, 0x70);
        auto* id = &character; std::memcpy(status + 0x70, &id, 8);
        put(context, 0x28, this);
        put(request, 0x660, methods.data()); put(request, 0x18, request.data());
        put(request, 0x6b0, uids.data()); put(request, 0x6a8, uid_sizes.data());
        put(request, 0x6e0, endurance.data()); put(request, 0x6d8, word_sizes.data());
        std::memcpy(request.data() + 0x78, "failed", 7);
    }
    ~ConsumerProbe() { std::memcpy(status + 0x70, &old_character, 8); }
    bool unlocked() const {
        for (const auto& a : owners.actors) {
            if (!get<std::uint32_t>(a.bytes, 0x10) || get<std::uint32_t>(a.owner_lock, 0x2c)) return false;
            auto* lock = reinterpret_cast<SRWLOCK*>(const_cast<std::uint8_t*>(a.owner_lock.data()) + 0x10);
            if (!TryAcquireSRWLockExclusive(lock)) return false;
            ReleaseSRWLockExclusive(lock);
        }
        return true;
    }
    static std::uint32_t get_capacity(void* p) noexcept {
        auto& q = *consumer_probe; q.valid &= p == q.request.data() + 0x660 && q.unlocked(); return q.capacity;
    }
    static void* acquire(void* context, void* out) noexcept {
        auto& q = *consumer_probe; ++q.acquisitions;
        q.valid &= context == &q && q.unlocked() && !item_read<std::uint32_t>(q.component.table(), 0);
        auto* request = q.request.data(); std::memcpy(static_cast<std::uint8_t*>(out) + 8, &request, 8); return out;
    }
    static int execute(void* p) noexcept {
        auto& q = *consumer_probe; ++q.executions;
        q.valid &= p == q.request.data() && q.unlocked() && !item_read<std::uint32_t>(q.component.table(), 0) &&
            get<std::uint16_t>(q.request, 0xd10) == 2 && get<std::uint64_t>(q.request, 0xd08) == 2 &&
            get<std::uint64_t>(q.request, 0xd88) == q.character && get<std::uint64_t>(q.request, 0xd80) == 8 &&
            get<std::uint64_t>(q.request, 0x1250) == 1;
        const auto n = get<std::uint64_t>(q.request, 0x670);
        q.valid &= n && n <= q.uids.size();
        for (std::size_t i = 0; i < (std::min)(n, std::uint64_t(q.uids.size())); ++i)
            q.valid &= q.uid_sizes[i] == 8 && q.word_sizes[i] == 2;
        return q.execute_result;
    }
    static void report() noexcept { ++consumer_probe->reports; }
    static std::uint32_t error_hash(const char* text, unsigned length) noexcept {
        consumer_probe->valid &= length == 6 && std::memcmp(text, "failed", 6) == 0; return 903;
    }
    static void error() noexcept { ++consumer_probe->errors; }
    static std::uint64_t actor_id(void*) noexcept { return 77; }
    static void* actor_text(void*, void* out) noexcept {
        const char* text = "fixture"; std::memcpy(out, &text, 8); return out;
    }
    static void* recipient(void* actor) noexcept { return actor; }
    static void broken(void* actor, const void*, const std::uint32_t* count) noexcept {
        auto& q = *consumer_probe; ++q.broken_notices;
        q.valid &= actor == q.owners.actors[1].bytes.data() && q.unlocked() &&
            *count == item_read<std::uint32_t>(q.component.bytes.data(), 0x18);
    }
    void bind(Arena& arena) {
        methods[5] = reinterpret_cast<std::uintptr_t>(&get_capacity);
        auto& a = owners.actors[1];
        a.methods[0xc8 / 8] = reinterpret_cast<std::uintptr_t>(&actor_id);
        a.methods[0xd0 / 8] = reinterpret_cast<std::uintptr_t>(&actor_text);
        a.methods[0xd8 / 8] = reinterpret_cast<std::uintptr_t>(&actor_text);
        a.methods[0xe8 / 8] = reinterpret_cast<std::uintptr_t>(&recipient);
        arena.redirect(0x2c1da50, reinterpret_cast<std::uintptr_t>(&acquire));
        arena.redirect(0x2776d00, reinterpret_cast<std::uintptr_t>(&execute));
        arena.redirect(0x2776e40, reinterpret_cast<std::uintptr_t>(&report));
        arena.redirect(0x1364810, reinterpret_cast<std::uintptr_t>(&error_hash));
        arena.redirect(0x2af2020, reinterpret_cast<std::uintptr_t>(&error));
        arena.redirect(0x29f52f0, reinterpret_cast<std::uintptr_t>(&broken));
        arena.pointer(0x2ad217f + 0x420989d, 901); // exact RIP-relative error globals
        arena.pointer(0x2ad2211 + 0x420978f, 902);
    }
};
DWORD invoke_consumer(ConsumeDirty fn, void* component, std::uint32_t* out) {
    __try { fn(component, out); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
std::pair<DWORD, std::uint32_t> consume_fixture(Arena& arena, const std::array<std::uint8_t, 0x200>& original_tls, ConsumerProbe& probe) {
    std::array<std::uint8_t, 0x260> extended{};
    std::copy(original_tls.begin(), original_tls.end(), extended.begin()); put(extended, 0x250, probe.context.data());
    consumer_probe = &probe;
    arena.pointer(0x6cffd00, reinterpret_cast<std::uintptr_t>(extended.data()));
    std::uint32_t result = UINT32_MAX; ++native_calls;
    const auto fault = invoke_consumer(arena.function<ConsumeDirty>(0x2ad1d70), probe.component.bytes.data(), &result);
    arena.pointer(0x6cffd00, reinterpret_cast<std::uintptr_t>(original_tls.data()));
    consumer_probe = nullptr;
    return {fault, result};
}
void dirty_consumer_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    require(item_read<std::uint64_t>(pe.rva(0x5b28f80 + 0x130, 8).data(), 0) == 0x142ad1d70ull,
        "Actual equipment vtable binds the complete dirty-slot consumer");
    install_item_allocator(arena); ItemHeap heap;
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 12; ++scenario) {
        ++consumer_cases; Fixture f; fixture = &f;
        tls[0x1d2] = 1; tls[0x1d4] = 0; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        initialize(f, arena, 0x12346, false);
        std::array<InventoryFixture, 2> inventory;
        for (std::size_t i = 0; i < 2; ++i) inventory[i].initialize(f.actors[i]);
        DirtyComponent dirty; std::copy(inventory[1].equipment.begin(), inventory[1].equipment.end(), dirty.bytes.begin());
        std::array<std::uint8_t, 64 * 0xd0> entries{};
        std::map<std::uint64_t, std::uint16_t> expected;
        const unsigned count = scenario == 8 ? 64 : 1;
        for (unsigned i = 0; i < count; ++i) {
            const auto at = std::size_t(i) * 0xd0;
            const auto value = std::uint16_t(scenario == 6 ? 0 : scenario == 7 ? 0xffff : 100 + i);
            put(entries, at, std::uint64_t(1000 + i)); put(entries, at + 8, std::uint16_t(scenario == 4 ? action::absent : 10));
            put(entries, at + 0x10, std::int64_t(scenario == 5 ? 0 : 1)); put(entries, at + 0x40, value);
            put(entries, at + 0xc8, std::uint16_t(i * 4 + 3));
            if (scenario && scenario != 3 && scenario != 4 && scenario != 5) expected.emplace(1000 + i, value);
        }
        put(inventory[1].equipment_table, 8, entries.data()); put(inventory[1].equipment_table, 0x10, count);
        ConsumerProbe probe(f, dirty); probe.bind(arena);
        if (scenario == 9) probe.capacity = 0;
        if (scenario >= 10) probe.execute_result = scenario == 10 ? 1 : -1;
        if (scenario == 11) put(probe.request, 0x50, std::int32_t(-1));
        registry::Source c(f.managers[0].bytes.data(), lookup_client, release_client), s(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{c.source(0x12346), s.source(0x12346)};
        {
            reference::Pair refs; require(refs.acquire(sources) == reference::Code::acquired, "Consumer retains real actor references");
            if (scenario) {
                lease::Group locks;
                const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
                    lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
                require(locks.try_acquire(bindings) == lease::Code::acquired, "Native dirty marking uses both source locks");
                for (unsigned i = 0; i < count; ++i) {
                    ++native_calls;
                    require(invoke_slot_changed(dirty.bytes.data(), std::uint16_t(scenario == 3 ? 99 : i * 4 + 3)) == 0, "Mark fixture slot");
                }
                if (scenario == 2) { ++native_calls; require(invoke_slot_changed(dirty.bytes.data(), 3) == 0, "Repeated slot coalesces"); }
            }
            const auto unchanged = entries;
            const auto [fault, result] = consume_fixture(arena, tls, probe);
            require(!fault && result == (scenario == 9 ? 901u : scenario == 10 ? 902u : scenario == 11 ? 903u : 0u),
                ("Complete original consumer success/error code, case " + std::to_string(scenario) + ", fault " + std::to_string(fault)).c_str());
            require(probe.valid && entries == unchanged && probe.unlocked(), "Consumer copies values without changing items and releases lock before persistence callbacks");
            require(probe.acquisitions == unsigned(!expected.empty()) && probe.executions == unsigned(!expected.empty() && scenario != 9),
                "Empty/missing/absent/consumed slots produce no persistence request");
            std::map<std::uint64_t, std::uint16_t> actual;
            for (std::size_t i = 0; i < expected.size(); ++i) actual.emplace(probe.uids[i], probe.endurance[i]);
            require(actual == expected, "Every queued UID and main-endurance pair is preserved independently of hash traversal order");
            require(probe.broken_notices == unsigned(scenario == 6) && probe.reports == unsigned(scenario == 10) &&
                probe.errors == unsigned(scenario >= 10), "Broken counter and persistence error reports follow original decisions");
            dirty.check(heap, {}, scenario != 0);
            require(heap.live() == unsigned(scenario != 0), "Only reusable node storage remains after consumer, including rejected persistence");
            const auto acquisitions = probe.acquisitions;
            const auto again = consume_fixture(arena, tls, probe);
            require(!again.first && !again.second && probe.acquisitions == acquisitions, "A second consume cannot resend cleared entries, even after failure");
            dirty.cleanup(heap, mode);
            require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls, "Native clear, temporary request list and final destructor balance every allocation");
        }
        require(f.valid && !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10), "Consumer releases actor ownership after callbacks");
    }
    fixture = nullptr;
}
