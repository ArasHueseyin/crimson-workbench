# Gliding speed

Implementation updated 18 October 2026. The local **250% boost-only test passed**:
the user confirmed the requested ordinary/fast glide comparison worked. The
module logged `ACTIVE` for the fast boost. Read-only observations measured
approximately **46.32 m/s** horizontal boost velocity, compared with **18.53 m/s**
in the 100% baseline, matching the requested 2.5x factor without the previous
runaway velocity. Ordinary wing glide is excluded from the hook's scope.
Gameplay at 500% has not yet been checked with this corrected implementation.

`CrimsonGlideSpeed.asi` applies a saved **100–500%** multiplier to the selected
player's sustained **fast glide boost** horizontal velocity. On the tested
controller mapping, Square opens the wings and X activates this boost. Ordinary
wing glide remains native. Incoming X/Z velocity is normalized by the saved
factor before the native update, then its fresh horizontal result is scaled.
Y, gravity, the original time step, spirit costs, eligibility and subsequent
native movement checks remain under the game's control. This does not change normal walking, mounts,
animation playback or global time. Starts, landing, braking starts and
dash-ready transitions retain native speed. A higher horizontal speed can
naturally change the outcome of a collision; the engine still handles it.

## Native evidence

Pinned executable **1.0.0.2976**, SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`,
PE timestamp `1790086912`, image size `389722112`.

- The game has no separate named GlideMovementMethod. NormalMovementMethod
  uses vtable `55cab28`; its control and actor owners are at `+18` and `+20`.
  State initialization `88c530` registers this method for modes 0–3.
- `ab96f0` composes the native speed and steering vector. Its final writes
  `abaa26`, `abaa2c` and `abaa32` populate output X/Y/Z at `+48/+4c/+50`.
  The same function first reads the carried X/Z values at `ab9d43` and
  `ab9d49`. Multiplying only the result can feed boosted velocity back into
  subsequent native updates. The wrapper divides incoming X/Z by the factor,
  calls the original once, retains its full RAX result, and multiplies its
  resulting X/Z. This also preserves cached native velocity without repeatedly
  increasing it. Only this invocation's transient output is changed.
- Both direct callers are pinned. At return `ab8775`, caller `ab8410` uses
  these components to compute movement length and integrates velocity times
  its unchanged `params+6c` time step at `ab8801`. At return `ab8c19`, caller
  `ab8bf0` integrates the same components and time step at `ab8c21`.
  Other return addresses do not qualify. No position is teleported by the mod.
- The original caller and state pipeline continue after that integration,
  including their movement checks. State tick `87e140` retains its native
  controller tick call `36a9770`. The existing ground-speed hook `369ffc0`,
  player/mount jump and aerial Force Palm hooks are untouched.
- The original lower glide package
  `0010/actionchart/bin__/loweraction/1_pc/1_phm/basic_lower_glide.paac`
  is 86,456 bytes, SHA256
  `b4ac187577fafc3aaa06102cae56e2d7e0120c6632286e0b220d513e4e0441a9`.
  The native-width decoder consumes it completely: 59 animation records,
  32 charts, 824 expression bytes, 181 expression references and 24,580
  cancel-expression bytes. Original animation-name indices at fixed-record
  `+90` and motion blend indices at `+96` identify five sustained fast boost
  records (32, 33, 34, 56, 58), with three distinct fingerprint/event pairs and
  blend indices 3, 4 or 5. The remaining 54 records, including ordinary glide,
  starts, landing and directional dodges, do not qualify.
- Native movement selects **upper layer 0 or lower layer 1**. The earlier
  implementation incorrectly treated layer 0 as lower and required the lower
  package's fingerprints there. Active lower-layer fast glide was therefore
  excluded by the selector guard.
  The corrected capture follows the actual native-selected slot at
  `control+88+8*selector`. Only lower layer 1 with the exact lower package and
  boost fingerprints qualifies; an upper-layer or inactive boost does not.
- The upper glide package is 67,224 bytes, SHA256
  `cd4db2678569395852bd80de73bc72fea616a228db032905a565a2963813212e`.
  Its complete parse contains 76 animations, 680 expression bytes, 107 expression
  references and 14,956 cancel-expression bytes. All upper records are excluded
  from scaling. Fifteen ordinary sustained/steering fingerprints are retained
  as explicit negative regression evidence. The user's read-only ordinary glide
  samples match this upper package.

The callback validates the selected actor's full identity, alive state,
protagonist kind, possession round trip, component count/ownership, exact
control/method types and control/state/method ownership. It follows the same
active-layer selector as `ab96f0`, accepts only the audited upper/lower slots,
binds classification to the selected layer, and checks complete
156-byte original-record fingerprints, event counts and exact package lengths.
Merely having an old glide animation in an inactive layer does not qualify.
Ownership and animation are captured before and rechecked after the native
call and immediately before publication. Addresses are never cached across
callbacks. Unknown/replaced animations, mounts, other actors, unsupported
methods, invalid memory, bad time steps, non-finite speeds or overflow pass
through unchanged. Recursive dispatch scales at most once.

Only transient per-invocation output is edited; shared tuning/PAAC buffers
are read-only. Publication compares the native values and rolls back only
its still-owned first component if the second is refused, preserving foreign
changes. This output is immediately consumed by the audited native callers.

## Setting and fail-closed startup

`%LOCALAPPDATA%/local.crimson.workbench/glide-speed.dat` is exactly eight bytes:
ASCII `CWG1`, then a little-endian unsigned 32-bit integer, inclusive range
100–500. Native code accepts every integer percentage. Missing, malformed,
oversized, inaccessible, linked, reparse or directory records use normal 100%.
The ASI reads this once at game startup; 100% installs no hook.

Executable header/hash and all 14 native code witnesses are checked **before**
handling the normal setting. An unsupported/synthetic EXE logs `DISABLED`,
never a misleading `NORMAL` success. Hook publication uses a pinned module,
an original trampoline published before activation, one installation attempt
and retained trampoline lifetime after any activation attempt, including a
failed activation. Hook failure leaves native passthrough available.

`READY` means that the selected multiplier's hook is armed. A separate, one-time
`ACTIVE` entry is written only after a nonzero horizontal glide velocity has
actually been scaled. Standing still or merely loading the module cannot
produce that entry.

## Verification

MSVC x64 `/W4 /WX` builds the ASI and owned fixture executable. Public tests
pass **1355 dispatch/consumer invocations**, plus all 401 valid settings,
malformed settings, publication rollback/foreign-change and hook-lifetime
cases. They cover both accepted actor kinds, four native normal methods,
both callers, repeated/recursive use, invalid/stale ownership, all 156
single-byte record mutations, landing exclusion, invalid floats and overflow.
Regression cases exercise both native-selected layers, cross-package
fingerprint rejection, stale gliding on an inactive layer and zero-velocity
activation reporting. Six hundred repeated invocations model incoming velocity
feedback and cached output at 125%, 250% and 500% through both accepted callers;
they verify linear acceleration without exponential growth. Another 240
invocations verify ordinary glide and excluded lower motion retain the exact
native incoming/outgoing velocity at those same multipliers.

These checks use fabricated animation records and an owned C++ movement
consumer. A test-only classifier is supplied explicitly; the default production
classifier retains the original fingerprints and rejects the fabricated records.
The consumer checks 1x/1.25x/2x/5x XZ displacement, unchanged Y and time step,
and no multiplier compounding. The public sources contain no copied original
animation records or integration body.

Optional private verification sets `CRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER` to an
absolute local header path. It supplies the original records and the owned copy
of native `ab8bf0..ab8cef`, redirecting its callee and cell-size constant into
owned memory. That run passes **1371 invocations**, including original orientation
forwarding. The header stays in ignored local storage; public CI needs none of
it, and the release build clears this option. Neither suite accesses a game
process. A separate private read-only audit verifies compiled code witnesses,
both direct callers, method slots, original transition exclusions and absence of
overlap with existing hook entries and independent hash guards.

In-game coverage established so far: the user completed the 100% ordinary/fast
comparison, then accepted the installed correction at 250%. The latter run
retained 171 fast-blend observations with a median horizontal speed of
46.31 m/s and a maximum of 46.32 m/s. These samples belong to the lower-layer
boost record and the native log confirms actual scaling.

Remaining gameplay coverage includes 500%, steering/braking and obstacle
collisions at the higher factor, other protagonists, mounts and unrelated
movement, save reloads and resetting to 100%. Automated fixtures do not establish
every gameplay or collision outcome at high multipliers.
