# Test plan and evidence

Status: phase 5, v0.5.9, 23.09.2026. Current status in [PROGRESS.md](PROGRESS.md), implementation limits in [MODS.md](MODS.md). Live-B0 is automatically tested on artificial installations. Basic change, inventory integration and confirmed third-party additions are implemented. The manual in-game validation remains **open**. At the request of the user from 20.09.2026, the manual in-game tests is carried out only after completion of the development of all phases. They do not block further development and are until then explicitly listed as untested.

## Inventory package serializer, module 0.23.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 36 additional cases: 26 direct samples of the original transmitter/stream selection and ten connections with repair transaction and client ax. Exact field widths and byte sequence, zero/maximum values, incorrect actor, UID change, missing receiver/buffer, discarded bytes and missing/double delivery checked. The private confirmation discards all unclear results and prevents replay.

Overall: 454 native scenarios, 4,812 counted calls, 206,786 conditions including repeated byte checks. 102 Code ranges, 127 pdata fragments, 60 private TLS locations plus unchanged stack sample on your own Windows thread. CTest conditions: 11/1,529/519/25/55/612/29/202/273. Production suites unchanged. Stream buffer, transport and server/storage termination remain artificial; The successful pool branch is not executed. [Details](../runtime/repair/INVENTORY_PACKET_2949.md).

## Separate offline inventory order

Slot 2 specifically changed when the game is closed: 200 lightweight and 200 heavy copper bags, in addition 30,000 silver. originals secured; Full semantic comparison, valid new IDs/slots/pointers, byte-accurate return checked for copies and integrity of the actually replaced files after reopening. Only `save.save`/`lobby.save` of this slot changed, eight more files unchanged. Display, use and re-save in-game remain open. [Report](../.local/inventory-currency/RESULTAT.md). This single order is not a general save writer and is not proof of repair memory termination.

## Native buffer preparation and compression, module 0.22.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 66 new cases on own data: 40 size/compression cases, four output reuse cases, six errors, 14 connections to the original file helper and two repetitions after opening errors. An independent LZ4 test decoder reconstructs the original bytes; the output contains the exact header and both size fields. All buffers released.

Overall: 418 native scenarios, 4,289 calls, 204,378 conditions, including many repeated byte checks. 100 code ranges, 125 pdata fragments and 58 private TLS replacements. The original stack sample reads the stack limit of your own thread. CTest conditions: 11/1,529/519/25/55/600/29/202/273. Production suites unchanged.

Repeating an entry after opening error recompresses already compressed data. Downstream processing and all file operations remain callbacks; no proof of integrity/encryption or data medium. Allocation errors, complete object serialization and game connection are missing. [Detection and limits](../runtime/repair/SAVE_ENCODING_2949.md), `.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.

## File Writer with Private File Receivers, Module 0.21.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 26 Direct cases and eight dispatcher integrations check buffer ownership and errors up to and including flush/close. All file operations are private callbacks with a synthetic handle. Coding, long path name and non-empty string destruction are not executed.

Overall: 352 native scenarios, 4,095 calls, 142,649 conditions. 91 code ranges, 118 pdata fragments and 55 private TLS locations. The CTest suites report 11/1,529/519/25/55/546/29/202/273 conditions. Production suites unchanged. The successful native expiration can release the earmarked payload and delete dirty despite a failed flush. This is expressly not a storage document.

Real encoder/writefile wrapper read only statically. Preparation may change the original payload; the preservation in case of errors applies so far only with the non-modifying test encoder. No access to game process or saves; No change to the installation. The installed EXE was only read. [Detection and limits](../runtime/repair/SAVE_FILE_2949.md), `.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.

## Save dispatchers and queues, module 0.20.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 39 new native cases: 16 gate/status combinations, eight error cases, eight Steam quota cases, one missing backend and six queue cases with growth and duplicates. File, platform and diagnostic recipients remain private test callbacks.

Overall: 318 native scenarios, 4,039 calls, 141,411 conditions. 88 code ranges, 115 pdata fragments and 51 private TLS locations. The nine CTest suites report 11/1,529/519/25/55/528/29/202/273 conditions. Writer and transaction unchanged.

