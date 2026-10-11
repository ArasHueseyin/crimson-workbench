# Confirmed fields for phase 5

New: [Repair runtime prototype](REPAIR_RUNTIME.md) with isolated native proof of execution. The following static table results remain valid.

21.09.2026. Exclusively Steam 25381195, EXE 1.0.0.2944, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. The analysis reads the EXE as a file and small selected archive tables; No debugger, process access, runtime hook or save reader.

The following virtual addresses refer exclusively to this EXE. The field names are derived from their serialisation error messages; Breadths and order from the associated readers. Addresses are not runtime patch destinations.

|Table|Data sets|Verified layout|
|---|---:|---|
|characterinfo|7,250|U32 key, length-prefixed UTF 8 name, block byte; prefix to vehicle u16, callMercenaryCoolTime u64, callMercenarySpawnDuration u64; Readers around 0x1414f03f6|
|regioninfo|1,007|Reader 0x141516f00; limitVehicleRun, isTown and then forbiddenMercenaryKeyList: u32-Count + **u8**-Keys; Complete end checked|
|inventory|21|IDs 2/8/9/13/15–19 editable. Slots u16; Reader 0x1415087c0. From v0.5.7 twenty prefixes read structurally, character slot start 9272 still hash-bound; Complete suffix checked. Details below|
|skill|2,069|Cooldown u32 directly to Blockbyte, Reader 0x141518830; from v0.5.6 fully structural matrix/suffix reader, no equality of skillGroupKey and Record-Key provided|
|equiptypeinfo|117|Reader 0x1414fc990; decreaseEndurancePercent u64 after 12+8+2 prefix bytes behind the record header; Complete end checked|
|spawningpoolautospawninfo|140|**u32** header count, u32 key; Reader 0x14151a910; Complete record checked|
|terrainregionautospawninfo|134|**u32** header count, u32 key; Reader 0x14151a450; Prefix and all spawn groups read, unchanged bitmap/scheduling suffix opaque|
|fieldinfo|8|Reader 0x141505b30: current last 15 scalar bytes; alwaysCallVehicle_dev at the end−10. Only Key 1 / MainField released|
|statusinfo|84|u32 key/name from own small, hash-tested table; No invented Stat IDs|
|iteminfo|6,816|Full typed 2.03 reader/writer of the existing MIT port; before and after complete round trip|
|stageinfo|52,080|Only two explicit spawn patrols are editable; u32 header count, current sequencer prefix and `resetSecond` u32 for reader 0x14151b1ae|
|conditioninfo|10,798|Only rule 1011130; complete examination of header, boolean tree, three flags, original expression and parser type; Reader 0x1414ef3b0|
|factionreblockadinginfo|108|u16 header count and u16 key; Full reader 0x1414ffd10, only delayTime u32 editable|

## Full skill field coverage v0.5.6

Own verification of the current `SkillInfo` reader `0x141518830` and its resource helpers. The u32 immediately behind the buff matrix is read at `0x1415188ee` and written at `0x141518904` after mem +0x28: `skillGroupKey`, **not** again the data record key. This is followed by the `parentSkill` reference (`0x141518907`). The previous search delimitation via identical identifier bytes is completely removed. Existing buff layouts now directly determine the suffix position; Unknown types and invalid counts/zero flags will be rejected. No reversion to a later identifier.

The buff matrix was already internally structurally read; up to v0.5.5, however, their predetermined end position still depended on this suffix search. Only v0.5.6 removes the search dependency in the entire product path, also in the Catalog read.

Additional read-only fields: headers, group/parent references, learning requirements, curve tag upgrade graphs, character/conditional lists, knowledge/faction references, resource lists, battery, flags, slots, skill groups, developer texts, and video reference. field names come from the reader error messages; Do not invent any gameplay units.

