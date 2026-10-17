# Lion and Rokade forced dismount height

17 October 2026. Feature #21 is **locally implemented and integrated**. The requested
behavior is to keep the rider attached to the circus lion (CharacterInfo key
**1002316**) and Rokade (**31378**) until five times the original fall-height
threshold. This is separate from their 40% jump-height increase, fall-damage
protection, voluntary dismounting and combat reactions.

This checkpoint contains read-only analysis of the pinned executable and
original archive entries. No game process was controlled, no game function was
called and no game files or saves were changed. `CrimsonMountDismount.asi` was
built only in an ignored fixture directory. It has not been installed or loaded
by the game. Parent integration and offline checks are complete; gameplay
acceptance remains separate work.

## Supported evidence

Executable 1.0.0.2976 SHA256:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
All native addresses below are RVAs. Private disassembly, original extracted
files, bounded scanners and a verification manifest are under ignored
`.local/marketplace-expansion-research/dismount/`.

Original VehicleInfo body SHA256:
`2a501f6f452e4fdb1bf0c1d15638a0aeac8e0831645f77ef61c5071591efa85b`.
Its header SHA256 is
`8c2ef32c90d2d3eabcedbc70776a2745e15b3ae4777a3ec1a5cfd0f0f1eebe3c`.

The selected-mount/seat ownership route used by the existing mount-protection
module is a useful read-only reference for a future policy. It does not identify
the falling threshold itself. The jump module's input scalar is also unrelated
to when the rider detaches; scaling it cannot implement this feature.

## Native removal routes

The native reason registration at `190750` distinguishes:

| Value | Native reason |
| --- | --- |
| 0 | Action |
| 1 / 2 | Throw_Driver / Throw_Vehidle |
| 3 | Catch |
| 4 / 5 | Dead_Driver / Dead_Vehicle |
| 6 / 7 | Sequencer / Logout |
| 8 | ActionAttributeMisMatch |
| 9 / 10 | SequencerEditor / SequencerCutSceneStart |
| 11 / 12 | AIFunction_ForceRideOff / Undefine |
| 13 / 14 | EndFocusActor / RiderChangedVehicle |

There is no separately named fall-height reason in this registered enum.
Reason zero or eight therefore cannot, by themselves, distinguish the requested
fall behavior from ordinary action changes.

`2228e40` is a thunk to the common paired-removal helper `e6f2e60`. The helper
forwards the rider identity, vehicle actor, flags and reason through the vehicle
component's virtual `+158` or the rider component's virtual `+150`. Its body has
no fall-height comparison. Its callers include throw, logout, undefined-state
cleanup and changing vehicles. Suppressing it globally would change several
unrelated behaviors and is not an acceptable implementation.

Action-attribute reconciliation at `2320ee0` can invoke those same virtual
removal methods with reason eight at `2321818` and `232187c`. The bounded body
does not supply a fall-distance comparison. It reconciles action/ride state,
which must remain intact for ordinary dismounts and combat transitions.

The server action-processing path `2a797c0 -> 2a7a300` is a narrower next lead.
`2a7a300` obtains the current action data through control `+30`, its selected
slot, and the action reference `+88 + slot*8`. At `2a7a364..2a7a379` it checks
bits `40/80` in action-data byte `+0a`; its later paired removal calls use reason
zero. These sites process an already selected action and contain no established
height threshold. The condition selecting that action must be traced upstream.

The client `RideOffMovementMethod` vtable is `5597190`; its main setup method
is `682c20`, with lifecycle thunks leading to `9145ff0` and `914b860`. These
methods prepare and clean up an existing ride-off movement. Their names and
motion scalars do not prove the earlier height decision.

## Height-looking fields that were excluded

The VehicleInfo loader's complete logical range `151d310..151d857` crosses
several PE unwind fragments. Its individual read-error descriptors establish
the following loaded-field meanings:

| Loaded offset | Field | Observed consumer |
| --- | --- | --- |
| `+88` | `_callVehicleVoxelType` | Summon/placement validation |
| `+8c` | `_checkDistanceToGround` | `9dd480 -> 9ddfb0` |
| `+9c` | `_maxAllowableHeight` | `22a5580` |
| `+a5` | `_platformHitEventMinAttackImpulseLevel` | Platform hit filtering |

The field names are resolved from the native deserializer's own descriptors,
not inferred from the offsets. In particular, `+a5` is not the ground-distance
field.

`22a5580` reads VehicleInfo `+9c` at `22a55f8`, obtains the actor's current world
position from `4f8220`, and compares the height limit against **absolute world
Y** at `22a564d`. This is the existing Blackstar altitude restriction. It is not
the difference between a jump's starting height and the current position.
Multiplying it by five would not establish the requested fall-distance rule.

VehicleInfo `+8c` is passed with the call-voxel flags from `9dd480` to `9ddfb0`.
This path validates a mercenary/summon candidate and its ground/placement rules;
it has no paired-rider-removal call in the reviewed body. It must not be edited
as a substitute for a verified separation threshold.

