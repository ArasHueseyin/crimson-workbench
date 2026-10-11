# Crimson Workbench – What needs to be tested?

## v0.6.3 language changes (11 October 2026)

- [x] English interface, installer text and Markdown documentation.
- [x] Automatic catalog language follows the selected installation's Steam manifest and is resolved again on reload.
- [x] Missing, malformed, unrelated, oversized and unsupported language settings fall back to English.
- [x] All 15 supported Steam language codes map to catalog languages.
- [x] Existing manual preferences remain usable; Automatic and manual choices persist through restart.
- [x] Read-only detection leaves the Steam manifest unchanged.
- [x] 17 frontend unit tests, 92 browser tests and 214 Rust library tests passed locally; five fixture-dependent Rust tests skipped. Clippy passed.
- [ ] Confirm the detected language on another user's installation; use manual selection for a separate in-game override or an unsupported launcher.

## Basestone bonuses in Workbench, 05.10.2026

- [x] 190 unique stones read from the local, verified ItemInfo;
German names/descriptions and complete Stat/Buff lists are available.
- [x] Destruction III: Attack +3 instead of raw value 3000, blank zero values hidden.
- [x] Stormwind III: attack speed +3 levels; No guessed percentages.
- [x] Ability description, effect level and explicit display of unknown fields.
- [x] Selection during loading, reading errors/repetitions, searches and existing
Locks against running game and outdated configuration checked.
- [x] 17 front-end tests and 10 extra sockets UI tests; Preview visually checked.
- [ ] Workbench Open via the desktop link → extra sockets:
Select Destruction III, Stormwind III and Flame of Judgment and check the preview right. For this, the game can continue.
- [ ] Select an already equipped extra sockets: Check basic bonuses/effects.
The insertion/emptiness itself still requires a closed play.


**Update 05.10.2026:** Real game start/loading and the corrected data on client/server pure reading confirmed: 10 / 11 / 15 places, amount each 1, identical base socket, helmet/sword refinement 10. Detection: `.local/extra-sockets-plus10-20261004/runtime-readonly-31728.json`. Visual display/use, stone effect and memory cycle still confirm.

## Ten additional bases: tank, helmet and mecha sword, 04.10.2026

**Correction and free choice of equipment:** The first game start/load attempt worked, but showed corrupted client values. Therefore, XS02 is not yet considered to have existed. Corrected Runtime→InitData transition is installed; No saved games has been changed or reset.

- [x] 36 additional cases with the full original network converter:
base socket 0–5, empty/stocked, prefix normal/0/255; quantity, flags, refinement, durability and storage limits obtained; Client extension restored.
- [x] General configuration v2 and migration v1: new UID, double UID,
inadmissible key, maximum number and atomic storage checked.
- [x] 329 read own equipment instances in the latest save; 326 suitable;
No save-write operations. Seven browser tests including +10 selection, double click, locked game, unknown format and outdated save.
- [ ] **XS05:** Select another suitable instance in Workbench, +10
add; ten empty additional places, existing stones and refinement unchanged. Equipable correctly in the game, then save/reload.
- [ ] **XS06:** Installation on second computer / portable package. The EXE
By itself, runtime mods and personal instance IDs are not set up.

The older list below documents the first installation. Currently total 86 native cases, 22 policy/file cases, 163 core unit tests (four ignored), eleven frontend unit tests and seven browser tests passed. The global `tests/live.rs` roundtrip test fails due to the hash of the already modified `meta/0.papgt`; Browser/catalogue and socket reading tests work. The registry was not touched in this change.

- [x] Exactly three existing instances identified; 10 new places each
in addition to 0 / 1 / 5 Base sockets: total 10 / 11 / 15.
- [x] 50 isolated original native routine/integration cases: all base values
Processed, dynamically copied deep, five basic entries written, additional live entries unchanged when stored.
- [x] 18 Policy/file cases: instance/key/padding check, checksum,
atomic additional file, obsolete write attempts without change rejected.
- [x] Core suite 162 passed / 4 intentionally ignored; Frontend eleven unit-
and four new browser tests: tenth extra sockets, double click, game running, outdated revision, outdated search response. There was a production build.
- [x] Winmm forwarding and joint loading of all seven ASIs in private
process; extra sockets-Mod deactivates in case of wrong EXE before the hooks.
- [x] When the game is closed/Workbench with kept launch protection installed;
Save backups and desktop EXE-backup created; 13 saves and total 22 checked inventory files unchanged; Desktop shortcut uses Update.
- [ ] **XS01:** Workbench Open on desktop → extra sockets: three matching
Items and total 10 / 11 / 15 places. Ten new seats each empty.
- [ ] **XS02:** Start game over Steam, load slot 1. No crash, all
previous items, stones, +10 refinement and inventory unchanged. `CrimsonExtraSockets.log` must report READY and EXTENDED for 10 / 11 / 15.
- [ ] **XS03:** When the game is closed e.g. extra sockets 10 of the mecha sword
equip in Workbench with destruction III, restart/load; Compare effect with previous status. The game surface shows a maximum of five lines.
- [ ] **XS04:** Save in-game, close completely and reload.
Additional stone remains occupied in Workbench; The effect is retained. Normal saves have another five padding entries; existing bases unchanged.

Secure the separate additional file together with the saves. Workbench emptiness directly removes the stone there; Prior assignment is secured in the project. [Evidence and installation backup](.local/extra-sockets-plus10-20261004/RESULTAT.md).

## Electric Mecha Long Sword: fifth base, 04.10.2026

- [x] In the latest save, exactly one instance is identified: 1003664,
four occupied bases and empty fifth place, refinement +10.
- [x] Exactly one decrypted data byte 4 → 5 changed; all other
fields and four stones. Passed independent HMAC/ChaCha/LZ4 read-back and byte-accurate take-back; Six native functional areas are pinned again.
- [x] After closing both apps, the freshest status checked again, original
backed up, only `slot0/save.save` replaced and independently read back. Twelve other save/lobby files unchanged; No new ASI or EXE update.
- [ ] **MS01:** Slot 1 loading; Mecha sword shows five open bases and +10,
The four previous stones are preserved.
- [ ] **MS02:** Place fifth socket on the witch with an existing stone;
save and reload, then check occupancy and effect.

The later extra sockets extension is described above; Their in-game tests is still open.

## Mecha sword: more than five bases, private feasibility, 04.10.2026

