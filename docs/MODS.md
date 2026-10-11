# Mod Workshop – B0 and B1–B3

Since v0.4.4 adds **check the file list** the mod workshop: Steamdepot comparison for names/sizes, EXE finds and archive groups. The report does not read PAZ content and does not yet confirm vanilla. [Operation and limits](INSTALLATION_CHECK.md).

21.09.2026, desktop v0.5.8. Phase 4 is developed to the documented extent; The in-game validation remains open. New B4–B11 functions and their still open points are available in [ADVANCED_MODS.md](ADVANCED_MODS.md). The Workbench calculates its own table changes and PAZ/PAMT overlays, displays its complete file list and can export and sample them in the project. The separate [Live application with Restore](LIVE_APPLY.md) has a controlled write path. Setting up and writing operations require an ended game. Preview and workspace rehearsals may run while playing; saved games remain unopened.

After playing, **Check file contents** can compare the complete installation with the local depothashes. progress, abandonment and export are in place; blocked during ongoing play. [Scope and limits](CONTENT_AUDIT.md).

## Operation

**baseline & Registry-backup** takes over an exported content report after a current metadata/registry comparison. Report and verified registry copy will remain in the project and can be re-verified later. [Operation and confidence limits](BASELINE.md). The manual in-game tests take place on user request only after the end of the development of all phases; Until then, they are considered open and do not block further development.

Under **backup & Restore**, you can load existing protected workspace rehearsals, check their backups and restore interrupted samples after a concrete return preview. Already reset samples are displayed accordingly; Missing fuses and intermediate changes block the process. [Operation and limits](BACKUP_RECOVERY.md).

1. Open in the desktop app **Mod Workshop**.
2. Shop inventory (default 999), daily refresh, additional items, drop quantities/
Opportunity Multiplier or Trust Multiplier.
3. Optional limit merchants/dropsets via name or ID. No choice means
all released records. Individual exceptions expressly take precedence over global value and choice. A factor 1 preserves the baseline value. Additional articles and guaranteed dropsets, on the other hand, have no effect without selection. A manual guarantee shall take precedence over opportunity multipliers; it sets the base rate to 100 % when the selected set is triggered. Under **Merchant Details** you can set your own item sets as well as daily refresh or original interval per merchant, even outside the global merchant selection.
4. **Predict**. Field, record, before/after value and file hashes
check. After a change in settings, the preview must be recalculated.
5. **Rehearse on workspace copy** performs Apply twice → Archive readback → Reapply →
Archive Re-read → Restore and then an intentional cancellation with recovery in a new `.local/rehearsals/…` folder. No original ETA is copied or altered; The registry is there exclusively as a test copy. The result shows the six file transitions including hashes and remote altgroups. New v3 samples hold start/source/backup locks on their own test files and check the rejection of a simulated source change. Details: [Protected workspace rehearsals](PROTECTED_REHEARSALS.md).
6. **Export preview** creates `exports/mod-preview-…/preview.json` and
`unapplied/…` with the generated candidate files. This is an unapplied, not yet in the game taken overlay. The manifesto contains all changes, open gates and source attributions.

The table shows at most 150 filtered changes at the same time; The export contains all. Large game values are transmitted as decimal strings. Factors are currently integers 0–1000, shop stocks 0–1.000.000. These limits limit the Workbench inputs; they are **not a confirmed engine maxima**.

## Coverage of the actual data build

The known EXE/metadata and table hashes are tested before interpretation. A matching reading schema is still not a vanilla proof.

|Area|Implemented|Still open|
|---|---|---|
|B0|Deterministic byte builder, encrypted LZ4 overlay, PAPGT registration, file preview; private core with backup, immutable transaction proof, reapply, up to eight groups, restore and recovery; Windows start/source/backup locks, update rejection, live setup via Steam user confirmation and fresh source hashes, live apply/restore, original data view to Apply, base change after updates, adjusted depot inventory and confirmed third-party additions|Actual power outage/in-game collection; Unknown foreign registry/original changes remain blocked|
|B1|Stock of 6,376 positions in 397 traders; selection and stock exemptions; Additional items/article sets and individual assortments at 208 dealers; Refresh exceptions per trader; Daily refresh at 369 interpreted intervals (246 changes)|39 opaque dealers, additions at special dealers, complete catalogue at all dealers simultaneously, in-game acceptance|
|B2|Quantity factors in 12,736 sets; Opportunity factors in 12,306 independent sets, selection/exceptions and manual 100% base rates|Automatic boss assignment, limited/weighted odds and sentinel sets; In-game effect|
|B3|Positive sets of the three `DropSet_Friendly_*` records; Negative values are retained|Effect/clamping in the game|

