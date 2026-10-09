// Isolated protected-registry/reference integration for EXE 1.0.0.2949.
// No process discovery, injection, loaded game module, save access or game writes.
#include "repair_registry.h"
#include "repair_reader.h"
#include "repair_writer.h"
#include "repair_transaction.h"
#include "repair_item.h"
#include "hash.h"
#include "pe.h"
#include <Windows.h>
#include <intrin.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

using namespace crimson::repair;
namespace {
unsigned checks = 0;
std::atomic<unsigned> native_calls{0};
void require(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
template<class T, std::size_t N> void put(std::array<std::uint8_t, N>& b, std::size_t at, T value) {
    if (at > N || sizeof(T) > N - at) throw std::runtime_error("Fixture write bound");
    std::memcpy(b.data() + at, &value, sizeof(value));
}
template<class T, std::size_t N> T get(const std::array<std::uint8_t, N>& b, std::size_t at) {
    if (at > N || sizeof(T) > N - at) throw std::runtime_error("Fixture read bound");
    T value;
    std::memcpy(&value, b.data() + at, sizeof(value));
    return value;
}
struct Function {
    std::uint32_t rva, size, tls;
    bool pdata;
    const char* sha;
    std::uint32_t tls2 = 0, tls3 = 0, tls4 = 0;
    auto tls_sites() const { return std::array{tls, tls2, tls3, tls4}; }
};
constexpr std::array functions{
    Function{0x8ae140, 0x1e5, 0, true, "854519c59e00f271c9a5eb4635a29065aeb7ab3e07a89fc18c58eff34d762253"},
    Function{0x2a82300, 0x21e, 0, true, "cda4603cc3a252f9d40f9fec9bd40882a0dd65eff422d612e457c8d3e9f29bcc"},
    Function{0x464290, 0x7d, 0, true, "071de40c7e03ac7ecc0373567ba72c146a45c51872c99d7cb7e448214c23eaae"},
    // Leaf actor-type helper plus its image-relative switch table.
    Function{0x466060, 0x58, 0, false, "5f31dba7c7e552dc6983e9288297f246d87d5fb9bfb90506e905b121ecc916a8"},
    Function{0x1434910, 0x9c, 0, true, "58962428c45832afd42ef53dbda8feeef6d8bebb53c709f1d79c683bbdc49860"},
    Function{0x14366a0, 0x2f5, 0x14366cd, true, "b3c91aeb0fd9e008cca5bbd8b8009d4ce10f7bf660d9cc3ef0155d31a090b9fa"},
    Function{0x14369a0, 0x98, 0x14369af, true, "38fab2877932d4e9bcab58e3d369d87f30faf0e2570709032e7c6a74d3dbcea0"},
    Function{0x1436b20, 0xd3, 0x1436b3a, true, "48beff72740de5f14839ccb4e3a0f2ce604c186a1c3d8e22163abc1b08bf7efa"},
    Function{0x1436c00, 0xf6, 0x1436c1b, true, "30eec32cccb2b3447885935521b2c92e9c951f9a7d6fb729b09f81833beca0d9"},
    Function{0x4f8ff0, 9, 0, false, "fe8bc8100bf885e0749bba30b7984f0327ae300d411bcc1d911dde3ece420529"},
    Function{0x4f9000, 12, 0, false, "1ce7b0dcd29cc09b04e9556f20fd10bb8f902700b65439d8f112ee8d7303da3b"},
    Function{0x8a6750, 5, 0, false, "d662675431d2c61d75765fffc65793f86b14feb5a92978260f919e30233c0f32"},
    Function{0x8a6760, 8, 0, false, "294e3de403e519830385dc27f21b9b061dee2c608360adf1de6c2c80e953002d"},
    Function{0x1371b60, 0x9b, 0x1371b90, true, "2618c096d4632c94f1100132b3b0bbf476cc14bd64272121b15ff3b1bd09dede"},
    Function{0x1371c00, 0x28, 0, false, "1400013b2e3ae5eb928c9fbfcdb5429f8e32af66e04973e410e97d2ba592266d"},
    Function{0x1371c30, 0xae, 0x1371c5f, true, "f928b8b10862b8348823b946709b6d75d7eed1f2a6665350a0cfa8d9cecbf606"},
    Function{0x212a100, 0x7f, 0, true, "136c1f162d8fae8b09b58acec04da5bfc779643af684ea7660e37895bd41a7b0"},
    // Complete function; four pdata fragments are registered separately below.
    Function{0x212f4b0, 0xe2, 0, false, "9d28f8810e6a2eeef87fb8738eb318ccb53fe8bcb94fdffc0ada86324a70e417"},
    Function{0x8b4480, 0x8c, 0, true, "a4b20462721ce4aff8800d2751b8848c81fdd2f189ebe4a402ccf5f7c4e50081"},
    Function{0x240d660, 0x195, 0, true, "16a75f0c5acbb658ee0c88710986679e28c0a81c68ec82d23dd5e72a025c805c"},
    Function{0x393650, 0x68, 0, true, "9361a1820340878d8bc677ec8cd21654fbab72c61102b5529939f23533e18e2f"},
    // Equipment wrapper's allocator TLS branches are not reached with delta 0.
    Function{0x20c90f0, 0x348, 0, true, "7f4a420f12671142e8b2b331ed4e6e2ebb076f7301c64f4779e7145984d32a29"},
    Function{0x240d4c0, 5, 0, false, "34744ce857e636a41d3ef60230f43644e9365ca29370510279b139edddd30490"},
    Function{0xf31ffd0, 0x129, 0, true, "af3a18e6f86b7cc217018938917c93051cfa10a9e6c61846754e8e6f40d0073a"},
    Function{0x98fb70, 0x305, 0x98fe2a, true, "435883cd7b55813ad7745ad6e0f8d8691e0dced431fae10c1d6cadeef73fbbc4"},
    Function{0x240e3f0, 0x1d1, 0, false, "c51935dad985c1cbb79725bf792ca1578f5838f68217ea5b58763b26456b04d1"},
    Function{0x2adfda0, 0x595, 0x2adfdf0, true, "a0ffa693e2ffbc985b051f8cb927af4c1a69b09ecfc4e4701e677057d299f360"},
    Function{0x20efb50, 0xb1, 0, false, "bc5b180c8736c84c99549fe94ab1a5d2f0f4763c3170e3f3cdf2a8b71dbc0508"},
#include "item_functions.inl"
#include "dirty_functions.inl"
#include "save_functions.inl"
#include "encode_functions.inl"
    Function{0x29ec2e0, 0x4c0, 0x29ec30a, true, "b227df1159b017f454a9362c085861f99697e526398de71a3f1fc738cf039d7f"},
    Function{0x2961c30, 0xab, 0x2961c4a, true, "98a78f3849be23c8fdf87be14801564a00b6f54f0438c40140a5130e5bd0842b"},
    Function{0x2358700, 0x2c9, 0x2358951, true, "4b72ca734a759f025c177b2792247f38cc1bd5241718e4551d3a3068f777504c", 0x2358988},
    Function{0x1373a40, 0x8e, 0x1373a72, true, "b24ed92cf23cde363dece41a355d5fc24b71c157ec73c734fa564b779d555178"},
    Function{0x394db0, 0x8e, 0x394df4, true, "fbf909548900f0892eb329d7d52aa7672335d1ab2d3472055e88ffc4f8c8847f"},
    Function{0x23556b0, 0x5, 0, false, "9a140b97891bf5b78202c69d125e45a8f5e5cc89e43b12c45ce43e9f7496c639"},
    Function{0xee09e40, 0x2e3, 0, true, "ae30d4532d725f078d02e2587f979cf6fbbf0bea1a26496f102b3e35900bbc80"},
    Function{0x235adb0, 0x5, 0, false, "0b84e2af6dc9b90e6e2a52e9d491703ebc7b601c7dedcbaabf413de4df0b884c"},
    Function{0xee1cd90, 0x55, 0, true, "d59745ff4a8b620e3d73f24df535409c4774cf34f519947353aaacf5c3dd68f0"},
    Function{0x235b5d0, 0x5, 0, false, "73e6c55db9e9ff0974ddaaddc8654cfd2d57b5c7f3ab2a76523583969cd243bb"},
    Function{0xee2a210, 0x13f, 0xee2a270, false, "df82349eabd8ebafcebca3863d902fa739bae773541df6b66136416b167e7f8a"},
    Function{0xa11ca0, 0x322, 0xa11eb9, true, "391c116f3606894b6a2a4de7623e51a56d8e05111099597c12a04b998486abe4", 0xa11ef1, 0xa11f7c},
    // Entire SQL execute shim, not merely the first pdata fragment.
    Function{0x2776d00, 0xc8, 0, false, "9cf1709ef75ad14106cdcd8e9f66fb55260612696421e28117e00f770b1ad5e3"},
    Function{0x2c1da50, 0x3f, 0, true, "6e744fe8978f3b0ae40d289c92e21a39779228870ac5ce901802e20ed8ba422d"},
};

class Arena {
    static constexpr std::size_t size = 0x17200000;
    std::uint8_t* base_ = nullptr;
    std::map<std::size_t, DWORD> pages_;
    std::vector<RUNTIME_FUNCTION> unwind_;
    bool registered_ = false;
public:
    Arena() {
        base_ = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS));
        if (!base_) throw std::runtime_error("Cannot reserve private arena");
    }
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    ~Arena() {
        if (registered_) RtlDeleteFunctionTable(unwind_.data());
        VirtualFree(base_, 0, MEM_RELEASE);
    }
    void write(std::uint32_t at, std::span<const std::uint8_t> bytes, DWORD protection) {
        if (bytes.empty() || at > size || bytes.size() > size - at) throw std::runtime_error("Arena bounds");
        const auto first = at & ~std::size_t(4095);
        const auto end = (at + bytes.size() + 4095) & ~std::size_t(4095);
        for (auto page = first; page < end; page += 4096) {
            if (!pages_.contains(page)) {
                if (!VirtualAlloc(base_ + page, 4096, MEM_COMMIT, PAGE_READWRITE)) throw std::runtime_error("Arena commit");
            } else {
                DWORD old;
                if (pages_.at(page) != protection || !VirtualProtect(base_ + page, 4096, PAGE_READWRITE, &old))
                    throw std::runtime_error("Arena reopen");
            }
            pages_[page] = protection;
        }
        std::memcpy(base_ + at, bytes.data(), bytes.size());
        for (auto page = first; page < end; page += 4096) {
            DWORD old;
            if (!VirtualProtect(base_ + page, 4096, protection, &old)) throw std::runtime_error("Arena seal");
        }
        if (protection == PAGE_EXECUTE_READ && !FlushInstructionCache(GetCurrentProcess(), base_ + at, bytes.size()))
            throw std::runtime_error("Arena instruction cache");
    }
    void pointer(std::uint32_t at, std::uintptr_t value) {
        write(at, {reinterpret_cast<const std::uint8_t*>(&value), sizeof(value)}, PAGE_READONLY);
    }
    void redirect(std::uint32_t at, std::uintptr_t callback) {
        std::array<std::uint8_t, 12> code{0x48, 0xb8};
        put(code, 2, callback); code[10] = 0xff; code[11] = 0xe0;
        write(at, code, PAGE_EXECUTE_READ);
    }
    void redirect_thunk(std::uint32_t at, std::uint32_t slot, std::uintptr_t callback) {
        // The CRT import thunks are adjacent six-byte instructions. A twelve-byte
        // absolute trampoline would overwrite the following thunk.
        std::array<std::uint8_t, 6> code{0xff, 0x25};
        put(code, 2, static_cast<std::int32_t>(slot - (at + 6)));
        pointer(slot, callback); write(at, code, PAGE_EXECUTE_READ);
    }
    template<class T> T function(std::uint32_t at) { return reinterpret_cast<T>(base_ + at); }
    void restore(Pe& pe, std::uint32_t rva) {
        const auto found = std::find_if(functions.begin(), functions.end(), [&](const Function& f) { return f.rva == rva; });
        require(found != functions.end(), "Restore only a manifest-pinned function");
        write_code(pe, *found);
    }
    void write_code(Pe& pe, const Function& fn) {
        auto code = pe.rva(fn.rva, fn.size);
        require(hash(code) == fn.sha, "Full function hash for build 2949");
        const auto original = code;
        for (const auto site : fn.tls_sites()) {
            if (!site) continue;
            const auto at = site - fn.rva;
            // The pinned save loader has one RCX load; every other site uses RAX.
            const bool rcx_load = site == 0x188c50a;
            std::array<std::uint8_t, 9> gs{0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
            if (rcx_load) gs[3] = 0x0c;
            require(std::equal(gs.begin(), gs.end(), code.begin() + at), "Exact fixture TLS-load site");
            std::array<std::uint8_t, 9> lea{0x48,0x8d,0x05,0,0,0,0,0x90,0x90};
            if (rcx_load) lea[2] = 0x0d;
            put(lea, 3, std::int32_t(0x6cffd00 - (site + 7)));
            std::copy(lea.begin(), lea.end(), code.begin() + at);
        }
        for (std::size_t i = 0; i < code.size(); ++i) {
            bool substituted = false;
            for (const auto site : fn.tls_sites()) substituted |= site && i >= site - fn.rva && i < site - fn.rva + 9;
            if (!substituted) require(code[i] == original[i], "Bytes unchanged outside private TLS dependency");
        }
        write(fn.rva, code, PAGE_EXECUTE_READ);
    }
    void load(Pe& pe, std::uint8_t* tls) {
        pointer(0x6cffd00, reinterpret_cast<std::uintptr_t>(tls));
        for (const auto& fn : functions) {
            write_code(pe, fn);
            if (fn.pdata) {
                const auto entry = pe.function(fn.rva, fn.size);
                unwind_.push_back(entry);
                write(entry.UnwindData, pe.unwind(entry), PAGE_READONLY);
            }
        }
        for (const auto [begin, length] : std::array<std::pair<std::uint32_t, std::uint32_t>, 61>{{
            {0x212f4b0, 0x44}, {0x212f4f4, 0x7a}, {0x212f56e, 0x11}, {0x212f57f, 0x13},
            {0x240e3f0, 0x28}, {0x240e418, 0x19e}, {0x240e5b6, 0xb},
            {0x2411840, 0x1c}, {0x241185c, 0x4b}, {0x24118a7, 0x21}, {0x24118c8, 0x4b},
            {0x2411913, 0x86}, {0x2411999, 8}, {0x24119a1, 0xe},
            {0xf32a0d0, 0x20}, {0xf32a0f0, 0x14}, {0xf32a104, 0x7e}, {0xf32a182, 5}, {0xf32a187, 0xb},
            {0x240fcc0, 0x2c}, {0x240fcec, 0x86}, {0x240fd72, 0xb},
            {0xe1db910, 0x116}, {0xe1dba26, 0xdf}, {0xe1dbb05, 0x17},
            {0x20cd9e0, 0x1a2}, {0x20cdb82, 0xde}, {0x20cdc60, 7},
            {0x40b7b0, 0x12}, {0x40b7c2, 0x1a}, {0x40b7dc, 0x94}, {0x40b870, 0x19}, {0x40b889, 6},
            {0x410590, 0x11}, {0x4105a1, 0x16}, {0x4105b7, 0x51}, {0x410608, 0x18}, {0x410620, 0x40},
            {0x2776d00, 0x37}, {0x2776d37, 0x27}, {0x2776d5e, 0x41}, {0x2776d9f, 0x1e}, {0x2776dbd, 0xb},
            {0x96fab0, 0x1c}, {0x96facc, 0x59}, {0x96fb25, 0x18}, {0x96fb3d, 0xd}, {0x96fb4a, 0xb},
            {0x244c460, 0x12}, {0x244c472, 0x6f}, {0x244c4e1, 6},
            {0x2409000, 0x1c}, {0x240901c, 0x96}, {0x24090b2, 0x58}, {0x240910a, 0x13}, {0x240911d, 0x1e},
            {0xee2a210, 0x2f}, {0xee2a23f, 0x41}, {0xee2a280, 0xa6}, {0xee2a326, 0xa}, {0xee2a330, 0x1f}}}) {
            const auto entry = pe.function(begin, length);
            unwind_.push_back(entry);
            write(entry.UnwindData, pe.unwind(entry), PAGE_READONLY);
        }
        std::sort(unwind_.begin(), unwind_.end(), [](const auto& a, const auto& b) { return a.BeginAddress < b.BeginAddress; });
        if (!RtlAddFunctionTable(unwind_.data(), static_cast<DWORD>(unwind_.size()), reinterpret_cast<DWORD64>(base_)))
            throw std::runtime_error("Arena unwind registration");
        registered_ = true;
    }
    void verify_protections() {
        for (const auto& [page, protection] : pages_) {
            MEMORY_BASIC_INFORMATION info{};
            require(VirtualQuery(base_ + page, &info, sizeof(info)) == sizeof(info) &&
                info.Type == MEM_PRIVATE && info.Protect == protection, "Private RX/R-only pages; never RWX");
        }
    }
};

using AcquireKeyed = bool (*)(void*, std::uint32_t, std::uint16_t, const std::uint32_t*) noexcept;
using AcquireUnkeyed = bool (*)(void*, std::uint32_t, std::uint16_t) noexcept;
using Lock = void (*)(void*, bool) noexcept;
using TryLock = bool (*)(void*, bool) noexcept;
AcquireKeyed normal_keyed, user_keyed;
AcquireUnkeyed normal_unkeyed, user_unkeyed;
Lock native_lock, native_unlock;
TryLock native_try;
registry::Lookup client_lookup, server_lookup;
registry::Destroy native_destroy;

struct alignas(8) Actor {
    std::array<std::uint8_t, 0xa8> bytes{};
    std::array<std::uintptr_t, 32> methods{};
    std::array<std::uintptr_t, 3> embedded{};
    std::array<std::uint8_t, 2> type{};
    std::array<std::uint8_t, 0x30> owner_lock{};
    std::array<std::uintptr_t, 9> owner_methods{};
    unsigned owner_attempts = 0, owner_enters = 0, owner_leaves = 0;
    SRWLOCK lock = SRWLOCK_INIT;
    bool user = false, server = false;
    unsigned accesses = 0, depth = 0, enters = 0, leaves = 0, tracked = 0, destroyed = 0, cleanup = 0;
};
struct alignas(8) Manager {
    std::array<std::uint8_t, 0xb8> bytes{};
    std::array<std::uint8_t, 0xa8> client_registry{};
    std::array<std::uint8_t, 256 * 3> buckets{};
    std::array<std::uint8_t, 16> node{};
    std::array<void*, 3> nodes{};
    std::array<std::uintptr_t, 8> methods{};
    bool server = false, held = false;
    unsigned enters = 0, leaves = 0, waits = 0;
    void* object() { return bytes.data() + (server ? 0x68 : 0x10); }
    SRWLOCK* srw() { return reinterpret_cast<SRWLOCK*>(static_cast<std::uint8_t*>(object()) + 0x10); }
};
struct Fixture {
    std::array<Actor, 2> actors;
    std::array<Manager, 2> managers;
    std::array<unsigned, 2> lookups{};
    std::array<std::uintptr_t, 2> returned_actors{};
    std::array<bool, 2> returned_valid{};
    std::vector<unsigned> releases;
    HANDLE entered_event = nullptr;
    unsigned watched_manager = 0;
    bool replace_registry_on_owner_lock = false;
    bool selecting_current = false;
    bool valid = true;
    Fixture() { releases.reserve(2); }
    Actor* actor(void* object, std::size_t offset = 0) noexcept {
        for (auto& a : actors) if (a.bytes.data() + offset == object) return &a;
        valid = false; return nullptr;
    }
    Manager* manager(void* object) noexcept {
        for (auto& m : managers) if (m.object() == object) return &m;
        valid = false; return nullptr;
    }
};
thread_local Fixture* fixture = nullptr;
void enter_registry(void* object, bool shared) noexcept {
    auto* m = fixture->manager(object);
    if (!m) return;
    fixture->valid &= shared && !m->held;
    if (fixture->entered_event && fixture->watched_manager == (m->server ? 2u : 1u))
        SetEvent(fixture->entered_event);
    ++native_calls; native_lock(object, shared); m->held = true; ++m->enters;
}
void leave_registry(void* object, bool shared) noexcept {
    auto* m = fixture->manager(object);
    if (!m) return;
    fixture->valid &= shared && m->held;
    ++native_calls; native_unlock(object, shared); m->held = false; ++m->leaves;
}
void wait_registry(void* object) noexcept {
    auto* m = fixture->manager(object);
    if (!m) return;
    ++m->waits;
    std::uint32_t ready = 0;
    std::memcpy(static_cast<std::uint8_t*>(object) + 8, &ready, sizeof(ready));
}
bool reference_under_registry(Actor& actor) noexcept {
    auto& manager = fixture->managers[actor.server ? 1 : 0];
    bool held = manager.held;
    if (TryAcquireSRWLockExclusive(manager.srw())) { held = false; ReleaseSRWLockExclusive(manager.srw()); }
    fixture->valid &= held;
    ++actor.accesses;
    return held;
}
bool acquire_keyed(void* object, std::uint32_t kind, std::uint16_t flags, const std::uint32_t* handle) noexcept {
    auto* a = fixture->actor(object);
    if (!a) return false;
    fixture->valid &= kind == 4 && flags == 0x10 && handle != nullptr && a->server;
    if (!reference_under_registry(*a)) return false;
    ++native_calls; return (a->user ? user_keyed : normal_keyed)(object, kind, flags, handle);
}
bool acquire_unkeyed(void* object, std::uint32_t kind, std::uint16_t flags) noexcept {
    auto* a = fixture->actor(object);
    if (!a) return false;
    fixture->valid &= kind == 4 && flags == 0x10 && !a->server;
    if (fixture->selecting_current) {
        fixture->valid &= get<void*>(fixture->managers[0].bytes, 0x50) == object;
        ++a->accesses;
    } else if (!reference_under_registry(*a)) return false;
    ++native_calls; return (a->user ? user_unkeyed : normal_unkeyed)(object, kind, flags);
}
void embedded_enter(void* object) noexcept {
    if (auto* a = fixture->actor(object, 0x18)) { AcquireSRWLockExclusive(&a->lock); ++a->depth; ++a->enters; }
}
void embedded_leave(void* object) noexcept {
    if (auto* a = fixture->actor(object, 0x18)) {
        fixture->valid &= a->depth == 1; --a->depth; ++a->leaves; ReleaseSRWLockExclusive(&a->lock);
    }
}
void register_reference(void* object, bool special) noexcept {
    if (auto* a = fixture->actor(object)) {
        fixture->valid &= !special && a->depth == 1;
        ++a->tracked; put(a->bytes, 0x48, std::uint16_t(get<std::uint16_t>(a->bytes, 0x48) + 1));
    }
}
void unregister_reference(void* object, bool special) noexcept {
    if (auto* a = fixture->actor(object)) {
        fixture->valid &= !special && a->depth == 1 && a->tracked > 0;
        --a->tracked; put(a->bytes, 0x48, std::uint16_t(get<std::uint16_t>(a->bytes, 0x48) - 1));
    }
}
void* reference_map() noexcept { return fixture; }
void* find_reference(void* map, void** object) noexcept {
    fixture->valid &= map == fixture;
    auto* a = fixture->actor(*object);
    return a && a->tracked ? &a->tracked : nullptr;
}
bool is_server(void* object) noexcept { auto* a = fixture->actor(object); return a && a->server; }
void* no_parent(void*) noexcept { return nullptr; }
bool no_reacquire(void*) noexcept { return false; }
void cleanup(void* object) noexcept {
    if (auto* a = fixture->actor(object)) { fixture->valid &= !a->depth && a->server; ++a->cleanup; }
}
void destroy(void* object) noexcept {
    if (auto* a = fixture->actor(object)) {
        fixture->valid &= !a->depth && !a->tracked && !get<std::uint32_t>(a->bytes, 0x10) &&
            !a->bytes[0x4a] && a->bytes[0x4b] == 1;
        ++a->destroyed; // Keep private storage allocated to detect duplicate destruction.
    }
}
void* lookup_client(void* manager, reference::Receipt* out, std::uint32_t handle) noexcept {
    ++fixture->lookups[0]; ++native_calls;
    auto* returned = client_lookup(manager, out, handle);
    fixture->valid &= returned == out && !out->bytes[0x11] && !get<std::uintptr_t>(out->bytes, 0x18);
    fixture->returned_actors[0] = get<std::uintptr_t>(out->bytes, 8);
    fixture->returned_valid[0] = out->bytes[0x10] != 0;
    return returned;
}
void* lookup_server(void* manager, reference::Receipt* out, std::uint32_t handle) noexcept {
    ++fixture->lookups[1]; ++native_calls;
    auto* returned = server_lookup(manager, out, handle);
    fixture->valid &= returned == out && !out->bytes[0x11] && !get<std::uintptr_t>(out->bytes, 0x18);
    fixture->returned_actors[1] = get<std::uintptr_t>(out->bytes, 8);
    fixture->returned_valid[1] = out->bytes[0x10] != 0;
    return returned;
}
void release_client(reference::Receipt* out) noexcept {
    for (const auto& actor : fixture->actors)
        fixture->valid &= get<std::uint32_t>(actor.owner_lock, 0x2c) == 0;
    fixture->releases.push_back(0); ++native_calls; native_destroy(out);
}
void release_server(reference::Receipt* out) noexcept {
    for (const auto& actor : fixture->actors)
        fixture->valid &= get<std::uint32_t>(actor.owner_lock, 0x2c) == 0;
    fixture->releases.push_back(1); ++native_calls; native_destroy(out);
}
bool try_owner(void* object, bool shared) noexcept {
    for (auto& actor : fixture->actors) {
        if (actor.owner_lock.data() != object) continue;
        ++actor.owner_attempts;
        fixture->valid &= !shared && !fixture->managers[0].held && !fixture->managers[1].held;
        for (const auto& owner : fixture->actors)
            fixture->valid &= get<std::uint32_t>(owner.bytes, 0x10) + owner.tracked > 0;
        ++native_calls;
        const bool acquired = native_try(object, shared);
        if (acquired) {
            ++actor.owner_enters;
            if (fixture->replace_registry_on_owner_lock) {
                // Simulate a replaced registry entry after both references were
                // acquired. The held objects remain alive; no raw re-resolution.
                put(fixture->managers[1].node, 8, std::uintptr_t(0x300000000));
                fixture->replace_registry_on_owner_lock = false;
            }
        }
        return acquired;
    }
    fixture->valid = false; return false;
}
void release_owner(void* object, bool shared) noexcept {
    for (auto& actor : fixture->actors) {
        if (actor.owner_lock.data() != object) continue;
        fixture->valid &= !shared && get<std::uint32_t>(actor.owner_lock, 0x2c) == 1;
        ++native_calls; native_unlock(object, shared); ++actor.owner_leaves;
        return;
    }
    fixture->valid = false;
}

void initialize(Fixture& f, Arena& arena, std::uint32_t handle, bool user) {
    for (std::size_t i = 0; i < 2; ++i) {
        auto& a = f.actors[i]; auto& m = f.managers[i];
        a.user = user; a.server = m.server = i == 1;
        a.methods[1] = reinterpret_cast<std::uintptr_t>(arena.function<void*>(0x14366a0));
        a.methods[2] = reinterpret_cast<std::uintptr_t>(&destroy);
        a.methods[0x58 / 8] = reinterpret_cast<std::uintptr_t>(&cleanup);
        a.methods[0x90 / 8] = reinterpret_cast<std::uintptr_t>(&is_server);
        a.methods[0xb8 / 8] = reinterpret_cast<std::uintptr_t>(&acquire_keyed);
        a.methods[0xc0 / 8] = reinterpret_cast<std::uintptr_t>(&acquire_unkeyed);
        a.methods[0xf0 / 8] = reinterpret_cast<std::uintptr_t>(&no_parent);
        a.embedded[1] = reinterpret_cast<std::uintptr_t>(&embedded_enter);
        a.embedded[2] = reinterpret_cast<std::uintptr_t>(&embedded_leave);
        put(a.bytes, 0, a.methods.data()); put(a.bytes, 0x18, a.embedded.data());
        put(a.bytes, 8, a.owner_lock.data());
        a.owner_methods[3] = reinterpret_cast<std::uintptr_t>(native_lock);
        a.owner_methods[4] = reinterpret_cast<std::uintptr_t>(native_unlock);
        put(a.owner_lock, 0, a.owner_methods.data());
        put(a.owner_lock, 0xc, std::uint32_t(0x80000000)); // published fixture lock; no lazy publication callback
        put(a.owner_lock, 0x28, UINT32_MAX);
        InitializeSRWLock(reinterpret_cast<SRWLOCK*>(a.owner_lock.data() + 0x10));
        put(a.bytes, 0x60, handle); put(a.bytes, 0x5e, std::uint16_t(user ? 0x14 : 0x10));
        a.bytes[0x4a] = 1; a.type[1] = 1; put(a.bytes, 0x88, a.type.data());
        m.methods[3] = reinterpret_cast<std::uintptr_t>(&enter_registry);
        m.methods[4] = reinterpret_cast<std::uintptr_t>(&leave_registry);
        m.methods[7] = reinterpret_cast<std::uintptr_t>(&wait_registry);
        put(m.bytes, m.server ? 0x68 : 0x10, m.methods.data()); InitializeSRWLock(m.srw());
        const auto key = m.server ? handle & 0xfffffu : handle;
        const auto bucket = std::size_t(key % 3) * 256;
        put(m.buckets, bucket, std::uint32_t(2)); put(m.buckets, bucket + 8, std::uint32_t(99));
        put(m.buckets, bucket + 16, key); put(m.buckets, bucket + 20, std::uint32_t(2));
        put(m.node, 4, key); put(m.node, 8, a.bytes.data()); m.nodes[2] = m.node.data();
        if (m.server) {
            put(m.bytes, 0x98, std::uint32_t(3)); put(m.bytes, 0x9c, std::uint32_t(1));
            put(m.bytes, 0xa8, m.buckets.data()); put(m.bytes, 0xb0, m.nodes.data());
        } else {
            put(m.bytes, 8, m.client_registry.data());
            put(m.client_registry, 0x88, std::uint32_t(3)); put(m.client_registry, 0x8c, std::uint32_t(1));
            put(m.client_registry, 0x98, m.buckets.data()); put(m.client_registry, 0xa0, m.nodes.data());
        }
    }
}

// Own item storage for the separately observed 2949 layout. Native accessors
// below verify these same fixtures; the engine host/transition gate is absent.
struct InventoryFixture {
    std::array<std::uint8_t, 0xd8> possessor{};
    std::array<std::uint8_t, 0xc0> parts{};
    std::array<std::uint8_t, 0x78> status{};
    std::array<std::uint8_t, 0x28> holder{};
    std::array<std::uint8_t, 0x208> equipment{};
    std::array<std::uint8_t, 0x30> bag{};
    alignas(8) std::array<std::uint8_t, 0xc8> item{};
    std::array<std::uint8_t, 0x18> equipment_table{};
    alignas(8) std::array<std::uint8_t, 0xd0> equipped{};
    std::array<std::uint8_t, 12> sockets{};
    std::array<std::uint8_t, 4> socket_padding{};
    std::array<void*, 1> buckets{};
    bool operator==(const InventoryFixture&) const = default;
    void initialize(Actor& actor) {
        put(actor.bytes, 0xa0, possessor.data()); put(possessor, 0xd0, actor.bytes.data());
        put(actor.bytes, 0x68, parts.data());
        put(parts, 0x20, status.data());
        put(parts, 0xb8, holder.data()); put(parts, 0x38, equipment.data());
        put(holder, 8, actor.bytes.data()); put(equipment, 8, actor.bytes.data());
        buckets[0] = bag.data(); put(holder, 0x18, buckets.data()); put(holder, 0x20, std::uint32_t(1));
        put(bag, 0, item.data()); put(bag, 0xc, std::uint16_t(1)); put(bag, 0x10, reader::carried_container);
        put(item, 0, std::uint64_t(11)); put(item, 8, std::uint16_t(10));
        put(item, 0x10, std::int64_t(1)); put(item, 0x40, std::uint16_t(35));
        put(item, 0x60, sockets.data()); put(item, 0x68, std::uint32_t(2)); put(item, 0x6c, std::uint32_t(2)); item[0x70] = 2;
        put(sockets, 0, std::uint16_t(20)); put(sockets, 2, std::uint16_t(5)); sockets[5] = 5;
        put(sockets, 6, action::absent); sockets[10] = 1;
        put(equipment, 0x90, equipment_table.data()); put(equipment_table, 8, equipped.data());
        put(equipment, 0x1f4, UINT32_MAX); put(equipment, 0x1f8, UINT32_MAX);
        put(equipment_table, 0x10, std::uint32_t(1));
        put(equipped, 0, std::uint64_t(22)); put(equipped, 8, std::uint16_t(20));
        put(equipped, 0x10, std::int64_t(1)); put(equipped, 0xc8, std::uint16_t(3));
    }
};
class CaptureMemory final : public writer::Access {
    struct Region { std::uintptr_t start; std::size_t size; bool item; };
    Fixture& fixture_;
    std::array<std::uint8_t, 0x38> context_{};
    std::vector<Region> allowed_;
    writer::LocalAccess local_;
    std::uintptr_t context_pointer_;
    static bool covers(const Region& region, std::uintptr_t address, std::size_t size) {
        return address >= region.start && address - region.start <= region.size && size <= region.size - (address - region.start);
    }
public:
    reader::Frame frame{0x180000000, 41, 7, reader::Frame::Build::steam_25455892};
    bool native_items() const noexcept override { return true; }
    std::uintptr_t change_address = 0;
    mutable unsigned changes_seen = 0, registry_reads = 0, protected_item_reads = 0;
    template<class T> void allow(const T& object, bool item = false) {
        allowed_.push_back({reinterpret_cast<std::uintptr_t>(&object), sizeof(object), item});
    }
    CaptureMemory(Fixture& f, std::array<InventoryFixture, 2>& inventories) :
        fixture_(f), context_pointer_(reinterpret_cast<std::uintptr_t>(context_.data())) {
        put(context_, 0x30, f.managers[0].bytes.data());
        put(f.managers[0].bytes, 0x50, f.actors[0].bytes.data());
        allow(context_);
        for (std::size_t i = 0; i < 2; ++i) {
            auto& a = f.actors[i]; auto& v = inventories[i];
            allow(a.bytes); allow(a.type);
            allow(v.possessor); allow(v.parts); allow(v.holder); allow(v.equipment);
            allow(v.bag); allow(v.equipment_table); allow(v.buckets);
            allow(v.item, true); allow(v.equipped, true); allow(v.sockets, true);
        }
        allowed_.push_back({reinterpret_cast<std::uintptr_t>(f.managers[0].bytes.data() + 0x50), 8, false});
    }
    bool copy(std::uintptr_t address, std::span<std::uint8_t> output) const noexcept override {
        bool copied = false;
        if (address == frame.image_base + reader::client_context_rva && output.size() == 8) {
            std::memcpy(output.data(), &context_pointer_, 8); copied = true;
        } else {
            for (const auto& region : allowed_) {
                if (!covers(region, address, output.size())) continue;
                if (region.item) {
                    for (auto& actor : fixture_.actors) {
                        if (get<std::uint32_t>(actor.owner_lock, 0x2c) != 1 ||
                            get<std::uint32_t>(actor.bytes, 0x10) + actor.tracked == 0) return false;
                        auto* srw = reinterpret_cast<SRWLOCK*>(actor.owner_lock.data() + 0x10);
                        if (TryAcquireSRWLockExclusive(srw)) { ReleaseSRWLockExclusive(srw); return false; }
                    }
                    ++protected_item_reads;
                }
                copied = local_.copy(address, output);
                break;
            }
        }
        if (!copied) {
            for (const auto& manager : fixture_.managers) {
                const Region region{reinterpret_cast<std::uintptr_t>(&manager), sizeof(manager), false};
                if (covers(region, address, output.size())) ++registry_reads;
            }
            if (address == frame.image_base + reader::server_context_rva) ++registry_reads;
        }
        if (copied && address == change_address && ++changes_seen >= 2 && !output.empty()) output.back() ^= 1;
        return copied;
    }
    bool writable(std::uintptr_t address, std::size_t size) const noexcept override {
        for (const auto& region : allowed_) if (region.item && covers(region, address, size)) {
            for (const auto& actor : fixture_.actors)
                if (get<std::uint32_t>(actor.owner_lock, 0x2c) != 1 ||
                    get<std::uint32_t>(actor.bytes, 0x10) + actor.tracked == 0) return false;
            return local_.writable(address, size);
        }
        return false;
    }
    bool write_word(std::uintptr_t address, std::uint16_t before, std::uint16_t after) const noexcept override {
        return writable(address, 2) && local_.write_word(address, before, after);
    }
};
void native_captures(Arena& arena, std::array<std::uint8_t, 0x200>& tls) {
    constexpr std::uint32_t handle = 0x12346;
    const std::array definitions{action::Definition{10, 100}, action::Definition{20, 30}};
    for (unsigned scenario = 0; scenario < 18; ++scenario) {
        Fixture f; fixture = &f;
        tls[0x1d2] = std::uint8_t(scenario != 2 && scenario != 3); tls[0x1d4] = 0;
        initialize(f, arena, handle, scenario == 1 || scenario == 3);
        std::array<InventoryFixture, 2> inventories;
        for (std::size_t i = 0; i < 2; ++i) inventories[i].initialize(f.actors[i]);
        CaptureMemory memory(f, inventories);
        reader::Code expected = reader::Code::captured;
        if (scenario == 4 || scenario == 5) expected = reader::Code::lock_busy;
        if (scenario == 6) { f.actors[0].bytes[0x4a] = 0; expected = reader::Code::owner_unavailable; }
        if (scenario == 7) { f.actors[1].bytes[0x4b] = 1; expected = reader::Code::owner_unavailable; }
        if (scenario == 8) {
            put(f.managers[0].bytes, 0x50, std::uintptr_t(0x300000000)); expected = reader::Code::identity_mismatch;
        }
        if (scenario == 9) { put(inventories[1].item, 0x60, std::uintptr_t(0x300000000)); expected = reader::Code::unreadable; }
        if (scenario == 10) {
            memory.change_address = reinterpret_cast<std::uintptr_t>(f.managers[0].bytes.data() + 0x50);
            expected = reader::Code::changed_during_read;
        }
        if (scenario == 11) { put(inventories[1].item, 0, std::uint64_t(12)); expected = reader::Code::identity_mismatch; }
        if (scenario == 12) { put(f.actors[1].bytes, 0x60, handle | 0x100000u); expected = reader::Code::owner_unavailable; }
        if (scenario == 13) f.replace_registry_on_owner_lock = true;
        if (scenario == 14) { put(f.actors[1].bytes, 0x5e, std::uint16_t(0)); expected = reader::Code::owner_unavailable; }
        if (scenario == 15) { put(f.managers[0].bytes, 0x50, std::uintptr_t(0)); expected = reader::Code::no_player; }
        if (scenario == 16) { put(inventories[1].item, 0x40, std::uint16_t(101)); expected = reader::Code::invalid_layout; }
        if (scenario == 17) {
            memory.change_address = reinterpret_cast<std::uintptr_t>(f.actors[1].bytes.data() + 0x4a);
            expected = reader::Code::owner_unavailable;
        }
        registry::Source client_source(f.managers[0].bytes.data(), lookup_client, release_client);
        registry::Source server_source(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{client_source.source(handle), server_source.source(handle)};
        const std::array bindings{lease::Binding{f.actors[0].owner_lock.data(), try_owner, release_owner},
            lease::Binding{f.actors[1].owner_lock.data(), try_owner, release_owner}};
        const auto before_client = inventories[0], before_server = inventories[1];
        std::atomic<bool> ready{false};
        std::jthread competitor;
        if (scenario == 4 || scenario == 5) {
            auto* srw = reinterpret_cast<SRWLOCK*>(f.actors[scenario - 4].owner_lock.data() + 0x10);
            competitor = std::jthread([&, srw](std::stop_token stop) {
                AcquireSRWLockShared(srw); ready.store(true); ready.notify_one();
                while (!stop.stop_requested()) Sleep(1);
                ReleaseSRWLockShared(srw);
            });
            ready.wait(false);
        }
        reader::Capture captured;
        captured.authority.actor = UINTPTR_MAX;
        reader::Code result;
        if (scenario == 0) {
            reader::HeldCapture held;
            result = held.open(memory, memory.frame, definitions, sources, bindings);
            require(result == reader::Code::captured && held.view(), "Native retained capture opens under both real owner locks");
            std::array<bool, 2> excluded{};
            std::jthread probe([&] {
                for (std::size_t i = 0; i < 2; ++i) {
                    auto* srw = reinterpret_cast<SRWLOCK*>(f.actors[i].owner_lock.data() + 0x10);
                    const auto acquired = TryAcquireSRWLockExclusive(srw);
                    excluded[i] = !acquired;
                    if (acquired) ReleaseSRWLockExclusive(srw);
                }
            }); probe.join();
            require(excluded[0] && excluded[1], "A competing thread cannot write either owner after open returns");
            const auto first = held.view()->snapshot;
            require(held.refresh(memory.frame) == reader::Code::captured && held.view()->snapshot == first &&
                f.actors[0].owner_attempts == 1 && f.actors[1].owner_attempts == 1,
                "Native retained refresh does not release/reacquire or replace its owners");
            captured = *held.view();
            require(held.release_locks() == reader::Code::released && !held.view() &&
                get<std::uint32_t>(f.actors[0].bytes, 0x10) == 1 && get<std::uint32_t>(f.actors[1].bytes, 0x10) == 1 &&
                !get<std::uint32_t>(f.actors[0].owner_lock, 0x2c) && !get<std::uint32_t>(f.actors[1].owner_lock, 0x2c),
                "Native notification phase retains actual actor references after unlocking");
        } else result = reader::capture_with_references(memory, memory.frame, definitions, sources, bindings, captured);
        if (competitor.joinable()) { competitor.request_stop(); competitor.join(); }
        require(result == expected, "Combined native references/owner locks/reader result");
        require(!memory.registry_reads, "Referenced reader performs no unprotected registry read");
        if (expected == reader::Code::captured) {
            require(captured.snapshot.items.size() == 2 && memory.protected_item_reads >= 12,
                "Inventory, equipment and socket reads plus verification hold both actual owner locks");
            // Compare the same private sources with the current native accessors.
            // Fixture lifetimes outlive this call; this is not a post-release
            // live-pointer permission for a future engine host.
            using NativeHolder = void* (*)(void*);
            using NativeSlot = void* (*)(void*, std::uint16_t, std::int16_t);
            for (std::size_t i = 0; i < 2; ++i) {
                native_calls += 2;
                require(arena.function<NativeHolder>(0x212a100)(f.actors[i].bytes.data()) == inventories[i].holder.data() &&
                    arena.function<NativeSlot>(0x212f4b0)(inventories[i].holder.data(), reader::carried_container, 0) == inventories[i].item.data(),
                    "The captured client/server fixtures match current native holder and inventory getters");
            }
            action::Plan plan;
            require(action::prepare({captured.snapshot.session, action::Scope::all, {}}, captured.snapshot, plan) == action::Code::prepared &&
                plan.count() == action::Count{2, 2, 1}, "Captured private snapshot feeds exact repair planning");
        } else require(captured.snapshot.items.empty() && !captured.authority.actor, "Combined failure clears old capture");
        require(before_client == inventories[0] && before_server == inventories[1], "Capture and plan never modify source item storage");
        require(f.valid && f.releases == std::vector<unsigned>{1,0}, "Native lock release precedes reverse receipt cleanup");
        for (auto& actor : f.actors) {
            require(!get<std::uint32_t>(actor.bytes, 0x10) && !actor.tracked &&
                !get<std::uint16_t>(actor.bytes, 0x48), "Combined capture returns every actual reference");
            require(actor.owner_enters == actor.owner_leaves && !get<std::uint32_t>(actor.owner_lock, 0x2c),
                "Actual owner lock recursion is balanced after success and partial failure");
            const auto free = TryAcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(actor.owner_lock.data() + 0x10));
            if (free) ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(actor.owner_lock.data() + 0x10));
            require(free, "Actual Windows owner lock is available after capture");
        }
        if (scenario == 4 || scenario == 5) require(!memory.protected_item_reads, "Busy owner means no item read");
        if (scenario == 12 || scenario == 14) require(!f.actors[0].owner_attempts && !f.actors[1].owner_attempts,
            "Failed second native reference does not attempt an owner lock");
    }
    fixture = nullptr;
}

