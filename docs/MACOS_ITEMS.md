# Native Mac inventory spawning

Goal: use the existing Item Picker to grant an exact item quantity through the
native game's inventory transaction, with receipts and no automatic retries.
Completion requires observing the additional quantity in the inventory and after
a normal save/reload. Building a module alone does not establish that spawning works.

## Scope and checks

The changes must reach the ARM64 runtime, its per-launch loader, local transport,
Rust `spawn_service`, existing request/receipt validation, Item Picker capability
labels, native fixtures, and packaging. Reuse the existing protocol and queue.
No Windows addresses, TLS layout, calling convention, or object sizes carry over
without independent Mac verification. Existing file and save write guards stay.

First prove a read-only runtime observation on a loaded save. Then implement
native item construction, server-thread execution, and inventory-delta checking.
Only after those pass may Workbench advertise spawning as available. Any uncertain
result must block new grants for that game session. Back up saves before a live
grant; the game may autosave inventory changes.

## Current executable observations

Observed locally on native Steam version 1.0.35, bundle build 338:

- ARM64 Mach-O UUID `0A2EA57A-719E-36B4-8AB1-EDA609AC004E`.
- Executable SHA-256 `fc0727d176ad7434e9fcea5b358cdb6b7c78ebb1f73975b76efb100c1c129a74`.
- Existing entitlements allow DYLD environment variables and disable library
  validation. A per-launch diagnostic library can therefore be attempted without
  modifying or re-signing the game, or changing system security settings.
- Most inventory methods are stripped. C++ RTTI and chained fixups identify
  `TrocTrCreateItemFromTrItemValueCheatReq` and its decoder at `0x1009e01d0`.
  The decoder calls the server handler at `0x100898fbc`, which reaches a candidate
  native grant transaction at `0x100b7da10`.
- The native InitData constructor is at `0x101c87774` and initializes through
  `0x1b4` bytes. The Mac grant reads the external item key at `+8` and quantity
  at `+0x10`; other fields still require independent verification.
- `TrocTrMoveActorReq` dispatch slot is `0x107e3e3f8`, originally pointing to
  `0x10098d510`. The callback has two arguments and returns a 32-bit result.
- The `ServerChildOnlyInGameActor` primary vtable begins at `0x107e2fe10`.
  The inventory-holder getter at `0x101c0ba14` uses an actor component at
  `actor+0x108` and holder at `component+0xb8`. These are candidate observation
  checks, not authority to write inventory or proof of the selected player.

## Historical live diagnostic progress

The exact-build read-only observer reached native movement dispatch on a loaded
save and confirmed the server actor class and holder-owner round trip. No grant
endpoint exists, and no items have been granted. The candidate inventory reader
still needs calibration against the game UI: the user reports 5 Fine Timber
and probably no ordinary Timber in Kliff's inventory. The first live read returned
13 Timber and 90 Fine Timber. The user then confirmed 5 Fine Timber again with
a screenshot. These totals are not calibrated and must not permit a grant.
The next probe separated bucket totals and took five identical readings: Fine
Timber is 5 in bucket 0 and 85 in bucket 5; ordinary Timber is 13 in bucket 5
and absent from bucket 0. This explains the aggregate mismatch. Bucket 0 agrees
with Kliff's visible inventory; bucket 5 semantics and the independent local
player identity link are still being checked. This remains a read-only probe.

The subsequent identity probe follows client global `0x108a6a938`, root `+0x10`
to exact `ClientActorManager` vtable `0x10833e4e0`, manager `+0x100` to exact
`ClientUserActor` vtable `0x10833e2c0`, and user `+0x220` to exact
`ClientChildOnlyInGameActor` vtable `0x10833cd88`. Its 32-bit actor key at
`+0x100` matched the server context actor's key at `+0x100` in the loaded game.
The `+0x130` server field previously compared with controller `+0x100` is an
ownership association and must not substitute for that actor-key comparison.

## Historical one-item diagnostic

The initial diagnostic defaulted to read-only. Its development-only per-process flag
`CW_MAC_ITEMS_TEST_ONCE=one-timber` arms at most one attempt, with no retry or
general grant endpoint. It additionally requires server TLS mode, the server
controller's `+0x220` actor association, the client/server identity match, and
the exact observed inventory baseline. The app does not expose this experiment.

Item preparation uses native typed constructor `0x10199c500` with an internal
item type and quantity, then default InitData constructor `0x101c87774` and
native exporter `0x1019a27a4`, then ItemVal destructor `0x1019a04e4`. The exporter
accepts a scalar 16-bit slot sentinel and fills fields the default constructor
does not, including `InitData+0x5e` from `ItemVal+0x70`. Do not build requests by
setting only a key and quantity in a default-constructed InitData.

