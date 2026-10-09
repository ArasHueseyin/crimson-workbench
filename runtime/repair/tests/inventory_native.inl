// Included in the private 2949 host after its arena/actor/reader fixtures.
// Native accessors read only caller-owned objects. No bootstrap or game writes.
unsigned inventory_cases = 0, equipment_cases = 0, selected_cases = 0, updater_cases = 0;
using InventorySlot = void* (*)(void*, std::uint16_t, std::int16_t);
using InventoryHolder = void* (*)(void*);
using EnduranceUpdate = bool (*)(void*, std::int16_t);
using EquipmentChange = void* (*)(void*, std::uint32_t*, std::uint16_t, std::int16_t);
using CurrentActor = void* (*)(void*, reference::Receipt*);
DWORD invoke_slot(InventorySlot function, void* holder, std::uint16_t type, std::int16_t slot, void** result) {
    __try { *result = function(holder, type, slot); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
thread_local std::array<std::uint8_t, 0x440> main_definition{}, socket_definition{};
thread_local std::array<std::uint8_t, 0xc0> character_definition{};
thread_local std::array<std::uint8_t, 0x60> inventory_mode{};
thread_local bool layout_callbacks_ok = true;
thread_local unsigned default_creates = 0, default_destroys = 0;
thread_local std::array<std::uintptr_t, 64> definition_queries{};
thread_local unsigned definition_query_count = 0;
void* character_info(void*) noexcept { return character_definition.data(); }
void* holder_mode(void*) noexcept { return inventory_mode.data(); }
void* item_definition(const std::uint16_t* key) noexcept {
    if (definition_query_count < definition_queries.size())
        definition_queries[definition_query_count++] = reinterpret_cast<std::uintptr_t>(key);
    else layout_callbacks_ok = false;
    if (*key == 10) return main_definition.data();
    if (*key != 20) layout_callbacks_ok = false;
    return socket_definition.data();
}
void* default_item(void* destination) noexcept {
    ++default_creates;
    // Only the wrapper's empty temporary item. Native nonzero-delta copy,
    // collection and event paths are deliberately outside this layout test.
    std::memset(destination, 0, action::item_size);
    const auto empty_uid = UINT64_MAX;
    std::memcpy(destination, &empty_uid, 8);
    std::memcpy(static_cast<std::uint8_t*>(destination) + 8, &action::absent, 2);
    return destination;
}
void dispose_default(void* destination) noexcept {
    ++default_destroys;
    std::uint16_t key;
    std::memcpy(&key, static_cast<std::uint8_t*>(destination) + 8, 2);
    layout_callbacks_ok &= key == action::absent;
}
void observe_layout(Pe& pe) {
    struct Observation { std::uint32_t rva, size; const char* sha; };
    for (const auto& o : std::array{
        Observation{0x39b670, 0x253, "0a5de335c54598f27c33104c3d54f130f403e8e1054467522336b0035a211f7b"},
        Observation{0x39ec90, 0x25a, "c089608d1f85347731dfbc633c0d3ad7fc0a371074145d9ed6a6487cd8d71531"},
        Observation{0x2a94c50, 0x14b, "54407dc82a093fe4abdd092c19ec08d42389302d8909afbc7112f83317de3c2c"}}) {
        require(hash(pe.rva(o.rva, o.size)) == o.sha, "Complete current-build anchor caller hash");
        pe.function(o.rva, o.size); // Static evidence only, not executed.
    }
    for (const auto [site, expected] : std::array<std::pair<std::uint32_t, std::uintptr_t>, 3>{{
        {0x39b7d9, reader::client_context_rva}, {0x39eccd, reader::client_context_rva},
        {0x2a94d1c, reader::server_context_rva}}}) {
        const auto instruction = pe.rva(site, 7);
        std::int32_t relative; std::memcpy(&relative, instruction.data() + 3, 4);
        require(instruction[0] == 0x48 && instruction[1] == 0x8b &&
            (instruction[2] == 0x05 || instruction[2] == 0x0d) &&
            static_cast<std::int64_t>(site) + 7 + relative == static_cast<std::int64_t>(expected), "Independent RIP-relative context anchor");
    }
    require(pe.rva(0x39b7e0, 8) == std::vector<std::uint8_t>{0x48,0x8b,0x48,0x30,0x48,0x8b,0x59,0x50} &&
        pe.rva(0x39ecd4, 8) == std::vector<std::uint8_t>{0x48,0x8b,0x48,0x30,0x48,0x8b,0x59,0x50},
        "Two current client context/manager/selection chains");
    require(pe.rva(0x2a94d23, 4) == std::vector<std::uint8_t>{0x48,0x8b,0x49,0x48} &&
        pe.rva(0x2a94d43, 8) == std::vector<std::uint8_t>{0x48,0x8b,0x48,0x68,0x48,0x8b,0x49,0x38},
        "Current server manager and actor-to-equipment caller chain");
}
void inventory_native_tests(Arena& arena) {
    std::array<std::uint8_t, action::item_size> empty{};
    put(empty, 8, action::absent);
    arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty.data()));
    std::array<std::uint8_t, 0x28> holder{};
    std::array<std::uint8_t, 0x30> bag{}, storage{};
    std::array<std::uint8_t, 3 * action::item_size> items{};
    std::array<std::uint8_t, 24> exclusions{};
    std::array<void*, 2> bags{storage.data(), bag.data()};
    put(holder, 0x18, bags.data()); put(holder, 0x20, std::uint32_t(2));
    put(storage, 0x10, std::uint16_t(8));
    put(bag, 0, items.data()); put(bag, 8, std::uint16_t(1));
    put(bag, 0xc, std::uint16_t(3)); put(bag, 0x10, std::uint16_t(2));
    for (std::size_t i = 0; i < 3; ++i) {
        put(items, i * action::item_size, std::uint64_t(i + 1));
        put(items, i * action::item_size + 8, static_cast<std::uint16_t>(10 + i));
        put(items, i * action::item_size + 0x10, std::int64_t(1));
    }
    auto compare = [&](std::uint16_t type, std::int16_t slot, std::uintptr_t expected) {
        const auto before_items = items;
        const auto before_empty = empty;
        const auto before_holder = holder;
        const auto before_bag = bag, before_storage = storage;
        const auto before_exclusions = exclusions;
        void* native = nullptr;
        ++native_calls; ++inventory_cases;
        require(invoke_slot(arena.function<InventorySlot>(0x212f4b0), holder.data(), type, slot, &native) == 0,
            "Current native inventory lookup completes on private storage");
        auto normalized = reinterpret_cast<std::uintptr_t>(native);
        if (native == empty.data()) normalized = 0;
        else {
            const auto first = reinterpret_cast<std::uintptr_t>(items.data());
            require(normalized >= first && normalized < first + items.size() && (normalized - first) % action::item_size == 0,
                "Native result is exactly one fixture item slot");
            const auto at = normalized - first;
            if (get<std::uint16_t>(items, at + 8) == action::absent || get<std::int64_t>(items, at + 0x10) <= 0) normalized = 0;
        }
        reader::LocalMemory memory;
        std::uintptr_t captured = UINTPTR_MAX;
        require(reader::inventory_slot(memory, reinterpret_cast<std::uintptr_t>(holder.data()), type, slot, captured) ==
            reader::Code::captured && captured == expected && normalized == expected, "Current native getter and reader agree");
        require(before_items == items && before_empty == empty && before_holder == holder && before_bag == bag &&
            before_storage == storage && before_exclusions == exclusions, "Lookup preserves all item/container/filter bytes");
    };
    for (std::int16_t slot = 0; slot < 3; ++slot) compare(2, slot, reinterpret_cast<std::uintptr_t>(items.data() + slot * action::item_size));
    for (const auto slot : {std::int16_t(-32768), std::int16_t(-1), std::int16_t(3), std::int16_t(32767)}) compare(2, slot, 0);
    compare(99, 0, 0);
    put(items, 8, action::absent); compare(2, 0, 0); put(items, 8, std::uint16_t(10));
    for (const auto quantity : {std::int64_t(0), std::int64_t(-1)}) { put(items, 0x10, quantity); compare(2, 0, 0); }
    put(items, 0x10, std::int64_t(1)); put(bag, 0x20, exclusions.data()); put(bag, 0x28, std::uint32_t(2));
    put(exclusions, 0, std::uint16_t(99)); put(exclusions, 12, std::uint16_t(10));
    compare(2, 0, 0); compare(2, 2, reinterpret_cast<std::uintptr_t>(items.data() + 2 * action::item_size));
    put(bag, 0, arena.function<void*>(0x1000));
    void* unused = nullptr;
    ++native_calls; ++inventory_cases;
    require(invoke_slot(arena.function<InventorySlot>(0x212f4b0), holder.data(), 2, 0, &unused) == EXCEPTION_ACCESS_VIOLATION,
        "Current four-fragment inventory unwind survives inaccessible private item");
    reader::LocalMemory memory;
    std::uintptr_t missing = 1;
    require(reader::inventory_slot(memory, reinterpret_cast<std::uintptr_t>(holder.data()), 2, 0, missing) ==
        reader::Code::unreadable && !missing, "Reader clears inaccessible private item result");

    std::array<std::uint8_t, 0xa8> actor{}, pawn{};
    std::array<std::uint8_t, 0xc0> parts{}, pawn_parts{};
    std::array<std::uint8_t, 0xd8> possessor{};
    std::array<std::uint8_t, 0x32> status{};
    put(actor, 0x68, parts.data()); put(actor, 0xa0, possessor.data()); put(parts, 0x20, status.data());
    put(parts, 0xb8, holder.data()); put(pawn, 0x68, pawn_parts.data()); put(pawn_parts, 0xb8, storage.data());
    put(possessor, 0xd0, pawn.data());
    auto resolve = [&](void* expected) {
        ++native_calls; ++inventory_cases;
        require(arena.function<InventoryHolder>(0x212a100)(actor.data()) == expected, "Current native holder owner/possessor selection");
    };
    put(character_definition, 0xbe, action::absent); resolve(holder.data());
    put(character_definition, 0xbe, std::uint16_t(1)); inventory_mode[0x5f] = 0; resolve(nullptr);
    inventory_mode[0x5f] = 3; resolve(holder.data()); inventory_mode[0x5f] = 1; resolve(storage.data());
    put(possessor, 0xd0, actor.data()); resolve(holder.data());
    put(possessor, 0xd0, std::uintptr_t(0)); resolve(nullptr); put(actor, 0xa0, std::uintptr_t(0)); resolve(nullptr);
}
void selected_native_tests(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        Fixture f; fixture = &f;
        const bool tracked = scenario == 2 || scenario == 7;
        tls[0x1d2] = std::uint8_t(!tracked); tls[0x1d4] = 0;
        initialize(f, arena, 0x12346, scenario == 1 || scenario == 6);
        auto& actor = f.actors[0];
        put(f.managers[0].bytes, 0x50, actor.bytes.data());
        if (scenario == 3) put(f.managers[0].bytes, 0x50, std::uintptr_t(0));
        if (scenario == 4) put(actor.bytes, 0x5e, std::uint16_t(0));
        if (scenario == 5 || scenario == 7) actor.bytes[0x4a] = 0;
        if (scenario == 6) put(actor.bytes, 0x5e, std::uint16_t(0x10));
        f.selecting_current = true;
        reference::Receipt receipt;
        ++native_calls; ++selected_cases;
        require(arena.function<CurrentActor>(0x8b4480)(f.managers[0].bytes.data(), &receipt) == &receipt,
            "Current-selection accessor writes its caller-owned receipt");
        const bool accepted = scenario < 3 || scenario == 5;
        require(receipt.actor() == (accepted ? reinterpret_cast<std::uintptr_t>(actor.bytes.data()) : 0),
            "Current-selection accessor uses native normal/user acquisition and validity byte");
        ++native_calls; native_destroy(&receipt);
        require(f.valid && !actor.tracked && !get<std::uint32_t>(actor.bytes, 0x10), "Selected receipt cleanup balances real ownership");
        require(actor.destroyed == unsigned(scenario == 5), "Direct dead selection remains readable ownership only, not repair permission");
    }
    fixture = nullptr;
}
void endurance_native_tests(Arena& arena) {
    put(main_definition, 0x400, std::uint16_t(100)); put(socket_definition, 0x400, std::uint16_t(30));
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        definition_query_count = 0;
        Actor actor; InventoryFixture v; v.initialize(actor);
        if (scenario == 2) put(main_definition, 0x400, action::absent);
        const auto before = v;
        ++native_calls; ++updater_cases;
        const auto changed = arena.function<EnduranceUpdate>(0x240d660)(v.item.data(), scenario == 1 ? 65 : 0);
        if (scenario == 1) require(changed && get<std::uint16_t>(v.item, 0x40) == 100 && get<std::uint16_t>(v.sockets, 2) == 0,
            "Current updater still consumes damaged socket with positive main repair delta; unsuitable as own repair writer");
        else require(!changed && before == v, "Current native zero-delta path preserves all item/socket bytes");
    }
    put(main_definition, 0x400, std::uint16_t(100));
}
void equipment_native_tests(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    std::array<std::uint8_t, action::item_size> empty{};
    put(empty, 8, action::absent); arena.pointer(0x6cfcd98, reinterpret_cast<std::uintptr_t>(empty.data()));
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        definition_query_count = 0;
        Fixture f; fixture = &f;
        tls[0x1d2] = 1; tls[0x1d4] = 0;
        initialize(f, arena, 0x12346, false);
        InventoryFixture v; v.initialize(f.actors[1]);
        std::array<std::uint8_t, 2 * 0xd0> entries{};
        std::copy(v.equipped.begin(), v.equipped.end(), entries.begin() + 0xd0);
        put(entries, 8, action::absent); put(entries, 0xc8, std::uint16_t(1));
        put(v.equipment_table, 8, entries.data()); put(v.equipment_table, 0x10, std::uint32_t(2));
        std::uint16_t slot = scenario == 1 ? 7 : 3;
        if (scenario == 2) put(v.equipment_table, 0x10, std::uint32_t(0));
        if (scenario == 3) put(entries, 0xd0 + 8, action::absent);
        if (scenario == 4) put(entries, 0xd0 + 0x10, std::int64_t(0));
        if (scenario == 5) put(socket_definition, 0x400, action::absent);
        if (scenario == 6) { put(entries, 0xd0 + 0x60, v.sockets.data()); put(entries, 0xd0 + 0x68, std::uint32_t(2)); entries[0xd0 + 0x70] = 2; }
        if (scenario == 7) put(entries, 0xd0 + 0x40, std::uint16_t(12));
        const auto before = entries;
        const auto before_inventory = v;
        const auto created = default_creates, destroyed = default_destroys;
        std::uint32_t error = UINT32_MAX;
        ++native_calls; ++equipment_cases;
        require(arena.function<EquipmentChange>(0x20c90f0)(v.equipment.data(), &error, slot, 0) == &error && error == 0,
            "Original equipment wrapper resolves second slot by tag/stride and completes its zero-delta path");
        const auto wanted_key = reinterpret_cast<std::uintptr_t>(entries.data() + 0xd0 + 8);
        const bool present = scenario == 0 || scenario >= 5;
        require(present ? std::find(definition_queries.begin(), definition_queries.begin() + definition_query_count, wanted_key) !=
            definition_queries.begin() + definition_query_count : definition_query_count == 0,
            "Native equipment path actually queries the matching second slot, and never a missing/empty/consumed slot");
        for (unsigned i = 0; i < definition_query_count; ++i)
            require(definition_queries[i] == wanted_key || (scenario == 6 &&
                (definition_queries[i] == reinterpret_cast<std::uintptr_t>(v.sockets.data()) ||
                 definition_queries[i] == reinterpret_cast<std::uintptr_t>(v.sockets.data() + 6))),
                "Native definition queries stay on the selected equipment item and its sockets");
        require(entries == before && v == before_inventory, "Native equipment layout probe leaves every source byte unchanged");
        const auto& lock = f.actors[1].owner_lock;
        require(!get<std::uint32_t>(lock, 8) && !get<std::uint32_t>(lock, 0x2c),
            "Original lock-owner helper and equipment return balance lock reference and recursion");
        const auto available = TryAcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(f.actors[1].owner_lock.data() + 0x10));
        if (available) ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(f.actors[1].owner_lock.data() + 0x10));
        require(available && default_creates == created + 1 && default_destroys == destroyed + 1 && layout_callbacks_ok,
            "Equipment wrapper frees actual Windows lock and exactly its fixture temporary item");
        put(socket_definition, 0x400, std::uint16_t(30));
    }
    fixture = nullptr;
}
void install_layout_dependencies(Arena& arena) {
    arena.redirect(0x389570, reinterpret_cast<std::uintptr_t>(&character_info));
    arena.redirect(0x3891b0, reinterpret_cast<std::uintptr_t>(&holder_mode));
    arena.redirect(0x38ab60, reinterpret_cast<std::uintptr_t>(&item_definition));
    arena.redirect(0x2409750, reinterpret_cast<std::uintptr_t>(&default_item));
    arena.redirect(0x240af10, reinterpret_cast<std::uintptr_t>(&dispose_default));
}
