// Native buffer preparation and LZ4 compression on private data. The opaque
// postprocessor remains a callback: no encryption/authentication or save claim.
unsigned save_encode_cases = 0;
using SaveEncode = bool (*)(void*, const char*, void*);

// Independent bounded test oracle, implemented from the LZ4 block description:
// https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md
// This is deliberately a bytewise decoder, not a copy of the native compressor.
bool decode_fixture_lz4(std::span<const std::uint8_t> encoded, const std::vector<std::uint8_t>& expected) {
    std::vector<std::uint8_t> decoded; decoded.reserve(expected.size());
    std::size_t at = 0;
    auto length = [&](std::size_t& n) {
        if (n != 15) return true;
        unsigned extra;
        do {
            if (at == encoded.size()) return false;
            extra = encoded[at++];
            if (n > expected.size() || extra > expected.size() - n) return false;
            n += extra;
        } while (extra == 255);
        return true;
    };
    while (at < encoded.size()) {
        const auto token = encoded[at++]; std::size_t literals = token >> 4;
        if (!length(literals) || literals > encoded.size() - at || literals > expected.size() - decoded.size()) return false;
        decoded.insert(decoded.end(), encoded.begin() + at, encoded.begin() + at + literals); at += literals;
        if (at == encoded.size()) return decoded == expected;
        if (encoded.size() - at < 2) return false;
        const std::size_t offset = encoded[at] | (std::size_t(encoded[at + 1]) << 8); at += 2;
        std::size_t match = token & 15;
        if (!offset || offset > decoded.size() || !length(match)) return false;
        match += 4;
        if (match > expected.size() - decoded.size()) return false;
        for (std::size_t i = 0; i < match; ++i) decoded.push_back(decoded[decoded.size() - offset]);
    }
    return false;
}
struct SaveEncodeProbe;
thread_local SaveEncodeProbe* save_encode_probe = nullptr;
struct SaveEncodeProbe {
    Arena& arena;
    ItemHeap& heap;
    unsigned mode;
    enum Failure : unsigned { none, compression, postprocess, opening, writing, flushing };
    Failure failure = none;
    alignas(8) std::array<std::uint8_t, 0x160> entry{};
    alignas(8) std::array<std::uint8_t, 0x18> output{};
    alignas(8) std::array<std::uint8_t, 0x30> handler{};
    alignas(8) std::array<std::uint8_t, 0x1a0> owner{};
    std::vector<std::uint8_t> source, intermediate;
    unsigned post_calls = 0, compression_fail_calls = 0, logs = 0, post_logs = 0;
    std::array<unsigned, 5> io_order{};
    unsigned io_calls = 0;
    bool compressed;
    bool valid = true;
    void* file = nullptr;
    static constexpr char key[] = "fixture-encoding-key";
    static constexpr char directory[] = "fixture-directory";
    static constexpr char path[] = "fixture-output";
    static constexpr std::uintptr_t handle = 0x34567812;
    SaveEncodeProbe(Arena& a, ItemHeap& h, unsigned m, bool compress, std::uint32_t size, bool random, std::uint32_t output_capacity = 0)
        : arena(a), heap(h), mode(m), source(size), compressed(compress) {
        std::uint32_t state = 0x12345678;
        for (std::uint32_t i = 0; i < size; ++i) {
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            source[i] = random ? static_cast<std::uint8_t>(state) : static_cast<std::uint8_t>(i % 13);
        }
        entry.fill(0); std::fill(entry.begin() + 0x138, entry.end(), std::uint8_t(0xc7));
        constexpr char name[] = "fixture-entry"; std::copy_n(name, sizeof(name), entry.begin());
        for (unsigned i = 0; i < 128; ++i) entry[0x98 + i] = static_cast<std::uint8_t>(i * 11 + 5);
        entry[0xa4] = compressed ? 2 : 0;
        auto* bytes = static_cast<std::uint8_t*>(heap.allocate(size + 2, 16, mode));
        require(bytes != nullptr, "Private encoder input allocated with bounded trailing space");
        std::copy(source.begin(), source.end(), bytes);
        put(entry, 0x120, bytes); put(entry, 0x128, size + 2); put(entry, 0x12c, size); entry[0x130] = 0xff;
        output[0x10] = 0xff;
        if (output_capacity) {
            auto* previous = heap.allocate(output_capacity, 16, mode);
            require(previous != nullptr && output_capacity >= 4, "Private reusable encoder output allocated");
            put(output, 0, previous); put(output, 8, output_capacity); put(output, 12, std::uint32_t(2));
        }
        put(handler, 0x10, owner.data()); put(owner, 0x180, key);
    }
    bool encodes() const { return failure != compression && failure != postprocess; }
    bool completes() const { return encodes() && failure != opening && failure != writing; }
    bool body_valid() const {
        const auto size = get<std::uint32_t>(entry, 0x12c);
        const auto* bytes = get<const std::uint8_t*>(entry, 0x120);
        if (!heap.covers(bytes, size)) return false;
        return compressed ? decode_fixture_lz4({bytes, size}, source) :
            size == source.size() && std::equal(source.begin(), source.end(), bytes);
    }
    static int post(void* record, const char* secret, void* buffer) noexcept {
        auto& p = *save_encode_probe; ++p.post_calls;
        p.valid &= record == p.entry.data() && secret == key && buffer == p.entry.data() + 0x120 && p.body_valid();
        const auto* bytes = get<const std::uint8_t*>(p.entry, 0x120);
        const auto n = get<std::uint32_t>(p.entry, 0x12c);
        if (p.heap.covers(bytes, n)) p.intermediate.assign(bytes, bytes + n);
        return p.failure == postprocess ? 77 : 0;
    }
    static int fail_compression(const void* source, void* destination, std::uint32_t size, std::uint32_t capacity) noexcept {
        auto& p = *save_encode_probe; ++p.compression_fail_calls;
        p.valid &= p.failure == compression && size == p.source.size() &&
            source == get<void*>(p.entry, 0x120) && p.heap.covers(destination, capacity) && capacity >= size;
        return 0;
    }
    static void log(std::uint8_t enabled) noexcept {
        auto& p = *save_encode_probe; ++p.logs; p.valid &= enabled == 1 && p.failure != none && p.failure != flushing;
    }
    static void post_log(std::uint8_t enabled) noexcept {
        auto& p = *save_encode_probe; ++p.post_logs; p.valid &= enabled == 1 && p.failure == postprocess;
    }
    void bind(Pe& pe) {
        save_encode_probe = this;
        arena.restore(pe, 0x235c750); arena.restore(pe, 0x12c61e0);
        arena.redirect_thunk(0x4898dce, 0x6cffd20, reinterpret_cast<std::uintptr_t>(&std::memcpy));
        arena.redirect_thunk(0x4898dd4, 0x6cffd28, reinterpret_cast<std::uintptr_t>(&std::memset));
        arena.redirect(0x235c040, reinterpret_cast<std::uintptr_t>(&post));
        arena.redirect(0x35d8d0, reinterpret_cast<std::uintptr_t>(&log));
        arena.redirect(0x35dd30, reinterpret_cast<std::uintptr_t>(&post_log));
        if (failure == compression) arena.redirect(0x12c61e0, reinterpret_cast<std::uintptr_t>(&fail_compression));
    }
    bool encoded_valid(const void* bytes, std::uint32_t n) const {
        if (n != 128 + intermediate.size() || !heap.covers(bytes, n + 2)) return false;
        const auto* b = static_cast<const std::uint8_t*>(bytes);
        return std::equal(entry.begin() + 0x98, entry.begin() + 0x118, b) &&
            std::equal(intermediate.begin(), intermediate.end(), b + 128) && !b[n] && !b[n + 1];
    }
    void verify_entry(const std::array<std::uint8_t, 0x160>& before, bool consumed) {
        auto expected = before;
        require(valid && heap.valid && heap.guards() && post_calls == unsigned(failure != compression) &&
            compression_fail_calls == unsigned(failure == compression), "Native compression/encoding dependency arguments and heap guards remain valid");
        if (failure != compression && compressed) {
            // Pointer is chosen by the heap; sizes/content are checked independently.
            std::copy_n(entry.begin() + 0x120, 16, expected.begin() + 0x120);
            if (!consumed) require(get<std::uint32_t>(entry, 0x128) == intermediate.size() + 2 &&
                get<std::uint32_t>(entry, 0x12c) == intermediate.size() && body_valid(),
                "Native compression replaces pending data with a valid block even before a later failure");
        }
        if (encodes()) {
            put(expected, 0xaa, static_cast<std::uint32_t>(source.size()));
            put(expected, 0xae, static_cast<std::uint32_t>(intermediate.size()));
        }
        if (consumed) std::fill(expected.begin() + 0x120, expected.begin() + 0x130, std::uint8_t(0));
        require(entry == expected, "Native encoder changes only expected payload/length fields and preserves all other header, identity and guard bytes");
        if (!consumed) {
            const auto* raw = get<const std::uint8_t*>(entry, 0x120);
            require(failure == compression ? heap.covers(raw, source.size()) && std::equal(source.begin(), source.end(), raw) : body_valid(),
                "Compression failure preserves original raw input; later failures retain a block reconstructing that input");
        }
    }
    void release() {
        const auto clear = arena.function<void (*)(void*)>(0x1373a40);
        ++native_calls; clear(entry.data() + 0x120);
        ++native_calls; clear(output.data());
    }
    void io(unsigned step) noexcept {
        valid &= io_calls < io_order.size(); if (io_calls < io_order.size()) io_order[io_calls] = step; ++io_calls;
    }
    static void format(char* out, std::uint32_t capacity, const void*, const char* const* dir, const char* const* name) noexcept {
        auto& p = *save_encode_probe;
        p.valid &= capacity == 0x105 && *dir == directory && *name == reinterpret_cast<const char*>(p.entry.data());
        std::copy_n(path, sizeof(path), out);
    }
    static bool open(void* self, const char* filename) noexcept {
        auto& p = *save_encode_probe; p.io(1); p.file = self;
        p.valid &= p.encodes() && p.post_calls == 1 && p.body_valid() && std::strcmp(filename, path) == 0 &&
            item_read<std::uintptr_t>(self, 0) == UINTPTR_MAX;
        if (p.failure != opening) std::memcpy(self, &handle, sizeof(handle));
        return p.failure != opening;
    }
    static bool write(void* self, const void* data, std::uint32_t size) noexcept {
        auto& p = *save_encode_probe; p.io(2);
        p.valid &= self == p.file && p.encoded_valid(data, size) && p.body_valid();
        return p.failure != writing;
    }
    static BOOL WINAPI flush(HANDLE h) noexcept {
        auto& p = *save_encode_probe; p.io(3);
        p.valid &= reinterpret_cast<std::uintptr_t>(h) == handle && p.body_valid();
        return p.failure != flushing;
    }
    static BOOL WINAPI close(HANDLE h) noexcept {
        auto& p = *save_encode_probe; p.io(4);
        p.valid &= reinterpret_cast<std::uintptr_t>(h) == handle;
        p.valid &= p.completes() ? get<std::uintptr_t>(p.entry, 0x120) == 0 : p.body_valid();
        return TRUE;
    }
    void bind_file() {
        arena.redirect(0x235ca70, reinterpret_cast<std::uintptr_t>(&format));
        arena.redirect(0x12b7a90, reinterpret_cast<std::uintptr_t>(&open));
        arena.redirect(0x12b7dd0, reinterpret_cast<std::uintptr_t>(&write));
        arena.pointer(0x51eb420, reinterpret_cast<std::uintptr_t>(&flush));
        arena.pointer(0x51eb480, reinterpret_cast<std::uintptr_t>(&close));
    }
};
DWORD invoke_save_encode(SaveEncode fn, void* entry, const char* key, void* output, bool* result) {
    __try { *result = fn(entry, key, output); return 0; }
    __except (save_fault(GetExceptionInformation())) { return GetExceptionCode(); }
}
void save_encode_native_tests(Pe& pe, Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    install_item_allocator(arena); ItemHeap heap(262144);
    const auto encode = arena.function<SaveEncode>(0x235c750);
    auto direct = [&](unsigned mode, bool compressed, std::uint32_t size, bool random, std::uint32_t capacity, SaveEncodeProbe::Failure failure) {
        ++save_encode_cases; tls[0x1fd] = static_cast<std::uint8_t>(mode);
        const auto allocs = heap.alloc_calls, frees = heap.free_calls;
        SaveEncodeProbe p(arena, heap, mode, compressed, size, random, capacity); p.failure = failure; p.bind(pe);
        const auto before = p.entry;
        const auto output_before = p.output;
        bool result = !p.encodes(); ++native_calls;
        const auto code = invoke_save_encode(encode, p.entry.data(), SaveEncodeProbe::key, p.output.data(), &result);
        if (code || result != p.encodes()) std::cerr << "encoder mode=" << mode << " compressed=" << compressed << " size=" << size
            << " failure=" << failure << " exception=" << std::hex << code << " rva="
            << (save_fault_ip - reinterpret_cast<std::uintptr_t>(arena.function<void*>(0))) << std::dec << '\n';
        require(!code && result == p.encodes(),
            "Original save-buffer encoder and native compression complete with controlled postprocessing status");
        p.verify_entry(before, false);
        if (p.encodes()) {
            const auto* out = get<const std::uint8_t*>(p.output, 0); const auto n = get<std::uint32_t>(p.output, 12);
            require(p.encoded_valid(out, n) && get<std::uint32_t>(p.output, 8) >= n + 2 && p.output[16] == 0xff,
                "Native output contains the exact128-byte header, original/encoded lengths, reconstructable body and two zero terminators");
            if (capacity >= n + 2) require(get<void*>(output_before, 0) == out, "Existing output allocation is reused when sufficiently large");
        } else require(p.output == output_before, "Early encoding failure leaves the output descriptor unchanged");
        require(p.logs == unsigned(failure == SaveEncodeProbe::compression) && p.post_logs == unsigned(failure == SaveEncodeProbe::postprocess),
            "Compression and postprocessing failures produce their own single diagnostic stage");
        p.release();
        require(!heap.live() && heap.valid && heap.guards() && heap.alloc_calls[1 - mode] == allocs[1 - mode] && heap.free_calls[1 - mode] == frees[1 - mode] &&
            heap.alloc_calls[mode] - allocs[mode] == heap.free_calls[mode] - frees[mode], "Native encoder ownership balances all temporary, retained and reused buffers in both TLS modes");
    };
    for (unsigned mode = 0; mode < 2; ++mode) {
        unsigned index = 0;
        for (const std::uint32_t n : {0u,1u,12u,13u,255u,256u,4096u,65546u,65547u,131072u}) {
            for (const bool compressed : {false,true}) direct(mode,compressed,n,++index % 3 == 0,0,SaveEncodeProbe::none);
        }
        direct(mode,false,32,false,512,SaveEncodeProbe::none);
        direct(mode,true,4096,false,8,SaveEncodeProbe::none);
        direct(mode,true,4096,false,0,SaveEncodeProbe::compression);
        direct(mode,false,4096,false,0,SaveEncodeProbe::postprocess);
        direct(mode,true,4096,false,0,SaveEncodeProbe::postprocess);
        for (unsigned scenario = 0; scenario < 7; ++scenario) {
            ++save_encode_cases;
            SaveEncodeProbe p(arena,heap,mode,scenario != 0,4096,false);
            const std::array failures{SaveEncodeProbe::none,SaveEncodeProbe::none,SaveEncodeProbe::opening,
                SaveEncodeProbe::writing,SaveEncodeProbe::flushing,SaveEncodeProbe::compression,SaveEncodeProbe::postprocess};
            p.failure = failures[scenario]; p.bind(pe); p.bind_file();
            const auto before = p.entry; bool result = !p.completes(); ++native_calls;
            require(!invoke_save_file(arena.function<SaveFile>(0x2358700),p.handler.data(),SaveEncodeProbe::directory,p.entry.data(),&result) && result == p.completes(),
                "Original file writer uses the original native encoder and LZ4 compressor before private file callbacks");
            p.verify_entry(before,p.completes());
            std::vector<unsigned> expected;
            if (p.encodes()) {
                expected.push_back(1);
                if (p.failure != SaveEncodeProbe::opening) {
                    expected.push_back(2);
                    if (p.failure != SaveEncodeProbe::writing) expected.push_back(3);
                    expected.push_back(4);
                }
            }
            require(p.io_calls == expected.size() && std::equal(expected.begin(),expected.end(),p.io_order.begin()) &&
                p.logs == (p.failure == SaveEncodeProbe::compression ? 2u : unsigned(!p.completes())) &&
                p.post_logs == unsigned(p.failure == SaveEncodeProbe::postprocess), "File stages stop at the actual failure and never treat a pending transformed payload as an unchanged source");
            require(heap.live() == unsigned(!p.completes()), "File writer frees encoded output on every path and only retains pending data after failures");
            if (p.failure == SaveEncodeProbe::opening) {
                // Deliberately repeat ONLY a private failed fixture to establish
                // why a live retry must regenerate its raw serialized input.
                ++save_encode_cases;
                const auto raw_source = p.source;
                const auto first_block = p.intermediate;
                require(first_block.size() < raw_source.size() && decode_fixture_lz4(first_block, raw_source),
                    "First failed file open retains an already compressed native block");
                const auto retry_before = p.entry;
                p.source = first_block; p.intermediate.clear(); p.failure = SaveEncodeProbe::none;
                p.post_calls = 0; p.io_calls = 0; p.io_order.fill(0); p.logs = 0; p.bind(pe);
                result = false; ++native_calls;
                require(!invoke_save_file(arena.function<SaveFile>(0x2358700),p.handler.data(),SaveEncodeProbe::directory,p.entry.data(),&result) && result,
                    "Original writer can be repeated on its private failed-open entry");
                p.verify_entry(retry_before,true);
                require(get<std::uint32_t>(p.entry,0xaa) == first_block.size() && get<std::uint32_t>(p.entry,0xaa) != raw_source.size() &&
                    decode_fixture_lz4(p.intermediate,first_block) && p.io_calls == 4 && !heap.live() && !p.logs,
                    "Repeating the same marked entry compresses its previous compressed bytes again and replaces the original-size header");
            }
            p.release(); require(!heap.live() && heap.valid && heap.guards(), "Integrated native preparation/write teardown balances private ownership");
        }
    }
    save_encode_probe = nullptr;
}