Two other similarly named leads are unrelated: `_fallOffByDistance` belongs
to action camera shake (`33c3120`), and `_fallOffType` belongs to
`ActionCameraShakeDesc`. `_minFallDistance` occurs in a gliding audio/effect
description. The generic `FallHeight`/`FallDistance` enum registration at
`15cce70` also includes quest, item, recipe and event parameters; its names alone
do not identify this mount decision.

## Original action-chart evidence

The original rider charts contain distinct fall, ordinary ride-off, crash and
rodeo animations. Read-only extracted files include:

- `basic_lower_riding.paac`
- `basic_upper_riding.paac`
- `ride_test3_lower.paac`
- `ride_upper.paac`
- `common_animal_ridehorse.paac`
- `r0035_carmabirdsaurus_00001_upper.paac`
- `r0035_ride_carmabirdsaurus_00001_upper.paac`

`basic_upper_riding` contains the `fallmount` animation; `ride_test3_lower`
contains walk/trot/canter/gallop falling loops and cliff-stop animations. The
native action-name table separately includes `RideOffFall` at string RVA
`59b8c50`, referenced by a name-table entry at `59b9258`. That establishes a
distinct action name, not its eligibility condition or a numeric fall distance.

The earlier structural-decoder blocker is resolved. The bounded private
`decode_chart.py` follows the native reader widths and reconciles eleven
original charts exactly to EOF, including all four actual lion/horse packages.
It rejects unsupported nonempty local-script tables. Exact EOF proves the
structure for these fixtures; it does not claim every condition's semantics.
`chart-summary.private.json` records per-file hashes, byte lengths and counts.

The native readers are `1f66560..1f66a2d` for the variable animation records,
`1f68370..1f69040` for the package, and `1f670b0..1f67198` for branch arrays.
The condition buffer is an array of 32-bit words; its finder contains 16-bit
**word indexes**, rather than byte offsets. Branch records are 52 bytes.
Registration `383460..38581a` maps 473 extension opcodes to native handlers.
Evaluator `242d370` uses cached base conditions below 131, extension dispatcher
`360be0`, and prefix operators 629/630/631 for AND/OR/NOT. These mappings were
checked against native instructions and original tapes, rather than guessed
from numeric values.

## Actual mount packages and fall gate

Pinned CharacterInfo records and StringInfo resolve the original groups:

| Mount | CharacterInfo | Group | Ridden package |
| --- | --- | --- | --- |
| Circus lion | 1002316 | `CD_M0002_Wolf_Elite` | `m0002_ride_wolf_upper.paac` |
| Rokade | 31378 | `CD_R0001_Horse` | `r0001_horserd2_movetest_03_upper.paac` |

The original `characteractionpackagedescription.xml` maps the groups to their
main and ridden packages; the companion compiled description contains the same
group, slot and path strings. The two character rows hash to
`d1f908b2c9d650be5c62c3a429f196f34a60dda70dc92b539e6520b5d71e3970`
and `2377d4ad08cf994dbc16f3dfd871eb2fde07270865cc7e6fee7300261817b5ae`.
The riding wolf package hashes to
`308172c1aac1053f71fc49e5fa70b59bfc90a6f0e9c5b0c16f20e827564c1e9a`;
the riding horse package hashes to
`350e7c187fc5c1a1e4445b85ec79e9369eaf618444661f7592d78b9002806d7c`.

The relevant native condition is opcode **404**, registered through thunk
`373800` to body `8785b60..8785bd4`. It reads control `+140`, then float `+530`,
and calls the native comparator `242db10` with the original threshold and
operator. Operator 4 is strict **measured value > threshold**. The handler
returns the engine's condition status by inverting the comparison bool.

The `+530` producer at `36a4c00` projects previous minus current position onto
the movement up vector. While movement byte `+50b` is set and the downward
projection is nonnegative, `36a52fe..36a5306` adds it to the accumulator;
`36a5310` otherwise resets it. A later collision correction can add to the
same accumulated value. This is relative accumulated downward displacement,
not absolute world altitude or the jump launch scalar.

Both original automatic-fall programs compare that value against **5.0**:

| Package | Branch / root / count | 404 word / finder | Destination |
| --- | --- | --- | --- |
| Riding wolf | 5 / 27 / 7 | 45 / 32 | `e196052f` |
| Riding horse | 609 / 1372 / 7 | 1122 / 1375 | `9ec06df8` |

The wolf tape is `AND(183, AND(attribute(0,39,2), AND(404>5, 213>5)))`.
The horse tape is `AND(elapsed>0.25, AND(404>5, AND(213>5,352>5)))`.
Conditions 213 and 352 remain unchanged native predicates. The destination
actions carry flags `8000000101`, including the falling-related bit 39 and
bit 8. In the rider's original `ride_upper`, branch 190 tests attribute bit 8
on actor selector 9. Native selector resolution `a2a030 -> 2217cf0` obtains the
current mounted actor through rider component `+48`, data `+18`, identity `+10`.
That branch selects rider action `c6932d24`, whose action-data byte `+0a` has
bit `80`, reaching the previously verified action-removal path.