- [x] Unchanged complete native base value routine: ten cases,
including six/ten entries, gaps, shelf life and limited release.
- [x] Unchanged full native base buffer copy: 10 cases;
six/ten data sets fully obtained, independent target buffer.
- [x] Native base-storage loop: twelve cases. Original writes five;
Prototype with dynamic comparison spells five/six/ten correctly.
- [x] Private complete 10-entry game test independently retrieved;
all other data received, inverse withdrawal byteaccurately.
- [ ] Connect and check full native loading with Game Allocator.
- [ ] Entire native memory cycle, shares and copies under real
Examine the object life cycle; Fixture callbacks have so far replaced external helpers.
- [ ] create and manage additional seats; Check witch operation/standard items.
- [ ] Only after full prototype targeted installation and in-game validation:
actual bonuses, unloading/creating, witch, saving/reloading.

## Additional mount families without their own template, 04.10.2026

Basic entry route technically tested, built and installed when the game/Workbench is closed. The game initialization and riding function are still unconfirmed.

- [x] 24 private save/lobby candidates: Raptor, elephant, both iguanas, wolves, all 13 adult camel variants and other families. Removing each new animal gives exactly the original Savebody.
- [x] No assumption of third-party HP, equipment, level, name, active assignment or spawn position. New number, HMAC and double lock checked; Unknown schemes are rejected.
- [x] Eight mount UI tests: base entry is selectable, game test status and backup are displayed. Search, images, game process lock and resumption of the same request remain checked.
- [x] Desktop update installed and independently read back: 37 files including all 13 saves unchanged, 23 backups checked; no animal allocation.
- [ ] **MT09:** Underneath mounts Select the latest save and re-read it in the unchanged tested slot 0 standing 288 Entries available; Raptor/Elephant/Leguan/Camels/Wolves show “basic entry” · in-game test open.
- [ ] **MT10:** First register exactly one desired exotic variant when the game is closed. Load this save; Check stall/special mount menu, visibility, HP and recalls.
- [ ] **MT11:** Ascend, run, sprint and descend. Subsequently store and reload; Animals must be preserved and reusable. Existing horses/bears/lions and active mount check.
- [ ] **MT12:** Only after successful individual testing can additional animal species be added. Report error message and variant if initialization or use fails; Do not perform blanket quest/knowledge unlocks.

The older checklist below documents the previous status with 219 addable entries and only templates of the same animal group.

## Extended mount templates and portraits, 04.10.2026

The extension is installed and independently read back; in-game validation is pending.

- [x] Private Savecopy: assigned, named circus lion serves as a template; The new copy has neither name nor active assignment. Originals remain unchanged after complete back-testing.
- [x] Missing HP, damaged names and unconfirmed assignment types will be rejected. Fresh animal/item numbers, HMAC readback and double lock checked.
- [x] 299 Images decoded: 89 assigned exactly and 210 explicitly labeled as an example variant. 23 Entries keep the general icon.
- [x] Seven mount UI tests including detailed and zoom sample captions; No registration by clicking on the image.
- [x] Closed game/Workbench installed desktop update: 37 files including all 13 saves and existing mods unchanged; 23 Backups and desktop link tested.
- [x] After installation newest slot 0 pure reading checked: 219 available species (210 horses, seven bears, two lions); no change of name or assignment of animals.
- [ ] **MT06:**Read the latest save. With unchanged tested slot 1, 219 animals should be available, including two other lions. For other animal groups, the basic entry route documented above now applies.
- [ ] **MT07:** Select horse variant without own portrait: ‘Example of species’ and check for different colour/equipment; in the exact portrait this note does not appear.
- [ ] **MT08:** Only if a new animal is desired: register when the game is closed, load real save, check new and original animal including name, assignment, equipment, riding and saving/reloading.

## Image Preview, Search, mounts and NPC Arrows, 04.10.2026

Implemented, technically tested and installed when the game is closed; in-game validation open.

- [x] Real catalogue: subword search, hyphens, capital/case, multiple search words, exact IDs and optional regex mode.
- [x] Invalid regex patterns are declared; After correction, the list of results appears again. Categories remain combinable.
- [x] 57 uniquely assigned real animal portraits decoded from game archives; Missing images remain recognizable as a general symbol.
- [x] Private Savecopy: named unassigned stable animal as a template; his name is preserved, new animals have fresh numbers and complete back-testing.
- [ ] **UI01:** Click item image and animal portrait on the right, view large preview and close with Esc or x. Selection is retained.
- [ ] **UI02:** Search for any partial words in the entire item catalog, e.g. `holz`, `pfeil` or `ring`. Regex optional testing, e.g. `^(Gift|Explosions).*pfeil`.
- [ ] **MT04:** Under mounts Select the latest slot and re-read it. Check the available number; **Only addable mounts** hides entries without a template.
- [ ] **MT05:** If a new animal is desired: save and close game, register animal, load the same save, check stable, ride and resave/reload.
- [ ] **AM01:** Lightning arrow 1001315, Fire arrow 1001316 and Cold arrow 1001314 show monster ammunition indication; No output possible. Arrow 50001, Poison Arrow 50003 and Explosion Arrow 1001321 remain selectable.
- [ ] **AM02:** Equip matching arrow variant in the game and shoot normally with the Aeserion arc. Do not test with already assigned NPC flash arrows; These were left unchanged.

The item output confirms the inventory quantity; it does not automatically confirm that each internal game item is usable for the player.

## Fishing rod and fall protection, 03.10.2026

- [x] Original crab fishing rod in the latest `slot2` supplemented; Save completely compared.
- [x] 999 Places, existing items/sockets and quest progress.
- [x] Full native case routine and 684 protection cases checked for 342 keys.
- [x] Remaining figures get original fall HP loss; Forwarding unchanged.
- [x] New ASIs and three existing ASIs loaded together in isolation; The wrong exe is rejected.
- [x] When the game is closed with launch protection installed, 31 existing files unchanged.
- [ ] Start over Steam and load the latest **slot 2**; Check crab fishing rod in inventory.
- [ ] Try fishing rod at the fishing place: check automatic catching.
- [ ] `bin64/CrimsonNoFall.log` check for new READY line.
- [ ] Safe moderate fall with cliff, Damiane and Oongka test: no HP loss.
- [ ] Check the same with Leo/Rokade; Test black star on landing and descent.
- [ ] Normal combat hits should continue to pull off HP; then save/reload.
- Ten baits: not yet added; The actual item name is missing.

[Result, precise range and fuses](.local/fishing-no-fall-20261003/RESULTAT.md).

## Lion and Rokade: Pace, Endurance and Lion HP, 03.10.2026

