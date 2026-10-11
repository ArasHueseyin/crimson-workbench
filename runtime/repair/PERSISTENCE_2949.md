# Repair 0.16.0 – Slot processing and completion of the batch

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **Continued development module, no installed repair function.** The app remains v0.5.9. Game process, game installation and saves are not changed.

## Implemented financial statements

**Clarification from 0.18.0:** The SQL execution function referred to here as the persistence interface can deliver success without reading the request and does not execute a database task itself. Downstream processing and storage/charging semantics remain open. [Complete proof](SQL_DISPATCH_2949.md).

The common transaction adapter now requires `Environment::finish_unlocked`. If this binding is missing, the entire order is rejected before copying and writing. The call is made exactly once after all item messages, with released owner locks and still valid references/copies. The host can further process the collected changes in it. Only then will `pending` be returned.

An error or session change in this step means `unknown` because source values and messages have already changed or changed. may be shipped. There is no automatic repetition. `Report::finalized` only confirms the successful completion of this bound call, no permanent storage. The queue still requires freshly read values and assigned receipts.

In the native equipment sample, this step is connected to the original consumer of the slot table. The previously artificial table release has been replaced by the original Clear/Destructor functions. This is carried out exclusively on privately created test components; the Workbench has no permission to independently destroy a real game component.

## Accepted behavior of the current build

|Function|RVA|Full length|
|---|---|---|
|Server equipment, vtable `+0x130`: Consumers|`0x2ad1d70`|`0x6db`|
|Hash iterator for slot entries|`0x411cd0`|`0xb6`|
|Reusable emptying of the table|`0x40b7b0`|`0xdf`|
|Final release of the table|`0x410590`|`0xd0`|

Full hashes: [Manifest](tests/dirty_functions.inl). The Clear/Destructor functions each have five separately tested pdata fragments. No shortened functional copies. Total: 55 code ranges, 69 pdata fragments, 32 private TLS locations. The extension of the private TLS data also allows context access at `+0x250`.

The consumer acquires the owner lock, runs over marked slots and searches for their current equipment sites. Missing slots, invalid item keys and 0 quantity are omitted. It collects UID and 16-bit main durability, empties the slot table and releases the lock. He then transfers the collected order to the persistence interface. In the case of main durability 0, the order contains a corresponding marking; after successful processing, an additional counter/message path follows. This decision must be distinguished from the previous Broken Zustand of the separate repair event.

**The list is already empty before the persistence interface responds.**Rejected capacity, positive execution error and stored negative error code are checked. A new consumer call will then no longer find entries. An empty list or successful UI update must therefore not confirm a repair success.

Clear releases all slot payloads, but retains the node array for reuse. Another clear on an empty list is ineffective. New markings work with reset serial numbers. Finally, the destructor also releases the array and sets the table to the empty initial state.

## Test test

Neun CTest-Suiten, MSVC Release `/W4 /WX`, passed:

- Transaction adapter: **43 Scenarios, 273 Conditions**, including missing
Closure adapter, closure error, and session change after item messages.
- Native Slot Table Cases: Clear, Multiple Clear, Reuse and Original
Final release to the previous collisions, duplicates and growth cases supplemented.
- **24 new native consumer cases**, both allocation paths: empty list,
single/double marking, missing slot, invalid/used item, main value 0, or 65,535, 64 Slots, capacity and execution errors. Exact UID/durability pairs, unchanged items and full release checked.
- **18 integrated equipment cases**: field step, native post-copies, marking,
Server/client events and consumers in the common adapter. Three new cases confirm that persistence errors permanently block the order as unclear even after successful client alignment. No replay and no success from repaired fields alone.
- Total **186 native scenarios, 2,826 counted calls, 124,527 Conditions**.
Many conditions are repeated table checks, not independent scenarios.

## Remaining work

The persistence interface is its own limited receiver in the test. No database and no saves are opened. Actual storage, its lifetime, error handling and unique order feedback are still open. The load used here does not pass ** any base data records**. For the complete base repair, therefore, the proof of the supplementary persistence path is still missing.

Concrete inventory messages for items carried, real effect/transport/UI processing, verified game host/engine thread/TLS/world change, input, loader and B0 installation remain open. The existing results are not a confirmed live engine commit. Manual testing will be postponed to later as agreed; in particular repair, normal end of play and reloading from R06.

[Adaptor source code](src/transaction.cpp), [Consumer samples](tests/dirty_consumer_native.inl), [Integrated equipment sequence](tests/equipment_events_native.inl), [Testcheckliste](../../TESTCHECKLISTE.md). Evidence: `.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`. Research: `.local/repair-runtime-v16-{components,dirty-candidates,callers}.json`. Predecessor: [Prepared copies 0.15.0](TRANSACTION_2949.md).
