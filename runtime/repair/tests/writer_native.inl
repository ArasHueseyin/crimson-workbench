unsigned writer_cases = 0;
void writer_native_tests(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    constexpr std::uint32_t handle = 0x12346;
    const std::array definitions{action::Definition{10, 100}, action::Definition{20, 30}};
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        ++writer_cases;
        Fixture f; fixture = &f;
        tls[0x1d2] = std::uint8_t(scenario < 4); tls[0x1d4] = 0;
        initialize(f, arena, handle, false);
        std::array<InventoryFixture, 2> inventories;
        for (std::size_t i = 0; i < 2; ++i) inventories[i].initialize(f.actors[i]);
        auto expected = inventories;
        CaptureMemory memory(f, inventories);
        registry::Source client(f.managers[0].bytes.data(), lookup_client, release_client);
        registry::Source server(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{client.source(handle), server.source(handle)};
        const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
            lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
        {
            reader::HeldCapture held;
            require(held.open(memory, memory.frame, definitions, sources, bindings) == reader::Code::captured,
                "Actual references and SRW locks protect the production field writer");
            const auto scope = scenario % 4 == 1 ? action::Scope::carried : scenario % 4 == 2 ? action::Scope::equipped :
                scenario % 4 == 3 ? action::Scope::one : action::Scope::all;
            action::Plan plan;
            require(action::prepare({held.view()->snapshot.session, scope, {11, action::Area::carried, 2, 0}},
                held.view()->snapshot, plan) == action::Code::prepared, "Production field plan from private native-owned sources");
            for (auto& v : expected) {
                if (scope != action::Scope::equipped) { put(v.item, 0x40, std::uint16_t(100)); put(v.sockets, 2, std::uint16_t(30)); }
                if (scope == action::Scope::all || scope == action::Scope::equipped) put(v.equipped, 0x40, std::uint16_t(30));
            }
            const auto result = writer::Batch::apply(held, memory.frame, plan);
            require(result.code == writer::Code::fields_written &&
                result.written_words == (plan.count().main_fields + plan.count().socket_fields) * 2,
                "Production LocalAccess writes exactly the planned words in both actual private allocations");
            require(inventories == expected && held.view() && action::verify_applied(plan, held.view()->snapshot) == action::Code::applied,
                "Native-owned sources match the independent byte oracle for carried/equipped/all/one");
            require(f.actors[0].owner_enters == 1 && !f.actors[0].owner_leaves && f.actors[1].owner_enters == 1 &&
                !f.actors[1].owner_leaves && !memory.registry_reads, "No lease gap or second unprotected registry search during mutation");
            require(writer::Batch::apply(held, memory.frame, plan).code == writer::Code::already_attempted,
                "Production native-owned batch cannot replay before events");
        }
        require(f.valid && f.releases == std::vector<unsigned>{1,0}, "Native field fixture closes locks before reverse source cleanup");
        for (const auto& actor : f.actors)
            require(!get<std::uint32_t>(actor.bytes, 0x10) && !actor.tracked && !get<std::uint16_t>(actor.bytes, 0x48) &&
                actor.owner_enters == actor.owner_leaves && !get<std::uint32_t>(actor.owner_lock, 0x2c),
                "Native direct/tracked references and owner locks balance after field mutation");
    }
    fixture = nullptr;
}
