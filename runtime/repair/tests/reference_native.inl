// Included inside the isolated harness namespace, after Arena and fixture helpers.
// Actual reference acquisition/release/last-owner decision; thread tracking,
// embedded object lock and final object destruction remain explicit callbacks.
using AcquireActor = bool (*)(void*, std::uint32_t, std::uint16_t, const std::uint32_t*) noexcept;
using ReleaseReceipt = void (*)(reference::Receipt*) noexcept;
AcquireActor native_acquire_actor = nullptr;
ReleaseReceipt native_release_receipt = nullptr;

struct alignas(8) ReferenceActor {
    std::array<std::uint8_t, 0x90> bytes{};
    std::array<std::uintptr_t, 32> methods{};
    std::array<std::uintptr_t, 3> embedded_methods{};
    unsigned lock_depth = 0, tracked = 0, destroyed = 0, enters = 0, leaves = 0;
    unsigned cleanup = 0;
};
struct ReferenceProbe {
    std::array<ReferenceActor*, 2> actors{};
    bool valid = true;
    ReferenceActor* find(void* object, std::size_t offset = 0) noexcept {
        for (auto* actor : actors) if (actor && actor->bytes.data() + offset == object) return actor;
        valid = false; return nullptr;
    }
};
ReferenceProbe* reference_probe = nullptr;
bool fixture_reference_server(void*) noexcept { return true; }
void* fixture_reference_parent(void*) noexcept { return nullptr; }
bool fixture_reference_reacquire(void*) noexcept { return false; }
void fixture_reference_cleanup(void* object) noexcept {
    if (auto* actor = reference_probe->find(object)) {
        reference_probe->valid &= !actor->lock_depth && get<std::uint16_t>(actor->bytes, 0x5e) == 0x40;
        ++actor->cleanup;
    }
}
void fixture_reference_enter(void* object) noexcept {
    if (auto* actor = reference_probe->find(object, 0x18)) { ++actor->lock_depth; ++actor->enters; }
}
void fixture_reference_leave(void* object) noexcept {
    if (auto* actor = reference_probe->find(object, 0x18)) {
        reference_probe->valid &= actor->lock_depth > 0;
        --actor->lock_depth; ++actor->leaves;
    }
}
void fixture_reference_destroy(void* object) noexcept {
    if (auto* actor = reference_probe->find(object)) {
        reference_probe->valid &= !actor->lock_depth && !get<std::uint32_t>(actor->bytes, 0x10) &&
            !get<std::uint16_t>(actor->bytes, 0x48) && !actor->bytes[0x4a] && actor->bytes[0x4b] == 1;
        ++actor->destroyed; // fixture stays allocated so a second delete is detectable
    }
}
void fixture_reference_register(void* object, bool special) noexcept {
    if (auto* actor = reference_probe->find(object)) {
        reference_probe->valid &= !special && actor->lock_depth == 1;
        ++actor->tracked;
        put(actor->bytes, 0x48, std::uint16_t(get<std::uint16_t>(actor->bytes, 0x48) + 1));
    }
}
void fixture_reference_unregister(void* object, bool special) noexcept {
    if (auto* actor = reference_probe->find(object)) {
        reference_probe->valid &= !special && actor->lock_depth == 1 && actor->tracked > 0;
        --actor->tracked;
        put(actor->bytes, 0x48, std::uint16_t(get<std::uint16_t>(actor->bytes, 0x48) - 1));
    }
}
void* fixture_reference_map() noexcept { return reference_probe; }
void* fixture_reference_lookup(void* map, void** key) noexcept {
    reference_probe->valid &= map == reference_probe;
    auto* actor = reference_probe->find(*key);
    return actor && actor->tracked ? &actor->tracked : nullptr;
}
void acquire_reference_native(void* object, std::uint32_t handle, reference::Receipt& receipt) noexcept {
    // Stand-in for the protected source resolver; these fixture actors cannot
    // disappear before acquisition. The actual host must hold registry protection.
    ++native_calls;
    const auto ok = native_acquire_actor(object, 4, 0x10, &handle);
    put(receipt.bytes, 8, object);
    receipt.bytes[0x10] = std::uint8_t(ok);
}
void release_reference_native(void*, reference::Receipt& receipt) noexcept {
    ++native_calls;
    native_release_receipt(&receipt);
}