- [x] Both animals are uniquely assigned, all six stages are structurally tested.
- [x] Twelve original/change cases checked with the game’s native data readers.
- [x] Twelve cut-off entries rejected; only 20 desired values changed.
- [x] Archive group `0049` installed and extracted when the game is closed with launch protection.
- [x] Older mods as well as all twelve save/lobby files checked unchanged.
- [ ] Start over Steam and re-call Lions and Rokade.
- [ ] Sprint longer and jump several times: Endurance should not run out.
- [ ] Compare speed on a level track; Configured multiplier 1,30.
- [ ] Check new maximum HP 1,350 at Leo; Existing current HP, if any. healing.
- [ ] Save/reload and re-check both animals.

[Outcome and limitations of offline testing](.local/lion-rokade-upgrades-20261003/RESULTAT.md).

## Request from Ben: Battles, 28.09.2026

- [x] Active battle step in the latest `slot1` identified and saved save.
- [x] Twelve save/lobby files finally checked unchanged.
- No modified quest to test: according to “only when safe” no intervention.
- [ ] As a regular way at Ben open the cattle farm administration, a registered
selecting animals and slaughtering them; then check the reporting step.

[Result and backup](.local/greymane-slaughter-20260928/RESULTAT.md).

## Current order: 30 hours of sleep and additional base, 28.09.2026

- [x] 288 native event/calendar cases with hook and day change checked.
- [x] Stranger time changes and shorter sleep durations remain unchanged.
- [x] Both sleep plugins are loaded together in the isolated loader test.
- [x] When the game is closed with launch protection installed; 17 Existing files identical.
- [x] Nine Save directory files backed up; Save and existing base unchanged.
- [ ] Start over Steam, check `CrimsonSleepDuration.log` for READY.
- [ ] Select the option still called “12 hours”: expect one day plus six hours.
- [ ] Check APPLIED log and actual timeline, animation and game world.
- [ ] Shorter sleep options and one-second cooldown test, save/reload.
- Five additional sockets and filling: **not implemented; according to user decision for the time being leave**, no in-game test is provided for this.

[Result, technical limits and backup](.local/sockets-plus5-sleep30-20260928/RESULTAT.md).

## Current job: Sleep cooldown 1 second, 27.09.2026

- [x] Own 1,000-ms-rule including border cases and existing locks tested.
- [x] Both original functions of the current EXE are tested in isolation with test data and hooks.
- [x] Automatic charging via ASI Loader is tested in the private test process.
- [x] When the game is closed, installed, files read back, nine save directory files backed up.
- [x] EXE, existing overlays/registration and save directory files unchanged.
- [x] READY entry from a real game start found in `bin64/CrimsonSleepCooldown.log` (28.09.); Practical test separately open.
- [ ] Sleep, wait at least a second and sleep again.
- [ ] If the lock is in place, reopen the interaction after one second.
- [ ] Optionally check waiting at the campfire; then save and reload.

[Result and exact test manual](.local/sleep-cooldown-1s-20260927/RESULTAT.md).

## Current order: Grey mowing and small bones, 27.09.2026

- [x] Twelve save/lobby files backed up; +250 small bones.
- [x] Complete comparison, inverse examination, launch protection and reading back passed.
- [x] Gray-mowed quenching stands only read; Progress unchanged.
- [ ] Latest `slot1` load and 253 check small bones in the backpack.
- [ ] Regularly store/reload and control quantity.
- [ ] Quest unlock: open, not yet technically verified/implemented.

[Quest Finding and backup](.local/greymane-unlock-bones-20260927/RESULTAT.md).

## Current order: donation reserve and rings, 27.09.2026

- [x] Current mission states read: Wood and weapons open; Money, food, stone completed.
- [x] 1,616 excellent wood, 821 ivory and two rings complemented; for each 300 material reserve.
- [x] Passed standard stacks, ring root stages, complete comparison, inverse testing and protected commit.
- [ ] Latest `slot2` load: 2,016 excellent wood, 992 ivory,
Check the darkness +3 and Demeniss seal +0.
- [ ] Check donation values at Carl: wood 19, ivory 24 points per piece.
- [ ] Donate up to 1,316 wood and 521 ivory for the final destinations; Keep the reserve.
- [ ] Check both quest completions, ring refinement and save/reload.

[Result and backup](.local/camp-donations-reserve-20260927/RESULTAT.md).

## Current order: Camp stocks and wood, 27.09.2026

- [x] Active item IDs and food donation value checked; backed up all twelve save files.
- [x] Each 400 of the three wooden materials and 100 Large meal is entered.
- [x] Complete comparison, inverse byte testing, launch protection and reading back passed.
- [ ] Latest `slot1` load: check each 400 wood and 100 large meal.
- [ ] At Carl, check the food donation value of the meals (calculated 648 per piece).
- [ ] Donate food and check progress at “A table full of plenty”.
- [ ] Regularly store/reload and check remaining quantities and progress.

[Result and backup](.local/camp-provisions-wood-20260927/RESULTAT.md).

## Current order: all III socket items and tempo, 27.09.2026

- [x] All 41 active III-Abyss-IDs checked and supplemented once in the backpack.
- [x] Rush III inserted in the first worn boot base; Devotion I returned.
- [x] Complete save comparison, inverse byte checking, launch protection and reading back.
- [ ] Load the latest `slot2`; 41 Add, rush III in the boot and surrender I in the backpack check.
- [ ] With current equipment, check status value **Motion speed 15** and running efficiency.
**30 was not set up**; 15 is the allowed regular avoidance target.
- [ ] Use a III stone in the witch, save/reload and control progress.

[Result, all quantities and backup](.local/abyss-iii-speed-20260927/RESULTAT.md).

## Current inventory order: inventories and Abyss equipment, 26.09.2026

- [x] Eight item IDs checked against the active table; 12 save/lobby files backed up.
- [x] Each 500 of the five stocks and each 10 of the three Abyss equipment is supplemented;
Passed standard stack, complete comparison, inverse testing, and protected commit.
- [ ] Latest `slot0` load: 715 Small bones, 500 platinum, 500 plentiful
barbecued meat, 505 improved Kraftfist herbal pastilles, 512 diamonds as well as each 10 Destruction III, Reinforcement III and Stormwind III test.
- [ ] use meat; Test pastilles at the next required resuscitation.
- [ ] Use Abyss equipment on a witch and check the respective bonuses.
- [ ] Regularly store, reload and control residual quantities as well as progress.

[Result and backup](.local/supplies-500-20260926/RESULTAT.md).

## Current inventory order: seven materials, 26.09.2026

- [x] Je 300 Iron ore, copper ore, azurite, silver ore, garnet, gold ore and epidote
Added to the freshly backed latest `slot0`. Complete comparison, inverse byte testing, launch protection and reading back passed.
- [ ] Latest state load: 320 iron ore, 433 copper ore, 315 azurite, 300 silver ore,
Check 336 garnet, 300 gold ore and 311 epidote in your backpack.
- [ ] Use, store/reload and check residual quantities and progress when forging.

