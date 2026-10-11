# Reference research: own application engine, shops, drops, trust and skills

Status: 19.09.2026. The basis is `SPEC (1).md`. CDUMM serves exclusively as a technical reference. No CDUMM export destination, no installation, no call and no runtime dependency. Phase 0 contains no implementation of the app and no writing tests on game or save files. The game is allowed to continue during this research. Subsequent Apply, Restore and Recovery paths must be locked while playing and re-checked immediately before committing; A one-time process check does not prevent simultaneous starting.

## Sources and licences

|Source|Examined revision|Commitdatum|Classification|
|---|---|---|---|
|[CDUMM](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/commit/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6)|`b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6`|13.09.2026, 19:33:13 +03:00|Current source code evidence, no runtime validation performed on this machine|
|[GildyBoye ShopEditor](https://github.com/GildyBoye/CrimsonDesertShopEditor/commit/c92d9153055c7a371c2c0cca51fc49d7c1c23d53)|`c92d9153055c7a371c2c0cca51fc49d7c1c23d53`|16.08.2026, 16:51:10 −04:00|Before September Update; historical notes, do not take current offsets|

CDUMMs [Root-LICENSE](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/LICENSE) is MIT, copyright 2026faisalkindi. In the case of copied essential parts, copyright and permission text would have to be carried. This is not a blanket statement about all contained files: [dropset_writer.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/dropset_writer.py#L1-L11) expressly carries MPL-2.0 and copyright RicePaddySoftware; the vendorte [Skillparser](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/_vendor/skillinfo_parser.py#L1-L7) also MPL-2.0. [schemas/NOTICE](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/schemas/NOTICE) identifies the complete NattKh scheme as MPL-2.0. [overlay_builder.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/overlay_builder.py#L42) designates ported DDS parts from JMM/MPL-2.0. None of these sources have been copied into product code.

For GildyBoye there is no `LICENSE` in the pinned file tree. [TermsOfService.md](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/TermsOfService.md) requires honest authorship, obtaining existing credits, naming original authors upon disclosure, describing edits as such and transparently documenting changes/values. This is not a general software copying permit. Therefore, only use format knowledge, write your own implementation. According to User-Spec also with knowledge from the editor credits in generated/distributed mods or Adopt profile/mod description, for example “format search: GildyBoye – Crimson Desert shop editor” with source link; Do not output your own generation as an output of your tool.

## B0: Overlay is plausible by reference code, not yet confirmed locally in the game

[overlay_builder.py, module description](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/overlay_builder.py#L1-L8) describes your own PAZ/PAMT groups whose virtual files take precedence over vanilla. The [Apply call](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/apply_engine.py#L2639-L2714) creates `NNNN/0.paz`, `NNNN/0.pamt` and an ownership mark. The number is determined from occupied and reserved IDs.

According to [PAPGT-Builder](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/papgt_manager.py#L401-L419), the concrete precedence follows the ** order in PAPGT: first hit wins**. New groups are therefore set before existing entries. It is not the larger numeric folder ID alone that gives priority. Workbench should generate exactly one merged output per virtual table path.

This makes “overlay for our data tables feasible” at the source code level yellow to green; “Started on the local build and every changed table loaded correctly” remains **open** until the controlled phase4 test. No archives have been changed for this. In case of body or header errors, a successful program start alone can be inadequate: affected shops, drops and skills must also be tested in the game.

### PAZ/PAMT and checksum chain

Proof: [build_overlay and _build_multi_pamt](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/overlay_builder.py#L845-L1044).

- Payloads are refilled to 16 byte limits. Size fields describe the actual payload sizes, not alignment padding.
- The considered builder creates one PAZ file per group. PAMT contains the external hash at offset 0; at +4 `paz_count=1`; at +8 `0x610E0232`; at +12 zero; at +16 the PAZ hash; at +20 the PAZ length.
- This is followed by size-predefined folder/node sections, 16-byte folder records and 20-byte filerecords. The latter consist of node reference, PAZ offset, compressed and uncompressed variable (each u32), u16 PAZ index (here 0), u16 flags. Virtual directory path and file name must match the installed PAMT.
- PAZ hash: `hashlittle(gesamte_paz_bytes, 0xC5EDE)`. PAMT hash: `hashlittle(pamt[12..], 0xC5EDE)`; Write this hash to PAMT +0 only after PAZ hash/length.
- `flags` must be assembled from the correct fields. Compression lies in the lower nibble, encryption in the next; ChaCha20 is `0x30`. Blindly copying the low 16 bits of a differently represented vanilla u32 flag mixes PAZ index and compression.
- Compression follows the real entry. Raw data remains raw, LZ4 is a block without a frameheader. Encrypted sources are also encrypted in the reference path in the overlay; the file name used for key derivation must remain correct. DDS details and `meta/0.pathc` affect texture overlays, not the table-only application path planned here.

The hash functions are additionally defined in [hashlittle.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/hashlittle.py), the LZ4 / ChaCha interface in [paz_crypto.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/paz_crypto.py). Integrity hashes of the game can be distinguished from cryptographic SHA-256 hashes for our backups.

### PAPGT registration

Beleg: [papgt_manager.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/papgt_manager.py#L1-L60).

`meta/0.papgt`: 12 Headerbytes, then 12 byte group entries, u32 stringtable size and NUL-terminated ASCII group names. One entry contains u32 flags, u32 name offset, u32 PAMT hash. PAPGT +4 contains `hashlittle(papgt[12..], 0xC5EDE)`. The other headerbytes are not zeroed flat: CDUMM gets +0..3/+8..11 and updates byte +8 with the group number; This limited approach is to validate against the local build, not to adopt as a universal parser.

Flags encode `is_optional` in bytes 0, u16 `lang_type` in bytes 1–2 and a null byte. New normal groups get optional=0, language=`0x3FFF`, so `0x003FFF00`. The [Reservation investigation](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/papgt_manager.py#L90-L115) also takes into account groups reserved in PAPGT that do not exist on disk. Since build 24934353, 0036–0040 are optional language package spaces. Never accept "0036 is free, folder missing".

If the PAPGT structure is not clearly recognized, the reference partly estimates the record number. Workbench must reject unknown/ambiguous forms. Similarly, our registrar may not automatically take over, delete or “repair” foreign groups; The vanilla / external mod gates from `docs/APPLY_DESIGN.md` are binding.

### Body and header are a couple

[table_ext.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/table_ext.py#L1-L40) documents the rename at 04.09.2026: `.pabgb` →`.staticinfobody`, `.pabgh` → `.staticinfoheader`. CDUMM resolves legacy aliases, but writes the actual name from the installed PAMT. Workbench must achieve the same physical name accuracy; Do not write the old suffix into the overlay.

For growing or shrinking tables, header offsets must be adjusted. [pabgh_rewrite.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/pabgh_rewrite.py#L1-L25) describes previous corruption by new bodies with old indexes. Header and body check together, stage together and roundtrip unmodified byte-identical. [Auto-Include](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/overlay_builder.py#L627-L676) only takes over a vanilla header if there is no explicitly recalculated header. This fallback idea does not replace offset recalculation for structural changes.

### merge, snapshot, recovery

[apply_engine.py, Merge](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/apply_engine.py#L3978-L4147) shows an important limit: byte merge with vanilla length can cut inserts. CDUMM selects a winner in size conflicts and reports omitted mods. Workbench should instead combine its own semantic changes before exactly one serialization per table; Report conflicting field values or incompatible structural changes before committing. Do not tacitly lose any change.

[SnapshotManager](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/snapshot_manager.py#L48-L76) documents that a rescan can incorrectly commit existing modified bytes as vanilla. Own hashes only prove identity, not vanilla origin. The snapshot process there also contains deletion/cleanup steps: in no case call as a reading scanner. Workbench requires verified initial hashes/trustworthy baseline, full backup proof and known build schemas.

[TransactionalIO](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/transactional_io.py#L120-L219) uses `.pre-apply`, renaming/moving and rollback. This provides helpful error cases, but does not prove a power failure-proof multifile transaction. For Workbench: persistent journal, flush, staging on the same volume, expected prior hashes directly before commit, group fully available before PAPGT visibility, recovery even with new files, old own group only removed after successful registration change. Restore will also be locked when the game is running and will not copy an outdated PAPGT via an update. [Deferred cleanup in the Apply Path](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/apply_engine.py#L2970-L3013) explains the specific error: prematurely deleted groups remain referenced in the old index after rollback and prevent the start of the game.

## Feature assignment and open borders

|Module|Concrete source finding|Phase-0 traffic lights and requirements|
|---|---|---|
|B1 Shops|`storeinfo.staticinfobody` + header; Stock list `_exchangeItemInfoListForSell` / `stock_data_list`; Item ID `value.payload.body` with duplicate `value.raw_q`; Stock/head values partly still `raw_*`|Yellow: Structure heavily occupied, all items/999/daily renewal not lump-secured; local full round trips and field validation required|
|B2 Drops|`dropsetinfo.staticinfobody`; Drops: `rates`, `rates_100` u64, `min_amt`/`max_amt` u64, Itemkey u32; Optional tagged trailers|Yellow: quantities known; Check probability scale and interaction with roll type/total weight locally. "100% Bossdrops" without boss assignment only manual selection|
|B3 Trust|`DropSet_Friendly_*`; Gildy marks historical offsets 59/63/67/71 relative to Name+NUL|Yellow/Red for Writable Multiplier: Note only, no validated current trust field, type/scale/limits open|
|B10 Skills|`skill.staticinfobody` + header; Resource Cost List and Buff List Known by CDUMM/Vendor Parser|Yellow for raw browser/cost; Global/per-skill cooldown remains unconfirmed in this research. No invented cooldown field names|

### B1: Eight new bytes are not the whole September difference

[storeinfo_native_parser.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/storeinfo_native_parser.py#L1-L91) describes variable stock list positions, not a fixed payload offset. The owner-storekey, plausible number and complete round trip serve as anchor. According to the [Pinned build variant 25116796](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/storeinfo_native_parser.py#L209-L254), **eight raw bytes were added between the optional `sub_data` and the count of `effect_list` per stock record**; in addition **a byte in the fixed store part**. The source mentions 436 stores and 6,376 stock records for their fixtures. These numbers are source statements, not a measurement of local installation. The eight bytes are zero and semantically unknown in these fixtures; Don’t call it “refresh.”

[storeinfo_writer.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/storeinfo_writer.py#L1-L28) preserves opaque areas of existing records unchanged and refuses new records with unknown non-zero values. “Every merchant sells everything” is therefore a structural change with additional limits, not a global u32 patch. GildyBoyes [Older parsers](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/index.html#L1421-L1486) works heuristically with signatures and old offsets; it must not replace current stock schemes.

### B2/B3: Separate trust notes from structured drop semantics

The MPL parser [dropset_writer.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/dropset_writer.py#L30-L124) contains additional dropset values such as `drop_roll_type`, `drop_roll_count`, conditions and `total_drop_rate`. Opportunities are therefore not to be treated as an isolated percentage without tested semantics. At `unk4==7` there are 28 bytes `friendly_data`, whose inner trust scheme leaves the parser shown raw.

GildyBoyes [Dropsetbrowser](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/index.html#L2408-L2444) searches for record names by text pattern and shows a 256 byte window. Its [Trust markings](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/index.html#L2520-L2526) mark only individual initial bytes. Neither derive u32/float nor multiply all friendly drops flat-rate. Reference mods and real current record limits must confirm type, scale and target values.

### B10: Confirmed Resource Costs Are Not Cooldown Detection

[skill_writer.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/skill_writer.py#L1-L66) is called `_useResourceStatList` and `_buffLevelList`. A resource record has 22 bytes: u8 Stattype, u32 Stathash, u8 Flag, i64 value, two u32 hashes. A DMM path `use_resource_stat_list[0].d` corresponds to this signed cost value. The source cites −20000 for certain spirit costs; There is no general divisor for all skills. The vendorte parser only confirms older builds in its description. The small [field_schema/skill.json](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/field_schema/skill.json) itself explains that its community field was not independently derived from a vanilla dump. As a current proof, it is not enough.

## Review of docs/APPLY_DESIGN.md

The existing design covers the necessary stricter gates compared to the reference heuristics. Concrete additions for phase 4: priority over PAPGT order; take into account all reserved language groups; output body/header together and after growth with new offsets; for own B0 tableoverlays do not unnecessarily change PATHC; Test in-game hash chain and SHA-256 backup hashes separately. Do not call Snapshot/Cleanup functions of the reference. Runtime confirmation is still open and will not be attempted while playing.
