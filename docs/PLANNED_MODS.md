# Planned Workbench Mods

Updated: 17 October 2026.

Status: implementation authorized on 16 October 2026. This is the complete list
of 25 user-selected mods and features, all intended for the Mod Marketplace.
Development and automated tests used workspace copies without changing the
running installation or requiring a game restart. Some related features already
existed in Workbench; the list records the agreed final scope. The finished local
test build was installed on 17 October, after verifying that the game and
Workbench were closed. All application labels and descriptions stay in English.

## Release preparation: 0.9.0

The current source has 35 Marketplace cards and 20 optional runtime modules.
All eleven UI/UX improvements are implemented: category/status filters, clearer
catalog/game/installation states, per-card feedback, retained drafts, keyboard
accessible dialogs, compact views, exact inputs and presets, consistent save
targets, Workshop steps, visible item-language labels and Save & connect.

The latest local verification passed 187 browser tests and 50 UI unit tests.
The existing native checkpoint passed 23 test groups and the 22-module loader
fixture. These checks do not replace actual gameplay acceptance. The latest
startup observation confirmed the saved Force Palm, glide, ordinary jump and
resource-protection values; Axiom waits for a loaded character.

A local launch-context issue was traced to inherited Windows AppData file
redirection from the development host. The installed app and all bundled files
were correct. Launching the installed app through the normal Explorer desktop
and restoring the existing choices to the normal per-user folder resolved the
settings visibility difference. No machine-specific paths or settings belong
in the release package.

The sections below retain earlier implementation checkpoints and private
evidence references as historical records, rather than current release totals.


### Later additions: search, Force Palm height, spirit, Axiom reach and gliding speed

The Marketplace now includes a search across all 35 cards, accepting English
metadata and common German aliases. Hidden cards retain unsaved settings.
Four additions extend the original list:

- **Force Palm height:** a separate 1×–100× slider for verified aerial launches.
  The ordinary jump multiplier does not compound with it. Chain-limit removal
  remains a separate mod; learned state and other native rules remain.
- **Unlimited spirit:** an independent switch alongside stamina in the existing
  shared resource module. Existing positive spirit is preserved on spending;
  learning, cooldowns and minimum-resource eligibility remain native.
- **Axiom armlet reach:** 2×–5× in 0.01× steps, plus normal 1× reset. Shared native
  acquisition/holding reach and distance-release tolerances are scaled in RAM.
  Weight, target eligibility and other non-distance restrictions remain.
- **Gliding speed:** a separate 100%–500% slider, with 100% as normal.
  It is independent of the ground-movement multiplier.

These choices persist locally and load at game startup. Setup and settings
changes require the game to be closed. The package now has 20 optional modules;
spirit and stamina use one module. Original-image audits and owned fixtures
cover these additions; actual gameplay acceptance remains pending.
See [Force Palm height](research/FORCE_PALM_HEIGHT.md),
[spirit](research/PLAYER_SPIRIT.md), [Axiom reach](research/AXIOM_REACH.md)
and [gliding speed](research/GLIDE_SPEED.md).

The combined local package passes all 23 Marketplace native test groups, the
synthetic loader with all 22 ASIs, all 20 optional-module lifecycle/recovery
checks and the base-package checks. UI unit tests and browser regressions cover
search, saved controls and automatic unlocking after the game closes without
losing drafts. Root repeated the original-image audits for the new modules.
The final Workbench build was installed and reopened while the user's game
continued running; every game runtime file remained byte-identical. New/updated
optional game modules are bundled for later installation with the game closed.
The final local installer and verification are in
`.local/marketplace-search-glide-final-installer-20261017/`.

### Original 25-feature checkpoint

