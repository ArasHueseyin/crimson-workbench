#include "../stamina/stamina_policy.h"
#include "../spirit/resource_file.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unordered_map>

namespace {
using namespace crimson::marketplace;
using stamina::Address;
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
        put(base + stamina::kStatusInfoManager, definitions);
        put(definitions + 8, std::uint32_t{84}); put(definitions + 0x58, rows);
        put(rows + 23 * 8, row); put(row, spirit::kSpiritKey);
        put(row + 0x14, std::uint32_t{23}); put(row + 0x11, std::uint8_t{2});
        put(base + stamina::kClientRoot, cr); put(base + stamina::kServerRoot, sr);
        put(cr + 0x30, cm); put(sr + 0x48, sm);
        put(cm, base + stamina::kClientManager); put(sm, base + stamina::kServerManager);
        put(cm + 0x50, client);
        actor(client, stamina::kClientActor, stamina::kClientStatus);
        actor(server, stamina::kServerActor, stamina::kServerStatus);
        put(sm + 0x98, std::uint32_t{1}); put(sm + 0xa8, bucket); put(sm + 0xb0, nodes);
        put(bucket, std::uint32_t{1}); put(bucket + 8, std::uint32_t{1});
        put(bucket + 12, std::uint32_t{3}); put(nodes + 3 * 8, node);
        put(node + 4, std::uint32_t{1}); put(node + 8, server);
        ce = client + 0x7000 + 13 * stamina::kEntrySize;
        se = server + 0x7000 + 13 * stamina::kEntrySize;
        for (const auto entry : {ce, se}) {
            put(entry, std::uint16_t{23});
            put(entry + 8, std::int64_t{150000});
            put(entry + 0x18, std::int64_t{150000});
        }
    }
};
unsigned forwarded{};
void* expected_entry{};
std::int64_t expected_target{};
std::uint64_t __fastcall original(void* entry, std::uint64_t time, std::int64_t target, std::uint16_t flag) {
    assert(entry == expected_entry && time == 123456789 && target == expected_target && flag == 0xa13f);
    ++forwarded;
    return 0xfedcba9876543210;
}

void settings() {
    assert(resources::kLegacyDefault == resources::kStamina);
    std::array<unsigned char, 9> bytes{'C', 'W', 'R', '1', 0, 0, 0, 0, 0};
    for (unsigned flags = 0; flags < 256; ++flags) {
        bytes[4] = static_cast<unsigned char>(flags);
        assert(resources::setting(bytes.data(), 8) == (flags <= 3 ? flags : 0));
    }
    bytes[4] = 3;
    for (std::size_t length = 0; length <= 9; ++length)
        assert(resources::setting(bytes.data(), length) == (length == 8 ? 3u : 0u));
    assert(resources::setting(nullptr, 8) == 0);
    for (unsigned i = 0; i < 4; ++i) {
        const auto saved = bytes[i]; bytes[i] = 'X';
        assert(resources::setting(bytes.data(), 8) == 0); bytes[i] = saved;
    }
    for (unsigned i = 5; i < 8; ++i) {
        bytes[i] = 1; assert(resources::setting(bytes.data(), 8) == 0); bytes[i] = 0;
    }
}

