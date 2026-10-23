#include "../src/inventory.h"
#include <cassert>
#include <map>

int main() {
    using namespace crimson::mac_items;
    std::map<std::uintptr_t, std::uintptr_t> memory{
        {0x1018, 0x2000}, {0x1020, 1}, {0x2000, 0x3000},
        {0x3000, 0x4000}, {0x3008, 2},
        {0x4008, 42}, {0x4010, 5}, {0x40d0, 43}, {0x40d8, 3}
    };
    auto read = [&](std::uintptr_t address, std::uintptr_t& word) {
        auto entry = memory.find(address);
        if (entry == memory.end()) return false;
        word = entry->second;
        return true;
    };
    auto count = [&](std::uint16_t type) { return count_items(0x1000, type, read); };
    assert(count(42).valid && count(42).quantity == 5);
    assert(count_items(0x1000, 42, read, 0).quantity == 5);
    assert(!count_items(0x1000, 42, read, 1).valid);
    assert(count(41).valid && count(41).quantity == 0);
    memory[0x40d0] = 42;
    assert(count(42).valid && count(42).quantity == 8);
    memory[0x40d8] = UINTPTR_MAX;
    assert(!count(42).valid);
    memory[0x1020] = 129;
    assert(!count(42).valid);
    memory[0x1020] = 1;
    memory[0x3008] = 16385;
    assert(!count(42).valid);
    memory[0x3008] = 65536;
    assert(!count(42).valid);
    memory[0x3008] = 1;
    memory[0x1020] = 2;
    memory[0x2008] = 0x5000;
    memory[0x5000] = 0x6000;
    memory[0x5008] = 1;
    memory[0x6008] = 42;
    memory[0x6010] = 85;
    assert(count(42).valid && count(42).quantity == 90);
    assert(count_items(0x1000, 42, read, 0).quantity == 5);
    assert(count_items(0x1000, 42, read, 1).quantity == 85);
    memory.erase(0x4008);
    assert(!count(42).valid);
    assert(!count_items(UINTPTR_MAX, 42, read).valid);
}