`UseResourceStat` (`0x141518720`) serializes u8 `statType`, u32 `statusInfo`, u8 `isRegen`, i64 `varyStatAmount`, u32 `increaseStatusInfo` and u32 `decreaseStatusInfo`. The last two words were only skipped before; they are **no**additional 64 bit number. The list reader `0x141527950` calls this helper. `UseResourceItem` (`0x14152cfe0`) reads a u32 item reference (`0x14152d062`, resolution with `0x14038aac0`) and eight bytes `useItemCount` (`0x14152d09f`). Their signedness and permitted range of change are not proven by this reading operation alone; Therefore named hex value and no new editing release. The existing scalar changes and buff paths remain compatible.

**Data sample:** 87,697 Header/Suffix fields over 2,069 Skills. All record bytes are reconstructed from these fields, matrix counts and buff fields; Seamless coverage and exact equality of all records tested. Also 4,607 buff entries and nine summon/one AddSubLevel payload. Synthetic samples use divergent group references, identifier bytes in UTF 8 texts, non-empty reference/resource lists, truncated records, and oversized counts.

Private evidence: `.local/phase5-v7-skill-reader.txt`, `phase5-v7-resource-readers.txt`, `phase5-v7-resource-stat.txt`, `phase5-v7-pinned.log`. No EXE, Runtime, Game or Save changes. This closes the previous representation gap outside the buff matrix; Unknown buff meanings/units and other phase 5 gaps remain open.

### Additions v0.5.5: repair cost path and discarded zero approach

The ItemInfo accesses on `mem+0x408` (repair list) and `mem+0x410` (count) lead to a common cost path `0x14240dfb0` and the server path `0x142be3ee0`. Both use the ItemInfo-Getter `0x14038ab60`. An empty repair list provides `eErrNoCantRepairItem` (global error code at `0x146cf7b5c`, name registration `0x1422072a7`). A missing suitable material/style rule gives `eErrNoCantRepairResourceItem` (`0x146cf7b70`). The currently empty 6,816 lists are therefore not rules with hidden price 0. It follows from this no claim that all conceivable script costs are known.

The cost aid `0x142411f20` uses `RepairData.resource_item_count` at `mem+8`. A global percentage value at `0x146cfd9b8` is initialized with 100 (`0x1401f42fb`, configuration hash `0x64cbd1c9`). At this standard value, the help delivers the number of materials directly back, without a zero-value clamp. The caller divides the available material by exactly this return value: `idiv rbx` at `0x14240e0e5` and `idiv r14` at `0x142be406d`. The special value −1 bypasses a material limitation; for ordinary quantities of material, there is no zero testing prior to division. **A zeroing of the number of materials can therefore trigger a division by zero.** No runtime crash was provoked; this is a static proof on the pinned EXE.

The v0.5.0 introduced general item writer already contained a zero-setting path for this. In the unchanged supported build, it was not reachable due to the empty lists and the core check. v0.5.5 completely removes the write path. Core and format API refuse `free_repair: true` regardless of the number of existing rules; Core checks global and individual flags before any advanced changes. `false` remains compatible with old requests. The interface no longer uses the pure rule number as a share and allows you to remove an old active option from item templates.

Another spawn candidate is now assigned: `_spawnIntervalTime` (`0x1423d2ba0` → `0x14f6e42c0`) registers with `GameData_TimelineEvent_SummonCharacter` via `0x140c34f80`. RTTI metadata VTable `0x145622f88`, object VTable `0x14562f498`. The editor text names seconds, but it is a summoning event within a timeline. This does not occupy a general NPC respawn controller. No timeline/EXE/Save change made.

Proofs: `.local/phase5-v6-repair-{cost,resource,globals}.txt`, `phase5-v6-repair-global-xrefs.json`, `phase5-v6-interval-{config,type,rtti}.txt`. Free repair remains open: no proof is yet available for a separate, suitable data path. Runtime phase D was not started.

### Additions v0.5.4: named summon fields

Type 10: `SummonBuffData` calls `0x141f2d790` for `mem+0x90` the reader `0x1414ed330` (`SummonCharacterData`); then follow u8/u32/u32 at `mem+0x180/184/188`. The embedded reader confirms the field names by its own error texts. The Select list has one u32 count and two u16 references (`0x1415439b0`). `0x14151f0a0` reads four fixed u32 hashes each, no dynamic count. Both text aids `0x14141f540`/`0x14141f710` have a u32 bytecount. Terrain data follows `0x1414ecd40`: u32, text, u8, u16, eight bytes. `0x142157fe0` reads a four-byte reference. Optional embedded conditions are missing in all nine observed payloads and are explicitly rejected if they occur.