Every one of the 25 points now has a Marketplace card and implementation path.
The final scope audit identified two missing ordinary-use families. Selected-item
preservation (#11) is now extended to 561 selectable definitions: nine arrows,
15 stat potions, two Holy Water healing items and 535 food/edible definitions.
Normal shooting, eating and drinking are protected; feeding, conversion,
release and summoning keep native costs. Root verified the original factory
and decoder evidence, native fixtures and coexistence checks, matching Rust
selection codec, three browser cases, and all 561 display names in both the
original English and German catalogs. Independent native review also passed.
Corpse loot for auto-loot (#8) is now implemented and independently reviewed.
The original-resource proof covers three normal corpse interactions, nine
variants and seven attack definitions, all reaching Skill 19 / target LootBuff
11. Native dispatch then queues opcode `0x7e8`
(`TrocTrProcessLootingDeadDropOnceTimer`), whose handler reaches the normal
inventory transaction. Root repeated the 104-pin, 88-range and ten-resource
audit. A reversed dead-state guard was corrected to require nonzero state
`+0x273`; owned fixtures cover that polarity, ownership/crime refusals,
temporary-record lifetime and cleanup failures. The shared coexistence test
also verifies the corrected CharacterInfo getter pin at `0x389575`.
All 25 points now have local implementations with their documented boundaries.

Current post-extension checks pass: 156 browser scenarios, 39 UI unit tests,
the frontend build, 19 Marketplace native test targets, Rust formatting and
warning-free workspace Clippy. Rust library tests pass with 25 app, 197 core
and 38 format tests; the existing opt-in environment tests remain ignored in
that library run. The specific all-561 English/German archive verification
was executed separately and passed without game writes. The final native build,
base-runtime package checks and all optional-module lifecycle/recovery checks
also passed. The requested local Workbench installation is complete and verified.
Actual gameplay acceptance remains for the user's test.

Final native staging: `.local/marketplace-expansion-complete-20261017/`.
It contains 17 optional Marketplace modules plus LiveItems/ExtraSockets.
The paired desktop build uses
`CRIMSON_EXTRA_SOCKETS_SHA256=704fd5cf8c55250eacf394c33b3581cb95a6726c2b826736e07c2abb65931f57`.
The NSIS installer is
`.local/marketplace-expansion-installer-20261017/Crimson-Workbench_0.8.0_local-test-setup.exe`
(SHA256 `97ce3cae05cc006c578c5da6634118f58ea5f33f3466d673f99b2dca0368d125`).
The installed application and every bundled runtime file were verified against
the build. The base runtime was migrated using the original 0.7.0 package's
ownership checks and the new package's installer. Personal socket data, the
existing loader and other installed ASIs retained their exact hashes. Optional
Marketplace modules were bundled, not automatically enabled. The game was not
started. Installation evidence is in
`.local/marketplace-expansion-installer-20261017/installation-result.json`.

Teleport (#14) has now passed independent integration review: 16 bookmarks,
separate map pins/waypoints, native ground and placement validation, normal
field-transition dispatch and later position observations. Saved returns require
the current FieldInfo; ordinary mainland markers require available geometry.
Other layers and unsuitable/unavailable destinations are refused. There is no
arbitrary frontend XYZ input or live gameplay acceptance.

Extended photo mode (#17) now has its English Marketplace card and live panel,
native PhotoCamera entry/exit, HUD visibility and observed scene pause. Initial
setup requires a closed game; the installed controls work while playing. The
normal photo camera's movement and bounds remain engine-owned. Late native
observations cannot turn an expired command into success or trigger a retry.

The earlier photo checkpoint passed 16 Marketplace native test targets, three LiveItems
targets, all 18 ASIs in the synthetic pinned-loader host, package lifecycle and
recovery fixtures, 33 frontend unit tests, the frontend build and Rust app check.
Five photo and five world Rust codec/owned-pipe tests passed. The 53 relevant
Marketplace/world/photo browser scenarios passed, including two corrected/new
card scenarios rerun separately. Original-image verification covers 32 photo
code ranges, six vtable entries and two original UI resources. None of these
checks is an in-game acceptance test.

Earlier teleport checkpoint staging:
`.local/marketplace-expansion-runtime-20261017-teleport-final/`.
It contains 17 optional Marketplace modules plus LiveItems/ExtraSockets, not an
installed desktop build. The paired desktop build hash is
`CRIMSON_EXTRA_SOCKETS_SHA256=1e994ce1f30467f4cda47b757d0a2b96d236b8a1357f989acc34c9a8710ca68c`.
The full native build passed 19 Marketplace targets, three LiveItems targets,
and the pinned-loader synthetic host with all 19 ASIs. Package lifecycle and
recovery checks passed. Independent original-image review verified 42 teleport
native code ranges and four table slots; four copied-native/policy suites,
eight Rust teleport tests, 39 frontend unit tests and all 155 browser
scenarios passed. The workspace Rust library run passed 25 app, 196 core and
38 format tests, with 21 environment/fixture tests intentionally ignored; the
specific owned Photo/World/Teleport pipe cases were also run separately.
This is implementation evidence, not a successful game session.
Current checkpoint evidence is in `.local/teleport-final-native-build.log`,
`.local/teleport-final-package-tests.log`,
`.local/teleport-root-original-pins.log`,
`.local/teleport-root-native-fixtures-final.log`,
`.local/teleport-root-rust-tests.log`,
`.local/marketplace-expansion-final-rust-tests.log`,
`.local/marketplace-expansion-final-ui-tests.log` and
`.local/marketplace-expansion-complete-browser-check.log`.
Earlier photo evidence logs are `.local/photo-native-package-build.log`,
`.local/photo-package-lifecycle.log`, `.local/photo-browser-tests.log`,
`.local/photo-marketplace-final-tests.log`, `.local/photo-ui-unit-tests.log`,
`.local/photo-rust-tests.log`, `.local/photo-world-regression.log` and
`.local/photo-app-check.log`. See [photo mode](research/PHOTO_MODE.md).

The user requested installation after the entire list is finished so they can
test it. That local installation is complete. No commit, push, PR or release has
been made for this expansion.
Earlier checkpoints below are historical and retain their then-current counts.

## Confirmed list

| # | Mod or feature | Requested behavior |
| --- | --- | --- |
| 1 | Selective skill unlocks | Search for and select individual abilities to unlock. Also provide an **Unlock all skills** button. |
| 2 | No fall damage | Prevent fall damage to the player. |
| 3 | Blackstar anywhere | Reproduce the user's current Blackstar mod configuration. Do not assume that every quest, indoor or placement restriction can be removed. |
| 4 | Larger and faster lion and Rokade | Make both mounts 30% larger. Preserve the user's existing movement speed setting of **x1.30** for both; matching multipliers do not necessarily mean identical absolute speeds. |
| 5 | 999 inventory slots | Set player inventory capacity to 999 slots. |
| 6 | Per-item stack limits | Search for and select the desired item, then set its maximum stack size. This is separate from changing the quantity currently owned. |
| 7 | Unlimited player stamina | Allow sprinting, climbing and swimming without consuming player stamina. |
| 8 | Auto-loot | Automatically collect nearby loot. |
| 9 | Live time and weather controls | Choose the time of day and weather while the game is running, with independent options to keep either fixed. Advance forward by a chosen number of hours, including new days. Provide controls and weather reset in the Mod Marketplace. These live-use requirements were confirmed on 17 October 2026. |
| 10 | Player jump height | Provide a slider for normal or increased player jump height. |
| 11 | Preserve selected consumables | Choose consumable items, such as arrows or potions, that should remain available after use. |
| 12 | No durability loss | Prevent durability loss for weapons, armor and tools. |
| 13 | Free crafting and upgrades | Allow material costs and money costs to be disabled independently for crafting and upgrades. |
| 14 | Waypoint and saved-position teleport | Teleport to a map marker. Also save custom positions and return to them. |
| 15 | Reveal world map | Remove map fog without deliberately completing quests or changing quest progress. |
| 16 | Player movement speed | Adjust player running and sprinting speed independently of mount speed. |
| 17 | Extended photo mode | Provide a free camera, a HUD visibility control and the ability to freeze a scene. |
| 18 | Automatic fishing | Detect bites and automatically perform the actions needed to reel in fish. |
| 19 | Protect the active mount | Prevent the player's active mount from being killed by combat damage. |
| 20 | Higher lion and Rokade jumps | Increase jump height by **40%** for the lion and Rokade. Updated from 30% at the user's request. |
| 21 | Higher automatic dismount threshold | For the lion and Rokade, trigger the forced separation of rider and mount only at **five times the original fall-height threshold**. |
| 22 | Transfer native weapon effects | Select an existing weapon effect and transfer it temporarily to a compatible target weapon, including its functional behavior and associated visuals. See the detailed requirement below. |
| 23 | Restricted-region return countdown | Increase the warning countdown for returning from a restricted region from **30 seconds to 30 minutes (1,800 seconds)**. Identify the specific boundary-return timer; this is not a general game-time multiplier. |
| 24 | Contribution value editor | Increase contribution values to a user-entered target amount. Identify the exact contribution balance/type and valid numeric range before exposing the action. Preserve unrelated points, balances and progression. |
| 25 | Unlimited Force Palm jumps | Remove the user's observed limit of three consecutive Force Palm jumps, allowing the move to be repeated without that count limit. Identify the actual aerial-use counter/reset rule; ordinary jump height and unlimited stamina are separate features. |

## Earlier implementation checkpoint, 17 October 2026

Work remains local on `codex/marketplace-expansion`. Nothing from this expansion
has been installed into the running game, committed, pushed or released.
Current scope: **21 of 25 points locally implemented; 4 remain**. Local
implementation includes automated checks, not completed in-game acceptance.

- Locally implemented with automated checks: selective skill unlocks (#1), the
  packaged existing fall protection (#2), both mounts' size/speed settings (#4),
  999-slot inventory definitions (#5),
  selected-item stack limits (#6), no durability loss (#12), native automatic
  reeling for all four fishing rods (#18; manual casting), and the four
  restricted-region return timers (#23). Native weapon-effect selection and
  reversible definition transfers (#22) are locally implemented for lightning
  on observed greatsword/spear/greathammer types and fire on observed spear types.
  In-game acceptance of this expansion remains pending.
- Existing Blackstar settings (#3) remain available through their runtime card.
- Player stamina protection (#7) now has its own locally built runtime module,
  Marketplace card, guarded selected-player policy and package tests. It uses
  the native Stamina definition key rather than an assumed status ordinal.
  In-game sprinting, climbing and swimming acceptance remains pending.
- Higher lion/Rokade jumps (#20), now **40%**, have a separate locally built
  runtime module and Marketplace card. The native height input is scaled once
  on a private event copy; both mounts still need in-game acceptance testing.
- Player jump height (#10) now has an independent runtime module and a
  100–300% Marketplace slider with explicit saving and reset. The setting is
  stored in the user's application profile, read at game startup and scoped to
  the selected player. Native, persistence and browser fixtures pass; measured
  in-game height and stability still need acceptance testing.
- Player movement speed (#16) now has an independent runtime module and an
  English Marketplace card with a 100–300% slider, explicit saving and reset.
  Fresh selected-player ownership and normal ground movement guards limit the
  horizontal velocity copy; vertical movement, time step and mounts are kept
  separate. Parent review, native fixtures, preference tests, browser tests,
  combined native staging and temporary package lifecycle tests pass. Measured
  movement, slopes and combat-motion acceptance still need game tests.
- Crafting/upgrades (#13) are **partially implemented**, not included in the
  completed-point count. Three components now have independent Marketplace cards:
  remove material/token costs from 17,069 upgrade recipes while retaining the
  owned equipment input; craft from 1,507 ordinary recipes using immutable
  material-free runtime views; and remove the verified 300-Clockwork-Insect fee.
  The runtime component preserves that fee when it has not been separately
  removed. It leaves original recipe files/manager rows and database validation
  unchanged, removing the earlier startup-order dependency. The complete
  recipe/group/tool/wallet audit and native cost-vector fixtures have since
  resolved the remaining recipe-money question: the existing currency component
  covers the only recipe fee. Shop, service and quest charges are separate from
  this crafting feature. In-game acceptance is still pending.
- Contribution balances (#24) now have an English Marketplace save-action card.
  Users can review increases to six existing faction balances independently,
  with exact decimal targets, save-pair/selection binding, automatic backups and
  interrupted-operation recovery. Other currencies, faction experience and
  levels stay unchanged; missing balance entries are not fabricated. Native
  field evidence, full historical-save fixtures and temporary pair-transaction
  tests pass. In-game display/use/save/reload acceptance remains pending.
- Reveal world map (#15) now has an English Marketplace save-action card with
  a bound preview, explicit confirmation, automatic backup and interrupted-edit
  recovery. Only the existing fog-code payload changes; all quest, knowledge,
  discovery and travel records remain byte-identical. Historical-save and
  temporary transaction fixtures pass. In-game map rendering and save/reload
  acceptance remain pending.
- Preserve selected consumables (#11) now has a dedicated native module and
  English Marketplace search/selection controls. The verified initial scope is
  nine arrow types and 15 stat potions; food, Holy Water and other use families
  remain unsupported. Explicitly saved selections keep native effects,
  cooldowns, ownership and other costs. Parent native/layout review, all
  browser tests, combined native build and temporary package lifecycle tests
  pass. In-game use and count/effect/cooldown acceptance remain pending.
- Active mount protection (#19) now has an independent native module and an
  English Marketplace card. It protects a currently ridden mount and the unique
  selected summoned owned mount after dismount. Player/NPC health, inactive
  registered animals, healing and stamina retain native behavior; direct
  scripted death/despawn is outside its scope. Parent ownership/HP review,
  278 forwarding cases, five lifecycle cases and package integration pass.
  A regression check covers the removed overlap with Blackstar's getter hook.
  Ridden/on-foot combat and save/reload acceptance still need game tests.
- Unlimited Force Palm jumps (#25) now have an independent native module and
  English Marketplace card. The native aerial chain can restart after the
  three-stage sequence without changing jump height or spirit costs. Parent
  original-EXE/archive review, copied-native recursion/ABI fixtures, sibling
  hook checks, browser controls and combined native/package tests pass. More
  than three observed aerial uses and normal spirit/landing/reset behavior
  still need in-game acceptance.
- Still under investigation or awaiting implementation: #8–9, #14 and #17.
  Existing stamina controls for mounts do
  not establish unlimited player stamina. Additional weapon-effect sources and
  target classes require their own compatibility evidence.
- Live time/weather (#9) has verified current-build clock units and native
  weather-state names, with a tested pure clock conversion helper. Authoritative
  setters, safe live dispatch and independent freeze behavior remain unfinished;
  no runtime controls are enabled for this point.
- Five-times dismount height (#21) is locally implemented as an independent
  runtime Marketplace card for the lion and Rokade. It changes their exact
  automatic fall-height condition from 5 to 25, retaining manual, combat and
  death-related removal rules. Parent source review, native fixtures, sibling
  hooks, browser controls and package lifecycle checks pass. Gameplay
  acceptance remains pending.

The timer implementation and its remaining game test are documented in
[Restricted-region return timer](research/REGION_RETURN_TIMER.md).
Rokade's isolated appearance recipe and pending game checks are documented in
[Dedicated Rokade appearance](research/ROKADE_APPEARANCE.md).
Identified weapon effects, compatibility and pending game tests are documented in
[Native weapon effect transfers](research/NATIVE_WEAPON_EFFECTS.md).
The selected-player stamina policy and pending game checks are documented in
[Player stamina protection](research/PLAYER_STAMINA.md).
The unfinished aerial-use-limit investigation is documented in
[Unlimited Force Palm jumps](research/FORCE_PALM_JUMPS.md).
The native height calculation, 40% mount-jump module and pending game checks are
documented in [Lion and Rokade jump height](research/MOUNT_JUMP_HEIGHT.md).
The separate player slider, native consumer and pending game checks are
documented in [Player jump height](research/PLAYER_JUMP_HEIGHT.md).
The partial currency component, native cost evidence and remaining work are
documented in [Crafting and upgrade costs](research/CRAFTING_COSTS.md).
The contribution balances and exact save-edit boundary are documented in
[Contribution balance editing](research/CONTRIBUTION_VALUES.md).
The separate player movement module and settings are documented in
[Player movement speed](research/PLAYER_MOVEMENT_SPEED.md).
The isolated fog bitmap, native loading evidence and save transaction are
documented in [World map fog reveal](research/MAP_FOG_REVEAL.md).
The eligible consumables, native self-cost boundaries and Marketplace controls
are documented in [Preserve selected consumables](research/PRESERVE_CONSUMABLES.md).
The five-times fall threshold and pending gameplay acceptance are documented in
[Mount dismount threshold](research/MOUNT_DISMOUNT_THRESHOLD.md).
The selected summoned/ridden mount scope and remaining acceptance checks are
documented in [Active mount protection](research/ACTIVE_MOUNT_PROTECTION.md).

### Previous pause checkpoint

The user requested a stop after finishing the current work, to continue tomorrow.
All delegated work stopped at that checkpoint. The user resumed work on
17 October 2026 with "weiter gehts"; the continuation below is now active.

The latest completed implementations are selected consumable preservation (#11)
and active mount protection (#19), including their English Marketplace controls
and native distribution integration. Clock/weather (#9) and forced-dismount
height (#21) have documented research checkpoints, not completed implementations.
At that earlier checkpoint, ordinary free crafting (#13) remained staged behind an unresolved startup-order
requirement; the two existing recipe-cost components remain independently usable.

The seven remaining points are auto-loot (#8), live time/weather with independent
freeze controls (#9), the rest of free crafting/upgrades (#13), teleport (#14),
extended photo mode (#17), five-times dismount fall height (#21), and unlimited
Force Palm repetitions (#25). No gameplay acceptance, installation, commit,
push or release is implied by the local completion count.

Final offline checks for this round passed: 196 core tests (17 ignored), ten
application Rust tests (one ignored), the frontend build, 36 Marketplace browser
tests, nine Marketplace native test targets, two LiveItems native test targets,
and the synthetic loader host with all 12 packaged ASIs. Temporary package tests
also passed independent module lifecycles, running-game refusal, foreign and
modified-file refusal, rollback and interrupted-operation recovery.

The final native staging directory is the ignored
`.local/marketplace-expansion-runtime-20261017-final/`. It contains ten optional
Marketplace ASIs plus LiveItems/ExtraSockets and pinned loader resources; it is
not an installed desktop build. The native build and package logs are
`.local/marketplace-final-native-build.log` and
`.local/marketplace-final-package-test.log`. No new desktop installer was built.
Current-build time encoding, weather masks and unfinished live control work are
documented in [Live time and weather controls](research/LIVE_TIME_WEATHER.md).

### Active continuation, 17 October 2026

The user resumed the full objective. Work is local and the running installation
is still untouched. Ordinary crafting materials now have an independent native
Marketplace card and tested immutable recipe views; the original invalid-empty-
archive dependency is not enabled. Combined native/package checks and 37 browser
tests pass. A complete source-table and native cost-vector audit has since
resolved #13's remaining money-cost question. The three independently selectable
crafting/upgrade components cover all reviewed normal recipe costs. Force Palm
repetition (#25) has also passed independent parent review
and joined its own English Marketplace card. Five-times dismount height (#21)
has also passed independent parent review and joined its own card; the completion
count has now reached **23/25**, after independent review and Marketplace
integration of auto-loot (#8) and live time/weather (#9). Two full points remain
open: teleport (#14) and extended photo mode (#17). This counts local
implementation and offline verification, not successful in-game acceptance.

The current native stage is
`.local/marketplace-expansion-runtime-20261017-advance-time-reviewed/`, containing
15 optional Marketplace modules plus LiveItems/ExtraSockets. It is a local test
package, not an installed desktop build. No commit, push, PR or release exists
for this expansion. Teleport and extended photo mode work continue in separate
owned files.

The current stage passes 15 Marketplace native targets, three LiveItems native
targets, the pinned loader's synthetic 17-ASI host and all temporary package
lifecycle/refusal/recovery checks. The coexistence fixture checks 215 guarded
code spans against 43 distinct hook entries. The parent also reran auto-loot's
69-pin/29-range/four-resource source audit and time/weather's 65-code/17-table
phase audit. The full browser regression passed 136 tests; two obsolete test
selectors/flows were corrected after removing Item database and both targeted
reruns passed, covering all 138 tests. This includes 41 Marketplace and five
world-control scenarios. All 22 UI unit tests, the frontend build and desktop
compile check also pass.
A separate ignored copied-native Force Palm fixture passes both of its tests.
The UI contains 30 cards; cards are not the same unit as the 25 planned points.

LiveItems now exposes a versioned, process-lifetime pinned borrowed-task
observer registry used by live world controls. This avoids a duplicate hook on
its task entry; it does not itself complete teleport.
See [borrowed task observers](research/LIVE_TASK_OBSERVERS.md) and the current
[teleport investigation](research/TELEPORT_RUNTIME_PATH.md).

The parent reran #13's five-table source audit, all 6,816 typed item roundtrips,
the 505-case copied-native cost fixture and the actual full-table Rust cost
implementation test. Conditional jumps in each copied native method were also
checked to stay inside its owned span. No extra money hook or Marketplace card
was needed. See [Recipe money-cost coverage](research/CRAFTING_MONEY_COMPLETION.md).

The world-controls card opens a dedicated live panel. Set the current day's
time, independently freeze/resume the clock, select rain/snow and five weather
parameters, freeze/unfreeze weather, or use **Reset weather**. Reset releases
only Workbench's weather override and keeps the time setting. Its native receipt
waits for ordinary weather production to resume. Initial setup requires the
matching bundled LiveItems runtime and a closed game. All control testing used
owned fixtures; actual time acknowledgement delivery, visible weather and
calendar-dependent quest behavior remain unmeasured. See
[live controls client](research/LIVE_WORLD_CLIENT.md).

The subsequent **Advance time** extension accepts 1–10,000 displayed game hours
and carries into later days/weeks. It retains an owned clock freeze and weather
settings, anchors against the current native clock, and rejects calendar overflow
before publication. It uses a distinct bounded operation rather than changing
the current day's hour. The native offline suite now covers 86,292 checks;
five Rust codec/owned-pipe tests and all 47 current Marketplace/world browser
scenarios pass. No real sleep action or gameplay test was run.

The separate Item database view was removed at the user's request. Item Picker
is now the default view; crafting item links open and select the matching item
there. Search, item names, icons and values continue using the local catalogue.
Mods operate on internal IDs and native fields, independently of the game's
display language; the Workbench interface remains English.

## Native weapon effect transfer

This replaces the earlier generic "Elemental weapons" idea. The request is to
reuse effects that already belong to particular weapons in the game, rather than
designing arbitrary elemental effects.

### User-provided example

- Source weapon: **Elektro-Mecha-Langschwert**.
- Source effect: **Donner-Waffenangriff**.
- Desired target: another compatible sword.
- Desired result: the target sword gains that weapon effect, including the
  lightning effects associated with it.

These German names and the described behavior were supplied by the user. The
identified item is Electro-Mecha Longsword 1001062, using native equipped passive
91101 at level 3. Flame Spear 1000451 provides a second native source. The linked
research note records the compatibility evidence and remaining acceptance tests.
Other source effects and weapon classes still need individual investigation.

### Intended controls and behavior

1. Search for and select the target weapon.
2. Inspect source weapons and their identified effects, including the source
   weapon name, effect name and compatibility information.
3. Select the effect to transfer. Only effects whose actual requirements have
   been checked should be presented as supported.
4. Preview the change before applying it.
5. Temporarily apply the effect with its relevant gameplay behavior and visual
   effects. A lightning appearance by itself does not satisfy the example.
6. Allow the transferred effect to be removed and the target's original effect
   configuration to be restored.

The source weapon must remain unchanged. Preserve unrelated target properties,
including its identity, upgrade level, sockets and statistics, unless an effect
requires a specific change that is explicitly shown in the preview. If the
target already has an effect, resolve compatibility and replacement behavior
explicitly instead of silently overwriting it.

Start by investigating transfers between compatible swords. Support for other
weapon classes depends on their animation, attack, resource and effect
requirements. Temporary application and restoration use the reviewed
archive-definition plan. This changes all copies of a selected definition with
the game closed; running-game controls and individual inventory-instance editing
are not implemented.

## Mount settings clarified

- The +30% size and +40% jump height apply to both the lion and Rokade.
- The movement setting remains the user's existing **x1.30** multiplier.
- The forced-dismount request concerns a large downward drop, not voluntarily
  dismounting or the player's ordinary jump height.
- Scale the original forced-dismount fall-height threshold by **5**. Do not
  additionally multiply that threshold by the +40% mount jump setting.
- Mount damage protection, player fall-damage prevention, mount jump height and
  forced-dismount behavior are separate requested effects. One does not establish
  that the others work.

## Planning boundaries

- Implement the entire confirmed list in the Mod Marketplace. The earlier
  planning-only restriction was superseded by the user's implementation request.
- Do not treat unselected brainstorming ideas as confirmed features.
- Verify game-build support, data dependencies and compatibility per feature.
- Determine which features can use the existing closed-game editing workflows
  and which require new runtime integration. Do not promise live controls before
  they have been implemented and tested.
- Reuse the user's verified existing mod settings where requested. Unrelated
  changes in those older mod packages are not automatically added to this scope.