[Result and backup](.local/ores-300-20260926/RESULTAT.md).

## Current inventory order: Refining materials, 25.09.2026

- [x] Current recipe/item tables checked, all twelve save/lobby files backed up.
- [x] Each 200 piece of 27 material types supplemented, total 5,400; native stack boundaries,
passed unique IDs, complete comparison and inverse byte testing.
- [x] Newest `slot1` from 21:28 watch replaced with closed game and read back;
Installation, remaining saved games, existing abilities and 999 places are obtained.
- [ ] Loading this latest state: e.g. 200 iron ore, 239 copper ore, 205 blood rocks,
251 Skoleziterz, 204 Abyss artifacts and 200 Check Aeserion's scale in the backpack.
- [ ] Refining the weapon and armor at the blacksmith, testing material extraction;
Check Aeserion's scale for usability at level 10.
- [ ] Save and reload: Received quantities, equipment, skills and inventory.

According to their native stacking boundary, Aeserion's scales are present as 200 individual pieces. [Full list of materials and backup](.local/refinement-materials-200-20260925/RESULTAT.md).

Status: 23.09.2026. Overview of functions: [PHASENSTATUS](PHASENSTATUS.md).

**The general phase tests remain set back.** For the separately commissioned inventory fix, a targeted in-game test runs. I carry out the automated checks during development. Below is the later acceptance with concrete expectations; still missing functions are expressly marked.

## Separate inventory order from 23.09.2026

**Additional order from 25.09.2026, currently withdrawn:** The addition of 70 Abyss items did not correspond to the desired skill resource according to the user.

- [x] Object identified from active game data; Twelve saves/lobbies.
- [x] Seven new stacks of 10 with unique IDs; complete semantic
Comparison, inverse byte testing and independent codec testing.
- [x] Live save and lobby read back; other ten files and mod installation
Unchanged, 999 Inventory locations and existing items are preserved.
- [x] At 15:31 clock completely withdrawn: both files and original
Change times byte accurately restored, originally received five copies.
- [x] User names Abyss links and confirms the addition for all three
Characters; the previous allocation of inventory items remains withdrawn.
- [x] At 15:36 clock each 70 additional units in all three abilities accounts
Entered: 0/16/42 → 70/86/112. Complete save comparison, inverse examination and reading back passed; stored capabilities, inventory, lobby and installation unchanged in the file comparison.
- **Misplaced in-game validation according to users:** The ability tree has been
reset after the +70 addition. Cause not clearly clarified.
- [x] On user request at 15:52 clock each **150 additional** units added:
70/86/112 → **220/236/262**. Six fixed fields with no data shift changed; all other unpacked save bytes are identical. refund counters examined against the current EXE; backup and read back completed.
- [ ] Slot 0 from 25.09., 01:23Clock loading and credits in the abilities menu
Kliff, Damiane and Oongka. In addition, check whether the skill tree is preserved; The previous reset is not proven to have been resolved.
- [ ] Link to learn to use, save/reload a skill
and check remaining balances as well as learned ability.

[Take-back report and collateralisation](.local/abyss-artifacts-rollback-20260925/RESULTAT.md). [Current addition of Abyss links](.local/abyss-links-all-150-20260925/RESULTAT.md).

**Latest Additional Order, 23:42Clock:** In the latest version `slot100` from 23:35Clock, 500 keys and 100 additional Hernand coins are entered.

- [x] Freshness backup, complete semantic comparison, inverse byte testing,
independent unpacking, unique IDs and 113,590 object pointers checked.
- [x] stacking table updated via the existing journal; Previous mod files
backed up, updated to project copy tested, installed files read back.
- [x] Gold bars and 999 places received; 10 other save/lobby files byte-identical.
- [ ] Latest stand loading: 500 keys in the backpack and 103 Hernand coins at
the special objects, one stack each.
- [ ] Use a key and insert a coin at Turnali;
then save/reload and check remaining quantities as well as 999 places.

[Report and fuses for key/coins](.local/keys-hernand-2976/RESULTAT.md).

**Current status, 20:51Clock:**Build 25477059 / EXE 1.0.0.2976; The newest slot 1. The earlier quantities/slots below document the first orders.

- [x] Current progress secured, 843 remaining gold bars of five
merging entries into a stack; 38,401,01 Silver and each 200 bag.
- [x] Gold bar stacking limit 1,000 and inventory maximum 999 in the new build
Installed, save extension 949. Archive and Save independently read back.
- [x] Complete save comparison, byte-accurate inverse testing, Apply/Restore
insisted on project copy and 22 library tests.
- [x] starts the game; latest slot 1 loaded: a stack with 843 gold bars
and 999 total places. User confirmation: “Yes, both are true”.
- [ ] Regularly store and reload: quantity, stack and capacity are retained.
- [ ] Gold bars use/sell and new items above the former
240 place limit; then save/reload.

[Current report](.local/inventory-stack-2976/RESULTAT.md).

- [x] When the game slot 2 is closed and secured by four stacks of 100 bags
and 30,000 silver. New money stock: 30,231,01 silver.
- [x] Free seats, unique IDs, lobby counters, complete comparison of
other data and integrity checked after reopening. Eight other files unchanged.
- [ ] Load the modified slot at the next desired in-game test: total
200 light/200 heavy copper bags and control the money stock.
- [ ] Use one bag each; then store regularly and again
loading. Remaining quantities, balances and remaining inventory must be retained.

The game was not started for this. [Report and original assurance](.local/inventory-currency/RESULTAT.md).

**Consequent order:** The latest slot 0 contains the previous bags and the balance after another regular saving process. There were additionally 1,000 gold bars entered; Pass new backup and re-integrity check.

- [ ] At the later in-game test Slot 0 load: 1,000 Gold bars and continue
Check 200 light/200 heavy copper bags and 30,231,01 silver.
- [ ] use/sell, regularly store and reload gold bullion;
Check remaining amounts and correctly changed credits.
- [x] 999 Backpacking set up: table maximum 999 installed,
Save extension set to 949 (50 + 949). Fuses, complete byte comparison, independent unpacking and integrity checked after reopening. Items and money as well as the nine other save/lobby files remain unchanged.
- [ ] Slot 0 Load: Check display of 999 total seats. Articles above
of the previous 109 places and sort, later also above the original table upper limit 240. Regularly store and reload; Capacity, items and quantities must be maintained.

