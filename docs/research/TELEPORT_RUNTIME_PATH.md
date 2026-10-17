# Waypoint and saved-position teleport

17 October 2026. Planned point #14 has a local native implementation and owned
offline fixtures. It has not been installed, injected, or accepted in the
running game. The original executable/resources were opened read-only; copied
native code executes only in this tool's own fixture allocations, with external
calls replaced by owned stubs. No game process, game directory, or save was
modified for this work.

The adapter captures bookmarks, lists the selected server player's map pins and
waypoints, resolves eligible mainland markers through native ground/placement
queries, and submits the game's real field transition. It does not write a
visual/physics position directly. Alternate map layers, unknown marker heights,
missing geometry, unsupported player states, and uncertain native calls fail
closed. In-game acceptance and broader layer/geometry support remain separate
from the passing offline checks.

## Original build and ownership

All RVAs refer to executable SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
The module verifies that file hash, PE identity, relevant function bodies, and
current virtual slots. Other builds are disabled.

`CrimsonLiveItems.asi` already owns the `2774b20` hook and exports the V1 leased
task observer. Teleport registers that observer; it adds no duplicate hook.
Every engine call runs synchronously on that borrowed task thread. The pipe
worker handles pointer-free requests, snapshots, and bookmark storage only.
Engine pointers never enter IPC or persistent bookmarks and are not retained
after the callback.

The frame checks server realm, original task callers, selected identity, player
metadata, actor/possessor round trip, component owners, field world ID, native
move eligibility, and transform transition state. Transform virtual slots
`+140/+148` resolve to `2bc4280/2bc42c0`; their borrow belongs to the exact
transform, with generic reference cleanup at `1434880` afterward.

Native C++ exceptions propagate. A frame guard releases a fully acquired known
lease on pre-Begin unwind. Unknown acquisition, Begin/publication, or hardware
faults latch the module without speculative cleanup, repetition, or fallback.
A secondary cleanup exception does not hide the primary exception. Normal
explicit cleanup errors also propagate.

## Captured positions and field transitions

`4f8220` computes compressed global position, including parent translation,
rotation, scale, and signed sector transforms. Decoding uses signed 16-bit X/Z
sectors and native 1000.0f sector extent. `17e9170` computes yaw including the
parent quaternion. Their input values are copied under the actor read guard;
arithmetic runs on a private value copy afterward. Transform `+c8` is packed
value data, not a retained parent pointer.

Old field names remain in the local format: `field_definition` is the external
**FieldInfo key**, and `stage` is the internal **FieldInfo index**. `4a2010`
reads `fieldinfo`, not `stageinfo`; its record's first u32 is the external key.
The transform's index must match the acquired field's `2af7d10` result. The
original table identifies key 1 as MainField. A saved return currently requires
the same FieldInfo key and index as the current character.

The actual transition is established by normal player/gimmick callers:

1. `23e91c0 → 2aa7b40` constructs the 0x38-byte field-move value from FieldInfo
   and character keys. Player routes `29226f0` and `2bc5320` also use its `+18`
   transition variant.
2. `2bbf630 → 106db020` deep-copies that value under the native write guard,
   requires an idle context, and publishes phase 1. The outstanding field
   borrow defers streaming while the accepted typed value is checked.
3. Normal release `2bbf140` decrements the borrow. The last release advances
   the native transition and calls `2bbf8c0`, queuing the normal field operation.
4. `2bbfa80` loads/resolves the field and applies the requested transform through
   the normal character lifecycle. `2bc0d00` / `2bc12a0` may select quest/login/
   spawn overrides. They are **not** a generic marker-to-ground projection.

The adapter supplies the current character definition, same FieldInfo, native
captured/resolved XYZ and yaw, `flag_18=1`, and no optional character list. It
revalidates actor/world/character/FieldInfo/realm and idle transition state
before one mutation attempt. Native acceptance means **queued**, not arrival.
A separate later position observation within 0.5 units has its own receipt.
World/identity changes, native spawn overrides, expiry or unknown publication
can produce uncertainty; the client never auto-retries.

## Marker source, world, layer, and height

Server component `+168` is `ServerContentsMiscActorComponent`, vtable
`5b1d318`. Normal pins are 0x18-byte rows in `+c8`; waypoints use `+d8`.
Rows contain full u64 ID, XYZ, shape and color. Capture uses the selected
actor's read guard and checks owner, vector counts/capacity, finite values,
metadata and duplicate source/ID pairs. Limits are 500 pins plus 20 waypoints.
Native reserve can exceed the gameplay count limit.

The original producer is `d91ef0 → b84320`, packet 0xa0f. Mainland cursor
creation explicitly inserts **Y=0**; alternate-layer creation inserts the
configured layer threshold **+1**, default 1501. These are layer sentinels,
not terrain elevations. `29d09c0 → 28c36b0` copies them unchanged. Other and
existing-icon-derived heights remain unknown and are rejected.

