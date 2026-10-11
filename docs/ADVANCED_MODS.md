# Phase 5 – World, equipment and skills

Read: 21.09.2026, v0.5.8 Preview. **Phase 5 is partially implemented.** The specification points expressly open below are not passed features. The manual in-game validation takes place on user request only after all development phases. Nothing was applied to the ongoing installation.

## Operation

In the **Mod Workshop** there are eight areas B4–B11 among the previous modules. All settings are first drafts. **Calculate Preview** shows the individual values and the planned files. **Export preview** and **Rehearse on workspace copy** use exactly this plan. After changes, it must be recalculated. The separate live application retains all B0 checks, in particular the closed game and the tested baseline. A visible invalid number also blocks export/sample and discards an already prepared live file approval, even if the last valid numerical value is still stored internally. After correction, Live-Apply needs a new file check.

|Module|Implemented|Border/open part of SPEC|
|---|---|---|
|B4 World|Character numbers and set limits in terrain/pool spawn groups; percentage factor and individual fields; Reset intervals of two spawn patrols and wait times of 108 Territorial reoccupation rules; MainField mount clearance, city run restriction and city flight rule; Duration and cooldown of the `Riding_*` characters|No general NPC respawn timers, no full script/questspawn coverage. In-game effects are untested.|
|B5 Dragon|Blackstar cooldown 0; duration 10/30/60/120 minutes or free seconds; Removal of the Mercenary lock 79 from region lists and targeted deactivation of the city flight departure condition|City flight rule also applies to other flight mounts. Quest/intersequence descents and maximum flight altitude remain unchanged. in-game validation open; 0 Seconds is not invented as unlimited.|
|B6 Inventory|Character, CampWareHouse, WareHouse, Kuku and Housing_Dresser, Housing_Refrigerator, Housing_Symbol, Housing_Collecting and Housing_GatheredMaterials; Starting places and maximum as free integers|Workbench allows 1..1,460, derived from the default cap in the checked inventory code path; No proof of all engine/save limits. Existing save slot numbers may deviate from the data standard.|
|B7 Stacks|Global size for already stackable items; additional category IDs; individual items; express experimental switch|1,000,000 is a Workbench test limit. Universal engine cap unknown. `apply_max_stack_cap` can additionally limit the total amount in the inventory and is retained.|
|B8 Durability|EquipType wear factor 0 plus Engine Sentinel 65,535 for all 122 items with finite durability|Current 6,816 items contain **no**`repair_data_list` entries. Free repair is blocked, even with existing rules. The found cost path does not allow safe zeroing; A suitable table path is missing. Scripted durability losses unchecked.|
|B9 Costs|skill/driver lists, 31 self-consumption values in two skills and 36 additional costs in 33 equipment buffs; Seven categories, individual exceptions and zero-cost switches|There is no universal unlimited function. skill categories derived from names; Equipment buffs separately. Positive regeneration, enemy drain effects and other status types unchanged.|
|B10 Skills|All 2,069 skills searchable; Known numbers can be processed; global/individual cooldowns; Full buff matrix inspection, 87,697 named base/resource fields and individual editing of confirmed i64 values including the three graph values|Unknown meanings/units remain marked as raw. References, types, counts, graph curves, and the newly split Summon/AddSubLevel payloads remain read-only.|
|B11 Items|edit stats and compatible buff pairs; copy and edit new Enchant lines from an original level; individual stacks; Reusable templates|No unconfirmed Buff IDs/levels. A new table level does not unlock an upgrade path and does not increase an existing item level.|

[Complete residual point test](PHASE5_REMAINING.md).

These open points require additional format/behavioural evidence. There are no hidden wildcard patches, runtime hooks, or changes to saves. Phase 6 and the optional runtime phase were not started.

## Rules and order

- Percentage values: 100 = unchanged, 50 = half, 300 = triple. Integer division
Rounded direction 0. Factors apply every time to the original, never to an already modified edition. Overflows are rejected.
- Spawn zeros and maximum sentinel values 255/65,535 are preserved globally.
Individual fields allow a conscious exception. Other border crossings lead to an error instead of silent saturation.
- **Patrol reset in percent** captures only stage 1017811
(Hidden Estate) and 1002224 (Watergate), both with direct spawn reference and originally `resetSecond = 259200`. 50 % gives 129600 seconds in the table units. Individual second values are available in the world browser; Workbenchlimit 1..31.536.000. No extrapolation to all NPCs or assurance of the same duration on the real clock.
- **Reset: waiting time in percent** scales exclusively `delayTime`
of the 108 `factionreblockadinginfo` rules. Original values: 24 × 86,400, 84 × 432.000. 50 % halves these table values. Individual values in the world browser (search by area name/ID), limit 1..31.536.000. Factor 1..10,000 %, off by default. Quest conditions, probabilities, other quest timers and area links remain byte-identical. This is not a general NPC respawn rule; Game time unit, progression dependencies and effect are only checked at the later in-game validation.
- City clearance and dragon region option deactivate same condition
1011130 (`IsInTown() && !IsAboveRoad(Bird,20)`) exactly once. All other 10,797 conditions including city, bounty and quest exams remain byte-identical. The common rule applies to all flight mounts.
- Individual skill factor replaces the global factor. Blackstar Options
Replace global mount values. **Explicit individual fields last have priority**.
- Inventory starting positions shall be less than or equal to the maximum. The change
of data standards does not replace save migration. Large numbers and reductions are to be checked before a release in the game.
- Unconfirmed fields, categories, skills, items, status IDs, buff pairs
and neither existing nor explicitly newly registered Enchant lines lead to an error message.
- Recognized as condition-dependent items are used in global stacks without
Experimental switch is omitted. Individual changes to a stack size above 1 are rejected. Equip types, finite durability, multiple enchant lines, passive skills/buffs, base data, load/sharpness characteristics, subitems and seal/container data are recorded. This proves no general stack compatibility.
- Item stats are signed raw numbers. Limit ±1,000,000,000;
`per_level` must fit into the actual i8 format −128..127. No invented percentage/damage units. buffs are derived from observed ID/level pairs of the same type of equipment; This also does not replace in-game test.

