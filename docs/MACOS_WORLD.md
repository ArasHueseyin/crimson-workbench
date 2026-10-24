# Native Mac world controls — implementation notes

Status: development branch `feat/live-macos-world`. Teleport, time and weather
writes and enable/disable controls are **not implemented or shipped yet**.
Existing item/mount functionality and the installed application are unchanged.
Archive-mod work is preserved at `backup/macos-archive-ports-20261024` (`6a265cf`).

## Supported executable

All addresses below are unslid arm64 VAs for SHA-256
`fc0727d176ad7434e9fcea5b358cdb6b7c78ebb1f73975b76efb100c1c129a74`,
UUID `0A2EA57A-719E-36B4-8AB1-EDA609AC004E`. The existing module verifies both
before installing its movement-task observer. Never use these on another build.

## Captured on a loaded save, 24 October 2026

The original read-only probe obtained 24 valid observations after the user loaded
a save and moved. Server actor, local selected actor, field/stage identity, clock
storage and weather-manager ownership all matched. No time/weather/teleport
command was issued. The launch used the existing verified save-backup launcher.
Private logs and save copies remain under `.local/macos-world/observation/`.

The fixed weather path is global `0x108ce55f0` -> `+0x40` -> `+0x08` -> active
world. World `+0xf68` contains the manager; manager `+0xd0` points back to this
world. Manager primary vtable is `0x1081c3b98`. A diagnostic pointer search found
this path; the current source uses only the fixed path and validates the backlink.

Actor `+0x108` -> components `+0x1a0` -> transform (vtable `0x107e33228`, owner
`+8`). Transform `+0x500` contains the field (`0x107e30b10`), phase is `+0x548`.
Field world ID `+0x220` matches user `+0x100`; field definition `+0x4b8`, definition
index `+4` matches transform `+0x28`. Field paused flag is `+0x21c`.

## Read layer staged for the next launch

`CW_MAC_WORLD_OBSERVE=1` enables at most 24 samples, one per second, exclusively
after the existing local server-task checks. Without the flag no world probe runs.
Callbacks serialize diagnostic reads without blocking another callback.

Native read functions below are statically traced and compiled, with fake-binding
tests for ownership, invalid values and exactly-once release. Their returned values
still need confirmation in a loaded world with the new module:

| Function | VA / ABI |
| --- | --- |
| Acquire field lease | `0x100863b14`, 48-byte aggregate result in X8 |
| Full lease destructor | `0x100e79bf4`, releases borrow and generic reference once |
| Current displayed field time | `0x100e94448`, 32-byte aggregate result in X8 |
| Actor world position | `0x100ec7748`, three-float HFA result in S0–S2 |
| Native realm clock | `0x100e350e0`, u64 result |

Lease vtable is `0x107e74b68`; field pointer is `+8`, valid byte `+0x10`,
transform owner `+0x28`. Do not separately call transform release and the full
destructor: that would release twice. Native virtual slots differ from Windows.

Weather result is manager `+0x60` interface minus `0x28`; primary resource vtable
`0x1081ca110`. Resource `+0x40/+0x48` are constant/atmosphere interfaces. Their
primaries are interface minus `0x28`, vtables `0x1081ca690/0x1081c9e40`; each
primary `+0x10` must point to the resource. The new read layer validates all of
these and finite scalar values before reporting ready.

## Traced mutation paths; not called yet

- Narrow authoritative clock publication: `0x100eaba18(transform, FieldTime*)`.
  CommonField setter is `0x100e94744`. Native nearby-target constructor
  `0x100c2f364`, ACK sender `0x10099ed7c`, target destructor `0x100c2f104`.
  Avoid whole set-time request `0x10086ed4c`: it also invokes progression and
  optional position/field changes. Server raw clock is `0x108a6ab00`, client is
  `0x108a6aae0`. FieldTime layout matches the existing Windows value type;
  nonlinear day/night arithmetic still needs runtime comparison on Mac.
- Weather producer job `0x10361ea0c`, vtable slot `0x108288ac8`, primary job
  vtable `0x108288ab0`. It obtains world from job `+0x90`, passes float delta from
  `+0xa0` to manager update `0x102dd0aa8`. Update calls `0x102dcc254`; manual flag
  is manager `+0x31`. Prefer this data-slot callback over an inline code patch.
- Weather native setters: manual `0x102dc2e3c`, precipitation `0x102e78c48`,
  cloudiness `0x102e791f8`, humidity `0x102e797a8`, wind `0x102e79d58`, atmosphere
  fog `0x102e876a0`, cloud density `0x102e887b0`. Setters take primary object and
  pointer to scalar. Precipitation derivation `0x102e9b11c(resource*)`.
- Field transition begin `0x10086018c(context, NativeFieldMoveValue*)`, context
  is transform `+0x4f0`; copies a 0x38-byte value and returns native result code.
  Do not confuse it with broader request wrapper `0x100866fa0` (additional
  replication/state side effects) or move-validator `0x100ea5d14` (takes a move
  argument; not a no-argument eligibility check). Character definition accessor
  `0x100ec5264` is components `+0x20` -> `+0x30`; yaw is `0x100ec7a9c`.

Remaining: confirm native reads on restart, establish teleport eligibility and
arrival observation, add bounded queues/receipts and Mac socket endpoints, connect
existing frontend controls, and validate publication and disable/restore. Restore
must be conditional on still owning the same field/manager and unchanged owned
values. No automatic resend after native uncertainty.

Run the focused checks with `make -C runtime/macos-items all test`.
