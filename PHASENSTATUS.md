# Crimson Workbench – What is finished, what is missing?

## v0.6.3: English interface and automatic catalog language (11 October 2026)

The interface, installer and project Markdown documentation use English. New desktop preferences default to Automatic: the selected installation's Steam manifest determines the installed catalog language, with English as the fallback. Existing manual preferences remain valid. Separate in-game overrides and Epic/Game Pass language settings still require manual selection. The single-EXE setup from v0.6.2 remains in place.

Local checks: 17 frontend unit tests, 92 browser tests and 214 Rust library tests passed; five fixture-dependent Rust tests were intentionally skipped. The subsequent three setup tests and socket regression tests passed after final label corrections. Clippy passed with warnings denied. Windows packaging and cross-platform CI are verified separately on the PR. No game installation or save was changed for this update.

**Socket display, phase 5, 05.10.2026:** extra sockets shows in the selection list the basic bonuses, i.e. Special effects. The selected stone contains all values, effect levels and the description; Already stocked extra sockets also show their bonuses. Attack/defense are converted from thousandths, speed and critical hit chance are displayed as levels. 190 Stones read from the tested local ItemInfo: 30 with direct values, 116 with named buff effects, 44 with ability descriptions. Unknown fields receive an explicit raw value/buff ID display; No invented percentages. The display uses basic data, does not calculate final values with all character/mod influences. Reading errors with repetition; No gameplay accesses. 17 Frontend tests, 10 extra sockets UI tests and local catalog exam passed. Installed via the existing desktop link; only Workbench-EXE replaced. Writing operations on scores/game files did not take place for this. The actual effect of the additional sockets in the game remains open separately.

**Running correction check, 05.10.2026:** Game started and save loaded. Purely read in the running process confirmed: Client ** and ** Servers have in the case of tank 10, helmet 11 and mecha sword 15 physical/open places, each set 1. Base socket on both sides identical, helmet/sword continues refinement 10. The corrupted client prefix 255 is fixed in this run. No game or save file changed for it. Visual confirmation, loading/effect and saving/reloading remain open. socket changes are intentionally blocked during play; Workbench does not have to be closed for this.

**extra sockets and correction, 04.10.2026:** Workbench → **extra sockets** can display further own weapons, armor and accessories from a selected save and extend suitable instances once by **ten empty bases**. Search, icons, instance ID and existing base number; specific grounds for blocking. In the latest save 329 instances read, 326 newly expandable. Up to 256 configured instances; No binding of new entries to Tom’s IDs.

The first installation started and loaded, but caused corrupted client ads (quantity 4294903295, open sockets 255). Cause: the separate Runtime→InitData converter has only five inline sockets and was not intercepted at first. The correction normalizes **both Save and Network InitData** to the original format; The client then restores the additional seats. Compatibility with the startup check of LiveItems is considered. Corrected mod and desktop EXE with closed game/Workbench installed. All 13 saves and 22 checked inventory files unchanged; Received old additional file. Tanks still **10**, Shadow Helmet **11**, Mecha sword **15**.

86 isolated native cases (including the full original network converter with base sockets 0–5 and corrupted prefix), 22 policy/file cases, 163 core unit tests, eleven frontend unit and seven browser tests passed. **The correction, effect and save/reload cycle still require the in-game test.** The game interface still shows a maximum of five bases; The others are set up in Workbench. Disclosure requires own installation, mods and item allocation; A portable installer/second computer is still open. [Conditions for transfer](docs/WEITERGEBEN.md). General phase development remains paused. [Implementation and in-game tests](.local/extra-sockets-plus10-20261004/RESULTAT.md).

**Separate mecha socket order, 04.10.2026:** With the equipped electric mecha long sword +10 in the latest `slot0` the fifth, previously locked socket opened. Exactly one data byte changed; Four existing stones and all other fields are preserved. After fresh check with closed game and Workbench installed with original backup and independently read back; twelve other save/lobby files unchanged. **Evidence at the Witch to check effect and save/reload still in play.** Previous research status before the above extra sockets installation: 32 passed isolated native cases for processing, copying and an adapted memory loop as well as a complete private ten-entry format sample. A concrete native approach for ten bases has been found; At that time, the charging path, storage cycle and surface remained open. The later implementation is mentioned above. General phase development remains paused. [Result and in-game tests](.local/mecha-sockets-20261004/RESULTAT.md). [Proof of concept for ten bases](.local/mecha-extra-sockets-20261004/RESULTAT.md).

