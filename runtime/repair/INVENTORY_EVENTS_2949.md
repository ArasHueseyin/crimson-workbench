# Repair 0.17.0 – Client feedback for items carried

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **Development module, still no usable repair in the app.** The desktop app remains v0.5.9. No game process, installation or save change.

## New connection

The original Inventory Repair Ack `0xa11ca0` is now running in a private inventory route of the common transaction adapter. Original item copies, field recorders and reference/lock functions work on the same own sources. The server part of this inventory route and its completion remain its own test callbacks; A complete engine transaction is not claimed.

The Ack assumes an inventory key, slot and **absolute 16-bit main value**. It translates the key, searches for the inventory owner, acquires its exclusive lock and checks the slot for existing item, valid key and positive amount. After the main value store, it releases the lock and reports the interface `Zielwert - vorheriger Wert` as 16-bit difference. Caps shall not be modified or transmitted in this message. Definitions, key translation, name formatting with empty test name and interface are limited test receivers. Non-empty names and their native string allocation branches are not listed here.

The common writer has already updated both Zustandcopies before the notification. The native Ack therefore writes the same main value and reports delta 0 in this integration sample. The actual response of the surface to delta 0 remains open; the UI test receiver only checks the call data.

## Why feedback alone is not enough

- **No UID check:** The Ack accepts another existing item in the
Same slot. The native direct case demonstrates this behavior; in the integration, a new protected capture subsequently refuses success. The later game route also requires an identity check before delivery.
- **No target value allowance:** Also 0 and 65,535 are written unchanged.
Only the tested own repair plan may determine the transferred target value.
- **No storage:** The alignment changes the client representation and does not replace
the server change or the persistence order or its confirmation.
- **External package status is not enough:** The statically tested package distributor
`0xbbee00` calls the inner ack, but then sets its outer result code to 0 independently. The host must not use this external success as evidence of a successful repair. This distributor was not exported.

## Build documents

|Area|RVA/Length|SHA-256|
|---|---|---|
|Complete native client ack|`0xa11ca0` / `0x322`|`391c116f3606894b6a2a4de7623e51a56d8e05111099597c12a04b998486abe4`|
|Static packet distribution anchor|`0xbbee00` / `0x379`|`5ac29646aa893ceb1983e76c63425142ca493bfe464a398f663bfb5840a3505d`|

RTTI `TrocTrRepairItemToInventoryAck`: vtable `0x5609728`, method `+0x10` leads to the distributor. This uses Actor components `+0xd8` for the repair component. The sample checks this access with precision. This is not proof of the actual life of a loaded game component.

The private host now has 56 fully pinned code ranges, 70 pdata fragments and 35 private TLS locations. The three new TLS positions are in the corner; unchanged bytes outside the permitted diversions are checked.

## Test test

Nine CTest suites and MSVC release `/W4 /WX` passed. The existing production suites remain at 43 transaction scenarios/273 conditions and 35 writing scenarios/202 conditions.

**36 new native inventory event cases:**

- 14 Direct cases in both TLS allocation modes: damaged, durability 0,
Already full value, target value 0/65,535, invalid translation, missing container, negative/too large slots, invalid key, set 0/-1, excluded items and an exchanged UID. The two modes do not achieve string allocation when the name is empty; They are not a test of both string allocation branches.
- Eight integration cases: normal, only base damaged, no-wear,
incorrect container translation by field step, UID change, missing delivery, failed completion and duplicate feedback.
- Exact change only of the main word in the direct case; inventory metadata;
Base, UID, quantity and server data will otherwise remain unchanged.
- formatting under client lock, UI call upon release, owner held,
native copy lifetime and complete cleanup checked.
- All errors after the field step produce an unclear result without replay.
Fresh data and complete artificial evidence are necessary for private success.

Total **222 native scenarios, 3,526 counted calls, 125,524 Conditions**. Many conditions are repeated table checks, not independent cases.

## Further knowledge on the socket storage path

The read-only analysis finds `sql->updateItemSocket` in the helper `0x2aefef0` (length `0x1d6`, SHA-256 `0b47ab9486b4a2f8de6a5efe1b5ff3146c420ac2cf3cabe36f169998b1178490`). The occupied callers `0x2ae91a0` and `0x2aea3a0` pass a 32 bit ItemInfo identifier and the base index there. The durability is separate in the temporary 24 byte dataset at `+0x10`; the SQL handover reads its identifier at `+0x14`. Subsequently, `0x240e310` sets key and durability in the 6 byte socket dataset.

This SQL helper is therefore **not a busy memory call for base durability**. It is not included as a supposed repair. There is no indication from this as to whether or where other storage paths ensure base durability, i.e. Re-initialize when loading. This must be further proven by the actual read/write paths; Save files are not opened for this.

## Open

Concrete server-side inventory reports and persistence, full base persistence/load semantics, real UI/effect/transport processing and assigned order receipts. After that, game host/thread/TLS/world change, input, loader and B0 build-in remain. General NPC respawn times also remain a residual point of phase 5.

[Native inventory samples](tests/inventory_events_native.inl), [Phasenstand](../../PHASENSTATUS.md), [Testcheckliste](../../TESTCHECKLISTE.md). Evidence: `.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`. Research: `.local/repair-runtime-v17-{rtti,socket-references,callers}.json`.
