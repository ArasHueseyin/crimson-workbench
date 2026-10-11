# Crimson Workbench – Progress

## 11 October 2026 — v0.6.3 English interface and automatic language

Translated the desktop interface, user-facing backend messages, installer and project Markdown documentation into English. Credits retain Tommy Tran's upstream crimson-rs copyright and explain its relationship to the original Workbench code.

The new `cd_core::language` resolver reads only the selected game's neighboring Steam manifest. It validates app ID and installation folder, prefers the installed language, maps all 15 catalog languages and falls back to English for unavailable or invalid settings. New preferences use `auto`; existing manual choices remain unchanged. The UI reports the language source and retains an optional manual selector.

Local validation passed: 17 frontend unit tests, 92 browser tests, 7 app library tests, 171 core library tests and 36 format library tests. Five fixture-dependent Rust tests were skipped. The final three setup tests also passed after correcting the Automatic option label. Production frontend build and Clippy with warnings denied passed. Documentation review preserved technical hashes, commands and link targets except for intentional translated placeholders, comments and updated release links. `.local` remains ignored and is not part of the PR or installer.

Status: 23.09.2026. **Phase 5: App v0.5.9; Repair adapter as development module 0.23.0. General NPC respawn times and free repair remain technically open.** Phase 4 is developed in its documented scope. Relevant remains the updated `SPEC.md`; Game acceptances are set back on user request. The repair development does not change the ongoing game or saves. The separate, explicitly mandated offline inventory order was executed when the game was closed; See your own entry below.

**User decision from 20.09.2026:** Manuals in-game tests are made only after completion of the development of all phases. Open in-game acceptances are conducted separately and do not block further development progress. Automated tests and protection tests remain necessary; A postponed acceptance shall not be deemed to have passed. During play, no change or lock continues to the current installation.

## Separate inventory order: offline completed

Slot 2 secured with the game closed and supplemented by 200 light/200 heavy copper bags as well as 30,000 silver. Four valid 100s stacks in free seats, unique IDs and adapted lobby counters. New money stock 30,231,01 Silver. Only two files changed, eight more unchanged. Modified files reopened and HMAC-checked; Display/use and re-saving in the game are unchecked. No general save writer in the app and no completion of phase 7. [Complete proof](../.local/inventory-currency/RESULTAT.md).

## Repair 0.23.0: Inventory package serializer and stream selection

- Two complete original functions integrated; own 0x300-byte-TLS context.
- 26 Direct cases for field widths, identifiers, receivers, stream selection and
Buffer/writer errors; Ten transactions with native client-Ack.
- No repair success in case of wrong actor, UID change, internal alignment error or
Missing/double delivery. The package does not contain item UID, base or order data. Successful pool branch and real transport remain open.
- MSVC Release `/W4 /WX`, nine CTest suites and 454 native scenarios passed:
4,812 Counted calls, 206,786 Conditions. App and production suites unchanged.
- player/manager/thread binding, actual server repair, storage,
Input, loader and B0 are still missing. [Evidence](../runtime/repair/INVENTORY_PACKET_2949.md).

## Repair 0.22.0: native preparation and compression

- Nine complete original functions for buffer preparation, allocation and
Compression added. Neighboring CRT thunks are privately bound without overlap.
- 66 new native cases: sizes up to 131,072 byte with/without compression, output
Reuse, error, 14 file helper integrations, and two repetitions. Independent LZ4 test decoder reconstructs the original bytes.
- Entries retrieved after an opening error are compressed again;
their size indication then refers to the already compressed content. New experiments must produce fresh raw data. No repetition released.
- MSVC Release `/W4 /WX` and nine CTest suites passed. Total 418 native
Scenarios, 4,289 Calls and 204,378 Conditions. App and production suites unchanged.
- Downstream processing remains a callback; integrity/encryption,
Complete serialization, queue/backup route, game host and final receipt are missing. [Evidence](../runtime/repair/SAVE_ENCODING_2949.md), `.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.

## Repair 0.21.0: original filewriter

- Three complete original functions for writing and cleanup included;
File operations and encoders are private callbacks. No save accesses.
- 34 new native cases check preparation/opening/writing errors, failed
Flush/Close calls, empty/large buffers, string variants, and both allocation modes. Eight of them connect helpers and original dispatchers via their own entry bridge.
- Failed flush/close calls do not prevent success or payload sharing;
The dispatcher can also delete Dirty. Therefore, no memory confirmation.
- MSVC Release `/W4 /WX` and nine CTest suites passed. Total 352 native
Scenarios, 4,095 Calls and 142,649 Conditions. App and production suites unchanged.
- Real Encoder and WriteFile Wrapper statically further assigned. The Encoder
may replace the input buffer before the file is opened; his behavior, complete serialization, queue/backup path and graduation document remain open.
- Phase overview and test checklist updated. [Evidence](../runtime/repair/SAVE_FILE_2949.md),
`.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.

## Repair 0.20.0: Save dispatchers and queues

