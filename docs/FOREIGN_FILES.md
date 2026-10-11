# Foreign mods and additional files – v0.4.10

Workbench separates original files, proven own files and third-party additions. Unknown files are not automatically harmful or mods: Logs, configurations and remains of other managers can also be additional files. Without explicit confirmation, they block Live-B0.

## Assisted handling

**Mod Workshop → Foreign Mods & Additional Files → Check Additional Files** creates an exact list of paths, sizes and SHA-256 values on request. Only additives are hashed; Original archives are not additionally read in full. The checking and confirmation does not change any game file and does not take any start/source locks. They are also possible during play. Meanwhile, the separate live writing processes remain blocked.

After reading the list, set the checkbox to keep and select **Confirm checked additional files**. Before saving, the list is rebuilt and hashed; Outdated previews are rejected without automatic retry. This fileset is confirmed, not any future version and not an entire folder with unchecked later content.

The confirmation is stored exclusively in the project. It is not a vanilla proof and no commitment to the technical compatibility of foreign mods. It does not start a content check or Apply. Then reload file list and live preview. **Remove Confirmation** revokes it without deleting or modifying the additional files. Live operations then require a permitted stock again; if additions are still present, reconfirm if necessary. An empty stock does not need confirmation.

## Confirmation limits

- Confirmable are additional ordinary files and folders outside the
Depot originals, registry/metadata domains and private Workbench paths. This can include unregistered PAZ groups, loader files and configurations.
- A foreign modified registry or registered foreign groups will not
taken over. First, take back the foreign mod with its manager and complete Steam file check after playing. Remaining unregistered additives can then be confirmed separately. Workbench does not merge any foreign table changes with its own modules.
- Missing, deviating or modified originals remain blocked. One
Additional confirmation cannot bypass original hashes, vanilla origin confirmation, known reading schemes, backup, process protection or current file previews. Same file size alone does not prove unchanged original content.
- Symlinks/Junctions, hardlinks, savepaths and additionally configured protected
Subfolders shall not be allowed to be maintained. Save contents are not read. Boundaries: 512 files, 1,024 additional folders, together 512 MiB and 128 log entries. The last entry is reserved for revocation.

## Integration into B0 and reports

Inventory and content verification identify confirmed external files separately. Original repositories can never disappear from their comparison by this assignment, even if Steam later adopts an additional path earlier. The inventory checks only metadata when mapping; an equally large subsequent change of content is rejected at the latest by the protected live check. A report with `foreign_approval` checks its listed custodial originals and refers to the separate confirmation; it does not certify the foreign files. Old reports without the optional field remain readable.

Setup, Apply/Reapply, Restore and Base Change record the confirmed files in their fresh SHA-256 comparison and the held source locks. Additional EXEs belong to the launch protection, even with capitalized file extension. Empty files are also checked with their actual hash. Third-party groups are omitted when assigning their own group IDs and remain unregistered if they were previously unregistered. Restore only removes its own changes.

Live previews also bind the current confirmation. Amendment or revocation shall invalidate them; a revocation triggered in parallel via the CLI is recognized at the protective limits of the ongoing process. The desktop app refuses to make changes to confirmation during its own live job. There is no automatic writing attempt for errors. If the base change is interrupted, the already confirmed third-party files must continue to fit exactly; their reconfirmation during an incomplete own history is not offered.

Immutable data sets are below `.local/foreign/<Installationskennung>/<laufende-Nummer>.json`. The most recent data set determines the current confirmation; An empty sentence is a revocation. Damaged or incomplete protocols do not allow takeover.

## CLI and later manual acceptance

```powershell
cargo run --release -p cd-cli --locked -- foreign-preview
cargo run --release -p cd-cli --locked -- foreign-confirm --review-id <review.review_id> --preserve
cargo run --release -p cd-cli --locked -- foreign-revoke --approval-id <approval_id>
```

After completion of all development phases, check manually: list and confirm known additional files, apply your own mod, find foreign files unchanged, withdraw your own changes and revoke the confirmation. Third-party registries, modified originals and modified additional content must continue to block the corresponding processes. The game effect of foreign mods is not proven by the automated tests. [Entire live process](LIVE_APPLY.md).