The table change consists of two Apply/Restore cycles on a project copy and is now installed; This is not a in-game test. [Final report and backup](.local/inventory-999-save/RESULTAT.md).

## Current requirement: The game has been updated

Installed is now **Steam-Build 25477059 / EXE 1.0.0.2976**. The four tables for the current inventory fix have been tested; Full general app support must be updated separately. The following proofs apply to the previous build **25455892 / EXE 1.0.0.2949**: **Workbench v0.5.9 supports its tables and still the previous build 25381195 / EXE 1.0.0.2944.** EXE/metadata files captured by 38 are changed 24. This is not a complete content check of the installation.

61 table, icon and language files used are compared; all 14 indexed table pairs consist of the byte-like round trip. During the mod comparison, all unselected stages including the two new entries are retained. The complete native repair is still not released. The historical test host refuses old functional copies from this EXE; The new sample has its own separate functional and layout proofs. An old test report or backup is not a basis for Apply/Restore over the new Zustand installation. [Proof of build](docs/BUILD_SUPPORT.md).

## Already checked automatically

- [x] App testing for v0.5.9: 181 Rust tests, seven front-end unit tests and
61 UI flows; In addition, the separate comparison of the previous Advanced tables. The integration tests read the new build 1.0.0.2949.
- [x] Built desktop app v0.5.9: preview with 3,359 changes over 13 tables,
Two apply/restore cycles on one project copy, no page errors. Game metadata and original test report remain unchanged.
- [x] Both build identities remain separate. Mixed backup reports
Metadata, incorrect build ID, missing files or incorrect sizes are rejected.
- [x] 52,082 Stages and 1,098 Quests remain fully readable. Modifications
concern only the two chosen patrols; Quest files remain outside the overlay.
- [x] Current repair development module 0.23.0: nine CTest suites,
MSVC release with warnings as errors.
- [x] 1,529 Action Conditions, 519 Reader/Reference Conditions,
25 Lock group conditions and 55 new registry adapter conditions.
- [x] Incorrect acquisition, old/full identifier, invalid bonding,
partially acquired references and release in reverse order.
- [x] Each source uses its own release method; No global
Method binding, which can be overwritten by another context.
- [x] Native acquisition/locking/inventory/event/storage object sample on 1.0.0.2949: 454 scenarios,
4,812 native calls and 206,786 conditions, including repeated byte/table checks. True Windows locks stop reference search during a write access; Subsequently removed entries are rejected.
- [x] Error with the second owner returns the first reference. Invalidity
Proofs are not considered possession despite the included pointer. Normal/user variants, old generation identifier, missing entries and delayed release are checked.
- [x] The reader uses only held owners and does not read an unprotected registry
or unobsessed replacement selection. If the owner lock is blocked, no items are read; Errors return all locks before the references.
- [x] Inventory, equipment and sockets shared under real-world control
owner locks; Changes of client selection, identifier, data and block binding are checked. The reading fields used now have their own 2949 documents.
- [x] Original inventory/owner getters of the new build match on identical
private sources agree with the reader. Four native wind fragments tested.
- [x] Current client selection and equipment slot selection natively checked; the
Equipment testing uses Delta 0 and does not yet prove any repair events.
- [x] Unknown/unspecified reader builds are rejected before purchase and reading.
- [x] Original Updater also reproduces the harmful positive in the new build
base repair case; it is not used as its own repair function.
- [x] Event preparation keeps the previous Broken-Zustand and takes into account
No-wear; Items carried must not use the equipment path.
- [x] Original Server Notifier and Client Ack on private repaired items:
Proper effect/UI calls, no-wear, socket maintenance, and complete after-values. Transport, effect processing and UI receiver are artificial test dependencies.
- [x] Missing/false/double feedback, wrong client slot, partial change,
Missing event documentation and changing sessions do not produce repair success.
- [x] Native item copies have their own socket/additional data. repeated assignment,
Self-assignment, empty data and both allocation paths receive the source and release all copy buffers. Additional 29 conditions check the owner contract.
- [x] Equipment events now use these real native aftercopies:
Copy under owner locks, notification after unlock, release thereafter.
- [x] Original marking of modified equipment slots: duplicates, collisions,
Growth and both allocation paths. The event sample shall be marked before unlocking; Consumers and native spreadsheet cleanup are now also integrated.
- [x] Category held: owner and locks remain for planning/pre-examination
obtained. Refresh, foreign thread, outdated batch and modified session checked; Errors release all locks before the references.
- [x] Field recorder: 35 Scenarios and 202 Conditions. Total batch pre-tested;
then only changed durability words are written in both Zustandcopies. Obsolete/overlapping/unwritable sources, partial errors, empty slots and metadata checked; Eight native integration cases use real memory blocks of their own.
- [x] Common flow: 43 Scenarios and 273 Conditions. Both subsequent copies
Each item is created before the first store; all means of reporting shall be available. Copying, writing, marking and reporting errors do not produce success.
- [x] The 18 native equipment cases use this adapter including
Original copies, scribes, slot marking and server/client event path. Confirmation reads subsequently under newly acquired owners; false slots or conflicting Zustandcopies are rejected.
- [x] Batch completion after all messages: lack of binding prevents
letters; Subsequent closing errors or session changes will not be successful.
- [x] 24 native cases for processing marked equipment slots. Exact
UID/main durability pairs, 64 slots, clear/reuse/release and error checked after already emptied list. The persistence interface remains a test receiver; The actual memory confirmation is thus not proven.
- [x] Empty and partially stored base spaces are checked: only
existing data sets are read, even if more base spaces are possible. Incorrect additional data and oversized allocations are rejected before copying.
- [x] 36 native inventory event cases: original client ax with absolute
Main values, invalid/empty slots and exact UI call. Eight cases link the Ack with copies and field writers; UID change, missing/double delivery and completion errors prevent success. Server, UI and persistence remain test recipients.
- [x] 36 additional cases for original inventory package serializer and stream selection:
Exactly ten useful bytes, no UID/socket/order data, missing receiver/buffer, inappropriate channels and failed pool reference. Ten transactions connect the sender to the client-ack; Errors, missing/double delivery, UID or Actor deviation do not count as success. Real transport/storage binding open.
- [x] New EXE is rejected by the old native test host before function execution.
- [x] 19 native cases with full SQL execution function and original
Request Acquisition: bypass, old errors, diagnostic deadlines, zero hands and integration with the slot consumer. Success is possible without reading or saving the request; This return is expressly not counted as a storage confirmation.
- [x] 38 native cases for Item → Memory Object → Item: Main values 0/damaged/full/
Sentinel/above Maximum, No-Wear, free and partially occupied sockets. 14 Cases use actual repair plans and native post-copies. dye data, reset/reuse, source conservation and full release checked. Both private allocation modes exist. This does not demonstrate a write to disk.
- [x] 39 native memory outflow cases: all skip/dirty/status combinations, four
Error levels, repetition, missing backend, Steam approval check and growth of three queues. Original dispatchers and markers run on private data; File/platform recipients remain artificial. [Boundaries](runtime/repair/SAVE_BACKEND_2949.md).
- [x] Data reading selects the new reading schema; The search index leads to the appropriate
Scheme ID. This does not grant vanilla approval or live apply approval.

