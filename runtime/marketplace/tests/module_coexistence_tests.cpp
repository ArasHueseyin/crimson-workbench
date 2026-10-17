#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "../mount-protection/mount_protection_targets.h"
#include "../consumables/consumables_targets.h"
#include "../stamina/stamina_targets.h"
#include "../jump/mount_jump_targets.h"
#include "../jump/player_jump_targets.h"
#include "../speed/player_speed_targets.h"
#include "../crafting/crafting_hooks.h"
#include "../force-palm/force_palm_targets.h"
#include "../force-palm-height/force_palm_height_targets.h"
#include "../axiom/axiom_targets.h"
#include "../glide/glide_targets.h"
#include "../dismount/mount_dismount_targets.h"
#include "../auto-loot/auto_loot_targets.h"
#include "../world-controls/world_native_targets.h"
#include "../photo-mode/photo_targets.h"
#include "../teleport/teleport_targets.h"
#include "../teleport/marker_targets.h"

namespace blackstar {
#include "../blackstar/targets.h"
#include "../blackstar/flight_targets.h"
#include "../blackstar/request_targets.h"
}
namespace fall {
#include "../fall/fall_targets.h"
}
namespace sleep_duration {
#include "../sleep/duration_targets.h"
}
namespace sockets {
#include "../../extra-sockets/src/pins.h"
}

namespace {
namespace mount = crimson::marketplace::mount_protection;
namespace consumables = crimson::marketplace::consumables;
namespace crafting = crimson::marketplace::crafting;
namespace force_palm = crimson::marketplace::force_palm;
namespace palm_height = crimson::marketplace::force_palm_height;
namespace axiom = crimson::marketplace::axiom;
namespace glide = crimson::marketplace::glide;
namespace resources = crimson::marketplace::stamina;
namespace dismount = crimson::marketplace::dismount;
namespace auto_loot = crimson::marketplace::auto_loot;
namespace world = crimson::marketplace::world_controls;
namespace photo = crimson::marketplace::photo;
namespace teleport = crimson::marketplace::teleport;
enum class Module { other, mount, consumables, crafting, force_palm, palm_height, axiom, glide, resources, dismount, auto_loot, world, photo, teleport };
struct Hook { Module module; std::uintptr_t rva; };
// Actual entry hooks from the packaged modules. Shared read-only witnesses are
// deliberately excluded: checking those witnesses in several modules is valid.
constexpr Hook hooks[] = {
    {Module::other, blackstar::kLoaderRva},
    {Module::other, blackstar::kResolverRva},
    {Module::other, blackstar::kSummonRva},
    {Module::other, blackstar::kRegionRva},
    {Module::other, blackstar::kTownRva},
    {Module::other, blackstar::kFlightRegionRva},
    {Module::other, blackstar::kNavigationGateRva},
    {Module::other, blackstar::kVoxelGateRva},
    {Module::other, blackstar::kInitialSummonRva},
    {Module::other, blackstar::kGimmickResolverRva},
    {Module::other, blackstar::kFrameSummonRva},
    {Module::other, blackstar::kReservedFrameSummonRva},
    {Module::other, blackstar::kCommonInitialRva},
    {Module::other, blackstar::kErrorDisplayRva},
    {Module::other, fall::kFallRva},
    {Module::other, sleep_duration::kEventRva},
    {Module::other, sleep_duration::kTargetRva},
    {Module::resources, crimson::marketplace::stamina::kCommitRva},
    {Module::other, crimson::marketplace::jump::kEventRva},
    {Module::other, crimson::marketplace::player_jump::kConsumeRva},
    {Module::other, crimson::marketplace::player_speed::kStepRva},
    {Module::other, sockets::kCtorRva},
    {Module::other, sockets::kWriterRva},
    {Module::other, sockets::kConvertRva},
    // SleepCooldown and LiveItems currently keep these entries in their .cpp.
    {Module::other, 0x17e6a60},
    {Module::other, 0x17e6b30},
    {Module::other, 0x2774b20},
    {Module::mount, mount::kDeltaRva},
    {Module::consumables, consumables::kUseRva},
    {Module::consumables, consumables::kLookupRva},
    {Module::consumables, consumables::kPopRva},
    {Module::crafting, crafting::kTargets[0].hook},
    {Module::crafting, crafting::kTargets[1].hook},
    {Module::crafting, crafting::kTargets[2].hook},
    {Module::crafting, crafting::kTargets[3].hook},
    {Module::crafting, crafting::kTargets[4].hook},
    {Module::crafting, crafting::kLookupRva},
    {Module::force_palm, force_palm::kEvaluateRva},
    {Module::palm_height, palm_height::kEventRva},
    {Module::glide, glide::kSpeedRva},
    {Module::dismount, dismount::kHandlerRva},
    {Module::auto_loot, auto_loot::kNearbyRva},
    {Module::world, 0x3dc4730},
    {Module::world, 0x3dc50e0},
    {Module::world, 0x9adf20},
    {Module::photo, 0x35d8080},
};
// MinHook writes a five-byte entry jump. These guards must remain usable when
// another packaged module has already installed its hook, in either load order.
constexpr bool overlaps_entry(std::uintptr_t start, std::size_t length, std::uintptr_t entry) {
    return start < entry + 5 && entry < start + length;
}
bool accepts_guard(Module owner, std::uintptr_t start, std::size_t length) {
    for (const auto& hook : hooks)
        if (hook.module != owner && overlaps_entry(start, length, hook.rva)) {
            std::printf("Guard %zx+%zu overlaps another module's hook %zx\n",start,length,hook.rva);
            std::fflush(stdout);
            return false;
        }
    return true;
}
}

