# B1/B2: additional field evidence for build 25381195

Status: 19.09.2026, Workbench v0.4.2. Own static evaluation of the local EXE and comparison with the already hash-bound tables. No debugger in the game process, no hook, no start/stop, no save evaluation. game bytes and Disassembly remain exclusively in excluded local work files.

Investigated EXE: `bin64/CrimsonDesert.exe`, version `1.0.0.2944`, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. addresses below are virtual addresses of the file at ImageBase `0x140000000`; they are not used for running time. The result is a field proof, not a vanilla certification or in-game acceptance.

## Dealer and daily refresh

Reader ranges `0x14151b9a0–0x14151bec7` (stock) and `0x14151c09d–0x14151c3cf` (store section): field names in error paths, read sizes and order can be assigned to the existing record bytes. `C` denotes the measured position of the stock list count in the store.

|Feld|Diskposition|Width|Treatment|
|---|---|---|---|
|resetHour|C−17|u32|Preservation, including sentinel|
|ResetDay|C−13|u32|Only confirmed 3/7 to 1; Existing 1 unchanged|
|buyableStockCount|C−9|u32|Increase in new sales offers|
|sellableStockCount|C−5|u32|Received|
|List number|C|u32|Rewrite with actual position number|
|maxRefillCount|Stock+18|u32|Existing stock option|
|stockIndex|Stock+26|u32|Continuous for new positions; Maintained stock|
|importantSaveIndex|Stock+30|u32|above the previous maximum; Gaining gaps|
|refillByResetStore|Stock+38|u8|Obtained from existing normal template|
|isStockSellable / isStockBuyable|Stock+39/+40|je u8|Player’s Perspective: Sell/Buy|
|isRestoreItem|Stock+41|u8|Received from submission|
|Itemkey and duplicate|Stock+43/+102|je u32|Replace both together|

All 397 interpreted stores have matching buyable/sellable counts; all 6,376 existing stock indices correspond to their list position. 369 Stores have reset intervals 1, 3 or 7. The global day option changes exactly **246** Records from 3/7 to 1. 123 are already daily. 28 interpreted stores with permanent/unknown interval and 39 opaque stores are preserved. The eight September additional bytes are **not** the refresh field and remain raw.

The mod author of [Daily Store Refresh](https://www.nexusmods.com/crimsondesert/mods/3515) provides a separate plausibility check: his description also calls 246 normal 3-/7-day intervals. It affects another build and does not replace our own field proof.

## New articles and assortments

Only an already existing, unconditional, top-up sell position of the same dealer serves as a template. The template is 127 bytes long, has flags `[1,0,1,0]` at +38, item value type 0 and no optional condition, additional bytes or effects from +110. Their pricing factors and unknown indoor bytes are preserved. No game bytes are permanently embedded in the program code.

210 Traders have such a template. For two traders, existing importantSaveIndices are not unique; They are rejected. Thus **208 merchants** are released for additions. Non-consecutive stock indices or conflicting counts also block insertion. Existing records, terms, prices and save indices will not be renumbered or replaced. An existing purchaseable item position is not duplicated; a mere long position shall not prevent an additional offer to sell.

Selection from all 6,816 localized items, even complete selection, is possible. New positions receive trader exception, global stock or standard
999. Since v0.4.3 `vendor_options` per merchant can the common article selection
Replace or suppress with an empty list. Separate refresh exceptions force daily replenishment or get the original interval. Explicit exceptions also apply outside of global merchant selection; `null` inherits the global setting again, including its selection. Exceptions remain separate. The table index is rebuilt during record growth. Real tests check a small assortment and all items at merchants 3101 together with unchanged other records and retrieval of the encrypted overlay.

The Workbench limits a plan to 100,000 combinations of supported trader and selected item as well as 9,999 positions per trader. These are preview/parser limits, not empirically proven engine maxima. A complete catalogue at all retailers at the same time is therefore not yet supported. Special dealers, any types of items, prices, UI behavior and persistent inventory still require in-game testing.

## Drop chances and manual guarantees

The dropset reader is at `0x1414f9be0–0x1414f9e08`. The selection function `0x1422cfd70–0x1422cffda` uses a new random value in the range 0 to 999,999 for roll type 0 and checks whether its rate is greater. Type 1 limits successful entries to the roll number. Type 2 uses a weighted cumulative selection. Types 1/2 are therefore treated **not** as independent percentage chances or rewritten to type 0.

The new modifier only accepts unconditional item inputs with roll type 0, matching item duplicate, valid quantities, rate ≤ 1,000,000, unchanged zero/special fields and total weight equals sum of rates. In the bound build, **12,306 Sets** meet these criteria. All also have positive minimum quantities and are suitable for manual warranty selection. The 32 bit fields after the 64 bit rate are not changed as a second chance.

Calculation: `min(original_rate × multiplier, 1_000_000)` with wide integer; Then add the total weight again. Roll type, roll number and all other fields are retained. Quantity and opportunity are recalculated together from the original. Individual exceptions shall take precedence over global factor/selection; explicit guarantees set the base rate to 1,000,000 and in turn take precedence over opportunity factors. A guarantee with an effective quantity factor 0 is rejected.

“Guaranteed” means the individual chance **when triggering this set**. Superordinate events, runtime modifiers, other game conditions and boss assignments are not changed or automatically proven. Boss sets must be selected manually. random selection and outcome transmission are statically examined; a measured gameplay effect is pending. Quantities and global opportunities can also affect recipe/quest rewards.

## B0: Finding of the local Steam cache

For the installed build, only the two assigned Depotcache manifestos were read:

- Depot 3321461, Manifest 1190168329432671189: 282 Entries.
- Depot 3321466, Manifest 4235648687492059552: 4 Entries.

The names are unencrypted, metadata is available; However, both signature sections have length **0**. File comparison with these local hashes alone does not confirm their origin. It did not start Steam-Verify, nor a download or a complete 152 GB archive hash run. Structure recognition was checked against the primary [SteamKit-DepotManifest-Readers](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs); No code from it was adopted and no dependency added. Live Apply still requires a trusted confirmed baseline, verified livebackup and integrated launch protection.

Local receipts: `.local/phase4-field-xrefs.json`, `.local/phase4-reader-evidence.txt`, `.local/phase4-drop-runtime.txt`, `.local/phase4-steam-manifest-inspection.json`, `.local/phase4-v3-rust-tests.log`, `.local/phase4-v3-native/result.json`.
