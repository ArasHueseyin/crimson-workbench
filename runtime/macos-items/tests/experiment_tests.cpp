#include "../src/experiment.h"
#include <cassert>
#include <cstring>

using namespace crimson::mac_items;
int main() {
    InventoryBaseline inventory{0, 5, 13, 85};
    unsigned calls = 0;
    auto observe = [&] { return inventory; };
    auto construct = [](void* init) -> void* {
        const std::int64_t uid = -1;
        const std::uint32_t key = 710001;
        const std::int64_t quantity = 1;
        std::memcpy(init, &uid, sizeof(uid));
        std::memcpy(static_cast<char*>(init)+8, &key, sizeof(key));
        std::memcpy(static_cast<char*>(init)+0x10, &quantity, sizeof(quantity));
        return init;
    };
    auto grant = [&](const void* init) {
        std::uint32_t key{};
        std::int64_t quantity{};
        std::memcpy(&key, static_cast<const char*>(init)+8, sizeof(key));
        std::memcpy(&quantity, static_cast<const char*>(init)+0x10, sizeof(quantity));
        assert(key == 710001 && quantity == 1);
        ++calls;
        ++inventory.timber;
        return 0U;
    };
    assert(one_timber_experiment(construct, grant, observe) == ExperimentResult::verified);
    assert(calls == 1 && inventory.timber == 1 && inventory.fine_timber == 5);
    assert(one_timber_experiment(construct, grant, observe) == ExperimentResult::rejected);
    assert(calls == 1);
    inventory = {0, 5, 13, 85};
    auto uncertain = [&](const void*) { ++calls; return 7U; };
    assert(one_timber_experiment(construct, uncertain, observe) == ExperimentResult::uncertain);
    auto no_delta = [](const void*) { return 0U; };
    assert(one_timber_experiment(construct, no_delta, observe) == ExperimentResult::uncertain);
    auto bad_construct = [](void* init) { return init; };
    assert(one_timber_experiment(bad_construct, grant, observe) == ExperimentResult::rejected);
    inventory.valid = false;
    assert(one_timber_experiment(construct, grant, observe) == ExperimentResult::rejected);
}
