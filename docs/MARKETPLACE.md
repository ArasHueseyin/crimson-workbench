# Free mod marketplace

Version 0.10.0 contains **36 cards** covering the original 25 requested features, the later Force Palm height, spirit, Axiom reach and gliding-speed additions, and the existing sleep and mount-stamina mods. All mods are free. It works locally without an account, payment, server or separate mod downloads. The interface is English; game definitions use internal IDs and are independent of the game's display language. Game assets and personal saves are not distributed.

The expansion is prepared for **EXE 1.0.0.2976** and matching verified data. Automated and original-image checks do not establish in-game acceptance. The newly added workflows still need the user's game tests.

## When changes apply

Each card carries an availability label:

| Card type | Setup and changes | Use in the game |
| --- | --- | --- |
| Archive recipes and save actions | Close the game before applying the reviewed change | Applied definitions or save changes take effect when loaded |
| More time beyond restricted regions | Its own Install mod / Remove mod buttons, with the game closed | The 30-minute scripts load at game startup and stay installed between starts |
| Runtime mods | Close the game to install/remove the ASI or save startup settings | The module works during play after the next game start |
| Live time/weather, photo and teleport | Install the matching runtime with the game closed | Adjust through the card's live panel while playing |

Browsing, searching and preparing a draft does not apply changes. Selecting a card is not an installation status. Runtime cards install independently of the combined archive plan.

**Search mods** filters names, effects and categories, including common German aliases. Category and status filters can be combined with the search. **Installed** and **Updates** use verified mod-file status, including the independent region mod; combined-plan selections are labelled **Draft selected** and do not imply installation. **Live controls** shows cards that can be adjusted while playing after setup. The search remains visible while scrolling on desktop-sized windows; **Ctrl+K** focuses it.

Unsaved startup settings stay on this device across navigation and status refreshes. **Save** writes the selected setting; **Discard changes** restores its last saved value. Sliders also provide exact number inputs and presets. Axiom explicitly separates normal **1×** reach from the extended **2×–5×** range. A runtime file being installed does not prove that its module has loaded in the current game session.

Progress and results appear on the affected runtime card. Dialogs support keyboard focus containment, **Escape**, and return focus to the button that opened them. Save editors identify the selected slot/account and show local modification dates beside their write actions.

## Available features

- **Mounts:** 30% larger lion and Rokade, x1.30 movement speed, no mount stamina cost, 40% higher jumps, five-times automatic fall-dismount threshold, and combat protection for the ridden or selected summoned owned mount. The shared lion model also affects other characters using it; Rokade has its own appearance. Scripted deaths/despawns retain native behavior.
- **Player movement:** independently selectable unlimited stamina and spirit, adjustable 100–300% ordinary jump height and ground movement speed, no fall damage, unlimited Force Palm aerial chains, and separate **1×–100× aerial Force Palm height**. Force Palm must be learned. Spirit protection prevents spending existing spirit; normal eligibility, minimum resource requirements and cooldowns remain. Ordinary jump height does not multiply Force Palm height. Gliding speed has its own **100%–500%** slider (normal to 5×).
- **Axiom reach:** choose **2×–5×** reach in 0.01× steps, or reset to normal 1×. The module scales the shared native acquisition/holding range and distance-release tolerances in memory. It preserves default tuning, object eligibility, weight, ability requirements and other non-distance rules. It refuses conflicting tuning overrides.
- **Inventory and equipment:** 999 inventory slots, per-item stack limits, no ordinary durability loss, and supported native weapon-effect transfers. Lightning and fire sources have explicit compatible targets. Definition changes affect all copies of a selected item; they are not edits to one inventory instance.
- **Progression and exploration:** individually selected skills or all 140 main-character skills, six existing faction contribution balances, world-map fog reveal, and a 30-minute restricted-region return countdown. Contribution targets are increases, not fabricated new balances; map fog does not complete quests.
- **Crafting:** separate cards for ordinary materials, equipment upgrade materials/tokens, and the verified 300-Clockwork-Insect backpack recipe fee. Normal unlocks, tools and owned upgrade equipment are still required. Free crafting allows up to 40 outputs per batch.
- **Convenience:** automatic reeling on all four rods after manual casting, auto-loot for eligible free ground items and normal corpse loot in the normal interaction radius, and preservation of individually selected supported consumables. Auto-loot retains native ownership, crime, animation and inventory-capacity checks; purchases, theft, custom-pivot and scripted interactions are excluded. Consumable preservation supports nine arrow types, 15 stat potions, 535 food definitions and two Holy Water healing items. Normal shooting, eating and drinking preserve selected items. Feeding, releasing or summoning creatures, item conversions, effects and cooldowns keep their native rules.
- **Existing runtime mods:** no sleep cooldown, 30-hour sleep through the game's labelled 12-hour option, and Blackstar's personal configuration with lifted regional/city/altitude restrictions, boosted regeneration, one-second summon cooldown and longer duration. Final placement, indoor and quest/state rules can still reject Blackstar.

## Apply archive recipes

1. Select the desired cards or configure the item stack/effect editors.
2. Use **Review combined plan** and inspect the changes.
3. Open **Live apply & restore**, complete the existing baseline/source checks, close the game and apply the reviewed plan.

The recipes combine with the current Workshop draft. Applying replaces the previous Workbench overlay. Deselect a recipe and apply again to remove it; restore the complete managed overlay through the existing restore workflow. Unknown or conflicting modifications remain blocked. A read-only preview/export of verified original data does not grant permission to overwrite unmanaged files.

## Restricted-region countdown

