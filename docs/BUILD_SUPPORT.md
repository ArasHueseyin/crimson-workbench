# Support for game builds

Status: 22.09.2026, Workbench v0.5.9.

|Steam-Build|EXE-Version|reading schema|Stages|Quests|
|---|---|---|---:|---:|
|25381195|1.0.0.2944|`steam-25381195-gamedata-2.3-v2`|52,080|1,097|
|25455892|1.0.0.2949|`steam-25455892-gamedata-2.3-v2`|52,082|1,098|

Support includes data reading, mod planning and samples on project copies. It is not a vanilla approval of the installation, no proof of the effect in the game and no release of the native repair functions for the new build.

## Proof of the update

The new EXE has SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. 24 of 38 captured EXE/metadata files are different from the previous build. This number describes the recorded files, not a complete content check of all game archives.

61 table, icon and language files used by the Workbench were read from the archives and compared with the previous complete hashes: 57 are byte-identical. Only the body and header of `questinfo` and `stageinfo` change. All 14 indexed table pairs can be reconstructed byte-identical. The item localizations of all 15 languages as well as the additionally used crafting and advanced tables remain identical.

- Stages: new IDs `1001269` and `1003266`; No previous data set changed
or removed. The two editable patrols `1002224` and `1017811` remain completely byte-identical to the old build.
- Quests: new ID `1000320`, changed ID `1000107`, no removed ID.
The Workbench does not modify these quest data.
- The mod integration test checks each stage against its original bytes. Only those
Both selected patrols may be changed. Both new stages must be preserved; Quest files must not appear in the overlay.

Reproducible comparative reading sample:

```powershell
cargo run --locked -j 2 -p cd-core --example audit_build_transition -- "C:\Program Files (x86)\Steam\steamapps\common\Crimson Desert"
```

The dataset comparison requires the hash-checked old stage/quest files in `.local/archive-probe/extracted`. They are not distributed with the project. The local report is below `.local/build-25455892-table-comparison.json`. The sample does not write game files and does not give build release itself.

## Selection and rejection

The EXE version selects a known set of reference hashes. After that, all captured EXE/metadata files and the tables used must fit exactly this set. A suitable version string alone is not enough. tables are bound to the already verified identity; a later re-reading of the EXE version may not select another sentence.

Search index and JSON outputs carry the actual schema ID used. Backup reports require build ID, file sizes, and hashes of a full known build. Mixed metadata of both builds, missing files and incorrect sizes are rejected. An old report is not declared the new B0 base. Further unknown EXE versions stop before opening a mod backup so that a game update is not reported as missing workspace rehearsal.

The historic native repair host remains tied to EXE 1.0.0.2944 and refuses the new EXE. A separate registry/reference host is now tested for 1.0.0.2949: 30 scenarios and 263 native calls to private data. Further inventory/modification/event methods and the actual game connection remain open. [Separate native detection](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

Further evidence: [TESTING](TESTING.md). Later manual removals: [TESTCHECKLISTE](../TESTCHECKLISTE.md).
