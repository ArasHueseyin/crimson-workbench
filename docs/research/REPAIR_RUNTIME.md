# Repair: isolated native proof of execution

## Current status: Inventory package serializer, module 0.23.0

Original transmitter and stream selection run on private data. 36 new cases, ten of them with transaction and native client alignment. Total 454 native scenarios, 4,812 calls, nine CTest suites. Ten useful bytes contain actor, container, slot and main value; no item UID, socket or order identifier. Your own process refuses success in the event of delivery errors or false/freshly changed identities. Transport, successful pool branch and server/store closure remain open. [Evidence](../../runtime/repair/INVENTORY_PACKET_2949.md).

## Historical status: native buffer preparation, module 0.22.0

Original encoders, buffer management and LZ4 compression run on private data. 66 new cases; Total 418 native scenarios/4,289 calls and nine CTest suites passed. Contents, header, lifetime and repetition checked for opening errors. Repetition can be double compressed; New raw data build-up is required. Downstream processing and file operations remain callbacks; Full integrity/encryption, storage and game host open. [Evidence](../../runtime/repair/SAVE_ENCODING_2949.md).

## Historical status: File Writer, Module 0.21.0

Original recorders and two clean-up assistants are executed on private buffers. 34 new cases, total 352 native scenarios/4,095 calls and nine CTest suites passed. Flush/close errors can still generate success, payload sharing and deleted dirty. File/encoder receivers are own callbacks; the integration replaces the queue/backup path with a single entry bridge. Real encoder and write wrapper statically assigned. Full coding, final confirmation and game connection remain open. [Evidence](../../runtime/repair/SAVE_FILE_2949.md).

## Historical status: save dispatchers and queues, module 0.20.0

Original dispatcher, Steam approval check and queue marker run on private objects. 39 new cases; Total 318 native scenarios/4,039 calls and nine CTest suites passed. Skip gates can report success without back-end work; A previous success status is retained. All four error levels, quota limits and three queue types in both allocation modes have been tested. File and platform recipients remain their own callbacks. Concrete file helpers and timers are statically assigned; The Writing Assistant ignores the flush status. Native file buffer processing, regular storage/confirmation and actual game host remain open. [Evidence](../../runtime/repair/SAVE_BACKEND_2949.md).

## Historical status: native memory object conversion, module 0.19.0

Original Item/ItemSaveData converters and their lifetime dependencies run on private objects: 38 new cases, of which 14 with actual repair planning/aftercopy. Main/socket values, native normalization at 0/no-wear, dye data, reset and reuse checked. In total 279 native scenarios/3,950 calls and nine CTest suites passed. This occupies conversions in memory; Regular serialization, storage termination, further optional data and full game connection remain open. [Evidence](../../runtime/repair/ITEM_SAVE_2949.md).

## Historical status: SQL execution shim, module 0.18.0

Full execution body and joint request acquisition checked on private data. 19 new cases; Total 241 native scenarios/3,674 calls and nine CTest suites passed. The shim can deliver success without reading the request; it does not execute a database order itself. Old request errors are retained in the bypass branch. Neither return 0 nor emptied slot markings prove storage. The downstream alternative processing and storage/charging semantics remain open. Actual switch value unknown in the game. [Evidence](../../runtime/repair/SQL_DISPATCH_2949.md). The concrete alternative transaction method also only reports success. Separately, Item/ItemSaveData conversions for main value and base are statically occupied. Your native execution and actual storage are pending. [Memory object detection](../../runtime/repair/ITEM_SAVE_2949.md).

## Previous status: Inventory client feedback, module 0.17.0

Original client Ack for inventory repair run on private sources and in the common adapter. 36 new cases, total 222 native scenarios with 3,526 calls; Passed nine CTest suites. The Ack sets main values absolutely, does not check any UID and does not change any bases. Independent confirmation denies UID changes, missing/double messages and completion errors. Inventory servers, actual persistence/socket loading semantics and real UI/effect/transport routes remain open. The examined socket SQL helper stores an ItemInfo identifier, not a proven durability value. [Evidence](../../runtime/repair/INVENTORY_EVENTS_2949.md).

## Previous status: Slot processing and batch completion, module 0.16.0

