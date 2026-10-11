# Repair runtime prototype

Status: 23.09.2026, development module 0.23.0. The separate runtime development and a separate repair action for existing damaged items are expressly approved. **No repair mod installable yet.** The Workbench remains v0.5.9; Your repair switch remains locked.

**New build detected:** Steam 25455892 / EXE 1.0.0.2949 is installed and supported by the app for tables. A new, separately pinned native registry/reference sample exists on this EXE. The historical repair host remains tied to 25381195 / 1.0.0.2944 and refuses the new EXE. [Separate proofs of build](../../docs/BUILD_SUPPORT.md). [Current purely reading native assignment](BUILD_2949.md).

New in 0.23.0: Original inventory package serializer and stream selection connected. 36 new cases, including ten connections between transaction, sender and client Ack. In total 454 native scenarios and nine CTest suites passed. The package does not contain UID/socket/order data; fresh examination and delivery document remain necessary. Transport and server/store termination continue to be custom test receivers. [Current proof and limits](INVENTORY_PACKET_2949.md).

Historical status 0.22.0: Original buffer preparation and LZ4 compression connected with private heap. 66 other cases, total 418 native scenarios and nine CTest suites passed. Repeated entries after opening errors can be compressed twice. Downstream processing and file recipients remain test callbacks; Actual storage and game host are still open. [Current proof and limits](SAVE_ENCODING_2949.md).

Historical status 0.21.0: Original filewriter and buffer cleanup performed on private objects. 34 new cases, total 352 native scenarios and nine CTest suites passed. Even a failed flush can lead to success and emptied payload. File/coding interfaces remain test callbacks; Actual storage and game connection are open. [Current proof and limits](SAVE_FILE_2949.md).

Historical status 0.20.0: Original save dispatcher, Steam approval check and entry reservation checked on private data. 39 additional cases, total 318 native scenarios and nine CTest suites passed. Skipping orders can report success and retain old status values. The real file path and timers are statically assigned; Actual storage remains open. [Current proof](SAVE_BACKEND_2949.md).

Historical status 0.19.0: Original item/storage object converters with construction, vector growth and release executed on private objects. 38 new cases, including 14 with productive repair planning/native post-copy, check main and base values, no-wear normalization, free spaces, staining dates, and reset and reuse. Nine CTest suites and a total of 279 native scenarios passed. The regular saving process and the game connection remain open. [Current proof and limits](ITEM_SAVE_2949.md).

Historical status 0.18.0: Full SQL execution function and request purchase checked in isolation. The function does not execute a database request; your bypass reports success without reading the request. 19 new native cases including slot consumers also prove received old errors. Nine CTest suites and 241 native scenarios passed. Actual storage is still open. [Detection and limits](SQL_DISPATCH_2949.md).

Historical status 0.17.0: Original inventory-client-Ack integrated into the private common process. 36 new native cases check absolute main values, slot errors, UID changes and missing/double feedback. The call does not have its own UID or target value check and does not replace a server/store confirmation. Nine CTest suites and 222 native scenarios passed. The examined socket SQL helper transmits item identifiers; a durability storage is thus not occupied. [Current proof and limits](INVENTORY_EVENTS_2949.md).

Historical status 0.16.0: A joint conclusion follows after all item messages. In this case, the equipment sample processes the marked slots with the original consumer and empties, i.e. destroys their private tables with original functions. Errors in the forwarding to the persistence interface do not confirm success, even if the UI message has already arrived. Nine CTest suites, 43 expiration scenarios and 186 native scenarios passed, including 24 new consumer cases. Actual persistence and its complementary socket path, concrete inventory events, real recipients/feedbacks and the game host remain open. [Current proof and limits](PERSISTENCE_2949.md).

Historical status 0.15.0: Joint process with complete pre-checking of all message paths and native postcopies before the first source letter. Field recorder, slot marking and equipment events are associated in the 15 native event cases. Empty/partially stored base positions and additional vector strides corrected. Nine CTest suites, 40 execution scenarios and 159 native scenarios passed. Concrete inventory events, slot consumers, real receivers and game host are missing. [Current proof and limits](TRANSACTION_2949.md).

Historical status 0.14.0: Full field recorder for items carried and equipped under held owner locks. Complete pre-checking before the first store, overlap/capacity checking and checking the entire reading stock afterwards. Eight CTest suites, 34 writing scenarios and 141 native scenarios passed. The connection with native aftercopies/events to the full engine transaction and the game host is still missing. [Current proof and limits](FIELD_WRITER_2949.md).

Since 0.13.0: Included capture for planning and re-examination under the same owner references and locks. Original marking of changed equipment slots including collisions and growth integrated into the event test. Seven CTest suites, 519 reader conditions and 133 native scenarios passed. The full writer, consumers of slot messages and game host are missing. [Current proof and limits](DIRTY_SLOTS_2949.md).

