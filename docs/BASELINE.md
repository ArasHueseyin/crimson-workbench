# Saved baseline and Registry-backup – v0.4.7

The Mod Workshop can compare a full exported content review with the current file list, Steam repository lists, and registry. After that, it saves the unchanged report and a re-checked copy of `meta/0.papgt` in the project. Thus, the observed baseline is available after an app restart. The original archives are not copied or hashed again; This function is not a complete backup.

Since v0.4.8, the separate [Live-Adapter](LIVE_APPLY.md) can use such a stand with additional confirmation of origin and fresh protected source hashes. The storage described here alone does not set up Live-B0.

## Operation

1. **Mod Workshop → baseline & Registry-backup → Load test benches**.
2. Select a previously exported report and **report for baseline
Select **. Files, sizes, repository and build identity, and current registry content must fit the report.
3. **baseline save** copies report and registry to a new
Project folder. The preview is checked again before the storage. A modified installation or report file invalidates the preview.
4. Later select the stored baseline and **Stored status
Select **. The app checks the stored files and compares the current installation metadata. Old fuses are not overwritten.

The original full content check remains locked while the game is running. Their already exported report, on the other hand, can be adopted during play: This process only reads metadata and the small registry, does not use start/write locks and writes exclusively to the project. Registry handles allow read/write/delete sharing. No process is controlled.

## Content and publication

Each stand has its own folder `.local/baselines/<id>` with:

|File|Content|
|---|---|
|`audit.json`|Byteidentical copy of the selected complete test report including all observed SHA 256 values|
|`registry.papgt`|Copy of the registry, whose size and SHA-256 match the test report|
|`snapshot.json`|Versioned final manifesto with report/registryhash, metadata identity and creation time|

Report and registry are re-read and compared after writing. The current metadata is also compared again immediately before the conclusion. Only then will the manifest atomar be published as a new file. An interrupted copy without a valid manifest cannot be opened as a stored state. it is maintained for diagnosis; Repeat creates a new folder. No existing status is supplemented or replaced.

Entries are limited to local report files directly under `exports/` with names `installation-audit-*.json`. Report size maximum 8 MiB, Registry maximum 1 MiB, Manifest maximum 16 KiB. Unknown/multiple specified JSON fields, incorrect report type, partial reports, conflicting success flags, claimed vanilla/write authorization, duplicate or insecure file paths, invalid hashes, inadmissible sizes and conflicting custodian lists are rejected. Project and game/save path boundaries as well as hardlink checks remain active.

The lists are limited to 2,000 directory entries and show at most 200 suitable names. Linked entries are not offered. The user selects the report or baseline expressly; A new check or storage process is not automatically started.

## Significance of status

- **Metadata fit:** Saved report and registry copy are intact;
Current build/depot/file metadata and registry match the stored status. This is not a re-examination of the contents of the large archives.
- **Installation deviates:** The stored status is preserved; they are:
Current installation has deviant or incomplete metadata. There is no repair and no restore over a possibly new build.
- **Installation not testable:** The stored status is internally intact,
But the installation could not be inventoried.
- A corrupted saved report, a modified registry copy, or
An invalid manifesto leads to an error instead of a success status.

The time of the original content check remains visible. Similarly large archive changes with matching metadata can go unnoticed; an own test expressly establishes this limit. Before live apply, the relevant source contents must be checked again under the protection provided for this purpose.

The origin remains **local, not independently authenticated Steam cache**. The manifest is an auditable filing and not an external certificate of origin. `certified_vanilla` and `can_apply` remain false. The stored base is not yet approved as a trusted B0 live base; The registry copy cannot be written back to the installation via this view.

## CLI and Desktop

```powershell
cargo run -p cd-cli --locked -- baseline-list
cargo run -p cd-cli --locked -- baseline-preview installation-audit-<name>.json
cargo run -p cd-cli --locked -- baseline-capture installation-audit-<name>.json --review-id <review_id>
cargo run -p cd-cli --locked -- baseline-status <id>
```

The four desktop commands `baseline_catalog`, `baseline_preview`, `baseline_capture` and `baseline_inspect` require a current catalog session and are only allowed for the local main window. Delayed UI responses after navigation or session changes cannot re-enable old memory previews.

## Test sequence

The user has decided on 20.09.2026 to carry out the manual in-game tests only after completion of the development of all phases. They remain documented and are considered open until then. Automated tests, backup/path checks and the requirements for live write operations remain independent of this.

Seven new core tests cover memory content, input limits, interrupted release, outdated previews, manipulation/hardlinks, update status, retention of old backups, and unchanged source files. Four new UI flows check takeover, reopen, error/update states and discarded previews.