This completely splits all nine payloads with 181 bytes, including the three one after the other in Skill 41358. Select lists and texts are length-based; no blanket assumption of a fixed total sumon length. There are 58 individual fields in the observed summon payload. References, counts and all new values remain write-protected. Integer views show bit values without guaranteed unit; Position, yaw and elevation range remain hex values. From `deadLimitTime`, `spawnPercent` or `spawnableCheckInterval` no general NPC respawn controller is derived.

Type 74: The unveiled recorder `0x141f315b0` first writes the resolved four-byte key from `mem+0x90` (Getter `0x14042fe50`), then four bytes from `mem+0x94`. The latter remains `payload.mem_148` in hex representation; no acceptance of a level integer format. The single observed payload is in skill 65145, line 0/column 1. The earlier eight-byte raw block is not reinterpreted as an editable i64 value.

The table sample reconstructs all matrix bytes including these ten payloads from the displayed fields. Synthetic tests check UTF-8, non-empty select lists, shifted follower fields, any truncated summon prefix and rejected structural changes. The search in the surface takes additional field paths into account; Existing editable paths remain stable. Proofs: `.local/phase5-v5-{types,summon-helpers,sublevel}.txt`, `phase5-v5-structured-buffs.json`, `phase5-v5-pinned-final.log`.

**Further search limits: ** `_respawnTimeSecond` according to RTTI belongs to `NPCScheduleMissenscenePatternSaveData` (metadata VTable `0x1458088f0`, initialization `0x14195ad90`), not to an occupied editable spawn table. The selectively read `gamedata/npcschedulespawnoption.xml` contains skill/spawn assignments, not a respawn timer (11,166 bytes, SHA-256`f2dabc48db34533385d44619e062228820232c61a214ae342f3ed244c4957f9a`). Save contents were not read. Empty repair lists and found repair components/GUI texts still do not provide an additional confirmed source of costs. These gaps remain open.

### Additions v0.5.3: Re-occupation and inventory cover

`FactionReblockadingInfo` is read in full at 0x1414ffd10: u16 key, UTF 8 name, block byte, quest list, fraction node list, `delayTime:u32`, `protectCombatPower:f32`. The quest list (0x14153b680) contains per entry quest reference u32, condition reference u32, rate u64 and `closeTime:u32`. Also, the condition reference occupying only two bytes in the memory is read at the file level with four bytes (0x14151efe0). The node references are u32. All 108 records consume exactly their end, contain quests and nodes; 24 Waiting times are 86400, 84 are 432000. Only the four bytes of `delayTime` are changed. No editing of the quest resets, close timers, conditions, probabilities or nodes. The time unit and the interaction with area/quest states are thus not yet in the game.

Additional hash-tested files: Body 10,295 Bytes, SHA-256 `8d17f717a2ac59b468419128cfe92746eb1c5f8aa39a26933a9166bc30b807f2`; Header 650 Bytes, SHA-256 `4ddc468506db2b615a4760328a1fe34fbee425d86d27589ad10ca1ece200aae9`. The hashes belong to the Advanced Manifesto; Missing/modified sources are rejected before a plan. Local receipts: `.local/phase5-v4-reoccupation.json`, `phase5-v4-reblock-reader.txt`, `phase5-v4-helpers.txt`, `phase5-v4-ref-widths.txt`.

The inventory path 0x142407770 compares a slot index with the global i32 at 0x146cfc078 at 0x142407b1f. For index >= value, the path is aborted. The static initialization 0x1401ef7bb sets this value to **0x5b4 = 1460** and registers the configuration with hash 0xf1197dab. Another access at 0x140ee5273 uses it to pre-allocate an inventory list. Therefore, v0.5.3 lowers the Workbench input limit for both slot fields of the inventories 2/8/9
1460. The format width remains u16; a border violation is refused, not
shortened. **This is a verified default cap of a code path and a conservative Workbench boundary, no proof of the current runtime value, all warehouse paths or save compatibility.** Configuration overwrites were not excluded. Proofs: `.local/phase5-v4-globals.txt`, `phase5-v4-global-xrefs.json`, `phase5-v4-stack-cap.txt`.