**Separate fishing/fall protection order, 03.10.2026:** A **Crab fishing rod** with the existing effect **automatic retrieval** in the latest `slot2` supplemented; 999 Received places and progress. New **CrimsonNoFall.asi** installed when the game is closed with launch protection: targeted case HP path protected for Kliff, Damiane, Oongka and 339 mount definitions. Complete native routine, 684 protective cases, forwarding for remaining figures and common loading with older mods checked. 31 existing files unchanged; 17 Fresh backups before ASI installation. **Real in-game test open. Ten baits open:** no corresponding item found in the table; The actual item name is requested. General phase development remains paused. [Result, fuses and tests](.local/fishing-no-fall-20261003/RESULTAT.md).

**Separate mount task, 03.10.2026, 01:50 watch:** Lion and Rokade receive movement multiplier **1,30**, endurance cost reduction **100%** and strong endurance regeneration. At Lion maximum/initial HP from **450 to 1,350**; Rokades bonuses in all five stages. When the game is closed with launch protection installed as an archive group `0049`. Only two of 7,250 character records, 20 number fields changed; Passed twelve native reading exams and twelve rejection cases. All twelve save/lobby files and older mods unchanged. **Actual tempo/endurance effect and HP in the game still to be checked.** [Result and other ideas](.local/lion-rokade-upgrades-20261003/RESULTAT.md).

**Separate quest, 28.09.2026:** "Request from Ben", step "Battle an animal from the cattle farm", clearly identified in the latest `slot1`. Twelve save/lobby files backed up and checked unchanged. **No artificial termination**, according to user condition "only when safe": complete termination process and consequences not verified. Regular alternative documented through Ben's cattle farm administration. [Test and backup](.local/greymane-slaughter-20260928/RESULTAT.md).

**Separate socket/sleep job, 28.09.2026, 00:29 watch:** Additional ASI mod for **30 playing hours installed over the previous 12-hour selection**; Labeling remains 12 hours. 288 native event/calendar cases and shared loading with the one-second mod passed. 17 existing files unchanged, nine save directory files backed up. **in-game test still open.** **Five additional sockets per created item and their filling not implemented:** active EXE loads/stores five entries and creates five UI lines. 18 tested created items, documented matching III stones as candidates; No save and no existing socket changed. After re-examination according to user decision “if possible, otherwise leave” the unsecured additional base extension together with filling is left for the time being. The general phase development remains paused. [Result and open work](.local/sockets-plus5-sleep30-20260928/RESULTAT.md).

**Separate sleep order, 27.09.2026, 20:29 Clock:** Own ASI mod for **1 second real sleep/waiting lock** installed when the game is closed. Time limits, existing locks, both native functions tested with test data and automatic loading in isolation. Two new files; EXE, existing mods and saved games unchanged. Eight save/lobby files plus Steam metafile backed up. **Real game start and sleep/wait test still open.** The mod checks the exact EXE version and deactivates its hooks in case of deviation. No Workbench UI integration; general phase development continues to pause. [Result, backup and test instructions](.local/sleep-cooldown-1s-20260927/RESULTAT.md).

**Separate inventory/quest, 27.09.2026, 15:09 watch:** +250 small bones in `slot1`, before 3, now 253; 5 stacks to 50. Complete comparison, inverse byte testing, launch protection and reading back passed. 999 Received places and progress. Desired activation of the grey mower series **not implemented**: starting conditions of the current version not fully decodeable; Later quest groups are partially completely missing in the save. “Camp expansion” is already open, all five donation quests have been completed. [Result and backup](.local/greymane-unlock-bones-20260927/RESULTAT.md).

**Separate inventory order, 27.09.2026, 14:39 Watch:** In the latest `slot2` **1,616 are excellent wood**, **821 Ivory**, **Mal of darkness +3** and **Demeniss seal +0** additionally in the backpack. Wood/ivory covers the final targets of the still open wood/weapons donations and contain 300 reserves. Money, food and stone are already completed according to mission states. Complete comparison, inverse byte testing, launch protection and reading back passed; Donation/ring acceptance in the game open. 999 Received places and progress. [Result, donation quantities and backup](.local/camp-donations-reserve-20260927/RESULTAT.md).

