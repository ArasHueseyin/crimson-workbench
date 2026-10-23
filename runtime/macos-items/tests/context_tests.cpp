#include "../src/context.h"
#include <array>
#include <cassert>
#include <cstring>
#include <map>

int main() {
    using namespace crimson::mac_items;
    std::array<std::uintptr_t, 0x110 / 8> actor{};
    std::array<std::uintptr_t, 0xc0 / 8> component{};
    std::array<std::uintptr_t, 2> holder{};
    std::array<std::uintptr_t, 4> context{};
    actor[0] = 0x12340000;
    actor[0x108 / 8] = reinterpret_cast<std::uintptr_t>(component.data());
    component[0xb8 / 8] = reinterpret_cast<std::uintptr_t>(holder.data());
    holder[1] = reinterpret_cast<std::uintptr_t>(actor.data());
    context[0] = reinterpret_cast<std::uintptr_t>(actor.data());
    auto read = [&](std::uintptr_t address, std::uintptr_t& value) {
        for (auto region : {std::pair{actor.data(), sizeof(actor)},
                            std::pair{component.data(), sizeof(component)},
                            std::pair{holder.data(), sizeof(holder)},
                            std::pair{context.data(), sizeof(context)}}) {
            auto start = reinterpret_cast<std::uintptr_t>(region.first);
            if (address >= start && address - start <= region.second - sizeof(value)) {
                std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
                return true;
            }
        }
        return false;
    };
    auto inspect = [&] { return inspect_context(reinterpret_cast<std::uintptr_t>(context.data()), 0x12340000, read); };
    assert(inspect().actor_matches && inspect().holder_matches);
    holder[1] = 0;
    assert(inspect().actor_matches && !inspect().holder_matches);
    actor[0]++;
    assert(!inspect().actor_matches);
    context[0] = 1;
    assert(!inspect().actor_matches);
    assert(!inspect_context(0, 0x12340000, read).actor_matches);
    assert(!inspect_context(UINTPTR_MAX, 0x12340000, read).actor_matches);
    std::map<std::uintptr_t, std::uintptr_t> memory{
        {0x108a6a938, 0x1000}, {0x1010, 0x2000}, {0x2000, 0x10833e4e0},
        {0x2100, 0x3000}, {0x3000, 0x10833e2c0}, {0x3220, 0x4000},
        {0x4000, 0x10833cd88}, {0x4100, 42}, {0x5100, 42}
    };
    auto local_read = [&](std::uintptr_t address, std::uintptr_t& value) {
        auto entry = memory.find(address);
        if (entry == memory.end()) return false;
        value = entry->second;
        return true;
    };
    auto local = [&] { return inspect_local_actor(0x5000, 0, local_read); };
    assert(local().keys_match);
    const auto snapshot = inspect_local_actor(0, 0, local_read);
    assert(snapshot.selection_valid && snapshot.client_key == 42 && !snapshot.keys_match);
    auto unstable = [&](std::uintptr_t address, std::uintptr_t& value) {
        static unsigned reads = 0;
        if (!local_read(address, value)) return false;
        if (address == 0x108a6a938 && ++reads == 2) value++;
        return true;
    };
    assert(!inspect_local_actor(0, 0, unstable).selection_valid);
    memory[0x5100] = 43;
    assert(!local().keys_match);
    memory[0x5100] = 42;
    memory[0x3000]++;
    assert(!local().keys_match);
    memory[0x3000]--;
    memory.erase(0x3220);
    assert(!local().keys_match);
}