## New Enchant Levels and Skill Details

Select an item under B11, open **Add Enchant Level** and select an original level and a new destination number. **Remark Enchant Level** copies their entire line including stats, buffs and prizes. After that, stats/buffs can be changed for the new level. Only original stages may be a copy source; Existing/double targets, external separation data or unsorted original structures are rejected. Limit: 64 new and 256 total rows per item, level number 0..65,535 (format/Workbenchlimits). When a pre-registered stage is removed, its pre-registered stats/buffs are also removed. Templates also store these copying rules.

New buffs or additional enchant levels can make a previously ordinary item state-dependent. If it then has a stack size above 1, the preview requires the experimental switch or explicitly stack size 1.

Open under B10 after the skill selection **Buff matrix & complete record**. All 4,607 entries (including zero) can be found via matrix row, column, type names and field names. Since v0.5.4, the nine summon payloads are divided into named fields and the AddSubLevel payload into reference and unknown four-byte fields. Also named spawn/time fields remain write-protected here; Names confirm neither units nor editing rules. Coordinates/unclear number formats remain visible as hex values. The structure positions `mem_*` are not confirmed field names; Byte positions refer to the file record. The view shows the original data, not an applied mod. The entire original bytes including still unknown suffix values are available as a hexan view. For confirmed i64 fields **Overwrite ** and enter an integer between −1,000,000,000 and 1.000.000.000. Each change replaces exactly eight bytes; All other matrix bytes are retained. Units will not get out of numbers. Selected raw values and cooldown/cost changes are combined in the same plan. Without an explicit buff change, the matrix remains completely byte-identical.

## Basic data, prerequisites and resources of a skill

Under **Buffmatrix & Complete Record** open the section **Basic Data, Requirements & Resources**. It shows all fields before and after the buff matrix: group/parent references, upgrade graphs, character/condition lists, status resources including regeneration flag, item resources, battery, UI flags and developer texts. The search finds field names, identifiers and original values. references are marked as such; Unconfirmed number formats such as `useItemCount` remain visible as hex values.

This area shows original values and is completely read-only. Continue editing already released numbers in the skill fields above; Changes there are retained during filtering. An original value in the detailed view is therefore not necessarily the pre-recorded value. No additional write privileges are derived from field names or references.

Since v0.5.6, the reader determines the end of the buff matrix directly from its format. `skillGroupKey` is a group reference and does not have to match the record key. The test reconstructs all 2,069 complete skill records including header and suffix byte-identical.

## Templates

Select an item, save values, assign a name and use **Save an item template**. The template contains only the item settings, no game files. It remains in the local WebView store of the same app. With **Apply template to Item** it is applied to the selected Item; the preview re-examines its actual suitability. Saving under the same name replaces the template. Maximum 100 templates, names up to 80 characters. The templates are not yet phase 6 profiles. Deleting the app memory removes them. Corrupted or formally invalid entries are not loaded.

## Technical demonstration

Parser, construction and boundaries: [Recherche](research/ADVANCED_TABLES.md). All changed tables enter the same B0 overlay. For longer item/shorter region data sets, header offsets are rebuilt. The primary check is carried out before each semantic interpretation using the known build/table hashes. Additional small tables are pinned separately in `crates/cd-core/schemas/steam-25381195.advanced.json`.

Automatic and manual checks: [TESTING.md](TESTING.md). Roundtrip and workspace rehearsals prove file consistency and withdrawal, not the gameplay effect.


## Unsupported repair options in old templates

Since v0.5.5, `free_repair: true` is always rejected both globally and per item. The material costs can be used in the game as a divisor; Zeroing is therefore not a released way to free repair. The switch for wear remains independent of this. Old templates remain readable: A repair option activated in them can be de-selected. Stats, buffs, stack size and enchant copies are retained. Then calculate the preview again and save the corrected template if necessary.

## Additional equipment and skill costs

Under B9, **equipment buffs** regulates the additional resource lists from `buffinfo`: 100 percent gets the original, 50 halved, 0 removes the confirmed own costs. The browser **Buff own costs** allows the search for buffname/ID and individual exceptions. The category affects the skills deposited in each buff; their references are not changed.

**Other Skills: Spirit** additionally captures the Buffmatrix consumption values of the skills 40013 and 10300. Individual values in B10 → Buff matrix have priority. The common zero-cost switches set all seven categories of the chosen resource; Existing individual exceptions are expressly retained. Positive resource values and regeneration flags are not zeroed out, nor are foreign attack/dot effects or buff runtimes. The function is not a promise of unlimited resources in any engine or script path.