The historical full native host on the **old** build had 203 views and 3,859 conditions (module 0.6.0). The current sample complements own evidence of inventory/equipment access and parts of the sequence of events; it is still not proof of a complete repair in the game. [Current test limits](runtime/repair/INVENTORY_EVENTS_2949.md). The engine threadmap and final object cleanup remain test dependencies. Details: [TESTING](docs/TESTING.md).

## Still to be technically tested by me

- [x] New table/language files, parser round trips, changed records, and
Assign mod diffs. [Comparison](docs/BUILD_SUPPORT.md).
- [x] Version selection before testing a modbackup: The supported new
Build loads its original tables; a test stock with an unknown PE version is rejected beforehand with the version error.
- [x] Protected client/server search with actual reference methods and
Share on the new build in isolation.
- [x] Use references contained directly for inventory collection; Not a second
unprotected search. Acquisition, owner locks and readers check for private data together.
- [x] Native functional limits and read layouts for inventory, owners, current
Select, occupy equipment and base on the new build.
- [x] Equipment events are properly prepared and the server/client call paths
with private transport, effect and UI receivers.
- [x] Original item construction, deep copy and destruction with own
Check allocations and integrate them into the event sample.
- [x] Integrate original slot marking into the equipment sample and create a
Implement continuous protected capture for repair pre-testing.
- [x] Complete durability field step for items carried and equipped
Implement with preliminary/review and treatment of unclear partial errors.
- [x] Field recorders with pre-prepared native post-copies
and to connect range-dependent message adapters; Test the equipment path natively isolated.
- [x] Connect the original inventory client to the private common flow;
Separately reject UID change and incomplete feedback.
- [ ] Server-side inventory messages, mixed orders and complete real
Check effect/UI/reporting pathways. The client-ack alone does not confirm any storage.
- [x] Consumers of slot messages and native cleanup in private
tethering equipment sequence; Empty list alone is not considered a confirmation of success.
- [ ] Connect actual persistence and complementary base path. The evidenced
Equipment consumer passes only UID and main durability. The examined socket SQL helper transmits the item identifier. The separate ItemSaveData path also adopts main and base values in the native sample; the actual save process is missing.
- [x] Run original item/storage object conversions on private sources:
Principal values, sentinels, multiple plinths, vacant places, identity and staining dates; Repaired copies and reuse.
- [ ] Additional optional item data and actual engine settings
Test for the number of caps. The sample itself binds these settings; The loader generates five data sets, even if the logical limit is smaller.
- [ ] Connect regular generation/serialization and termination of storage objects;
the conversion in the main memory alone does not yet confirm a stored task.
- [x] Check file helpers with private buffer/handle receivers: 34 native cases
for preparation/opening/write/flush/close error, empty output and lifetime in both allocation modes. [Evidence](runtime/repair/SAVE_FILE_2949.md).
- [x] Native buffer preparation, allocation and LZ4 compression with sizes up to
131,072 Check byte; Include errors, output reuse and retrieved entries. 66 Cases existed. [Evidence](runtime/repair/SAVE_ENCODING_2949.md).
- [ ] downstream processing, including integrity/encryption, which:
Connect queue/backup/renaming path and robust terminating document. After opening fails, new attempts must use fresh raw data. Flush/close errors can still generate success and emptied buffers in the original code. An internal report of success is not enough.
- [ ] Identify repair changes and complete event/effect paths on the new build.
- [ ] Resolve current players and managers on a busy engine thread;
World/character change, reuse of identifiers and TLS mode change must not accept foreign or obsolete owners.
- [ ] Complete repair transaction including inventory/equipment events,
Develop and test derived effects and unambiguous feedback.
- [ ] New modules against errors, partial cancellations, reapplication, take-back and
Check for updates. The test data is outside the game installation.
- [ ] Check phases 6–8 automatically after their implementation.

## Later test order for you

Only after support of the then installed build and completion of the development: first use app, then check B0 with a small change, then each module individually and last combinations. Close game for installation changes; Check the effect after the next start. Take back also when the game is closed. The Workbench does not write saves.

For a comparison, use the same initial situation and record before/after values. A single random drop occupies no multiplier. In case of problems, case ID, app/game version, settings, actual result and, if necessary, capture a screenshot.

### Phases 0–3: Data and operation

|ID|Test|Expected result|
|---|---|---|
|A01|Select supported installation; Then check an unknown build|loading appropriate data; Unknown build is clearly rejected, no seemingly valid data or changes|
|A02|Search for German item, combine filters, browse, open details and icon|Correct data set, no reversed/outdated details, operation remains fluid|
|A03|Changing Language and Repeating Search|Names/details match the language; Missing translations and raw fields remain visible|
|A04|Use available item/recipe links and JSON export|Proper goals and values; Export lands in the project, no change to the game|
|A05|Calculate well-known recipe with one and more target amounts|The material tree and the overall requirements are correct; Batch rounding is traceable|
|A06|Change inventories, recipe alternatives and shared materials|Inventories are deducted only once in total; No negative quantities or double counted residual requirements|
|A07|After a supported game update, reopen the data|Data and search index are part of the new build; old backup reports do not receive an automatic write authorization for the new installation|

Unknown merchant/found location allocations must appear as unknown and are not a successful source comparison. The complete development of these links is still lacking.

### Phase 4: Apply, Shops, Drops and Trust

|ID|Test|Expected result|
|---|---|---|
|B01|Check Small Change in Preview|Only selected field/module and related files change; Preview does not write anything into the game|
|B02|Try Apply/Restore while playing|blocked before any change; The game continues unchanged|
|B03|When the game is closed, apply small change, start, check effect, close and withdraw|The selected effect is present; Takeback confirms the basis of the current build|
|B04|Reapply the same setting, then use another factor and finally deactivate the module|No multipliers stacked on top of each other; Deactivated effect disappears|
|B05|A supported dealer: small range, stock, purchase, change of day, exceptions|Proper purchaseable items and replenishment; Unique offers remain unique, no duplicates|
|B06|Drop chance and quantity first separately, then check together|fitting quantities; Compare chances over a sufficient number of equal attempts. Manual warranty only applies to the selected supported set|
|B07|Compare the same confidence action with the same NPC|Positive growth matches the factor; Negative penalties remain unchanged|