**Separate inventory order, 27.09.2026, 14:27 Watch:** In the latest `slot1` are each **400 timber, high-quality wood and excellent wood** as well as **100 Large meal** registered. According to the active table 64,800, the meals correspond to food points for “A table full of abundance” and must be donated to Carl. 29 new stacks; 999 Received places and progress. Complete comparison, inverse examination, launch protection and reading back passed; Donations/Questions open in the game. Jewelry selection created, jewelry not supplemented. [Result, selection and backup](.local/camp-provisions-wood-20260927/RESULTAT.md).

**Separate inventory/speed order, 27.09.2026, 02:51 clock:** In the latest `slot2` all **41 Abyss-Sockelitems III are entered once in addition**. In addition, **Hurry III** is in cliffs laid out frost curse plate shoes; **Surrender I** was returned to the backpack. Equipment contribution now computationally 15 movement levels; The regular maximum was used as a permitted evasive target. **30 is not set up.** Full comparison, inverse byte checking, launch protection and reading back passed; Ingame display and effect remain to be checked. Get progress and 999 places. [Results](.local/abyss-iii-speed-20260927/RESULTAT.md).

**Separate inventory order, 26.09.2026, 14:06 Watch:** In the freshly secured latest `slot0` are entered each **500 additional small bones, platinum, abundant grilled meat, improved Kraftfist herbal pastilles and diamonds** as well as each **10 Destruction III, Reinforcement III and Stormwind III**. 2,530 pieces in 108 new stacks; 582 Backpack entries at 999 seats. Complete comparison, inverse byte testing, launch protection and reading back passed. maintaining existing progress and installation; in-game validation open. [Quantities, result and backup](.local/supplies-500-20260926/RESULTAT.md).

**Separate inventory order, 26.09.2026, 13:52 Watch:** In the freshly secured latest `slot0` are each **300 additional iron ore, copper ore, azurite, silver ore, garnet, gold ore and epidote** entered (2,100 piece, 42 new stack of 50s). Existing items, skills, money, 842 gold bars and 999 places are preserved in the complete save comparison. Passed inverse byte testing, launch protection and live readback; in-game validation open. Phase development continues to be paused. [Quantities, results and safeguards](.local/ores-300-20260926/RESULTAT.md).

**Separate inventory order, 25.09.2026, 21:40 watch:** In the latest `slot1` (save from 21:28 watch), **200 additional pieces of 27 types of materials** are entered for the refinement of weapons, shields and armor, in total 5.400. Regular stacking limits: 308 new entries, including 200 individual Aeserion scales. Existing items, skills, money, 843 gold bars and 999 places are preserved in the complete save comparison. backup, inverse byte testing, launch protection and reading back passed. Use when forging as well as saving/reloading remain to be checked in the game. The general phase development remains paused. [List of materials, results and safeguards](.local/refinement-materials-200-20260925/RESULTAT.md).

Status: 23.09.2026. Desktop app **v0.5.9**. The free repair is developed separately (development module **0.23.0**) and is not yet usable in the app. This overview describes the current overall status; Older entries in `docs/PROGRESS.md` are development progression.

**New build detected:**Installed is now Steam **25477059 / EXE 1.0.0.2976**. For the current inventory order, the four affected tables and 38 EXE/metadata files were retested. The general app doesn’t fully support this latest build yet. The table support for the previous build 25455892 / EXE 1.0.0.2949 is added in v0.5.9; Build 25381195 also remains supported. 61 files used were compared, 14 indexed table pairs byte-identical were reconstructed. The new stage/squeeze grades are retained. The native repair connection for the new EXE is still open; A B0 vanilla base has not been established. [Proof of build](docs/BUILD_SUPPORT.md), [What needs to be tested](TESTCHECKLISTE.md).

**Your specification:** in-game tests only after the development of all phases. These decreases remain open and are not counted as passed. While you are playing, neither gaming process nor installation is changed. Repair development does not change saves. On your separate order, slot 2 was supplemented with 200 light/200 heavy copper bags and 30,000 silver when the game is closed; [Result and backup](.local/inventory-currency/RESULTAT.md).

**Pause on your request:** The phase-5 work is paused until the continuation. The subsequent inventory order adds gold bars in the now latest slot 0 1,000; Bags and money are preserved. **999 Backpacks are set up for your new direct order:** The inventory table allows 999 seats, the latest slot 0 contains 949 additional seats to the 50 starting seats. Installation and Save have been secured and independently read back; The display, use of higher seats and regular storage/reloading in the game remain unchecked. The targeted change does not establish a general B0 vanilla base. [Closure and collateralisation](.local/inventory-999-save/RESULTAT.md).