Since 0.12.0: Native item values with fixed construction, low assignment and release. base/additional data have independent allocations; Erroneous headers are rejected before the target change. The event sample now uses original item copies. Seven CTest suites, 30 Owner Conditions and 117 native scenarios with 851 Calls and 13,639 Conditions passed. Full writing path, slot invalidation, event processing and game host remain open. [Current proof and limits](ITEM_LIFECYCLE_2949.md).

Since 0.11.0: Event metadata from the repair plan, with active no-wear and previous broken-Zustand. Original Server Notifier and Client Ack checked together on private repaired items, including incorrect feedback. Six CTest suites, 1,537 Action Conditions and 103 native scenarios with 758 Calls and 9,311 Conditions passed. Effect/transport/UI receivers remain test dependencies; Full game transaction and host are still missing. [Current proof and limits](EQUIPMENT_EVENTS_2949.md).

Since 0.10.0: Own proof of inventory/owner getters, client selection, equipment slot selection and base fields on 2949. The reader demands a known build. Six CTest suites, 385 reader conditions and 88 native scenarios with 578 views and 6,219 conditions passed. The equipment test uses Delta 0; Full changes/events and the game host remain open. [Current proof and limits](INVENTORY_2949.md).

Since 0.9.0: Inventory capture directly from the held references, without second unprotected registry search. Valid identifier and living owners are mandatory. 362 Reader requirements and six CTest suites passed. The common native sample includes 48 scenarios, 503 calls and 3,867 conditions with real registry and owner locks. Your inventory layout is a private fixture of the previous reader, not yet a layout release for the new build. [Current proof and limits](PINNED_CAPTURE_INTEGRATION.md).

Since 0.8.0: Protected client/server search, concrete normal/user reference methods and shared natively executed sharing, with real Windows registry locks on private test objects. 30 Scenarios, 263 Calls, 3,490 Conditions and six CTest suites passed. Current player/manager and engine thread binding remain open. [Detection and limits](REGISTRY_NATIVE_INTEGRATION.md).

Since 0.7.0: Registry source adapter with its own acquisition/release binding per manager; No global release method. Five CTest suites and 55 new conditions with artificial sources passed. [Registry contract and then build change](REGISTRY_INTEGRATION.md). [Concrete later test checklist](../../TESTCHECKLISTE.md).

## Previous reading game adapter and own repair action

Since 0.6.0: Management of native actor references and capture under references and inventory locks. Errors first return locks, then references. Owners who have been registered or are already in destruction will be rejected. The concrete actor release path is tested in isolation, including pending Zustandcleaning. The live acquisition from protected sources still remains open. [Reference contract and test limits](REFERENCE_INTEGRATION.md).

Since 0.5.0: Shared, non-waiting owner locks and associated inventory collection. Original lock methods are tested in the private test host with real Windows SRW locks, recursion and competing threads. The locks alone do not ensure the life of the game objects. [Contract, Native Evidence and Limits](LOCK_INTEGRATION.md).

Since 0.4.0: accepted repair orders can wait for confirmation. Order number/session, freshly recorded after values and confirmed engine events must fit. Until then, no success and no second order. In case of partial errors, world change or timeout, the queue is locked without repetition. The native client path can remove sockets before an error message; This case has now been proven isolated. [Confirmation and native sample](ACK_INTEGRATION.md).

`crimson_repair_reader.lib` reads the documented client/server layout and copies existing inventory/equipment sites into private images. Since 0.10.0 there are own proofs for both builds 2944 and 2949. Identifier, owner, UID, position and both Zustandcopies must fit; Changes in control reading discard the capture. No heapscan and no game access from the Workbench. The native inventory function and your own reader match on the same artificial containers. [Layout and boundaries](READER_LAYOUT.md).

`crimson_repair_action.lib` implements single repair and RepairAll for carried/equipped items including damaged base inserts. Empty repair lists and missing material are not a prerequisite for this own way. The action does not create any lost items and does not change quantities or other properties. The existing No-Wear-Override can be combined.

Planning, testing of both Zustandcopies, complete pre-checking of a batch, application to own memory images and a queue protected against duplicate/delayed inputs are developed. The common transaction adapter has existed since 0.15.0. **The complete game connection is missing:** The executable backend uses only private test data; Real receivers and hosts are not connected. The library is not loaded by the app.

Historical native status 0.6.0 / EXE 1.0.0.2944: 249 Test conditions for readers and reference management, 25 for the blocking group and 1,481 for the action passed. In addition, an isolated copy of the real engine updater correctly reads the fields repaired by its own action. Total 203 native calls and 3,859 test conditions in the native host, four CTest suites, MSVC release `/W4 /WX` passed. The original updater is unsuitable for sockets with positive delta; This regression is reproduced.

