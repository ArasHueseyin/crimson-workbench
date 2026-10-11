# backup and recovery – v0.4.6

The mod workshop can reopen existing **protected workspace rehearsals**, check their backup and display the return in advance. This function works exclusively under `.local/rehearsals/<id>` in the project. It cannot restore a game installation and does not issue live write authorization.

## Operation

1. **Mod Workshop → backup & restore → workspace rehearsals load**.
2. Select a workspace rehearsal and select **Check backup and return**.
Immediately after a new sample **last workspace rehearsaltest** leads to the same test sequence. Loading the list alone does not start a restore.
3. The preview is called fuse size and SHA-256, tested sources, open
Transactions as well as the files to be replaced or removed with their before/after hashes. Empty own group folders are additionally mentioned.
4. **Recover project copy** completes an open transaction and
Then restore the original registry. The app then reads the actual Zustand again. If the sample is already completely reset, the button is locked.

Normal successful samples are already reset. The function helps with aborted samples or a later reopening; the user does not have to create an abort. Old v2 samples and damaged markers are displayed in the list with justification and are not selectable for this expiration. The existing CLI `mod-recover` is retained for old samples.

## Safeguarding and preview limits

- The list only reads markers. It is on 2,000 directory entries
limited and shows at most 200 entries descending by folder name.
- The selected v3 sample must display its hash-bound protective manifesto, its sources,
have the existing engine lock, the registry backup and the history. **The new preview and subsequent recovery never recreate missing backups or histories.** An incomplete initialization remains for diagnosis; A new sample gets its own folder.
- The test keeps the existing start/source/backup/folder locks at the
Project copy and the exclusive transaction lock. No content files, markers, backups or journals are created or modified. Windows can modify file system access metadata through read accesses.
- The return preview is to the physical project path, marker, backup hash,
next transaction number, open transaction, registry state and the specific files to be removed. Immediately before Restore, it is reconstructed and compared under renewed locks. Another project copy or outdated plan will be rejected.
- The plan identifier is a change comparison, not a certificate of origin and
No security secret. Existing Path Protection, Protective Manifesto and re-source/backup/history checks remain necessary.
- Missing active files, foreign content, damaged backups, hardlinks,
altered sources or registry and an already running protected test program prevent the recovery. There is no automatic repair or repetition after an error.

In the case of an open transaction, the already visible registry decides as in the previous core: Before-Zustand means rollback, After-Zustand means completion of the commit. Only proven own obsolete files are removed. After this, a separate restore transaction may be created, which first resets the registry to the backup and only then removes the active own overlay files. Original archives remain unaffected. backup, earlier journals and private staging data are preserved.

A result report `recovery-<id>.json` is generated within the same sample. If only this report export fails, the restore can already be completed; **check again** then shows the actual Zustand. It is never automatically rewritten.

## CLI and API

```powershell
cargo run -p cd-cli --locked -- mod-backups
cargo run -p cd-cli --locked -- mod-restore-preview .local/rehearsals/<id>
cargo run -p cd-cli --locked -- mod-restore .local/rehearsals/<id> --review-id <review_id>
```

The `review_id` comes from the preview checked immediately beforehand. `apply::recovery::{list, inspect, restore}` uses the same project path boundary as the previous samples. Desktop commands `rehearsal_list`, `rehearsal_review` and `rehearsal_restore` require a current catalog session and are only shared for the local main window. A change of session or view prevents the display of delayed UI responses. An already started transaction ends under its locks; Navigation is not an abort.

## Evidence and open work

Six new core tests check reading previews, missing backup, identical before/after hashes, open transactions before/after commit, obsolete or third-party previews, corrupted sources/backups/registry/overlays, hardlinks, protected paths and limited lists. At all 69 existing Apply/Reapply/Restore abort points, the preview hashes are additionally checked against the actually restored or removed files. Four UI flows check the explicit sequence, faulty backups, outdated plans, empty/old entries, navigation during a check and 1024px width.

The successful complete content comparison of the user installation remains a [Separate cache comparison](CONTENT_AUDIT.md). Independently confirmed vanilla origin, live backup/update integration, full protection of live start/load paths and in-game acceptance remain open. Phase 4 is not yet completed; Phase 5 was not started.
