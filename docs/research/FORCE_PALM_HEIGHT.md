# Adjustable aerial Force Palm height

Local implementation, 17 October 2026. Native and owned fixture tests pass.
**Gameplay acceptance has not been observed or claimed.** No game, save or
archive file was modified, and no process was installed, injected, restarted or
controlled during this implementation.

## Integration contract

The module is `CrimsonForcePalmHeight.asi`, Marketplace ID `force_palm_height`.
Its source is `runtime/marketplace/force-palm-height/force_palm_height_mod.cpp`.
Build for Windows x64, MSVC C++20, `/W4 /WX`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX`,
linking pinned MinHook, `bcrypt`, `shell32`, `ole32` and `uuid`.

The startup preference is the current Windows user's
`LocalAppData/local.crimson.workbench/force-palm-height.dat`. It is exactly
eight bytes: ASCII `CWF1`, followed by a little-endian unsigned height
percentage from **100 through 10000**, corresponding to **1x through 100x**.
It accepts intermediate percentages, so the UI can offer a multiplier slider.
Missing, malformed, out-of-range, oversized, inaccessible, linked or non-regular
files select **100**, normal height. At 100 the module installs no hook.
It reads the preference once after executable validation; installation,
removal and preference changes take effect on the next game start.

The setting controls the native **height input**, independently of the existing
ordinary player jump slider and the unlimited aerial Force Palm chain module.
It does not alter gravity, time step, inputs, spirit costs or learned-skill
eligibility. A 100x height setting increases the native launch velocity by 10x,
consistent with `v = sqrt(2 * abs(gravity) * height)`.
Actual travel still depends on ceilings, terrain, collisions, animation and
other native movement rules.

## Pinned original evidence

Supported executable: **1.0.0.2976**, PE timestamp **1790086912**, image size
**389722112**, SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
Original `fist_upper.paac`, parsed to EOF at `2477d0`, SHA256
`950c6059444aa672af95efe4fe1c074dfaef3a47f75e44a7f8ee68231c5a940d`.
The previous chain-limit study establishes the seven Skill-15035 animation
routes: 1462, 1463, 1465, 1466, 1467, 1468 and 1469.

The native client frame-event factory switch at `4f3ef4` maps type **59** to
`4effd8`, which invokes constructor `4e1a80`. That constructor installs
`ClientFrameEventJump` vtable **5585170**, whose execute slot `08` points to
the five-byte thunk **6c38a0**, `e9 7b cc b0 08`, leading to body **91d0520**.
The new module hooks that independent thunk, leaving the mount module's body
hook intact.

Native event-reference producer **1f621c0** establishes:

| Field | Borrowed native value |
| --- | --- |
| event `00` | Current 20-byte animation event instance |
| event `08` | Package global event buffer plus instance word offset times four |
| event `10` | Animation's original condition owner from animation `30` |
| event `18` | Package `20` |
| animation `10` | Package owner |
| animation `20` | Event instance view, array at `00` and byte count at `08` |
| animation `38` | Original fixed 156-byte animation record |
| package `1a0` / `1a8` | Global event data pointer / byte length |

The producer's complete 689-byte function has SHA256
`f9ef5ec59e3145662b6c8b80c73acdec7bf8054bcd46a21837d6cda86902826a`.
Package reader `1f68370` establishes the original global event buffer length
**531344**, expression buffer length **11212** and finder count **2325**.
Animation reader `1f66560` establishes the instance view and fixed record.
The current animation comes from the selected actor's current control layer
(`control+88` / `control+90`, layer `18`), also used by native event dispatcher
`232fab0` and client event queue method `869160`.

The actual launch records are narrowly identified by both original animation
and original instance, rather than matching every Jump event or a float value:

| Animation | Instance | Native event word | Height |
| --- | --- | --- | --- |
| 1462 | 13 / 15 | `1cf45` / `1cf96` | 3.5 / 4.5 |
| 1463 | 19 / 21 | `1cf45` / `1cf96` | 3.5 / 4.5 |
| 1466 | 28 | `1d528` | 10 |
| 1467 | 4 / 5 | `1d52f` / `1d536` | 4.5 / 5.5 |
| 1468 | 15 / 16 | `1d656` / `1d65d` | 6 / 7 |
| 1469 | 17 | `1d686` | 5 |

Native conditions choose the applicable event; this module does not evaluate,
remove or replace those conditions. The original **0.01** setup/reset events
are deliberately preserved. Animation 1465 contains only a setup Jump event,
so it receives no invented launch. Animations 1462 and 1463 have identical fixed
records but distinct instance counts, **33** and **39**, which are part of
their identity checks.

## Scope and behavior

Each callback resolves the current selected client actor afresh. It requires
matching manager/actor/control types, live selected identity, playable metadata,
component bounds, possessor backlink, control owner and movement-state owner.
Mounts, other actors and stale selection fail closed.

It then checks the current layer, package bounds, animation event count,
instance pointer/index, original condition owner and global data offset.
Bounded FNV-1a 64-bit fingerprints read every byte of the 156-byte animation,
20-byte instance and 26-byte typed Jump record. The compiled metadata identifies
seven animation/event-count pairs and ten launch sites without embedding the
serialized records. It copies the native 80-byte reference and 26-byte data for
that synchronous invocation, checks the copy's fingerprint, multiplies only data
float `0c`, and revalidates ownership and source fingerprints before passing the
copy to the original function. Native eligibility/status handling and source
chart data remain intact. The original executes exactly once with all other
arguments and its return value forwarded. Native exceptions remain native; only
our memory reads are fault-guarded. No borrowed address survives the call, and
repeated jumps cannot accumulate changes in the original chart.

The original Jump body copies the parameters and writes its own pending scalar
at movement state `32c`. The original consumer **88bf40** converts this scalar
using its native gravity magnitude at **88c054–88c076**. It keeps the original
horizontal motion, gravity override, eligibility and pending-state consumption.

`player_jump_scope.h` now excludes these exact original Palm animations from
the ordinary player jump slider, checking both before preparation and immediately
before its scalar write. Thus **3x ordinary jump + 100x Palm remains 100x Palm**,
rather than compounding to 300x. Unrelated ordinary animations retain the ordinary
slider. Incomplete/unreadable ownership is conservatively refused by its
existing guarded adjustment. The separate chain-limit hook **242d370** and
mount jump hook **91d0520** are unchanged.

## Lifetime and coexistence

Startup verifies the complete on-disk executable hash, PE identity and nine
narrow native witnesses. The new entry is **6c38a0** only. Pins in the mount
Jump body and ordinary jump consumer lie beyond their existing patched entries.
The private audit checks 97 existing hash-range guards and 62 conservative hook
candidates; none overlap the new entry or its code witnesses. It makes no claim
about external, unexamined third-party ASIs.

The module is pinned before its worker starts and before hook creation. Its
trampoline is atomically published before activation. After any activation
attempt, cleanup disables the entry but retains the pinned module and published
trampoline until process termination, including activation failure. An in-flight
callback cannot lose its original function.

## Verification and pending acceptance

The ASI and public tests build with MSVC `/W4 /WX` and assertions active.
`force_palm_height_tests.cpp` passes **435 event callbacks** using fabricated
records and an explicitly supplied fixture catalog. The default production
catalog rejects those records. Tests cover all ten launch-site layouts, range
boundaries, normal/disabled passthrough, repeated calls, actor/mount/ownership
and package failures, selection changes, native exception forwarding, hook
failure lifetime, setup-event rejection and ordinary-jump behavior. All seven
animation/event-count pairs are checked. Every byte of all fixture records is
mutated individually: **1,552 mutations** across seven 156-byte animations,
ten 20-byte instances and ten 26-byte payloads. Three guard-page cases, null
pointers and address overflow verify bounded-read failure behavior.

Independent private Python and C++ derivations agree on all seven original
animation/event-count pairs, all ten launch-site metadata entries and their
27 fingerprints. They also verify the unchanged package bounds and every
single-byte mutation against the privately preserved originals. The production
catalog contains fingerprints and semantic metadata, not serialized assets.

Optional private verification sets
`CRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER` to an absolute local header
path. With the original records it passes **445 event callbacks**, including
production ordinary-jump/Palm coexistence. It also executes an owned copy of
the existing 34-byte compatibility signature for native gravity/height
conversion, with its data reference relocated to an owned absolute-value mask.
Across the native launch heights, a 100x scalar yields 10x initial vertical
speed. The negative-value fallback traps rather than calling the game. The
private header is ignored, public CI requires none of it, and the release build
clears this option.

The earlier private build under `.local/force-palm-height-research` passed
three CTest targets, including the ordinary player-jump suite and an owned copy
of native `1f621c0` with external calls replaced by an owned trap. Owned MinHook
thunk/body fixtures verified coexistence in either activation order. These
additional original-function fixtures remain private; no live engine address
is executed by any of these checks.

Private regeneration/audit entry points are `inspect_events.py`, `generate.py`,
`make_native_fixture.py` and `audit.py`. The last records nine matching native
witnesses and a read-only collision audit in `audit.private.json`.

Still required in the game: compare 1x and higher settings on the same aerial
Palm route; test ceilings, landing and repeated chains, resource exhaustion,
character changes and removal; test the ordinary, mount and unlimited-chain
modules together. Automated fixtures establish implementation behavior and
native data/ABI evidence, not measured gameplay height or stability at 100x.
