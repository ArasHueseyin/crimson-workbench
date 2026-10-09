unsigned item_cases = 0;
struct ItemHeap;
thread_local ItemHeap* item_heap = nullptr;
struct ItemHeap {
    struct Allocation { std::uint8_t* base = nullptr; std::uint8_t* data = nullptr; std::size_t size = 0; };
    HANDLE heap = HeapCreate(0, 0, 0);
    std::array<Allocation, 256> allocations{};
    std::array<unsigned, 2> alloc_calls{}, free_calls{};
    bool valid = true;
    std::uint32_t max_bytes = 65536;
    explicit ItemHeap(std::uint32_t limit = 65536) : max_bytes(limit) {
        require(heap && !item_heap && limit <= 262144, "Bounded private item heap created without nested allocator context"); item_heap = this;
    }
    ~ItemHeap() { item_heap = nullptr; if (heap) HeapDestroy(heap); }
    bool owns(const void* p) const {
        for (const auto& a : allocations) if (a.base && a.data == p) return true;
        return false;
    }
    bool covers(const void* p, std::size_t size) const {
        for (const auto& a : allocations) if (a.base && a.data == p && size <= a.size) return true;
        return false;
    }
    unsigned live() const {
        return static_cast<unsigned>(std::count_if(allocations.begin(), allocations.end(), [](const auto& a) { return a.base != nullptr; }));
    }
    bool guards() const {
        for (const auto& a : allocations) if (a.base)
            for (std::size_t i = 0; i < 16; ++i)
                if ((a.data - 16)[i] != 0xd7 || a.data[a.size + i] != 0xd7) return false;
        return true;
    }
    void* allocate(std::uint32_t size, std::uint32_t alignment, unsigned mode) noexcept {
        ++alloc_calls[mode];
        if (!size || size > max_bytes || (alignment != 2 && alignment != 4 && alignment != 8 && alignment != 16 && alignment != 32)) { valid = false; return nullptr; }
        for (auto& a : allocations) if (!a.base) {
            a.base = static_cast<std::uint8_t*>(HeapAlloc(heap, 0, size + 32 + alignment)); a.size = size;
            if (!a.base) { valid = false; return nullptr; }
            a.data = reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(a.base + 16) + alignment - 1) & ~std::uintptr_t(alignment - 1));
            std::memset(a.base, 0xd7, size + 32 + alignment); std::memset(a.data, 0xa5, size);
            valid &= reinterpret_cast<std::uintptr_t>(a.data) % alignment == 0;
            return a.data;
        }
        valid = false; return nullptr;
    }
    void free(void* p, unsigned mode) noexcept {
        ++free_calls[mode]; valid &= guards();
        for (auto& a : allocations) if (a.base && a.data == p) {
            valid &= HeapFree(heap, 0, a.base) != 0; a = {}; return;
        }
        valid = false; // Never free source/foreign pointers, even in a failed test.
    }
};
void* item_alloc0(std::uint32_t size, std::uint32_t alignment) noexcept { return item_heap->allocate(size, alignment, 0); }
void* item_alloc1(std::uint32_t size, std::uint32_t alignment) noexcept { return item_heap->allocate(size, alignment, 1); }
void item_free0(void* p) noexcept { item_heap->free(p, 0); }
void item_free1(void* p) noexcept { item_heap->free(p, 1); }
void install_item_allocator(Arena& arena) {
    arena.redirect(0x47ef960, reinterpret_cast<std::uintptr_t>(&item_alloc0));
    arena.redirect(0x47efa28, reinterpret_cast<std::uintptr_t>(&item_alloc1));
    arena.redirect(0x47f026c, reinterpret_cast<std::uintptr_t>(&item_free0));
    arena.redirect(0x47f036c, reinterpret_cast<std::uintptr_t>(&item_free1));
}
item::Binding item_binding(Arena& arena) {
    return {arena.function<decltype(item::Binding::construct)>(0x2409750),
        arena.function<decltype(item::Binding::assign)>(0x240b020),
        arena.function<decltype(item::Binding::destroy)>(0x240af10)};
}
template<class T> T item_read(const void* p, std::size_t at) {
    T result; std::memcpy(&result, static_cast<const std::uint8_t*>(p) + at, sizeof(T)); return result;
}
struct ItemSource {
    alignas(8) std::array<std::uint8_t, action::item_size> raw{};
    alignas(8) std::array<std::uint8_t, 24> sockets{}, entries6{}, block24{};
    alignas(8) std::array<std::uint8_t, 64> entries16{};
    alignas(8) std::array<std::uint8_t, 16> block16{};
    bool operator==(const ItemSource&) const = default;
    void setup(unsigned kind, std::uint32_t count = 2) {
        raw.fill(0);
        put(raw, 0, std::uint64_t(789)); put(raw, 8, std::uint16_t(10)); put(raw, 0xa, std::uint16_t(7));
        put(raw, 0x10, std::int64_t(1)); put(raw, 0x18, std::uint64_t(17)); put(raw, 0x20, std::uint64_t(28));
        for (std::size_t i = 0x28; i < 0x40; ++i) raw[i] = static_cast<std::uint8_t>(i);
        put(raw, 0x40, std::uint16_t(100)); put(raw, 0x48, std::uint64_t(54)); put(raw, 0x50, std::uint64_t(61));
        put(raw, 0x58, std::uint16_t(9)); put(raw, 0x88, std::uint32_t(11)); put(raw, 0x8c, std::uint32_t(12));
        put(raw, 0x90, std::uint16_t(13)); put(raw, 0x98, std::uint64_t(14)); raw[0xa0] = 1; raw[0xa1] = 2;
        auto fill = [](auto& a) { for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<std::uint8_t>(i + 37); };
        fill(sockets); fill(entries6); fill(entries16); fill(block24); fill(block16);
        if (!kind) return;
        put(raw, 0x60, sockets.data()); put(raw, 0x68, count); put(raw, 0x6c, std::uint32_t(4));
        raw[0x70] = static_cast<std::uint8_t>(count);
        if (kind == 1) return;
        put(raw, 0x78, entries16.data()); put(raw, 0x80, count); put(raw, 0x84, std::uint32_t(4));
        put(raw, 0xa8, entries6.data()); put(raw, 0xb0, count); put(raw, 0xb4, std::uint32_t(4));
        put(raw, 0xb8, block24.data()); put(raw, 0xc0, block16.data());
    }
};
void check_item_copy(const item::Value& value, const ItemSource& source, const ItemHeap& heap) {
    const auto* copied = static_cast<const std::uint8_t*>(value.data());
    require(copied != nullptr && heap.valid && heap.guards(), "Native item copy and allocator guards valid");
    for (const auto [at, length] : std::array<std::pair<std::size_t, std::size_t>, 5>{{
        {0, 12}, {0x10, 0x32}, {0x48, 0x12}, {0x88, 10}, {0x98, 10}}})
        require(std::equal(source.raw.begin() + at, source.raw.begin() + at + length, copied + at),
            "Native copy preserves all independently observed scalar fields");
    for (const auto [at, stride] : std::array<std::pair<std::size_t, std::size_t>, 3>{{{0x60,6}, {0x78,16}, {0xa8,6}}}) {
        const auto count = get<std::uint32_t>(source.raw, at + 8);
        require(item_read<std::uint32_t>(copied, at + 8) == count, "Native nested vector count copied");
        if (!count) continue; // Native clear may retain an owned allocation.
        const auto* from = get<const std::uint8_t*>(source.raw, at);
        const auto* to = item_read<const std::uint8_t*>(copied, at);
        require(from != to && heap.covers(to, count * stride), "Nested vector owns independent bounded allocation");
        for (std::size_t i = 0; i < count; ++i)
            require(std::equal(from + i * stride, from + i * stride + (stride == 16 ? 13 : stride), to + i * stride),
                "Original vector copier retains meaningful record bytes, excluding padding");
    }
    require(copied[0x70] == source.raw[0x70], "Logical socket bound copied separately from allocation capacity");
    for (const auto [at, size] : std::array<std::pair<std::size_t, std::size_t>, 2>{{{0xb8,24}, {0xc0,16}}}) {
        const auto* from = get<const std::uint8_t*>(source.raw, at);
        const auto* to = item_read<const std::uint8_t*>(copied, at);
        require(from ? from != to && heap.covers(to, size) && std::equal(from, from + size, to) : to == nullptr,
            "Optional native item payload copied independently or removed");
    }
}
DWORD invoke_item_copy(item::Value* value, const void* source, item::Code* out) {
    __try { *out = value->assign(source); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void item_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    // Earlier layout tests intentionally replaced just the empty temporary
    // lifecycle. Restore complete pinned functions before owning real copies.
    arena.restore(pe, 0x2409750); arena.restore(pe, 0x240af10);
    arena.write(0xff42434, pe.rva(0xff42434, 4), PAGE_READONLY);
    install_item_allocator(arena);
    ItemHeap heap;
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 10; ++scenario) {
        ++item_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        const auto before_allocs = heap.alloc_calls, before_frees = heap.free_calls;
        ItemSource source; source.setup(scenario == 0 ? 0 : scenario == 1 ? 1 : 2);
        if (scenario >= 7) source.raw[0x70] = 6;
        if (scenario == 8 || scenario == 9) put(source.raw, 0x68, std::uint32_t(0));
        if (scenario == 8) { put(source.raw, 0x60, std::uintptr_t(0)); put(source.raw, 0x6c, std::uint32_t(0)); }
        const auto original = source;
        item::Value value(item_binding(arena));
        ++native_calls;
        require(value.construct() == item::Code::constructed && item_read<std::uint64_t>(value.data(), 0) == UINT64_MAX &&
            item_read<std::uint16_t>(value.data(), 8) == action::absent && !heap.live(), "Original default constructor owns no allocations");
        auto copy = [&](const ItemSource& from) {
            const auto unchanged = from;
            item::Code result = item::Code::empty;
            ++native_calls;
            require(invoke_item_copy(&value, from.raw.data(), &result) == 0 && result == item::Code::copied,
                "Full native item assignment completes with original nested copiers");
            check_item_copy(value, from, heap);
            require(from == unchanged, "Native assignment never mutates or frees the source");
        };
        copy(source);
        if (scenario == 3) { source.setup(2, 3); copy(source); }
        if (scenario == 4) { ItemSource empty; empty.setup(0); copy(empty); }
        if (scenario == 5) {
            const auto allocs = heap.alloc_calls, frees = heap.free_calls;
            ++native_calls;
            require(value.assign(value.data()) == item::Code::copied && heap.alloc_calls == allocs && heap.free_calls == frees,
                "Original self-assignment does not clear or reallocate the owned copy");
            check_item_copy(value, source, heap);
        }
        if (scenario == 6) {
            const auto allocs = heap.alloc_calls, frees = heap.free_calls;
            std::array<std::uint8_t, action::item_size> before{}; std::memcpy(before.data(), value.data(), before.size());
            put(source.raw, 0x6c, std::uint32_t(1));
            require(value.assign(source.raw.data()) == item::Code::invalid_source && heap.alloc_calls == allocs && heap.free_calls == frees &&
                std::memcmp(before.data(), value.data(), before.size()) == 0, "Invalid source capacity refused before destination cleanup");
            source = original;
            check_item_copy(value, source, heap);
        }
        if (scenario != 3) require(source == original, "Every source field and nested payload remains unchanged");
        ++native_calls;
        require(value.release() == item::Code::released && !value.data() && !heap.live() && heap.valid && heap.guards(),
            "Original destructor releases every native nested allocation exactly once");
        require(heap.alloc_calls[1 - mode] == before_allocs[1 - mode] && heap.free_calls[1 - mode] == before_frees[1 - mode] &&
            heap.alloc_calls[mode] - before_allocs[mode] == heap.free_calls[mode] - before_frees[mode],
            "Both original TLS allocator routes balance all copies and replacements");
    }
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 6; ++scenario) {
        ++item_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        ItemSource source; source.setup(scenario == 4 ? 2 : 1);
        source.raw[0x70] = 6;
        put(source.raw, 0x40, std::uint16_t(35));
        put(source.sockets, 0, std::uint16_t(20)); put(source.sockets, 2, std::uint16_t(5)); source.sockets[4] = 0;
        put(source.sockets, 6, action::absent); source.sockets[10] = 1;
        if (scenario == 1 || scenario == 2) put(source.raw, 0x68, std::uint32_t(0));
        if (scenario == 1) { put(source.raw, 0x60, std::uintptr_t(0)); put(source.raw, 0x6c, std::uint32_t(0)); }
        if (scenario == 3) { put(source.raw, 0x68, std::uint32_t(1)); put(source.raw, 0x6c, std::uint32_t(1)); }
        const auto original = source;
        action::Snapshot snapshot; snapshot.session = {41, 0x12346, 7};
        snapshot.definitions = {{10, 100, scenario == 5 ? action::absent : std::uint16_t(100), scenario == 5}, {20, 30}};
        action::Item entry; entry.identity = {789, action::Area::equipped, 0, 3};
        for (auto* image : {&entry.authority, &entry.presentation}) {
            image->bytes = source.raw; image->sockets.resize(get<std::uint32_t>(source.raw, 0x68));
            for (std::size_t i = 0; i < image->sockets.size(); ++i)
                std::copy_n(source.sockets.begin() + i * 6, 6, image->sockets[i].bytes.begin());
        }
        snapshot.items.push_back(entry); action::Plan plan;
        require(action::prepare({snapshot.session, action::Scope::equipped, {}}, snapshot, plan) == action::Code::prepared,
            "Production plan supports initialized count smaller than logical slot limit and allocation");
        for (const auto realm : {item::Realm::authority, item::Realm::presentation}) {
            item::Value copy(item_binding(arena)); ++native_calls;
            require(copy.construct() == item::Code::constructed, "Original native after-copy constructed before source stores");
            ++native_calls;
            require(copy.prepare_repair(source.raw.data(), plan, entry.identity, realm) == item::Code::prepared && source == original,
                "Production after-copy builder uses original assignment, repairing only owned data");
            require(item_read<std::uint16_t>(copy.data(), 0x40) == 100 && item_read<std::uint8_t>(copy.data(), 0x70) == 6 &&
                item_read<std::uint32_t>(copy.data(), 0x68) == entry.authority.sockets.size(), "Finite vanilla target and sparse geometry retained");
            const auto* sockets = item_read<const std::uint8_t*>(copy.data(), 0x60);
            if (!entry.authority.sockets.empty()) require(sockets != source.sockets.data() && heap.covers(sockets, entry.authority.sockets.size() * 6) &&
                item_read<std::uint16_t>(sockets, 2) == 30 && sockets[5] == source.sockets[5], "Only owned socket endurance changes");
            ++native_calls;
            require(copy.release() == item::Code::released && !heap.live() && heap.valid && heap.guards() && heap.alloc_calls == heap.free_calls,
                "Prepared native after-copy frees all buffers without touching source allocations");
        }
    }
    tls[0x1fd] = 0;
}
