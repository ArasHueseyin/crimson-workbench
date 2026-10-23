#include "../src/execute.h"
#include <cassert>
#include <map>

using namespace crimson::mac_items;
void* construct_item(void* value, const std::uint16_t* type, std::int64_t quantity) {
    const std::int64_t uid = -1;
    std::memcpy(value, &uid, 8);
    std::memcpy(static_cast<char*>(value)+8, type, 2);
    std::memcpy(static_cast<char*>(value)+0x10, &quantity, 8);
    return value;
}
void* initialize_item(void* init) { return init; }
void serialize_item(const void* value, std::uint16_t slot, void* init) {
    assert(slot == 0xffff);
    std::memcpy(init, value, 8);
    const std::uint32_t key = 710001;
    std::memcpy(static_cast<char*>(init)+8, &key, 4);
    std::memcpy(static_cast<char*>(init)+0x10, static_cast<const char*>(value)+0x10, 8);
}
void* destroy_item(void* value) { return value; }
int main() {
    using namespace crimson::live_items;
    ItemBindings bindings{construct_item, initialize_item, serialize_item, destroy_item};
    std::map<std::uintptr_t, std::uintptr_t> memory{
        {0x108590c20,0x1000}, {0x1008,1}, {0x1058,0x2000}, {0x2000,0x3000},
        {0x3000,710001}, {0x3428,0}, {0x33f0,1},
        {0x1085cb7a0,0x4000}, {0x4008,1}, {0x4058,0x5000}, {0x5000,0x6000}, {0x6000,0xffff0002},
        {0x7018,0x8000}, {0x7020,1}, {0x8000,0x9000},
        {0x9000,0xa000}, {0x9008,1}, {0x9010,0}, {0xa008,0xffff}, {0xa010,0}
    };
    auto read = [&](std::uintptr_t address, std::uintptr_t& value) {
        auto entry = memory.find(address);
        if (entry == memory.end()) return false;
        value = entry->second;
        return true;
    };
    unsigned grants = 0;
    auto grant = [&](std::uintptr_t holder, const void* init) {
        assert(holder == 0x7000 && value_field<std::uint32_t>(init,8) == 710001);
        ++grants;
        memory[0xa008] = 0;
        memory[0xa010] += value_field<std::int64_t>(init,0x10);
        return 0U;
    };
    auto run = [&](std::uint32_t key, std::uint32_t quantity, bool context = true) {
        return execute_item(bindings, grant, [=] { return context; }, 0x7000, key, quantity, 0, read);
    };
    assert(run(710001,2).state == State::applied && grants == 1 && memory[0xa010] == 2);
    assert(run(710001,0).error == Error::invalid_request && grants == 1);
    assert(run(7,1).error == Error::unsupported_item && grants == 1);
    memory[0x6000] = 8;
    assert(run(710001,1).error == Error::special_storage && grants == 1);
    memory[0x6000] = 2;
    memory[0x33f0] = 0;
    assert(run(710001,101).error == Error::quantity_limit && grants == 1);
    assert(run(710001,1,false).error == Error::context && grants == 1);
    auto no_delta = [](std::uintptr_t, const void*) { return 0U; };
    const auto uncertain = execute_item(bindings, no_delta, [] { return true; }, 0x7000, 710001, 1, 0, read);
    assert(uncertain.state == State::uncertain && uncertain.error == Error::inventory_mismatch);
}