#include "inventory_native.inl"
#include "item_native.inl"
#include "dirty_native.inl"
#include "dirty_consumer_native.inl"
#include "writer_native.inl"
#include "equipment_events_native.inl"
#include "inventory_events_native.inl"
#include "sql_native.inl"
#include "save_native.inl"
#include "save_dispatch_native.inl"
#include "save_file_native.inl"
#include "save_encode_native.inl"
#include "inventory_packet_native.inl"

void native_tests(const std::filesystem::path& path) {
    Pe pe(path, NativeBuild::registry_2949);
    std::array<std::uint8_t, 0x200> tls{};
    tls[0x98] = 1;
    Arena arena;
    arena.load(pe, tls.data());
    for (const auto [at, callback] : std::array<std::pair<std::uint32_t, std::uintptr_t>, 5>{{
        {0x1434500, reinterpret_cast<std::uintptr_t>(&register_reference)},
        {0x1434730, reinterpret_cast<std::uintptr_t>(&unregister_reference)},
        {0x14342c0, reinterpret_cast<std::uintptr_t>(&reference_map)},
        {0x3d1e80, reinterpret_cast<std::uintptr_t>(&find_reference)},
        {0x1434430, reinterpret_cast<std::uintptr_t>(&no_reacquire)}}}) arena.redirect(at, callback);
    // These are real Windows locks on caller-owned storage. The tested registry
    // methods pass shared=true, so Acquire's exclusive TLS path is unreachable.
    arena.pointer(0x51eb208, reinterpret_cast<std::uintptr_t>(&AcquireSRWLockShared));
    arena.pointer(0x51eb210, reinterpret_cast<std::uintptr_t>(&AcquireSRWLockExclusive));
    arena.pointer(0x51eb218, reinterpret_cast<std::uintptr_t>(&ReleaseSRWLockShared));
    arena.pointer(0x51eb220, reinterpret_cast<std::uintptr_t>(&ReleaseSRWLockExclusive));
    arena.pointer(0x51eb228, reinterpret_cast<std::uintptr_t>(&TryAcquireSRWLockShared));
    arena.pointer(0x51eb230, reinterpret_cast<std::uintptr_t>(&TryAcquireSRWLockExclusive));
    arena.pointer(0x51eb2e0, reinterpret_cast<std::uintptr_t>(&GetCurrentThreadId));
    native_lock = arena.function<Lock>(0x1371b60); native_unlock = arena.function<Lock>(0x1371c00);
    native_try = arena.function<TryLock>(0x1371c30);
    normal_keyed = arena.function<AcquireKeyed>(0x4f9000); user_keyed = arena.function<AcquireKeyed>(0x8a6760);
    normal_unkeyed = arena.function<AcquireUnkeyed>(0x4f8ff0); user_unkeyed = arena.function<AcquireUnkeyed>(0x8a6750);
    client_lookup = arena.function<registry::Lookup>(0x8ae140); server_lookup = arena.function<registry::Lookup>(0x2a82300);
    native_destroy = arena.function<registry::Destroy>(0x1434910);
    for (const auto vt : {0x57cda50u, 0x5b1dc30u, 0x5585c00u, 0x5b255a0u, 0x55b5958u}) {
        const auto raw = pe.rva(vt + 8, 8);
        std::uint64_t method; std::memcpy(&method, raw.data(), 8);
        require(method == 0x1414366a0ull, "Current concrete actor release override");
    }
    for (unsigned scenario = 0; scenario < 26; ++scenario) {
        Fixture f; fixture = &f;
        const auto handle = scenario == 22 ? 0x60012346u : 0x12346u;
        const bool user = scenario == 1 || scenario == 4 || scenario == 12;
        const bool tracked = scenario == 3 || scenario == 4 || scenario == 15 || scenario == 16 || scenario == 19;
        tls[0x1d2] = std::uint8_t(!tracked && scenario != 2); tls[0x1d4] = std::uint8_t(scenario == 2);
        initialize(f, arena, handle, user);
        auto& client = f.actors[0]; auto& server = f.actors[1];
        if (scenario == 5) put(f.managers[0].client_registry, 0x8c, std::uint32_t(0));
        if (scenario == 6) put(f.managers[1].bytes, 0x9c, std::uint32_t(0));
        if (scenario == 7) put(server.bytes, 0x60, handle | 0x100000u);
        if (scenario == 8) put(f.managers[0].node, 4, std::uint32_t(98));
        if (scenario == 9) put(f.managers[1].node, 4, std::uint32_t(98));
        if (scenario == 10) put(client.bytes, 0x5e, std::uint16_t(0));
        if (scenario == 11 || scenario == 18 || scenario == 19) put(server.bytes, 0x5e, std::uint16_t(0));
        if (scenario == 12) put(client.bytes, 0x5e, std::uint16_t(0x10));
        if (scenario == 13) server.type[1] = 2;
        if (scenario == 14 || scenario == 15) client.bytes[0x4a] = 0;
        if (scenario == 18) put(server.bytes, 0x10, std::uint32_t(1));
        if (scenario == 19) { put(server.bytes, 0x48, std::uint16_t(1)); server.tracked = 1; }
        if (scenario == 23) put(f.managers[1].bytes, 0x98, std::uint32_t(0));
        if (scenario == 24) put(f.managers[0].client_registry, 0x88, std::uint32_t(0));
        if (scenario == 25) {
            put(f.managers[0].bytes, 0x1c, std::uint32_t(0x40000000));
            put(f.managers[0].bytes, 0x18, UINT32_MAX);
        }
        const auto requested = scenario == 20 ? 0u : scenario == 21 ? handle | 0x10000000u : handle;
        registry::Source client_source(f.managers[0].bytes.data(), lookup_client, release_client);
        registry::Source server_source(f.managers[1].bytes.data(), lookup_server, release_server);
        const std::array sources{client_source.source(requested), server_source.source(requested)};
        const bool accepted = scenario < 5 || scenario == 14 || (scenario >= 16 && scenario <= 19) || scenario == 22 || scenario == 25;
        const auto before_client_node = f.managers[0].node, before_server_node = f.managers[1].node;
        const auto before_client_buckets = f.managers[0].buckets, before_server_buckets = f.managers[1].buckets;
        {
            reference::Pair pair;
            const auto result = pair.acquire(sources);
            const auto expected = scenario == 20 ? reference::Code::invalid_source :
                accepted ? reference::Code::acquired : reference::Code::unavailable;
            require(result == expected, "Protected native lookup/reference pair outcome");
            if (accepted) {
                require(pair.actor(0) == reinterpret_cast<std::uintptr_t>(client.bytes.data()) &&
                    pair.actor(1) == reinterpret_cast<std::uintptr_t>(server.bytes.data()), "Both owned receipts expose the expected actors");
                require(get<std::uint32_t>(client.bytes, 0x10) == (tracked ? 0u : 1u) &&
                    get<std::uint16_t>(client.bytes, 0x48) == (tracked ? 1u : 0u), "Actual direct/tracked reference acquisition");
                if (scenario == 16) { put(server.bytes, 0x5e, std::uint16_t(0x20)); server.bytes[0x4a] = 0; }
                if (scenario == 17) { client.bytes[0x4a] = 0; server.bytes[0x4a] = 0; }
            } else require(!pair.actor(0) && !pair.actor(1), "A failed second lookup never exposes the first actor");
        }
        require(f.lookups[0] == unsigned(scenario != 20), "Zero handle does not reach a native source");
        const std::vector<unsigned> expected_release = f.lookups[1] ? std::vector<unsigned>{1,0} :
            f.lookups[0] ? std::vector<unsigned>{0} : std::vector<unsigned>{};
        require(f.releases == expected_release, "Source-specific receipt cleanup is once, in reverse order");
        if (scenario == 7 || scenario == 11)
            require(f.returned_actors[1] == reinterpret_cast<std::uintptr_t>(server.bytes.data()) && !f.returned_valid[1],
                "Native failed keyed acquisition retains a pointer but never grants ownership");
        if (scenario == 12)
            require(f.returned_actors[0] == reinterpret_cast<std::uintptr_t>(client.bytes.data()) && !f.returned_valid[0],
                "User reference kind and normal actor state mask are distinct");
        require(!get<std::uint32_t>(client.bytes, 0x10) && get<std::uint32_t>(server.bytes, 0x10) == unsigned(scenario == 18),
            "Native direct references balance, including rollback and existing ownership");
        require(!client.tracked && server.tracked == unsigned(scenario == 19) &&
            get<std::uint16_t>(server.bytes, 0x48) == unsigned(scenario == 19), "Tracked reference cleanup preserves outer ownership");
        require(client.destroyed == unsigned(scenario == 14 || scenario == 17) &&
            server.destroyed == unsigned(scenario == 16 || scenario == 17), "Destruction occurs only after last reference, once");
        require(server.cleanup == unsigned(scenario == 16) && (!server.cleanup || get<std::uint16_t>(server.bytes, 0x5e) == 0x40),
            "Actual derived release performs deferred server cleanup");
        require(f.valid && !client.depth && !server.depth && client.enters == client.leaves && server.enters == server.leaves,
            "Reference and registry callback contract");
        for (auto& m : f.managers) {
            require(!m.held && m.enters == m.leaves, "Native registry lock balanced");
            const auto available = TryAcquireSRWLockExclusive(m.srw());
            if (available) ReleaseSRWLockExclusive(m.srw());
            require(available, "Registry admits a writer after lookup");
        }
        require(f.managers[0].waits == unsigned(scenario == 25), "Explicit fixture publication-wait callback");
        require(f.managers[0].node == before_client_node && f.managers[1].node == before_server_node &&
            f.managers[0].buckets == before_client_buckets && f.managers[1].buckets == before_server_buckets,
            "Lookup leaves all registry keys, slots and pointers unchanged");
        reference::Receipt invalid; put(invalid.bytes, 8, std::uintptr_t(1));
        ++native_calls; native_destroy(&invalid);
        require(!invalid.actor(), "Invalid non-null receipt is never dereferenced");
    }
    fixture = nullptr;
    tls[0x1d2] = 1; tls[0x1d4] = 0;
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        Fixture f;
        initialize(f, arena, 0x12346u, false);
        const unsigned role = scenario / 2;
        const bool remove_before_lookup = (scenario % 2) != 0;
        auto& manager = f.managers[role];
        const HANDLE entered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!entered) throw std::runtime_error("Cannot create fixture event");
        f.entered_event = entered; f.watched_manager = role + 1;
        std::atomic<bool> finished{false};
        reference::Code outcome = reference::Code::empty;
        AcquireSRWLockExclusive(manager.srw());
        std::jthread worker([&] {
            fixture = &f;
            registry::Source client_source(f.managers[0].bytes.data(), lookup_client, release_client);
            registry::Source server_source(f.managers[1].bytes.data(), lookup_server, release_server);
            const std::array sources{client_source.source(0x12346u), server_source.source(0x12346u)};
            { reference::Pair pair; outcome = pair.acquire(sources); }
            fixture = nullptr;
            finished.store(true);
        });
        const bool attempted = WaitForSingleObject(entered, 2000) == WAIT_OBJECT_0;
        const bool excluded = attempted && !finished.load() && f.actors[role].accesses == 0;
        if (remove_before_lookup) {
            if (role == 0) put(manager.client_registry, 0x8c, std::uint32_t(0));
            else put(manager.bytes, 0x9c, std::uint32_t(0));
        }
        // Always unblock/join before any throwing assertions or fixture teardown.
        ReleaseSRWLockExclusive(manager.srw());
        worker.join(); CloseHandle(entered);
        require(excluded, "Actual registry reference acquisition waits for a concurrent exclusive writer");
        require(outcome == (remove_before_lookup ? reference::Code::unavailable : reference::Code::acquired),
            "Lookup observes registry removal under the writer lock, not a cached actor");
        require(f.valid && !get<std::uint32_t>(f.actors[0].bytes, 0x10) && !get<std::uint32_t>(f.actors[1].bytes, 0x10),
            "Concurrent lookup and partial rollback leave no actor references");
        require(f.releases == (role == 0 && remove_before_lookup ? std::vector<unsigned>{0} : std::vector<unsigned>{1,0}),
            "Concurrent first/second source failure uses balanced source-specific cleanup");
    }
    observe_layout(pe);
    install_layout_dependencies(arena);
    std::array<std::uint8_t, action::socket_size> empty_socket{};
    put(empty_socket, 0, action::absent);
    arena.pointer(0x6cfcd90, reinterpret_cast<std::uintptr_t>(empty_socket.data()));
    arena.pointer(0x6cf7248, 0);
    inventory_native_tests(arena);
    selected_native_tests(arena, tls);
    endurance_native_tests(arena);
    equipment_native_tests(arena, tls);
    native_captures(arena, tls);
    item_native_tests(pe, arena, tls);
    dirty_native_tests(pe, arena, tls);
    dirty_consumer_native_tests(pe, arena, tls);
    writer_native_tests(arena, tls);
    equipment_events_native_tests(pe, arena, tls);
    inventory_events_native_tests(pe, arena, tls);
    sql_native_tests(pe, arena, tls);
    save_native_tests(pe, arena, tls);
    save_dispatch_native_tests(arena, tls);
    save_file_native_tests(arena, tls);
    save_encode_native_tests(pe, arena, tls);
    inventory_packet_native_tests(pe, arena, tls);
    arena.verify_protections();
    pe.verify_unchanged();
}
void manifest_contract() {
    std::vector<std::uint32_t> seen;
    for (const auto& fn : functions) {
        require(fn.rva && fn.size && std::string_view(fn.sha).size() == 64, "Native fixture manifest geometry");
        for (const auto site : fn.tls_sites())
            require(!site || (site >= fn.rva && site + 9 <= fn.rva + fn.size), "TLS substitution stays inside function");
        require(std::find(seen.begin(), seen.end(), fn.rva) == seen.end(), "Unique manifest function start");
        seen.push_back(fn.rva);
    }
}
}
int wmain(int argc, wchar_t** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try {
        manifest_contract();
        const bool native = argc == 3 && std::wstring_view(argv[1]) == L"--exe";
        if (argc != 1 && !native) throw std::runtime_error("Usage: repair-registry-native [--exe path]");
        if (native) native_tests(argv[2]);
        std::cout << "{\"module\":\"0.23.0\",\"checks\":" << checks << ",\"native_calls\":" << native_calls.load()
            << ",\"native_executed\":" << (native ? "true" : "false")
            << ",\"scenarios\":" << (native ? 48 + inventory_cases + selected_cases + updater_cases + equipment_cases + event_cases + item_cases + dirty_cases + consumer_cases + writer_cases + inventory_event_cases + sql_cases + save_cases + save_dispatch_cases + save_file_cases + save_encode_cases + inventory_packet_cases : 0)
            << ",\"inventory_cases\":" << inventory_cases << ",\"selection_cases\":" << selected_cases
            << ",\"endurance_cases\":" << updater_cases << ",\"equipment_cases\":" << equipment_cases
            << ",\"equipment_event_cases\":" << event_cases << ",\"packet_transport\":\"fixture_callback\""
            << ",\"inventory_event_cases\":" << inventory_event_cases << ",\"inventory_server_route\":\"fixture_callback\""
            << ",\"inventory_packet_cases\":" << inventory_packet_cases << ",\"inventory_packet_serializer\":\"native_stream_with_fixture_transport\""
            << ",\"sql_cases\":" << sql_cases << ",\"sql_execute_is_persistence_proof\":false"
            << ",\"save_conversion_cases\":" << save_cases << ",\"save_repair_cases\":" << save_repair_cases << ",\"save_disk_commit\":false"
            << ",\"save_dispatch_cases\":" << save_dispatch_cases << ",\"save_backend\":\"fixture_callbacks\""
            << ",\"save_file_cases\":" << save_file_cases << ",\"save_file_io\":\"private_callbacks\",\"flush_failure_can_report_success\":true"
            << ",\"save_encode_cases\":" << save_encode_cases << ",\"save_postprocess\":\"fixture_callback\",\"save_compression\":\"native_lz4\""
            << ",\"native_item_cases\":" << item_cases << ",\"item_allocator\":\"private_guarded_heap\""
            << ",\"dirty_slot_cases\":" << dirty_cases << ",\"dirty_slot_consumer\":\"native_with_fixture_persistence\",\"consumer_cases\":" << consumer_cases
            << ",\"field_writer_cases\":" << writer_cases << ",\"engine_commit\":false"
            << ",\"exe_version\":\"1.0.0.2949\",\"real_registry_srw\":" << (native ? "true" : "false")
            << ",\"real_owner_srw\":" << (native ? "true" : "false")
            << ",\"reader_layout\":\"2949_accessors_verified_host_unbound\""
            << ",\"thread_tracking_callbacks\":true,\"live_installable\":false}\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