**Current inventory order, 20:51 clock:** After the Steam update, the game starts again, but the old mod registration was replaced. In the now latest slot 1, the remaining **843 gold bars were merged into a stack**. New archives set the stacking limit to **1,000** and the inventory maximum to **999**; The save again contains the matching 949 expansion slots. Money, items and new game progress are preserved. Installation and Save are checked offline. **The user confirms the start of the game, a stack with 843 gold bars and 999 inventory spaces.** Regular saving/reloading after this correction remains open as a follow-up test. [Current report and safeguards](.local/inventory-stack-2976/RESULTAT.md).

**Additional inventory order, 23:42 Clock:** In the freshly secured latest state `slot100` from 23:35 Clock, **500 keys are added in a stack**. The existing 3 Hernand coins for refinement have been increased by 100 to **103 in a stack**. Stacking boundaries: key 500, Hernand coins 200; Gold Stack Limit 1,000 and 999 Complete save comparison, inverse examination, archive change sample and live readback passed. The new game test remains open. [Report and safeguards](.local/keys-hernand-2976/RESULTAT.md). After further play/save, the fresh original save already had 843 gold bars in a stack and 949 expansion slots.

**Inventory order from 25.09.2026, 15:24 clock:** In the latest slot 0 (stored at 01:23 clock) **70 finished Abyss artifacts** were added; with the existing 5 it is **75**. The addition is in seven regular 10-stacks. saved games backed up, complete semantic comparison, inverse byte testing and read-back after commit passed. Game installation, existing items and 999 seats received. The use in the skills menu remains to be checked. [Report and safeguards](.local/abyss-artifacts-70-20260925/RESULTAT.md).

**Correction, 25.09.2026, 15:31 Clock:** The 70 Abyss items were completely taken back on user request **** because they do not correspond to the intended ability resource. Both affected files and change times are returned to the status immediately before the addition; the unchanged post-hashes were checked before withdrawal. Original 5 specimens, progress and previous mods are preserved. The correct target resource has yet to be clarified; No replacement was given. [Withdrawal and backup](.local/abyss-artifacts-rollback-20260925/RESULTAT.md).

**Adjacent assignment, 25.09.2026, 15:36 Clock:**What is desired are **Abyss shortcuts for all three characters**. Each 70 additional units are entered into the three abilities accounts (0/16/42 → 70/86/112). The corresponding total credit has been adjusted per account; The stored capabilities and inventory remained unchanged in the file comparison. New backup, complete comparison, inverse byte testing and reading back passed. **After that, the user reported a reset ability tree in the game.** The in-game validation failed; The cause has not yet been clarified. [Report on previous addition](.local/abyss-links-all-70-20260925/RESULTAT.md).

**Calibration on user request, 25.09.2026, 15:52 Clock:** Each **150 additional Abyss shortcuts** entered for all three characters. Accounts in Save: 70/86/112 → **220/236/262**. Total refund value per account 262; its use was additionally tested against the current EXE. The change affects six existing 16 bit fields without data shift; all other unpacked save bytes remain identical. New backup, complete comparison, independent codec and live readback passed. The cause of the previous reset is thus **not proven to be solved**. Loading, ability state, use and saving/reloading remain to test. [Current report and safeguards](.local/abyss-links-all-150-20260925/RESULTAT.md).

|Phase|Thema|Standing|
|---|---|---|
|0|Research and feasibility|Completed; Unknown fields documented|
|1|File formats, Core and CLI|Implemented and automated checked|
|2|Desktop app and item database|implemented; Parts of the source links are missing|
|3|Production computer|Implemented calculation; Sources of procurement incomplete|
|4|Apply Engine, Shops, Drops, Trust|Implemented to the documented extent; Boundaries and matches open|
|5|World, Dragon, Inventory, Durability, Skills and Items|In work; Substantial parts present, residual points below|
|6|Profiles and Farm Preset|Not yet started|
|7|Reading save reader and progress tracker|Not yet started|
|8|Interactive card|Not yet started; First concretize feasibility|
|D, optional|Farm Hotkey in the ongoing game|Not started; Separate subsequent release required|

## Phase 0 – Research and feasibility

**Implemented / delivered:** Build captured, file formats and feature assignment examined, sources and licenses documented. Reports: [FORMATS](docs/FORMATS.md), [FEASIBILITY](docs/FEASIBILITY.md), [CREDITS](CREDITS.md).

**Still open:** The functions recognized there as unknown are further investigated in the respective later phases. Research is not proof of effectiveness in the game.