Update, power failure and crash cases are first checked automatically in isolated installations. You should not provoke a crash or power outage. All items at all dealers and an automatic boss assignment are not released.

### Phase 5: Extended modules

|ID|Test|Expected result|
|---|---|---|
|C01|Compare a selected spawn group and an exception|Acceptance of spawn numbers/limits; Excluded groups remain unchanged|
|C02|Initiate supported patrol/re-establishment rule|The chosen rule changes in a traceable manner; This is not a test of a general NPC respawn timer that is still missing|
|C03|mount in city, multiple regions, ascents, interaction and quest transition|Documented releases/durations are effective; No stuck Zustand or unexpected loss of interactions|
|C04|Call Blackstar, call again, check ride duration and region boundaries|Cooldown/Duration as set; separately documenting general borders and special zones|
|C05|Check inventory and any storage area used with a moderate change of space|Purchasing, picking up, moving, sorting and reloading work; No disappeared objects|
|C06|Stack, share, purchase, store and reload materials/consumer goods|Quantities are maintained exactly and stack boundaries are correct|
|C07|Examine experimental instance items separately|Durability, enchantments, plinth and identity are preserved per specimen; No mixing|
|C08|Weapons/tools/sockets load and compare actual durability|In the case of no-wear, no actual loss in value; A larger maximum display alone is not enough|
|C09|Compare sprinting, climbing, flying and various skills with cost factor 1, 0,5 and 0|Consumption fits the supported category; regeneration is maintained; Keeping unknown costs separately|
|C10|Changing a skill and global cooldown, then setting an exception|The selected priority is effective; other skills remain unchanged according to their attitude|
|C11|Change, equip and reload an item with confirmed Stat/Buff/Enchant|The display and actual effect fit; No endless loop, lost properties or unintended changes|

Size limits, special containers and instance items have not yet been universally accepted. Do not use immediate maximum values as a first test.

### Free repair: only after complete play connection

The repair is currently locked and not yet manually testable.

- [ ] **R01:**Repair existing damaged individual item: correct target value,
no consumption of money/materials; Quantity, UID, enchantment and base occupancy are preserved.
- [ ] **R02:** Separately and together repair items carried and equipped;
Test the main item and damaged base inserts independently.
- [ ] **R03:** Undamaged items, items without durability and empty slots result
no unnecessary changes; Consumed/destroyed specimens are not newly produced.
- [ ] **R04:** Combine with activated No-Wear; Values, representation and later
Use must correspond to the defined repair objective.
- [ ] **R05:** Repetitively trigger quickly as well as equipment/character changes and
Check shop/region change: no double repair, wrong owners or hanging orders.
- [ ] **R06:** Repair success must be in inventory, equipment, derived effects
and after regular saving/reloading through the game. Additionally, close the game normally, restart and compare main item as well as each socket. A success message or immediately correct display alone is not enough. Record before/after values and values after restart for the same copy.
- [ ] **R07:** If rejected, no change; If the result is unclear
clear message and no automatic repetition. Technical errors are first simulated by me in the test host.
- [ ] **R08:** Check each controllable protagonist individually, then with
present companions and when riding: Repair only affects the selected player area, no companion, NPC, dealer or warehouse items.
- [ ] **R09:** Retrieve after regular reloading: An old order must not be
enter the new session, even if character and objects look the same.
- [ ] **R10:**Repair a still existing item with durability 0 and with a
Compare only partially damaged items: Equipment values and buffs must fit directly, without unplugging or reopening the inventory. Test the same with pure base repair and no-wear activated; No duplicate effects.
- [ ] **R11:** Two copies of the same item with different bases and
Possessing properties: first targeting one, later repairing both. Each identity and its characteristics must be preserved; no transfer between specimens. Then regularly reload and compare again.
- [ ] **R12:** Repair multiple damaged equipment slots in one order,
then immediately check values/effects and trigger them again. Each selected slot updates exactly once; Already full items remain unchanged. Even after moving out / dressing, region change and regular reloading, the values are correct.
- [ ] **R13:** An item with available but unoccupied base slots and a
Repair with only partially occupied seats. Main value and existing bets are right; Free places remain free. No new bases or objects are created. After that, store/reload normally and compare occupancy and properties.
- [ ] **R14:**Repair a colored item with several different colors,
Regularly store and reload. All colors/properties remain with the correct specimen; a subsequently repaired other item does not take over old data.

For R02/R06/R10, use the baseline individually:

|Starting position|Check after repair|
|---|---|
|Main item damaged, base intact|principal value full; Base, quantity and characteristics unchanged|
|Main item still present, durability 0|principal value full; Equipment effects previously deactivated are effective again|
|Main item intact, base damaged|full bases; no base disappeared, no additional main item effects|
|Main item and base damaged|Both independently fully; Display and actual effect are correct|
|No-Wear aktiv|Achieved a defined repair goal; No incorrect or double effect activation|

### Phases 6–8: after their implementation

|ID|Test|Expected result|
|---|---|---|
|D01|Create profiles, export/import and change normal ↔ farm|All settings complete; Drops ×10/Spawns ×3 adjustable; No change during the current game|
|D02|Importing a faulty profile|understandable refusal; Existing profiles are maintained|
|E01|Select Save, compare known quest progress and continue playing/saving in the game|Proper status and updating; no write operations or blocking of the game by the reader|
|E02|Unsupported/damaged/just incompletely written save read|No invented 0 %; Error detectable, last valid status is maintained|
|F01|Compare Card Points in Multiple Known Places|correct positions also outside calibration points; Search, levels and details fit|
|F02|Compare collection status if supported by Saveformat|Completed markers agree; Missing data is kept as unknown|

The optional farm hotkey is its own later topic and is not yet part of this acceptance.

## Simple results protocol

|Datum|Fall-ID|App-/Spielversion|Setting/Preliminary value|Expected|Observed|Passed / Error / Open|
|---|---|---|---|---|---|---|
| | | | | | | |

Extensive developer credentials: [docs/TESTING.md](docs/TESTING.md).

## Additional order Black Star – installed on 01.10.2026