void reference_native_tests(Pe& pe, Arena& arena) {
    struct Function { std::uint32_t rva, size, tls; const char* sha; };
    const std::array functions{
        Function{0x1434900, 0x9c, 0, "58962428c45832afd42ef53dbda8feeef6d8bebb53c709f1d79c683bbdc49860"},
        Function{0x1436690, 0x2f5, 0x14366bd, "0a7467851808949a1dddfbbda6f4d1270cefe1262ce80c796da571a171a7b8ad"},
        Function{0x1436bf0, 0xf6, 0x1436c0b, "30eec32cccb2b3447885935521b2c92e9c951f9a7d6fb729b09f81833beca0d9"},
        Function{0x1436990, 0x98, 0x143699f, "17015ad6da39a35b7248364f8e9622fe0d6e221284e201acb38a08f3abc0386b"},
        Function{0x4f9000, 0xc, 0, "90521d0c1b64f081e463c6d47ce1ce830aadf6a417b7607269319860382e5eb6"}};
    std::array<std::uint8_t, 0x200> tls{};
    tls[0x98] = 1; // fixture thread initialized; never call real game TLS init
    const auto tls_pointer = tls.data();
    arena.write(0x6cffd00, {reinterpret_cast<const std::uint8_t*>(&tls_pointer), sizeof(tls_pointer)}, PAGE_READONLY);
    for (const auto& fn : functions) {
        auto code = pe.rva(fn.rva, fn.size);
        require(hash(code) == fn.sha, "Pinned reference function hash");
        const auto original = code;
        if (fn.tls) {
            const auto offset = fn.tls - fn.rva;
            const std::array<std::uint8_t, 9> gs{0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
            require(std::equal(gs.begin(), gs.end(), code.begin() + offset), "Reference fixture pinned TLS read");
            std::array<std::uint8_t, 9> replacement{0x48,0x8d,0x05,0,0,0,0,0x90,0x90};
            const auto displacement = std::int32_t(0x6cffd00 - (fn.tls + 7));
            std::memcpy(replacement.data() + 3, &displacement, 4);
            std::copy(replacement.begin(), replacement.end(), code.begin() + offset);
        }
        for (std::size_t i = 0; i < code.size(); ++i)
            if (!fn.tls || i < fn.tls - fn.rva || i >= fn.tls - fn.rva + 9)
                require(code[i] == original[i], "Reference bytes unchanged outside fixture TLS read");
        arena.write(fn.rva, code, PAGE_EXECUTE_READ);
    }
    arena.redirect(0x14344f0, reinterpret_cast<std::uintptr_t>(&fixture_reference_register));
    arena.redirect(0x1434720, reinterpret_cast<std::uintptr_t>(&fixture_reference_unregister));
    arena.redirect(0x14342b0, reinterpret_cast<std::uintptr_t>(&fixture_reference_map));
    arena.redirect(0x3d1e80, reinterpret_cast<std::uintptr_t>(&fixture_reference_lookup));
    arena.redirect(0x1434420, reinterpret_cast<std::uintptr_t>(&fixture_reference_reacquire));
    native_acquire_actor = arena.function<AcquireActor>(0x4f9000);
    native_release_receipt = arena.function<ReleaseReceipt>(0x1434900);
    constexpr std::uint32_t handle = 0x12345;
    // Verify the release override in every concrete actor table we examined.
    for (const auto vt : {0x57cda50u, 0x5b1dc30u, 0x5585c00u, 0x5b255a0u, 0x55b5958u}) {
        const auto bytes = pe.rva(vt + 8, 8);
        std::uint64_t method = 0;
        std::memcpy(&method, bytes.data(), 8);
        require(method == 0x141436690ull, "Actual actor VTable uses the derived lifecycle release, not base refcount release");
    }
    for (int scenario = 0; scenario < 13; ++scenario) {
        std::array<ReferenceActor, 2> actors;
        ReferenceProbe probe{{&actors[0], &actors[1]}};
        reference_probe = &probe;
        tls[0x1d2] = std::uint8_t(scenario < 6 || scenario == 11);
        tls[0x1d4] = std::uint8_t(scenario == 9); // second direct-reference mode
        const bool direct = tls[0x1d2] || tls[0x1d4];
        for (auto& actor : actors) {
            actor.methods[1] = reinterpret_cast<std::uintptr_t>(arena.function<void*>(0x1436690));
            actor.methods[2] = reinterpret_cast<std::uintptr_t>(&fixture_reference_destroy);
            actor.methods[0x58 / 8] = reinterpret_cast<std::uintptr_t>(&fixture_reference_cleanup);
            actor.methods[0x90 / 8] = reinterpret_cast<std::uintptr_t>(&fixture_reference_server);
            actor.methods[0xf0 / 8] = reinterpret_cast<std::uintptr_t>(&fixture_reference_parent);
            actor.embedded_methods[1] = reinterpret_cast<std::uintptr_t>(&fixture_reference_enter);
            actor.embedded_methods[2] = reinterpret_cast<std::uintptr_t>(&fixture_reference_leave);
            put(actor.bytes, 0, actor.methods.data()); put(actor.bytes, 0x18, actor.embedded_methods.data());
            put(actor.bytes, 0x60, handle); put(actor.bytes, 0x5e, std::uint16_t(0x10));
            actor.bytes[0x4a] = 1;
        }
        auto& first = actors[0]; auto& second = actors[1];
        bool accepted = true;
        if (scenario == 1) { put(second.bytes, 0x60, handle | 0x100000u); accepted = false; }
        if (scenario == 2) { put(first.bytes, 0x5e, std::uint16_t(0)); accepted = false; }
        if (scenario == 3) { // flags lost, but an existing direct reference permits nesting
            put(first.bytes, 0x5e, std::uint16_t(0)); put(first.bytes, 0x10, std::uint32_t(1));
        }
        if (scenario == 4 || scenario == 7) {
            first.bytes[0x4a] = 0;
            accepted = direct; // actual fast path DOES allow a dead actor
        }
        if (scenario == 8) { // tracked nesting uses the thread-map lookup path
            put(first.bytes, 0x5e, std::uint16_t(0)); put(first.bytes, 0x48, std::uint16_t(1)); first.tracked = 1;
        }
        const std::array sources{
            reference::Source{first.bytes.data(), handle, acquire_reference_native, release_reference_native},
            reference::Source{second.bytes.data(), handle, acquire_reference_native, release_reference_native}};
        {
            reference::Pair refs;
            require(refs.acquire(sources) == (accepted ? reference::Code::acquired : reference::Code::unavailable),
                "Original handle/state/reference-mode acquisition semantics");
            if (accepted) {
                require(refs.actor(0) == reinterpret_cast<std::uintptr_t>(first.bytes.data()) &&
                    refs.actor(1) == reinterpret_cast<std::uintptr_t>(second.bytes.data()), "Native receipts expose matching actors");
                require(get<std::uint32_t>(first.bytes, 0x10) == (direct ? (scenario == 3 ? 2u : 1u) : 0u),
                    "Native direct refcount increment");
                require(get<std::uint16_t>(first.bytes, 0x48) == (direct ? 0 : (scenario == 8 ? 2 : 1)),
                    "Tracked reference registration branch");
                if (scenario == 10 || scenario == 11 || scenario == 12) put(first.bytes, 0x5e, std::uint16_t(0x20));
                if (scenario == 5 || scenario == 6 || scenario == 9 || scenario == 11) {
                    first.bytes[0x4a] = 0; second.bytes[0x4a] = 0;
                    require(!first.destroyed && !second.destroyed, "Held references defer destruction");
                }
                if (scenario == 12) {
                    // Client override returns false; state transition remains,
                    // but the server-specific cleanup callback is skipped.
                    first.methods[0x90 / 8] = reinterpret_cast<std::uintptr_t>(&fixture_reference_reacquire);
                }
            } else require(!refs.actor(0) && !refs.actor(1), "Failure exposes neither partial actor");
        }
        require(get<std::uint32_t>(first.bytes, 0x10) == (scenario == 3 ? 1u : 0u) &&
            !get<std::uint32_t>(second.bytes, 0x10), "Native receipt cleanup balances direct refs");
        require(first.tracked == unsigned(scenario == 8) && !second.tracked &&
            get<std::uint16_t>(first.bytes, 0x48) == (scenario == 8 ? 1 : 0), "Tracked cleanup preserves outer ownership");
        const unsigned first_destroy = unsigned(scenario == 4 || scenario == 5 || scenario == 6 || scenario == 9 || scenario == 11);
        const unsigned second_destroy = unsigned(scenario == 5 || scenario == 6 || scenario == 9 || scenario == 11);
        require(first.destroyed == first_destroy && second.destroyed == second_destroy,
            "Original last-reference destructor dispatch occurs exactly once");
        require(first.cleanup == unsigned(scenario == 10 || scenario == 11) && !second.cleanup &&
            (scenario < 10 || get<std::uint16_t>(first.bytes, 0x5e) == 0x40),
            "Derived actor release performs deferred 0x20 to 0x40 transition and server cleanup after unlocking");
        require(probe.valid && !first.lock_depth && !second.lock_depth && first.enters == first.leaves && second.enters == second.leaves,
            "Balanced embedded locks and tracking callbacks");
        // Invalid receipt can contain a bogus pointer. Native cleanup must skip
        // it based on validity, and may safely be called again after release.
        reference::Receipt invalid;
        put(invalid.bytes, 8, std::uintptr_t(1));
        release_reference_native(nullptr, invalid);
        require(!invalid.actor(), "Invalid non-null receipt is never dereferenced");
        if (scenario == 0) {
            reference::Receipt special;
            put(special.bytes, 8, first.bytes.data()); special.bytes[0x10] = 1; special.bytes[0x11] = 1;
            put(first.bytes, 0x48, std::uint16_t(1)); first.bytes[0x4a] = 0;
            release_reference_native(nullptr, special); release_reference_native(nullptr, special);
            require(first.destroyed == 1 && !special.actor() && !get<std::uint16_t>(first.bytes, 0x48),
                "Special native receipt releases its tracked ref once and invalidates itself");
            require(probe.valid && !first.lock_depth && first.enters == first.leaves, "Special receipt unlocks before destruction");
        }
    }
    reference_probe = nullptr; native_acquire_actor = nullptr; native_release_receipt = nullptr;
}
