# Support for game builds

Status: 26.10.2026, Workbench v0.10.1.

|Steam-Build|EXE-Version|reading schema|Stages|Quests|
|---|---|---|---:|---:|
|25381195|1.0.0.2944|`steam-25381195-gamedata-2.3-v2`|52,080|1,097|
|25455892|1.0.0.2949|`steam-25455892-gamedata-2.3-v2`|52,082|1,098|
|25477059|1.0.0.2976|`steam-25477059-gamedata-2.3-v2`|Not recorded here|Not recorded here|

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

The EXE version selects a known set of reference hashes. Required EXE/metadata files and the tables used must fit exactly this set. A suitable version string alone is not enough. Tables are bound to the already verified identity; a later re-reading of the EXE version may not select another build.

The original registries for all three supported builds mark groups `0036`, `0037`, `0038`, `0039` and `0040` as optional. Only `0037/0.pamt`, assigned to German, was included in the observed reference files and incorrectly required by earlier releases. The other four groups were already allowed to be absent. This was checked against private original registry snapshots whose SHA-256 matches each checked-in build reference.

From v0.10.1, the known-build observations record the optional status of `0037/0.pamt`: an installation with the entire group absent is accepted by the metadata and installation-baseline checks. Other language configurations can contain a different optional group, such as `0039`. A present `0037/0.pamt` still needs its exact known size and SHA-256; a partially installed group or an IO error is rejected. The EXE, original registry, required metadata, core tables and selected item localization retain their existing validation. This does not certify that an installation is vanilla or bypass the separate Steam content audit.

Search index and JSON outputs carry the actual schema ID used. Backup reports require the build ID, file sizes and hashes of the required known-build files and any installed optional reference files. Mixed build metadata, missing required files and incorrect sizes are rejected. An old report is not declared the new B0 base. Further unknown EXE versions stop before opening a mod backup so that a game update is not reported as missing workspace rehearsal.

The historic native repair host remains tied to EXE 1.0.0.2944 and refuses the new EXE. A separate registry/reference host is now tested for 1.0.0.2949: 30 scenarios and 263 native calls to private data. Further inventory/modification/event methods and the actual game connection remain open. [Separate native detection](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

Further evidence: [TESTING](TESTING.md). Later manual removals: [TESTCHECKLISTE](../TESTCHECKLISTE.md).