**More time beyond restricted regions** uses its own **Install mod / Remove mod** buttons. Close the game, install once and start the game. It changes the four supported desert, snow, water and general-region return warnings from 30 seconds to 30 minutes. Other quest timers and world boundaries keep their native rules.

The installer builds four scripts from the user's hash-verified original assets and adds a separate archive registration. It preserves every existing registration and refuses another mod overriding these same scripts. Removal checks its own files and removes only its registration from the current list, preserving mods added later. Registry backups and a recoverable transaction are stored under `bin64/CrimsonWorkbench-mods/region_return_30min`. **Recover operation** finishes recovery before another installation is attempted. Modified or unknown files are preserved.

The combined archive workflow may require removing this independent registration first; it does not adopt or overwrite unknown registrations. Legacy CLI plans retain their original region recipe support. The desktop card no longer selects that combined-plan recipe.

## Save actions

**Skill unlocks**, **Contribution points** and **Reveal world map** each require an exact selected save, a reviewed preview and confirmation. Save and close the game before writing. Workbench creates a backup and uses bound save-pair transactions with interrupted-operation recovery. Changed saves, inconsistent or unknown structures and invalid/overflowing targets are refused.

Skill unlocks preserve available points and refund accounting. Contribution edits preserve unrelated currencies, faction experience and levels. Fog reveal preserves quest, knowledge, discovery and travel records. These are progression edits, not reversible switches; restoring an older save backup also discards later progress.

## Runtime setup

With the bundled loader installed, close the game and click **Install mod** on the desired runtime card. Workbench stays open; Windows may request administrator permission. Start the game to load it. **Remove mod** disables only that card's managed ASI and preserves other mods, saves, loader and personal data. Refresh runtime status after external changes.

The complete setup bundles all 20 optional Marketplace ASIs plus Live Items and Extra Sockets. No extra download or command is needed. Time/weather and teleport require the **matching bundled Live Items** task broker. Photo mode uses its own guarded native UI callback.

Save Force Palm height, Axiom reach, gliding speed and the independent stamina/spirit choices with the game closed, then install or update the corresponding module. Settings load at the next game start and persist between starts. The existing stamina module supplies both resource switches through one shared hook. Setting normal height/reach or turning off a resource stops that effect on the next start. See [Force Palm height](research/FORCE_PALM_HEIGHT.md), [spirit](research/PLAYER_SPIRIT.md) and [Axiom reach](research/AXIOM_REACH.md) and [gliding speed](research/GLIDE_SPEED.md).

Payload hashes and transaction receipts are checked. **Import & update mod** explicitly adopts a recognized older personal version after backing it up. Unknown/changed ASIs are refused. **Recover operation** resumes recovery of a pending transaction; it does not start another installation. Backups have a `.bak` extension and cannot load as ASIs.

## Live time and weather

Set the time of day, or **Advance time** by 1–10,000 game hours into later days. Freeze/resume the clock independently of weather. Adjust precipitation, clouds, humidity, wind and fog, freeze the weather, or **Reset weather** to release Workbench's override and restore the previous native policy. Resetting weather retains the clock settings. Hour advance retains an owned clock freeze and weather settings.

These controls update the native field clock/weather paths, not the computer clock. They do not simulate a sleep action or prove every calendar-dependent quest behaves like normal elapsed play. See [live controls](research/LIVE_WORLD_CLIENT.md).

## Photo mode

Enter the normal movable photo camera, hide/show its HUD and leave through the native photo lifecycle. Scene pause is reported only after the actual normal photo camera and zero effective simulation delta are observed. Camera controls and bounds remain the game's normal photo controls. Closing the Workbench panel does not close photo mode. An already active photo session opened in the game is not adopted. See [photo mode](research/PHOTO_MODE.md).

## Teleport

Capture the current global position and facing into one of **16 named slots**, return to a saved position, remove a slot, or choose a current map pin/waypoint. Saved returns require the current field to match. Mainland map markers in MainField require both a native ground hit and valid placement; alternate/unknown layers and unavailable/unsuitable geometry are refused. A pin's stored map height is never used as ground height.

Travel goes through the normal native field-transition queue and may load the destination. A queued command is not arrival. A later observed player position is a separate receipt. Lost responses, context changes, native faults and timeouts never trigger automatic repeats. Bookmarks live in local Workbench data for this Windows user, not the game save. Storage failures remain visible; polling does not rewrite them automatically. See [teleport controls](research/TELEPORT_CONTROLS.md).

## Verification

Owned native, UI, protocol, package-lifecycle and recovery fixtures cover implementation behavior without modifying or controlling the running game. Public checks use synthetic records, semantic metadata and short compatibility/ABI signatures. Full original-record and copied engine-function fixtures, decoded assets and private saves stay in ignored local storage and are not distributed. Optional private verification is separate from the game-free CI checks. The current implementation checkpoint and remaining game acceptance work are recorded in [the planned feature list](PLANNED_MODS.md) and each linked research note.

## Unlock journal knowledge

The **Unlock journal knowledge** save action completes 3,939 supported visible
journal entries for game build 1.0.0.2976. Select a save and review its new,
upgraded and already-complete counts. Only missing entries and levels are added;
existing higher levels, skill knowledge, skill points and unrelated save records
are preserved. Journal content includes people, places, creatures, recipes and
quest information, including story spoilers. Hidden internal groups are excluded.

Close the game before applying. Workbench binds the preview to both save files,
creates a backup, checks the game build and commits through the existing guarded
save transaction. An interrupted request can be checked or recovered using the
same request ID. Load that selected save in game afterwards. This is a progression
edit, so reverting to the backup also discards later progress. No personal save
or account data is bundled; different users receive changes calculated from their
own save and can use any supported game text language.