## Phase 1 – Core and CLI

**Implemented:** Installation recognition for Steam/Epic/Game Pass, archive reading, decryption and decompression, build/hash checking, versioned parsers, byteidentical round trips, localization in 15 languages, SQLite full text index and CLI for inspection, search, round trip and build comparison.

**Still open / Limits:** New game builds require their own examination. Unknown fields remain raw. A local Linux test proof is missing; Windows has been tested. The detection of a save path is not yet a save reader.

## Phase 2 – Desktop App and Item Database (A1)

**Implemented:** Tauri-/React app with dark layout, 6,816 Items, full text search, filters, sorting, virtual table, real extractable icons, language change, detail view, raw fields, item references and protected JSON export. The desktop shortcut opens the built app.

** Still open:** Full link Item → Dealer → City / Region / Price / Stock and Dropset → Concrete opponent / Location. Recipe links were added in phase 3. Unknown categories/stats are not labeled with invented meanings.

## Phase 3 – Crafting planner (A2)

**Implemented:**1,108 calculable recipes, target quantity, recursive material tree, aggregated residual demand, existing stocks, batch surpluses, recipe and material alternatives, manual intermediates, cycle warnings, item recipe links as well as plan export and CLI.

** Still open:** Dealers, regions, prices and concrete drop sources for materials. Special/strengthening recipes are not released flat-rate as predictable. Taking supplies from a save depends on phase 7.

## Phase 4 – Apply Engine, Shops, Drops and Trust (B0–B3)

**Implemented:** Own overlay builder without external mod manager, preview, apply/reapply/restore, backups and hash checking, journal and recovery, build change/conflict checking, game start protection, vanilla origin checking, inventory and confirmed receipt of third-party additional files. Reapplication rebuilds from the initial stock and the current settings.

- **Shops:** Stocks, selected additional articles/article sets, individual
Assortments and daily refresh for supported dealers.
- **Drops:** Quantity and Opportunity Factors, Individual Exceptions and Manually Selected
100% base rates for supported dropsets.
- **Trust:** Multiplication of proven positive gains; Negative penalties are maintained.

**Still open / Limits:** No universal coverage of all special dealers and opportunity variants; No automatic boss classification. The complete catalogue at all dealers at the same time is not released. Foreign changes to original sources are not confirmed compatible mods. The current installation is not yet approved as a B0 vanilla base; the stored test report alone is not an approval. Real game effect and manual removal are still pending.

Development has been completed for this documented scope; This does not mean that every original desire is universally covered. [Details](docs/MODS.md).

## Phase 5 – Advanced mod modules (B4–B11)

|Module|Implemented|Still open|
|---|---|---|
|B4 World|Spawn numbers/groups, selection and exceptions; two stage patrols and 108 replacement rules; Accepted city, riding duration and cooldown values|General NPC respawn timer; Game effect and quest dependencies|
|B5 Dragon|Blackstar cooldown, duration/preset and proven regional/city flight rules|in-game validation of region changes; Special zones and quest descents remain their own rules|
|B6 Inventory/Lager|Free start/maximum values for nine ranges, ratio check|Universal Engine/Save boundaries not proven; Special containers remain protected|
|B7 Stapel|Global size, categories, exceptions, experimental instance items|Universal safe maxima and in-game tests for instance items|
|B8 Durability|occupied No-Wear-Sentinel for 122 Items and wear factors|in-game validation, in particular special equipment/socket; No restoration of destroyed items|
|B8 Kostenlose Reparatur|Common process with batch checking, native copies, field recorders and message adapters. Equipment path and inventory-client Ack connected in isolation. Native item/storage object conversion gets repaired main/socket values and tested dye data|Server-side inventory messages, regular saving process and its confirmation, other additional data, real effects / UI / feedbacks, manager / player / thread binding, input, loader and B0 installation are missing. Still locked in the app|
|B9 Endurance/Spirit|Seven cost categories, including zero costs; Additional self-consumption values from Skills/Buffs|costs outside the occupied lists; No universally proven Unlimited|
|B10 Skills|All 2,069 skills, field/raw data view, global/individual cooldowns and occupied numerical values|Meaning/units of unknown fields and in-game validation|
|B11 God-Items|Stats, compatible buffs, additional enchant lines and reusable item templates|in-game validation; Templates do not change existing save instances|