In the same path, `ItemInfo._applyMaxStackCap` (mem +0x111) switches the check against the item-related `maxStackCount` (mem +0x18). A separate default value 200 is used only in a special case for originally individual items. It is **not a global stack maximum**. Therefore, the previous explicit Workbench stack limit 1,000,000 remains; A universal engine maximum is still open. No configuration or EXE has been changed.

`LocalizableString` starts with a category, **not** with a boolean. UTF-8 lengths do not contain an assumed zero terminator: the next byte in the record header is the real block byte. Old reference readers sometimes confuse blockbyte and cooldown here. Old FieldInfo/RegionInfo layouts are also not transferable to this build.

## Spawn-Gruppen

AutoSpawnTargetData (0x14151a120) contains group lists and filters as well as spawnLimitCount u16. AutoSpawnGroupData (0x141519c40) contains a character list; AutoSpawnCharacterData (0x141519ab0) serialized:

`character u32, group u16, reason u32, subcharacter u32, subgroup u16, subreason u32, characterCount u8, subCharacterCount u8`.

Color values of the group are four f32 (16 bytes). References, conditions, percentage and distance values are not reinterpreted for a quantity factor. The product only changes counts/limits. An unrecognized record locks the catalogue.

The EXE contains `_respawnTimeSeconds` in ItemInfo/GimmickInfo and reflected `_respawnTimeSecond`/`_spawnIntervalTime` names. This results in **no occupied general NPC respawn field** in these spawn tables. Therefore, no invented timer position and no reinterpretation of spawn distances to seconds.

**Counter-sample v0.5.1:** The pinned NattKh surface is called `spline.raw_qword` as a respawn timer. In the current scheme, this u64 is at the end of the group according to color values and three flags. The current EXE reader expressly refers to it at `0x14151a0a4` as **`_spawnPercent`**. All 1,246 groups were only checked reading; Values 0..1,000,000 match a scaled probability and do not prove milliseconds. This candidate is thus discarded as a general respawn timer and is retained in the event of changes in quantity. Proofs: `.local/phase5-v2-spawn-group-reader.txt`, `.local/phase5-v2-spawn-timer-candidate.log`.

## mounts and regions

Riding_Dragon_1 is Key 1000799, vehicle 16984, Cooldown 3600, duration 600. Boss_Dragon_60003 is a separate dataset and is not edited. The current Mercenary table identifies Vehicle_Dragon as Key 79. Region_Abyss (Key 8) contains the prohibited list `[65,78,79,80,82,81]`; other regions do not contain any entries. The option only removes 79. All other entries shall be retained; Count and table offsets are updated.

The global city feature edits limitVehicleRun only at isTown == 1 and alwaysCallVehicle_dev only in the MainField. It does not change lobby, rematch, UI or other instance fields. The development does not claim a proven shutdown of all possible Dismount/Summon test paths.

## Skills and items

The complete skill post-buff suffix is read structurally. The buff matrix remains in case of changes byte-identical; since v0.5.1 its structure is additionally read-only inspectable (details below). Stamina = status 1000026, Mp = 1000027; only Stat type 3 and negative costs are scaled. Examples: Climb −10000, ClimbStruggle −40000, Swimming +20000 (regeneration), Swimming_Run −10000. This difference is also confirmed in the mutation test.

Since v0.5.1, a selected original enchant line can be copied to a new target level. The original order must be strictly ascending, all separators must be zero. Whole lines are copied via the typed reader/writer and then sorted; 64 new/256 entire rows are Workbenchlimits. Stats/Buffs will be changed afterwards. An achievable upgrade path is thus not proven. integer widths follow the format; Insert changes the actual array length and leads to the rebuild of the table header. The list of known buff/level pairs comes from other original items of the same type of equipment. Complete semantics/balance/compatibility is not proven.