int main() {
    // Reproduce the removed CharacterInfo guard: Blackstar legitimately patches
    // this entry, while mount protection only reads its manager's rows directly.
    assert(!accepts_guard(Module::mount, blackstar::kResolverRva, 45));
    std::size_t checked = 0;
    for (const auto& pin : mount::kCodeSpans) {
        assert(accepts_guard(Module::mount, pin.rva, pin.count));
        ++checked;
    }
    for (const auto& pin : consumables::kPins) {
        assert(accepts_guard(Module::consumables, pin.rva, pin.length));
        ++checked;
    }
    for (const auto& pin : crafting::kFingerprints) {
        assert(accepts_guard(Module::crafting, pin.rva, pin.length));
        ++checked;
    }
    for (const auto& pin : force_palm::kPins) {
        assert(accepts_guard(Module::force_palm, pin.rva, pin.length));
        ++checked;
    }
    for(const auto& pin:palm_height::kPins) {
        assert(accepts_guard(Module::palm_height,pin.rva,pin.length));++checked;
    }
    for (const auto& pin : axiom::kPins) {
        assert(accepts_guard(Module::axiom, pin.rva, pin.length));
        ++checked;
    }
    for (const auto& pin : glide::kPins) {
        assert(accepts_guard(Module::glide, pin.rva, pin.length));
        ++checked;
    }
    assert(accepts_guard(Module::resources,resources::kCommitRva,sizeof(resources::kCommitPrologue)));
    assert(accepts_guard(Module::resources,resources::kBoundsRva,sizeof(resources::kBounds)));
    assert(accepts_guard(Module::resources,0xc7eff12,sizeof(resources::kCommitStores)));
    checked+=3;
    for (const auto& pin : dismount::kCode) {
        assert(accepts_guard(Module::dismount, pin.rva, pin.size));
        ++checked;
    }
    for (std::size_t i=0;i<std::size(hooks);++i)
        for (std::size_t j=i+1;j<std::size(hooks);++j)
            assert(!overlaps_entry(hooks[i].rva,5,hooks[j].rva));
    for (const auto& pin : auto_loot::kPins) {
        assert(accepts_guard(Module::auto_loot, pin.rva, pin.bytes.size()));
        ++checked;
    }
    for (const auto& pin : world::kWorldCodePins) {
        assert(accepts_guard(Module::world, pin.rva, pin.length));
        ++checked;
    }
    for (const auto& pin : photo::kCodePins) {
        assert(accepts_guard(Module::photo, pin.rva, pin.size));
        ++checked;
    }
    for (const auto& pin : teleport::kCodePins) {
        assert(accepts_guard(Module::teleport, pin.rva, pin.bytes));
        ++checked;
    }
    for (const auto& pin : teleport::kMarkerCodePins) {
        assert(accepts_guard(Module::teleport, pin.rva, pin.bytes));
        ++checked;
    }
    assert(checked == 181 + std::size(palm_height::kPins) + std::size(axiom::kPins) + std::size(glide::kPins) + std::size(auto_loot::kPins) + std::size(teleport::kCodePins) + std::size(teleport::kMarkerCodePins));
    std::printf("PASS: %zu new-module guards coexist with all %zu distinct packaged hook entries; conflicting Blackstar getter guard rejected.\n",checked,std::size(hooks));
}
