// Full original slot-marker on owned private component/table allocations.
// Marking a slot is not a receipt from the engine's later consumer.
unsigned dirty_cases = 0;
using SlotChanged = void (*)(void*, std::uint16_t) noexcept;
SlotChanged native_slot_changed = nullptr;
using TableCleanup = void (*)(void*) noexcept;
TableCleanup native_dirty_clear = nullptr, native_dirty_destroy = nullptr;
DWORD invoke_table_cleanup(TableCleanup fn, void* table) {
    __try { fn(table); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
struct DirtyComponent {
    alignas(8) std::array<std::uint8_t, 0x208> bytes{};
    DirtyComponent() {
        bytes.fill(0x5a);
        std::fill(bytes.begin() + 0x1e8, bytes.end(), std::uint8_t(0));
        put(bytes, 0x1f4, UINT32_MAX); put(bytes, 0x1f8, UINT32_MAX);
    }
    const std::uint8_t* table() const { return bytes.data() + 0x1e8; }
    void check(const ItemHeap& heap, const std::map<std::uint16_t, std::uint32_t>& expected, bool retained = false) const {
        const auto* t = table();
        const auto count = item_read<std::uint32_t>(t, 0), cap = item_read<std::uint32_t>(t, 4);
        auto* nodes = item_read<const std::uint8_t*>(t, 0x18);
        require(count == expected.size() && item_read<std::uint32_t>(t, 8) == expected.size(),
            "Dirty set count and serial count increase only for new slots");
        require(heap.valid && heap.guards(), "Dirty-table allocations retain guards");
        if (expected.empty()) {
            require((retained ? cap >= 8 && heap.covers(nodes, std::size_t(cap) * 32) : !cap && !nodes) && item_read<std::uint32_t>(t, 0xc) == UINT32_MAX &&
                item_read<std::uint32_t>(t, 0x10) == UINT32_MAX, "Empty dirty set has the expected fresh or retained allocation state");
            if (retained) for (std::uint32_t i = 0; i < cap; ++i) {
                const auto* n = nodes + std::size_t(i) * 32;
                require(item_read<std::uint32_t>(n, 0) == 0xffff0000 && !item_read<std::uint32_t>(n, 4) &&
                    item_read<std::uint64_t>(n, 8) == UINT64_MAX && item_read<std::uint64_t>(n, 0x10) == UINT64_MAX &&
                    !item_read<std::uintptr_t>(n, 0x18), "Native clear resets all bucket links/masks/payloads and retains only reusable storage");
            }
            return;
        }
        require(cap >= 8 && cap <= 2048 && heap.covers(nodes, std::size_t(cap) * 32) &&
            reinterpret_cast<std::uintptr_t>(nodes) % 32 == 0, "Dirty table uses an owned 32-byte-aligned node array");
        std::map<std::uint16_t, std::uint32_t> actual;
        std::vector<const void*> payloads;
        std::vector<std::uint32_t> buckets;
        for (std::uint32_t i = 0; i < cap; ++i) {
            const auto* n = nodes + std::size_t(i) * 32;
            require(n[0] == 0 || n[0] == 2, "Insertion leaves no reserved or moved nodes behind");
            if (item_read<std::uint32_t>(n, 0xc) != UINT32_MAX) buckets.push_back(i);
            if (n[0] != 2) continue;
            const auto* payload = item_read<const std::uint8_t*>(n, 0x18);
            require(heap.covers(payload, 8) && std::find(payloads.begin(), payloads.end(), payload) == payloads.end(),
                "Each occupied node owns a unique slot payload");
            payloads.push_back(payload);
            const auto slot = item_read<std::uint16_t>(payload, 0);
            const auto key = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(slot)));
            require(item_read<std::uint32_t>(n, 8) == key, "Native wrapper preserves the signed slot hash key");
            require(actual.emplace(slot, item_read<std::uint32_t>(payload, 4)).second, "No duplicate slot payloads");
            const auto* bucket = nodes + std::size_t(key % (cap - 4)) * 32;
            const auto first = item_read<std::uint32_t>(bucket, 0xc);
            require(first <= i && i - first < 32, "Slot belongs to a bounded collision group");
            const auto* head = nodes + std::size_t(first) * 32;
            require(i - first < head[1] && (item_read<std::uint32_t>(head, 4) & (1u << (i - first))),
                "Collision movement retains the member count and mask");
        }
        require(actual == expected, "Rehash preserves every slot and serial");
        std::vector<std::uint32_t> visited;
        auto previous = UINT32_MAX, index = item_read<std::uint32_t>(t, 0xc);
        while (index != UINT32_MAX) {
            require(index < cap && std::find(visited.begin(), visited.end(), index) == visited.end(), "Bucket links are bounded and acyclic");
            visited.push_back(index);
            const auto* n = nodes + std::size_t(index) * 32;
            require(item_read<std::uint32_t>(n, 0x10) == previous, "Collision movement preserves reverse links");
            previous = index; index = item_read<std::uint32_t>(n, 0x14);
        }
        require(previous == item_read<std::uint32_t>(t, 0x10), "Dirty bucket tail matches forward traversal");
        std::sort(visited.begin(), visited.end());
        require(visited == buckets, "Every nonempty hash bucket occurs once in the traversal");
    }
    // Called only on privately owned fixture tables, never an engine component.
    void cleanup(ItemHeap& heap, unsigned) {
        ++native_calls;
        require(invoke_table_cleanup(native_dirty_destroy, bytes.data() + 0x1e8) == 0 && heap.valid,
            "Original table destructor releases native payloads and node storage");
        check(heap, {});
    }
};
DWORD invoke_slot_changed(void* component, std::uint16_t slot) {
    __try { native_slot_changed(component, slot); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void dirty_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    int cpu[4]{}; __cpuidex(cpu, 7, 0);
    require((cpu[1] & (1 << 8)) != 0, "CPU supports the original dirty-table BMI2 instructions");
    require(item_read<std::uint32_t>(pe.rva(0x7b5fcf8, 4).data(), 0) == 8, "Pinned table growth constant");
    arena.write(0x7b5fcf8, pe.rva(0x7b5fcf8, 4), PAGE_READONLY);
    arena.write(0x1693108f, pe.rva(0x1693108f, 8), PAGE_READONLY);
    const auto vt = pe.rva(0x5b28f80 + 0x198, 8);
    require(item_read<std::uint64_t>(vt.data(), 0) == 0x142ae0650ull, "Actual server equipment vtable binds the slot marker");
    native_slot_changed = arena.function<SlotChanged>(0x2ae0650);
    native_dirty_clear = arena.function<TableCleanup>(0x40b7b0);
    native_dirty_destroy = arena.function<TableCleanup>(0x410590);
    install_item_allocator(arena);
    ItemHeap heap;
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++dirty_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        DirtyComponent component;
        std::map<std::uint16_t, std::uint32_t> expected;
        component.check(heap, expected);
        std::vector<std::uint16_t> sequence;
        if (scenario == 1) sequence = {3};
        if (scenario == 2) sequence = {3,3,3,3};
        if (scenario == 3) sequence = {0,1,4,5,8,9,0,1,4}; // forces relocation of another bucket's head
        if (scenario == 4) for (std::uint16_t i = 0; i < 64; ++i) sequence.push_back(static_cast<std::uint16_t>(i * 4));
        if (scenario == 5) for (std::uint16_t i = 0; i < 64; ++i) sequence.push_back(i);
        if (scenario == 6) for (std::uint16_t i = 64; i; --i) sequence.push_back(static_cast<std::uint16_t>(i - 1));
        if (scenario == 7) sequence = {0,0x7fff,0x8000,0xffff,1,0xfffe,0x8000,0xffff,0};
        for (const auto slot : sequence) {
            const auto size = static_cast<std::uint32_t>(expected.size());
            expected.emplace(slot, size);
            ++native_calls;
            require(invoke_slot_changed(component.bytes.data(), slot) == 0, "Complete original dirty-slot marker returns without exception");
            require(std::all_of(component.bytes.begin(), component.bytes.begin() + 0x1e8, [](auto v) { return v == 0x5a; }),
                "Dirty marker changes only the component's inline table");
            component.check(heap, expected);
            require(heap.live() == expected.size() + 1, "Rehash frees prior arrays without leaking allocations");
        }
        const auto prefix = component.bytes;
        const auto cap = item_read<std::uint32_t>(component.table(), 4);
        const auto nodes = item_read<const void*>(component.table(), 0x18);
        ++native_calls;
        require(invoke_table_cleanup(native_dirty_clear, component.bytes.data() + 0x1e8) == 0, "Original reusable dirty-table clear completes");
        component.check(heap, {}, cap != 0);
        require(item_read<std::uint32_t>(component.table(), 4) == cap && item_read<const void*>(component.table(), 0x18) == nodes &&
            std::equal(prefix.begin(), prefix.begin() + 0x1e8, component.bytes.begin()), "Clear retains allocation and preserves component prefix");
        const auto frees = heap.free_calls;
        ++native_calls;
        require(invoke_table_cleanup(native_dirty_clear, component.bytes.data() + 0x1e8) == 0 && heap.free_calls == frees,
            "Clearing an already empty table is harmless and frees nothing twice");
        ++native_calls;
        require(invoke_slot_changed(component.bytes.data(), 3) == 0, "Table remains reusable after native clear");
        component.check(heap, {{std::uint16_t(3), std::uint32_t(0)}});
        component.cleanup(heap, mode);
        require(!heap.live() && heap.valid && heap.alloc_calls == heap.free_calls, "Fixture releases all dirty-slot allocations exactly once");
    }
    tls[0x1fd] = 0;
}