From 14,747 Dropsetrecords 13,035 Item variants byte-identical are reconstructed in a typed manner. Of these, 299 have empty lists, quantity sentinels or unreleased min/max ratios. They remain unchanged in the event of global quantity changes and are rejected in the event of explicit selection. Three friendly records are interpreted separately, 1,709 further variants remain opaque. Recipes and quest rewards also use dropsets; A global change affects this with. There is no automatic boss classification.

## Field and format certificates

**Store:** Key u16, name with u32 length; measured list positions relative to the name end +47 (310 Records), +95 (85), +99 (1), +115 (1). The Workbench only accepts exactly one suitable candidate with owner key, item duplicate and complete list round trip. She chooses neither a legacy format nor a best-fitting parser. The stock is in the current stock record at +18 as u32. The discriminator at +42 must be 1, item keys at +43 and +102 must match. 114 Headbytes is followed by an optional block, **eight raw September bytes** and a list of 12 byte entries. All unknown bytes, prices, conditions and external record parts are preserved. The eight bytes are not interpreted as refresh. The confirmed refresh/index fields, offer templates and their limits are in [B1/B2 field evidence](research/PHASE4_FIELDS.md).

**Drops:** The confirmed item branch has 64 bytes per drop. Min/Max are 64 bits; the preview refuses sentinel/overflow values. The separate odds option changes only `rate` and the total with supported roll type 0, with scale 1,000,000 and cap to 100 %. Roll type, roll number and unknown fields are retained. The friendly branch additionally has 28 raw bytes; These will be preserved. Its Min/Max values are interpreted with signs. Current evidence: Donate +50, Talk +5, Threat −200. At x3 the isolated output is +150, +15, unchanged −200. This is a byte/schema proof, not yet a measured NPC confidence gain in the game.

**Pair of tables:** Empty replacement reconstructs real bodies and headers byte-identical. The general rebuild feature preserves physical record order, prefix, and original header key order, and recalculates offsets when size changes. In addition to the artificial growth test, real shop additions check both small item sets and all 6,816 items at a single retailer together with unchanged other records and readable overlay.

**Overlay:** Own implementation over existing MIT crypto/compression base, no CDUMM runtime. LZ4 blocks, ChaCha per correct filename, alignment 16, Jenkins checksums for PAZ/PAMT/PAPGT. SHA-256 serves separately as proof of identity. The encryption header bytes and speech mask come from the tested source group. The new group is at the forefront in PAPGT, original entries and headerbits remain. The ID does not conflict with registered/reserved groups or with existing folders. The reader confirms the generated files in an isolated copy. The priority in the current game is thus not proven.

## Range and limits of selection

Additional items are attached to existing normal sales offers. Already purchaseable items are not duplicated. The template comes from the same trader and preserves its price factors. Existing goods/save indices are retained; New indices follow the previous maximum. A pure long position does not prevent a new sale offer. The stock of new offers follows the merchant exception, the global value or, by default, 999.

“Select all items” refers to 6,816 items. A plan allows maximum 100,000 combinations of supported trader and item, a trader maximum 9,999 positions. For the complete catalogue, therefore, choose individual dealers. These limits protect the preview and are not proven engine maxima. Explicitly selected unsuitable merchants/dropsets lead to an error message. Global options receive unsupported variants; the cover remains visible.

**Merchant Details:** An own product selection replaces the common additional product selection for this merchant. An empty list of its own adds nothing there. “Global attitude” or deactivated own selection again inherits value and scope of the global selection. Explicit exceptions also apply outside the selected merchants. “Keep the original interval” suppresses the global daily refresh without inventing a new interval. Exceptions remain separately adjustable. The plan limit counts the actually selected item sets.

## Transaction sample and release limit

