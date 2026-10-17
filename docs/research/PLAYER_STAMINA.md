# Selected-player stamina protection

Local implementation checkpoint: 17 October 2026. No module was installed into
or loaded by the running game. No gameplay input or native game function was
sent by the read-only observer. Sprinting, climbing and swimming acceptance is
still pending; this is not a released or gameplay-verified feature.

## Supported build and native evidence

The module admits only executable SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`,
PE timestamp `1790086912` and image size `389722112` (game build 1.0.0.2976).
The installed executable was inspected read-only; extracted game assets and
process addresses remain in ignored local research files.

- The verified fall-damage routine at RVA `21a0860` calls generic stat delta
  dispatcher `17ae080`. Its non-HP branch calls `17ae4b0`, whose dynamic-stat
  branch calls `17b3230`.
- `17b3230` jumps to `c7e0860`. That routine resolves the actor's entry through
  `17adcc0` and applies a signed delta through `17b3e10` / `c7e7960`.
- The resulting target value is committed through `17b41b0` / `c7efde0`.
  The relocated native function takes the entry, native time, signed target
  value and 16-bit flag. It retains native bounds/regeneration handling and
  writes the current value at entry offset `08`, the native state at `20`,
  native time at `38`, revision at `48` and flag at `50`.
- The module hooks `c7efde0` once and checks its prologue and final flag/current
  stores in addition to the complete executable hash. It forwards every call
  through the original function and preserves its return value, time and flag.

The semantic Stamina definition key is **1000026**. In the original StatusInfo
table it resolves to index **22**. Indices 17 and 20 are Fullness and Relief,
not alternate stamina gauges. The implementation reads the loaded StatusInfo
manager at RVA `6d69ae8`, validates the row's semantic key and own resolved
index, and requires the dynamic resource kind. It never treats a copied ordinal
from another build as proof of a resource's meaning.

## Scope and behavior

Only a decrease of a positive current stamina value belonging to the currently
selected character is replaced with that current value. Increases, natural
regeneration, native clamping and zero-valued entries use the original behavior.
The module does not refill every actor or write resource memory directly.

Each callback reads the selected client actor from the current client manager,
resolves its matching server representation from the live server registry, and
checks full identity, liveness, known actor/status classes, component ownership,
possessor backlinks and the stat container's actual count. The candidate entry
must be aligned inside one of those two owned containers. Roots, selection,
registry and component backlinks are rechecked before accepting it. No actor
pointer survives into the next invocation. Unavailable, unfamiliar, ambiguous
or changed state forwards the unmodified native target; guarded read failures
also forward unchanged.

Health, spirit, breath, hunger/relief, NPCs, inactive companions and mount
resources do not qualify. Mount stamina remains a separate archive recipe.
Unknown character classes deliberately do not qualify and require further
compatibility evidence before support can be widened.

The Marketplace card is **Unlimited player stamina**, backed by
`CrimsonPlayerStamina.asi`. It installs/removes independently of the other
runtime cards and uses the existing ownership, backup and recovery workflow.
The game must be closed for module installation/removal and a subsequent start
must load the module. No live injection or hot-loading is implemented.

## Local checks

- Native policy fixtures cover both selected representations, semantic key
  mapping after index reordering, all unrelated resource keys, regeneration,
  unknown/NPC entries, stale generations, death, ownership breaks, malformed
  containers, ambiguous registry entries and a world change during inspection.
  Tests operate only on owned fixtures. Native forwarding and SEH failure paths
  preserve original arguments and return values.
- A separate ignored observer used `OpenProcess` with query and read access
  only. With the same C++ selection policy it accepted exactly one current
  Stamina entry and rejected the other 19 client status entries. The matching
  server identity and ownership checks passed. This validates selection, not
  the loaded hook or gameplay effect.
- All five Marketplace module install/remove lifecycles and recovery tests
  passed in newly created synthetic folders. The loader smoke admitted all
  seven ASIs and each rejected the unsupported synthetic executable before
  enabling a hook.
- Frontend build, 19 Marketplace browser tests and desktop Rust check passed.

## Pending in-game acceptance

1. At a convenient restart, install/load the locally built module and verify
   its ready message on the exact supported executable.
2. Check extended sprinting, climbing and swimming, including low stamina before
   enabling the module. Confirm breath/drowning and spirit are unchanged.
3. Check Kliff, Damiane and Oongka, mount/dismount transitions, character swaps,
   death/respawn and returning to the main menu/reloading a save. Unknown class
   gates must be investigated if an intended character forwards unchanged.
4. Verify ordinary health loss, enemy behavior, mount stamina and other resource
   meters are unchanged, and removal restores ordinary stamina consumption.
