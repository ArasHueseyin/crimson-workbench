// Original per-entry file writer and buffer cleanup, private objects only.
// Path formatting, encoding and ALL file/handle operations are test callbacks.
// No OS file handle is created; the only handle value is a synthetic integer.
unsigned save_file_cases = 0;
using SaveFile = bool (*)(void*, const char*, void*);
struct SaveFileProbe;
thread_local SaveFileProbe* save_file_probe = nullptr;
struct SaveFileProbe {
    Arena& arena;
    ItemHeap& heap;
    unsigned mode, scenario;
    alignas(8) std::array<std::uint8_t, 0x30> handler{};
    alignas(8) std::array<std::uint8_t, 0x1a0> owner{};
    alignas(8) std::array<std::uint8_t, 0x158> entry{};
    std::array<std::uint8_t, 32> payload{};
    std::array<unsigned, 8> order{};
    unsigned calls = 0;
    bool valid = true;
    void* file = nullptr;
    std::uint8_t* encoded = nullptr;
    std::uint8_t* original = nullptr;
    std::uint32_t encoded_size = 37;
    static constexpr char directory[] = "private-fixture-directory";
    static constexpr char path[] = "private-fixture-output";
    static constexpr char external_name[] = "private-external-entry";
    static constexpr char external_key[] = "private-external-key";
    static constexpr std::uintptr_t fake_handle = 0x12345678;
    enum Step : unsigned { format_step = 1, prepare_step, open_step, write_step, flush_step, close_step, log_step };
    SaveFileProbe(Arena& a, ItemHeap& h, unsigned m, unsigned s) : arena(a), heap(h), mode(m), scenario(s) {
        entry.fill(0xc7); std::fill(entry.begin(), entry.begin() + 0x138, std::uint8_t(0));
        constexpr char name[] = "private-inline-entry", key[] = "private-inline-key";
        std::copy_n(name, sizeof(name), entry.begin());
        std::copy_n(key, sizeof(key), owner.begin() + 0x78);
        if (scenario == 12) { put(entry, 0x88, external_name); put(owner, 0x180, external_key); }
        put(handler, 0x10, owner.data());
        for (unsigned i = 0; i < payload.size(); ++i) payload[i] = static_cast<std::uint8_t>(i * 7 + 3);
        original = static_cast<std::uint8_t*>(heap.allocate(static_cast<std::uint32_t>(payload.size()), 8, mode));
        require(original != nullptr, "Private pending save payload allocated");
        std::copy(payload.begin(), payload.end(), original);
        put(entry, 0x120, original); put(entry, 0x128, std::uint32_t(32)); put(entry, 0x12c, std::uint32_t(32));
        entry[0x130] = 0xff; // Deliberately bypass the game's allocator-accounting globals.
        if (scenario == 8 || scenario == 9) encoded_size = 0;
        if (scenario == 11) encoded_size = 4096;
    }
    bool prepare_ok() const { return scenario != 1 && scenario != 2; }
    bool open_ok() const { return scenario != 3 && scenario != 10; }
    bool write_ok() const { return scenario != 4; }
    bool succeeds() const { return prepare_ok() && open_ok() && write_ok(); }
    void step(unsigned value) noexcept {
        valid &= calls < order.size();
        if (calls < order.size()) order[calls] = value;
        ++calls;
    }
    bool original_intact() const noexcept {
        return heap.owns(original) && std::equal(payload.begin(), payload.end(), original) &&
            get<void*>(entry, 0x120) == original && get<std::uint32_t>(entry, 0x12c) == 32;
    }
    bool original_cleared() const noexcept {
        return !heap.owns(original) && !get<std::uintptr_t>(entry, 0x120) && !get<std::uint64_t>(entry, 0x128) && entry[0x130] == 0xff;
    }
    static void format(char* out, std::uint32_t capacity, const void*, const char* const* dir, const char* const* name) noexcept {
        auto& p = *save_file_probe; p.step(format_step);
        p.valid &= out && capacity == 0x105 && dir && *dir == directory && name &&
            *name == (p.scenario == 12 ? external_name : reinterpret_cast<const char*>(p.entry.data()));
        if (out && capacity >= sizeof(path)) std::copy_n(path, sizeof(path), out);
    }
    static bool prepare(void* entry, const char* key, void* out) noexcept {
        auto& p = *save_file_probe; p.step(prepare_step);
        p.valid &= entry == p.entry.data() && out && p.original_intact() &&
            key == (p.scenario == 12 ? external_key : reinterpret_cast<const char*>(p.owner.data() + 0x78));
        p.valid &= !item_read<std::uintptr_t>(out, 0) && !item_read<std::uint64_t>(out, 8) && item_read<std::uint8_t>(out, 16) == 0xff;
        if (p.scenario != 1 && p.scenario != 8) {
            const std::uint32_t capacity = p.encoded_size ? p.encoded_size + 11 : 16;
            p.encoded = static_cast<std::uint8_t*>(p.heap.allocate(capacity, 8, p.mode));
            if (!p.encoded) { p.valid = false; return false; }
            for (std::uint32_t i = 0; i < capacity; ++i) p.encoded[i] = static_cast<std::uint8_t>(i * 3 + 11);
            std::memcpy(out, &p.encoded, sizeof(p.encoded));
            std::memcpy(static_cast<std::uint8_t*>(out) + 8, &capacity, sizeof(capacity));
            std::memcpy(static_cast<std::uint8_t*>(out) + 12, &p.encoded_size, sizeof(p.encoded_size));
        }
        return p.prepare_ok();
    }
    static bool open(void* self, const char* name) noexcept {
        auto& p = *save_file_probe; p.step(open_step); p.file = self;
        p.valid &= self && std::strcmp(name, path) == 0 && p.original_intact() &&
            item_read<std::uintptr_t>(self, 0) == UINTPTR_MAX && !item_read<std::uint32_t>(self, 8) &&
            item_read<void*>(self, 16) == p.arena.function<void*>(0x6a65940) && !item_read<std::uint8_t>(self, 24);
        if (p.open_ok() || p.scenario == 10) std::memcpy(self, &fake_handle, sizeof(fake_handle));
        return p.open_ok();
    }
    static bool write(void* self, const void* bytes, std::uint32_t count) noexcept {
        auto& p = *save_file_probe; p.step(write_step);
        p.valid &= self == p.file && bytes == p.encoded && count == p.encoded_size && count > 0 &&
            p.heap.covers(bytes, count) && p.original_intact() && item_read<std::uintptr_t>(self, 0) == fake_handle;
        if (p.heap.covers(bytes, count)) for (std::uint32_t i = 0; i < count; ++i)
            p.valid &= static_cast<const std::uint8_t*>(bytes)[i] == static_cast<std::uint8_t>(i * 3 + 11);
        return p.write_ok();
    }
    static BOOL WINAPI flush(HANDLE handle) noexcept {
        auto& p = *save_file_probe; p.step(flush_step);
        p.valid &= reinterpret_cast<std::uintptr_t>(handle) == fake_handle && p.original_intact();
        return p.scenario != 5 && p.scenario != 7;
    }
    static BOOL WINAPI close(HANDLE handle) noexcept {
        auto& p = *save_file_probe; p.step(close_step);
        p.valid &= reinterpret_cast<std::uintptr_t>(handle) == fake_handle &&
            (p.succeeds() ? p.original_cleared() : p.original_intact());
        return p.scenario != 6 && p.scenario != 7;
    }
    static void log(std::uint8_t enabled) noexcept {
        auto& p = *save_file_probe; p.step(log_step); p.valid &= enabled == 1 && !p.succeeds() && p.original_intact();
    }
    void bind() {
        save_file_probe = this;
        arena.redirect(0x235ca70, reinterpret_cast<std::uintptr_t>(&format));
        arena.redirect(0x235c750, reinterpret_cast<std::uintptr_t>(&prepare));
        arena.redirect(0x12b7a90, reinterpret_cast<std::uintptr_t>(&open));
        arena.redirect(0x12b7dd0, reinterpret_cast<std::uintptr_t>(&write));
        arena.redirect(0x35d8d0, reinterpret_cast<std::uintptr_t>(&log));
        // Replace import slots in the PRIVATE arena, never the game or its DLLs.
        arena.pointer(0x51eb420, reinterpret_cast<std::uintptr_t>(&flush));
        arena.pointer(0x51eb480, reinterpret_cast<std::uintptr_t>(&close));
    }
    void verify(const std::array<std::uint8_t, 0x158>& before) {
        std::vector<unsigned> expected{format_step, prepare_step};
        if (!prepare_ok()) expected.push_back(log_step);
        else {
            expected.push_back(open_step);
            if (!open_ok()) expected.push_back(log_step);
            else {
                if (encoded_size) expected.push_back(write_step);
                expected.push_back(write_ok() ? flush_step : log_step);
            }
            if (open_ok() || scenario == 10) expected.push_back(close_step);
        }
        require(valid && calls == expected.size() && std::equal(expected.begin(), expected.end(), order.begin()),
            "Original file writer preserves callback arguments, exact stage order and stops after preparation/open/write errors");
        auto after = before;
        if (succeeds()) std::fill(after.begin() + 0x120, after.begin() + 0x130, std::uint8_t(0));
        require(entry == after && (succeeds() ? original_cleared() : original_intact()),
            "Native payload cleanup follows successful write path even after failed flush/close; pre-write failures preserve pending bytes");
        require((!encoded || !heap.owns(encoded)) && heap.valid && heap.guards(),
            "Original file writer releases every temporary encoded buffer, including partial preparation and empty-output branches");
    }
    void release_pending() {
        if (heap.owns(original)) {
            ++native_calls; arena.function<void (*)(void*)>(0x1373a40)(entry.data() + 0x120);
        }
    }
    static bool dispatch_save(void* self, const void* record) noexcept {
        auto& d = *save_dispatch_probe;
        if (!d.step(3, self, record)) return false;
        auto& p = *save_file_probe;
        // This bridge deliberately models ONE entry. It is not the original
        // queue traversal, backup/rename logic, encoder or scheduling policy.
        ++native_calls;
        const bool result = p.arena.function<SaveFile>(0x2358700)(p.handler.data(), directory, p.entry.data());
        if (!result) d.fail_stage = 3;
        return result;
    }
};
DWORD invoke_save_file(SaveFile fn, void* handler, const char* directory, void* entry, bool* result) {
    __try { *result = fn(handler, directory, entry); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void save_file_native_tests(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    install_item_allocator(arena); ItemHeap heap;
    const auto write = arena.function<SaveFile>(0x2358700);
    for (unsigned mode = 0; mode < 2; ++mode) for (unsigned scenario = 0; scenario < 13; ++scenario) {
        ++save_file_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        const auto allocs = heap.alloc_calls, frees = heap.free_calls;
        SaveFileProbe p(arena, heap, mode, scenario); p.bind();
        const auto entry_before = p.entry; const auto owner_before = p.owner; const auto handler_before = p.handler;
        bool result = !p.succeeds(); ++native_calls;
        require(!invoke_save_file(write, p.handler.data(), SaveFileProbe::directory, p.entry.data(), &result) && result == p.succeeds(),
            "Original per-entry writer handles private preparation/open/write/flush/close outcomes without any file access");
        p.verify(entry_before);
        require(p.owner == owner_before && p.handler == handler_before, "Native write helper leaves its owner and handler unchanged");
        p.release_pending();
        require(!heap.live() && heap.valid && heap.alloc_calls[1 - mode] == allocs[1 - mode] && heap.free_calls[1 - mode] == frees[1 - mode] &&
            heap.alloc_calls[mode] - allocs[mode] == heap.free_calls[mode] - frees[mode],
            "Native success cleanup and explicit fixture teardown balance every private buffer in both TLS modes");
    }
    for (unsigned mode = 0; mode < 2; ++mode) for (const auto scenario : {0u, 3u, 4u, 5u}) {
        ++save_file_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        SaveFileProbe p(arena, heap, mode, scenario); p.bind();
        SaveDispatchProbe d(arena); d.bind(); d.prior_saved = 1;
        d.record[0x698] = 1; d.record[0x69b] = 1;
        d.methods[5] = reinterpret_cast<std::uintptr_t>(&SaveFileProbe::dispatch_save);
        const auto entry_before = p.entry;
        auto record_after = d.record;
        record_after[0x698] = p.succeeds() ? 0 : 1; record_after[0x69b] = p.succeeds() ? 1 : 0;
        bool result = !p.succeeds(); ++native_calls;
        require(!invoke_save_dispatch(arena.function<SaveDispatch>(0x23556b0), d.context.data(), d.record.data(), &result) && result == p.succeeds(),
            "Original dispatcher receives the original per-entry writer result through a private single-entry bridge");
        p.verify(entry_before);
        require(d.record == record_after && d.calls == (p.succeeds() ? 4u : 3u) && d.logs == unsigned(!p.succeeds()) && d.reports == unsigned(!p.succeeds()),
            "Failed flush still clears native dispatcher dirty state; open/write errors retain dirty state and clear saved status");
        require(d.valid, "Integrated dispatcher argument and status checks remain valid");
        p.release_pending();
        require(!heap.live() && heap.valid && heap.guards(), "Combined native writer/dispatcher releases all private allocations");
    }
    save_file_probe = nullptr; save_dispatch_probe = nullptr;
}