void owned_setting_files() {
    wchar_t temp[MAX_PATH]{};
    assert(GetTempPathW(MAX_PATH, temp));
    const auto folder = std::filesystem::path(temp) /
        (L"crimson-resource-fixture-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    assert(!std::filesystem::exists(folder));
    assert(resources::load_setting_from_directory(folder.wstring()) == 1);
    assert(std::filesystem::create_directory(folder));
    assert(resources::load_setting_from_directory(folder.wstring()) == 1);
    const auto path = folder / L"player-resources.dat";
    const std::array<char, 8> bytes{'C', 'W', 'R', '1', 3, 0, 0, 0};
    {
        std::ofstream output(path, std::ios::binary); output.write(bytes.data(), bytes.size());
    }
    assert(resources::load_setting_from_directory(folder.wstring()) == 3);
    HANDLE held = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(held != INVALID_HANDLE_VALUE);
    assert(resources::load_setting_from_directory(folder.wstring()) == 0);
    CloseHandle(held);
    const auto link = folder / L"second-link.dat";
    assert(CreateHardLinkW(link.c_str(), path.c_str(), nullptr));
    assert(resources::load_setting_from_directory(folder.wstring()) == 0);
    assert(std::filesystem::remove(link));
    assert(resources::load_setting_from_directory(folder.wstring()) == 3);
    {
        std::ofstream output(path, std::ios::binary | std::ios::app); output.put('X');
    }
    assert(resources::load_setting_from_directory(folder.wstring()) == 0);
    assert(std::filesystem::remove(path));
    assert(std::filesystem::create_directory(path));
    assert(resources::load_setting_from_directory(folder.wstring()) == 0);
    assert(std::filesystem::remove(path));
    assert(std::filesystem::remove(folder));
}

void positive_and_coexistence() {
    for (const bool server : {false, true}) {
        for (const auto resource : {stamina::kStaminaKey, spirit::kSpiritKey}) {
            for (unsigned flags = 0; flags < 5; ++flags) {
                Fixture f;
                f.put(f.row, resource);
                const auto entry = server ? f.se : f.ce;
                const auto before = f.bytes;
                const auto bit = resource == spirit::kSpiritKey ? resources::kSpirit : resources::kStamina;
                const auto protected_value = flags <= 3 && (flags & bit) ? 150000 : 145000;
                assert(stamina::configured_target(f, base, entry, 145000, flags) == protected_value);
                assert(stamina::configured_target(f, base, entry, 150000, flags) == 150000);
                assert(stamina::configured_target(f, base, entry, 170000, flags) == 170000);
                assert(f.bytes == before);
            }
        }
    }
    Fixture f;
    f.put(f.ce, std::uint16_t{50}); f.put(f.rows + 50 * 8, f.row);
    f.put(f.row + 0x14, std::uint32_t{50});
    assert(spirit::selected_spirit(f, base, f.ce));
    assert(spirit::protected_target(f, base, f.ce, 145000) == 150000);
    f.put(f.client + 0x1001, std::uint8_t{4});
    assert(spirit::selected_spirit(f, base, f.ce));
}

void scope_and_bounds() {
    for (const auto key : {1000000u, 1000075u, 1000077u, 1000026u, 1000078u, 0u}) {
        Fixture f; f.put(f.row, key);
        assert(spirit::protected_target(f, base, f.ce, 145000) == 145000);
    }
    for (unsigned test = 0; test < 33; ++test) {
        Fixture f;
        switch (test) {
        case 0: f.put(base + stamina::kClientRoot, Address{}); break;
        case 1: f.put(base + stamina::kServerRoot, Address{}); break;
        case 2: f.put(f.cm + 0x50, Address{}); break;
        case 3: f.put(f.server + 0x60, std::uint32_t{0xb0100001}); break;
        case 4: f.put(f.server + 0x96, std::uint8_t{}); break;
        case 5: f.put(f.client + 0x96, std::uint8_t{}); break;
        case 6: f.put(f.client + 0x1001, std::uint8_t{3}); break;
        case 7: f.put(f.client + 0x30d0, f.server); break;
        case 8: f.put(f.client + 0x5008, f.server); break;
        case 9: f.put(f.client + 0x6000, f.server); break;
        case 10: f.put(f.client + 0x6060, std::uint32_t{13}); break;
        case 11: f.put(f.client + 0x6060, std::uint32_t{4097}); break;
        case 12: f.put(f.sm + 0x98, std::uint32_t{}); break;
        case 13: f.put(f.bucket, std::uint32_t{32}); break;
        case 14: f.put(f.node + 4, std::uint32_t{2}); break;
        case 15: f.put(f.bucket + 12, std::uint32_t{1'000'000}); break;
        case 16: f.put(f.bucket, std::uint32_t{2}); f.put(f.bucket + 16, std::uint32_t{1}); break;
        case 17: f.put(f.row + 0x11, std::uint8_t{1}); break;
        case 18: f.put(f.row + 0x14, std::uint32_t{22}); break;
        case 19: f.put(f.ce, std::uint16_t{84}); break;
        case 20: f.put(f.client, base + stamina::kServerActor); break;
        case 21: f.put(f.client + 0x5000, base + stamina::kServerStatus); break;
        case 22: f.watch = base + stamina::kClientRoot; break;
        case 23: f.put(f.row + 0x10, std::uint8_t{1}); break;
        case 24: f.put(f.ce + 0x18, std::int64_t{140000}); break;
        case 25: f.put(f.ce + 0x28, std::int64_t{-1}); break;
        case 26: f.put(f.ce + 0x18, (std::numeric_limits<std::int64_t>::max)()); f.put(f.ce + 0x20, std::int64_t{1}); break;
        case 27: f.put(f.ce + 0x18, (std::numeric_limits<std::int64_t>::min)()); f.put(f.ce + 0x20, std::int64_t{-1}); break;
        case 28: f.put(f.ce + 0x28, std::int64_t{150001}); break;
        case 29: f.watch = f.ce + 8; break;
        case 30: f.watch = f.ce + 0x18; break;
        case 31: f.watch = f.row; break;
        case 32: f.watch = f.row + 0x11; break;
        }
        assert(spirit::protected_target(f, base, f.ce, 145000) == 145000);
    }
    Fixture f;
    f.put(f.ce + 0x18, std::int64_t{100000}); f.put(f.ce + 0x20, std::int64_t{50000});
    assert(spirit::protected_target(f, base, f.ce, 145000) == 150000);
    for (const auto amount : {std::int64_t{0}, std::int64_t{-1}}) {
        f.put(f.ce + 8, amount);
        assert(spirit::protected_target(f, base, f.ce, -5000) == -5000);
    }
    const auto npc = Address{0x900000};
    f.put(npc, std::uint16_t{23}); f.put(npc + 8, std::int64_t{150000});
    f.put(npc + 0x18, std::int64_t{150000});
    assert(spirit::protected_target(f, base, npc, 145000) == 145000);
    assert(!spirit::selected_spirit(f, base, f.ce + 1));
    assert(!stamina::selected_resource(f, base, f.ce, 1000000));
}

void native_forwarding() {
    std::array<unsigned char, 0x90> local{};
    for (const auto flags : {0u, 1u, 2u, 3u, 4u}) {
        expected_entry = local.data(); expected_target = 500;
        assert(stamina::dispatch_commit(original, 0, expected_entry, 123456789, 500, 0xa13f, flags) == 0xfedcba9876543210);
        expected_entry = reinterpret_cast<void*>(1); expected_target = -7;
        assert(stamina::dispatch_commit(original, base, expected_entry, 123456789, -7, 0xa13f, flags) == 0xfedcba9876543210);
    }
    assert(forwarded == 10);
}
} // namespace

int main() {
    settings(); owned_setting_files(); positive_and_coexistence(); scope_and_bounds(); native_forwarding();
    std::puts("PASS: independent stamina/spirit flags; semantic controlled-client/server resource ownership; native bounds, grants/regen/zero, stale/foreign state and SEH forwarding. Owned fixtures only.");
}