Additional static proof: eight file/constructor/timer functions, five VTables, 13 instruction anchors and the two file imports. The original writing assistant ignores the flush result; It has not yet been executed. No save files, Steam API or running game objects used. No data carrier termination and no installable repair mod demonstrated. [Detection and limits](../runtime/repair/SAVE_BACKEND_2949.md), `.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.

## Item/storage object conversion, module 0.19.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 38 new native cases run original converters and their lifetime functions on private objects; 14 Cases use productive repair plans and native post-copies. Main values including 0/No-Wear/Sentinel, base geometries, dye data, reset/reuse and full release checked in both allocation modes.

Overall: 279 native scenarios, 3,950 calls, 139,784 conditions. 82 code ranges, 108 pdata fragments, 50 private TLS locations. The nine CTest suites report 11/1,529/519/25/55/492/29/202/273 conditions; the 492 manifest conditions do not belong to an additional game test. Writer and transaction unchanged.

No regular serialization on disks, no actual save completion and no complete engine commit. Catalog, watch, reflection notifications, socket guidelines and heap are controlled test dependencies. Other optional item data remains open. Unchanged Rust/UI parts not retested. [Detection and precise limits](../runtime/repair/ITEM_SAVE_2949.md), `.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`.

## Complete SQL job path, module 0.18.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 19 new native cases: eight execution branches/error cases, three request purchase cases and eight combined slot consumer cases in both private TLS allocation modes. Status/deadline fields, temporary diagnostic pointer, old errors received, zero pointer and unwind checked.

Overall: 241 native scenarios, 3,674 calls and 128,415 conditions. 58 code ranges, 76 pdata fragments and 35 private TLS locations. Production suites unchanged: 43 Transaction scenarios/273 Conditions and 35 Writing scenarios/202 Conditions.

Execution can deliver success without request access and does not contain a database call. The native test result expressly shows `sql_execute_is_persistence_proof=false`. No real storage yet, no full engine commit and no installable repair mod. Clock, diagnosis, capacity management and parts of event recipients remain test dependencies. Unchanged Rust/UI parts are not retested. [Evidence](../runtime/repair/SQL_DISPATCH_2949.md), `.local/repair-runtime-v18-{build-test.log,native-result.json,validation.json}`.

Additional static proof: seven function hashes, four RTTI types, jump/VTable mapping and 15 instruction anchors for alternative transaction and item/memory object conversions. No other native success counted. The conversions contain main and base durability, but are not yet executed with own data; the actual save process has not been verified. [Evidence](../runtime/repair/ITEM_SAVE_2949.md), `.local/repair-runtime-v18-save-route.json`.

## Inventory client feedback, module 0.17.0

Nine CTest suites and MSVC release `/W4 /WX` passed. 36 additional native cases run the original inventory ax: 14 direct cases in both TLS modes and eight cases in the common adapter with native copies/field recorders. Absolute main values, quantity/key/slot/container error, unchanged base and correct lock/reference sequence checked. UID change, missing/double delivery and completion errors do not result in repair success or replay.

Overall: 222 native scenarios, 3,526 calls, 125,524 conditions. 56 code ranges, 70 pdata fragments and 35 private TLS locations. Existing production suites: 43 Transaction scenarios/273 Conditions, 35 Writing scenarios/202 Conditions.

Container translation, blank name formatting and UI are test receivers; the non-empty string paths are not executed. Inventory servers and persistence are also artificial. The integration sample reports after the previous field step delta 0; the actual UI reaction has yet to be verified. No game repair claimed. The only statically tested socket-SQL allocation does not occupy any durability storage path. Unchanged Rust/UI parts not re-executed. [Evidence](../runtime/repair/INVENTORY_EVENTS_2949.md), `.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`.

## Slot processing and batch completion, module 0.16.0

Nine CTest suites and MSVC release `/W4 /WX` passed. Expiration suite: 43 Scenarios, 273 Conditions; Writer unchanged 35 Scenarios/202 Conditions. Missing terminating adapter prevents copying and writing. Closing errors or change of sessions after messages do not confirm success. Call order, blocks released and references/copies held until the end are checked.

24 new native consumer cases include both allocation paths, empty lists, duplicates, missing/invalid items, main values 0 and 65,535, and 64 slots. UID/durability pairs, native table cleanup, reuse and release are checked. Capacitance errors, positive execution errors and stored negative errors occur after emptying; Running again does not restore the markings. The persistence interface is its own test receiver.

18 Equipment cases use the common adapter and slot consumer. Three additional error cases lock the order as unclear, although the client corner and field change were successful. Total: 186 native scenarios, 2,826 counted calls, 124,527 conditions; Many conditions repeatedly check the tables. 55 Code ranges, 69 pdata fragments and 32 private TLS locations are pinned.

Actual storage is not occupied; In particular, the supplementary route for base data sets is missing. Inventory events and game host remain open. No access to the game process or saves, no installation change. App and original test report as well as 38 EXE/metadata files remain hash-same. Unchanged Rust/UI parts were not retested. The later in-game validation stands with concrete expectations in the [Testcheckliste](../TESTCHECKLISTE.md). [Evidence](../runtime/repair/PERSISTENCE_2949.md), `.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`.

## Joint process and prepared post-copies, module 0.15.0

Nine CTest suites and MSVC release `/W4 /WX` passed. New flow suite: 40 Scenarios, 247 Conditions. Field recorder: 35 Scenarios, 202 Conditions. Action/reader/lock/registry/item owner: 1,529/519/25/55/29 Conditions. All copies checked before the first store, total notification approval, incorrect thread, inaccessible additional data, allocation budget, copy/write/message cancellations, session change and independent confirmation. Errors from the first store attempt are never considered unchanged rejection or success.

The 15 native equipment cases use the production adapter instead of a manual field loop. Original item copies, field recorder, slot marking, unlocking and server/client message path connected. New capture after the process intentionally refuses deviant slots and Zustandcopies. 32 native item cases also check empty/partially stored sockets and copy preparation. Overall: 159 native scenarios, 2,023 counted calls, 119,953 conditions. 51 Code ranges/57 pdata fragments/26 TLS locations pinned unchanged.

Corrections: Read only stored sockets; Logical places may be above the allocation capacity. The additional vectors at `+0x78`/`+0xA8` have 16/6 byte data sets. One test checks the previously overlooked posterior overlap. Previous counters contain comparisons of unstored sockets and the now replaced manual event setup; They are not directly comparable as case numbers.

No complete live repair: concrete inventory events, slot consumers, real receivers and host remain open. Unchanged Rust/UI parts not retested. [Details](../runtime/repair/TRANSACTION_2949.md), `.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`.

## Full durability field step, modulus 0.14.0

Eight CTest suites and MSVC release `/W4 /WX` passed. New writing suite: 34 Scenarios, 197 Conditions. Action/reader/lock/registry/item conditions remain 1,537/519/25/55/30. All ranges and both Zustandcopies, No-Wear, Sentinel, empty orders and only changed words are checked.

Cross-checks: capacitance error, common/overlapping item/socket/additional buffers, protected structures, late field writability denied, outdated batch and session change. Changed values during the pre-check, byte-identical misplaced arrays and empty slots prevent all stores. Partial errors, incorrect write feedbacks and changes after writing is started never result in success or automatic repetition. Own Windows pages check alignment, overflow and page protection. 2,048 Items/4,096 Stores within the implementation budget exist including complete follow-up control.

Eight additional native cases connect the new recorder with original reference/lock functions and actual private memory addresses, with direct and registered reference management. All source bytes agree with an independent follow-up comparison. Overall: 141 native scenarios, 1,408 calls and 120,069 conditions. The original function manifest remains at 51 code ranges, 57 pdata fragments, 26 private TLS reading points and 306 manifest conditions.

`fields_written` is expressly not a confirmed engine commit. The complete connection with pre-created native copies and the events is still missing; the previous 15 incidents retain their separate private field setup. Game host, slot consumer, inventory events and real effect/transport processing remain open. Unchanged app/Rust/UI parts not retested. [Details](../runtime/repair/FIELD_WRITER_2949.md), `.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`.

## Capture and native slot marking, module 0.13.0

Seven CTest suites and MSVC release `/W4 /WX` passed. Readers now 519 Conditions; Action/lock/registry/item owner unchanged 1,537/25/55/30. `HeldCapture` keeps both owners and locks for planning and re-examination; Separate unlocking allows for references for notification. Reacquisition, incorrect thread, session/source changes, outdated batches and error/exception release checked. A competing thread cannot purchase either of the two actual Windows locks after `open`.

16 new native cases check slot marking with duplicates, collisions, shifted bucket chains and growth for 64 entries, in both TLS allocation modes. The 15 incidents use the original marking under the locks in front of the server notifier. Total 133 native scenarios, 1,296 calls and 119,997 conditions; Many of them check the same table after each insertion. Manifest: 51 code ranges, 57 pdata fragments, 26 private TLS readers; 306 Manifest Conditions without EXE. All private allocations with protective markings tested and balanced; the additional insert-unwind alias is byte-identical.

The slot consumer and the real list cleanup are open. Testheap, effect/transport/UI receivers remain their own dependencies. No statement about full native writer, game host or successful repair in the game. No changed app/UI code; Therefore, no new Rust/UI check is claimed. [Details](../runtime/repair/DIRTY_SLOTS_2949.md), `.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`.

## Native Item Copy and Event Integration, Module 0.12.0

Seven CTest suites, MSVC release `/W4 /WX`, 30 new conditions for the native item owner. Fixed constructor/allocation/destructor binding, thread bound, header check before target change, overlap, double release and unusable native returns are tested. Action/reader/lock/source suites remain at 1,537/385/25/55 conditions.

117 native scenarios with 851 counted calls and 13,639 conditions passed. 14 new cases run the original item methods together with nested copy helpers. Sockets, both additional lists and optional 24/16 byte blocks are copied independently; Growth, replacement, self-assignment and invalid source capacity tested. Both TLS allocation paths use their own Windows heap with protection markings and fully balanced releases. The source is preserved.

The 15 incidents now use the original native aftercopies under the existing Actor references: copy under the owner locks, notification afterwards, destruction upon return. The manifest includes 45 code ranges, 49 pdata fragments and 24 private TLS redirects. A byteidentical, pinned Unwind alias is normalized for the chain. Without EXE only 270 manifest conditions.

No proof of real engine allocation, complete slot/inventory change, effect processing, message transmission or game host binding. Exceptions and allocation errors of foreign functions remain outside the sample. App/UI code unchanged, no new Rust/UI test execution claimed. [Details](../runtime/repair/ITEM_LIFECYCLE_2949.md), `.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`.

## Event preparation and native equipment events, module 0.11.0

Six CTest suites and MSVC release `/W4 /WX` passed. 1,537 Action Conditions, 385 Reader Conditions, 25 Lock Group Conditions and 55 Source Adapter Conditions. The own event preparation takes into account active maxima, no-wear, signed durability, pure base repair and still broken post-conditions.

103 native scenarios with 758 counted calls and 9,311 conditions passed. 15 new cases combine original server notification, event initialization, client ax and socket collector with private effect/transport/UI receivers. Tested are Repaired-Zustandsart 3 and their arguments, optional child events, no-wear, base maintenance and rejection of false/missing/double feedback, missing slots, partial changes, missing receipts and changed session.

The private repair keeps native Actor references over the entire sequence of events; Owner locks are released beforehand. Success requires actual fixture callbacks and fresh matching after values. A return value 0 alone is not enough. 28 code ranges, 25 pdata fragments and eight private TLS read redirects; without EXE, the manifest check only performs 84 conditions and no original functions.

The real effect processor, message transport, UI and persistence are not tested. Engine writing/item copy, slot invalidation, inventory events, manager/thread/world binding and installation remain open. App/front end code unchanged; No new Rust/UI tests claimed. Game process and saves not used. [Details and limits](../runtime/repair/EQUIPMENT_EVENTS_2949.md). Evidence: `.local/repair-runtime-v11-{build-test.log,native-result.json,validation.json}`.

## Native inventory/equipment accesses on 2949, module 0.10.0

Six CTest suites, MSVC release `/W4 /WX`, 385 reader conditions and 88 native scenarios with 578 calls and 6,219 conditions passed. The extension occupies 21 inventory/owner cases, eight cases of current client selection, eight equipment slot cases and three durability updater cases. The reader requests an explicitly known build and rejects unconfirmed selectors beforehand.

Native inventory getters and their own readers provide matching results for the same fixtures, including presence normalization, exclusion lists, and invalid slots. An inaccessible own slot checks all four unwind fragments. Client selection checked with valid/invalid native reference documents as well as normal/user and direct/registered mode. Positive base repair via the original updater remains demonstrably unsuitable: main item 35 → 100, base 5 → 0.

Equipment wrapper with delta 0: Native lock reference and real exclusive locks, second slot tag with distance `0xd0`, missing/empty/used items, sentinel and sockets. The logged definition queries must actually point to the selected slot; Return value 0 alone is not proof of selection. Source bytes, block count and release of the empty temporary item copy are checked.

24 code ranges and 20 pdata fragments pinned; six TLS readers are redirected to private data in their own copies. Three fully hashed callers additionally occupy client/server context and component paths statically. 72 Manifest Conditions run without EXE and do not count as native execution.

The 2949 reading fields thus have their own proofs. The engine thread/manager binding, transitions and the actual repair commit are still open. The equipment test does not perform non-zero delta events; Table resolvers, temporary empty items and parts of lifetime management remain fixtures. App/Rust/UI code unchanged; No new examination of these unchanged areas is claimed. Play and saves not used. Evidence: `.local/repair-runtime-v10-{build-test.log,native-result.json,reader-result.json,validation.json}`. [Precise test limits](../runtime/repair/INVENTORY_2949.md).

## Capture from held references 0.9.0

Six CTest suites, MSVC release `/W4 /WX`, 362 reader/reference conditions and the common native sample with 48 scenarios, 503 invocations and 3,867 conditions passed. Tested are the solely held owners, missing/other client selection, changes in the control reading and the release for errors. Explicitly inaccessible registry areas are not read in the protected reader.

The native 2949 host now connects real references and non-waiting Windows owner locks with the reader. Competing locks lead to termination before item access; all purchased locks are returned before references. Inventory, equipment and socket including control reading are checked under both owner locks. A registry node replaced in the meantime does not provide access to an unobsessed actor. Item sources remain unchanged.

16 Code ranges, ten pdata boundaries and five targeted redirected private TLS readers. The test without EXE only confirms 48 manifest conditions. The new lock-try feature uses real Windows imports on private data. The inventory/client anchor layout of this sample remains a legacy fixture; its interaction with the native methods is not 2949 layout approval. Manager/thread binding, world change and repair transaction remain open.

The action/blocking group/source adapter suites still exist with 1,481/25/55 conditions. App/front-end code has not been modified; No new Rust/UI test execution is claimed. Ongoing play and saves remain unaffected. Evidence: `.local/repair-runtime-v9-{build-test.log,native-result.json,reader-result.json,validation.json}`. [Details and limits](../runtime/repair/PINNED_CAPTURE_INTEGRATION.md).

## Common native registry/reference acquisition 0.8.0

Six CTest suites and MSVC release `/W4 /WX` passed. The separate native host for EXE 1.0.0.2949 consists of 30 scenarios with 263 calls and 3,490 conditions: protected client/server lookups, normal/user reference methods, PaPtr sharing, generation/masks, invalid receipts despite pointers, existing ownership, rollback, delayed cleanup and one-time destruction. Four cases use competing real Windows SRW write locks and remove registry entries before sharing. The subsequent search observes the current registry state.

15 full code ranges, nine pdata boundaries, concrete Actor VTables and the full EXE hash are pinned. Only four nine-byte long TLS read accesses are redirected to private data in the private copies. Threadmap, parent references and final cleanup/destruction are fixture dependencies; No full live host or engine commit claimed. Direct native acquisition accepts Alive=0; the additional rejection in the reader remains necessary.

Old host refuses the new EXE; New registry host denies an unknown PE test file before native execution. The remaining suites confirm unchanged 1,481 Action, 249 Reader/Reference, 25 Lock Group and 55 Adapter conditions. The sixth CTest suite only checks the 45 manifest conditions without game EXE; the native success comes from the separate local call.

App v0.5.9, desktop shortcut, original audit and the 38 captured installation hashes unchanged. No new Rust/front end test required or claimed; No game process/save access. Evidence: `.local/repair-runtime-v8-{build-test.log,native-result.json,build-refusals.json,validation.json}`. [Accurate paths and boundaries](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

## Table support for Steam 25455892 / App v0.5.9

- 181 Rust tests in the workspace, a separately executed test of the previous
Advanced tables, seven front-end unit tests and 61 UI flows passed. Passed Clippy for Workspace/all targets with `-D warnings` and format check.
- The complete file comparison includes 61 actually used tables,
Icon and language files: 57 byte-identical, only quest/stage body and header changed. All 14 indexed pairs have byte-identical round trips. New number: 52,082 Stages, 1,098 Quests. Complete stage mod comparison: only the two selected patrols are changed, all other stages including both new IDs are obtained; No quest file in the overlay.
- Build selection and backup vouchers check both known builds separately.
Wrong build ID, mixed registry/EXE metadata, incomplete files and incorrect sizes are rejected. Index/Export use the appropriate schema ID.
- Release-CLI: `metadata_matches=true`, `read_schema_supported=true`, Schema
`steam-25455892-gamedata-2.3-v2`, `certified_vanilla=false`; Table call successful. An own PE test stock with unknown version `0.5.9.0` is rejected with Exit 2 and version errors before a modbackup is checked. This Workbench-EXE copied as a test file was not executed.
- Real release app v0.5.9 in the specially launched hidden test window:
3,359 Changes reviewed over 13 tables, 2,069 skills, nine inventory areas, 122 no-wear items, 36 buff additional costs and 31 skill self-consumption costs. Repair remains locked in the surface as well as in direct requests. Passed two apply/restore cycles on a project copy including reapplication, recovery, launch protection and simulated update rejection. No lateral errors; 1,024 pixel views checked, final view additionally visually controlled.
- The game ran during the app rehearsal. 38 EXE/metadata hashes and the original
audit remained unchanged; There is no live apply and no B0 setup.

The normal Tauri build output could not replace a now open older Workbench (`os error 5`). The already compiled release EXE from `target/release/deps` was provided as `target/release/crimson-workbench-0.5.9.exe` after version/hash control and exactly this file was checked in the above desktop test. SHA-256: `30eba52e4d07237aabe35ae1ca9a6daa3de07607be1f1e8ee2db62a72cba7b18`, 16,183,808 bytes. Desktop shortcut and start script use this file; The existing window was not closed.

The native repair runtime is therefore not approved for the new EXE. A separate read-only RTTI/pdata sample recaptures eight relevant VTables. The lack of functional limits remains explicitly indeterminate; No native execution and no transfer of old addresses.

Evidence: `.local/phase5-build-update-{workspace-tests,pinned-tests,frontend-tests,ui-tests,clippy}.log`, `phase5-build-update-{fingerprint,tables,unknown-version,delivery,shortcut}.json`, `.local/phase5-build25455892-native/result.json` and `.local/build-25455892-table-comparison.json`. [Extent of build](BUILD_SUPPORT.md), [Subsequent user acceptance](../TESTCHECKLISTE.md).

## Registry source adapter 0.7.0 and build change (previous app status)

User-readable cases with expected values: [TESTCHECKLISTE.md](../TESTCHECKLISTE.md). Five CTest suites and MSVC release `/W4 /WX` passed: 55 new registry conditions, 249 reader/reference conditions, 25 lock group conditions, 1,481 action conditions. The new adapter tests use artificial manager lookups and two different sharing methods. Errors in both sources, same actor address, zero binding, full identifier, correct context binding and exact one-time return are checked.

The installed EXE is now 1.0.0.2949 / Steam 25455892. 24 of the previously 38 captured files differ. The pinned native host rejects the new EXE; This was tested as an expected negative test. CLI fingerprint reports `metadata_matches=false` and `read_schema_supported=false`. The additional CLI table test already stops at the missing project sample backup; it does not occupy a specific build error message. No native test of success for 0.7.0 and no new UI/Rust/table roundtrip test claimed.

App and audit unchanged. 38 new observed hashes confirmed again after work. No game/savewrite operations and no process interaction. Evidence: `.local/repair-runtime-v7-{build-test.log,validation.json,build-refusals.json}`, `repair-runtime-v7-{action,reader,lease,registry}-result.json` and `repair-runtime-v7-current-fingerprint.json`. [Build Observation](builds/steam-25455892.observed.json), [Treaty and borders](../runtime/repair/REGISTRY_INTEGRATION.md).

## Repair reference management 0.6.0 (previous status)

Four CTest suites and MSVC release `/W4 /WX` passed. 249 Reader/Reference Conditions, 25 Group Conditions and 1,481 Action Conditions. New Evidence: Acquisition error of both sources, balanced release also of invalid documents, blocking from references, full identifier, Alive/Destruction state before/after capture, dual/aliased owners, wrong thread, uncopiable possession and exception wind.

203 native calls and 3,859 conditions. 13 Reference scenarios with original purchase, existing reference check, PaPtr release and concrete actor release. The actual release override was controlled on five Actor VTables. Checked are both direct TLS modes and the registered path, delayed destruction, invalid receipts with pointer, special receipts and pending cleanup. Three TLS read accesses are redirected to private data; Threadmap, embedded locks and cleanup/destruction dependencies remain fixture callbacks. No claim of a full live host or actual game events.

38 Installation hashes, app and original audit unchanged. No game/save access; No new Rust/front end check. Evidence: `.local/repair-runtime-v6-build-test.log`, `.local/repair-runtime-v6-validation.json`, `repair-runtime-v6-{action,reader,lease}-result.json`. [Details and limits](../runtime/repair/REFERENCE_INTEGRATION.md).

## Repair owner locks 0.5.0 (previous status)

Four CTest suites and MSVC release `/W4 /WX` passed. 1,481 Action Conditions, 163 Reader Conditions and 25 Conditions for the blocking group. Reader samples deny item access without both locks; Error cases check return of both locks, incorrect bindings, changed anchors and identical lock objects.

140 native calls with 2,408 conditions. Original try/release copies use real Windows SRW imports on their own test objects. Tested are exclusivity, recursion, competing threads, retrieval at second lock and rejection of a write attempt at held read lock. Exactly one nine-byte long TLS read access of the try copy is redirected to private data per thread; Restbytes are unchanged. This complements the older samples with their lock stubs.

EXE verified before/after sample; 38 Installation hashes, app and original audit unchanged. No new app test run, no game/save accesses. Durability references of the real game objects and the engine commit are missing. Evidence: `.local/repair-runtime-v5-build-test.log`, `.local/repair-runtime-v5-validation.json` and `repair-runtime-v5-{action,reader,lease}-result.json`. [Details and limits](../runtime/repair/LOCK_INTEGRATION.md).

## Repair final control 0.4.0 (previous status)

Three CTest suites and MSVC release `/W4 /WX` passed. 1,481 Conditions for the action, 124 for the unchanged reader. New tests: asynchronously accepted orders do not count as a success yet; Confirmation needs all after values and events. Stranger IDs, old sessions with the same ID, wrong thread, double confirmation, partial changes, missing items, catalog/world change and timeout. After acceptance, no termination/retry; Unclear conclusion permanently blocks the queue.

122 native calls with 2,221 conditions. Six additional cases in the client corner: partial change and socket removal before list errors, correct images with delta 0, wrong server list despite correct images, normal wear as an apparent success, successful socket removal and missing slot with error code 0. The post-downgrade test distinguishes these cases from the actual repair goal.

Original updater, lock constructor and base remover; For the client, exactly one nine-byte TLS access to private test data is redirected. All other Ack bytes tested unchanged, three base unwind fragments together with prologue chain registered. UI, list allocation and virtual lock methods are stubs. No real game event shipping and no complete engine commit. EXE tested before/after sample. 38 EXE/metadata hashes, audit and app unchanged; No game process/save access. No new Rust/front end check claimed.

Evidence: `.local/repair-runtime-v4-{build-test.log,contract.log,validation.json}`, `repair-runtime-v4-{action,reader}-result.json`. [Contract conclusion, native findings and test limits](../runtime/repair/ACK_INTEGRATION.md).

## Reading repair adapter 0.3.0 (previous)

Three CTest suites and MSVC release `/W4 /WX` passed. 124 Conditions check the reader: client/server assignment via character identifier, possessor/owner checks, only existing items in the character inventory and equipment, UID/slot match, socket data, exclusions, layout/reading errors and changes during capture. 2,048 Items successfully captured; Exceeding the total budget discards the entire expenditure. Repair of the resulting private images leaves all source data unchanged.

The native host exists 116 calls with 1,387 conditions. New: Comparison of the own inventory reader with the original function on the same artificial data, Holder/Possessor paths and twelve registry cases. Real registry/type function copies, but explicitly artificial non-throwing lock/actor access methods. An invalid result can still carry an actor pointer; Validity byte checked. Your own reader does not yet have such an engine access release.

All four `pdata` fragments of the inventory getter, including their unwind chains, are registered. An intentionally inaccessible private slot returns to the test host as an expected exception. The reader rejects the same area without direct dereference. The existing action suite remains at 1,377 successful conditions. Complete EXE hash tested before/after native sample.

38 EXE/metadata hashes, app v0.5.8 and original audit unchanged. No access to the game process/saves, no installation. Real engine leases, change transaction/notifications, operator input, loader and B0 remain open. No new in-game validation, persistence commitment or Rust/front end check claimed. The existing Windows CI command captures all three suites; passed locally, no GitHub-CI run started.

Evidence: `.local/repair-runtime-v3-{build-test.log,contract.log,validation.json}`, `repair-runtime-v3-{action,reader}-result.json`. [Readers, native receipts and remaining boundaries](../runtime/repair/READER_LAYOUT.md).

## Own repair action 0.2.0 (previous status)

Both CTest suites and MSVC release with `/W4 /WX` passed. The new action suite checks 1,377 conditions: single/range selection, main/socket durability, no-wear combination, unchanged remaining item bytes, complete batch rejection for changes, identity/quantity/layout errors, sentinel and layout limits, world change, wrong threads, expiration/termination and simultaneous inputs. An unclear commit blocks follow-up orders; There is no automatic repetition.

The extended native host runs a total of 83 calls and 1,252 check conditions. Four additional calls to the original Updater copy confirm the base deviation at positive delta and the correct further processing of your own repair values with delta 0, normal wear and no-wear override. The getter and all item/socket punches are artificial test data; complete EXE hash is tested before and after the sample.

The new static library and test programs remain in the private buildorder. The actual game adapter, engine notifications, operator input, loader and B0 installation are still missing. No in-game validation or persistence commitment. App v0.5.8 remains unchanged; No new Rust/frontend results claimed. 38 EXE/metadata hashes, existing app and original audit unchanged. No gaming process or save access, no installation.

The Windows CI step is supplemented; his command was executed locally without game files and passed both suites. No new GitHub CI run has been started.

Evidence: `.local/repair-runtime-v2-{build-test.log,action-result.json,validation.json}`. [Implementation and reproduction](../runtime/repair/README.md), [Game adapter contract](../runtime/repair/ACTION_INTEGRATION.md).

## Repair runtime prototype 0.1.0 (previous)

Separate C++ release built under `.local/repair-runtime-build/Release/`; Desktop app remains v0.5.8. CTest contract test, 79 native function calls with 1,236 test conditions, three Python regressions and two existing Rust repair locks passed. Compiler warnings are errors (`/W4 /WX`). No new frontend/Tauri code; the previous app checks were not issued as new tests or re-executed completely.

Native sample: original, coordinated change candidate and restore; Appropriate, empty and inappropriate repair rules; volume/cost cases; error paths; unchanged inputs/material orders and empty prepare/commit transactions. The two intentionally triggered divisions by zero remain limited to their own test host. Source EXE fully verified before/after execution. The audit confirms 16 direct references, including overlapping opcode candidates; Indirect calls shall not be identified as audited.

38 EXE/metadata hashes, original audit and app EXE unchanged. No game process/save access, no loader installed. The repair function remains blocked due to the lack of a complete call/UI/installation path. This sample is not in-game validation.

Evidence: `.local/repair-runtime-v1-validation.json`, `repair-runtime-v1-build-test.log`, `repair-runtime-v1-audit.json`, `repair-runtime-v1-{audit-tests,core-guard,writer-guard}.log`. [Reproduction and borders](../runtime/repair/README.md).

## Additional own costs v0.5.8

**Tested:** 179 Rust Tests, a separately executed hash-tested table sample, seven front-end unit tests and 61 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. All 292 BuffInfo records and table files byte-identical reconstructed; 36 Additional costs at 0/50/200 percent checked for precise diffs. 31 further Skill own costs including single value priority checked.

Finished EXE v0.5.8: 3,359 Changes over 13 tables, two apply/restore cycles on the project copy. Reapply, Recovery, launch protection and Update Rejection passed. New cost view at 1,024 pixels and finished preview visually checked. 38 EXE/metadata hashes and the original test report unchanged; no live application, no save access, own test instance terminated. The existing desktop shortcut opens v0.5.8.

Evidence: `.local/phase5-v9-{rust,pinned,clippy,unit,ui,build}.log`, `phase5-v9-native/result.json`, `phase5-v9-validation.json`. Phase 5 remains open due to the documented technical gaps.

Manual addition to the later in-game validation (do not perform now):
- Test equipment buff with additional costs, such as myurdine, at 100/50/0 percent.
Test endurance and mind separately; Behaviour after discharging the equipment.
- Test referenced active skills and ongoing consumption separately; one
Positive regeneration effect must continue to work.
- “Additional Skills: Spirit” for resource consumption/elemental gain testing;
The individual buff exception must take precedence over the factor. Additional consumption outside these table fields shall be recorded separately.
- Enemy drain effects and dot debuffs may remain unchanged.
- Export and Restore must include the additional `buffinfo` files.

## Phase 5 residual test v0.5.7 (previous status)

**Tested:** 176 Rust Tests, a separately executed hash-tested table sample, seven front-end unit tests and 60 UI flows; Fmt, Clippy `-D warnings`, TypeScript and release build green. All 6,816 items tested for precise durability diff and idem potency, all 2,069 complete skill records byte-identical reconstructed.

Finished EXE v0.5.7: 122 Durability changes and all six new bearings tested in the actual UI, preview and export. Total 3,323 changes over twelve tables, two apply/restore cycles to the project copy; Reapply, Recovery, launch protection and Update Rejection passed. New storage and durability view at 1,024 pixels as well as preview visually tested. No browser errors. 38 EXE/metadata hashes and original test report unchanged. No mods applied live or saved content read. Own test instance terminated; The existing desktop shortcut opens v0.5.7.

Evidence: `.local/phase5-v8-{rust,pinned,clippy,unit,ui,build}.log`, `phase5-v8-native/result.json`, `phase5-v8-validation.json`. Matches will be postponed. Phase 5 is still not fully completed due to the documented technical gaps.

Additional regressions: all 6,816 items on exact durability diff, unchanged 0/65535 values, combination with item changes and idem potency; complete inventory move structures, aborted inputs and protected special containers; all nine slot areas and their limits; six additional warehouses along with durability in UI, export and project apply/restore.

[Residual point matrix and technical gaps](PHASE5_REMAINING.md). Manual in-game tests remains deferred.

## Phase 5 extension v0.5.6 (previous status)

The reader is checked with a group reference unlike Record ID and embedded identifier bytes in a UTF 8 text. Both valid cases remain legible; an invalid buff type must not be rescued by a later suffix. Additional tests separate u16/u32 references, negative resource values, regeneration flag, and item cost bytes, and check counts and any truncated prefix of a non-empty example. All new inspection fields remain write-protected. Existing buff changes preserve the new suffix byte-identical.

The real, hash-tested sample reconstructs all **2,069 complete records** from 87,697 base fields, matrix counts and 4,607 buff entries. No more gap in the byte cover outside the buff matrix. The UI sample checks name/value search, blank hits, protection against editing, unchanged existing preview and display at 1,024 pixels.

**Tested:** 175 Rust Tests, a separately executed hash-tested table sample, seven front-end unit tests and 59 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. All 2,069 complete skills byte accurately reconstructed.

Finished EXE v0.5.6: Base fields of real skills tested via IPC and UI, references protected by write protection; Name/value search and display at 1,024 pixels. 3,189 regular changes over twelve tables, export and two apply/restore cycles on one project copy successfully. Reapply, Recovery, launch protection and Update Rejection passed. No browser errors. The new basic data view and the finished preview were visually checked.

38 EXE/metadata hashes and original test report unchanged. No mods applied live, no save content read. Own invisible test instance terminated. The existing desktop shortcut opens v0.5.6. Manuals in-game tests remain deferred as agreed.

Evidence: `.local/phase5-v7-{rust,pinned,clippy,unit,ui,build}.log`, `phase5-v7-native/result.json`, `phase5-v7-validation.json`.

Manual Workbench check after completion of all phases: select skill, open **Buffmatrix & complete data set → Basic data, prerequisites & resources**; search for `varyStatAmount`, `parentSkill` and an identifier. Original values remain separate from pre-registered changes. Edit only already released skill fields above; References do not have a write desk. No new gameplay effect claimed by the pure inspection view.

## Phase 5 extension v0.5.5 (previous status)

The repair lock is checked with synthetically existing rules: global and individual flags (even to unknown item keys) must fail before another valid skill change. Neither changes nor replacements must be left behind. A separate format test checks that old requests with `free_repair: true` do not reach the writer; the flag remains readable for the targeted correction of old templates and other values are retained.

The new UI flow returns intentionally non-empty repair metadata. Nevertheless, both switches remain blocked at first. An old template with an active flag does not provide an exportable preview. The tick can be removed; then the preview with the remaining unchanged item values is again possible.

The actual zero error was only statically proven, not triggered in the current game.

**Completion:**173Rust Tests, a separately executed hash-tested table sample, seven front-end unit tests and 58 UI flows successfully. Fmt, Clippy `-D warnings`, TypeScript and release build green.

Native invisible release test v0.5.5: global and individual `free_repair: true` requests are rejected via real IPC. Normal preview with 3,189 changes over twelve tables, export and two apply/restore cycles on one project copy successfully. Reapply, Recovery, launch protection and Update Rejection passed. Repair view at 1,024 pixels and finished preview visually checked; No browser errors. 38 EXE/metadata hashes and the original test report unchanged. No mods applied live and no save content read. Own test instance terminated. in-game validation continues to be postponed on user request.

Proofs: `.local/phase5-v6-rust.log`, `phase5-v6-pinned.log`, `phase5-v6-clippy.log`, `phase5-v6-unit.log`, `phase5-v6-ui.log`, `phase5-v6-build.log`, `phase5-v6-native/result.json`, `phase5-v6-validation.json`. Free repair is still considered open; Phase 5 is not yet fully developed.

## Phase 5 extension v0.5.4 (previous status)

New checks cover the complete field delineation of nine sumon payloads and one AddSubLevel payload. All 4,607 matrix entries of the 2,069 skills are reconstructed from the displayed values byte-identical. The two new variants do not contain editable payload fields. Tests reject changes to references, counts, and newly read time/spawn values. Existing common buff changes must keep the summon payload unchanged.

Synthetic cases: non-empty select list, multi-byte UTF 8 text and consequently shifted follower fields, each truncated summon prefix, oversized counts and unsupported embedded conditions. AddSubLevel reference and unknown four-byte field are checked individually; No i64 reinterpretation.

UI: field name search including upper case, no hits, reset search filter and no write fields for new payload values. An initial new test expected an Advanced Request with an unchanged form; It is correct to omit it completely. The assertion has been corrected accordingly without changing the product code for it. **Completion:**171Rust Tests, a separately executed hash-tested table sample, seven front-end unit tests and 57 UI flows successfully. Fmt, Clippy `-D warnings`, TypeScript and release build green. After a final text clarification, the release was built again; the native test uses only this finished v0.5.4 and additionally checks its version display.

Native invisible release test: all ten new payloads via real IPC, reference protection and field name search; 3,189 Changes over twelve tables, export and two apply/restore cycles to a project copy. Reapply, Recovery, launch protection and Update Rejection passed. Summon view at 1,024 pixels and finished preview visually checked; Even the inner detail area does not have a horizontal overflow. No browser errors. 38 EXE/metadata hashes and the original test report unchanged; No live application or save content read. Own test instance then terminated.

Proofs: `.local/phase5-v5-rust.log`, `phase5-v5-pinned-final.log`, `phase5-v5-clippy.log`, `phase5-v5-unit.log`, `phase5-v5-ui.log`, `phase5-v5-build-final.log`, `phase5-v5-native/result.json`, `phase5-v5-validation.json`. Manual in-game tests remain deferred; Phase 5 is still not considered fully developed.

## Phase 5 extension v0.5.3 (previous status)

**169Rust-Tests**, **1 separately executed local table sample**, **7 Frontend unit tests** and **56 UI flows passed**. Fmt, Clippy with `-D warnings` and TypeScript successful.

- Re-occupation: complete structure, empty lists, oversized counts,
truncated records, suffixbytes and timer sentinels; Factor, individual exception and limits. All 108 real rules are parsed and halved, only the four bytes of `delayTime` may differ. Quest shortcuts, conditions, rates, close timers, nodes and protection values remain byte-identical. Header/body are rebuilt and read in. The integration test now exports and unpacks twelve changed tables.
- Inventory: both fields of all three real inventories allow 1460;
1461 and 65535 are rejected on the backend. The UI takes the same boundary from the catalogue and blocks invalid previews.
- The optional new request property remains completely with old requests
Absent. Preview, single exceptions, reset and separate patrol/spawn settings are checked in the UI.
- A new UI test uncovered an existing release marginal case:
An invalid visible number did not change the last valid request. The DOM validity is now considered after the input handlers. Export, sample and live file preview will be blocked; a prepared live release expires and does not return automatically after correction. Previous failed test runs are maintained as a diagnosis; The final full run is green.

Proofs: `.local/phase5-v4-rust.log`, `phase5-v4-pinned-final.log`, `phase5-v4-clippy.log`, `phase5-v4-unit.log`, `phase5-v4-ui-pass.log`. The table sample remains explicitly `ignored` in the ordinary workspace test and was additionally executed separately.

Native invisible release test v0.5.3: **3,189 Changes over twelve tables**, 108 rules of replacement (one single exception), 2,922 Advanced Records, Export and two Apply/Restore cycles exclusively on the project copy. Reapply, Recovery, launch protection and Update Rejection passed. All 38 tested EXE/metadata files and the original test report remained the same; Game ran, no live application or initialized Workbench in the game folder, no save content read. Worldview at 1,024 pixels and finished preview visually checked; No horizontal overflow, no browser errors. Own test instance then terminated. Proofs: `.local/phase5-v4-native/result.json`, `phase5-v4-build.log`, `phase5-v4-validation.json`.

## Phase 5 extension v0.5.2 (previous status)

**167Rust tests**, in addition **1 explicit local table sample**, **7 frontend unit tests** and **53 UI flows passed**. Fmt and Clippy with `-D warnings` successful. Two initial UI test assessments distinguished between a missing optional property and `undefined`; The productive JSON/Plan canonization correctly omitted both. The corrected tests continue to verify that removing overrides does not leave an outdated plan.

New checks: accurate i64 change and identity of all remaining buffbytes; Boundaries, fractions, exponents, unknown paths, structural fields, references and opaque values are rejected. Graph curves remain protected. Cooldown and buff changes on the same real skill arrive together in the overlay. Old requests without new optional fields retain their serialized form.

Patrols: both original timers, factor and single override, positive boundaries, sentinel rejection and cut-off prefixes. All other stagebytes and records remain unchanged. City flight: exact rule identity, expression, tree and flags; Foreign values and already changed rules are rejected. Both control options only change the same condition record; all other 10,797 conditions remain byte-identical. The archive integration builds and unpacks eleven tables exclusively in the project/temp area.

Final protocols: `.local/phase5-v3-rust-final.log`, `.local/phase5-v3-pinned-final.log`, `.local/phase5-v3-clippy-final.log`, `.local/phase5-v3-unit.log`, `.local/phase5-v3-ui-final.log`. The optional table sample test remains in the normal workspace run `ignored` and was actually run separately. Manual in-game validation remains open.

TypeScript and release build successful. Native invisible test of EXE v0.5.2: **3,081 Changes over eleven tables**, i64 buff change plus cooldown, both patrol timers and city flight rule in the same plan; Export and two apply/restore cycles only on the project copy. Reapply, Recovery, Start/Backup Protection and Update Rejection Successful. All 38 tested EXE/metadata files and the original test report unchanged; Game ran, no live application or initialized `.workbench` in game. Own test instance then terminated. Screenshots at 1,024/1,440 pixels visually checked. The first native launch in the Sandbox did not have accessible WebView test access; The final run with a local test profile and necessary execution rights was complete. Proofs: `.local/phase5-v3-native/result.json`, `.local/phase5-v3-build.log`, `.local/phase5-v3-validation.json`.

## Phase 5 extension v0.5.1 (previous status)

**162Rust tests**, in addition **1 explicit local table sample**, **7 frontend unit tests** and **51 UI flows passed**. Fmt, Clippy with `-D warnings`, TypeScript and release build successful.

New checks: Enchant copy with prices/buffs/stats, sorted insertion with correct disconnectors, rejection of duplicate targets, concatenated copy sources, unknown sources and unconfirmed original structures. State change on a new level, newly built headers and unchanged other item records. Stacks are also checked for newly added buffs/enchant lines. Old templates without copying rules remain compatible; Empty optional lists do not make a preview erroneously obsolete.

All 2,069 skill matrices and 4,607 entries including zero are completely and seamlessly inspectable. Each field representation output is converted back into bytes in the test and compared with the original bytes. tests for cropped data, invalid flags/counts, additional bytes and signed values; New IPC query rejects outdated sessions. Buff values remain read-only, raw data/type limits are not confirmed gameplay semantics.

Native Invisible Run: `.local/phase5-v2-native/result.json`, **3,077 Changes across nine tables**, new Enchant Level and Skill Matrix view, Export and two Apply/Restore cycles to project copies only. Reapply, Recovery, Start/Backup Protection and Update Rejection Successful. All 38 tested EXE/metadata hashes and the original test report unchanged. the game was played; No initialized `.workbench` in the installation, no live application. Screenshots at 1,024 and 1,440 pixels visually checked.

Protokolle: `.local/phase5-v2-{rust,pinned,unit,ui,clippy}-final.log`, `.local/phase5-v2-build.log`, `.local/phase5-v2-validation.json`.

## Phase 5 tests v0.5.0 (previous status)

Rust-Workspace plus local additional real file sample, frontend unit tests and headless UI flows check the new status. Final logs and counts are below `.local/phase5-*-final.log` or `.local/phase5-validation.json`.

Result: **158Rust tests**, additionally **1 local table sample**, **6 frontend unit tests** and **49 UI flows passed**. The optional table sample test is skipped in the general workspace run and was explicitly performed separately. Fmt, Clippy `-D warnings`, TypeScript and release build successful.

New core tests: negative costs without alteration of positive regeneration; original structure instead of repeated multiplication; precedent globally → skill → single field; unchanged opaque bytes; Sentinel maintenance, overflow rejection, unknown IDs/categories/fields; blocked non-existing repair rules. The local hash-checked sample reads all supported records, modifies multiple tables, inserts a stat, and checks the new header and all other item records for byte identity. The regular real file integration builds an overlay with nine tables and unpacks it exclusively in Temp.

Frontend: exact requests, canonical comparison for Rust sorting, invalid inputs, locked repair option, signed skill values, local templates via restart and to another item, late IPC responses and 1024px without horizontal overflow. Invalid master memory is rejected.

Native run `.local/phase5-native/result.json`: v0.5.0, 2,811 Records, 2,069 Skills, 6,816 Items, 2,833 Items without detected Zustand Features, 0 Items with repair rules, 3,076 scheduled changes across nine tables. Passed workspace rehearsal with two Apply/Restore cycles, Reapply, Recovery, Hold Protection and Update Rejection. 38 Original EXE/metadata files and the original audit unchanged. No live app, no save access, no control or lock of the current game. Screenshots have been reviewed.

### Manual acceptance B4–B11 – only after all stages of development

All of the following points are **open**, none were automatically rated as a gameplay success. Requirements from the B0 test plan continue to apply. In each case, check a small, clearly observable example individually and then carry out restorations. For each line, document original/mod/restore observation and build number.

|Module|Later examination|
|---|---|
|B4|Compare identified terrain/pool group with ×2 and single exception; No uninvolved spawns are changed. mount call and move in the MainField/in a city; Measure duration and cooldown. Observe Hidden Estate/Watergate reset with factor and individual value; Record real clock and playtime separately. General NPC timers remain open.|
|B4 Resettlement|Original, 50 % and a single exception in known fraction areas. Existing conditions/quest states, playing time versus real time and check the original waiting time after Restore. Do not assume general NPC coverage.|
|B5|Blackstar cooldown, 10-/30-minute duration and own value measurement. Check Abyss regional exclusion and off-road flight separately, including with other mounts; Bounty/quest conditions unchanged. Examine other boundaries; Record exceptions/scripted descents. Boss Dragon unchanged.|
|B6|Check start/maximum seats for all nine areas including Kuku and five housing warehouses individually, including pre-existing saved games. Only moderate increase; large values and reduction do not take over as existed.|
|B7|Ordinary stackable items, selected category and individual items compare; Splitting, buying, looting, moving and reloading. Test experimental equipment separately with different durability/enchant/socket conditions; Merging/loss would be a mistake.|
|B8|Known wear action with "No durability loss" and according to Restore compare: ordinary equipment, special equipment with finite durability and socketed items separately. Existing specimens are not a recovery test. Repair cost verification is postponed because this build does not yet have a released cost path.|
|B9|Compare Original, 50 %, 0 % per category; Cover movement/combat and driver cases. Positive regeneration must remain unchanged. Explicitly document unrecognised costs.|
|B10|Change an actual non-empty cooldown globally and individually, consider priority; Check known numbers on a single skill. Targetedly change a confirmed i64 buff value; Other matrix bytes remain unchanged. Do not declare unknown meanings as tested effects.|
|B11|Test an item/enchant level with a stat and a compatible buff. In addition, copy a new level from an original level; Table existence alone does not prove an achievable upgrade path. Equip/Unequip, actually reachable levels, new/existing item and reloading compare. Apply submission to a suitable second item; Inappropriate sources/levels/IDs must be rejected.|

Then check joint overlay of several modules, restore and reapply. Thus, neither the open format functions nor the in-game validation are anticipated.

## Phase 4 tests v0.4.10

**154 Rust tests passed**, including four real file integrations. Fmt and Clippy with `-D warnings` successful.

Nine new core scenarios: exact additional confirmation for inventory/content report and Apply/Reapply/Restore, unchanged foreign PAZ/PAMT/DLL files, collision-free own groups, empty files and additional `.EXE`, same sized subsequent changes, new children and obsolete previews; Revocation before/during protected work with release of handles; locked original changes, foreign registries, saves including configured protected subfolders, private paths and hardlinks; damaged confirmations and input limits; paths later taken over by Steam; Basic change while receiving foreign files. The last of the 128 log entries is reserved for revocation. All eight foreign file tests passed again after supplementing this border case.

44 UI flows and 4 frontend unit tests passed, TypeScript and frontend build fit. Five new UI flows check voluntary call, explicit accurate confirmation without live action, revocation even if the new preview is blocked, outdated responses, new checkbox for errors, empty inventory and 1024px layout. The new process was not applied to a real third-party mod installation.

Release v0.4.10 and native Windows check passed. Additional file preview checked on the real installation purely reading: empty stock, no confirmation and no live setup. Three new session locks, existing seven live locks, workspace rehearsals and restore successfully. No JavaScript errors; 1024px view visually checked. All 38 EXE/metadata hashes and the original user report unchanged; No complete PAZ hash-run and no game lock. Evidence: `.local/phase4-v11-validation.json`, `.local/phase4-v11-native/result.json`, `.local/phase4-v11-rust-tests.log`, `.local/phase4-v11-foreign-final.log`.

Later manual removal: see [Fremddatei-Checkliste](FOREIGN_FILES.md). The confirmation of additional files is not a test of their game effect. Registered third-party modes are not merged with Workbench mods; Original changes first need to restore an occupied base.

## Phase 4 tests v0.4.9

**145 Rust tests passed**, including four real file integrations. Fmt and Clippy with `-D warnings` successful. Eight new core tests check byte-like archive maintenance and Apply/Restore on the new basis, six interruption limits, partially new backups, name collisions, equally large changes of own files, foreign children, source changes, cancellation, double assignment of archive/source, journal manipulation, hardlinks, partial project publications and the integrated inventory/content check. Synthetic unknown builds will continue to be rejected by the public approval path.

Five new UI flows check preview and confirmation of origin at base change, write lock during play, targeted recovery, invalidated selection and separate display of own/foreign files. 39 Passed UI flows and four frontend unit tests; TypeScript and frontend build successful. A Windows test result on open child files when renaming a folder has been fixed: archive files are re-verified after moving and held until completion. A regression test for pure preview with locked output cache checks that missing live approvals do not require unnecessary write permission.

Release build and native Windows check passed: real inventory/stored base/live preview read only, seven live session locks checked and existing mod/recovery workspace rehearsals successfully reappeared. No live setup, apply or base change on the real game, no game lock. All 38 observed metadata and EXE hashes as well as the user report unchanged. No JavaScript errors; 1024px screenshot. Evidence: `.local/phase4-v10-validation.json`, `.local/phase4-v10-native/result.json`, `.local/phase4-v10-rust-tests.log`.

Subsequent manual acceptance after completion of all development phases:

- Apply your own mod, open inventory: own groups separately, registry as
own mod marked; New Vanilla content report blocked.
- After completing the update/Steam-Verify capture new base. If unknown
Scheme no application; in the case of a known scheme, preview and start base change.
- Old history and groups in the archive, new original registry unchanged;
Recalculate the mod, apply it and then reset it to a new basis.
- Explicitly continue interrupted base change; No automatic
Repeat when opening the app or when navigating.

## Phase 4 tests v0.4.8

**137 Rust tests, 4 frontend unit tests and 34 headless UI flows.** Full Rust run including four real file tests; Fmt, Clippy with `-D warnings`, Frontend and Windows release build successful. The 69 previous write-off points remain included. After completing correct body/header pairs in the new artificial test archives, all five new adapter tests passed.

Apply/Reapply/Restore were tested on synthetic installations, byte-identical original sources and original data view to Apply, outdated previews, foreign files, same-sized source changes, missing/defective backups, cancellation with handle release, recovery before and after registry commit and the write lock of the pure inspection core. An additional worker test checks individual order, session isolation, discontinuation, and correctly reported late success.

The five new UI flows check explicit confirmation of origin, write lock during play, connection of the current field plan, obsolete releases without automatic retry, independently available withdrawal, worker cancellation and delayed responses after changed mod values. All 34 UI flows passed in the first total run.

Native Release v0.4.8: The real live setup preview was checked only read with 285 original files. No origin confirmation set, no setup or application triggered. All six IPCs refuse invalid sessions. Existing workspace rehearsals and restore existed; no JavaScript error, 1024px screenshot checked. The game continued, no live transaction folder was created, 38 observed EXE/metadata hashes and original user report unchanged.

Evidence: `.local/phase4-v9-rust-tests.log`, `.local/phase4-v9-native/result.json`, `.local/phase4-v9-validation.json`. [Expiration and remaining borders](LIVE_APPLY.md). Actual game effect, arbitrary power outages and arbitrary external starting paths are not proven by these tests. The live setup of the user computer has expressly not taken place.

## Phase 4 tests v0.4.7

**131 Rust tests, 4 frontend unit tests, 29 headless UI flows.** Fmt and Clippy with `-D warnings` passed. Release build and native Windows acceptance successful; four real file integrations and previous 69 transaction breakpoints are still included.

Seven new basic tests: unchanged report and matching registry copy; missing/contradictory reporting fields, duplicate fields/paths, unauthorised flags of origin, path breaks and size overflows; obsolete preview/build change; cancellations by report copy, by registry copy and before publication of the manifest; manipulation/hardlinks; Getting old backup for changes; Protected targets and input limits. The same file size alone is expressly not considered as proof of content again; a corresponding negative test passes without confidence release.

Four new UI flows: explicit adoption and reopening after navigation, refusal of changed previews without repeat attempt, update/corruption status, discarded selection and delayed responses. Entire UI run on the first run green. 1024px without horizontal overflow.

Native acceptance takes over the real user report from 20.09.2026 byte-identical, creates the matching 679 byte registry copy in the project and checks its hash after reopening. Four new IPCs refuse outdated sessions. CLI confirms the persisted status after termination of its own test instance. User report and 38 observed game EXE/metadata hashes unchanged; The game continued. Existing mod/recovery samples also passed. No game or savewrites, no game locks, no new complete hash run of the original archives.

Evidence: `.local/phase4-v8-rust-tests.log`, `.local/phase4-v8-native/result.json`, `.local/phase4-v8-baseline-status.json`, `.local/phase4-v8-validation.json`. [Operation and limits](BASELINE.md).

## Phase 4 tests v0.4.6

**124 Rust tests and 4 frontend unit tests passed; 25 successfully tested various UI flows.** Fmt/Clippy with `-D warnings` and Release Build passed. Four real game data integrations are included. The 69 existing transaction breakpoints now additionally check the concrete withdrawal preview against the before/after hashes and actually removed files.

Six new core tests: missing backup is not recreated, preview does not change content files or directories, restorers comply with the plan, open transactions before/after commit are correctly arranged, plans become obsolete in new transactions and are not transferable between project copies. source updates, foreign files, tampered registry/overlays, corrupted backup, hardlinks and protected paths are rejected without cleanup; List size limited.

The first run of the new tests failed due to an incomplete synthetic table pair in the fixture; The necessary header has been added. In the first UI run, the 21 previous and two new flows existed. Two new test selectors have been corrected: `option.disabled` check directly and address the item database button including the counter. After that, all four new flows passed. No production code has been adapted to circumvent these fixture/selector errors.

Native release acceptance: backup status and restore deactivated with already reset sample; deliberately missing final marker in a new copy of our sample; Open transaction correctly detected, restored via UI, final marker byte content identical and subsequent Zustandchecking green. All new IPCs refuse outdated sessions. CLI List/Preview/Restore on the same copy also successful. 1024px without overflow and visually checked; No JavaScript errors. Existing modflows and process/update protection samples green. Game continued, 38 observed game EXE/metadata hashes unchanged.

Evidence: `.local/phase4-v7-rust-tests.log`, `.local/phase4-v7-native/result.json`, `.local/phase4-v7-restore-preview.json`, `.local/phase4-v7-cli-recovery.json`, `.local/phase4-v7-validation.json`. [Operation and limits](BACKUP_RECOVERY.md).

## Phase 4 tests v0.4.5

**User acceptance of full content verification, 20.09.2026:**Exported desktop report `exports/installation-audit-1789903405-11284-1-0.json` checked. 285 Files / 154,097,618,899 Bytes / 195 PAZ archives in 140 seconds; all SHA-1 comparisons are successful, metadata according to the report stable. Report consistency and match of 38 previously observed SHA-256 values confirmed without re-accessing game content. Detection: `.local/user-content-audit-review.json`. This complements the automated acceptance described below; Vanilla certification, live apply and manual in-game module testing remain open.

**118 Rust tests, 4 frontend unit tests, 21 headless UI flows passed.** Fmt/Clippy with `-D warnings` error-free; four real file tests and previous 69 transaction breakpoints included.

New [Content verification](CONTENT_AUDIT.md): known SHA-1/SHA-256 vectors, multiblock/blank files, same sized content changes, cancellation, game status errors and new game launch, changes of already hashed sources, depot/build change, growth, additional/missing/hard linked sources. The real public process check is checked before content accesses; Our own read handles allow competing writes and recognize the subsequent metadata change. Worker tests check an active order, session limits, abandonment and panic. UI flows check activation after metadata checking when the game is over, progress, change of view, cancellation, error message, deviations and report export.

An early total run met a process query for the end of a `probe.exe` of the same name from another test. The protective barrier remained closed; the affected own process fixtures are now serialized. Pass the isolated test and re-run. The UI fixture had to replicate the fresh objects of real IPC responses instead of changing React state objects in-place.

Native release acceptance v0.4.5 passed: new content button locked during the running game; Start/Cancel/Export refuse invalid sessions. All 18 commands are consistent with the purely local main-window capability. The great original hash run was not started; Positive content runs use synthetic files. Existing modflows and protected workspace rehearsals including recovery and refusal; CLI recovery of the same sample as well. 1024px without overrun, screenshot checked, no JavaScript errors. All 38 observed EXE/metadata hashes unchanged after the native sample. Evidence: `.local/phase4-v6-validation.json`, `.local/phase4-v6-native/result.json`, `.local/phase4-v6-rust-tests-final.log`.

## Phase 4 tests v0.4.4

**106 Rust tests, 4 frontend unit tests, 17 headless UI flows.** Fmt and Clippy with `-D warnings` error-free. Four real file tests were carried out; 69 existing transaction termination points remain included.

Ten new Rust tests for [Installation test](INSTALLATION_CHECK.md): all cut-off positions of a synthetic repository list, CRC/IDs/sizes/signature status, path breakouts/devices/devices/duplicates, protobuf overflows, additional/missing files, type changes, registry defects, missing/contradictory caches, Steam update states, metadata limits, depth and a Windows junction. Same file sizes with changed content must never be considered as proof of content or vanilla.

Two new UI flows: conscious start instead of automatic scan, additional EXEs, confidence limit and repetition; visible error message with restart. 285 The first native acceptance found a missing Tauri-ACL assignment for the new read command. `build.rs` The final release EXE consisted of the repetition including real inventory, existing modflows, protected workspace rehearsals 1024px without overrun, no JavaScript errors; CLI recovery of the same v3 sample also successful. 38 observed EXE/metadata hashes unchanged after the native sample.

Evidence: `.local/phase4-v5-validation.json`, `.local/phase4-v5-native/result.json`, `.local/phase4-v5-rust-tests-final.log`, `.local/phase4-v5-cli-recovery.json`.


## Phase 4 tests v0.4.3

**96 Rust tests, 4 frontend unit tests and 15 headless UI flows passed**; Fmt/Clippy with `-D warnings` error-free. Four real file tests were actually performed. The 69 previous transaction breakpoints remain included.

- Integrated session locks via Apply/Reapply/Restore/Recovery: EXE start,
Source/Backupwrites and renaming of relevant parent folders blocked.
- Already on-going own test copy detected before state/backup creation; Real estate
Process paths are canonized on both sides. Process testing errors also lock recovery and restores; Handles remain held until the end of the session.
- Abort after intent release with held locks, then recovery.
- Simulated source update before recovery and after commit: new sources/registry
byte-identical remain, old backup and own overlay files are not written back or removed via a foreign / new Zustand.
- Manipulated protection manifests, v3 downgrades, damaged backups, hardlinks,
Path breakouts and a claimed vanilla origin are rejected.
- Real merchant data: own empty item selection and suppress original refresh
global values; Another merchant outside the selection gets his own items and daily refresh. All other merchant records remain byte-identical.
- UI: submit individual selection/refresh, then inherit again and
Remove the exception; Protective/Update probe is displayed with its test scope.

The first protection tests revealed that exclusive EXE opening alone does not detect already running images and attribute access does not hold an effective folder lock. Another test required the process path comparison, which was canonized on both sides. After these corrections, the tests passed without weakening their requirements.

Proofs: `.local/phase4-v4-rust-tests.log`, `.local/phase4-v4-mods-tests.log`, `.local/phase4-v4-protected-tests.log`. Expiration and Limits: [Protected workspace rehearsals](PROTECTED_REHEARSALS.md). The final release EXE v0.4.3 also passed the native WebView2 test with real data: 34,488 previous changes, protected trial cycles with recovery, refusal to update, daily refresh, table growth, drop chances and manual warranties. The individual merchant selection correctly replaced the global item selection; Export and 1024px passed without JavaScript errors. An initially ambiguous test selector for nested details has been corrected; The same EXE then passed the complete run. The new CLI subsequently restored the same v3 sample idempotent. Documents under `.local/phase4-v4-native/`, `.local/phase4-v4-cli-recovery.json` and `.local/phase4-v4-validation.json`.

Windows has actually been tested; no Linux version and still no live apply, no vanilla certification and no in-game acceptance.

## Phase 4 tests v0.4.2 (historical)

**87 Rust tests, 4 frontend unit tests and 14 headless UI flows.** Fmt/Clippy with `-D warnings` error-free. The full workspace run contains four actual real file tests; then passed the extended shop test for all 6,816 items at a dealer also.

- New stock positions receive new list/save indices; existing gaps,
Prices, conditions and original positions remain byte-identical. Double save indices and changed list order block additions.
- A pure long position does not prevent a new purchaseable offer of the same
items. Buyable/Sellable counts remain consistent.
- Daily Fresh only changes the confirmed daily field; Sentinel intervals
remain intact. Real global preview: exactly 246 changes.
- Small addition and complete range at retailers 3101: body growth,
recalculated header, encrypted overlay readback and all other merchant records unchanged.
- Quantities ×3 and Opportunity ×2 at Set 175521: Rate 35,000 → 70,000
generated bytes are tested. Exact scaling, zero factor, 100 % cap and wide intermediate values; manual warranty takes precedence over opportunity factor 0.
- rejection of weighted/conditional odds, guarantee with quantity factor 0,
unknown items, unsuitable dealer and oversized preview selection.
- UI transfers daily replenishment, additional items and manual warranties
jointly; Reset resets the options. Existing export/trial/exception tests and locked live apply remain green.

The final release EXE v0.4.2 passed the native WebView2 test with real data: existing 34,488 change plan, two Apply/Reapply/Restore cycles plus recovery, 246 daily refresh changes, two new merchant items 3101, Opportunity ×2 and Quantity ×3 in a common plan, and explicit 100% base rate. The grown table also consisted of two test cycles plus recovery; Exports and 1024px checked without overflow. No JavaScript errors. The game continued.

A first native test run failed due to incorrectly coded umlauts in the test script. After script correction, the same unchanged release EXE passed all exams. The launcher only closed its own invisible test instance.

Evidence: `.local/phase4-v3-rust-tests.log`, `.local/phase4-v3-all-items-test.log`, `.local/phase4-v3-native/result.json` and `.local/phase4-v3-validation.json`. Static field documents: [PHASE4_FIELDS.md](research/PHASE4_FIELDS.md). Live B0, universal dealer coverage and in-game proofs remain open.

## Phase 4 tests v0.4.1 (historical)

Result: **83 Rust tests, 4 front-end unit tests and 13 headless UI flows green**; Fmt/Clippy error-free. Full workspace run with 82 tests plus final format suite with 18 instead of 17 tests after completion of the exact length test. The final release EXE v0.4.1 passed the native test with 34,488 changes, reapply/recovery, six file transitions, export and 1024px display.

Additional checks on the new private transaction core:

- Two encrypted overlay groups, reapplying modified content,
Receive old files until registry commit, then restore byte-identical.
- 69 injected quits at Apply, Reapply and Restore, including partial writes,
Publication of the group, registry commit, cleanup and proof of financial statements.
- Repeated cancellation during recovery before and after registry commit.
- Protection against occupied IDs, outdated plans, foreign content, damaged backups,
Parallel locks, hardlinks and failed process checks.
- Testing of the entire PAMT/PAZ pair including the exact decompressed length, and
Reject duplicate virtual table paths across multiple groups.
- Public recovery command denied protected, misplaced,
Unmarked and unknown versioned samples and linked markers.
- Desktop sample with real data: two Apply/Reapply/Restore cycles, Recovery,
six file transitions; Headless UI checks the display including remote files.

New native proofs are under `.local/phase4-v2-native/`, Rust output under `.local/phase4-v2-rust-tests.log` and `.local/phase4-v2-format-tests.log`. `mod-recover` was also repeated by CLI on the native sample (idempotent, original registry obtained). The final comparison confirms unchanged hashes of all 38 observed EXE/metadata files; No complete ETA reconciliation. Injected abortions are not actual power outages or evidence of all file system/disc failures. Live launch protection and in-game acceptance remain open.

## Phase 4 tests v0.4.0 (historical)

Result: **74 Rust tests, 4 frontend unit tests and 13 headless UI flows green**, as well as native testing of the final release app v0.4.0. Fmt/Clippy error-free.

- New `mods_live` test with actual tables: exact coverage, unchanged
Table pairs, deterministic overlay, targeted merchant/dropset selection, all other records byte-identical, three friendly values and negative penalty, sentinel rejection, unknown option names and input limits.
- Synthetic store test: unknown September bytes, optional block and
The effect list is maintained exactly when the inventory is changed. Index growth preserves body prefix and nonphysical header key order.
- Overlaytest reads encrypted LZ4 body/header via the real archive reader
back, checks determinism, collisions, pairs, and forbidden file names.
- Private transaction sample: six Apply/five Restore interruption limits,
Reopen/recovery, damaged backup, competing lock, foreign registry/group change, hardlinks and ongoing/unclear process check.
- Windows start protection test exclusively with a copied own test EXE:
Start with exclusive file handle refused, successful after release.
- Four additional headless UI flows: explicit preview, always blocked
Live Apply, verified Plan ID during export/trial run, stale preview, selection and exceptions, input error, 1024px and backend rejection.

`app/scripts/native-mods-smoke.mjs` checked the real interface in a separately launched invisible Workbench window: all shops 999, quantities ×2, Trust ×3, file plan, two trial runs, export with credits, Trust penalty unchanged and block an outdated preview. Result/screenshots under `.local/phase4-native/`. The script does not start and operate a game. The associated launcher then terminates only its own Workbench test instance.

The interruption tests are not a power failure simulation. Full recovery of any cancellation during file I/O, live reapply, multiple groups and built-in protection against any launcher path are still necessary before live sharing.

## Phase 3 detection

65 Rust tests, 4 frontend unit tests and 9 headless edge flows passed. `crates/cd-core/tests/crafting_live.rs` checks the complete recipe/group round trip, all shared item dropsets, the concrete arrow/pot examples and the planning of each of the 1,108 shared recipes. Missing local configuration is explicitly skipped; Configured unknown builds fail.

`app/scripts/native-crafting-smoke.mjs` checks the real app with the local data in an invisible own WebView2 window. Screenshots/result: `.local/phase3-native/`. Passed debug and final release EXE; the previous item browser including language change was then checked again with the release EXE (`.local/phase3-item-regression/`). No game start, no in-game action, no save access.

Reproducible commands: same workspace/frontend commands at the bottom; Additional `cargo test -p cd-core --test crafting_live --locked -- --nocapture`.

Manual view:

- Production target arrow (50001), quantity 31, timber stock 3: 7 timber and
2 Iron ore absent; 60 Arrows are made and 29 remain.
- Choose mixed stew, change recipe 343, seafood alternative:
Tree, Residual Requirements and Stock List must use the same item selected.
- Intermediate product present / manually procure, change recipes, empty supplies.
- Switch to the computer via item details → shortcuts → production.
- Source gaps, unknown opportunities and cycle warnings not as secured
Misunderstand world sites, 0 % or free production.

Linux was still not running locally. The numbers of the earlier phases below are historical evidence.

## Phase 2 detection

```powershell
npm ci --prefix app
npm run build --prefix app
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --test-threads=2 --nocapture
cargo clippy --workspace --all-targets --locked -- -D warnings
```

57 Rust tests, 4 frontend unit tests and 6 headless edge flows passed on Windows. The new real file test checks German AGV search, complete Stringinfo round trips, actual DDS icon decoding, stats, pagination, raw fields and JSON export in an isolated temporary project. No spending in the game.

In addition, the built Tauri app ran with real WebView2 in a **invisible own test window**, not in the browser mock: real item 2200, icon, raw fields, JSON output and language change ger → narrowly passed, no JavaScript errors. Tester: `app/scripts/native-smoke.mjs`; Results/screenshots under `.local/phase2-native/`. The own test process was then closed.

Manual view, without in-game intervention:

- Starting the app; Check for automatic detection or path selection.
- `Stumpfpfeil` search, view icon/name/fields; Combine several filters.
- Scroll list far, try language changes, unknown fields and item links.
- Save JSON and check the `exports/` path displayed.
- Missing merchant/drop/prescription relationships are visibly marked as open and
are not interpreted as “no sources exist”.

Linux-CI is prepared, not executed here. The following phase 1 numbers remain the proof at that time; The current overall status is above.

## Phase 1 Detection and repetition

```powershell
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
cargo test --workspace --locked -- --test-threads=2 --nocapture
cargo build --release --locked -p cd-cli
.\target\release\cd-cli.exe roundtrip --all-fields
.\target\release\cd-cli.exe search Stumpfpfeil
```

51 Tests passed on Windows, including the actual test against the local installation. The CLI confirmed 34 byte comparisons and the field coverage of all 6,816 items. Release CLI end-to-end checks also controlled FTS5, cache reuse, language change, builddiff, unknown builds, and file output protection. Logs remain under `.local/` and are not checked in.

The real file test reads `.env` in the project root and then the game archives. If `CD_GAME_DIR` is missing, it reports **SKIP**; `--nocapture` makes this visible in the log. A set but invalid or modified build is an error, not a skip. No original files are written back or save contents are opened. Synthetic error/path tests use temporary, isolated directories.

GitHub Actions checks Windows and Ubuntu; Linux did not run locally. Frontend testing starts with the frontend in phase 2. The 13-fold raw data body round trip is not a semantic editor release; PALOC recompression is not detected as byte-identical.

## Execution limit

The user plays during development. In phase 0 and 1, the game directory and saved games remain unchanged; Also Steam file verification, Apply, Restore and game restart are not triggered automatically. Manual in-game tests is carried out by the user according to his later decision only after completion of the development of all phases.

## Phase 1: Core and Parser

- `cargo test --workspace`: synthetic valid/invalid data, abbreviated headers,
Test invalid lengths/offsets/references, unknown builds and limits.
- Real input only from `CD_GAME_DIR` and `CD_SAVE_DIR` in `.env`;
Missing installation is expressly skipped. An early return of a reference test does not count as a passed test with real game data.
- Parse and serialize each supported table body and its header:
Byte comparison, not just the same data set number or the same JSON. Also unknown fields, padding and order must be preserved.
- Check hash and complete reading position formats designated for lossless.
An unchanged raw byte block alone does not occupy a correct field interpretation.
- localization with German texts, umlauten, missing keys and the
Check the current LZ4 envelope. Invalidate search index including FTS5 after change of build and rebuild deterministically.
- Read code does not receive writable game/save handles. Testing only uses
isolated test folders for their expenses. Check Linux compilation in CI separately.

## B0: Required before the first module application

- Unknown build, foreign hashes, unchecked backup, insufficient memory,
Missing rights and ongoing/unreliably verifiable game process must cancel before the first change. This also applies to restore and recovery.
- Dry Run is not allowed to change a game file. He mentions every file, technical change,
Before/after hashes, overlay ID and conflicts.
- Create overlay in isolated copy and check with an independent reader:
Registry, PAMT, checksums, PAZ offsets, compression, body/header consistency.
- Vanilla → Apply → Restore: all affected original files byte-identical;
then test Apply → Restore → Apply with deterministic result.
- Change settings/deactivate modules: recalculation from vanilla; None
Multiplication of already modified values, no remaining modules.
- Simulate crash/abort at each transaction boundary: after backup, individual
Staging files, overlay deployment, registry change and verification.
- Simultaneous Apply, Game Launch During Commit and Third Party Changes Between
Preview and commit test. Process testing alone does not protect this process.
- Simulate update to Apply; never have registry/backups of an old build over
Write new game archives. Safely clean up or lock down with accurate diagnosis.
- Final manual proof: start game with the smallest reversible change,
Load main menu/save, check effect, close game, restore, restart. No release solely due to a formally valid overlay file.

## Manual module checks (all still open)

First document current vanilla values per module, activate only this module, preview, close game, perform Apply, start game and watch. After that, close the game and perform Restore together with the hash comparison. Combinations only after successful individual tests. saved games are never written by Workbench; For tests, the user uses his own backup if necessary.

|Module|To be checked in game|
|---|---|
|B1 Shops|Selected dealers and items, price unchanged if not edited, stock 999, change of day/refresh, purchase of multiple slots, excluded and quest items; Check large lists for UI/engine boundaries.|
|B2 Drops|Selected dropset before/after change; Check chance/number and group semantics separately. 100 % of confirmed selection, no unattested boss classification; Statistical tests for random drops.|
|B3 Trust|The same NPC and the same action before/after change; Measured confidence growth, limits/negative or special values, no unintended item drop changes.|
|B4 World|Spawn number, expiry of the respawn timer, multiple regions, mounts in cities, ride duration/cooldown; Explicitly check missions, duels, cutscenes and interactions.|
|B5 Dragon|Blackstar call, re-call, expiration of ride duration and crossing several regional borders; Check special zones and mission-related barriers separately.|
|B6 Inventory|Inventory and stock separately: free number, purchase/loot/resorting, storage/reloading, limit values. Do not release a maximum value as safe before it is empirically proven.|
|B7 Stacks|Stack/share/buy/store/reload consumer goods and materials. Equipment with durability, enchantment or bases exclusively in the explicit experimental test; Instance values must not be lost.|
|B8 Durability|Compare the actual durability of several weapons/tools before and after load: no loss is accepted, not just high durability through max_endurance=65535. repair price/action separately; Exclude negative/invalid states.|
|B9 Stamina & Spirit|Sprinting, climbing, flying and skills separately; Check factors 0/0,5/1 as well as unlimited mode, UI display and actual consumption.|
|B10 Skills|Global and individual cooldown, overlap, multiple skill types, special numerical values; Test effects of other values only after confirmed semantics.|
|B11 God-item|Change individual confirmed Stat/Buff, then add several; Display and actual effect, equipment/reloading, empty/multiple occupied arrays, size change and header offsets. Reject incompatible Weapon condition passives on non-weapons before Apply and test this refusal automatically; documented candidate for continuous loading/heavy storage consumption.|
|C Profile|Normal ↔ Farm with drops ×10/Spawns ×3; Disabled modules disappear, repeated Apply identical, running game blocks changes, JSON import errors do not damage a profile.|

Additional v0.4.2 acceptance for B1/B2, still open:

1. Small range of items only at a supported retailer: new items
buyable, old stock unchanged, prices plausible, no duplicate offers.
2. Purchase → change of day → re-visit: replenishing normal offers; once a
Offers remain unique. Save/Reload by the user checks that new save indices do not change other items of goods.
3. Per merchant putting own items, with empty list global supplements
suppress and receive original refresh; Check for explicit exception outside of global selection and subsequent reset for inheritance. Only then larger sets of articles; Test special items and surface boundaries. All items at all merchants are not released in this version.
4. Small known roll type 0 set: chance ×2 and set ×3 separated and combined,
then manual base rate 100 %. separately consider higher-level triggers and possible runtime modifiers; Not an automatically recognized boss.
5. Chance 0 without guarantee: no corresponding entry; With an explicit guarantee
has that priority. Volume factor 0 plus warranty must be rejected before export.

## Other functions

A1: Search, Categories, German/Fallback, unknown fields as offset/type/raw value, verified cross-references. A2: alternative recipes, cycles, output volumes, rounding and commonly used own stock. A4: Save is not blocked during storage; incomplete reading time is read again, last valid state remains visible; Check quest progress against known game situations. A4 additional: incorrect HMAC, unknown save version and incomplete file copy must be detected as invalid reading, not as 0% progress. A3: calibrate transform from several known points, check at independent points; missing position/collection states visible as unknown.