All 6,816 current item repair lists are empty. `max_endurance == 65535` is a sentinel on normal items and is not used as proof of active durability. `item_charge_type == 2` and a single consumption are also present on ordinary ammunition. The conservative stack filter therefore additionally takes into account actual equipment/enchant/socket/charge, subitem and sealing features; He is not an assurance of lossless Stackens.

## Skill buff matrix v0.5.1

Own static reading analysis of the EXE file (no process access): Factory `0x141f24930`, type switch `0x141f289b8`, common reader `0x141f28cc0..0x141f29284`, virtual payload reader to VTable + `0x50`. RTTI provides the type names. The format description `crates/cd-core/schemas/steam-25381195.skill-buffs.json` contains only type names and structure widths, no game values or code sections.

All 2,069 skills are read deterministically up to the separately validated suffix: 4,607 entries including zero. No search run with a first suitable payload limit and no longer-rate runtime cache. Variables are u32 strings, string/reference arrays and the type-dependent immune array. Type 35 contains a u32 array, 44 a string plus 30 bytes, 45 a u32 value, stringarray and flag, 87 a reference array and two flags. Type 61 reads one string and u32 (forwarding `0x14e047180`), type 92 reads two u16 (`0x14e04fce0`).

**Historical boundary v0.5.1–v0.5.3:** Payload 10 (Summon, 181 Bytes) and 74 (AddSubLevel, 8 Bytes) were only demarcated as build-bound raw blocks. The division from v0.5.4 is documented below; No complete semantic interpretation or editing release of these fields. Since v0.5.2, graphs are visible as three i64 values and u32 Curventag. The three values are editable, the curve tag remains write-protected. Numbers are raw interpretations of their bit width, not a claimed unit. `common.mem_*` follows structure positions, not guessed gameplay names. The complete raw view also contains all references and unknown values outside the matrix. Deviant, cut-off or ambiguous structures are rejected, not skipped tacitly.

Evidence: `.local/phase5-v2-buff-*.txt/json`, `.local/phase5-v2-deterministic.log`, `.local/phase5-v2-pinned.log`. The original ambiguous research sample was not adopted as a product parser. The test checks complete gapless matrix coverage per record.

## Origin and reproducibility

### Additions v0.5.2

**Skill buffs:** `common.mem_24/32/40`, fixed i64 payload values and the three i64 components of the graph reader `0x1424047b0` can be overwritten. References, counts, flags, curve tags and opaque payloads will be rejected. Per skill maximum 1,000, total maximum 10,000 changes; Numbers are preserved decimally as strings, without JavaScript rounding. Each change writes exactly eight bytes. The reader validates the same matrix boundary before and after processing. This is an editing release of the occupied number structure, not a claim of its gameplay significance.

**Stage reset:** Own current prefix reader of the SequencerDesc (`0x142348900`) and StageInfo (`0x14151abc0`). Research sample: 51,150 delimited prefixes, 930 explicitly excluded with complex bindings; No failed reads within the supported variant. From 618 direct spawn references have exactly two positive finite `resetSecond` values: 1017811 / `Faction_Byron_HiddenEstate_Block_FactionPatrol` and 1002224 / `Watergate_Block_Patrol`, each 259200. The product only allows these two keys and names, with a positive timer and direct spawn reference. The other 52,078 stages, including sentinel values and questtimers, remain unchanged. `stageinfo` is already included in the original Hashmanifest. Evidence: `.local/phase5-v3-stage-*.txt`, `phase5-v3-stages.json`.

