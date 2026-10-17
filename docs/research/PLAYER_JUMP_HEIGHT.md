# Player jump height

Local implementation, 17 October 2026. No module or setting from this expansion
has been installed into the user's game. In-game acceptance is pending.

## Verified native path

Build 1.0.0.2976 executable SHA256:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.

The jump consumer at RVA `88bf40` accepts a borrowed character control state,
an additional argument and two byte flags. Its 672-byte body has SHA256
`789e9485a26d41cbefd94fd4d5c222e64002eceff78c6207883c3d558810df88`.
It checks the pending-jump byte at state `+320` and blocking byte at `+34d`,
then derives initial vertical velocity from pending height `+32c` using
`sqrt(2 * abs(native gravity) * height)`. A successful invocation clears both
the pending flag and pending height. The module scales the height input,
leaving the native conversion and gravity intact.

The consumer belongs to the currently selected client actor only when fresh
selection, identity, actor type, possession, component owner and control-state
checks all agree. The client root is resolved afresh for each invocation. No
actor, component or state address is retained between calls. Mounted animals
and unrelated actors fail these ownership checks.

The wrapper changes one pending height immediately before the original call.
It forwards every original argument and the original boolean return value. If
the original declines the jump, the wrapper restores its still-matching edit
only while the borrowed actor identity and component/control ownership remain
valid. This restoration also handles a selection change inside a callback.
An original replacement value is preserved. A thread-local invocation guard
prevents recursive callbacks from applying the multiplier again.

The independent lion/Rokade module hooks jump-event RVA `91d0520`. The player
module hooks the consumer prologue at `88bf40`; neither module patches the
other's hook or code-validation span. Mount jump height remains **+40%**.
This does not remove a Force Palm repetition limit or change a dismount rule.

## Settings and Marketplace behavior

The English Player jump height card has a **100–300%** slider in 5-point steps,
an explicit Save jump height action and a Reset to normal draft action. Moving
the slider does not write a file. Installing while there is an unsaved setting
is disabled. Saving and module installation/removal are separate actions.
Game-running guards and unsupported-build guards apply to setting writes.

Settings use the current Windows user's local application data folder:
`local.crimson.workbench/player-jump.dat`. Tauri resolves its application local
data directory; the ASI resolves Windows' LocalAppData known folder and appends
the same application identifier. The record is exactly eight bytes: ASCII
`CWJ1`, then an unsigned little-endian height percentage. Only 100 through 300
are accepted. Missing, damaged, oversized or unsupported records use normal
height in the ASI; non-regular, linked or inaccessible files are not used.

Saving publishes a checked same-directory temporary file atomically, without
editing game files or other preferences. A second process check before
publication refuses a game started during preparation. The ASI reads the
setting once at startup after validating the executable. A normal setting
installs no hook. Updated settings apply at the next game start.

## Local verification and pending game checks

- Rust fixtures cover exact record bytes, normal/default behavior, both range
  boundaries, malformed records, explicit repair, atomic replacement,
  unrelated-file preservation and refusal when the process state is running,
  unknown or changes during publication.
- Native fixtures cover selected-player ownership, original argument/return
  forwarding, repeated jumps, normal height, mount exclusion, blocked and
  absent jumps, stale selection/ownership, invalid float inputs, recursive
  entry, failed-jump restoration and invalid-memory fallback.
- Marketplace browser fixtures cover explicit saving, draft reset, an
  independent install action, failed saves and running/unsupported locks.
- Native packaging validates both jump modules together, including independent
  removal. Loader smoke tests use an unsupported synthetic executable and
  must disable every runtime module before a game hook can be installed.

Still required in the game: measure normal, 140% and 300% jumps from the same
flat surface, repeat while moving, change controlled character, enter/leave a
mount, and verify unchanged mount jumps, gravity, landing behavior and native
jump eligibility. Test the player and +40% mount modules together. Local
fixtures establish code behavior, not measured in-game height or stability.