**Phase 5 is not yet finished.** The repair development is only checked with private test data and functional copies in the own test process. It is not an already installed mod. Details: [Residual points](docs/PHASE5_REMAINING.md), [Repair development](runtime/repair/README.md), [Object references and test limits](runtime/repair/REFERENCE_INTEGRATION.md). [Common native path of acquisition on the new build](runtime/repair/REGISTRY_NATIVE_INTEGRATION.md). [Current detection and its test limits (0.9.0)](runtime/repair/PINNED_CAPTURE_INTEGRATION.md). [Inventory/equipment proof for the new build (0.10.0)](runtime/repair/INVENTORY_2949.md). [Event preparation and isolated server/client sample (0.11.0)](runtime/repair/EQUIPMENT_EVENTS_2949.md). [Native Item Copy and Lifetime (0.12.0)](runtime/repair/ITEM_LIFECYCLE_2949.md). [Slot marking and held capture (0.13.0)](runtime/repair/DIRTY_SLOTS_2949.md). [Field recorder for inventory and equipment (0.14.0)](runtime/repair/FIELD_WRITER_2949.md). [Prepared copies and common process (0.15.0)](runtime/repair/TRANSACTION_2949.md). [Slot processing and batch completion (0.16.0)](runtime/repair/PERSISTENCE_2949.md). [Inventory client feedback (0.17.0)](runtime/repair/INVENTORY_EVENTS_2949.md). [SQL job path and missing memory confirmation (0.18.0)](runtime/repair/SQL_DISPATCH_2949.md).

**Currently tested:** 454 native scenarios and nine CTest suites. 38 Cases run the [Item/storage object path](runtime/repair/ITEM_SAVE_2949.md), including 14 with the actual repair plan and native aftercopies. Damaged/full main values, no-wear, free/partially occupied bases, dye data and reuse of the objects are tested. The native loader normalizes some values; These special rules are documented. This does not prove the completion of the regular saving process. The SQL success return alone also confirms no storage; The repair switch remains blocked.

39 other cases check the [Save dispatchers and their queues](runtime/repair/SAVE_BACKEND_2949.md), including error cancellations, repeated calls and Steam approval checks with private platform data. Other 34 cases run the [File-writers](runtime/repair/SAVE_FILE_2949.md) with private file recipients. Flush/close errors can still result in success and emptied payload. Further 66 cases check the [native buffer preparation and compression](runtime/repair/SAVE_ENCODING_2949.md). Entries reused after an opening error may be compressed twice; New experiments need fresh raw data. Subsequent processing, regular write closure and game host binding remain open.

36 new cases check the [Original Inventory Package Serializer](runtime/repair/INVENTORY_PACKET_2949.md) and stream selection; ten connect them to the repair transaction and the client-ack. The package lacks item UID, socket and order data. Missing/double delivery or a wrong actor will be rejected in the private process. Real transport binding, successful pool branch and server/store closure are missing.

## Phase 6 – Profiles and Farm Preset (C)

**To be implemented:** Named full module profiles, JSON import/export, farm preset with drops ×10 and spawns ×3, customizable values and profile changes over B0. Existing item templates are not yet a complete profile system. Profile changes should remain blocked during the running game and act on the next start.

## Phase 7 – save reader and 100% tracker (A4)

**To be implemented:** Save selection and reading parsing, quest/stage progression, grouping by region/category, progress displays, change updates, and collectibles and inventory import, as far as the format gives them. No Save write function is provided.

## Phase 8 – Interactive map (A3)

**To be implemented:** Concrete feasibility report first, then available map textures/tiles, confirmed position sources, coordinate calibration, levels, search and marker details. Collected objects only if supported by the save reader. A complete card is not yet promised.

## Optional D – Farm Hotkey in the game

**Not started.** Separates later ASI/Runtime module for the farm hotkey, only after the other functions and own release. The already released repair development is not automatically a release of this farm hotkey.

## Next steps and evidence

1. Further edit technical remaining points from phase 5, currently native functions of the new build and the repair connection.
2. Then phase 6, then 7 and 8 according to phase plan.
3. Manual in-game tests after completion of the development of all phases, as agreed.

Current review and changes: [PROGRESS](docs/PROGRESS.md). Automated proofs and later game acceptances: [TESTING](docs/TESTING.md). Specific cases with expected result: [TESTCHECKLISTE](TESTCHECKLISTE.md).

## Additional order – Black Star (01.10.2026)