**City flight:** The reference to condition 1011130 comes from [Flight Freedom, signatures.h](https://github.com/shin2344234/flight-freedom/blob/d61a9c37fc2fc0970d155329ddbf143038f0910d/mod/src/game/signatures.h) (MIT); Search note, no adopted plugin code. Our own static check in the installed build confirms the expression `IsInTown() && !IsAboveRoad(Bird,20)`. The table is additionally hashed; Her body/header hashes are in `steam-25381195.advanced.json`.

ConditionInfo reads the tree over `0x1422b4300`, then three u8 flags, original expression and parser type. Factory `0x142440540` uses Opcode 1 for `AndContentsLogicFunction`, 2 for `NotContentsLogicFunction`, 3 for a ConditionData with a u16 type. Type 2 has RTTI `ConditionData_CheckNone` (VTable `0x145950b00`), no payload and answers the check with Yes=0. `NotContentsLogicFunction` (`0x1424414f0`) converts 0 to No=1, 1 to 0 and gets Unknown=2. So `!CheckNone()` is a busy false condition, not a guessed flag. Original key, stringkey, blockbyte, three flags and parser type are preserved; Expression and tree are consistently replaced. The table header is then rebuilt. Other 10,797 conditions remain byte-identical. This applies to the common city flight rule, also for other flight mounts. No statement about quest/intersequence descents or height limits. No change to EXE, Runtime or Saves. Evidence: `.local/phase5-v3-condition-*.txt/json`.

The additionally tested candidate `MercenaryInfo._isMustHideAndRestore` is already on 0 at Blackstar. It is therefore not offered as an apparent border clearance. All item repair lists remain empty; No cost field was derived from the existing repair UI/component names. General NPC timers and engine caps remain open.

### Previous sources

- [crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0), locally pinned
Commit `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0`, MIT; Existing item port.
- Locally pinned NattKh modding tools and NattKh editor served as
format references. No transfer of their parser code to the new core readers; Their old layout assumptions were tested on the current EXE reader.
- Own limited in-memory readers under `mods/advanced/reader.rs`;
New item facade `vendor/crimson-format/src/item_mods.rs`.
- Private extracts and disassemblies are ignored under `.local/`,
no game tables/EXE snippets are shipped with the source project.
- `pinned_advanced_records_and_edits` checks the previously extracted sources
explicitly against the published hashes, then identity, mutations, offset new build, positive resource gains and stat insertion. The test is conscious outside of this local research environment `ignored`.


## Residual test v0.5.7: Item durability and other stocks

The basis remains the hash-tested EXE 1.0.0.2944 of the Steam build 25381195. Only file reads, no access to process memory or save content.

`ItemInfo::_maxEndurance`: Reader `0x14150fd15`/`0x14150fd25` reads two bytes in object position `+0x400`; ItemInfo resolution over `0x14038ab60`. The common durability updater `0x14240d650` compares with `0xffff` at `0x14240d687` and then skips the write of the instance durability (`+0x40`). In the same updater, the Sentinel check also applies to attached subitems (`0x14240d75c`). Further broken/usability tests also compare with 0xffff, e.g. Functions `0x14e6f9230`, `0x149d52c90`, `0x14ed246d0`. Thus 65,535 is a proven special value, not an invented large durability number.

122 of the 6,816 items have finite, positive durability. The new writer replaces only this u16 and leaves 0 and 65,535 unchanged. `is_destroy_when_broken`, repair lists and other flags remain byte-identical. Together with EquipType factor 0, the existing `no_wear` switch reaches it; no change of the stored request/template format is required. All 6,816 records were tested for precise field diff and idem potency. A no longer existing entity is not restored by this.

Inventory: Reader `0x1415087c0`; Move array `0x141535d90`, Move record `0x141508490`, nested ItemMoveData `0x141508360`. Selector lists each contain u16+u8. Move: u8, two u16 references, u32 item reference, three localization texts, Count + 17-Byte ItemMove records, optional condition, localization text. The twenty non-character records have no embedded Move condition. Unknown condition variants are rejected. Character keeps his hash-tested slot start at 9,272 because his tree is still opaque. The complete suffix is checked up to EOF.

Newly released IDs and slot offsets: Kuku 13/117, Housing_Dresser 15/148, Housing_Refrigerator 16/147, Housing_Symbol 17/130, Housing_Collecting 18/141, Housing_GatheredMaterials 19/164. These positions result from the reader; a search for plausible slot numbers would be ambiguous in the dresser (0/0 at 144). Character and both previous camps remain supported. Currency, quest, recovery and vehicle containers remain without an editor. 1,460 is still a cautious boundary from a known inventory path, not a universal save proof.

Stack code path: `0x1424078e2` sums up the instance sets over the inventory. If `applyMaxStackCap` (`ItemInfo +0x111`, check `0x142407a32`) is set, the remaining quantity is derived from maxStackCount minus sum at `0x142407ad6`. The flag thus also affects a total lid; It is not blindly deactivated. A universal maximum does not follow from this.

Further spawn names `_spawnTime`, `_spawnableTime`, `_spawnDelayMin` and `_spawnTermMin` can be found in reflection code, but do not provide a proven NPC table timer. The environment contains, among other things, spawn volume and spline parameters. Without class/file assignment, no editor follows. The previously proven counter-examples (probability, savedata, timeline) remain relevant. No new general respawn patch.

Local receipts: `.local/phase5-v8-{focused,inventory-arrays,inventory-move, inventory-nested,inventory-conditions,gap-readers,endurance-consumers}.txt`, `phase5-v8-inventories.json`, `phase5-v8-pinned.log`. Disassembly and original content are exclusively ignored local research data.


## v0.5.8: complete BuffInfo and additional self-consumption costs

Own static analysis of the same pinned EXE build, no process access. `BuffInfo` reader `0x1414f98d0`: header (u32 key, UTF-8, block flag), u32 number of (u32 level, nullable polymorphic buff), min/maxLevel (u32), sequencer text, calculation type (u8), three 4 byte references and two Boolean terminating flags. Nullable Dispatch `0x141f2a190`, existing common buffreader/factory. All 292 records and both complete table files are round-trip-checked.

Type 114 `AdditionalUseResourceStatBuffData`, Reader `0x141f351f0`: first a u32-counted skill reference list (four bytes each), then a u32-counted list `UseResourceStat`. Common reader `0x141518720`: StatType u8, StatusRef u32, isRegen u8, varyStatAmount i64, IncreaseStatusRef u32, DecreaseStatusRef u32. 36 negative costs in 33 BuffInfo records. Only StatType 3 and status 1000026 (Stamina) / 1000027 (Mp) released. 0/50/200 percent patches replace only these eight bytes per entry; all other bytes are tested. The responsible `CommonAdditionalUseResourceStatBuffProcessor` (`0x1420a9eb0`) assigns the resource list to each referenced skill ID; Registration and removal are done via `0x1417addd0` / `0x1417adf40`. The addition thus applies to the skills named in the buff and is not a change in the general resource maximum or the other buff effects. New pure reading layouts are marked as opaque. Type 69 consists of two 4 byte fields after jumping from `0x141f310d0` to `0x14e04cb70`.

Two named skill identities represent self-consumption via type 12: 40013 `Active_UseResource_Mp` (30 values) and 10300 `Skill_ElementalReinforce_UseMp` (one value). Reader `0x141f2b970`: StatusRef four bytes, then three i64; Only the first i64 according to status 1000027 is scaled. The two remaining values (limits), common part and other payloads are preserved. No blanket scaling of all negative buffs: opposing drain and dot effects as well as negative regeneration modifiers are no own costs. Explicit `skill_buffs` values override calculated factors before the patch.

New source fingerprints in the Advanced Manifesto: BuffInfo-Body 486,717 Bytes, SHA-256 `67bb4107c1161a2f9684903d1053c061948a2f4381fe87f9e3c82b874d983ec7`; Header 2,338 Bytes, SHA-256 `a5ec6b9db438898c93baaec0d28e60056319a649ffad5833de0fdb9c69a7a85d`.

### Other excluded paths

The repair helper knows set -1 as a special path, but the normal wrapper reads the real resource set and rejects <= 0 (`0x140a11577..140a1157e`); also the server-side caller (`0x142be4311..142be4316`). This does not make a zero-cost patch safe. RepairAll has a different path, but does not eliminate the still achievable division in the single repair path.

`AICondition_IsOverMiseensceneSpawnableTime`: Vtable `0x1458c1bf8`, Check `0x1422e3b10` calls `0x1417160a0`. The check compares fields from NPC plan entries with the current time of day; No confirmed general respawn distance. No save, EXE or runtime changes made.