- [ ] **BS01:** Start the game over Steam, load the latest save; normal take-off without crash. Check current READY line with `early summon action support` and `14 hooks`; Do not use older lines as proof.
- [ ] **BS02:** Black Star released and called again; It's about a second cooldown, not a double instance. Repeat after storing/recharging.
- [ ] **BS03:** Several minutes of flight, acceleration and fire breath; Endurance practically does not run out, life points remain normal.
- [ ] **BS04:** Summon in previously locked free outdoor area; Examine rejection at home, cave and interior border.
- [ ] **BS05:** Other mounts and existing sleep/inventory mods work unchanged.
- [ ] **BS06:** With sustained flight, Black Star remains summoned for more than ten minutes. The new base duration and the existing residual timer in slot 1 were set to 1,000 minutes; Save/reload and reconvoke also check. Full 1,000-minutes-in-game test has not been performed yet.
- [ ] **BS07:** Fly into one of the red areas from the screenshot and fly low in a city away from roads; no dropping by the previous area/city examinations.
- [ ] **BS08:** Black Star at exactly the last blocked outpost in the red area. The user reported with the previous version further approximately "mounts can not be called here"; This reduction has so far failed. If rejected again, hold location and message, then save and close the game completely. Evaluate new `GAME_ERROR`-/`ATTEMPT`-/`REFUSED` lines from `bin64/CrimsonBlackstar.log` The current version captures previous actions; Universal success is still unconfirmed.
- [ ] **BS09:** Fly over the previous altitude limit; No dropping by the height test. Other mounts keep their previous rules.
- [ ] **BS10:** Test outer surface on a previously locked gimmick/socket; Interiors, lack of space and invalid spawn positions continue to be rejected. No double black star instance.

The summoning time is 1,000 minutes. urban, regional and elevation tests are approved for black stars; other reasons for flying away or refusal by expiration/placement checks are still possible. The new version is technically tested, the in-game validation remains open. Details and technical evidence: [Black stars](docs/BLACKSTAR.md).

For the next targeted diagnostic step **BS01 and a BS08 attempt** at the same location are sufficient. The further cases remain part of the later complete decrease.
## Item selection / Live spawner, 03.10.2026

- [x] Real catalog with 6,816 items, icon, groups, combined search and exact numeric ID checked.
- [x] retrievable basic values and refinement levels; User interface and desktop build checked.
- [x] Passed eight browser tests for catalogue/selection; Passed core tests.
- [x] Reading sample and common loader tested with the four existing mods; Foreign EXE rejected.
- [x] Probe observed a player inventory context on Realm 1. No proof of writing.
- [x] New Runtime: 13 Arguments/original result, competing queue, UUID protection, incorrect play instance, cancellation, expiration and error lock checked.
- [x] Twelve tests with actual build-2976 constructor/converter/destructor in your own process; Quantity 1/1,000 and 0–5 caps. Definitions/Allocator are test data.
- [x] Real Windows Pipe Rust/C++ tested in its own test process; no play connection.
- [x] Eleven browser tests, including quantity checking, one-time sending, cancellation, and restored request ID in case of lost response.
- [x] New ASI and desktop version installed and read back when the game is closed; Desktop shortcut updated. 33 existing files unchanged, 19 files freshly backed up.
- [x] Automatic output: actual native task dispatcher/wrapper tested with MinHook in its own process, reference lifetime, original call, nested tasks and double-assignment protection. Native registry/inventory dependencies are test data.
- [x] protocol 2, missing/wrong context, load cancellation, character identifier and pause/alt tab checked; Production build and six ASIs passed in the common loader.
- [x] Automatic ASI and desktop version with closed game/Workbench replaced and read back; 36 existing files including 13 saves unchanged, 23 backups created.
- [ ] save load and play normally for short: connection is automatically ready. No cancellation required. New `ARMED` and `CONTEXT` lines in `CrimsonLiveItems.log` confirm the entry.
- [ ] First enter **an iron ore** by button, **without canceling anything**. If it waits, return to normal play. See crowd, save normally, finish game and reload the same save. Quantity must be preserved.
- [ ] Then check stack/full inventory/pause/alt-tab/character change, cancel waiting request and let it expire for paused game 30 seconds.
- [ ] During a waiting request to change the character or load another save: order is rejected, no allocation to another character. After missing context, the connection must be ready again.
- [ ] Test higher quantities and other types of items only after this detection.

The new connection is installed and loaded at the next normal game start; The real inventory/store/reload test is still pending. Details: [LIVE-ITEMS.md](docs/LIVE-ITEMS.md).

## Extension bag/arrow/mounts/knowledge/quantity, 03.10.2026

Passed technical examinations; new version installed on 04.10.2026 with closed game/Workbench and read back. Match acceptances remain open:

- [ ] **LI01:** Give extra large bag 6003, amount 1. No old general error; Pocket visible, save and reload the same slot.
- [ ] **LI02:** Flash arrow 1001315, set 100. Confirm quantity change, store/reload. In case of unclear or partial result, no new request for suspicion.
- [ ] **LI03:** Empty set field, 100 type, mark/replace and paste. With actual stacking limit 10 and enough space: total +100 pieces in a maximum of ten new stacks; Existing ones can be filled. Moddefinition may differ from the catalogue.
- [ ] **LI04:** Compare read and unread writing with knowledge reward. Choose the right slot; after reading/storing “Re-read in knowledge”. status changes; Document without clear assignment shows unknown.
- [ ] **MT01:** Register a suitable ordinary mount type when the game is closed. Loading proper slot, checking stable and new animal; existing animals, equipment, abilities and quests unchanged.
- [ ] **MT02:** Riding, saving and reloading new animal. Same species then marked as present and not doubled.
- [ ] **MT03:** Ongoing game or missing template block registration. If the answer is lost, check the same request; no double award by repetition.

backup and limits: [mounts](docs/REITTIERE.md). The installation alone does not add animals or items.

## Complete ability trees, 10.10.2026

Entered directly in the latest save for all three characters; Passed technical examinations and reading back. in-game validation open:

- [x] 140 uniquely assigned different abilities from 142 main tree entries;
all at the intended maximum level.
- [x] Full comparison of all memory blocks, position references,
pass independent HMAC/decompression test and byte-accurate reversal test.
- [x] Get free points, inventory, equipment and quests. When the game is over
with launch protection installed; Originals backed up and final file read back.
- [ ] **SK01:** Start the game and select **Continue**. Loading works;
Current equipment, inventory and quest progress.
- [ ] **SK02:** Open skills tree of Kliff, Oongka and Damiane. Skills
and stages fully learned; Examine health, endurance and mental maximum.
- [ ] **SK03:** For each character, a previously missing fighting ability and a
use mobility capability; Behavior and effects are right.
- [ ] **SK04:** Regularly save, close game and repeat the same status
loading. Unlocks and free points are retained.

Details and backup: [Capability unlocking](docs/FAEHIGKEITEN-FREISCHALTUNG.md).
