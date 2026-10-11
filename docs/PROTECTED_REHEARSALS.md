# Protected workspace rehearsals – v0.4.3

This protective layer is used by the desktop sample and `mod-recover` for new v3 samples. These samples reach only marked project folders under `.local/rehearsals/<id>`. Since v0.4.8 the separate [Live-Adapter](LIVE_APPLY.md) uses the same protective layer with explicit permission.

Since v0.4.6, a [Safeguard/restoration view](BACKUP_RECOVERY.md) supplements the process: check existing backup and history, display cancellation and execute exactly this preview under renewed locks. Missing backups are never recreated in this new process.

## Sequence and locks held

1. The sample copies our own executable file
`probe/workbench-guard-probe.exe` and generates `probe/source.bin` as an explicitly synthetic update test file. No game EXE and no original PAZ will be copied.
2. `protection.json` contains sizes and complete SHA 256 hashes of the captured
Sources as well as the hash of the original registry. The marker `rehearsal.json` binds this manifest by its hash. The only accepted origin type is `project-copy-only`; No vanilla release can occur from this.
3. Before creating the engine lock or backup, the session checks the known
Executable paths against the Windows process list. Candidates are matched via their actual, canonized image path; Junction/pathal aliases are dissolved. An unreliably questionable candidate blocks the session.
4. The session holds EXE handles without sharing. For unchanged source files,
Only read-sharing is allowed. Root, `meta` and all source parent folders are held with read access without delete sharing. Reparse points, hardlinks and incorrect file types are checked before and after opening via the actual handle.
5. Complete source hashes are checked from the handles held. After the
Acquisition of all blocks is again a process check. Only then does the private transaction core open its history and create/verify the backup.
6. Also `.workbench` and `baseline.papgt` are held; The backup hash is
re-examined. Planning, Apply, Reapply, Restore and Recovery keep the locks until the end of the session. Before the write limits, process status and handle properties are checked again. An error does not prematurely release the locks; The entire session must be closed.

The process queries do not read any process memory and do not terminate any program. In the actual desktop sample, the copied EXE is not started. Only automated tests invisibly start their own test EXEs and then close exactly their own child processes.

## Detected error cases

The new test suite checks actual Windows behavior with your own files:

- Parallel start during Apply/Reapply/Restore locked, possible after release.
- Already running copy recognized before state/backuper creation.
- Source/Backupwrites and renaming of protected parent folders blocked.
- errors in process checks before/after acquiring handle: no backup system,
All purchased handles are released again.
- Protection remains active with injected transaction cancellation and rejected recovery.
- Update before recovery and after the Apply is complete: new source files and
Registry as well as old backup remain. No restoration of a new build.
- Manipulated protective manifest, downgrade of a v3 marker to v2, damaged
Backup, hardlinks, path breakout and claimed vanilla origin are rejected.

Two original assumptions have been refuted and corrected by these tests: An exclusive read handle alone does not recognize already running images, and a pure attribute access does not hold an effective renaming lock. In addition, both sides of the process path comparison had to be canonized. The implementation therefore uses process matching ** and ** held read handles. Primary documentation: [CreateFileW and Sharing](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [QueryFullProcessImageNameW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew).

## Update sample and recovery

After the two Apply/Reapply/Restore cycles and the recovery test, only the synthetic source file is changed. Recovery must reject this deviation without overwriting the registry. Subsequently, the sample is returned to its original synthetic bytes and recovery is checked again. If discontinued during this intentional update attempt, the sample may be left as changed; it remains for diagnosis and can be replaced by a new sample. No unknown Zustand is automatically accepted as a new base.

Existing v2 samples remain recoverable via the previous kernel and are not tacitly upgraded. New v3 samples must always have their appropriate protective manifesto and sources. A renamed or changed source path will not be replaced by another file hash.

## Limits before a live release

The Windows protection layer is now integrated into the **project transaction**. Still open are independently confirmed full game base hashes, a derived complete directory of all relevant original archives and launch paths, the live backup/update workflow and in-game acceptance. Uncaptured executable copies outside the known starting paths and a hostilely altered computer are not a proven scope of protection.

New protected samples require Windows and do not have an unsecured replacement path on other platforms. The previous v2 samples remain separately readable/recoverable. Tested breakpoints are still not a complete power failure proof. Spiel and Saves were neither blocked nor altered by this work.