ID 0 is valid: the producer starts its free-bit scan with zero at `28c385f`,
carries it in R9D at `28c387c`, and assigns full R9 at `28c38a9`; the component
constructor clears those bitmaps. An older branch packs a generation byte into
the upper 32 bits. Only `UINT64_MAX` means missing. Source and all ID bits are
preserved; a pin and waypoint with equal numeric IDs cannot be confused.

Marker travel requires MainField key 1 and the ordinary mainland sentinel.
The selected actor's server navigation component at index 0x35 (`+1a8`, vtable
`5b1f2a8`) must have the correct owner and a valid native navigation context.
At least 54 components are required; count 53 covers transform only.

Height resolution uses native geometry:

1. ServerTransform virtual `+178 → 21a1060` is the down-distance query;
   `+180 → 21a12f0` is its upward counterpart. The down helper creates a mode-3
   navigation context (`1eebd20`) and calls `1f04bd0`, returning native hit
   distance or FLT_MAX.
2. The adapter builds the same ray/filter ABI and searches downward at marker
   X/Z from immediately below the alternate-layer threshold. The finite
   4096-unit search bound is not a destination altitude. Missing/unloaded
   geometry or a missing/nonfinite/out-of-range hit rejects before movement.
3. At the measured hit, constructor `1fe25c0` and placement gate `1efe790`
   validate native character placement, accepting only classification mask 1.
   Mode 2's unconditional placement bypass is deliberately unusable. No broad
   nearby-XZ search runs.
4. Returned SRT position uses native parent/sector decoding. Excessive shift,
   alternate-layer results or a failed final short down-ray reject placement.
   Marker source/ID/XYZ and world/FieldInfo are recaptured before dispatch.

Navigation references, placement filters, and ray crossing arrays follow exact
normal native ownership and same-thread allocator cleanup. Known C++ unwind
cleans completed resources and propagates. Hardware faults perform no guessed
cleanup. Original terrain XML was checked too: its sector ranges/offsets are
not an exact capsule-valid point. Terrain assets are not bundled and do not
replace native geometry queries.

## IPC, persistence, and verification

Request is 112 bytes: source u32@104, reserved u32@108. Marker requests accept
an ID/source, never arbitrary XYZ. Reply is 14016 bytes: marker count/status/
observation time @1520/1524/1528; 520 rows start @1536. Each 24-byte row has
ID@0, float XYZ@8, source@20, layer@21, shape@22, color@23. Header static
assertions fix the offsets.

Lists are captured at most twice per second. Wrong/stale/unavailable contexts
clear their list and timestamp. Travel recaptures its target independently.
Sessions, epochs, deadlines, command ordering and exact-payload idempotency
prevent automatically repeated travel.

Sixteen bookmarks contain values and stable keys, protected by Windows DPAPI
and bound to the original EXE hash. Names are strict UTF-8, max47 bytes, zero
terminated/padded, with C0/C1 controls rejected. Capture/remove completion
advances a revision. The single serial pipe worker attempts storage once per
new revision. Failure stays dirty without polling retries. A new explicit edit
can retry/recreate an externally deleted/replaced file. Storage status reports
the last explicit write result, not continuous file integrity monitoring.
Parallelizing the worker would require another ordering/store review.

Local evidence:

- Tracked protocol/value/persistence/policy tests cover ID0, source separation,
  UTF-8, queued-versus-arrived receipts, duplicate commands, geometry failures,
  list expiry, and balanced leases.
- Copied original value/position fixtures include 196608 bit-identical signed
  sector cases and native descriptor construction.
- Copied complete global getter/yaw/field queue/release fixtures cover parent
  transforms, deep copy, busy rejection, deferred last-borrow enqueue and the
  native immediate queue. All external calls use owned stubs.
- Copied complete down helper, placement constructor and placement gate verify
  exact ABI and the real mode-2 bypass. Resolver fixtures cover geometry and
  placement failures, shifts, marker mutation, source/layer/world restrictions,
  C++ unwind and hardware latching.
- Independent exception and real-DPAPI fixtures use owned paths only, checking
  exception identity, known pre-Begin cleanup, no uncertain replay, explicit
  repair after storage deletion/replacement, and no polling writes.
- The isolated ASI builds with MSVC `/W4 /WX` and remains uninstalled.

Private disassembly, resource extracts, generators and copied-native bytes stay
ignored under `.local/marketplace-expansion-research/teleport/`. Tracked sources
contain layouts, control logic, RVAs and hashes, not original game code/assets.

Rejected alternatives remain rejected: ordinary `MoveActorReq → 2bc5420 →
2bcd8a0` requires available transform/cell data and is not a general field
transition. AI routes do not prove player support. Release debug
MovePlayer/MoveField handlers parse and return without moving. Older Trinity
signatures do not match this build.