The core can be reached via protected workspace rehearsals and since v0.4.8 via the separate [Live-Adapter](LIVE_APPLY.md). The confirmation of origin is explicit; Observed hashes alone are not enough. Windows Process Verification errors refuse the process. The following paragraphs also describe the older workspace rehearsals and are not considered proof of an actual in-game tests.

New v0.4.3 samples also use a hash-bound protection manifest as well as retained Windows file/folder handles and process path checks. The blocks concern only a copy of our own EXE and a synthetic source; no handle locks the running game during the rehearsal. After creation and hash checking, the registry backup is also held against changes.

Since v0.4.1, the sample has an exclusive copy lock and a verified registry backup. Each transaction first writes an immutable `intent.json` with a before/after state. Both overlay files are created in a private staging folder; After checksum, decompression and length checks, the complete group is published. In Windows, release and registry replacement are done with `MoveFileExW` and write-through.

Reapply reserves new group IDs and always builds from the original base. Only after provision of all new groups will the registry be replaced atomically; only then demonstrably own old files are removed. The private core supports up to eight groups and denies duplicate virtual table paths. The current B1–B3 modules still require only one group.

Recovery decides on the basis of the actual registry between before and after state. An immutable `complete.json` binds the termination to the hash of the intent. Incomplete private staging files remain for diagnosis; known sections of backup and proof of completion can be written on. Foreign changes, foreign group content, collisions, reparse points, hardlinks or deviating registries/backups prevent further changes. Only own files are removed, without recursive deletion.

Tests break to **69 Apply/Reapply/Restore write limits**, including semi-written files, and reopen the core for recovery. Abortions during recovery are also resumed. This proves these discharge limits, **no complete robustness against any I/O failures or power failure**. The history shall be preserved; There is no automatic archive cleanup yet. The former v1 core remains unchanged as a test reference. Old v1 samples are not migrated; For v2, a new workspace rehearsal is created. The test that an exclusive Windows file handle prevents a parallel start also runs exclusively with a copy of our own test EXE. Since v0.4.3, this mechanism is integrated together with a complete process path check. Live B0 captures all EXEs and original files of the full test report; In fact, three EXEs were observed. Any external or renamed start copies are thus not claimed as checked.

A foreign change/game update is not overwritten with an old backup. The live connection is tested on artificial installations; The real in-game test remains open.

## CLI

```powershell
cargo run -p cd-cli -- mod-info
cargo run -p cd-cli -- mod-preview .local/mod-request.json --output exports/mod-plan.json
cargo run -p cd-cli -- mod-rehearse .local/mod-request.json
cargo run -p cd-cli -- mod-export .local/mod-request.json
cargo run -p cd-cli -- mod-recover .local/rehearsals/ORDNER-ID
```

`mod-recover` only accepts marked v2-/v3-samples under `.local/rehearsals/<id>` of the same project. It completes an open transaction, then restores the original **copy** and saves a new recovery report. It does not require a current table parser; Protected game/save paths and unmarked folders remain excluded. V3 also requires the appropriate protection manifeststhash and unchanged source files. V2 is not automatically upgraded; A v3 downgrade is rejected.

Example of a configuration (all other values are unchanged):

```json
{
  "shop_stock": 999,
  "vendors": [3101],
  "vendor_overrides": { "3201": 50 },
  "vendor_options": {
    "3101": { "items": [2200], "daily_refresh": false }
  },
  "daily_refresh": true,
  "shop_items": [2200, 50001],
  "quantity_multiplier": 3,
  "chance_multiplier": 2,
  "dropsets": [175521],
  "chance_overrides": {},
  "guaranteed_dropsets": [],
  "trust_multiplier": 3
}
```

Unknown fields, unreleased explicit IDs and values outside the boundaries are rejected. The request file is limited to 1 MiB. Expenditure ends up only in `.local/` or `exports/`; The original game remains protected.

## Next necessary work

Independently confirmed complete vanilla hashes and backup/update paths are a prerequisite for a live writing path. A locally cached Steam manifesto name alone does not certify this base. This requires more comprehensive transaction/start protection tests and manual in-game acceptance from [TESTING.md](TESTING.md). Both examined local Steam depot manifests contain no signature data; their hash stock alone is not independently verified proof. B1/B2 are implemented to the extent described, universal merchant coverage and gameplay acceptance remain open. A Steam check or game start is not triggered while playing.
