# Phase 0 – Feasibility

**Supplement phase 2:** Desktop interface, search/filters, raw fields, direct item links and real icons are available. A1 is not yet complete due to the still unexplained merchant, drop and recipe sources. [Current status](PROGRESS.md), [Operation and limits](DESKTOP.md). The following sections are historical research findings from phase 0 or the phase 1 supplement indicated below.

**Supplement phase 1:** The reading Rust core with CLI, item localization and SQLite-FTS5 is available; Tests and limits see [PROGRESS.md](PROGRESS.md). The following report documents the phase 0 investigation. Its open gameplay, editor, save and card questions remain; The GUI only follows in phase 2.

Status: 19.09.2026, updated Spec with its own apply engine B0. The study included the six reference projects and the local Steam installation. No app code was written and no game/save file was changed.

**Result:** The reading core and the item database have a proven technical basis. An own overlay engine is well-founded by reference code, but not yet tested in the game here. The complete wish list is not covered by known scalar patches alone: recipes, card calibration, engine maxima, complete cost/wear-free, and some Trust/Dragon properties require further verification or reverse engineering.

## Traffic lights and evidence

- **🟢 Green:** Data structure/field or technical path concretely occupied. This is not
Release release for an unbuilt function.
- **🟡 Yellow:** parts known; at least one required relationship, semantics, or
Current build check is missing.
- **🔴 Red:** for the mentioned part no reliable proof of implementation.

Source code findings and local examinations are separate. A field name or historical offset alone is not proof of its effect. The build is **observed**, not yet certified as a vanilla or fully supported Workbench build. Details and sources: [FORMATS](FORMATS.md), [Table searches](research/TABLES.md), [Apply-Recherche](research/APPLY.md), [Archivrecherche](research/ARCHIVES.md).

## Technical basis

|Particle|Ampel|Proof/Decision|
|---|---|---|
|PAZ/PAMT lesen|🟢|reference library built; real tables from group 0008 decrypted/decompressed. PAMT and PAPGT are serialized in memory byte-identical.|
|Iteminfo basic structure|🟢|6,816 local items, 6,465,724 bytes: parse → serialize byte-identical. This proves the structure preservation of this concrete file, not the semantics of every unknown field.|
|German localisation|🟢|Group 0027, `gamedata/stringtable/binary__/ger`, 39 Namespaces found. `item.paloc` actually packed LZ4; 13,575 Entries. Passed payload round trip after decompression.|
|Direct Rust-Git-Dependency crimson-rs|🟡|MIT and locally buildable; Currently only cdylib/staticlib and private Rust modules. An unchanged `git`-Dependency does not provide the required Rust-Core API. Recommendation: small documented fork with rlib/public API or limited MIT port, each pinned; No untested full integration.|
|Edit all tables lossless|🟡|Iteminfo demonstrated; Several other loaders are deliberately lossy. Each additional editor remains locked until its own current roundtrip.|
|Vanilla Proof / Build Release|🟡|EXE version, Steam build ID, metadata and 28 table file hashes. The origin of an unchanged baseline set and versioned schema must be determined before write authorization.|
|SQLite FTS5, Tauri/React, Profile|🟢|No format reverse engineering required. Not yet implemented; Windows/Linux build and real search/UI tests follow in the planned phases.|

## Functions

The file names in the table mean `.staticinfobody` plus the table-specific `.staticinfoheader` under `gamedata/binarystaticinfo__/bin`. Field names are source names, not an already released Workbench API.

