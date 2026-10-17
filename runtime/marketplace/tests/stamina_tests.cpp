#include "../stamina/stamina_policy.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <unordered_map>

namespace {
using namespace crimson::marketplace::stamina;
constexpr Address base = 0x140000000;
struct Fixture {
    std::unordered_map<Address, unsigned char> bytes;
    Address watch{};
    unsigned watches{};
    template<class T> void put(Address at, T value) {
        const auto source = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) bytes[at + i] = source[i];
    }
    template<class T> T at(Address address) {
        if (address == watch && ++watches == 2) return T{};
        T result{};
        auto target = reinterpret_cast<unsigned char*>(&result);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            const auto found = bytes.find(address + i);
            if (found != bytes.end()) target[i] = found->second;
        }
        return result;
    }
    Address definitions = 0x100000, rows = 0x110000, row = 0x120000;
    Address cr = 0x200000, sr = 0x210000, cm = 0x220000, sm = 0x230000;
    Address client = 0x300000, server = 0x400000;
    Address bucket = 0x500000, nodes = 0x510000, node = 0x520000;
    Address ce{}, se{};
    void actor(Address address, Address type, Address status_type) {
        put(address, base + type);
        put(address + 0x60, std::uint32_t{0xa0100001});
        put(address + 0x96, std::uint8_t{1});
        put(address + 0x88, address + 0x1000);
        put(address + 0x1001, std::uint8_t{1});
        put(address + 0x68, address + 0x2000);
        put(address + 0x70, std::uint32_t{57});
        put(address + 0xa0, address + 0x3000);
        put(address + 0x30d0, address);
        put(address + 0x20b8, address + 0x4000);
        put(address + 0x4008, address);
        put(address + 0x2020, address + 0x5000);
        put(address + 0x5000, base + status_type);
        put(address + 0x5008, address);
        put(address + 0x5018, address + 0x6000);
        put(address + 0x6000, address + 0x5000);
        put(address + 0x6058, address + 0x7000);
        put(address + 0x6060, std::uint32_t{20});
    }
    Fixture() {
        put(base + kStatusInfoManager, definitions);
        put(definitions + 8, std::uint32_t{84});
        put(definitions + 0x58, rows);
        put(rows + 22 * 8, row);
        put(row, kStaminaKey);
        put(row + 0x14, std::uint32_t{22});
        put(row + 0x11, std::uint8_t{2});
        put(base + kClientRoot, cr); put(base + kServerRoot, sr);
        put(cr + 0x30, cm); put(sr + 0x48, sm);
        put(cm, base + kClientManager); put(sm, base + kServerManager);
        put(cm + 0x50, client);
        actor(client, kClientActor, kClientStatus);
        actor(server, kServerActor, kServerStatus);
        put(sm + 0x98, std::uint32_t{1});
        put(sm + 0xa8, bucket); put(sm + 0xb0, nodes);
        put(bucket, std::uint32_t{1}); put(bucket + 8, std::uint32_t{1});
        put(bucket + 12, std::uint32_t{3}); put(nodes + 3 * 8, node);
        put(node + 4, std::uint32_t{1}); put(node + 8, server);
        ce = client + 0x7000 + 12 * kEntrySize;
        se = server + 0x7000 + 12 * kEntrySize;
        put(ce, std::uint16_t{22}); put(se, std::uint16_t{22});
        put(ce + 8, std::int64_t{440000}); put(se + 8, std::int64_t{440000});
        put(ce + 0x18, std::int64_t{440000}); put(se + 0x18, std::int64_t{440000});
    }
};
unsigned forwarded{};
void* expected_entry{};
std::int64_t expected_target{};
std::uint64_t __fastcall original(void* entry, std::uint64_t time, std::int64_t target, std::uint16_t flag) {
    assert(entry == expected_entry && time == 987654321 && target == expected_target && flag == 0xfedc);
    ++forwarded;
    return 0x123456789abcdef0;
}
}

