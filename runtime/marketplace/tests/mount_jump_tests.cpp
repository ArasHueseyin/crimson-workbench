#include "../jump/mount_jump_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>

namespace {
using namespace crimson::marketplace::jump;
template<class T> void put(void* output, std::size_t offset, T value) {
    std::memcpy(static_cast<unsigned char*>(output) + offset, &value, sizeof(value));
}
template<class T> T get(const void* input, std::size_t offset) {
    T value{}; std::memcpy(&value, static_cast<const unsigned char*>(input) + offset, sizeof(value)); return value;
}
struct Fixture {
    Address base{};
    std::array<unsigned char, 0x100> actor{};
    std::array<unsigned char, 2> metadata{};
    std::array<unsigned char, 0x48> components{};
    std::array<unsigned char, 0x40> status{};
    std::array<unsigned char, 0xd0> control{};
    std::array<unsigned char, 0x70> state{};
    std::array<unsigned char, 0x60> definitions{};
    std::array<Address, 48> rows{};
    std::uint32_t key = kLion;
    std::array<unsigned char, 16> event{};
    std::array<unsigned char, 26> data{};
    Address actor_reference{};
    Fixture() {
        // Reserve address space for the native global's relative address; only
        // its one owned fixture page is committed. No process is opened.
        base = reinterpret_cast<Address>(VirtualAlloc(nullptr, 0x6d6a000, MEM_RESERVE, PAGE_NOACCESS));
        assert(base);
        assert(VirtualAlloc(reinterpret_cast<void*>(base + 0x6d69000), 4096, MEM_COMMIT, PAGE_READWRITE));
        put(reinterpret_cast<void*>(base + kCharacterInfoManager), 0, definitions.data());
        put(definitions.data(), 8, std::uint32_t{48});
        put(definitions.data(), 0x58, rows.data());
        rows[7] = reinterpret_cast<Address>(&key);
        put(actor.data(), 0x60, std::uint32_t{0xa0101234});
        put(actor.data(), 0x68, components.data());
        put(actor.data(), 0x70, std::uint32_t{9});
        put(actor.data(), 0x88, metadata.data());
        put(actor.data(), 0x96, std::uint8_t{1});
        metadata[1] = 6;
        put(components.data(), 0x20, status.data());
        put(components.data(), 0x40, control.data());
        put(status.data(), 8, actor.data());
        put(status.data(), 0x30, std::uint16_t{7});
        put(control.data(), 8, actor.data());
        put(control.data(), 0xc8, state.data());
        put(state.data(), 0x68, actor.data());
        put(event.data(), 0, std::uint64_t{0x123456789abcdef0});
        put(event.data(), 8, data.data());
        for (unsigned i = 0; i < data.size(); ++i) data[i] = static_cast<unsigned char>(i + 1);
        put(data.data(), 12, 2.5f);
        actor_reference = reinterpret_cast<Address>(actor.data());
    }
    ~Fixture() { assert(VirtualFree(reinterpret_cast<void*>(base), 0, MEM_RELEASE)); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
};
Fixture* expected{};
bool modified{};
unsigned calls{};
void* __fastcall original(void* self, void* result, const void* event, std::uintptr_t time, const void* actor) {
    assert(self == expected && result == &calls && actor == &expected->actor_reference);
    assert(time == 0xfedcba9876543210);
    if (modified) {
        assert(event != expected->event.data());
        assert(get<std::uint64_t>(event, 0) == get<std::uint64_t>(expected->event.data(), 0));
        const auto data = get<const unsigned char*>(event, 8);
        const float height = get<float>(data, 12);
        assert(std::abs(height - 3.5f) < 0.00001f);
        for (std::size_t i = 0; i < expected->data.size(); ++i)
            if (i < 12 || i >= 16) assert(data[i] == expected->data[i]);
    } else assert(event == expected->event.data());
    ++calls;
    return result;
}
void invoke(Fixture& f, bool change) {
    expected = &f; modified = change;
    const auto before = f.data;
    const auto event = f.event;
    assert(dispatch(original, f.base, &f, &calls, f.event.data(), 0xfedcba9876543210, &f.actor_reference) == &calls);
    assert(f.data == before && f.event == event);
}
struct ChangingRead : NativeRead {
    Address watched{};
    unsigned reads{};
    template<class T> T at(Address address) {
        if (address == watched && ++reads == 2) return T{};
        return NativeRead::at<T>(address);
    }
};
} // namespace

int main() {
    {
        Fixture f;
        for (auto key : {kLion, kRokade}) {
            f.key = key;
            for (auto kind : {1, 3, 4, 5, 6, 8}) {
                f.metadata[1] = static_cast<unsigned char>(kind);
                invoke(f, true); invoke(f, true); // never compounds original data
            }
        }
        for (auto key : {0u, 1u, 4u, 6u, 31377u, 30021u, 1002317u}) {
            f.key = key; invoke(f, false);
        }
        f.key = kRokade;
        f.rows[37] = reinterpret_cast<Address>(&f.key);
        put(f.status.data(), 0x30, std::uint16_t{37});
        invoke(f, true);
        for (const float height : {0.f, -1.f, std::numeric_limits<float>::infinity(),
                                  -std::numeric_limits<float>::infinity(),
                                  std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
            put(f.data.data(), 12, height); invoke(f, false);
        }
    }
    for (unsigned test = 0; test < 14; ++test) {
        Fixture f;
        switch (test) {
        case 0: f.actor_reference = 0; break;
        case 1: put(f.actor.data(), 0x96, std::uint8_t{}); break;
        case 2: f.metadata[1] = 2; break;
        case 3: put(f.actor.data(), 0x70, std::uint32_t{8}); break;
        case 4: put(f.actor.data(), 0x70, std::uint32_t{58}); break;
        case 5: put(f.status.data(), 8, Address{}); break;
        case 6: put(f.control.data(), 8, Address{}); break;
        case 7: put(f.state.data(), 0x68, Address{}); break;
        case 8: put(f.status.data(), 0x30, std::uint16_t{48}); break;
        case 9: put(f.definitions.data(), 8, std::uint32_t{100001}); break;
        case 10: f.rows[7] = 0; break;
        case 11: put(f.event.data(), 8, Address{}); break;
        case 12: put(f.event.data(), 8, Address{1}); break; // SEH read failure
        case 13: put(f.actor.data(), 0x60, std::uint32_t{}); break;
        }
        invoke(f, false);
    }
    {
        Fixture f; ChangingRead read;
        read.watched = reinterpret_cast<Address>(&f.actor_reference);
        Prepared prepared;
        assert(!prepare(read, f.base, reinterpret_cast<Address>(f.event.data()), reinterpret_cast<Address>(&f.actor_reference), prepared));
    }
    // The height input reaches native sqrt(2*g*h); the requested 40% increase
    // means x1.40 height, rather than x1.40 launch velocity (which yields x1.96).
    const float gravity = 9.81f, original_height = 2.5f;
    const float speed = std::sqrt(2 * gravity * original_height * kMountHeightMultiplier);
    assert(std::abs(speed * speed / (2 * gravity) / original_height - 1.4f) < 0.00001f);
    std::printf("PASS: %u native dispatch checks; both mount keys, all admitted actor kinds, unrelated actors, malformed/transient data, non-compounding 40%% height and original arguments preserved. Own fixtures only.\n", calls);
}