The first test accepts zero active socket/extra-record lists. The constructor's
five-entry backing list at `ItemVal+0x68` is normal even with active count zero.
General export bounds are active byte `+0x70 <= 5`, 32-bit list count `+0x80 <= 12`,
and 32-bit list count `+0xb0 <= 3`; broader item support remains unverified.
The native grant is two arguments `(holder, InitData*)`, returns a 32-bit result,
and is at `0x100b7da10`. A successful experiment requires observed on-hand Timber
to change 0 to 1, Fine Timber to remain 5, and bucket 5 to remain 13/85. The game
UI and save/reload still need independent confirmation after any native grant.

Native bucket destruction at `0x101c23e84` independently confirms holder `+0x18`
bucket pointer array, holder `+0x20` 32-bit count, bucket `+0` slots, bucket `+8`
32-bit slot count, and `0xc8` item stride. The reader's initial 16-bit slot count
has been corrected, with a regression test rejecting 65536 slots. Native item
validation at `0x1019a08d0` reads type at `+8` and signed 64-bit quantity at
`+0x10`. These structural checks alone do not resolve the visible count mismatch.

`ServerUserActor` has primary vtable `0x107e30310`. Its `+0x218` field is a
registered type-1 actor association: native code preserves it when already set
and clears it when that actor is removed. It must not be described as proof of
current local selection. A zero movement decoder result also does not prove
accepted movement; rejected and skipped requests can return zero.

## Verified first grant, 23 October 2026

The user authorized one ordinary Timber. Eleven save files were freshly backed
up and hash-verified in `.local/macos-items/backups/20261023T102903Z-before-one-timber`
before launching the armed diagnostic. Native preparation reported type 1243,
quantity 1, backing list 5, and zero active/extra/property lists. Exactly one
native grant was submitted; the post-transaction delta check passed.

The user independently confirmed Timber x1 and Fine Timber x5 in the game UI,
then saved and exited. A subsequent launch with the read-only library (no armed
grant code) read Timber x1 and Fine Timber x5 again. Bucket 5 remained Timber 13
and Fine Timber 85. Logs are local in `.local/macos-items/game-one-timber.log`
and `game-persistence.log`. No additional grants were submitted.

This proves a native single-material spawn and persistence on this pinned build.
This experiment alone does not establish general item/quantity support or other
native features.

Addresses are unslid virtual addresses for this exact executable. They are not
cross-version signatures. Local disassembly and metadata remain private; no game
bytes are distributed.

## Item Picker integration

The production `libCrimsonItems.dylib` retires the one-off environment flag and
accepts only explicit app requests. The Mac launcher requires the game to be
closed, verifies the executable hash, copies discovered save folders into the
app's local data directory under `item-backups/`, verifies each copy against
SHA-256 before and after copying, and records `workbench-backup.json`. A partial
or failed backup prevents launch. Keep Steam open. No game bundle files or OS
security settings are changed.

The local socket lives in an owned mode-0700 directory, with a mode-0600 socket.
The app verifies its peer UID and PID before sending any request. The runtime
reuses the existing bounded request queue and receipts; a request ID is never
replayed. Inventory execution occurs in the verified native server callback,
with local actor identity, owner, holder and server TLS checks repeated before
submission. No engine pointer is retained by the transport thread.

Only items targeting native InventoryKey 2 (Character inventory) are accepted.
Special-storage destinations are rejected. Quantities are limited to 10,000;
non-stackable items to 100. Native value export additionally requires bounded
socket and property lists. Both carried inventory (bucket 0) and aggregate
inventory must increase by exactly the requested quantity. Any uncertain native
result blocks further grants for that game session. A lost reply retains the
request ID and permits cancellation or receipt polling, not acknowledgement of
a possibly queued request. After an old game session ends, inventory must be
checked before acknowledging its unknown saved outcome.

Builds compile the native module automatically and bundle it as a Mac resource.
Fixtures cover malformed contexts, sparse buckets, serializer bounds, destination
restrictions, post-grant delta checks, request replay, socket identity and save
backup failures. The completed live proof remains ordinary Timber x1; broader
item behavior and the app-driven live path still need hardware confirmation.

Integration verification (23 October 2026): the Rust workspace passed, followed
by nine focused spawn/backup/process tests, all five Mac native test programs,
50 frontend unit tests and 16 Item Picker/setup browser tests. Removing the poll
identity guard made the new lost-reply race regression fail; restoring it passed.
The packaged native app opened and loaded all 6,816 items. Its real launch command
refused the already-running diagnostic game, and its grant button stayed disabled
while disconnected. The final app passes strict deep code-signature verification
with a local ad-hoc identity; the DMG checksum verifies. Developer ID signing and
notarization have not been performed. The app-driven live connection check is
pending normal exit of the read-only diagnostic game; no second grant is authorized.