int main() {
    Fixture source;
    for (const auto entry : {source.ce, source.se}) {
        Fixture f = source;
        const auto before = f.bytes;
        assert(selected_stamina(f, base, entry));
        assert(protected_target(f, base, entry, 430000) == 440000);
        assert(protected_target(f, base, entry, 440000) == 440000);
        assert(protected_target(f, base, entry, 450000) == 450000);
        assert(f.bytes == before);
    }
    // The semantic key remains correct if another supported definition order
    // puts Stamina at a different resolved index. No ordinal is hard-coded.
    {
        Fixture f = source;
        f.put(f.ce, std::uint16_t{50}); f.put(f.rows + 50 * 8, f.row);
        f.put(f.row + 0x14, std::uint32_t{50});
        assert(protected_target(f, base, f.ce, 1) == 440000);
    }
    for (const auto key : {1000000u, 1000075u, 1000077u, 1000027u, 1000078u}) {
        Fixture f = source; f.put(f.row, key);
        assert(protected_target(f, base, f.ce, 1) == 1);
    }
    // Unknown/dead actors, NPC entries, stale generations, incomplete worlds,
    // ambiguous registries and broken ownership must all use the native value.
    for (unsigned test = 0; test < 23; ++test) {
        Fixture f = source;
        switch (test) {
        case 0: f.put(base + kClientRoot, Address{}); break;
        case 1: f.put(base + kServerRoot, Address{}); break;
        case 2: f.put(f.cm + 0x50, Address{}); break;
        case 3: f.put(f.server + 0x60, std::uint32_t{0xb0100001}); break;
        case 4: f.put(f.server + 0x96, std::uint8_t{}); break;
        case 5: f.put(f.client + 0x96, std::uint8_t{}); break;
        case 6: f.put(f.client + 0x1001, std::uint8_t{3}); break;
        case 7: f.put(f.client + 0x30d0, f.server); break;
        case 8: f.put(f.client + 0x5008, f.server); break;
        case 9: f.put(f.client + 0x6000, f.server); break;
        case 10: f.put(f.client + 0x6060, std::uint32_t{12}); break;
        case 11: f.put(f.client + 0x6060, std::uint32_t{4097}); break;
        case 12: f.put(f.sm + 0x98, std::uint32_t{}); break;
        case 13: f.put(f.bucket, std::uint32_t{32}); break;
        case 14: f.put(f.node + 4, std::uint32_t{2}); break;
        case 15: f.put(f.bucket + 12, std::uint32_t{1'000'000}); break;
        case 16: f.put(f.bucket, std::uint32_t{2}); f.put(f.bucket + 16, std::uint32_t{1}); break;
        case 17: f.put(f.row + 0x11, std::uint8_t{}); break;
        case 18: f.put(f.row + 0x14, std::uint32_t{17}); break;
        case 19: f.put(f.ce, std::uint16_t{84}); break;
        case 20: f.put(f.client, base + kServerActor); break;
        case 21: f.put(f.client + 0x5000, base + kServerStatus); break;
        case 22: f.watch = base + kClientRoot; break;
        }
        assert(protected_target(f, base, f.ce, 1) == 1);
    }
    {
        Fixture f = source;
        const auto npc = Address{0x900000};
        f.put(npc, std::uint16_t{22}); f.put(npc + 8, std::int64_t{440000});
        assert(protected_target(f, base, npc, 100) == 100);
        assert(!selected_stamina(f, base, f.ce + 1));
        f.put(f.ce + 8, std::int64_t{});
        assert(protected_target(f, base, f.ce, -1) == -1);
        f.put(f.client + 0x1001, std::uint8_t{4});
        assert(selected_stamina(f, base, f.ce));
    }
    // Native dispatch never owns a stat write. It always forwards time/flags,
    // return value and unrecognized entries, including guarded read failures.
    std::array<unsigned char, 0x90> local{};
    expected_entry = local.data(); expected_target = 500;
    assert(dispatch_commit(original, 0, expected_entry, 987654321, 500, 0xfedc) == 0x123456789abcdef0);
    expected_entry = reinterpret_cast<void*>(1); expected_target = -7;
    assert(dispatch_commit(original, base, expected_entry, 987654321, -7, 0xfedc) == 0x123456789abcdef0);
    assert(forwarded == 2);
    std::puts("PASS: selected client/server stamina only, semantic IDs, regeneration, bounds, stale/foreign actors and SEH forwarding. Fixture memory only; no game writes.");
}
