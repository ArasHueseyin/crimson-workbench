# Installation Test – Metadata Inventory

Status: 20.09.2026. The Mod Workshop offers **Check file list**. The check reads directory metadata, the local Steam app/depot list and `meta/0.papgt`. It does not open PAZ, EXE, DLL or save content, does not set launch locks, does not start Steam verification and does not write anything in the game directory. The CLI also works for an unknown data build:

```powershell
cargo run -p cd-cli --locked -- installation-check --output .local/installation.json
```

Without `--output`, JSON appears on stdout. Output files are subject to the existing project path control. There is still no live writing command. In the desktop app, the report is tied to the open catalog session; For unsupported catalog builds, the CLI can diagnose separately.

## Statement of the report

- `directory_scan_complete`: directory tree listed without read/budget errors.
Symbolic links/reparse points are reported as unchecked and not tracked.
- `depot_comparison_available`: all installed depot lists found and
structurally tested; IDs and total sizes match the Appmanifest. A missing depot does not lead to a supposedly complete partial comparison.
- `size_matches`: Name, type and size match the local depot list.
**No content comparison.** Equally large changes remain unrecognized.
- `missing`, `size_mismatch`, `type_mismatch`, `additional`, `unsupported_entry`
Differentiations. Additional files may be legitimate runtime files; From this alone, no extraneous mod findings are derived.
- registry groups shall be cross-checked against the directory/depository stock;
Uninstalled optional language groups are not an error. Unregistered archive groups and missing required groups are reported.
- EXE list combines expected and found EXE paths within the installation,
Also missing or additional EXEs. An EXE ending proves neither its role nor all possible external starting and charging paths.

`content_verified`, `certified_vanilla`, `can_apply` and the depot property `authenticated` remain **always false**. The report is not importable as a B0 snapshot and cannot grant write permission. The registry comparison with the observed build is additionally diagnostic, not a vanilla certificate.

## Findings from this installation

Steam-Build **25381195**, testing during the current game:

|Finding|Results|
|---|---|
|Expected / found files|285 / 285, all names and sizes suitable|
|Total expected file size|154,097,618,899 Bytes (only metadata summed)|
|Depot 3321461|Manifest 1190168329432671189, 281 Files and a directory entry|
|Depot 3321466|Manifest 4235648687492059552, 4 files|
|EXE-Funde|`bin64/CrimsonDesert.exe`, `bin64/crashpad_handler.exe`, `bin64/pers.exe`|
|Registry|SHA-256 corresponds to the previously observed build|
|Cache-Signaturen|both empty; No authenticated guarantee of origin|
|Deviations|None of these metadata checks|

Detection: `.local/phase4-v5-installation.json`. This local report contains file names, sizes and expected SHA 1 values from the cache, no game content. It shall not be placed in the repository.

## Parsers and borders

own Rust reader for the publicly documented binary custodian format: payload, metadata, signature section and end marker; limited protobuf reading without unpacking chunk data or downloading file content. Payload CRC32 including length prefix, identity, file hash lengths, total size, path/type conflicts and duplicate fields are checked. An existing signature blob would also be automatically authenticated **not**.

Limits: 16 MiB per depot manifest, maximum 32 depots, 16,384 combined directory entries, depth 32; Appmanifest and Registry each 1 MiB. Encrypted file names and depotlinks are not supported. Conflicting cache copies, duplicate VDF keys, pending target buildID or changes to the Appmanifest during reading prevent the comparison. Non-Steam installations will still receive a directory inventory with an explicit comparison gap.

Without launch locks, this is not an atomic snapshot. Simultaneous changes or path changes are possible. Again, the report is never used as a source for live apply, restore or recovery. The later live session must check all relevant sources from held handles; See [Protected workspace rehearsals](PROTECTED_REHEARSALS.md).

Since v0.4.5, a separate [Content verification](CONTENT_AUDIT.md) supplements this metadata report. It is blocked in the case of ongoing or unknown game status and does not confirm a vanilla even with suitable content.

## Testing and next B0 step

Synthetic tests check each truncated prefix of a valid repository list, corrupted CRCs, traversal/device paths, duplicates, overflowing vaints, false IDs/sizes, missing and conflicting caches, update metadata, corrupted registry, type changes, additional EXEs and archive groups, size changes, same size content changes, and directory depth and Windows junctions. Even with fully matching metadata, Live-Apply remains blocked.

Since v0.4.9, the inventory assigns files from an intact own live approval, transaction history and completed base changes separately. It lists them as Workbench files instead of unknown additions; Original repositories always remain in the original comparison. Your own active registry is explicitly displayed and prevents a new vanilla content report. Missing or damaged proof of ownership, open change and foreign children remain errors. The assignment has no archive content and does not certify a vanilla.

The user has completed the full original run; Live approval, source protection, backup and base change are implemented separately. On the real installation, these writing sequences were not executed. The manual in-game tests remain shifted on user request. [Live process](LIVE_APPLY.md).

Format references, no code transfer: [SteamKit DepotManifest](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs), [Steam-Protobuf definition](https://github.com/SteamDatabase/Protobufs/blob/master/steam/content_manifest.proto).

Since v0.4.10, the inventory also expressly confirms foreign additives separately. These remain foreign; They are not Workbench files and not vanilla originals. Unconfirmed, new or size different entries remain in the deviation report. [Confirmation and limits](FOREIGN_FILES.md).