The common sequence ends only after all item messages. Original slot consumer, reusable clear and native table sharing are integrated on private components. Nine CTest suites, 43 expiration scenarios and 186 native scenarios with 2,826 calls passed, including 24 consumer and 18 equipment cases. Persistence errors after emptied list and successful client alignment do not produce success. The persistence recipient is artificial; the occupied path does not transmit base data records. Actual persistence including socket path, inventory events, real receivers/feedbacks and game host are missing. [Evidence](../../runtime/repair/PERSISTENCE_2949.md).

## Previous status: common sequence, module 0.15.0

Prepared native post-copies, field recorders and message adapters connected; the 15 equipment cases use the common production adapter. Nine CTest suites, 40 expiration scenarios and 159 native scenarios with 2,023 calls passed. Base number separated from logical space limit, additional vector strides corrected. Concrete inventory events, slot consumers, real receiver/feedback assignment and game host are still missing. [Evidence](../../runtime/repair/TRANSACTION_2949.md).

## Previous status: Durability field recorder, module 0.14.0

Own full field step for inventory and equipment, both Zustandcopies, with continuous owners/locks implemented. Complete pre-checking, only changed 16 bit fields, complete control reading and unclear result for partial errors. Eight CTest suites, 34 writing scenarios and 141 native scenarios passed. The engine transaction with prepared native copies and full events remains open, as does the game host. [Evidence](../../runtime/repair/FIELD_WRITER_2949.md).

## Previous status: Slot marking and capture held, module 0.13.0

Continuously held owner references and locks for capture, planning and refresh are implemented. Original equipment slot marking including place search, collisions and rehash is executed on private components and integrated into the event sample prior to notification. Seven CTest suites, 519 reader conditions and 133 native scenarios with 1,296 calls passed. Slot consumer, full writer, inventory events and host remain open. [Evidence](../../runtime/repair/DIRTY_SLOTS_2949.md).

## Previous status: native copies of items for 2949, module 0.12.0

Native lifetime management per item value, tested with original constructor, copy and destructor functions as well as all list helpers used. The event sample uses these post-copies. Seven CTest suites, 30 owner conditions and 117 native scenarios with 851 calls and 13,639 conditions passed. The own heap and effect/transport/UI receiver remain test dependencies. Writing path, slot update, inventory events and game host remain open. [Evidence](../../runtime/repair/ITEM_LIFECYCLE_2949.md).

## Previous status: Equipment events for 2949, Module 0.11.0

Own event preparation from the plan and native connection of server notifier, client corner and socket collector on private repaired items. Six CTest suites, 1,537 action conditions, 103 native scenarios with 758 calls and 9,311 conditions passed. Effect/child/transport/UI call decisions tested; their actual engine processing remains replaced by test receivers. No full commit, game host or installable mod. [Evidence](../../runtime/repair/EQUIPMENT_EVENTS_2949.md).

## Previous status: Inventory accesses for 2949, module 0.10.0

Own evidence for current inventory/owner getters, client selection, equipment slot selection and base fields. Six CTest suites, 385 reader conditions and 88 native scenarios with 578 views and 6,219 conditions passed. Original getters and readers agree on the same private sources; Reader build must be explicitly known. The equipment wrapper is tested with delta 0, not as a complete write/event path. Host/thread binding and repair transaction remain open. [Evidence and limits](../../runtime/repair/INVENTORY_2949.md).

## Previous status: direct acquisition from held references 0.9.0

Protected capture no longer reads an unprotected registry. The new common proof combines original 2949 reference/lock methods and the inventory reader on private legacy fixtures: 48 scenarios, 503 native calls, 3,867 conditions; In addition, 362 reader conditions and six CTest suites. Inventory layout/player/thread binding and transaction remain open. [Implementation and exact limits](../../runtime/repair/PINNED_CAPTURE_INTEGRATION.md).

## Previous status: joint protected acquisition 0.8.0

