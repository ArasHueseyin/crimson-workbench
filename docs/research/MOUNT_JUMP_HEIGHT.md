# Lion and Rokade jump height

Local implementation checkpoint: 17 October 2026. The user changed the requested
height increase from 30% to **40%**. This module has not been installed into or
loaded by the game. Both mounts still require in-game acceptance testing.

## Native height evidence

Supported executable SHA256:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`
(game build 1.0.0.2976, PE timestamp 1790086912, image size 389722112).

`ClientFrameEventJump` execute thunk RVA `6c38a0` leads to `91d0520`. Its native
actor-reference argument identifies the actor receiving the jump. The event
reads its data pointer from the third argument at offset `08`, copies 24 bytes
of jump data, reads the resource/status reference at offset `18`, and uses the
float at offset `0c` for the pending jump scalar. Native status/skill adjustment
remains in that event. It writes the resulting scalar to movement state `32c`
and sets the pending flag at `320`.

The consumer at `88bf40` establishes the scalar's role:

- It checks the pending flag at `320` and the original blocking flag at `34d`.
- It derives the native gravity magnitude, allowing the existing override at
  `324` to take precedence when positive.
- At `88c054–88c07d` it computes the square root of twice the gravity magnitude
  multiplied by the pending scalar at `32c`, then stores the initial vertical
  speed at `340`.
- It retains the original horizontal-motion calculations and clears the pending
  flag and scalar at `88c1b8–88c1bf`.

This is the native `v = sqrt(2*g*h)` conversion. Multiplying the **height input**
by 1.40 produces the requested nominal height increase; multiplying initial
vertical speed by 1.40 would instead multiply nominal height by 1.96.
Collisions, ceilings, slopes and subsequent animation/movement remain subject
to the game's original rules.

## Implementation and scope

`CrimsonMountJump.asi` hooks the pinned native jump-event body once. For each
invocation it checks the actor's liveness, admitted native actor kind, component
array bounds, status/control owner backlinks and movement-state owner backlink.
The loaded CharacterInfo manager at RVA `6d69a48` resolves the current status
component's definition handle. Only the semantic definition keys **1002316**
(circus lion) and **31378** (Rokade) qualify. Indices and actor pointers are not
cached. Ownership and definition references are rechecked before acceptance.

For a qualifying invocation, the module gives the synchronous original function
a private copy of the 16-byte event reference and 26-byte jump data. It changes
only the float at jump-data offset `0c`, multiplying it by **1.40**. Source action
data and native actor memory are not written by the policy. The original
function retains all eligibility/status checks and performs its normal pending
jump update. The event body does not retain the temporary reference or data.

Zero, negative, non-finite and overflowing heights, unknown/dead actors, missing
components and guarded read failures forward the original event unchanged.
Ordinary horses, other animals and the three player definitions do not qualify.
Repeated jumps cannot compound the multiplier because the original action data
remains unchanged. Size, movement speed, stamina, fall damage and the forced
dismount threshold are separate features.

Startup checks the complete executable hash, PE metadata, jump-event prologue
and native height-to-speed instruction sequence. Unsupported executables never
enable the hook. The separate English Marketplace card is **Higher lion &
Rokade jumps**. Installation/removal uses the existing closed-game ownership and
recovery workflow and takes effect at the next game start.

## Verification and remaining acceptance

Passed locally:

- 52 native dispatch checks on owned fixtures: both definition keys, all six
  admitted actor kinds, unrelated characters, malformed ownership, definition
  index changes, temporary-state changes, exceptional reads, invalid floats,
  repeated non-compounding jumps, original arguments and result forwarding.
- Native marketplace policy tests and the eight-ASI loader smoke test on an
  unsupported synthetic host, which must reject the host before enabling hooks.
- Six Marketplace module install/remove lifecycles, backups, conflict refusal,
  rollback/recovery and running-game refusal in synthetic test directories.
- Frontend build, 20 Marketplace browser tests, desktop Rust check and visual
  inspection of the new card.

The earlier read-only actor inspection confirmed the player character-control
component and state-owner route. A later optional live-code check was unavailable
because no Crimson Desert process was found; it did not establish a loaded mod
test and no game restart or installation was attempted.

Pending acceptance: observe the native jump route on **each mount**, measure
40% higher ordinary jumps from a consistent flat surface, test standing/moving
jumps and repeated jumps, verify normal player/other-horse jumps, and confirm
removal restores the original jump behavior. Test compatibility with the size,
speed and fall-protection mods and with the future separate five-times forced
dismount threshold. Local fixtures do not prove these gameplay outcomes.