Exact scope, layout proofs and still required engine integration: [ACTION_INTEGRATION.md](ACTION_INTEGRATION.md).

## Existing cost prototype 0.1.0

`CrimsonRepairPrototype.dll` creates a coordinated change candidate for two fully hash-tested functional copies of the build Steam 25381195, EXE 1.0.0.2944. The common and server-side helpers retain their repair volume calculation, validity checks and durability limitation. Material costs are set to 0, their division and material limitation are bypassed. Negative costs and quantities below -1 are rejected. The original special quantity value -1 is retained. The server does not create a material order.

The library only processes separate data buffers. Both full function hashes must fit before an output buffer is changed. Unknown bytes, an already modified function, incorrect ABI, incorrect lengths, zero pointers and overlapping buffers are rejected. The caller must provide valid buffers that are unchanged during the call.

The native test host reserves its own memory and only performs copies of the selected functions. Item definition, instance, repair rule, material inventory, cost resolver and authorization service are artificial test data or Stubs. The empty material transactions are also executed by the original, unchanged prepare/commit function copies. Entries remain unchanged; Resetting the functional copies restores the normal calculation.

**Previous detection 0.1.0:**79 passed native calls, 1,236 test conditions, CTest contract test and three Python regressions. The two intentionally executed zero-cost divisions of the original code are intercepted exclusively in their own test host as expected exceptions. No gaming process is opened, started, stopped or changed.

## Still open / Delineation of the two paths

- The 6,816 item repair lists of the current build are empty. The prototype
receives the original rejection of such lists; his positive tests explicitly use artificial, non-empty rules with positive repair amount. The new own action does not need these lists.
- Upstream UI/server callers require an existing repair material.
They are not being circumvented here. Quantity 0 remains a rejection.
- Further original UI calculations and RepairAll have not yet been converted. The Global
Cost helper remains unchanged as more callers divide by its value.
- The reading resolver and the isolated tested protected reference acquisition are
developed. The actual manager/player/thread binding as well as the complete transaction including inventory/UI notifications are missing. A dedicated operating input is also not yet connected.
- Loader, synchronized activation at the next game launch and installation/
Restoration over B0 is missing. The DLL has no `InitializeASI` function, no automatic hook and no installation command.

`crimson_repair_installable()` therefore always yields `false`. Do not include the prototype as an activatable option in the Workbench. The release for development already exists; This is a technical loophole, not a pending consent. Farm mode hotkey and phase 6 are not part of this repair job.

## Reproduced

Windows x64, Visual Studio 2022 C++ Build Tools, Windows SDK and CMake required. From the project folder, first only the synthetic contract tests:

```powershell
./runtime/repair/test.ps1
```

The Windows step in `.github/workflows/core.yml` also runs these six suites without game files. The native EXE detection remains a separate local sample; no game content is required or provided in CI.

Optionally specify the current pinned EXE 1.0.0.2949 for the registry sample exclusively as a read source:

```powershell
./runtime/repair/test.ps1 -RegistryGameExe "$env:CD_GAME_DIR/bin64/CrimsonDesert.exe"
```

The separate switch `-GameExe` is still intended exclusively for the historical host and EXE 1.0.0.2944. Both selectors demand their own hash.

Output remains below `.local/repair-runtime-build/`. The complete EXE is hashed before and after the sample. Only selected functional bodies are copied into private memory areas; the EXE is neither loaded nor started. Code pages are when running RX, when filling RW. The test host registers the appropriate Stack-Unwind information. In the historical host, the debit commit only runs the path for an empty list. Its C++ cleanup handler and the cleanup handler of the non-throwing stub-tested registry/client rack are not registered. Inventory/socket-unwind chains are completely preserved. In the client alignment, only the checked TLS read access in the private copy is redirected to artificial Allocator data; Details in the Ack-proof. This is not a general engine emulator.

The additional caller audit requires the locally available packages `pefile` and `capstone`:

```powershell
python -m unittest discover -s runtime/repair/tools -p test_audit_build.py
python runtime/repair/tools/audit_build.py "$env:CD_GAME_DIR/bin64/CrimsonDesert.exe" > .local/repair-audit.json
```

The audit finds 16 direct E8/E9 calls/jumps to the three investigated helpers and confirms their instruction limits. It does not cover indirect calls. Overlapping opcode-like bytes can no longer obscure real references. The import list confirms XInput ordinals 2/3 and two DXGI factory functions; No tested loader solution follows from this.

Other documents: [Maturity field proof](../../docs/research/REPAIR_RUNTIME.md), [Progress](../../docs/PROGRESS.md), [Residual points](../../docs/PHASE5_REMAINING.md).