The new isolated sample combines original client/server lookups on 1.0.0.2949 with specific normal/user reference methods and Actor/PaPtr release. Actual Windows SRW locks protect the purchase. 30 Scenarios, 263 native calls, 3,490 Conditions and six CTest suites passed. These include competing registry write locks and temporarily removed entries. Direct acquisition accepts Alive=0; the separate reader must also reject this under the owner locks. Full live resolution, thread binding, change transaction and events are missing. Threadmap/cleanup remain fixture dependencies. [Details](../../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

## Source adapter 0.7.0 (previous)

Manager lookup and PaPtr sharing are implemented as contextual sources. 55 passed synthetic adapter conditions and a total of five CTest suites. The installed EXE has since been recognized as 1.0.0.2949 / Steam 25455892. The previous host refuses them before execution. New studies must not be mixed with the native evidence for 1.0.0.2944. [Adapter, update limit and remaining work](../../runtime/repair/REGISTRY_INTEGRATION.md).

Addendum to v0.5.9: Tables of the new build are tested and supported separately. Eight relevant native VTables were reassigned from the exactly hash-tested EXE. The new server lookup address is `0x2a82300`; previous preliminary v7 addresses will not be adopted. No new native execution takes place. [Reproducible static observation](../../runtime/repair/BUILD_2949.md).

## Reference management 0.6.0 (previous status)

Native reference documents are held during inventory collection and only released after inventory locks. Alive/destructive state and full identifier shall be controlled under lock. The original direct purchase can accept Alive=0; A valid document alone does not allow for repair.

The specific Actor VTables use `0x1436690` for release, not the base path `0x14345c0`. The override can trigger Zustand 0x20 → 0x40 and server-side cleanup. Native samples test these pathways with documented fixture dependencies. Four CTest suites, 249 reader/reference conditions, 203 native calls and 3,859 conditions passed. Protected live sources, host retention, transaction and installation are still missing. [Addresses, hashes, test limits](../../runtime/repair/REFERENCE_INTEGRATION.md).

## Owner lock 0.5.0 (previous status)

Non-waiting blocking group and inventory copies captured under it are implemented. Try `0x1371c20` and Release `0x1371bf0` are tested in the private test host with real Windows SRW imports. TLS of the try copy is redirected at exactly one read access to private thread data; No change to a game process.

Four CTest suites, 1,481 Action Conditions, 163 Reader Conditions, 25 Group Conditions; 140 native calls and 2,408 conditions passed. The separately examined Actor reference management is not yet integrated. Block group and readers already require owners who are protected against release. Also, complete engine transaction and installation are missing. [Methods, Hashes and Limitations](../../runtime/repair/LOCK_INTEGRATION.md).

## Final control 0.4.0 (previous status)

Asynchronous acceptance is now separate from confirmed repair success. Queue and post-review checking require matching feedback for job and session, both updated item images and confirmed engine events. Partial error, world change and timeout block further orders without retry or reset.

The `0x98fb70` client alignment modifies data before comparing it to the server message. The `0x240e3e0` function does not merely read bases: it removes used bets and returns their list. A later error leaves these changes. Six native cases check this, correct pre-repaired images and error code 0 without repair. Original lock constructor and base remover, Ack with exactly one TLS redirection; UI/Allocator/Lock dependencies are stubs.

1,481 Action Conditions, 124 Reader Conditions, 122 Passed native calls with 2,221 Conditions. No engine event actually shipped; No game transaction or installation. Table of methods, hashes, changes to the private function copy and limits: [ACK_INTEGRATION.md](../../runtime/repair/ACK_INTEGRATION.md).

## Reading adapter 0.3.0 (previous status)

The own reader follows the current client/server structure and provides the action with separate inventory/equipment images. Character Identifier, Owner, UID, Position and both Item Copies are checked; Control Reader, meanwhile, recognizes changed data. Unknown/ambiguous states are rejected.

Inventory Getter `0x212f4a0` and own readers deliver identical existing slots for the same artificial containers. The Holdergetter `0x212a0f0` confirms Actor/Possessor/Component paths. Server lookup `0x2a82730` and Typegetter `0x466060` are tested in isolation with artificial registry/actor locks. The lookup can keep a pointer if the lease is invalid; Validity byte is required. No real engine locks or game transactions implemented.

124 Reader checks, 1,377 Action checks and 116 passed native calls with 1,387 conditions. Current globals, structured paths, functional hashes, unwind chains and explicitly unconfirmed integration assumptions: [READER_LAYOUT.md](../../runtime/repair/READER_LAYOUT.md).

## Own action 0.2.0 (previous status)

In addition to the cost prototype documented below, the user-approved own repair action is developed. It repairs existing main items and socket inserts on private memory images; No repair rule and no material required. 1,377 Passed action checks.

The test host also loads a hash-bound copy of the item updater `0x240d650` (405 bytes). Positive delta arithmetic repairs base not reliable; Delta +65 sets a 10/100 socket to 0. Your own action sets every value independently. Original code then confirms the full durability, normal wear and the combination with no-wear. Thus, a total of 83 native calls and 1,252 conditions existed.

The game adapter is not yet implemented: neither its own operating input nor player resolvers, full engine transaction/notification or loader. The shifted manual in-game validation is separate from this. Technical details, layout deviations from older sources and integration contract: [ACTION_INTEGRATION.md](../../runtime/repair/ACTION_INTEGRATION.md).

## Cost prototype 0.1.0 (previous)

21.09.2026. User-approved runtime development for B8. Source code: `runtime/repair/`; No interference with the ongoing installation. The results complement the static [Field evidence](ADVANCED_TABLES.md).

## Binding to the build

EXE-SHA-256: `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. All following addresses are **RVAs**, without the preferred base `0x140000000`.

|Function|RVA|Longitude|SHA-256 of the complete body|
|---|---:|---:|---|
|Common Repair Helpers|0x240dfb0|437|61e7429f0415e591df434017a4500a6d93a4e059b11155741fd18ef5d4733aaf|
|Server-side repair helpers|0x2be3ee0|691|3f90895c1cb5e7fcdaa2599f74b18a0e6eba0b6b867f7668de2708e491d13bd5|
|Preparation of materials|0x27b04c0|280|Tied over full EXE hash and pdata boundaries|
|Execution of materials|0x27b05e0|630|Tied over full EXE hash and pdata boundaries|

The repository contains only structural information, hashes and own replacement instructions. Original code is only read in the private test host from the local EXE. No original bytes or game tables are shipped.

## Coordinated Candidate

|Area, end exclusive|Amendment|
|---|---|
|0x240e0d1–0x240e0ef|Negative checks, cost register EBX on 0, material division/clamp bypass|
|0x2be4052–0x2be4077|Appropriate check and cost register R14 on 0|
|0x2be40bf–0x2be4162|Do not attach any material deposition; Report success through the original epilogue|

Both complete functional copies are checked and prepared together. The cost helper `0x2411f20` remains unchanged. Maximum durability, repair quantity, resource rule, error paths and the original prologist/epilogues remain outside the three ranges byte-identical. Existing material orders are not removed or changed.

The two material transaction functions accept an empty list without further inventory access in the native test. The commit checks the count at `0x27b0618` and reaches 0 to `0x27b07dc`, where success is written. This is confirmed by the free material page within this cutout.

## Execution and limits

79 native function calls passed: 63 calls of the original/modified repair helpers and 16 prepare/commit calls for empty material lists. Covered are full repair, residual quantities, step sizes, large costs, original special quantity value -1, missing material, empty lists, inappropriate rule, full/unlimited durability, negative quantities/costs, authorization errors, existing material orders and reset to the original code.

At an artificial price 0, the original helpers reproducibly generate an integer division-by-zero exception in the test host. The coordinated candidate passes through the same inputs with correct calculated durability and cost 0. The errors were never triggered in the game.

Itemgetter, cost resolver and authorization service are stubs; Error codes are artificial constants. Real tables still do not contain a positive repair rule. The tests prove neither the accessibility of the repair UI nor actual update/synchronization of a game instance. A general hook, RepairAll, secure start activation and B0 installation are expressly not implemented. Therefore, no released mod function.

The new caller audit finds 16 direct references. In the former private scanner, a previous opcode-like byte sequence could swallow a real E8; The new Lookahead scanner with instruction check also finds `0x240e0cc`. Three synthetic regressions back up this behavior. Indirect calls remain outside the audit scope.

Evidence: `.local/repair-runtime-v1-{build-test.log,audit.json,validation.json}`. Technical basis for lateral protection and cache care: [Microsoft VirtualProtect](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect). For a later loader, the [Microsoft documentation for DLL initialization](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices) also applies.