|Function|Ampel|Concrete database and remaining work|
|---|---|---|
|**A1 Item database**|🟡|`iteminfo`: Key, `string_key`, category/type, icon reference, `max_stack_count`, stats/buffs. Item-Roundtrip and German PALOC passed locally. FTS5/Filter/Virtual Table are implementation work. Item→Localization, Icons, Store→Region and Drop→Source and Recipe Links are still fully checked. Unknown bytes as offset/type/value.|
|**A2 Crafting**|🟡|`crafttoolinfo`, `crafttoolgroupinfo`, `multichangeinfo`, `itemgroupinfo`: `_ingredientsItemGroupInfo`, `_recipeItemGroupInfoList`, `_fixedMaterialDataList`, `_resultDropInfoList`, `_craftToolInfo`. First three tables available/hated locally. 🔴 Full ingredient/output graph including quantities, alternatives and rules of success still unproven. Compare only known recipes against game advertisement; then calculate recursion, cycles and jointly consumable own stock.|
|**A3 card**|🟡|asset path `ui/texture/image/worldmap/`; `UIMapTextureInfo`, `BitmapPositionInfo`, `FactionNodeInfo`, `GamePlayTriggerInfo`, `GimmickGateInfo`, `KnowledgeInfo` contain position/texture/boundary candidates. 🔴 Full atlas, coordinate transform and complete collectible instances are missing. UIMapTexture can designate icons instead of background. No card UI before this proof.|
|**A4 100%-Tracker**|🟡|`questinfo`, `stageinfo`, Mission/Gimmick tables plus `QuestSaveData`/`FieldGimmickSaveData`. Local save path found; Save Content not analyzed. Zustandsmapping and robust denominator about optional/branched content is missing. Collectibles are not presented as derivable from Save. Shared read, stable re-read and implement filewatcher later.|
|**B0 Apply-Engine**|🟡|PAZ/PAMT and PAPGT registry are structurally known; Current registry checked locally. CDUMM creates own groups, calculates Jenkins checksums and registers in front. No CDUMM call/dependency required. 🔴 Local Engine Lead, Apply/Restore Cycles, Vanilla Origin, Crash Recovery and Game Start Race still untested. [Draft draft](APPLY_DESIGN.md).|
|**B1 Shops**|🟡|`storeinfo`, item reference, stock/price/refresh fields. Body/Header hashed locally. Current source describes +8 raw bytes per stock structure and an additional store byte. Historical parsers are called 105→113, which does not result in a general fixed record size for optional blocks/lists. 999 inventory and daily refresh only after own round trip, current field tests and examination of large dealer lists; All items can include special/questitems.|
|**B2 Drops**|🟡|`dropsetinfo`: Item references, group/individual chances and quantities. Current references distinguish set and item opportunity; Do not reduce semantics to a single percentage. 🔴 Automatic boss classification is missing; First, manual selection. Define global/override priority and opportunity/quantity limits build-related.|
|**B3 Trust**|🟡|GildyBoyes `DropSet_Friendly_*`-Heuristics marks four byte positions for offsets 59/63/67/71 relative to Name+NUL. No field type can be derived from this yet. This is a locatable historical clue, **not a confirmed current Trust field**. 🔴 Type, unit, event assignment and multiplier effect require controlled testing.|
|**B4 World**|🟡|`characterinfo`: `_terrainRegionSpawnPerCount`, `_callMercenaryCoolTime`, `_callMercenarySpawnDuration`; `regioninfo`: `_limitVehicleRun`, `_isTown`, `_vehicleMercenaryAllowType`; `fieldinfo`: `_canCallVehicle`; Faction/Collection/Gimmick data for more spawns/timers. Some fields are heuristics/dev-only. Do not equate respawn sources and spawn instances.|
|**B5 Dragon**|🟡|Mount fields from `characterinfo`, `vehicleinfo` and region/field flags. Historic Dragon ID is only candidate. 🔴 Blackstar assignment, reliable border crossing without dismount and meaning of duration/cooldown 0 not confirmed.|
|**B6 Inventory/Lager**|🟡|`inventory`: `default_slot_count`, `max_slot_count` (u16) in the reference parser; Table hashed locally. Reference seeks plausible value pairs heuristically. 🔴 Warehouse entry and hard engine maximum unknown. u16 maximum 65,535 is not a permissible product limit value without proof; Reference mods are still missing.|
|**B7 Stacks**|🟡|Current typed `iteminfo.max_stack_count` and `_applyMaxStackCap`; Old sources contradict each other with field widths. Preset 999999 is not a maximum value document. 🔴 Engine maximum and lossless stacks with instance state unknown. Equipment/Enchant/Sockets/Durability exclusively experimental after tests.|
|**B8 Durability**|🟡|`iteminfo._maxEndurance`, repair/equip data and `_decreaseEndurancePercent` as candidates. 🔴 Maximum durability 65535 does not mean "no wear"; Repair cost zero and real loss shutdown are not confirmed.|
|**B9 Stamina & Spirit**|🟡|`skill._useResourceStatList/_useDriverResourceStatList` and corresponding Stat hashes. 🔴 All promotion categories, units and universal free consumption are missing. Equipment reduction/regeneration does not replace a confirmed global multiplier.|
|**B10 Skills**|🟡|`skill`, `skilltreeinfo`, `cooltime`, resource/buff lists. table hashed locally; Additional round trip and effect tests are required. A reference button "Zero Cooldowns" sets 100; Zero semantics therefore do not derive from UI text.|
|**B11 God-item**|🟡|`iteminfo` stat/buff/passive/enchant lists and `drop_default_data`; Structure roundtrip-capable. Check namespaces and state effects separately. Length change requires new header index; RegisterConditionSkillBuffData on inappropriate items is a documented loading error candidate. Templates only for confirmed combinations.|
|**C Profile / Farm**|🟡|Profile JSON and deterministic settings do not require game data. Farm = Drops ×10, Spawns ×3; Effect depends on B0/B2/B4. Change in the current game plans at most changes and requests later closure; It does not run an Apply/Restore.|
|**D Runtime-ASI**|🔴|No hooks investigated, no implementation started. Own later phase exclusively after explicit Go.|

## Map: upstream feasibility report

There are asset path and position candidates as well as a reader for several PAZ compression variants. A usable complete world map background, its tile arrangement/projection, unambiguous instance positions of all collectible objects or a complete save allocation are not proven. Partially compressed textures require a variant check; Comments in older readers describe partly still unsupported cases.

Next steps in Phase 8: Invent and decode specific assets, collect several known world/pixel pairs, determine axes and scales, check transformation at independent points, then connect layer data with stable IDs. Until then, Leaflet `CRS.Simple` is a planned representation and not a validated coordinate solution.

## Priorities for phase 1 after review

1. Read-only detection, build observation and clear separation of known schemes
and unknown builds; no writing authority for the core read module.
2. Pinned Rust integration, archive/item/PALOC proof into real
Transfer Workbench tests with `.env` paths; preserve unknown fields.
3. Implement CLI and SQLite Index; other tables only with suitable
Take a round trip. GUI/editors and especially Apply remain later phases.

There are no questions that prevent the delivery of this research report. The missing reference mods and proofs of efficacy are documented dependencies of later functions. Phase 1 does not start automatically.
