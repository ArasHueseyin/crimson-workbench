// Real ItemSaveData construction/conversion/vector growth/destruction on private
// objects. Catalog, clock, reflection-key interning and observer notifications
// are controlled dependencies; no file serializer or game save is invoked.
unsigned save_cases = 0, save_repair_cases = 0;
using SaveConstruct = void* (*)(void*);
using SaveDestroy = void (*)(void*);
using ItemToSave = void (*)(void*, const void*, std::uint16_t);
using SaveToItem = void (*)(const void*, void*, std::uint16_t*);
struct SaveProbe;
thread_local SaveProbe* save_probe = nullptr;
struct SaveProbe {
    Arena& arena;
    alignas(8) std::array<std::uint8_t, 0x138> saved{};
    std::array<std::array<std::uint8_t, 0x440>, 7> definitions{};
    unsigned key_calls = 0, notifications = 0, clock_calls = 0;
    bool valid = true;
    explicit SaveProbe(Arena& a) : arena(a) {
        saved.fill(0); std::fill(saved.begin() + 0x118, saved.end(), std::uint8_t(0xc7));
        for (unsigned i = 0; i < definitions.size(); ++i) {
            auto& d = definitions[i];
            put(d, 0, i == 0 ? 0u : i == 1 ? 10010u : 10018u + i);
            put(d, 0x400, std::uint16_t(i == 1 ? 100 : 30));
            put(d, 0x250, std::uint32_t(8)); put(d, 0x2e8, std::uint16_t(50));
            d[0x1fa] = 1; d[0x240] = 0x12;
        }
    }
    static void* definition(const std::uint16_t* key) noexcept {
        auto& p = *save_probe;
        if (*key == action::absent) return p.definitions[0].data();
        if (*key == 10) return p.definitions[1].data();
        if (*key >= 20 && *key <= 24) return p.definitions[*key - 18].data();
        p.valid = false; return p.definitions[0].data();
    }
    static std::uint16_t* external_key(std::uint16_t* out, std::uint32_t key) noexcept {
        auto& p = *save_probe;
        if (!key) *out = action::absent;
        else if (key == 10010) *out = 10;
        else if (key >= 10020 && key <= 10024) *out = static_cast<std::uint16_t>(key - 10000);
        else { p.valid = false; *out = action::absent; }
        return out;
    }
    static void* property_key(std::uint32_t* out, const void*, unsigned count, unsigned flags) noexcept {
        auto& p = *save_probe; ++p.key_calls;
        p.valid &= count == 1 && flags == 0x2ffff &&
            (out == reinterpret_cast<std::uint32_t*>(p.saved.data() + 0x98) ||
             out == reinterpret_cast<std::uint32_t*>(p.saved.data() + 0xb8) ||
             out == reinterpret_cast<std::uint32_t*>(p.saved.data() + 0xd8));
        *out = 0; return out;
    }
    static void notify(void* owner, void* key, void* vector) noexcept {
        auto& p = *save_probe; ++p.notifications;
        const auto at = reinterpret_cast<std::uintptr_t>(vector) - reinterpret_cast<std::uintptr_t>(p.saved.data());
        p.valid &= owner == p.saved.data() && (at == 0x80 || at == 0xa0 || at == 0xc0) &&
            key == static_cast<std::uint8_t*>(vector) + 0x18;
    }
    static std::uint64_t clock() noexcept { ++save_probe->clock_calls; return 10000; }
    static void* clear(void* out, int value, std::size_t size) noexcept {
        save_probe->valid &= value == 0 && size == 4;
        if (size <= 4) return std::memset(out, value, size);
        return out;
    }
    bool guards() const {
        return std::all_of(saved.begin() + 0x118, saved.end(), [](auto x) { return x == 0xc7; });
    }
    void bind() {
        save_probe = this;
        arena.redirect(0x38ab60, reinterpret_cast<std::uintptr_t>(&definition));
        arena.redirect(0x38aac0, reinterpret_cast<std::uintptr_t>(&external_key));
        arena.redirect(0x12bed40, reinterpret_cast<std::uintptr_t>(&property_key));
        arena.redirect(0x3d6170, reinterpret_cast<std::uintptr_t>(&notify));
        arena.redirect(0x1416c10, reinterpret_cast<std::uintptr_t>(&clock));
        arena.redirect(0x4898dd4, reinterpret_cast<std::uintptr_t>(&clear));
        arena.pointer(0x6d69458, 0); arena.pointer(0x6cef5d8, 0);
        for (const auto vt : {0x58b7b28u, 0x58b70a8u, 0x58b75e8u})
            arena.pointer(vt, reinterpret_cast<std::uintptr_t>(arena.function<void*>(0x44b220)));
    }
};
struct SaveSource {
    alignas(8) std::array<std::uint8_t, action::item_size> item{};
    alignas(8) std::array<std::uint8_t, 32> sockets{};
    alignas(8) std::array<std::uint8_t, 48> dyes{};
    bool operator==(const SaveSource&) const = default;
    void setup(unsigned count = 2, unsigned logical = 5) {
        put(item, 0, std::uint64_t(789)); put(item, 8, std::uint16_t(10));
        put(item, 0xa, std::uint16_t(7)); put(item, 0x10, std::int64_t(3));
        put(item, 0x20, std::uint64_t(28)); put(item, 0x40, std::uint16_t(35));
        put(item, 0x48, std::uint64_t(12000)); put(item, 0x50, std::uint64_t(25000));
        put(item, 0x58, std::uint16_t(9)); put(item, 0x90, action::absent);
        if (count) put(item, 0x60, sockets.data());
        put(item, 0x68, static_cast<std::uint32_t>(count)); put(item, 0x6c, static_cast<std::uint32_t>(count));
        item[0x70] = static_cast<std::uint8_t>(logical);
        for (unsigned i = 0; i < 5; ++i) {
            put(sockets, i * 6, static_cast<std::uint16_t>(20 + i));
            put(sockets, i * 6 + 2, static_cast<std::uint16_t>(5 + i * 3));
            sockets[i * 6 + 4] = static_cast<std::uint8_t>(i);
        }
    }
};
std::uintptr_t save_fault_ip = 0;
int save_fault(EXCEPTION_POINTERS* e) {
    save_fault_ip = e->ContextRecord->Rip; return EXCEPTION_EXECUTE_HANDLER;
}
DWORD invoke_item_to_save(ItemToSave f, void* out, const void* source, std::uint16_t slot) {
    __try { f(out, source, slot); return 0; }
    __except (save_fault(GetExceptionInformation())) { return GetExceptionCode(); }
}
DWORD invoke_save_to_item(SaveToItem f, const void* source, void* out, std::uint16_t* slot) {
    __try { f(source, out, slot); return 0; }
    __except (save_fault(GetExceptionInformation())) { return GetExceptionCode(); }
}
void save_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    install_item_allocator(arena); ItemHeap heap;
    arena.write(0x5d75240, pe.rva(0x5d75240, 16), PAGE_READONLY);
    // These values are private dependencies. They do not observe the game's flags.
    arena.pointer(0x6cf7248, 0);
    std::array<std::uint8_t, 6> empty_socket{}; put(empty_socket, 0, action::absent);
    std::array<std::uint8_t, action::item_size> empty_item{}; put(empty_item, 8, action::absent);
    arena.pointer(0x6cfcd90, reinterpret_cast<std::uintptr_t>(empty_socket.data()));
    arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty_item.data()));
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 19; ++scenario) {
        ++save_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode); tls[0x1ec] = 0;
        SaveProbe probe(arena); probe.bind(); SaveSource source;
        source.setup(scenario == 4 || scenario == 16 ? 0 : scenario == 5 || scenario == 17 ? 1 : scenario == 6 ? 5 : 2,
            scenario == 7 ? 0 : scenario == 17 ? 1 : 5);
        if (scenario == 1 || scenario == 15) put(source.item, 0x40, std::uint16_t(0));
        if (scenario == 2 || scenario == 13) put(source.item, 0x40, std::uint16_t(100));
        if (scenario == 3 || scenario == 14) put(probe.definitions[1], 0x400, action::absent);
        if (scenario == 8) put(source.sockets, 0, action::absent);
        if (scenario == 9 || scenario == 15) put(source.sockets, 2, std::uint16_t(0));
        if (scenario == 10) put(source.item, 0x40, action::absent);
        if (scenario == 11) put(source.item, 0x40, std::uint16_t(150));
        if (scenario == 18) {
            put(source.item, 0x78, source.dyes.data());
            put(source.item, 0x80, std::uint32_t(3)); put(source.item, 0x84, std::uint32_t(3));
            for (unsigned i = 0; i < 3; ++i) for (unsigned j = 0; j < 13; ++j)
                source.dyes[i * 16 + j] = static_cast<std::uint8_t>(17 + i * 20 + j);
        }
        const auto before = source;
        const auto allocs = heap.alloc_calls, frees = heap.free_calls;
        item::Value repaired(item_binding(arena));
        const void* save_source = source.item.data();
        if (scenario >= 12) {
            ++save_repair_cases;
            action::Snapshot snapshot; snapshot.session = {41, 0x12346, 7};
            snapshot.definitions = {{10, 100, scenario == 14 ? action::absent : std::uint16_t(100), scenario == 14}};
            for (std::uint16_t key = 20; key <= 24; ++key) snapshot.definitions.emplace_back(key, std::uint16_t(30));
            action::Item entry; entry.identity = {789, action::Area::carried, 0, 9};
            for (auto* image : {&entry.authority, &entry.presentation}) {
                image->bytes = source.item; image->sockets.resize(get<std::uint32_t>(source.item, 0x68));
                for (std::size_t i = 0; i < image->sockets.size(); ++i)
                    std::copy_n(source.sockets.begin() + i * 6, 6, image->sockets[i].bytes.begin());
            }
            snapshot.items.push_back(entry); action::Plan plan;
            require(action::prepare({snapshot.session, action::Scope::carried, {}}, snapshot, plan) == action::Code::prepared &&
                plan.count().main_fields == unsigned(scenario != 13) &&
                plan.count().socket_fields == entry.authority.sockets.size(),
                "Production repair plan records exactly the damaged main and initialized socket fields");
            ++native_calls; require(repaired.construct() == item::Code::constructed, "Native repair copy constructed");
            ++native_calls;
            require(repaired.prepare_repair(source.item.data(), plan, entry.identity, item::Realm::authority) == item::Code::prepared,
                "Save integration uses production repair preparation and original native item copying");
            save_source = repaired.data();
            require(item_read<std::uint16_t>(save_source, 0x40) == 100 && source == before,
                "Repair uses the finite vanilla target while preserving the source even under No-Wear");
            const auto* copy_sockets = item_read<const std::uint8_t*>(save_source, 0x60);
            for (unsigned i = 0; i < entry.authority.sockets.size(); ++i)
                require(item_read<std::uint16_t>(copy_sockets, i * 6 + 2) == 30,
                    "Prepared repair heals each existing socket before native save conversion");
        }
        const auto ctor_allocs = heap.alloc_calls, ctor_frees = heap.free_calls;
        ++native_calls;
        require(arena.function<SaveConstruct>(0x188b8f0)(probe.saved.data()) == probe.saved.data(),
            "Native ItemSaveData construction returns the private caller-owned object");
        require(probe.key_calls == 3 && probe.valid && probe.guards() && heap.alloc_calls == ctor_allocs && heap.free_calls == ctor_frees,
            "Save construction initializes three reflection vectors without data allocations");
        ++native_calls;
        const auto save_result = invoke_item_to_save(arena.function<ItemToSave>(0x188ccc0), probe.saved.data(), save_source, 9);
        require(!save_result, ("Original item-to-save fault " + std::to_string(save_result) + " at RVA " +
            std::to_string(save_fault_ip - reinterpret_cast<std::uintptr_t>(arena.function<void*>(0)))).c_str());
        require(source == before && probe.valid && probe.guards() && heap.valid && heap.guards(),
            "Native save conversion leaves every source byte and private guard intact");
        require(get<std::uint64_t>(probe.saved, 0x30) == 789 && get<std::uint32_t>(probe.saved, 0x38) == 10010 &&
            get<std::uint16_t>(probe.saved, 0x3c) == 9 && get<std::int64_t>(probe.saved, 0x40) == 3 &&
            get<std::uint16_t>(probe.saved, 0x50) == 7 && get<std::uint16_t>(probe.saved, 0x60) == item_read<std::uint16_t>(save_source, 0x40),
            "Original save fields preserve UID, external key, slot, quantity, enhancement and main endurance");
        const auto* records = get<const std::uint8_t*>(probe.saved, 0x80);
        require(get<std::uint32_t>(probe.saved, 0x88) == 5 && heap.covers(records, 5 * 0x30) &&
            probe.saved[0x78] == get<std::uint32_t>(source.item, 0x68) && probe.saved[0x79] == source.item[0x70],
            "Original save vector grows to five independent records and retains both socket count meanings");
        for (unsigned i = 0; i < 5; ++i) {
            const bool stored = i < get<std::uint32_t>(source.item, 0x68) && i < source.item[0x70];
            const auto* input_sockets = item_read<const std::uint8_t*>(save_source, 0x60);
            const auto key = stored ? item_read<std::uint16_t>(input_sockets, i * 6) : action::absent;
            const auto durability = stored ? item_read<std::uint16_t>(input_sockets, i * 6 + 2) : std::uint16_t(0);
            require(item_read<std::uint16_t>(records, i * 0x30 + 0x28) == durability &&
                item_read<std::uint32_t>(records, i * 0x30 + 0x2c) == (key == action::absent ? 0u : 10000u + key),
                "Every saved socket has the source durability and external identity, or the private empty record");
        }
        const auto saved_before_load = probe.saved;
        std::array<std::uint8_t, 5 * 0x30> records_before_load{};
        std::memcpy(records_before_load.data(), records, records_before_load.size());
        const auto dye_count = get<std::uint32_t>(source.item, 0x80);
        const auto* saved_dyes = get<const std::uint8_t*>(probe.saved, 0xa0);
        require(get<std::uint32_t>(probe.saved, 0xa8) == dye_count && (!dye_count || heap.covers(saved_dyes, dye_count * 0x38)),
            "Native save conversion owns exactly the source dye records");
        std::array<std::uint8_t, 3 * 0x38> dyes_before_load{};
        if (dye_count) std::memcpy(dyes_before_load.data(), saved_dyes, dye_count * 0x38);
        for (unsigned i = 0; i < dye_count; ++i) {
            require(std::memcmp(saved_dyes + i * 0x38 + 0x28, source.dyes.data() + i * 16 + 6, 6) == 0 &&
                item_read<std::uint32_t>(saved_dyes, i * 0x38 + 0x30) == get<std::uint32_t>(source.dyes, i * 16) &&
                item_read<std::uint16_t>(saved_dyes, i * 0x38 + 0x34) == get<std::uint16_t>(source.dyes, i * 16 + 4) &&
                saved_dyes[i * 0x38 + 0x36] == source.dyes[i * 16 + 12],
                "Save conversion preserves all thirteen meaningful dye bytes across the different record layout");
        }
        {
            item::Value loaded(item_binding(arena)); ++native_calls;
            require(loaded.construct() == item::Code::constructed, "Private destination item constructed before load conversion");
            std::uint16_t slot = action::absent;
            ++native_calls;
            // Only this fixture's freshly constructed, independently owned value.
            const auto load_result = invoke_save_to_item(arena.function<SaveToItem>(0x188c440), probe.saved.data(), const_cast<void*>(loaded.data()), &slot);
            require(!load_result, ("Original save-to-item fault " + std::to_string(load_result) + " at RVA " +
                std::to_string(save_fault_ip - reinterpret_cast<std::uintptr_t>(arena.function<void*>(0)))).c_str());
            const auto main = scenario == 3 || scenario == 14 ? action::absent : scenario == 1 ? std::uint16_t(1) :
                scenario == 10 || scenario == 11 ? std::uint16_t(100) : item_read<std::uint16_t>(save_source, 0x40);
            require(slot == 9 && item_read<std::uint64_t>(loaded.data(), 0) == 789 &&
                item_read<std::uint16_t>(loaded.data(), 8) == 10 && item_read<std::int64_t>(loaded.data(), 0x10) == 3 &&
                item_read<std::uint16_t>(loaded.data(), 0xa) == 7 && item_read<std::uint16_t>(loaded.data(), 0x40) == main,
                "Original loader preserves identity/quantity/enhancement and applies its actual main-endurance normalization");
            const auto* sockets = item_read<const std::uint8_t*>(loaded.data(), 0x60);
            require(item_read<std::uint32_t>(loaded.data(), 0x68) == 5 && heap.covers(sockets, 30) &&
                item_read<std::uint8_t>(loaded.data(), 0x70) == source.item[0x70],
                "Original initializing constructor materializes five socket records with the saved logical bound");
            for (unsigned i = 0; i < 5; ++i) {
                const auto external = item_read<std::uint32_t>(records, i * 0x30 + 0x2c);
                require(item_read<std::uint16_t>(sockets, i * 6) == (external ? external - 10000u : action::absent) &&
                    item_read<std::uint16_t>(sockets, i * 6 + 2) == item_read<std::uint16_t>(records, i * 0x30 + 0x28),
                    "Original loader and item constructor preserve all five saved socket identities and endurance words");
            }
            require(probe.saved == saved_before_load && std::memcmp(records, records_before_load.data(), records_before_load.size()) == 0,
                "Original loader leaves the entire source save object and its socket vector unchanged");
            const auto* loaded_dyes = item_read<const std::uint8_t*>(loaded.data(), 0x78);
            require(item_read<std::uint32_t>(loaded.data(), 0x80) == dye_count &&
                (!dye_count || (heap.covers(loaded_dyes, dye_count * 16) &&
                    std::memcmp(saved_dyes, dyes_before_load.data(), dye_count * 0x38) == 0)),
                "Native loader independently owns the dye vector and leaves its saved source intact");
            for (unsigned i = 0; i < dye_count; ++i)
                require(std::memcmp(loaded_dyes + i * 16, source.dyes.data() + i * 16, 13) == 0,
                    "Repaired save round trip retains each dye record's meaningful bytes");
            require(source == before && probe.valid && probe.guards() && heap.valid && heap.guards(),
                "Round trip mutates only its own independent destinations");
            if (scenario == 18) {
                SaveSource replacement; replacement.setup(1, 5);
                put(replacement.item, 0, std::uint64_t(790)); put(replacement.item, 8, action::absent);
                const auto socket_capacity = get<std::uint32_t>(probe.saved, 0x8c);
                const auto dye_capacity = get<std::uint32_t>(probe.saved, 0xac);
                ++native_calls;
                require(!invoke_item_to_save(arena.function<ItemToSave>(0x188ccc0), probe.saved.data(), replacement.item.data(), 2),
                    "Native invalid-item conversion resets an already populated save object");
                require(get<std::uint64_t>(probe.saved, 0x30) == UINT64_MAX && !get<std::uint32_t>(probe.saved, 0x38) &&
                    get<std::uint16_t>(probe.saved, 0x3c) == action::absent && !get<std::int64_t>(probe.saved, 0x40) &&
                    !get<std::uint16_t>(probe.saved, 0x60) && !get<std::uint32_t>(probe.saved, 0x88) &&
                    !get<std::uint32_t>(probe.saved, 0xa8) && get<const std::uint8_t*>(probe.saved, 0x80) == records &&
                    get<const std::uint8_t*>(probe.saved, 0xa0) == saved_dyes &&
                    get<std::uint32_t>(probe.saved, 0x8c) == socket_capacity && get<std::uint32_t>(probe.saved, 0xac) == dye_capacity,
                    "Original reset removes valid item identity and both record counts while retaining reusable owned capacity");
                put(replacement.item, 8, std::uint16_t(10));
                const auto replacement_before = replacement;
                ++native_calls;
                require(!invoke_item_to_save(arena.function<ItemToSave>(0x188ccc0), probe.saved.data(), replacement.item.data(), 2),
                    "Original conversion repopulates the same save object with a different item");
                ++native_calls;
                require(!invoke_save_to_item(arena.function<SaveToItem>(0x188c440), probe.saved.data(), const_cast<void*>(loaded.data()), &slot),
                    "Native loader replaces an already populated destination item");
                require(slot == 2 && item_read<std::uint64_t>(loaded.data(), 0) == 790 &&
                    item_read<std::uint16_t>(loaded.data(), 0x40) == 35 && !item_read<std::uint32_t>(loaded.data(), 0x80),
                    "Reused destination has the replacement identity/endurance and no previous dye entries");
                const auto* replaced_sockets = item_read<const std::uint8_t*>(loaded.data(), 0x60);
                require(item_read<std::uint16_t>(replaced_sockets, 0) == 20 && item_read<std::uint16_t>(replaced_sockets, 2) == 5,
                    "Reused destination receives the replacement socket");
                for (unsigned i = 1; i < 5; ++i)
                    require(item_read<std::uint16_t>(replaced_sockets, i * 6) == action::absent &&
                        !item_read<std::uint16_t>(replaced_sockets, i * 6 + 2), "Old sockets never survive save object reuse");
                require(replacement == replacement_before && source == before && probe.valid && probe.guards() && heap.valid && heap.guards(),
                    "Reset/reuse/replacement preserves all sources and heap guards");
            }
            ++native_calls; require(loaded.release() == item::Code::released, "Loaded native item destroyed");
        }
        ++native_calls; arena.function<SaveDestroy>(0x188bcb0)(probe.saved.data());
        if (scenario >= 12) {
            ++native_calls; require(repaired.release() == item::Code::released, "Prepared repair copy released after save round trip");
        }
        require(!heap.live() && heap.valid && heap.guards() && probe.guards(),
            "Original save/vector and native item destructors release every private allocation");
        require(heap.alloc_calls[1 - mode] == allocs[1 - mode] && heap.free_calls[1 - mode] == frees[1 - mode] &&
            heap.alloc_calls[mode] - allocs[mode] == heap.free_calls[mode] - frees[mode],
            "Both TLS allocation modes balance full item/save round trips");
    }
    save_probe = nullptr;
}
