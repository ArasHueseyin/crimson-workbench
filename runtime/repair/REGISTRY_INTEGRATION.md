# Registry adapter 0.7.0 and detected game build change

Status 22.09.2026, proof of development for module 0.7.0. Not an installable repair mod. Addendum: App v0.5.9 now supports the tables of the new build; The native boundaries described below remain in place. [Table release](../../docs/BUILD_SUPPORT.md). Further development: [common native registry/reference sample 0.8.0](REGISTRY_NATIVE_INTEGRATION.md). The following results describe the previous status 0.7.0.

## Implemented adapter

`crimson_repair_registry.lib` connects a host-provided manager and its native lookup/PaPtr sharing method to `reference::Pair`. The lookup receives the full character identifier and the 32 byte output document. The adapter does not use a pre-read actor address as a source of purchase.

Each source retains its own method binding. For this, the release callback now receives the same context as the Acquire callback. There is no global sharing feature that could replace a second host/manager. Invalid manager/method bindings and 0 identifier do not provide a usable source. The source adapter is not copyable or displaceable and must survive its acquired references.

The native lookup is responsible for the registry lock and the acquisition before the release of this lock. These calls can wait; they belong exclusively to the engine dispatch thread still to be used. This is a different requirement than the non-waiting inventory lockdown attempt from 0.5.0.

## Current audit

Five CTest suites, MSVC Release `/W4 /WX`, 55 Registry Adapter Conditions, 249 Reader/Reference Conditions, 25 Lock Group Conditions and 1,481 Action Conditions passed. The new tests explicitly use artificial registry callbacks. Complete identifier, errors of both sources, same actor address in both roles, missing methods/managers, no call for invalid bindings, exactly one release per result and reverse order are checked. Two different release functions occupy the binding per source.

## Significant new frontier: game update

During this work step, the installed EXE was recognized as a new version:

|Characteristic|So far proven|Now installed|
|---|---|---|
|Steam-Build|25381195|25455892|
|EXE-Version|1.0.0.2944|1.0.0.2949|
|EXE-Bytes|397,242,776|385,363,864|
|SHA-256|`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`|`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`|

24 of the 38 previously recorded EXE/metadata files differ from the old observation status. The new [Observation report](../../docs/builds/steam-25455892.observed.json) was not a schema release at first. The later table release in v0.5.9 is documented separately; Still no vanilla certification, native clearance or full installation inventory. The original build is preserved.

The previous native test host rejects the new EXE before function execution. At the development level v0.5.8, the read CLI build diagnosis reported `metadata_matches=false` and `read_schema_supported=false`. The additional CLI table call fails beforehand due to a lack of project sample backup; This call therefore does not count as proof of the matching build error message. The fingerprint diagnosis provides the independent version proof.

The 203 native calls from 0.6.0 remain a historical proof for 1.0.0.2944. For 0.7.0, **is not claimed a new native proof of success**. The initially found client registry functions are research findings, not yet a released method binding for the new build. Addresses, wind limits, dependencies and tables must be re-checked together; Just replacing the EXE hash would be wrong.

## Still missing

- Full support and isolated native proof of the new build.
- Current player/manager, secured manager lifetime and occupied
Engine thread/TLS binding. The adapter does not solve these host requirements itself.
- Combined proof of real client/server registry lookups and
Acquisition of reference and subsequent release.
- Complete repair transaction, event allocation, input, loader and B0 installation.

No game files or saves written; No gaming process access. Manual tests are further deferred. The user-readable acceptance list is in [TESTCHECKLISTE.md](../../TESTCHECKLISTE.md).

Evidence: `.local/repair-runtime-v7-build-test.log`, `.local/repair-runtime-v7-build-refusals.json`, `.local/repair-runtime-v7-current-fingerprint.json`, `.local/repair-runtime-v7-validation.json`.
