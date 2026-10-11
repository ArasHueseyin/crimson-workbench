# Crafting planner – phase 3

Status: 19.09.2026, Steam-Build 25381195, EXE 1.0.0.2944. The app calculates **1,108 recipes with a unique item result** from the local installation. Dealers, prices, stocks, regions and the assignment of dropsets to specific opponents/world objects are still open. The planner is usable, but A2’s full source-mapping scope remains incomplete.

## Operation

1. Open **Crafting planner**, search for an item and enter the desired total quantity.
2. If several recipes exist, select a variant in the material tree. Interchangeable ingredient groups offer a choice of actual item names.
3. Enter existing stock, including intermediate products or the target item. Stock is consumed only once across the whole tree. Clear the manual quantities to start over.
4. The shopping list summarizes missing materials. Choosing to acquire an intermediate product stops its production branch.
5. Expand material rows to inspect item details, known drop-set references and explicitly unverified vendor/world sources.

Recipe links in item details open the appropriate recipe. Selection and manual quantities survive navigation, but reset when changing installations, reloading data, changing the catalog language or restarting. The planner does not read saves or apply game changes.

Each ingredient group uses one selected material. The default is the first choice sorted by localized name; this is not price optimization. The planner does not optimize mixtures of replacement materials within one group slot. Knowledge and conditions are prerequisites, not unlocks confirmed from a save. A missing calculable recipe does not imply that an item cannot be crafted in the game.

## Where the recipes are

Archive group `0008`, directory `gamedata/binarystaticinfo__/bin`:

|Table|Rows|Utilisation|
|---|---:|---|
|`multichangeinfo`|18,576|Tools, prerequisites, solid materials, ingredient groups, result dropsets|
|`itemgroupinfo`|1,602|Group members and subgroups|
|`dropsetinfo`|14,747|Result items and quantities; also general Item→Dropset references|
|`crafttoolinfo`|19|tool keys; Continue the already checked table stock|

All indices have a u16 line counter. Multichange and Dropset use u32 keys, ItemGroup u16 keys; offset in each case u32. The recipes come **not** from a scan of any item IDs or a built-in recipe list.

The previous 28 table fingerprints continue to apply. The two new ItemGroup fingerprints are available in `crates/cd-core/schemas/steam-25381195.crafting.json`. They are compared before interpreting. The crafting scheme is independently implemented for this build; the existing item search index `…gamedata-2.3-v2` does not change because it does not store recipe data.

## Measured data set structure

All numbers little-endian. `bytes` = u32 length + bytes; `text` = u8 category + u64 localization index + `bytes`; `array(T)` = u32 number + T entries. Unknown fields are preserved, but not named with invented meaning.

**MultiChange:** u32 key, bytes stringKey, u8 blocked, u16 tool, u8 consume, array(u32 reference + text) conditions, u32 knowledge, bytes tag, 5 raw flags, array(fixed), array(group), u32 elemental, array(u32) states, text description, text name, 3 u32 string references, text complete, array(u32) results, array(u32) additional.

- `fixed`: u32 item/character/gimmick, u64 count/coupon, u16 enchant – **30 Bytes**.
- `group`: u16 group, u64 count, u16 unexplained additional field – **12 Bytes**.
- 17,069 rows carry consumption type 3 and belong predominantly to
amplification variants; The calculator does not interpret them as normal recipes.
- All 1,507 rows with consumption type 1 have exactly one result dropset reference.
399 of which are excluded due to special/random/amplification results.
- All 18,576 records are read in full and returned from their fields
Serialised; The bytes correspond exactly to the original.

**ItemGroup:** u16 key, bytes stringKey, u8 blocked, text groupName, array(u16) groups, array(u32) items, array(u8) unexplained types, 11 crude final bytes. The final bytes contain known field candidates such as sorting/icon, but are not used semantically here. All 1,602 lines roundtrip exactly. subgroups are recursively united and deduplicated; Cycles, blocked groups, unknown group types and missing members are not silently ignored.

**Dropset, limited item reader:** u32 key, bytes stringKey, u8 blocked, u8 rollType, u32 rolls, bytes condition, u32 tag, array(itemDrop), u16 slots, u64 weight/totalRate/unknownTail, bytes originalString, u8 unknownEnd.

`itemDrop`: u8 flag, u32 item/unknown/kind, 5 raw bytes, u32 condition/post, u64 rate, u32 rate2/unknown2, u64 max/min, u16 enchant, u32 duplicateItem. The confirmed simple item entry is **64 bytes**, not the unchecked 68 bytes of the read reference parser. Variant flag=1/kind=0 is read; Other variants remain completely opaque. 13,035 Dropset records consist exclusively of this read variant and roundtrip exactly. The other 1,712 are not “fixed” by search markers or interpreted as successful.

For calculable results, additional conservative conditions apply: one item, equal positive min/max, no additional conditions or special fields, matching duplicate item ID, base/standard gain level, rate and total value 1,000,000, confirmed simple roll configuration. General table rates of other dropsets are shown raw; from this no unconfirmed percentage chance or world source is calculated.

The index/record limits of all three tables are also reconstructed byte-identical. A round trip proves the byte coverage, not automatically the gameplay significance of each field. Unclarified consumer rules remain excluded from the predictable part.

## Concrete local cross-checks

- MultiChange 1 → Dropset 160100 → Item 50001: 5 Timber (710001) and
1 iron ore (720001) make 30 arrows. Target 31, stock 3 timber: 2 operations, 60 created arrows, 7 timber and 2 iron ore are missing, 29 arrows left.
- MultiChange 343 → Dropset 1015536 → Item 1002066, “mixed stew”:
5 Water and four ingredient groups with quantities 2/3/2/2. The seafood group 17494 resolves to 48 items. The group is not misunderstood as a need for all 48 alternatives.
- All 1,108 released recipes generate a limited amount in the real file test
Plan. Further game rules, unlocks and effects were not tested in the current game.

## Calculation rules and limits

`batches = ceil(missing quantity / output per batch)`. Exact u64 arithmetic with overflow test; via IPC/JSON are sets of decimal strings. Stores and surpluses produced are part of a common ledger for the whole tree. The user can procure intermediate products externally. Recipe cycles generate a visible warning and a procurement item, not an endless loop. Maximum 1,000 tree nodes, 32 levels, target set 1 to 10^12. No probabilistic calculation of expected values, no global cost/recipe optimizer.

CLI:

```powershell
.\target\release\cd-cli.exe recipes --item 50001
.\target\release\cd-cli.exe craft .local\plan.json --output exports\materialplan.json
```

`plan.json`, self-designed in the project:

```json
{"target":50001,"quantity":"31","owned":{"710001":"3"},"recipes":{"50001":1},"choices":{},"acquire":[]}
```

JSON outputs use the same protected output policy as the previous core. The computer does not have a writing method for game archives or saves.

## Sources and independent implementation

The pinned sources and licensing decisions are in [CREDITS](../CREDITS.md) and [of table searches](research/TABLES.md). The NattKh schemes provided field candidates; their registration order is not the serialization order. The MIT crimson-rs loaders occupied keys/names and index widths, but did not provide a recipe calculator. NattKhs MPL Dropset Editor was read as a format comparison; Its byte format was not adopted unchecked. The new Rust reader/writer and the planner are their own implementations based on the locally measured data. No foreign python/editor code and no game file has been copied into the project.