**Installed, in-game test Open:** Own ASI extension for virtually unlimited endurance, summon cooldown of one second and the targeted cancellation of regional summon rejection for black stars. Interior and obstacle tests remain active. The old saved timer in the latest slot 1 has also been shortened to one second, with exactly checked save comparison. Then summon time is increased to **1,000 minutes (16 hours 40 minutes)**, including the stored remaining timer in slot 1. Fresh backup of the previous extension and all save files created; Save comparison shows only the new duration. Passed native reader/hook tests and loader exams.

According to the game report about closed cities and red map areas also installed: targeted clearances for Schwarzstern city/area flight and altitude, as well as further navigation/voxel/platform checks during its summoning. Original interior and final placement checks remain active. Current save files unchanged; previous ASI and all saves freshly secured. Native condition/instance lookups, mount references, ten codehooks, condition slot and loader tested. Decrease of red areas in the game open.

Details, backup, dismantling and manual removal: [Black stars](docs/BLACKSTAR.md). This additional contract does not change the development status of the other phases.

**Additional order Item selection / Live spawner, 03.10.2026:** Selection implemented with icons, descriptions, table values per refinement level, main/subcategories, text/ID search and virtual list. Build 2976 checked for basic reading catalogue; Desktop shortcut updated. Reading diagnostic mod installed, 32 existing game/save files checked unchanged. **Live Item Entry is now implemented and tested outside of the game:** native Item Creation, Inventory Transaction, Local Connection, Queue, UUID Protection, Cancellation and Result Query. New ASI and desktop versions are installed when the game is closed; Desktop shortcut has been updated. 33 existing files checked unchanged, 19 files freshly backed up. A real inventory/store/reload test is still pending. The reading sample remains additionally present. Details: [Item selection and live spawner](docs/LIVE-ITEMS.md).

**Automatic Item Output, 03.10.2026:** The additional pickup is replaced by a character task hook. He processes the request with the native reference held by the game and checks the active character again. Status remains usable with confirmed figure even during Alt-Tab; Execution takes place in the course of the game. UUID protection, discontinuation and result query are retained. Actual native dispatcher/wrapper and MinHook in own test process, protocol 2, queue/context change/breaks, production build and loader passed. ASI and Workbench closed game/Workbench replaced and read back; 36 existing files unchanged, 23 backups. **An iron ore without picking up, storing and reloading can still be checked in the real game.**

**Further reported summon lock, 01.10.2026:** The user continues to report after the city flight update approximately "mounts can not be called here". The old update was loaded according to runtime log; the spawn tests recorded so far do not show the concrete rejection. Another update detects Black Star during the previous client summon action and logs common/server-side rejections as well as error code and call path of the game message. Passed three native test suites and loader exam; only replaces the ASI when the game is closed. All nine saves and 19 protected files checked unchanged, fresh backup created. **The residual cause and success at the reported location are still open; Further play is required.**

## Additional order Item selection – bag, arrows, mounts and knowledge (03.10.2026)

**Implemented, checked outside the game and installed on 04.10.2026 when the game is closed/Workbench:** Misclassification of an optional definition field corrected, in particular Extra Large Pocket 6003 and Lightning Arrow 1001315. Native standard paths tested; Base/quantity checks remain active, error messages distinguish the causes. The set field allows emptying, typing and insertion with stack estimation.

New mount selection with search, family, description, score selection and possession status. When the game is closed, exactly one matching animal can be registered in the stable, provided that a confirmed unassigned template of the same family is available in the save. Fresh numbers, backup, read-back, preservation of existing save areas and protection against double assignment implemented. Live registration, special summons and individual Tiericons remain open; Stall/Ringing/Save/Reloading in the game still to be checked.

Documents and items with confirmed knowledge reward show their status from the selected **stored**save; Missing assignment means unknown. Read in again after reading/storing. Comparison with the game open. This is a limited part of the save reader, not a finished quest/100% tracker of the 7 phase.

Details: [Live-Items](docs/LIVE-ITEMS.md), [mounts](docs/REITTIERE.md).

## Additional order for additional mount families (04.10.2026)