- Original dispatcher, Steam approval test and queue marker with six more
Code ranges are executed on private objects. 39 new cases test skip/status combinations, four error levels, repetition, quota limits and queue growth.
- Nine CTest suites and MSVC release `/W4 /WX` passed. Total 318 native
Scenarios, 4,039 Calls and 141,411 Conditions; Production suites unchanged.
- Concrete file backend type, three file methods and three timers statically assigned.
The original file helper checks Open/Write but ignores the result of `FlushFileBuffers`. Its native testing and actual persistence are lacking.
- File and platform recipients are their own test callbacks. change of status; and
Depleted queues do not confirm the termination of data carriers. Server-side inventory reports, full game connection and installation remain open.
- App v0.5.9, installation and saves unchanged. Phase overview and test checklist
updated. [Evidence](../runtime/repair/SAVE_BACKEND_2949.md), `.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.

## Repair 0.19.0: native item/storage object conversion

- 24 additional full original functions pinned; Converter, construction,
Vector growth, reset and destruction performed on own objects.
- 38 new native cases in both allocation modes. Including 14 repair plan/
Post-copy integrations, main value 0/damaged/full/sentinel, no-wear, free/partially occupied sockets and three dye data sets. Reuse removes old identities, sockets and dye data; Sources remain unchanged.
- Loader normalizes main values and generates five base data sets. His private
Policy values are not equated with actually running game settings.
- MSVC release `/W4 /WX`, nine CTest suites and a total of 279 native scenarios,
3,950 Calls and 139,784 Conditions passed. Production suites unchanged.
- updated phase overview and test checklist; later in-game test R14 supplemented
Color data retention after repair/reloading. Regular save completion, additional optional data, full events and game host remain open.
- App v0.5.9, installation and saves unchanged. No repair clearance.
[Evidence](../runtime/repair/ITEM_SAVE_2949.md), `.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`.
- Superordinate component collector and backend distributor then statically
assigned: six full functional hashes, three VTables, three jumps and 14 instruction anchors. The backend distributor can also deliver true if work is skipped. No further native success or memory completion counted.

## Repair 0.18.0: SQL success is not a memory confirmation

- Complete SQL execution shim with all five wind fragments as well as
the original request purchase is pinned and executed on private data. The bypass provides 0 without request access; the other branch sets status fields, controls diagnostic outputs and delivers 1. No database call.
- 19 new native scenarios check deadlines, old errors, zero pointers and
Connect with the original equipment slot consumer. A successful status with an already emptied list still does not confirm persistence.
- Nine CTest suites and MSVC release `/W4 /WX` passed. Total: 241 native
Scenarios, 3,674 Calls, 128,415 Conditions; Production suites unchanged.
- Downstream Result Processing Calls at Switch 1 A virtual
the transaction method; the concrete `ItemPopInventory` also only reports success there. The actual game switch value is unknown.
- Separate item/storage object converter associated with opposite path: main value
and base durability are transferred. Seven functional hashes, four RTTI types and 15 instruction anchors tested statically. Native transformation samples and actual storage are still pending. [Static detection](../runtime/repair/ITEM_SAVE_2949.md).
- Phase status and test checklist updated. R06 requires documented
Values before/after repair and after regular restart for main item and base. App, installation and saves unchanged; Phase 5 remains open.
- [Evidence](../runtime/repair/SQL_DISPATCH_2949.md),
`.local/repair-runtime-v18-{build-test.log,native-result.json,validation.json}`.

## Repair 0.17.0: original inventory client ack

- Original client feedback for items with private inventory sources
and the common transaction adapter. The Ack sets the main value absolutely; Bases remain unchanged. Formatting under lock and UI call checked after release with owner references still held.
- **36 new native cases:** 28 Direct cases and eight integration cases. Invalidity
Slots, exchanged UID, missing/double delivery and completion error checked. The Ack itself does not check either UID or target value approval. The outer packet distributor ignores its error code; This external status must not confirm any success.
- **Tested:** nine CTest suites, MSVC release `/W4 /WX`; Total 222 native
Scenarios, 3,526 Calls and 125,524 Conditions. Existing production suites unchanged. Name, UI, inventory server and persistence are test dependencies.
- Socket-SQL-Helper and both callers assigned reading only: saved
an ItemInfo identifier; the durability is separate and is only set in the subsequent memory image. No occupied persistence path for base durability.
- **Open:**Inventory server path, actual persistence/load semantics, real
UI/effects/transport/feedbacks, host, input and loader/B0. Phase 5 remains open.
- [Evidence](../runtime/repair/INVENTORY_EVENTS_2949.md),
`.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`. phase overview/test checklist updated; Desktop app, game installation and saves unchanged.

## Repair 0.16.0: Slot processing and batch completion

- The transaction adapter requires a final call after all
item messages. Lack of commitment prevents any writing; Later errors result in an unclear order without repetition. References and copies remain valid during completion, owner locks are already released.
- Original equipment slot consumer included. He collects UID and
Main durability, empties the marks and then transfers the job to the persistence interface. This interface remains artificial in the test.
- Replace Original Clear with Reuse and Final Table Sharing
manual cleaning of private test components. Both allocation paths are tested.
- **Tested:** nine CTest suites, MSVC release `/W4 /WX`; 43 Processing scenarios with
273 Conditions, 24 new consumer cases and 18 integrated equipment cases. Overall: 186 native scenarios, 2,826 calls and 124,527 conditions; Many of them repeated table checks. Mistakes after successful client ack do not produce success and no repetition despite an already empty slot list.
- **Open:** Actual persistence and supplementary base path – the investigated
Consumers do not transmit base data records. Concrete inventory events, real effects/transport/UI, feedback allocation, host, input and loader/B0 are missing.
- Phase overview and test checklist updated. R06 also requires normal
Quit/restart and re-compare the main item and each base. App v0.5.9 unchanged; No game process/save access, no installation change.
- [Evidence](../runtime/repair/PERSISTENCE_2949.md),
`.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`.

## Repair 0.15.0: prepared copies and common process

- New library `crimson_repair_transaction`: check all message paths in advance,
Prepare both native post-copies of each item, then perform field step, mark, unlock, and message submission. References remain valid until the end; the order then waits for fresh data and associated feedback.
- Additional data before native copy calls to readability and allocation budget
check. Partial errors lead to an unclear result without repetition.
- Storage base number and logical space boundary separately; Free spaces
Do not need a read data set. Interchanged additional vector strides corrected in the overlap test and tested with targeted regression.
- The 15 native equipment cases use the common adapter. Originals
Copies, writers, slot marking and server/client events connected; Confirmation reads under newly acquired owners. Recipients remain test callbacks.
- **Tested:** nine CTest suites, MSVC release `/W4 /WX`; 40 Processing scenarios with
247 Conditions, 35 Writing scenarios with 202 Conditions, 32 native item cases. Overall: 159 native scenarios, 2,023 calls, 119,953 conditions. Counter changes are documented by the replaced manual body and unstored socket.
- **Open:** concrete inventory events, slot consumers/cleaning, real
Effect/transport/UI processing and feedback assignment, host, input, loader/B0.
- Phase overview and test checklist are supplemented, in particular R13 for free base slots.
App v0.5.9 unchanged; No game process/save access, no installation change.
- [Evidence](../runtime/repair/TRANSACTION_2949.md),
`.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`.

## Repair 0.14.0: Field recorder for complete repair batches

- New library `crimson_repair_writer`: fresh collection under held
Owner lock, full pre-check and actual writing of the changed durability words in inventory/equipment and both Zustandcopies.
- addresses originate from the collection; impermissible capacity, storage overlap,
Indescribable fields and an outdated later entry will stop the entire batch before the first store. Unchanged words are not written.
- The complete reading stock is compared before/after, including empty
Slots and metadata. partial errors give an unclear result without automatic repetition; Obsolete prior data are hidden after writing has begun.
- **Tested:** eight CTest suites, MSVC release `/W4 /WX`; New Writing Suite with
34 Scenarios/197 Conditions, including 2,048 Items and 4,096 Stores. Eight new native integration cases with actual private storage addresses and original reference/lock management. Overall: 141 native scenarios, 1,408 calls and 120,069 conditions.
- **Open:** Field step with pre-prepared native post-copies and
Connect complete inventory/equipment events. Slot consumers, actual effects/transport/UI, host, input, loader and B0 are still missing.
- updated phase overview and test checklist; App v0.5.9 unchanged.
No game process/save accesses or write operations to the installation.
- [Evidence](../runtime/repair/FIELD_WRITER_2949.md),
`.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`.

## Repair 0.13.0: Slot marking and continuous capture

- `HeldCapture` holds both references and owner locks after capture for
Planning/pre-examination. Refresh re-checks the current session and all reading data; Errors discard old results and return locks before references.
- Original marking of modified equipment slots fully executed,
including location search, collisions, growth and bucket chaining. Both TLS allocation pathways are tested on a private heap with 32 byte alignment.
- The 15 equipment event cases now mark under both locks before the
Notification runs on the same private component after unlocking. The own item copy and actual actor references are preserved.
- **Tested:** seven CTest Suites, MSVC Release `/W4 /WX`, 519 Reader Conditions;
133 native scenarios, 1,296 calls and 119,997 conditions. The high counter contains repeated complete table checks after individual insertions.
- **Open:** complete native writer, consumer/cleaning of
Slot messages, inventory events and real effect/transport processing, player/manager/thread/world binding, and input, loader and B0 installation.
- updated phase overview and test checklist; R12 complements several repaired
Equipment slots. No game process/save accesses or installation change.
- [Evidence](../runtime/repair/DIRTY_SLOTS_2949.md),
`.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`.

## Repair 0.12.0: own native copies of items and use them for events

- New library `crimson_repair_item`: construction, deep allocation and
Release bound per object, without copyable/displaceable owner. Wrong thread, invalid headers and missing bindings are rejected; Failed native returns do not provide a usable item value.
- Original item methods and all list/allocation helpers used
carried out on private sources. Own socket, list and optional data copies remain independent of the source; All buffers are released.
- All 15 event tests use the native aftercopy: Copy under the
Owner locks, notification after their release, then destruction.
- **Tested:** seven CTest suites, MSVC `/W4 /WX`, 30 new owner conditions;
117 native scenarios, 851 calls and 13,639 conditions. 45 code ranges, 49 pdata fragments and 24 private TLS redirects. Both allocation paths, growth, empty data, self-assignment and rejection of invalid capacity are tested.
- **Open:** full write path, update of changed slots,
Inventory events, actual effect/transport processing, host and installation. The private heap is still a test dependency. The slot method has been statically further narrowed down, not yet implemented or executed.
- phase status/test checklist updated; R11 complements different examples
The same item. App, game installation and saves remain unchanged.
- [Evidence](../runtime/repair/ITEM_LIFECYCLE_2949.md),
`.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`.

## Repair 0.11.0: Prepare equipment events and check them together

- Event data from the immutable repair plan: exact identity/session,
previous and subsequent Broken-Zustand. Active no-wear override is considered; Items and false slots will be rejected.
- Original server notifier and current client with own test objects
connected. Native references are retained; Both owner locks are released before the events. Delta 0 gets correctly repaired base values.
- 15 new native cases prove effect/child/UI call decisions and
deny sham successes in case of partial change, missing/false/double feedback, missing slot, event document or session change.
- **Tested:** six CTest suites, MSVC release `/W4 /WX`, 1,537 action conditions;
Total 103 native scenarios, 758 calls and 9,311 conditions. 28 code ranges, 25 pdata fragments and eight private TLS redirects.
- **Limit:** True effect processing, transport and UI remain fixture receivers.
Engine writing/item copy, slot invalidation, inventory events, host, input, loader and B0 are missing. No installable repair mod claims.
- updated phase status and test checklist; R10 complements the direct comparison
Broken/damaged items, sockets and no-wear for immediately correct effects. App v0.5.9 remains unchanged; no game process/save accesses or playwrite accesses.
- [Evidence](../runtime/repair/EQUIPMENT_EVENTS_2949.md),
`.local/repair-runtime-v11-{build-test.log,native-result.json,validation.json}`.

## Repair 0.10.0: Reading fields and accesses confirmed on the new build

- Own 2949 proofs for inventory slots, Holder/Possessor resolution, current
Client selection, equipment slots and base panels. The original getters and the reader agree on identical private sources.
- The reader expressly requests a known build. Not settled and
Unknown selectors fail before acquiring references, blocking and memory reading.
- Original equipment path with Delta 0 uses real native lock references and
Exclusive Windows locks. Logged definition queries prove the actual choice of the second matching slot. Non-zero delta events are not considered proven by this layout test.
- Positive main-item repair delta continues to use in the original updateer
damaged base on 0. This path remains unsuitable for own repair; The result is now natively reproduced on the new build.
- **Tested:** six CTest Suites, MSVC Release `/W4 /WX`, 385 Reader Conditions;
88 native scenarios, 578 calls, 6,219 conditions. Full EXE hash, 24 code ranges, 20 pdata fragments; six targeted private TLS diversions.
- **Open:** actual manager/player/engine thread binding, world change,
Complete repair changes and events/effects, input, loader, B0. Table resolvers, empty temporary item construction/destruction and parts of lifetime management are expressly limited test dependencies.
- App v0.5.9 unchanged; Game process and saves not used, installation not
changed. [Evidence](../runtime/repair/INVENTORY_2949.md), `.local/repair-runtime-v10-{build-test.log,native-result.json,reader-result.json,validation.json}`. updated phase status and test checklist; No early in-game validation.

## Repair 0.9.0: Record owners directly

- A second unprotected server registry search is removed from the protected reader.
Only the already held pair is read; Other client selections are rejected before they are dereferenced. Full identifier and living owners are now also mandatory in the pure blocking path.
- Common native sample around the original WindowsRWLock-Try path of 2949
Extended. Actual registry/actuator references and owner locks protect inventory, equipment and socket capture, including control reading.
- **Tested:** six CTest Suites, MSVC Release `/W4 /WX`, 362 Reader Conditions;
48 native scenarios, 503 calls and 3,867 conditions. Blocked owners, changed selection/identifier, replaced registry nodes, errors and sharing order.
- **Detection Limit:** The combined test used for inventory/client anchor
private fixtures of the previous reader. New occupational/blocking functions are natively tested; This does not result in live approval of this inventory layout on 2949. Manager lifetime, actual engine dispatch/world change, transaction and events, input, loader and B0 build-in remain open.
- Workbench v0.5.9 remains unchanged; no game process/save access, or
Installation changes. in-game tests continues to be deferred on user request.
- [Evidence of integration](../runtime/repair/PINNED_CAPTURE_INTEGRATION.md),
`.local/repair-runtime-v9-{build-test.log,native-result.json,reader-result.json,validation.json}`. [Phasenstatus](../PHASENSTATUS.md) and [Testcheckliste](../TESTCHECKLISTE.md) updated, including protagonist/accompany demarcation and reloading.

## Repair 0.8.0: protected registry/reference purchase on the new build

- Completely pinned native sample for EXE 1.0.0.2949.
It performs client/server lookups, specific normal/user reference methods and Actor/PaPtr sharing together via the existing source adapter.
- Original Lock Methods Use Real Windows SRW Locks on Private
Registry objects. references are acquired before unlocking; Failed second searches return the first possession. A competing write lock stops the lookup; Subsequently removed entries are no longer accepted.
- **Tested:** six CTest suites, MSVC release `/W4 /WX`; 30 native scenarios,
263 native calls and 3,490 conditions. Direct/registered reference modes, normal/user parameters, obsolete generation identifier, invalid receipts with pointer, nested possession, delayed cleanup and destruction are recorded.
- **Separate shares:** old native host still refuses 1.0.0.2949;
New registry host refuses unknown EXEs. No transfer of old addresses.
- **Remain open:** current player/manager resolution, lifetime of managers,
Engine thread/TLS binding, additional inventory/modification/event methods, complete repair transaction, input, loader and B0 installation. Threadmap, higher-level references and final cleanup remain explicit test dependencies. The app repair is still locked.
- App v0.5.9 and desktop shortcut unchanged. No new Rust/front end test
claims; These proofs are available in the following app step. No change to the current game, saves or installation.
- [Evidence of integration](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md),
`.local/repair-runtime-v8-{build-test.log,native-result.json,validation.json}`. [Phasenstatus](../PHASENSTATUS.md) and [Testcheckliste](../TESTCHECKLISTE.md) updated.

## Game Update: Table Support and Desktop v0.5.9

- Steam 25455892 / EXE 1.0.0.2949 additionally supported; The previous build
Stay independent in the code. 61 compared files used, of which 57 unchanged. Stage/quest changes are structurally and dataset-wise demonstrated.
- All 14 indexed table pairs reconstruct exactly their original bytes.
The two new stages will be preserved in mod overlays; Only the two chosen patrols will be changed. New status: 52,082 Stages, 1,098 Quests, still 6,816 Items and 2,069 Skills.
- Schema ID for index and export is chosen appropriately. Proofs of security shall be:
Make a complete build and identity; Mixed evidence was rejected. Unknown versions lead directly to the version error instead of a missing project sample backup. An own PE test installation confirms this rejection.
- **Tested:** 181 Rust Tests plus a separate Advanced table test,
seven front-end tests, 61 UI flows, clippy and format checking. The real release app checks 3,359 changes over 13 tables and two apply/restore cycles in the project.
- **Provided:** `target/release/crimson-workbench-0.5.9.exe`. A meanwhile
opened Workbench prevents replacing the previous filename; the finished release artifact was tested and provided separately. Desktop link and start script point to v0.5.9. The existing window remains open.
- **Unchanged:** running game, original audit and the 38 recorded
EXE/metadata files. No B0 device, no savewrite operations.
- **Remain open:** native repair connection, general NPC respaw times and
the other specified phase 5 limits. Eight native VTables for the new build are now once again purely read; This is not yet a native execution release.
- [Proof of build](BUILD_SUPPORT.md), [Test test](TESTING.md),
[Updated phase overview](../PHASENSTATUS.md), [later test cases with expectations](../TESTCHECKLISTE.md).

## Registry adapter 0.7.0 and game update (previous app status)

- Registry source adapter implemented: full identifier to the lookup,
Keep native receipts and release them for each source using the appropriate method. Source release now gets its own context. No global method binding.
- **Tested:** five CTest suites, MSVC release `/W4 /WX`; 55 new registry conditions,
249 Reader/Reference Conditions, 25 Lock Group Conditions, 1,481 Action Conditions. The registry tests are synthetic, not a new native test of success.
- **Update detected:** Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. 24 changed from 38 previously captured EXE/metadata files. The new `docs/builds/steam-25455892.observed.json` report is explicitly not a schema release.
- Previous native host refuses the new EXE before function execution;
CLI fingerprint reports the build as unknown. 203 native calls/3,859 Conditions remain historically limited to 1.0.0.2944. A separate table call stops even in the absence of project sample backup and does not count as a matching build error message.
- **Remain open:** new build/table credentials, current player/manager and
Engine thread/TLS connection, merged native registry sample, repair transaction including events, input, loader and B0 installation.
- App v0.5.8 and original audit unchanged; current 38 files since
new observation again unchanged. No game process/save access or writing into the installation through this work.
- [Registry contract](../runtime/repair/REGISTRY_INTEGRATION.md),
`.local/repair-runtime-v7-validation.json`.
- New: [TESTCHECKLISTE.md](../TESTCHECKLISTE.md) contains cases/expectations
Phases, technical checks and separate results protocol. Your in-game tests will remain deferred until all stages of development have been completed.

## Repair: Object references and capture 0.6.0 (previous status)

- Non-copiable native reference documents for both owners and balanced
Release implemented. Failed acquisition also discards an already held first reference. An invalid document is not considered possession despite pointers.
- Reader holds references on complete capture and gives locks first
back. Full identifier, Alive/Destruction state are checked before/after under lock; Deregistered owners will be rejected.
- Native PaPtr release and concrete actor override tested. The basic path is
no replacement: The actual override can additionally trigger a pending Zustandadjustment. Direct acquisition can be successful despite Alive=0; The reader rejection of this case is tested.
- **Tested:** four CTest Suites, MSVC Release `/W4 /WX`, 249 Reader/Reference Conditions,
25 Lock Group Conditions, 1,481 Action Conditions; 203 native calls and 3,859 conditions. Threadmap, embedded locks and cleanup dependencies are fixture callbacks; No full engine lifetime proof is claimed.
- **Remain open:** Purchase of current player references from protected live sources,
Concrete host/TLS binding, complete repair transaction with event allocation, input, loader and B0 installation. Phase 5 remains open.
- App v0.5.8, original audit and 38 installation hashes unchanged.
No game process/save access. [Reference contract](../runtime/repair/REFERENCE_INTEGRATION.md), `.local/repair-runtime-v6-validation.json`.
- New: [PHASENSTATUS.md](../PHASENSTATUS.md) sums up implementation and remaining points
the phases 0–8 and the optional farm hotkey together.

## Repair: owner lock and capture 0.5.0 (previous status)

- Common, non-waiting blocking group for client/servers is implemented.
In case of conflict, a previously acquired block is released. bindings are fully pre-tested; Recursion, order and thread membership checked.
- Inventory readers collect and control the data under both locks;
Changed owner/lock pointers and invalid data discard the result. The returned copy does not have a persistent write authorization.
- Original try/release methods in your own test process with real Windows
SRW locks tested, including competing threads. A TLS read access of the private try copy is redirected to data per test thread.
- **Tested:** four CTest suites, MSVC release `/W4 /WX`, 1,481 action conditions,
163 Reader conditions, 25 Group conditions; 140 native calls with 2,408 conditions. App v0.5.8, Audit and all 38 captured installation files unchanged.
- **Remain open:** independently secured lifetime of the game objects, complete
Change transaction including event allocation, input, loader and B0 installation. No game process/save access. Phase 5 remains open.
- [Evidence and integration agreement](../runtime/repair/LOCK_INTEGRATION.md),
`.local/repair-runtime-v5-validation.json`.

## Repair: confirmed order conclusion 0.4.0 (previous status)

- Asynchronous acceptance and confirmed closure implemented separately. The
Order management keeps the plan and does not report success until confirmation. Order number plus original session, freshly recorded post values of both representations and confirmed notifications are required.
- Old/third-party feedback is rejected; No second order, cancellation
or commit again during the waiting period. Partial change, world change and five seconds without confirmation permanently lock the queue. No reset or automatic repetition after possible change.
- Original client-ack, lock-constructor and socket remover isolated tested.
The Ack first changes data and then compares the server list. It can remove bases before an error message; Even error code 0 does not prove a successful repair. These cases are reproducible regressions.
- **Checked:** 1,481 Action Conditions, 124 Reader Conditions, 122 native views
with 2,221 conditions; passed three CTest suites and MSVC release `/W4 /WX`. The client alignment uses an explicitly redirected TLS read access and artificial UI/Allocator/Lock dependencies; Not a real engine message.
- **Remain open:** real engine leases, complete change path and assignment
actual game events, operator input, loader and B0 installation. Phase 5 remains open; The manual in-game validation remains reset.
- App/link v0.5.8 unchanged. 38 EXE/metadata hashes and original
audit unchanged; No gaming process/save access or live installation.
- [Final management and supporting documents](../runtime/repair/ACK_INTEGRATION.md),
`.local/repair-runtime-v4-validation.json`.

## Repair: Reading adapter 0.3.0 (previous status)

- Current client/server structure connected: targeted search via the full
Character Identifier, Additional Owner Exams, Character Inventory and Equipment Only. Item IDs, positions and both Zustandcopies must fit together.
- Item/socket data is copied exclusively into private images.
Full control reading detects changed data and discards the output. Missing/ambiguous identifiers, invalid layouts and budget overruns are rejected. The reader does not replace an engine lock.
- Current inventory layout checked with native copies: slot limit `+0x0c`,
contiguous `0xc8` byte entries and exclusion lists. Readers and original deliver the same existing slots on the same fixtures. Native registry search including access rejection checked; Real lock/lease methods are stubs.
- **Checked:** 124 Reader Conditions, 1,377 Action Conditions, 116 native views
with 1,387 conditions. Passed three CTest suites and MSVC release `/W4 /WX`; Also 2,048 Items and the entire rejection of a too large batch checked.
- **Remain technically open:** real engine access releases, confirmed
Change/notification transaction, operation, loader and B0 installation. The native registry return pointer alone is not a valid proof of access.
- App/link remains v0.5.8; Repair is not yet activated. 38 EXE-/
Metadata hashes, app and original audit unchanged. No access to game process or saves; shifted in-game tests and phase-5 remaining points remain open.
- [Readers and layout documents](../runtime/repair/READER_LAYOUT.md),
[Reproduction](../runtime/repair/README.md), `.local/repair-runtime-v3-validation.json`.

## Own repair action: Development module 0.2.0 (previous status)

- User decision: own repair action for still existing,
Complete damaged items. Even previously not repairable items via game rules are thus in scope; No recovery of lost items.
- Developed own planning and application to private item images: individual items,
Inventory, equipment or both; Main and base durability independent. Needs neither the empty item repair lists nor repair materials.
- UID/position check, comparison of both Zustandcopies, complete batch
Pre-testing and writing limitation on durability words. Combination with the well-known no-wear override repaired to the verified vanilla maximum.
- queue against duplicate input, wrong threads and delayed execution;
Change of sessions discards old orders. Unclear commit blocks further orders.
- **Tested:** 1,377 Conditions for the action; 83 native calls with 1,252
Conditions, two CTest suites, MSVC Release `/W4 /WX`. The real updater reads the repaired main/socket values correctly. Its positive delta path is unsuitable for RepairAll; the error case is reproduced in isolation.
- **Not yet usable in the game:** Player resolver, complete engine-
Transaction/notifications, operation, loader and B0 installation are missing. The existing backend works exclusively with its own memory images. The native sample proves field use, not actual game persistence.
- No game process/save access. desktop app unchanged v0.5.8; None
Installation, no full phase 5 release. in-game tests remain shifted.
- [Action and integration status](../runtime/repair/ACTION_INTEGRATION.md),
[Tests](../runtime/repair/README.md), `.local/repair-runtime-v2-validation.json`.

## Released repair runtime development: prototype 0.1.0 (previous status)

- The current “go” gives the separate runtime development for free
Repair free. No re-consent is required. in-game tests remain shifted; Farm mode hotkey and phase 6 were not started.
- Developed own C++ module and native test environment under `runtime/repair/`.
Coordinated, hash-tested changes to both repair helpers: cost 0 without material division/limitation; no additional material application. Errors, valid quantity calculation, maximum durability and existing orders checked.
- 79 native function calls existed in private copies, including
unchanged prepare/commit functions with empty material list and restore. 1,236 Test conditions, CTest contract test, three Python regressions and two existing Rust repair locks passed. MSVC Release `/W4 /WX` green.
- Audit improved: 16 direct references including the previously overlooked
joint call for costs. No statement about indirect calls.
- **No installable repair mod yet:** Real repair lists are empty;
Upstream material selection, further UI/RepairAll paths, loaders and B0 installation are missing. Positive tests use artificial repair rules. The actual stored durability of a game item has not been changed.
- 38 EXE/metadata hashes and original audit unchanged. No access to
the gaming process or saves, not a live application. Desktop app remains v0.5.8; Your repair switch remains locked. No full phase 5 release.
- [Prototype and reproduction](../runtime/repair/README.md),
[Technical evidence](research/REPAIR_RUNTIME.md), `.local/repair-runtime-v1-validation.json`.

## Phase 5: v0.5.8 – additional self-consumption costs in buffs and skills (app stand)

- Own full reader for all 292 BuffInfo records, including
the nested skill and resource lists of buff type 114.
- 36 additional self-consumption costs in 33 equipment buffs separately for endurance and
Spirit controllable. Preview, individual exceptions, export and B0 integrated.
- 31 Spirit consumption values in `Active_UseResource_Mp` and
`Skill_ElementalReinforce_UseMp` about "Further Skills". Explicit buff changes have priority; Original → final value appears once in the diff.
- Zero-cost switches set all seven categories of the respective resource.
Positive regeneration, enemy drain effects, buff times, limits, target skill references and regeneration flags are maintained.
- **Tested:**179Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 61 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release green. Final EXE: 3,359 Changes over 13 tables; two apply/restore cycles on the project copy including BuffInfo. Passed Reapply, Recovery, launch protection and Update Rejection; new view visually checked.
- 38 EXE/metadata hashes and original report unchanged. Desktop
Link points to v0.5.8; End your own test instance.
- EXE: 16,156,160 Bytes, SHA-256 `b32583fbf48680d66519fda761185a8a8ca7d18a1235186aa5d245a1a4ae5dbb`.
Evidence: `.local/phase5-v9-validation.json`, [Test plan](TESTING.md).
- Repair caller tested: The special quantity value -1 is in the normal
Material path not reachable because the callers reject quantities <= 0. No secure table path for free repair has been proven. The initial requested approval for maturity development has now been granted; The current prototype see above.
- Further spawn research: `IsOverMiseensceneSpawnableTime` checks time window
of an NPC plan, no general respawn distance. No change to that.
- Phase 5 remains open; Phase 6 and runtime phase D not started. in-game tests
remain postponed on user request, current game and saves unaffected.

## Phase 5: v0.5.7 – Durability, storage and testing of all remaining points (previous status)

- “No durability loss” combines the EquipType factor with the confirmed
Engine Sentinel for all 122 items with finite durability. No save migration or recovery of consumed items; Repair remains locked.
- Kuku and five housing warehouses added: now nine editable slot areas.
Twenty inventory prefixes read structurally, complete suffix check of all 21 records; the character condition tree is retained hash-bound.
- Re-matched all B4–B11 requirements. [Restpunktmatrix](PHASE5_REMAINING.md)
differentiates between implemented functions, technical gaps and game acceptances. General NPC respawns, free repair and universal engine/save maxima are still unoccupied; No full phase 5 termination is claimed.
- **Tested:**176Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 60 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. Final EXE: 3,323 Changes over twelve tables; Two apply/restore cycles on the project copy, including all new stocks and 122 item durability values. Reapply/Recovery/launch protection/Update rejection passed. New views visually checked at 1,024 pixels; No browser errors.
- 38 EXE/metadata hashes and original test report unchanged. No mods
Applied live. Desktop shortcut points to v0.5.7, own test instance terminated.
- EXE: 16,133,632 Bytes, SHA-256 `248109d7c629b658109303f39dd11955e8eb719682be5225d08289e27543fd7e`.
Evidence: `.local/phase5-v8-validation.json`, [Test plan](TESTING.md).
- Phase 6 and runtime phase D not started. Manual tests on user request
shifted. The ongoing installation and save contents remain unaffected.

## Phase 5: v0.5.6 – Skill baseline data and complete structural reader (previous status)

- All 2,069 skills now show additional 87,697 named header/suffix fields:
Requirements, resources, references, flags and texts. Own search by field names or values; References/unclear values remain read-only. Existing skill and buff changes remain compatible and are not discarded during filtering. Original values are marked as such.
- The complete skill reader follows the occupied structure. So far, they have
used search for repeated key bytes is omitted. `skillGroupKey` is a group reference and is no longer equated with the record ID. Invalid matrices are rejected without search fallback.
- All 2,069 complete skill records from displayed fields, matrix counts
and buff values are reconstructed with byte precision. New regressions for deviating group references, identifier bytes in text, resource lists, and read errors.
- **Tested:**175Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 59 UI flows; Fmt, Clippy `-D warnings`, TypeScript and release build green. Finished EXE: 3,189 Changes over twelve tables with export and two apply/restore cycles on one project copy; Reapply, Recovery, launch protection and Update Rejection passed. Basic data view at 1,024 pixels and finished preview visually checked; No browser errors.
- 38 EXE/metadata hashes and original test report unchanged. No mods
live applied or saved content read. Own invisible test instance terminated. The existing desktop shortcut points to v0.5.6.
- EXE: 16,122,368 Bytes, SHA-256
`61637a148446e1d0c49bcd41bbb7a81264af8fc634ea16c5334343d3887028f2`. Evidence: `.local/phase5-v7-validation.json`, [Test plan](TESTING.md), [Field evidence](research/ADVANCED_TABLES.md).
- Further open: general NPC respawn timer, free repair,
Universal engine/save maxima and unknown skill meanings/units. No new editing release for references or unconfirmed number formats. Phase 5 remains partially implemented; Phase 6 and runtime phase D not started. Manuals in-game tests remain deferred upon user request.

## Phase 5: v0.5.5 – repair cost path checked and zero setting error removed (previous status)

- The common and server-side repair path uses the material costs
as a divisor. Zeroing is therefore not a suitable way to free repair. The previous general Writer branch has been removed. It was already blocked by Core in the currently empty repair lists of the supported build.
- Global and individual repair flags will now always and above all
Advanced changes are rejected, even for synthetically existing rules or direct API calls. The low format API also refuses the process. The number of read rules is no longer considered to be a release for the switch.
- Old templates remain readable. An active repair option can be
targeted removal; Stats, buffs, stack size and Enchant copies are retained. The separate wear regulator functions unchanged.
- Another spawn interval candidate belongs to timeline summoning events;
there is no general NPC respawn coverage. Free repair, general NPC respawns, universal engine/save maxima, and unknown skill meanings remain open. Phase 6 and runtime phase D were not started.
- **Tested:**173Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 58 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. Finished EXE: global and individual repair requests explicitly rejected; 3,189 regular changes to the project copy via twelve tables with export and two apply/restore cycles. Reapply, Recovery, launch protection and Update Rejection passed.
- Repair view at 1,024 pixels and finished preview visually checked.
No browser errors; 38 EXE/metadata hashes and original test report unchanged. No mods applied live and no save content read. Own invisible test instance terminated; existing desktop shortcut points to v0.5.5.
- EXE: 16,109,056 Bytes, SHA-256
`553b18a22af5c0092b57db2b10671a418ac8dc7b64423f32ebd5babe577925a6`. Detection: `.local/phase5-v6-validation.json`, [Test plan](TESTING.md), [Field evidence](research/ADVANCED_TABLES.md). Manuals in-game tests remain deferred as agreed.

## Phase 5: v0.5.4 – further skill payloads and field search (previous status)

- All nine summon payloads (also several per skill) are placed in named fields.
split: character/area references, spawn details, docking options, formation and other structure values. The AddSubLevel payload shows its reference and the following unknown four-byte field separated.
- Field names are searchable in the Skill browser; Breaking long names.
The new fields remain write-protected, since name and byte width alone do not prove editing rules or units. Existing buff changes use the same paths unchanged, replacing eight bytes each.
- NPC research: `_respawnTimeSecond` belongs to Save-Zustandsmetadata;
the selectively read NPC spawn XML does not contain a timer. No save contents read and no table controller derived therefrom. Repair cost source outside the empty item lists still not proven.
- **Tested:**171Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 57 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. The finished EXE checks all ten new payloads via IPC, field name search and protected references as well as 3,189 changes to the project copy via twelve tables with export and two apply/restore cycles. Reapply, Recovery, launch protection and Update Rejection passed.
- New detail view at 1,024 pixels and finished preview visually checked;
No horizontal overflow, no browser errors. 38 EXE/metadata hashes and original test report unchanged. No live application or save content read. Own invisible test instance terminated; existing desktop shortcut points to v0.5.4.
- EXE: 16,108,544 Bytes, SHA-256
`ca9cc071204b5ecdab794597bc0cd891a86faaba2a3f5b3b11071aafd2b2657e`. Detection: `.local/phase5-v5-validation.json`, [Test plan](TESTING.md), [Field evidence](research/ADVANCED_TABLES.md).
- Phase 5 remains partially implemented; general NPC respawn timers;
Repair cost paths, universal engine/save maxima and unknown skill meanings remain open. Manual in-game tests remains deferred. Phase 6 and runtime phase D were not started.

## Phase 5: v0.5.3 – Territory reoccupation and input limits (previous status)

- 108 Re-occupation rules for 125 linked faction areas: own
Percentage factor and individual exceptions in the world browser. Only `delayTime` changes; Quest conditions, probabilities, other quest timers and area links remain byte-identical. No general NPC respawn coverage or already tested gaming effect is claimed.
- Inventory and both warehouses for the time being allow a maximum of 1,460 starting/maximum places.
This standard lid was detected in an inventory code path of the pinned EXE. Further engine/save limits and possible configuration deviations remain unchecked; 65,535 is no longer offered as permissible input.
- Invalid visible numbers block export/workspace rehearsal and discard a
existing live file sharing, even if internally the last valid value is still available. After correction, the old live release does not return automatically.
- **Tested:**169Rust Tests, a separately executed hash tested table sample,
seven front-end unit tests and 56 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. Native invisible EXE test: **3,189 Changes over twelve tables**, 108 Territory replacement values including single exception, export and two application/restore cycles to the project copy. Reapply, Recovery, launch protection and Update Rejection passed. New worldview at 1,024 pixels as well as preview visually tested; No browser errors.
- play ran during the rehearsal; 38 EXE/metadata hashes and original
Test report unchanged. No live application, no save content read. Own invisible test instance terminated. Desktop shortcut points to v0.5.3.
- EXE: 16,115,200 Bytes, SHA-256
`77580d1346dcc82eddb9edc97be131c447c635d3069c6f690afccba4fb48014d`. Detection: `.local/phase5-v4-validation.json`, [Test plan](TESTING.md), [Operation](ADVANCED_MODS.md), [Field evidence](research/ADVANCED_TABLES.md).
- **Remain open:** general NPC respawn timers outside of these occupied
Patrol/territory rules, additional repair cost paths, universal stack/inventory/save maxima and unknown skill meanings/opaque payloads. Manual in-game tests remains deferred. Phase 6 and runtime phase D were not started.

## Phase 5: v0.5.2 – skill values, patrols and city flight (previous status)

- Edit confirmed i64 values in skill buff matrices individually, including:
of the three graph components. Types, references, counts, curve tags and opaque payloads remain protected. merging with cooldowns/costs in the same preview/B0 plan; In each case, exactly eight bytes per numerical value.
- Reset factor and individual second values for two occupied spawn patrols
(Hidden Estate and Watergate), each originally 259200. The other 52,078 stages remain unchanged; No reinterpretation of all quest resets or the already refuted spawn percentage field to NPC timers.
- The city/dragon option deactivates condition 1011130 for the
City flight descent. Current EXE serialization and false replacement condition checked separately. Other 10,797 conditions remain byte-identical. Common rules for flying mounts, hence impact on other aviators; Region list 79 remains the second targeted blocking path.
- **Still open development:** general NPC respawn timer, the actual
Source of further repair costs, engine maxima and unknown meanings and opaque skill payloads. No shutdown of quest/intersequence descents or maximum flight altitude claimed. No runtime implementation.
- **Tested:**167Rust Tests, additional hash-tested table sample,
seven front-end unit tests and 53 UI flows; Fmt, Clippy `-D warnings`, TypeScript and release build green. Invisible test of the finished EXE: 3,081 changes over eleven tables, export and two apply/restore cycles only on the project copy, reapply, recovery, launch protection and refusal to update. 38 EXE/metadata hashes and original test report unchanged; Game ran, no live application, no save content read.
- EXE v0.5.2: 16,107,520 Bytes, SHA-256
`1a5be87e93bdb15046f84b88e58a71058b830155d533519ebe50747a0f8127e4`. desktop link still valid; End your own invisible test instance. Detection: `.local/phase5-v3-validation.json`, [Test plan](TESTING.md), [Operation](ADVANCED_MODS.md), [Field evidence](research/ADVANCED_TABLES.md).

## Phase 5: v0.5.1 – Enchant lines and skill details (previous)

- copy new enchant levels from a selected original level, with
Full stats/buffs/prices, sorted inserts and new header offsets. Changes at new levels and reusable templates are available. A table level does not unlock an upgrade path and does not increase save values.
- All 2,069 skill matrices with 4,607 entries including zero are complete
demarcated. Detailed view with RTTI type names, structure/byte positions, raw values and full original bytes. Unknown meanings/units and two payload variants delimited only as raw block are expressly marked. Matrix editing is not released.
- Stack protection takes into account newly introduced buffs and enchant lines.
Double targets, interlinked sources and unconfirmed separation structures are rejected. Existing templates and old serialized plans remain compatible.
- A Respawn candidate from a reference surface was statically refuted:
The current serializer calls the relevant u64 `_spawnPercent`. It is not edited as a timer. No invented function as a substitute.
- **Remain open:** general NPC respawn times, all Dragon Dismount times
Paths, repair costs and full semantic skill buff processing. Engine maxima remain unproven. Phase 6 and runtime phase D have not begun.
- **Tested:** 162 Rust Tests, plus a local hash-tested table sample
with conversion of all matrix values into original bytes, seven front-end unit tests and 51 UI flows. Fmt, Clippy `-D warnings`, TypeScript and release build green. Native Invisible Test: 3,077 Changes across nine tables, new Enchant Level, Skill View, Export and two Apply/Restore cycles of project copies. 38 EXE/metadata files and the original test report unchanged; Game ran, no live application, no access to save content.
- EXE v0.5.1: 16,037,376 Bytes, SHA-256
`3e19c17e7a4f68e63cead2c5bf7810a670fa8d9b3febd9131dafa1c833f1a463`. The existing desktop link continues to lead to this EXE. Evidence: `.local/phase5-v2-validation.json`, [Test plan](TESTING.md), [Operation](ADVANCED_MODS.md), [Field evidence](research/ADVANCED_TABLES.md). Manuals in-game tests remain postponed and unpassed as agreed.

## Phase 5: v0.5.0 – occupied table functions (previous status)

- 2,811 editable/announceable records, including all 2,069 skills, and
Detailed editing of the 6,816 items. Spawn groups, city/mount flags, Blackstar, three inventories, stacks, equipment wear, skill cost/cooldowns, and stats/buffs on existing enchant levels. Named item templates are retained locally in app storage.
- Changes run through the same preview/B0 builder. Values are derived from
Originals rebuilt, single exceptions are deterministic, array inserts/removals receive new header offsets. Experimental stacks require the express switch.
- New limited readers are against the current EXE serializer and the
real, hash-tested tables tested; no transfer of obsolete field positions. Repair rules are empty in this build: locked option.
- **Open:** general NPC respawn times, all Dragon Dismount paths,
Repair costs, complete skill buff matrix and new Enchant lines. Engine maxima not occupied, file format/Workbenchlimits clearly separated. [Functional scope and operation](ADVANCED_MODS.md), [Technical research](research/ADVANCED_TABLES.md).
- Native test of the EXE v0.5.0: 3,076 Changes over nine tables,
Export and two apply/restore cycles exclusively in project copy, reapply, recovery, start/backup protection and refusal of update successful. 38 EXE/metadata files and original content report unchanged. the game was played; No `.workbench` in the actual installation.
- Existing desktop shortcut further refers to the updated EXE.
Phase 6 and runtime phase D were not started. Open development points are not merely shifted in-game tests and are not run as completed.

**Final exam of this level:** 158 Rust tests, plus a local hash-tested table sample, six frontend unit tests and 49 UI flows passed. Fmt, Clippy `-D warnings`, TypeScript and release build successful. EXE 15,938,560 Bytes, SHA-256 `b78a222e9611dd33bcf0b0eacaef9e845724c817b6307b738180d3a91888475f`. Detection: `.local/phase5-validation.json`. No release of missing functions or claim of a successful manual in-game validation.

## Phase 4: v0.4.10 – confirmed external files and completion of development

- New opt-in view for additional foreign files with paths, sizes, SHA-256,
explicit confirmation and revocation. Confirmation only writes an immutable data set in the project; It does not trigger a live event.
- Exactly confirmed additions remain with Setup, Apply/Reapply, Restore and
Receive base change. All content is freshly checked under kept protection before live writing; Additional EXEs are included in the launch protection. Changes, new children and revocation do not create tacit release.
- Inventory and content report separate these additions from originals and own
files. An optional report reference indicates the confirmation status; Old reports remain readable. Original paths can never be filtered out.
- Foreign registry registrations and changes to original sources remain
locked and require take back/Steam check first. No assumption of foreign table changes and no claimed compatibility. Saves and links are excluded. [Scope and operation](FOREIGN_FILES.md).
- Three local IPCs and CLI `foreign-preview`, `foreign-confirm --preserve`,
`foreign-revoke`. Live previews bind the confirmation; Revocation is also recognized during protected work. The app does not allow a parallel change of confirmation during its live job.

**154 Rust tests, 44 headless UI flows and 4 frontend unit tests.** Fmt, Clippy with `-D warnings`, TypeScript and frontend build successful. Nine new core scenarios test confirmed co-existence, content preservation, foreign group collisions, revocation, source changes, exclusion limits, protocol manipulation, configured save paths and base changes with confirmed additions. Four real file integrations remain included. After supplementing the protocol boundary, all eight foreign file tests passed again; the last entry remains available for revocation.

Windows release v0.4.10 and invisible native WebView2 check passed. The additional file preview was called on the real game only read; no confirmation, live setup or application has been triggered. Checked all three new session blocks and seven live session blocks; No JavaScript errors, 1024px view controlled. Inventory: 285 Original files, no additions or deviations. The 38 EXE/metadata hashes and the original user report remained unchanged. No complete PAZ hash run and no game lock.

Release: `target/release/crimson-workbench.exe`, 15,507,968 Bytes, SHA-256 `da9e49c8ba41413b6fbef63a89d2aa6051bc1fa6c84d2f825dc239929e7ab217`. The existing desktop link has been checked and points to this EXE. Evidence: `.local/phase4-v11-validation.json`, `.local/phase4-v11-native/result.json`, `.local/phase4-v11-rust-tests.log`, `.local/phase4-v11-foreign-final.log`, `.local/phase4-v11-clippy.log`.

**Next phase after Review/Go: Phase 5 (B4–B11).** Manuals in-game tests are still expected only after completion of all development phases. The optional Runtime/ASI theme remains separate and has not been started.

## Phase 4: v0.4.9 – Basic change and own files in the inventory

- The inventory separates proven own mod, backup and archive files
of unknown additives. Original files remain fully subject to verification. The assignment does not read ETA content; Foreign children and defective evidence are not hidden. Active own registry explicitly visible.
- Content checks and starting points also work according to their own
Application or Steam reset. Active mod registry and open base change prevent a new vanilla report.
- New checked base change in desktop and CLI. A new basis is required
well-known scheme, matching new original registry, current file preview, reconfirmation of origin and fresh protected original hashes.
- Old own files and complete transaction history will be byte-identical
archived on the same drive; The new game registry will not be written. An immutable project journal activates the new base only after completion. Interruptions, including partial protocols and new fuses, shall continue with the intended base.
- New IPC `live_update_preview`, existing cancelable worker for base change
Extended. CLI `live-update-preview` and `live-update`. No automatic setup, application or migration on the real installation.

**145 Rust tests, 4 frontend unit tests and 39 headless UI flows.** Fmt, Clippy with `-D warnings`, TypeScript and frontend build successful. Eight new core tests including six migration abort limits and partial protocol releases; Five additional UI flows. The four real file tests and 69 existing transaction breakpoints remain included. Windows release v0.4.9 and invisible native WebView2 check passed. All seven live commands deny invalid sessions, no JavaScript errors; 1024px view tested. Real inventory still 285 original files, no live approval and no Workbench folder in the game. The 38 EXE/metadata hashes and the original user report remained unchanged. No complete PAZ hash run and no game lock during this development.

Release: `target/release/crimson-workbench.exe`, 15,235,072 Bytes, SHA-256 `b9f8ad207bbd16d573679d1913468ddfc195d028bcd33501d694f3eb8861a518`. The existing desktop link points to this EXE. Evidence: `.local/phase4-v10-validation.json`, `.local/phase4-v10-native/result.json`, `.local/phase4-v10-rust-tests.log`, `.local/phase4-v10-ui-tests.log`. [Operation and exact limits](LIVE_APPLY.md).

**Open in phase 4:** Confirmed handling of foreign mods/manager remains; Conservative rejection. Manual in-game validation continues to be deliberately shifted.

## Phase 4: v0.4.8 – Live connection with protected sources

- Own live adapter for setup, file preview, apply/reapply, restore and
Recovery. Desktop with cancelable background job and six new local IPCs; CLI `live-status`, `live-setup-preview`, `live-setup`, `live-preview`, `live-execute`. No automatic live setup or application.
- Approval explicitly requires user confirmation that Steam Verify before
the selected content report has been completed. The report remains an observed cache report. The origin is thus user-confirmed, not independently cryptographically certified. All recorded original sources are hashed again completely under held Windows locks before each write operation; Registry, backup and own history are verified separately.
- All recorded EXEs will be exchanged against a new start before reading large archives.
held. Process/discontinuation testing also during hashing. Outdated previews, foreign files, altered sources and corrupted backups refuse writes. Own groups are replaced in the reapply; Original archives remain unchanged.
- The pure reading original data view explains its own modified registry
by backup, history and overlay files. As a result, new mod plans continue to build from original tables. The inspection kernel cannot write.
- New transaction intentions contain a timestamp; Remaining old journals
legible. A worker’s degree does not claim a failed commit in the event of late termination. Incomplete setup remains resumable with the same base.

**137 Rust tests, 4 frontend unit tests and 34 headless UI flows passed.** Fmt and Clippy with `-D warnings` as well as Windows release build successfully. Five new core tests test live adapter workflows on artificial installations: multiple apply/reapply with original data view and restore; obsolete plans, foreign files and similarly large source changes; termination and handling release; Recovery before/after registry commit; Writeless inspection and damaged backup. New worker test and extended session protection test. The four real file tests and 69 existing transaction breakpoints remain included. Five new UI flows check confirmation of origin, ongoing play, current mod values, outdated responses, failed starts, and cooperative cancellation.

Native **Windows acceptance v0.4.8** Passed: real installation and saved report read in the live preview, 285 files detected, origin confirmation not set and setup locked when the game is running. All six new orders deny invalid sessions. 1024px screenshot tested; No JavaScript errors. Existing mod/recovery/baseline samples also passed. Neither `.local/live` nor `<Spiel>/.workbench` were generated; **no live apply, no game locks, no complete archive hash run during development**. All 38 observed EXE/metadata hashes and the user report remained unchanged.

Release: `target/release/crimson-workbench.exe`, 15,003,648 Bytes, SHA-256 `1db048d67eb73374608241c259d456f84b6930b025e731862dfb472ca30c3b31`. The existing desktop shortcut continues to point to this updated EXE. Evidence: `.local/phase4-v9-validation.json`, `.local/phase4-v9-native/result.json`, `.local/phase4-v9-rust-tests.log`, `.local/phase4-v9-live-setup-preview.json`. [Operation, concrete model of trust and limits](LIVE_APPLY.md).

**Next development point:** adjusted inventory according to own live application, change of base / history according to game updates and defined handling of foreign mods. Phase 4 is therefore not yet marked as fully completed. The deferred manual in-game tests does not block this work.

## Phase 4: v0.4.7 – permanent baseline and registry-backup

- Exported full content reports can be re-compared with
Installation, build, repository lists, file stock and registry can be transferred to the new view **baseline & Registry-backup**. There will be no new major archival hash run and no game lock.
- The app stores the unchanged report, the matching registry copy and
A final manifesto under `.local/baselines/<id>`. copies are re-examined after writing; The manifesto will be published last. Missing financial statements, damaged files, unauthorized inputs and outdated previews are rejected. Existing estates are never overwritten.
- Saved statuses can be reopened after navigation or restart.
The app distinguishes intact backup with suitable current metadata, interim deviations and non-auditable installation. Old backups are retained during updates. No restoration in the installation.
- Four new local IPCs as well as CLI `baseline-list`, `baseline-preview`,
`baseline-capture --review-id` and `baseline-status`. [Operation, persistence and confidence limits](BASELINE.md).

**131 Rust tests, 4 frontend unit tests and 29 headless UI flows passed.** Fmt and Clippy with `-D warnings` as well as release build successfully. Seven new core tests check content preservation, input limits, termination before completion, outdated previews, manipulation/hardlinks, update states and protected paths. A test explicitly demonstrates that equally large archival changes can remain undetected when re-comparing metadata; This does not result in write authorization. Four new UI flows check save, reopen, error/update states, and delayed responses. The four real file integrations and 69 existing transaction abort points remain included.

Native **Windows release acceptance v0.4.7** Passed: The report provided by the user was adopted unchanged, a 679 byte registry copy was created, both checked and reloaded after a change of view. All four new commands deny invalid sessions and are registered in the local capability. CLI `baseline-list` and `baseline-status` confirmed the stored status after the end of the test instance. No JavaScript errors, screenshot and 1024px checked; Previous mod/recovery/update samples were also successful.

Actually stored status: `.local/baselines/1789906043-17808-0`. Check report: 285 Files / 195 PAZ archives / 154,097,618,899 Bytes, content check from 20.09.2026 at 13:25:45 clock (Europe/Vienna). SHA-256 of the registry-backup: `c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea`. The game continued during the decline. All 38 previously observed game EXE / metadata hashes as well as the user report remained unchanged.

Release: `target/release/crimson-workbench.exe`, 14,376,448 Bytes, SHA-256 `691a2327ba988325fd471adf9c6a7e0099ce1893edf9f406c5a3397be6a96275`. Evidence: `.local/phase4-v8-validation.json`, `.local/phase4-v8-native/result.json`, `.local/phase4-v8-rust-tests.log`, `.local/phase4-v8-baseline-status.json`.

**Development status:** The observed baseline and registry-backup are integrated. The trustworthy approval as a live base and the live connection with re-checked source/start paths as well as backup/update integration are still open. A saved cache report does not automatically become vanilla. Manual in-game acceptance is deferred separately until the end of all development phases; It does not block their further development.

## Phase 4: v0.4.6 – checked withdrawal of existing workspace rehearsals

- New desktop view **backup & restore**: loading existing samples,
Select a protected v3 sample, check backup and sources, view specific returns with file hashes and execute them in a targeted manner. successfully reset samples are recognizable as such; No automatic restore.
- Open existing Zustand without initialization: preview and new restore
Do not re-create missing backups or a lost history. Before writing, the preview bound to path, marker, backup, history generation, commit state and existing files is recalculated under the protection locks. Obsolete plans or plans created for other project copies are rejected.
- Open transactions are based on their actual registry before/after commit
Classified. Foreign changes, missing active files, corrupted backups or source updates lead to cancellation. After Restore, Zustand and hashes are checked again; backup and history are preserved.
- New CLI commands `mod-backups`, `mod-restore-preview` and `mod-restore --review-id`.
Three new local Tauri commands with session verification; No live writing ability. [Operation, order and limits](BACKUP_RECOVERY.md).

**124 Rust tests, 4 frontend unit tests and 25 various headless UI flows passed.** Fmt and Clippy with `-D warnings` error-free; Included are four real file tests. Six new core tests check the withdrawal, including error/update cases and read-only previews. In addition, the before/after hashes of the redemption plans at all 69 previous transaction breakpoints are checked against the actual remaining files. Four new UI flows pass after correction of two test selectors; The 21 existing flows remained green.

The final **Windows release v0.4.6** consisted of the native WebView2 acceptance: all three new commands correctly in the capability, invalid sessions rejected, existing backup checked via the interface. In an additional copy of our sample, a final marker was deliberately removed; the visible process recognized and ended the open transaction without writing back any foreign data. the re-closing is the same as the original; Zustand afterwards recognized as already restored. CLI list, preview and re-establishment of the same copy passed. 1024px without overrun, screenshot checked, no JavaScript errors. The previous mod previews, protected Apply/Reapply/Restore samples and update rejection also passed again.

The game continued during the native decline. All 38 previously observed EXE/metadata hashes are unchanged; the current file list still has 285 matching entries. No more complete ETA hashing. The existing desktop shortcut points to the updated EXE.

Release: `target/release/crimson-workbench.exe`, 14,097,920 Bytes, SHA-256 `a03e8068a842f3450200cfa98284b35c53bb3f6fadcfc54ea6dd579a2c652614`. Evidence: `.local/phase4-v7-validation.json`, `.local/phase4-v7-native/result.json`, `.local/phase4-v7-rust-tests.log`, `.local/phase4-v7-restore-preview.json`, `.local/phase4-v7-cli-recovery.json`.

**Phase 4 remains open.** The complete user content comparison from v0.4.5 is successfully completed. Independently confirmed vanilla origin, full live start/charge path protection, live backup/update integration and in-game acceptance are still missing. The new view only restores project copies. Phase 5 was not started.

## Phase 4: v0.4.5 – cancelable content verification

** Addendum 20.09.2026:** The user has run the full original run in the desktop app and provided the exported report. From 13:23:25 to 13:25:45 clock (Europe/Vienna) 285 files including 195 PAZ archives with 154,097,618,899 bytes were checked. All SHA 1 values match the local Steam deposit lists; According to the report, the metadata remained stable. The subsequent report evaluation confirms the internal consistency and all 38 previously observed EXE/metadata SHA 256 values. No game files were read or changed again. The complete reading content test has thus been carried out; an independent vanilla proof and the live write authorization do not follow from this. Evidence: `exports/installation-audit-1789903405-11284-1-0.json` and `.local/user-content-audit-review.json`. SHA-256 of the original report: `f32c50a354dbd81e85889cb764b4ecdf6d83958ed09c9b9eef70077cc48a64dc`.

- New complete content comparison of the installed repositories: SHA-1
against the local Steam cache and additional SHA256 recording in a common read run. Sizes, timestamps, file identifiers, registry and custodian stock shall be rechecked before/after the run. The same file sizes are no longer sufficient for a successful content comparison.
- Own background order with byte/file progress, cancellation and export
Complete reports. The catalog lock is not held during hashing. Change of view allow the order to continue; Installation/language change cancels it. Not more than one order per instance; no partial reporting release.
- The run is blocked if the game status is current or unknown. Process checks
also during the reading and at the end; The detected start of the game ends the examination. No launch locks, game-writes or save accesses. Handles still allow writing/deletion; actual handling paths, type and link number are checked.
- CLI `installation-verify` complements the separate metadata command. In case of
Content deviations there is a diagnostic report and exit code 2; Not a full report in case of termination. Reports remain observations of the unconfirmed Steam cache: `certified_vanilla` and `can_apply` are always false.

**118 Rust tests, 4 frontend unit tests and 21 headless UI flows.** Fmt and Clippy with `-D warnings` error-free. Ten new core tests cover hash vectors, multi-block/blank files, same-sized manipulations, abort, process error/start, source/depot change, growth, missing/foreign/hard-linked sources, the real process barrier and simultaneous writeability of the read handles. Two new worker tests test exclusivity, session limits, discontinuation, and panic treatment. Four new UI flows cover lock, progress, change of view, breakdown, errors, deviations and export. The four real file tests and 69 previous transaction breakpoints remain included.

The first complete run found a Windows test collision: a separate `probe.exe` of another test ended between process snapshot and path query (error 87). The process check correctly refused access. The isolated test passed; The fixture lifetimes of the same name are now serialized. All unchanged protection tests pass in the new parallel overall run. Two first UI cases failed due to reused mutable mock objects; The fixture now maps the real, deserialized responses for each IPC.

The final **release EXE v0.4.5** consisted of the native WebView2 acceptance: 285 matching metadata entries, correctly locked content button during the game, all new commands in the local capability and rejection of invalid sessions. The existing mod preview with 34,488 changes, export, protected workspace rehearsals, reapplication, recovery, refusal and merchant/drop options remain green. 1024px without overrun, screenshot checked, no JavaScript errors. CLI recovery of the same v3 sample successfully; only the own invisible test instance ends. All 38 observed EXE/metadata hashes then unchanged. 156 Git-visible files without game/save content.

Release: `target/release/crimson-workbench.exe`, 13,844,992 Bytes, SHA-256 `3c78d6c0d86716695987256d753403c1961b1f36a0b08a6f9f2c08bcf2b52bc6`. Evidence: `.local/phase4-v6-validation.json`, `.local/phase4-v6-native/result.json`, `.local/phase4-v6-rust-tests-final.log`, `.local/phase4-v6-installation.json`, `.local/phase4-v6-cli-recovery.json`. [Operation, scope and limits](CONTENT_AUDIT.md).

**Phase 4 remains open.** The content reviewer is implemented; the user has then successfully executed the complete 154 GB run (added above). Independent vanilla proof, secured live start/load paths, live backup/update workflow and in-game acceptances are still missing. No live write command, no automatic Steam verification and no start of phase 5.

## Phase 4: v0.4.4 – Steam Depot and directory inventory

- New action **Check file list** in the mod workshop and CLI command
`installation-check`: directory tree, all installed repositories, registry groups and all EXE finds within the installation.
- Own limited depot/protobuf reader with payload CRC32, identity/
Size inspection and conflict detection. Missing or contradictory caches are not issued as a successful partial comparison. Insecure paths, duplicates, encrypted names and links are rejected.
- Missing, additional, size-deviating or otherwise typed entries
are separately visible. Directory links are not tracked. Uninstalled optional registry groups are not considered errors.
- The report remains a reading snapshot: no PAZ/EXE/DLL content,
No launch lock, no Steam verification and no write access to the installation. Appropriate names/sizes do not confirm content or vanilla; All trust/applyflags remain false, even with signature blobs in the cache.

At the real installation: **285 expected and 285 found files**, all names/sizes appropriate, no additional or missing entries. According to metadata, two installed repositories comprise 154,097,618,899 bytes. Three EXEs captured: `CrimsonDesert.exe`, `crashpad_handler.exe`, `pers.exe` under `bin64/`. The game continued; Registry corresponds to the previous observation. [Operation, limits and sources](INSTALLATION_CHECK.md).

**106 Rust tests, 4 frontend unit tests and 17 headless UI flows.** Fmt/Clippy with `-D warnings` error-free. New cases: truncated/manipulated custody lists, false identity/size, missing/contradictory caches, duplicate VDF keys, update metadata, corrupted registry, additional EXEs/groups, type/size changes, intentionally equal unrecognized content changes, and a real Windows junction outside the tested tree. The four real file tests and previous 69 transaction breakpoints remain green.

The first native acceptance found a missing Tauri ACL allocation for the new read command. `build.rs` and the local main window capability have been added; No general or remote access is added. The plan selector in the native test has also been narrowed down to the mod preview, as the installation test now also uses a result header.

The final **release-EXE v0.4.4** consisted of renewed native WebView2 acceptance: 285 matching files, three EXEs, no inventory deviations, existing plan with 34,488 changes, export, two protected Apply/Reapply/Restore cycles, cancellation recovery and refusal to update. Daily refresh, article additions, opportunities/quantities/guarantees and dealer exceptions remain checked. 1024px without overrun, screenshot checked, no JavaScript errors. CLI recovery of the same v3 sample also successfully. Only your own invisible test instance ends. All 38 observed EXE/metadata hashes are also unchanged afterwards; No complete ETA hash run. 150 Git-visible files without game/save content.

Release: `target/release/crimson-workbench.exe`, 13,586,944 Bytes, SHA-256 `589dba1b4ca254e23bf90845b97f5a2993f2aa1996d99284fca8994100a60aaa`. Evidence: `.local/phase4-v5-validation.json`, `.local/phase4-v5-rust-tests-final.log`, `.local/phase4-v5-installation.json`, `.local/phase4-v5-native/result.json`, `.local/phase4-v5-cli-recovery.json`.

**Phase 4 remains open:** The directory/depository list is now implemented; Full content verification and trusted vanilla proof, validation of all actual live start/load paths, live backup/update workflow and in-game acceptances are still missing. Live writing and phase 5 were not started.

## Phase 4: v0.4.3 – Hold locks and dealer details

- New v3-workspace rehearsals: A private session holds start, source, backup and
Folder locks via Scheduling, Apply/Reapply, Restore and Recovery. Before and after the acquisition, as well as at writing boundaries, actual process paths are checked. Marker and protective manifest are bound via SHA-256; the origin type remains expressly `project-copy-only`, never certified vanilla.
- Complete hash check of the detected sample sources from held handles.
Changes to a source file prevent recovery from a new/foreign Zustand, both on open transaction and after commit. Backups are re-examined and held over the session against changes.
- The desktop sample only uses a copy of our own EXE
and a synthetic source file. Two Apply/Reapply/Restore cycles, recovery and an intentional source change including rejection are integrated. Windows is required; No unsecured replacement path. V2 samples remain recoverable separately; A v3 downgrade is rejected.
- merchant details: individual item sets replace the common selection,
Empty sets suppress global additions. Refresh per trader can be daily, preserve the original interval or inherit globally again. Explicit exceptions also apply outside of global merchant selection. stock exceptions shall remain separate; all changes are rebuilt from the original.

**Exam: 96 Rust-Tests, 4 Frontend unit tests, 15 Headless UI flows.** Fmt and Clippy with `-D warnings` error-free. The four real file tests and the 69 previous transaction breakpoints are included. New protection tests refuted two original Windows assumptions (running images and pure attribute handles); In addition, the process path comparison was canonized on both sides. These cases remain unchanged after the corrections.

The final **release EXE v0.4.3** passed the native WebView2 test: existing 34,488 changes plan, protected trial cycles, cancellation recovery, refusal to update, 246 daily refresh changes, growing shop table, opportunities/quantities/guarantees and individual merchant selection (2200 replaces globally selected 50001). No JavaScript errors and 1024px without overrun. An ambiguous test selector has been specified; The same unchanged app passed the full rerun. The updated CLI then restored the same v3 sample correctly.

Meanwhile, the game continued. Only own invisible test instances have been started/closed; No game or save locks. All 38 observed EXE/metadata files are unchanged, not a complete PAZ hash comparison. 145 Git-visible files without game/save/cache content.

Release: `target/release/crimson-workbench.exe`, 13,431,296 Bytes, SHA-256 `341eca06ab9957a0a6007c3cf6c1a87b569e9051552f7526db0a63022990d13e`. Evidence: `.local/phase4-v4-validation.json`, `.local/phase4-v4-rust-tests.log`, `.local/phase4-v4-native/result.json`, `.local/phase4-v4-cli-recovery.json`. [Protective drain and borders](PROTECTED_REHEARSALS.md).

**Phase 4 is still not complete.** Open are the trustworthy confirmed full game base, the full live start path/source directory, the live backup/update workflow based on it and in-game acceptances. Unknown special dealers and the complete catalogue at all dealers at the same time are still not released. There is no live writing command; Phase 5 was not started and no save contents were read.

## Phase 4: v0.4.2 – Assortments, daily refresh and drop opportunities (historical)

- Additional articles and article sets from all 6,816 items at 208 supported
dealers. Existing positions, conditions and save indices are retained; New positions take normal price factors of the same trader. The table header is rebuilt during growth. A complete catalogue at an individual retailer is checked against the real data format.
- Daily refresh at the occupied daily fields: 369 supported intervals,
of which 246 changes from 3/7 to 1. Unknown/permanent intervals and one-off flags are retained; The eight September bytes remain raw.
- Opportunity multipliers and individual exceptions for 12,306 unconditional item dropsets
with independent roll type 0, limited to 100 %. Manual warranty selection sets the base rate to 100 % and takes precedence over opportunity factors. Quantities and opportunities are recalculated together from the original data.
- Own static EXE/table analysis proves refresh/stock indices and the
Roll type 0 denominator 1.000.000. Weighted and limited roll types are not interpreted as independent percentage chances. No automatic boss classification.
- Steam-Cache examined in more detail: Both installed depot manifests contain
File hashes, but empty signature sections. They do not independently certify the base. No Steam check, no download or full archive hash run started.

Test: **87 Rust tests, 4 frontend unit tests and 14 headless UI flows passed**, Fmt and Clippy with `-D warnings` error-free. The extended real file test for the complete range also existed afterwards. Previous 69 transaction cancellation scenarios and recovery tests remain included.

The final release EXE **v0.4.2** There were real ones. WebView2Test: Existing 34,488-Change plan, daily refresh with 246 Changes, two new articles at dealers 3101, Goblin rate 35,000 → 70,000 and quantity 1 → 3 As well as manual base rate 1.000.000Export and two Apply/Reapply/Restore cycles plus recovery also passed with grown shop table. No JavaScript errors, 1024px without overrun. An initial transcribing error in the test script was corrected; the App EXE remained unchanged. The game ran on, only its own invisible test instance was closed. 38 observed EXE/metadata hashes are unchanged.

Release: `target/release/crimson-workbench.exe`, 13,281,792 Bytes, SHA-256 `ac3edb3dc5824ef9bb966260917efdd33b13f828a2f72a08572912a71eceeff6`. Evidence: `.local/phase4-v3-validation.json`, `.local/phase4-v3-rust-tests.log`, `.local/phase4-v3-all-items-test.log` and `.local/phase4-v3-native/result.json`. Field positions, data coverage and sources: [B1/B2 field evidence](research/PHASE4_FIELDS.md).

**Phase 4 remains open.** B0 requires the trustworthy confirmed vanilla base, live backup/update paths, integrated launch protection and manual in-game acceptance. No full power failure protection yet and no live writing command. B1 does not support additions at all special dealers, no different item sets per dealer within a plan, and not all items at all dealers at the same time: 100,000 dealer/item combinations per preview are a Workbench limit, not a confirmed engine maximum. B2 guarantees relate to the individual opportunity in the set trigger, not higher-level events/runtime modifiers. Gameplay effects of B1–B3 and large assortments remain to be checked manually. Phase 5 was not started; saved games were not opened.

## Phase 4: v0.4.1 – Reapplication and recovery (historical)

- New private transaction core: up to eight separate overlay groups,
Reapply with new IDs, registry commit before removing old groups, and complete file change plan with before/after hashes.
- Immutable intentions and financial statements, hash check of history, verified
Registry-backup, recovery after partial writes and re-abortions of recovery. Incomplete private files remain for diagnosis.
- Two real Apply → Reapply → Restore cycles plus intentional cancellation/recovery
per desktop sample; six visible file transitions. CLI `mod-recover` restores only marked workspace rehearsals.
- protection against foreign files, registry/backup deviations, hardlinks,
Reparse points, group collisions and duplicate virtual table paths.

Independent full vanilla proofs, live backup/update paths, integrated launch protection, power failure and in-game acceptance remain open. B1/B2 field gaps from v0.4.0 continue to exist. **Phase 4 is not completed; no live writing path.** The old v1 core remains unchanged as a test reference; Samples are not automatically migrated.

Test: **83Rust tests**, **4 front-end unit tests**, **13 headless UI flows** passed; Fmt and Clippy with `-D warnings` error-free. The complete workspace run included 82 tests, then the format suite with additional length regression case passed (18 instead of 17 tests). The 69 injected Apply/Reapply/Restore terminations and interrupted recovery are included.

The final **release EXE v0.4.1** passed the native WebView2 test with real data: 34,488 field changes, two Apply/Reapply/Restore cycles, recovery, six file transitions, export/credits, unchanged trust penalty, obsolete preview locked and 1024px rendering without horizontal overrun. No JavaScript errors. The game continued; Only your own invisible test instance has been closed. The CLI recovery call of the same already restored sample also existed.

Release: `target/release/crimson-workbench.exe`, 13,178,368 Bytes, SHA-256 `7cf40162604c74395ef818d0d23630f3bde1dacad3418b6569847e133b96d173`. All 38 captured EXE/metadata files match the completion of v0.4.0; No full PAZ hash check and no vanilla certification. 140 Git-visible files without game/save/cache content; 69 relative document links checked. Evidence: `.local/phase4-v2-validation.json`, `.local/phase4-v2-rust-tests.log`, `.local/phase4-v2-format-tests.log`, `.local/phase4-v2-native/result.json` and `.local/phase4-v2-cli-recovery.json`.

## Phase 4: v0.4.0 – first preview status

- Desktop page **Mod Workshop**: Shop inventory of existing positions,
Drop volumes and positive trust gains, selection and individual exceptions, verifiable field/file plan and export of not yet applied overlays.
- Own deterministic PAZ/PAMT builder with encryption, compression,
Alignment and checksum chain; Registry entry before vanilla, collision-free group selection, observing reserved groups. No CDUMM dependency.
- Private transaction core for **Project copies**: exclusive lock, verified
Replace registry backup, journal, registry last, remove your own files, hash comparison and restart at tested interruption limits.
- Real data coverage: 397 merchant / 6,376 goods items,
12,736 Quantity dropsets, three friendly records. Other 39 dealers, 1,709 special dropsets and 299 empty/sentinel item dropsets remain unchanged.
- Donate +50 and Talk +5 become +150/+15 at ×3; Threat −200 remains.
This is a confirmed byte test, not an in-game effect test.

**Open before completion of phase 4:** complete independent vanilla base and live backup, integrated launch protection, more comprehensive staging/crash recovery, productive reapply with group change, multiple groups and manual in-game acceptance. B1: new articles/article sets/all and daily refresh. B2: Opportunity and roll semantics as well as guaranteed bossdrops. A1/A2 source mapping remains open. The user interface and CLI have **no live write command**.

Exam: **74Rust-Tests**, **4 Frontend unit tests**, **13 Headless UI flows** Fmt and Clippy with `-D warnings` green. Real body/header round trips, unchanged out-of-choice records, unknown September bytes, encrypted overlay readback, and six apply/five restore interruptions checked. A start protection test uses only a copied own test EXE. Linux and real power outages have not been tested.

Native testing of the **final release EXE v0.4.0** successful: Shops 999, drops ×2, Trust ×3 result in 34,488 changes in three scheduled files. Two apply/restore cycles on a project copy, export with full changes/credits, outdated preview locked, 1024px display and no JavaScript errors. The game continued; the invisible own Workbench test instance was then closed. A first test filter on “Friendly” also hit regular item dropsets; with the correct module filter "trust", the same unchanged release EXE passed the complete test.

Release: `target/release/crimson-workbench.exe`, 13,021,696 Bytes, SHA-256 `0ed85abcd034c634780f080d25653120b566439b811e80245433a00e43814b5f`. Evidence under `.local/phase4-rust-tests.log`, `.local/phase4-native/result.json` and `.local/phase4-verification.jsonl`. [Operation and limits](MODS.md). Final comparison: all 38 previously captured EXE/metadata files unchanged; No complete PAZ hash comparison and no vanilla certification. 137 Git-visible files without game/save/cache content, 69 valid relative document links. Summary proof: `.local/phase4-validation.json`.

The following sections preserve the status of previous phases.

## Phase 3: current status

- 18,576 Recipe records and 1,602 material groups read completely typed
and byte-identical reconstructed. 13,035 simple item dropsets as well; 1,712 Special variants remain expressly opaque.
- **1,108 Predictable recipes**; 17,069 gain/special consumption lines and
399 Unreleased result variants are excluded.
- Objective and quantity, multi-stage material tree, aggregated residual demand, common
Inventories, reuse of batch surplus, choice of recipe/materials, manual procurement of intermediates and visible cycle warnings.
- New desktop page **Crafting planner**, real icons, recipe links in item details,
as well as CLI commands `recipes` and `craft`. Quantities remain exact as decimal strings.
- Unknown world sources and trader details remain visibly unexplained.
Dropset IDs are not claimed opponent or location assignments.
- Requirements are displayed, not checked against a save. None
Best price optimization, mixture of material alternatives or save import.

Exam: **65Rust-Tests**, **4 Frontend unit tests**, **9 Headless UI flows** Including real round trips of the current installation and planning of each released recipe. Native WebView2 testing with separate invisible Workbench window: arrows 31 → 60, supplies, material alternatives, item recipe links and 1024px presentation successful; No JavaScript errors. The own test window was then terminated. Game and visible desktop were not operated.

[Format, volume documents, operation and limits](CRAFTING.md). Local evidence: `.local/phase3-rust-tests.log`, `.local/phase3-native/result.json` and screenshots in `.local/phase3-native/`.

Final release check: `target/release/crimson-workbench.exe` v0.3.0, 12,508,672 bytes, SHA-256 `d89c9eff1e91f7eca5225125a09e4b8151ec84b05486855adc92a164ec4914c2`. Manufacturing smoke test with exactly this EXE successfully. The subsequent item browser regression check (item 2200, real icon, raw fields, protected JSON export, ger → eng) also successful; Result under `.local/phase3-item-regression/result.json`. The combined test script first required an explicit switch back to the item page; After this pure test correction, the unchanged release app ran through.

CLI-Planexport successfully tested. All 38 originally captured EXE/metadata hashes are still identical; No complete PAZ hash comparison. 123 Git-visible files, no game/save/cache files underneath. `cargo fmt --all -- --check` and Clippy with `-D warnings` green. The new header/body round trip check was then confirmed again with real files. Linux was not tested locally.

**Point of stop for viewing:** Phase 4 / Apply was not started. A2 is still incomplete during source mapping; this point is not recorded as fulfilled. The following sections preserve the state of the earlier phases at that time.

## Phase 2: delivered status

- Tauri-2-App with React, TypeScript, Vite, Zustand and virtualized TanStack-
Table. Dark layout, item list with fixed detail view and data source page.
- 6,816 real items, FTS5 search, combined type/category/tier/stack/stat ID
Filter, global sorting, reloading and keyboard operation.
- Real local icons: additional SHA-tested `stringinfo`, **31,812** typed
Stringrecords with byteidentical body/header round trip, Item-Key → String → DDS → PNG. Real DXT5 texture of Item 2200 successfully displayed.
- Detailed overview, all raw fields with type/offset/hexbytes/value, field search and
Filter for unknown fields. Large integers remain exact in the frontend.
- Direct typed item references to existing data sets; No links from
any u32 values. JSON output as a new file exclusively under `exports/`.
- 15 Selectable languages. Session and request IDs prevent obsolete results;
Language/build indexes are separate. Current Core-reading schema: `…gamedata-2.3-v2`.
- Load/empty/fault states, locked unknown builds, path selection and manual
Re-reading. Six explicitly allowed native commands, no FS/Shell/Apply plugins. No production test data or network assets in the surface.

**A1 is not yet complete:**Merchant/city/region/price/stock, DropSet sources and recipe relationships are missing. The existing MIT storeloader only parses names/keys and documents even variable, unreliably read stock structures. Unchecked sources are therefore not claimed from byteheuristics. Categories/Stats remain partially raw IDs; Their gameplay designations and units are not invented. [Details and operation](DESKTOP.md).

|Current audit|Results|
|---|---|
|Total Rust workspace|**57 Tests passed**, 0 failed: 1 App, 3 CLI, 36 Core, 15 Format and 2 Actually executed real file tests.|
|Frontend|**4 Unit tests passed**, TypeScript and Vite-Build successfully.|
|Headless Edge UI|**6 Passed end-to-end tests**: Search/Details/Export/Links, Virtualization/Pagination/Filters, obsolete responses, unknown builds, 1024px blank state and browser without desktop bridge. Fixtures are explicitly synthetic and only in tests.|
|Real Tauri-/WebView2 app|Invisible separate test window: real item 2200 "Bump Arrow", real PNG icon, 210 Fields, `max_stack_count=100`, JSON output under `exports/`, language change to "Stub Arrow"; **no JavaScript error**.|
|Finale Windows-EXE|`npm run desktop:build` successful; `target/release/crimson-workbench.exe`, 12,057,088 bytes. The same native UI test was then successfully executed with exactly this release EXE.|
|Formatting / Clippy|`cargo fmt --all` and `cargo clippy --workspace --all-targets --locked -- -D warnings` successfully.|
|Final installation comparison|All 38 originally captured EXE/metadata hashes remain identical; No complete PAZ hash comparison.|
|Repository test|109 Git visible files, no game/save/cache/build files under them; All relative markdownlinks are valid.|
|Platform border|Windows actually built and tested. Linux CI including Tauri system libraries; locally no Linux test proof.|

Local receipts: `.local/phase2-rust-tests.log`, `.local/phase2-native/result.json`, `.local/phase2-native/item-database.png`, `.local/phase2-native/data-sources.png`. Release test protocol: `.local/phase2-native-release.log`. The real UI exam ran invisibly and the own test instance was then closed. Game process and visible desktop were not operated.

**Historical breakpoint after phase 2:** Phase 3 was not started at that time. The open A1 source mapping is not a successfully declared test and not a released modification. The following section preserves the phase 1 detection as a course.

## Phase 1: Core and CLI – completed proof

- Rust workspace with `cd-core`, `cd-cli` and a limited native
[MIT format porting](FORMAT_PORT.md). No python or modmanager dependency.
- installation detection for Steam libraries, epic manifests and game pass,
Includes explicit selection, Xbox content directories, and Proton Saves. Savepaths are only found; Save contents are not read.
- read PAZ/PAMT/PAPGT, check totals, decrypt supported codecs and
Decompression. Unknown codecs, damaged structures and oversized inputs are rejected. No need to fully unpack the installation.
- Versioned reading schema `steam-25381195-gamedata-2.3-v1`: EXE version, 38
EXE/metadata files, all table files used 28 and the selected item PALOC must match the tested SHA-256 hashes. Unknown builds only receive diagnoses, not semantic interpretation.
- 6,816 Typed items with full field ranges; 13 more tables
have tested key/offset indices and raw data sets obtained unchanged. These raw data are not confirmed shop, recipe, quest or skills chemata.
- Item localization in 15 verified languages, German as standard.
Numerical PALOC references are resolved directly; Fallbacks remain visible.
- SQLite FTS5 transactional index for names, descriptions and internal keys,
Type/Category ID filter, pagination and automatic rebuild for other fingerprint, reading schema or other language.
- CLI: `detect`, `fingerprint`, `languages`, `tables`, `dump`, `roundtrip`,
`index`, `search`, `item`, `diff`. [Guidance and API](CORE.md).
- JSON files and SQLite caches only under `.local/` or `exports/`
in the project. New JSON issues do not overwrite anything. Game/Save paths, junction redirects and problematic hardlinks/SQLite sidecars are blocked.

## Tests carried out

Windows, Rust 1.95; lokale Steam-App `3321460`, Build `25381195`, EXE `1.0.0.2944`.

|Test test|Result and limit|
|---|---|
|`cargo test --workspace --locked -- --test-threads=2`|**51 Passed**, 0 failed: 3 CLI, 32 Core, 15 Format and 1 Actually executed real file test.|
|`cargo fmt --all -- --check`|Successful.|
|`cargo clippy --workspace --all-targets --locked -- -D warnings`|Successful, no warnings.|
|`cargo build --release --locked -p cd-cli`|Successful; Executable under `target/release/cd-cli.exe`.|
|Release CLI end-to-end testing|20 Examinations passed: Recognition, Languages, Fingerprint, 14 Tables, Roundtrip, Index Building/Reuse, German FTS Search, Item/Dump Data, Language Change, Builddiff, Output Protection, Unknown Build and Final Metadata Hashes.|
|Real files parse serialize|**34 Byteidentical tests**: 14 body/header pairs, additional typed item body/header, PALOC payload/container, PAPGT and PAMT. Other table bodies remain expressly opaque.|
|Full Item Field Coverage|All 6,816 records checked; native format verifier captured **1,763,837** contiguous field areas without gaps/overlaps.|
|Localization Verifier|All 15 Item PALOCs read, hashed and received without loss; German contains 13,575 entries. Payload is newly serialized, original compressed shell is preserved unchanged. No claim of byteidentical recompression.|
|Unchanged installation metadata|Finally, all **38** captured EXE/metadata files remain identical to the original hashes. No complete hash comparison of all PAZ files.|
|Linux|Ubuntu/Windows CI configured; Linux is not running here. No running Docker daemon, no local Linux build proof.|
|Frontend|Starts in phase 2; No front-end tests are yet applicable.|

Local documents excluded from Git: `.local/phase1-tests.log`, `.local/phase1-smoke.log`, `.local/phase1-smoke-20260919-162802/`. Research aids under `.local/` are not runtime dependencies.

## Open borders and decisions

- The matching read build is **no vanilla certification**.
`certified_vanilla` is wrong. Apply/Restore, backups, third-party mode evaluation, process lock, crash recovery and manual in-game proof belong to phase 4.
- Steam was recognized and used on this machine. Epic/Game Pass detection
is verified by synthetic manifests; Their deviating data builds are not automatically released. Further builds require their own hashes/schemas.
- Item structure and field bytes are fully captured, but not every
Field meaning/gameplay effect is confirmed. Unknown fields are marked in raw form; Categories remain IDs, icons remain references. No invented labels.
- Quest/Stage indices have a 32 bit count; other recorded indices
16-Bit-Count. This was checked on the current files. Storebody and its September extensions are not yet semantically interpreted.
- Recipes, vendor/dropset relationships, map atlas/coordinates, complete
Collectibles, inventory/stack maxima and various game effects remain the subject of the later phases. `reference/mods/` does not yet contain any mods.
- An unchanged cache index scheme is reused. The CLI still checks
the installation at each start; Not yet a permanent GUI session or watcher.
- The unchanged crimson-rs-Gitdependency was due to private modules and missing
`rlib`-API unsuitable. Therefore, targeted, documented MIT porting instead of a foreign manager. Sources and limitations are in [CREDITS.md](../CREDITS.md).

No game/save files were modified, no save contents were read, no gaming process was controlled and neither Steam-Verify nor Apply/Restore were executed. Intermediate clippy findings and two review margins (Xbox content selection, closed stdout pipe) have been resolved; The final stand is green.

## Phase 0 and next stopping point

Phase 0 is completed. Your search remains in [FORMATS.md](FORMATS.md), [FEASIBILITY.md](FEASIBILITY.md), [APPLY_DESIGN.md](APPLY_DESIGN.md), [ARCHIVES.md](research/ARCHIVES.md), [TABLES.md](research/TABLES.md) and [APPLY.md](research/APPLY.md). The [Build Observation](builds/steam-25381195.observed.json) contains hashes and findings at that time, no game content.

On the go, phase 2 was implemented as documented above. Modexport, Apply Engine and Save Reader remain in their intended phase; Runtime phase D remains separate.
