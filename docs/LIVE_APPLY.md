# Live B0 – Setup, Apply and Restore (v0.4.8)

The desktop and the CLI now have a live writing path. It is not set up during the installation of the user ** and was not executed there**. The development and adoption of this version does not alter or block the current game. Manuals in-game tests remain on user request until the completion of all development phases.

## Model of trust

A saved cache report is not automatically allowed as a vanilla. For the setup, the user must explicitly confirm that Steams file check ** was completely terminated before the selected content report**. If this actual requirement is missing, have Steam checked later and then create a new complete report as well as baseline. The Workbench does not run Steam Verify itself and never confirms this step itself.

This is a user confirmation of the origin, not an independent cryptographic certification of the Steam deposit lists. The existing reporting fields `certified_vanilla` and `can_apply` remain unchanged `false`. Live-B0 additionally checks report, registry copy, known read build, installation path and file inventory. Immediately before setup, Apply or Restore, ** all captured original files except the separately handled registry** are fully SHA-256-checked, while their file handles protect against writing and replacing. The registry is checked against the tested backup and own history.

Previous documents required an as yet unavailable independent vanilla proof. The implemented admission path is specifically the Steam user confirmation explained above plus complete re-checking of content. A mere checkbox click without actually previous Steam check does not meet the requirement.

## Desktop workflow for the later test

1. Close the game. **Mod Workshop → Live application & restore → Load live status**.
2. Select a suitable stored baseline and **Live setup
Check **. Check path, report date and file number. Confirm Steam exam only if it was completed before this exact report.
3. **Check and set up the live base** creates the
Registry-backup in the transaction folder. No mod is used yet.
4. Set B1–B3 and calculate **Preview**. Next up **Live file preview
Calculate **. The preview shows actual group IDs, registry replacement and, if necessary, removal of previous own groups with before/after hashes.
5. **Apply tested mod to game** checks sources, process status, history and
Preview again. Only then will the prepared changes be written.
6. **Check Live-Restore → Run Tested Live-Restore** takes own changes
back. No modplan is required. When a transaction is interrupted, its actual commit state is first resolved and then reset.

Large installations take several minutes to compare content. The app remains operable; The live job can be cancelled. A cancellation during writing can leave a recoverable transaction. Therefore, check status and restore afterwards; There is no automatic rewrite attempt. An already successfully completed process is not subsequently presented as unsuccessful by a delayed cancellation request. After a change of view, **Load Live status** shows the current job of the same app session again.

## Protection and storage

- `.local/live/<Installationspfad-Hash>/admission.json`: expressly confirmed
Origin, baseline-ID, installation path and report/registry hash. This dataset will be published before the first live backup in the project so that a discontinued setup can continue with the same base. It alone does not mean a successful content comparison or backup completion.
- `<Spiel>/.workbench/baseline.papgt`: Verified original registry. Original PAZ/PAMT
are received and not patched. Staging and journal are also here, so that atomic renamings remain on the same file system.
- `<Spiel>/.workbench/transactions/<id>/intent.json` and `complete.json`: unchangeable
Before/after states with hashes and commit decision. New intents contain `started_at` as UTC epoch seconds; Old journals without this value remain readable.

Before reading large archives, all known EXE paths are opened and held against a new start. Already running processes are recognized by name and actual path. Process verification errors refuse the process. Source, parental directory and backup locks are maintained throughout the transaction. Abort/process status are checked during hashing at the latest about all 250 ms between read operations. There is no unprotected non-Windows fallback. The statement includes the recorded EXEs within the installation; Any external, renamed copies or unknown launch mechanisms are not claimed to be tested.

A separate, read-only view only accepts its own modified registry if original backup, full history, and actual overlay files explain it. Database, manufacturing and modbuilder then read the original groups; A reapply does not multiply already changed values again. This view does not take start/source locks and cannot execute transactions.

## Basic change after Steam update or file verification

Since v0.4.9, **Change base after game update** shows a separate process:

1. Complete Steam file verification after playing. The registry must already
Original registry correspond to the new base. An old restore is not a repair path for a new build.
2. Create File List and Full Content Report, Export Report
and new baseline. own files are shown separately based on the intact authorization and transaction history; Foreign files remain visible. An active own mod registry prevents a vanilla report.
3. Select new base and check **base change**. The preview calls old/new
Base, all files to be archived and the destination folder. Once again expressly state the confirmation of origin. Unknown reading schemes are denied.
4. When the game is over, execute. All new original sources are available under
Start/source protection freshly hashed. The new registry, meanwhile, is held against changes and is never written by the base change.
5. Previous own groups and the complete private transaction folder
unchanged after `<Spiel>/.workbench-history/<Nummer>/` renamed. The preview binds hashes of all archived files. No old original archives are copied and no archive statuses are deleted. After that, a new registry backup is created with an empty transaction history; then recalculate a mod with current settings and apply it explicitly.

The immutable project protocol is below `.local/live/<Installationskennung>/updates/<Nummer>/{intent.json,complete.json}`. The first registration record shall not be overwritten. An open process blocks Apply/Restore and Content Report and offers to resume with exactly its intended base. Deterministic `.part` files also allow continuation of an interrupted protocol release. A partially newly created registry-backup is only completed with suitable content.

Archive groups are moved before their history. Windows does not allow renaming with open child files: each tree is checked against its hashlist before and after; Afterwards, all archive files remain held against writing/deletion until completion. Simultaneous changes to other programs are not a supported workflow and lead to discontinuation in the event of deviations. Full original sources and new registries remain protected throughout. Colliding groups, third-party content, links, damaged backups and modified own mod files are denied.

The general inventory still does not read PAZ content. The assignment **Own Workbench files** is a metadata finding from your own history and not a re-checking of content. Original repositories can never disappear from the original comparison by this assignment. Well-known archives are checked in later inventories about names, sizes and structure.

Since v0.4.10, carefully checked additional files can be explicitly confirmed for unchanged retention. They remain foreign files and are freshly checked and protected from live writing. Modified original files and foreign registry registrations remain blocked. [Assisted handling, revocation and limits](FOREIGN_FILES.md). Real power outages and game effect remain unchecked; Manual in-game tests are postponed until after all development phases.

## CLI

```powershell
cargo run --release -p cd-cli --locked -- live-status
cargo run --release -p cd-cli --locked -- live-setup-preview <Ausgangsstand-ID>
cargo run --release -p cd-cli --locked -- live-setup <Ausgangsstand-ID> --review-id <Vorschau-ID> --steam-verified-before-audit
cargo run --release -p cd-cli --locked -- live-update-preview <Neue-Ausgangsstand-ID>
cargo run --release -p cd-cli --locked -- live-update <Neue-Ausgangsstand-ID> --review-id <Vorschau-ID> --steam-verified-before-audit
cargo run --release -p cd-cli --locked -- live-preview .local/live-request.json
cargo run --release -p cd-cli --locked -- live-execute .local/live-request.json --review-id <Vorschau-ID>
```

Request-JSON: `{"action":"restore"}` or `{"action":"apply","settings":{"trust_multiplier":2}}`. Unspecified mod parameters use the same unchanged default values as the mod preview. CLI write commands have the same origin, preview, source and process checks. The desktop version also offers the cooperative cancellation button.