**Implemented, tested on private saves, built and installed; Game initialization explicitly unconfirmed.** New basic entries allow for additional families without a saved template of the same species: Raptor, elephant, iguanas, wolves, adult camels and other normal mounts. The private copy of slot 0 has 288 instead of 219 addable entries. 24 separate candidates, including all 13 adult camel variants, pass full back comparison, encryption, fresh numbers and double lock. The route does not adopt animal values or equipment, but creates empty structures with current field names/types and an initialization flag false. The desired initialization by the game must first be observed. Livestock, camel calves, special summons and unknown schemes remain outside this route. Existing healthy family templates keep the previous way. Eight mount UI tests, eleven front-end tests, five app tests and 159 core unit tests pass. The strict vanilla round trip of the active mod installation continues to refuse its modified `meta/0.papgt`; it is not reset for this order. Even the strict archive mod preview refuses the changed registry outside their journal. The real reading browser/catalog test and the save checks pass. The desktop update was installed when the game/Workbench was closed: 37 files including all 13 saves unchanged; 23 Backups and desktop link independently tested. No animal allocation and no real save changes through development. Evidence: `.local/mount-family-investigation-20261004/` in-game tests MT09–MT12 in [TESTCHECKLISTE](TESTCHECKLISTE.md).

## Additional order mount templates, image preview and search (04.10.2026)

**Follow extension installed and independently read back; in-game validation open:** Healthy, assigned template animals are supported. Only new copies lose name and the three confirmed assignment fields; Existing animals remain protected by complete back-testing. The newly read private copy of slot 1 thus shows 219 instead of 217 addable animals, including two lion variants. Missing families, task/feeding fields, missing HP and unknown assignment formats remain blocked. 89 Exact animal portraits and 210 labeled sample images cover 299 of 322 entries; 23 keep a general symbol. Common textures are decoded once, the example labeling remains on the individual variant. Private evidence: `.local/mount-catalog-expansion-20261004/`. The desktop update was installed when the game was closed and Workbench was closed; 37 Existing files including all 13 saves and mods are unchanged. 23 Backups and the existing desktop link have been independently tested. No animals were awarded during the installation. The previous version is documented in the following section.

**Implemented, technically tested and installed on 04.10.2026 when the game is closed.** An own animal designation no longer blocks an otherwise suitable unassigned stall template. In a private copy of the current slot 1, 219 animal species can be added; the advertisement shows number, families and the cause of missing submission. A bear, a horse and a lion were produced exclusively in private Save candidates and tested by complete back-testing.

Item images and mount images are larger and can be enlarged by click. 57 real animal portraits could be clearly assigned and decoded; Missing images remain recognizable as a general animal symbol. The normal item search now finds subwords in names, descriptions and internal names, regardless of upper/lower case and hyphens. Several words are combined. Pure numerical item IDs remain exact. Optional regex mode for item database, item selection and mounts with error message for invalid patterns.

158 Core tests, five app tests, eleven front-end tests and 16 targeted UI tests have been passed. The real catalogue test includes 6,816 items, partial word/regex search and the 57 animal portraits. The existing desktop link opens the new EXE. 37 existing files, including all 13 save files and the native mods, are unchanged; 23 fuses have been tested. The in-game validation remains open.

**Arrow error:** Despite the presence of lightning arrows, the normal shot with the Aeserion arc reported "not enough arrows". Purely reading, 200 pieces of the ID 1001315 were synchronously confirmed in the client/server inventory. The item note refers to lightning arrow 1001315, fire arrow 1001316 and cold arrow 1001314 as monster ammunition. The selection identifies these variants and also blocks their re-output in the backend. Arrow 50001, Poison Arrow 50003 and Explosion Arrow 1001321 remain available. The explosion arrow is explicitly intended for players. Existing NPC arrows have not been removed or converted; the use of a matching arrow variant with the Aeserion arc must still be checked in the game.

Both installed files are independently read back; 36 existing files including 13 current saves and desktop shortcut checked unchanged, 23 backups created and checked. During installation, no items or animals were assigned and no saves were processed. Playing games remain open.

## Additional order complete skill trees (10.10.2026)

**Entered in the latest save and independently read back; in-game validation open.** Kliff, Oongka and Damiane: all 142 entries of their three main trees, corresponding to 140 different abilities, at the intended maximum level. 69 newly unlocked, 16 stepped higher, 55 already received. The available points remain at 108 / 236 / 116; Additional learning costs were offset with an equal credit of the total meter. Inventory, equipment, quests, extra sockets and the remaining saves are unchanged. Complete comparison of all 1,179 memory blocks, independent HMAC/decompression testing and byte-accurate reversal sample passed. When the game was closed with launch protection, a save file was atomically replaced and read back. The twelve original save/lobby files are backed up.

implementation as a personal save intervention; A general Workbench function for this remains open. Start of the game, all three trees, skill use and save/reload are still to be checked in the game. Details and backup: [Capability unlocking](docs/FAEHIGKEITEN-FREISCHALTUNG.md).