There is no deduplicated-leaf ambiguity in these sources. Word 45 occurs once
in the entire wolf finder, at 32; word 1122 occurs once in the horse finder,
at 1375. Scanning **both condition ranges of every branch** finds exactly the
one containing program listed above. The runtime repeats these uniqueness and
branch-range checks and refuses any added reuse.

The unrelated 3.1 value in `basic_upper_riding` belongs to a roll-start action's
transition. It is not used by this candidate. Neither the r0035 dinosaur
package nor an animation-name match determines the lion/Rokade implementation.

## Candidate and native call contract

`runtime/marketplace/dismount/` owns the candidate. Its sole entry hook is the
opcode-404 body at **8785b60**, separate from Force Palm's evaluator hook.
The native dispatcher `360ce7..360d1d` forwards seven arguments: control,
one-pointer node reference, shared context, frame context, mode byte and two
opaque output/context arguments. The original is called **once**, and every
argument except the borrowed node reference is forwarded unchanged. Its result
and exceptions remain native.

The formerly unused third argument supplies package provenance. At
`232e5d5..232e5f6`, the branch evaluator constructs the condition buffer at
owner `+110` and shared context at the **same owner +20**. The handler checks
the node pointer against that owner's pinned buffer offset, original buffer
size, finder count, branch count, complete seven-node tape and branch target.
It checks that the finder and all branch ranges give the leaf one owner.
This validates the current invocation; merely finding a similar 5.0 float is
insufficient.

Scope is rebuilt from the currently selected player's client/server pair,
current ride identities and seat, the mount's reciprocal seat entries and
loaded CharacterInfo key. The actor receiving the condition must own the
current control and match the selected lion or Rokade. Borrowed pointers,
definition handles, selected roots and program contents are reread before
capture acceptance using volatile reads. Invalid or changed ownership,
unreadable buffers, arithmetic overflow, another mount, or another program
forwards the original reference unchanged.

For an admitted call, the candidate copies the original 20-byte node and
checks all five words against the pinned leaf. It multiplies only the float
**5.0 by 5.0, producing 25.0**, then supplies an eight-byte private reference
to that private node for the synchronous original call. Original native data
is never written, so repeated falls cannot compound the multiplier. The
accumulator, air/ground/time predicates, removal helper, hit reactions, death,
manual dismount and other mount definitions retain their original paths.

Activation pins the module before worker creation, verifies the executable
and native spans, publishes an atomic original trampoline before enabling,
and enables the policy only after successful activation. After any activation
attempt, cleanup disables the entry but retains the module and trampoline
until process exit. Lifecycle failures do not retry or free a potentially
executing trampoline.

The independent audit is
`.local/marketplace-expansion-research/dismount/verify_expanded.py`; its compact
result and detailed hashes are in `expanded-evidence.private.json`.
It checks 19 pinned original source files, 13 additional native spans, eleven
exact-EOF decodes, the two unique leaf routes, the rider removal route, and
the checked-in program/target metadata. `verify_evidence.py` separately retains
the original twelve-span removal/altitude exclusions. No original chart was
patched or redistributed.

## Next exact work and acceptance

The isolated Windows x64 MSVC build passed with C++20 and `/W4 /WX`.
The fixture passed **115 dispatch checks**, both mount keys and actor
representations, 5/25 boundaries, repeated non-compounding invocations, seats,
unrelated definitions/programs, leaf reuse, malformed and transient reads,
changed copied data, disabled passthrough, original output/status and exception
forwarding. It executes a copied native float comparator only inside owned
fixture memory; no game image or game reference helper executes. Lifecycle
fixtures cover create failure, null trampoline, publication failure,
activation/disable failures, retained lifetime and no retry.

Parent integration should add target `CrimsonMountDismount` from
`dismount/mount_dismount_mod.cpp`, link `minhook` and `bcrypt`, and add test
`tests/mount_dismount_tests.cpp`. The private standalone CMake project supports
independent verification without editing shared build files. Package/lifecycle
integration and sibling-hook checks remain the parent's responsibility.

Required later tests include both mounts below, at and above the original and
five-times thresholds; different starting elevations; repeated falls; normal
dismount input; throws and combat transitions; full removal/restoration; and
compatibility with the separate size, speed, 40% jump and damage-protection mods.
No such game test was performed during this checkpoint. Native fixture success
does not establish gameplay acceptance.

## Independent parent integration

The parent reviewed scope, exact leaf copying and hook lifetime, reran the
original-source audit, and integrated ID `mount_dismount`, binary
`CrimsonMountDismount.asi`, and the English card **Stay mounted on longer falls**.
The card includes **Works while playing** and **Close game for setup** labels.
Native coexistence checks cover 81 guards against 39 distinct hook entries.
The combined package passes 13 Marketplace and three LiveItems native targets,
the pinned loader's owned 15-ASI host, independent removal preserving mount jump
and protection, and running-game/foreign-file/tamper/recovery checks. The frontend
build and all 39 Marketplace browser tests pass; the parent also inspected the
card screenshot. Evidence is staged under ignored
`.local/marketplace-expansion-runtime-20261017-dismount-reviewed/` and the
`marketplace-mount-dismount-*` logs. No game installation, gameplay acceptance,
commit, push, PR or release is implied.
