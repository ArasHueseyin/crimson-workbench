# Player movement speed

Local native implementation, 17 October 2026. This work has not changed the
installed game, launched the game or updated an ASI in the game directory.
In-game acceptance is pending. The English Marketplace card and local package
integration are implemented and tested; nothing has been installed in the game.

## Scope

`CrimsonPlayerSpeed` applies a saved **100–300%** multiplier to the selected
player's normal ground locomotion. It scales the horizontal drive velocity
before the native movement servo consumes it. Vertical velocity, gravity,
the original time step and collision result output are forwarded unchanged.
It does not change a global time scale, movement status table, animation
playback rate or mount speed definition.

The wrapper requires the selected client actor's current full identity,
alive state, protagonist kind, possession round trip, component ownership,
control object type, control-state ownership and controller pointer to agree.
It also requires a normal movement method owned by that same control/actor,
native ground mode and one of four reviewed ground-stepper callers. Other
actors, mounted-animal controllers, climbing/scripted movement methods and
airborne/swimming mover call sites do not pass these guards. All addresses
are borrowed for one invocation and are re-read before using the copied
velocity; none are cached across callbacks.

The input vector is copied into three local floats. Only X and Z in this
copy are multiplied. The game-owned input vector is never edited. The native
function synchronously consumes this vector, does not retain its pointer,
and writes collision information through a separate fifth argument. All
seven original arguments retain their native order and types; the second
argument is explicitly a float to preserve its XMM1 calling convention.
The wrapper also returns the original full-width RAX value on both scaled
and unmodified paths. The reviewed ground callers ignore that register;
preserving it also protects the helper's other callers from cleanup-code
clobbering. Recursive entry forwards its existing vector without scaling
it again and preserves its own original result.
Invalid pointers, unsupported movement state, non-finite input/time step,
non-positive time step or overflow fall back to the original arguments.

## Verified native path

Supported executable: **1.0.0.2976**, SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
PE timestamp `1790086912`, image size `389722112`.

- Native actor tick `a00ee0` resolves actor components `+40` control, then
  control `+c8` state before invoking state tick `87e140`.
- State tick `87e140` reads state `+60` control and control `+140` native
  movement controller, then invokes controller tick `36a9770`.
- Controller tick reaches dispatcher `36a12a0`. Controller byte `+2ed == 0`
  selects ground mover `36a3850`; values 1 and 2 select different movers.
- Ground mover invokes servo stepper `369ffc0` at call RVAs `36a3d07`,
  `36a421d`, `36a4739` and `36a4b61`. Only the corresponding exact return
  addresses are accepted by this module.
- Stepper `369ffc0` takes the drive vector through R8. Its first input reads
  use only offsets 0, 4 and 8. It writes the drive to the native proxy `+c0`
  before invoking physics. The 4,225-byte body has SHA256
  `e6efc4c2dd3f1895f67c76b68f5630d345fe041b1ff89ab0aeccc3b8c4aa0147`.
- Current movement method is selected by state byte `+f0`, indexing the
  method pointer array at `+f8`. Initialization `88c530` registers the
  same NormalMovementMethod for modes 0–3. Other modes are refused.
- NormalMovementMethod constructor `9fdbab0` sets exact vtable `55cab28`,
  control owner `+18` and actor owner `+20`. The module rechecks these
  relationships and exact ClientCharacterControlActorComponent vtable
  `55b5460` before applying speed.

Initialization verifies the executable hash and twelve native code spans,
including the hook prologue, dispatcher, actor/control chain, method lookup,
constructor ownership assignments and all four ground calls. An executable
or reviewed code mismatch disables the module before hook installation.
These checks are separate from the player/mount jump modules' patch spans.

The old third-party locomotion signature is absent from this executable.
The implementation uses the independently traced current executable path;
it does not reuse that signature, its old controller offsets or its
ungated global velocity multiplication. Third-party source was a research
lead only; no third-party implementation was copied into this module.

## Per-user setting

The setting belongs to the current Windows user's LocalAppData folder:
`local.crimson.workbench/player-speed.dat`. It is independent of player
jump height. Exact record format: ASCII `CWS1` followed by a little-endian
unsigned 32-bit percentage, inclusive range 100–300.

The Rust settings module reads/writes only this preference file. It uses
an atomic same-directory temporary file, validates the complete record and
re-reads the published result. It refuses unsupported builds, running or
unknown game process state and a game started before publication. It does
not edit game files or saves. The ASI checks an ordinary unlinked eight-byte
file and reads it once at game startup. Missing, malformed, oversized,
inaccessible, directory, hard-linked or reparse-point records use 100%.
At 100%, no movement hook is installed.

Marketplace integration provides an explicit Save movement speed
button, a 100–300% slider and a Reset to normal draft action. Moving a slider
must not write a preference. Saving settings and installing the runtime
module remain independent, with running/build guards and unsaved-draft
protection matching the player jump card.

## Local verification and pending acceptance

The standalone Windows x64 MSVC build compiles the ASI and native fixtures
with `/W4 /WX`. CTest passes 83 native fixture invocations covering both
selected-player kinds, all four allowed normal methods and ground callers,
original argument/output/full-width result forwarding, repeated/recursive use, unchanged
source vectors, normal/invalid settings, stale ownership, mounts, other
controllers, air/scripted/method exclusions, invalid memory and invalid
float/time-step inputs. The private read-only audit independently verifies
twelve compiled-in code spans, ten native function body hashes and the
stepper's eleven input-vector reads against the pinned original EXE.

Parent review required full RAX-result forwarding on every native path; the
updated native tests verify it for scaled, passthrough and recursive calls.
The parent rebuilt the native module and fixtures and independently reran the
code-pin audit. The real Rust settings module now executes its two persistence
and guard tests. The Marketplace browser suite passes 30 tests, including speed
draft isolation, explicit saving, reset, rejected-save behavior, unsupported
builds, running-game guards and independent speed install/removal. The app
compiles with the matching Tauri command and session guard.

The complete local native package now includes eight Marketplace ASIs. Its
synthetic loader host checks all ten packaged ASIs, including LiveItems and
ExtraSockets, rejecting an unsupported executable before hooks. Temporary
package tests verify all eight install/removal lifecycles, speed removal without
touching player/mount jumps, reinstallation, conflicts, rollback/recovery,
running-game refusal and package integrity. Local stage:
`.local/marketplace-expansion-runtime-20261017-playerspeed`.

Required in-game checks: compare 100%, 150% and 300% walking/running/sprinting
on flat terrain and uphill/downhill; verify stops and direction changes;
change selected character; mount/dismount a lion, Rokade and a horse; jump,
fall, swim and climb. Verify movement collision, stairs, camera behavior and
combat movement. Some combat root-motion paths may reuse normal ground
locomotion, so this static scope is not evidence that every attack/dodge
translation is unchanged. The module does not speed up attack animations.
No measured speed, stability or combat-movement acceptance is claimed until
these game checks are performed.
