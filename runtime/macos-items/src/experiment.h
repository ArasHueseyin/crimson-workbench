#pragma once
#include <cstdint>
#include <cstring>

namespace crimson::mac_items {
struct InventoryBaseline {
    std::int64_t timber{}, fine_timber{}, other_timber{}, other_fine_timber{};
    bool valid{true};
};
enum class ExperimentResult { rejected, verified, uncertain };

// shortcut: one approved Timber experiment only; use the existing request queue before exposing the Item Picker.
template<class Prepare, class Grant, class Observe>
ExperimentResult one_timber_experiment(Prepare prepare, Grant grant, Observe observe) {
    const auto before = observe();
    if (!before.valid || before.timber != 0 || before.fine_timber != 5
        || before.other_timber != 13 || before.other_fine_timber != 85) return ExperimentResult::rejected;
    alignas(16) unsigned char init[0x1c0]{};
    if (prepare(init) != init) return ExperimentResult::rejected;
    std::int64_t uid{}, quantity{};
    std::uint32_t key{};
    std::memcpy(&uid, init, sizeof(uid));
    std::memcpy(&key, init+8, sizeof(key));
    std::memcpy(&quantity, init+0x10, sizeof(quantity));
    if (uid != -1 || key != 710001 || quantity != 1) return ExperimentResult::rejected;
    if (grant(init) != 0) return ExperimentResult::uncertain;
    const auto after = observe();
    return after.valid && after.timber == 1 && after.fine_timber == 5
        && after.other_timber == 13 && after.other_fine_timber == 85
        ? ExperimentResult::verified : ExperimentResult::uncertain;
}
}
