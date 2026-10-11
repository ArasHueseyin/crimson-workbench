# Repair 0.11.0 – Preparation and verification of equipment events

Historical evidence of events. The sample now uses native copies of items: [Development module 0.12.0](ITEM_LIFECYCLE_2949.md).

Status: 22.09.2026. Development module for Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. The app remains v0.5.9; Free repair is still not unlocked.

## Implementation

`action::equipment_notice` gets the event data from the immutable repair plan: session, exact item/slot identity, and broken-Zustand before and after the change. The calculation corresponds to the original equipment logic: active maximum durability unequal to `0xffff` and **signed** current durability <= 0. The active no-wear override is thereby taken into account. Any subsequent value is not enough to reconstruct the previous Zustand.

Missing or postponed items, unprepared plans and items carried will be rejected; Old output data will be deleted. The function does not read game pointers and generates neither an event receipt nor an access share. Also `is_broken` is output: A pure base repair on a main item that is still broken in the engine sense must not be treated as a Broken→Repaired change.

## Native connection in the private test host

The test combines your own planning and queue with the original server notifier and the original client alignment. Existing native Actor references are maintained throughout the sample. The repair values are copied exclusively into own test objects under both native owner locks. Before the notifications, both blocks are released; The callbacks are checking this again.

Both test representations independently receive their correct item/socket values. Then the notifications run with **Delta 0 and empty distance list**. The standard positive updater remains unsuitable for repair because it can consume bases. Delta 0 also does not repair anything.

|Original function|RVA|Bytes|Implementation|
|---|---|---:|---|
|Server-Equipment-Notifier|`0x2adfda0`|1,429|Repaired/unchanged states, effect call, optional child event call and packet boundary|
|Event initialisation|`0x20efb50`|177|Original field initialisation; Runtime constants as private fixtures|
|Client-Ack|`0x98fb70`|773|Delta, Base List Check, Error and UI Call|
|Destructive socket collector|`0x240e3f0`|465|Three unwind fragments tested; removes existing finite bases in durability <= 0|

The current server VTable `0x5b28f80`, slot `+0x190`, refers to `0x2adfda0`. The full function hashes are in `tests/registry_native.cpp`. The entire manifest includes 28 code ranges, 25 pdata fragments and eight targeted private TLS redirects. Original C++ cleanup handlers are not registered; Fixture dependencies do not throw exceptions. The native repaired branches require activated AVX, which the test checks before execution.

## Results and error cases

Six CTest suites and MSVC release `/W4 /WX` passed. Total **103 native scenarios, 758 counted calls and 9,311 test conditions**; 1,537 Action, 385 Reader, 25 Lock Group and 55 Source adapter conditions. Without EXE, only 84 manifest conditions are checked, no original functions are executed.

15 new integration cases:

- Captured main item, normally damaged item and pure base repair.
- No-wear and unlimited main item; No invented Broken→Repaired events.
- Original Repaired Branch passes Zustandsart 3, correct slot and item
the effect processor. Optional child events include matching identifier and fixture time; missing effect feature skips the processor.
- Missing package delivery, wrong distance list and missing client slot.
Even error code 0 in the absence of a slot does not produce any repair success.
- Unrepaired client despite UI call; a base still standing on 0
Already removed before the list error. Return error does not mean return.
- Missing event document, changed session and duplicate delivery. Only suitable
Subsequent images **and** complete documents may complete the order.

After partial error or timeout, the queue remains locked, without repetition or speculative resetting. Quantities, identities, base occupancy and other items are fully verified for successful cases.

## Precise limit of detection

**This is still not a complete game transaction.** Transport at `0x29ead70`, actual effect processing at `0x17b1d30`, child reference list, clock, table resolver, UI receiver and memory sharing are test dependencies. The sample confirms the call decision and arguments of these interfaces, not their effect in the game, a real package shipment or persistence. Also the callback assignment to order numbers is only given in the private test.

In particular, it remains open:

- Engine thread, manager life, current player engagement and world change.
- True item writing path including engine-owned item copy/lifetime; Two crude
Memory changes and the sequence of events shown here are not sufficient as release.
- Slot invalidation (`+0x198`), full effect/child/transport processing,
still broken down poster states and inventory events for items carried on board.
- Real, unambiguously assigned confirmations; Input, loader and B0 installation.

No game processes or saves were opened and no installation files were written. The desktop app was not rebuilt for this runtime change.

Reproduction: `./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"`. Local evidence: `.local/repair-runtime-v11-build-test.log`, `.local/repair-runtime-v11-native-result.json`, `.local/repair-runtime-v11-validation.json`. Later manual cases: [TESTCHECKLISTE](../../TESTCHECKLISTE.md), in particular R01–R10.
