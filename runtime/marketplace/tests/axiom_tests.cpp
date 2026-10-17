#include "../axiom/axiom_policy.h"
#include "../axiom/axiom_targets.h"
#include <Windows.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <map>
#include <limits>
#include <vector>

namespace {
using namespace crimson::marketplace::axiom;
struct SimMemory {
    Address base = 0x100000000, root = 0x200000000, manager = 0x200001000,
        actor = 0x200002000, possessor = 0x200003000;
    std::map<Address, std::uint64_t> values;
    Address unreadable{};
    unsigned reads = 0, writes = 0, fail_write = 0;
    bool foreign_edit = false, edit_after_commit = false;
    SimMemory() {
        values[base + kCrtState] = 2; values[base + kClientRoot] = root;
        values[root + 0x30] = manager; values[manager] = base + kClientManager;
        values[manager + 0x50] = actor; values[actor] = base + kClientActor;
        values[actor + 0x60] = 17; values[actor + 0x96] = 1;
        values[actor + 0xa0] = possessor; values[possessor + 0xd0] = actor;
        for (const auto& tuning : kTuning) {
            values[base + tuning.record] = base + kFloatTuningType;
            values[base + tuning.record + 0x28] = tuning.key;
            values[base + tuning.record + 0x38] = bits(tuning.normal);
            values[base + tuning.record + 0x3c] = bits(tuning.normal);
            values[base + tuning.record + 0x40] = 0xfeedbeef;
        }
    }
    template<class T> bool read(Address address, T& out) {
        ++reads;
        if (address == unreadable || values.find(address) == values.end()) return false;
        const auto value = values.at(address); std::memcpy(&out, &value, sizeof(out)); return true;
    }
    bool compare_exchange(Address address, std::uint32_t before, std::uint32_t after) {
        ++writes;
        if (writes == fail_write) {
            if (foreign_edit) values[base + kTuning[0].record + 0x38] = bits(99.0f);
            return false;
        }
        if (values[address] != before) return false;
        values[address] = after;
        if (edit_after_commit && writes == 4) values[base + kTuning[1].record + 0x38] = bits(93.0f);
        return true;
    }
    void originals() {
        for (const auto& tuning : kTuning) {
            assert(values.at(base + tuning.record + 0x38) == bits(tuning.normal));
            assert(values.at(base + tuning.record + 0x3c) == bits(tuning.normal));
            assert(values.at(base + tuning.record + 0x40) == 0xfeedbeef);
        }
    }
};
void settings() {
    std::array<unsigned char, 9> bytes{'C','W','A','1',0,0,0,0,0};
    assert(setting(nullptr, 8) == 100);
    for (unsigned value = 0; value <= 1000; ++value) {
        bytes[4] = static_cast<unsigned char>(value); bytes[5] = static_cast<unsigned char>(value >> 8);
        assert(setting(bytes.data(), 8) == (valid_percent(value) ? value : 100));
    }
    bytes[4] = 0xf4; bytes[5] = 1; bytes[6] = 0; bytes[7] = 0;
    assert(setting(bytes.data(), 8) == 500);
    for (std::size_t count = 0; count < 12; ++count)
        if (count != 8) assert(setting(bytes.data(), count) == 100);
    bytes[0] = 'X'; assert(setting(bytes.data(), 8) == 100); bytes[0] = 'C';
    bytes[7] = 1; assert(setting(bytes.data(), 8) == 100);
}
void success_and_scope() {
    SimMemory normal; assert(apply(normal, normal.base, 100) == Result::Normal);
    assert(normal.reads == 0 && normal.writes == 0); normal.originals();
    for (unsigned percent : {200u, 210u, 333u, 500u}) {
        SimMemory memory;
        assert(apply(memory, memory.base, percent) == Result::Applied && memory.writes == 4);
        for (const auto& tuning : kTuning) {
            assert(memory.values.at(memory.base + tuning.record + 0x38) ==
                bits(tuning.normal * (static_cast<float>(percent) / 100.0f)));
            assert(memory.values.at(memory.base + tuning.record + 0x3c) == bits(tuning.normal));
            assert(memory.values.at(memory.base + tuning.record + 0x40) == 0xfeedbeef);
        }
        // A second initialization cannot scale the already changed cells again.
        assert(apply(memory, memory.base, percent) == Result::Conflict && memory.writes == 4);
    }
    for (unsigned value : {0u, 99u, 101u, 199u, 501u, 0xffffffffu}) {
        SimMemory memory; assert(apply(memory, memory.base, value) == Result::Conflict);
        assert(memory.writes == 0); memory.originals();
    }
    for (unsigned field = 0; field < 10; ++field) {
        SimMemory memory;
        const std::array<Address, 10> addresses{{memory.base + kCrtState, memory.base + kClientRoot,
            memory.root + 0x30, memory.manager, memory.manager + 0x50, memory.actor,
            memory.actor + 0x60, memory.actor + 0x96, memory.actor + 0xa0, memory.possessor + 0xd0}};
        memory.unreadable = addresses[field];
        assert(apply(memory, memory.base, 200) == Result::Waiting && memory.writes == 0);
        memory.unreadable = 0;
        assert(apply(memory, memory.base, 200) == Result::Applied);
    }
    SimMemory late;
    late.values[late.base + kCrtState] = 1;
    for (unsigned wait = 0; wait < 1000; ++wait)
        assert(apply(late, late.base, 200) == Result::Waiting && late.writes == 0);
    late.values[late.base + kCrtState] = 2;
    assert(apply(late, late.base, 200) == Result::Applied);
}
void conflicts_and_rollback() {
    for (const auto& tuning : kTuning) {
        for (Address field : {Address{0}, Address{0x28}, Address{0x38}, Address{0x3c}}) {
            SimMemory memory; const auto address = memory.base + tuning.record + field;
            memory.values[address] ^= 1;
            const auto before = memory.values;
            assert(apply(memory, memory.base, 200) == Result::Conflict && memory.writes == 0);
            assert(memory.values == before);
        }
        SimMemory unreadable; unreadable.unreadable = unreadable.base + tuning.record + 0x38;
        assert(apply(unreadable, unreadable.base, 200) == Result::Conflict && unreadable.writes == 0);
    }
    for (unsigned failure = 1; failure <= 4; ++failure) {
        SimMemory memory; memory.fail_write = failure;
        assert(apply(memory, memory.base, 500) == Result::RolledBack);
        memory.originals(); assert(memory.writes == failure + failure - 1);
    }
    SimMemory concurrent; concurrent.fail_write = 3; concurrent.foreign_edit = true;
    assert(apply(concurrent, concurrent.base, 200) == Result::RollbackConflict);
    assert(concurrent.values.at(concurrent.base + kTuning[0].record + 0x38) == bits(99.0f));
    for (std::size_t i = 1; i < kTuning.size(); ++i)
        assert(concurrent.values.at(concurrent.base + kTuning[i].record + 0x38) == bits(kTuning[i].normal));
    SimMemory postflight; postflight.edit_after_commit = true;
    assert(apply(postflight, postflight.base, 200) == Result::RollbackConflict);
    assert(postflight.values.at(postflight.base + kTuning[1].record + 0x38) == bits(93.0f));
    for (std::size_t i : {0u, 2u, 3u})
        assert(postflight.values.at(postflight.base + kTuning[i].record + 0x38) == bits(kTuning[i].normal));
}

template<class T> void put(Address address, T value) { std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(value)); }
struct OwnedMemory {
    template<class T> bool read(Address address, T& out) {
        std::memcpy(&out, reinterpret_cast<const void*>(address), sizeof(out)); return true;
    }
    bool compare_exchange(Address address, std::uint32_t before, std::uint32_t after) {
        return static_cast<std::uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address), after, before)) == before;
    }
};
void native_consumers() {
    const Address base = reinterpret_cast<Address>(VirtualAlloc(nullptr, 0x6d6a000, MEM_RESERVE, PAGE_NOACCESS));
    assert(base);
    for (Address page : {Address{0x1000}, Address{0x6c88000}, Address{0x6c97000}, Address{0x6d69000}})
        assert(VirtualAlloc(reinterpret_cast<void*>(base + page), 4096, MEM_COMMIT, PAGE_READWRITE));
    std::array<unsigned char, 0x38> root{}; std::array<unsigned char, 0x58> manager{};
    std::array<unsigned char, 0xb0> actor{}; std::array<unsigned char, 0xd8> possessor{};
    put(base + kCrtState, std::uint32_t{2}); put(base + kClientRoot, reinterpret_cast<Address>(root.data()));
    put(reinterpret_cast<Address>(root.data()) + 0x30, reinterpret_cast<Address>(manager.data()));
    put(reinterpret_cast<Address>(manager.data()), base + kClientManager);
    put(reinterpret_cast<Address>(manager.data()) + 0x50, reinterpret_cast<Address>(actor.data()));
    put(reinterpret_cast<Address>(actor.data()), base + kClientActor);
    put(reinterpret_cast<Address>(actor.data()) + 0x60, std::uint32_t{7});
    put(reinterpret_cast<Address>(actor.data()) + 0x96, std::uint8_t{1});
    put(reinterpret_cast<Address>(actor.data()) + 0xa0, reinterpret_cast<Address>(possessor.data()));
    put(reinterpret_cast<Address>(possessor.data()) + 0xd0, reinterpret_cast<Address>(actor.data()));
    for (const auto& tuning : kTuning) {
        put(base + tuning.record, base + kFloatTuningType); put(base + tuning.record + 0x28, tuning.key);
        put(base + tuning.record + 0x38, tuning.normal); put(base + tuning.record + 0x3c, tuning.normal);
    }
    const auto code = base + 0x1000;
    // Actual 2976 acquisition vcomiss from 58fe86. Only its RIP displacement is
    // relocated to owned tuning memory; the wrapper adapts the x64 input ABI.
    std::vector<unsigned char> acquire{0xc5,0xf8,0x29,0x74,0x24,0x08,0xc5,0xf8,0x28,0xf0};
    acquire.insert(acquire.end(), kPin15, kPin15 + 8);
    const auto distance_cell = base + kTuning[0].record + 0x38;
    const auto acquire_disp = static_cast<std::int32_t>(distance_cell - (code + 18));
    std::memcpy(acquire.data() + 14, &acquire_disp, 4);
    for (auto byte : {0x0f,0x96,0xc0,0xc5,0xf8,0x28,0x74,0x24,0x08,0xc3}) acquire.push_back(static_cast<unsigned char>(byte));
    std::memcpy(reinterpret_cast<void*>(code), acquire.data(), acquire.size());
    // Actual 2976 held-object range vminss from1180270, same owned relocation.
    const auto clamp_code = code + 64;
    std::vector<unsigned char> clamp{0xc5,0xf8,0x28,0xd8};
    clamp.insert(clamp.end(), kPin18, kPin18 + 8);
    const auto clamp_disp = static_cast<std::int32_t>(distance_cell - (clamp_code + 12));
    std::memcpy(clamp.data() + 8, &clamp_disp, 4); clamp.push_back(0xc3);
    std::memcpy(reinterpret_cast<void*>(clamp_code), clamp.data(), clamp.size());
    DWORD before{}; assert(VirtualProtect(reinterpret_cast<void*>(code), 4096, PAGE_EXECUTE_READ, &before));
    assert(FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(code), 4096));
    const auto can_reach = reinterpret_cast<bool(__fastcall*)(float)>(code);
    const auto held_clamp = reinterpret_cast<float(__fastcall*)(float)>(clamp_code);
    assert(can_reach(20.0f) && !can_reach(20.01f)); assert(held_clamp(80.0f) == 20.0f);
    OwnedMemory memory; assert(apply(memory, base, 500) == Result::Applied);
    assert(can_reach(100.0f) && !can_reach(100.01f)); assert(held_clamp(80.0f) == 80.0f);
    assert(held_clamp(120.0f) == 100.0f);
    assert(VirtualFree(reinterpret_cast<void*>(base), 0, MEM_RELEASE));
}
} // namespace
int main() {
    settings(); success_and_scope(); conflicts_and_rollback(); native_consumers();
    std::puts("PASS: Axiom settings, delayed world initialization, four-cell transaction/rollback, conflict preservation and copied native acquisition/drag consumers.");
}
