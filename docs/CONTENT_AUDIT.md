# Full content check – v0.4.5

Once the game has been closed, the Mod Workshop can read all installed repositories in blocks, compare them with the SHA-1 values in the local Steam cache and additionally record SHA-256. No game files are changed, repaired, reset or locked against a game start. The result is its own report type, not B0-write authorization.

## Operation

1. Finish the game and select in the mod workshop **Check file list**.
2. With a complete, error-free file list and detected ended game
will check **file contents** available. The perimeter is indicated in advance; At the moment it is about 143,51 GiB or 154,097,618,899 Bytes.
3. Progress shows current file, read bytes and finished files. The
The test shall continue when the view is changed. **Cancel content check** requests cancellation. A change of installation or item language also cancels the order. A maximum of one job runs per desktop instance.
4. A completed report shows matching or deviating content. With
**Export test report** a new JSON is written under `exports/`. Timestamps and limits remain visible in the report.

CLI, also only when the game is over:

```powershell
cargo run --release -p cd-cli --locked -- installation-verify --output exports/installation-content.json
```

Progress appears on stderr, without `--output` the result appears on stdout. Content deviations result in a complete diagnostic report and exit code 2. A cancelled or incomplete run does not produce a complete report. Ctrl+C terminates the CLI; The desktop app has its own cancellation button. Reports are written only on express export; there is no automatic storage of partial readings and no resumption of a terminated hash run.

## Scope and origin

The examiner himself takes his file list from the current installation and the associated depot lists. A list submitted by the frontend, foreign JSON or an old metadata result cannot specify a hash run. Missing, additional, differently typed files, unknown repositories and registry problems must be clarified first. Only expected repositories are opened, no additional runtime files or savepaths. No Steam login, download or automatic "Verify integrity" is started.

SHA-1 is only for compatibility with the existing Steam format. SHA-256 is created in the same read run. The cache manifests are bound to the report via SHA-256. A suitable cache comparison does not confirm its origin: `all_files_match_cache` may become true, but `certified_vanilla` and `can_apply` always remain false. This report cannot be imported as a vanilla snapshot. The historical metadata report still retains `content_verified: false`; the two types of reports are separate.

Since v0.4.7, an exported content report with a matching registry copy can be saved as [observed baseline](BASELINE.md) in the project. This is expressly not an import as a certified vanilla base and does not grant write authorization. The original report remains unchanged.

## Start of the game, changes and paths

The real Windows process check is done before preparation, before each file, between reading blocks after at least 250 ms and before completion. Unclear process status prevents a successful run. A recognized game is never ended; Instead, the content check stops. Abort requests are checked between 1 MiB blocks. An already running file system operation must first return; There is no guaranteed termination time limit for a blocking data carrier.

Windows sources are opened with read access and read/write/delete sharing. Before the first content byte, the actual path of the opened handle is checked against the installation. Reparse points, directories and hardlinks are rejected. File size, change/create time, and file identifier are compared before and after reading and at the end of the entire run. Growth is limited to at most one additionally read byte and leads to termination. Registry, custodian lists and the entire file/size inventory are also checked again.

These controls detect normal updates and changes during the run. Without kept write/launch locks, the ** is not an atomic snapshot** and does not prove against targeted manipulation of content and timestamps. A later game start or an update can immediately make the finding obsolete. The live engine must re-check its sources under its own locks; See [Protected workspace rehearsals](PROTECTED_REHEARSALS.md).

## Evidence and open acceptance

Synthetic testing: complete SHA-1/SHA-256 comparison including known `abc` test vectors, multiple read blocks and empty files; the same amount of altered content; termination before/during the contract; game status error and game start; Changes to already read sources or Steam metadata; Growth, missing, additional and hard-linked sources. A kept audit read handle allows write operations, whose metadata change is detected afterwards.

Worker tests only check an active assignment, session limits, discontinuation, and panic treatment without a success report. UI tests check the launch lock during ongoing or unknown play, progress, change of view, cancellation, deviations and report export. The native desktop test does not automatically start the 154 GB run.

The user has run the full original run on 20.09.2026 from 13:23:25 to 13:25:45 clock (Europe/Vienna) via the desktop app and provided the report `exports/installation-audit-1789903405-11284-1-0.json`: 285 files, including 195 PAZ archives, together 154,097,618,899 bytes; all SHA 1 values are consistent with the local custodian lists. The report reports stable metadata. The following pure report evaluation confirms all individual comparisons, unique paths, total size and the 38 previously observed EXE/metadata SHA256 values. Detection: `.local/user-content-audit-review.json`; SHA-256 of the original report: `f32c50a354dbd81e85889cb764b4ecdf6d83958ed09c9b9eef70077cc48a64dc`.

Thus, the complete reading original run has taken place. An independently confirmed vanilla proof and in-game acceptance remain open. Live backup and base change are now implemented separately, but not yet executed on this installation. The reporting values `certified_vanilla` and `can_apply` remain false. The report describes the Zustand at the time of testing and was not updated by a new scan of the installation when it was evaluated.

References: [RustCrypto SHA-1 API and use for old formats](https://docs.rs/sha1/0.10.6/sha1/), [Windows CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [GetFinalPathNameByHandleW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew).

Since v0.4.9, the content checker uses the Workbench-conscious inventory. Only files associated with intact own history are removed from the list of originals to be checked; Foreign additives remain blocking. An active mod registry or an open base change prevents the start. The original file list remains backwards compatible for reports and stored outputs.

Since v0.4.10, exactly confirmed additional files may remain. The report continues to check all custodial originals and, where applicable, mentions `foreign_approval` as a reference to the separate confirmation. He does not certify their content. Changes in the confirmation status during the test discard the result. Before live writing accesses, the additions are freshly hashed and protected. [Details](FOREIGN_FILES.md).
