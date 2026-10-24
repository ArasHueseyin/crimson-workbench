# Native Mac world controls — implementation notes

Status: development branch `feat/live-macos-world`. Native clock and weather
controls, their Mac socket, Tauri connection and frontend panel are implemented.
Both passed a bounded live trial on 24 October 2026; the user confirmed visible
rain. Saved-position teleport is implemented but awaits its first live trial. The installed app has not yet been
replaced. Existing item/mount functionality is preserved; archive-mod work is
preserved at `backup/macos-archive-ports-20261024` (`6a265cf`).

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

## Read layer and native clock calibration

`CW_MAC_WORLD_OBSERVE=1` enables at most 24 samples, one per second, exclusively
after the existing local server-task checks. Without the flag no world probe runs.
Callbacks serialize diagnostic reads without blocking another callback.

Native reads below were confirmed on a loaded save in the next launch. The field
lease returned the expected field and transform owner, displayed time advanced and
player position was finite. A loading stage returned hour 24 and was rejected.
Ownership, invalid values and exactly-once release also have fake-binding tests.

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

Weather result is manager `+0x60` interface minus `0x28`; primary **WeatherConstantGroup** vtable
`0x1081ca3c0` (not the base WeatherResource vtable `0x1081ca110`). Resource `+0x40/+0x48` are constant/atmosphere interfaces. Their
primaries are interface minus `0x28`, vtables `0x1081ca690/0x1081c9e40`; each
primary `+0x10` must point to the group. The group constructor is `0x102e9ab70`;
`0x100d9cbb8` assigns child parents. The manager registration calls
`0x102e76f34/50` only add observers through `0x100d9a9b4`; they do not change
parentage. The first reader used the base resource type; a later correction also
misread observer registration as parent assignment. Both are corrected in source,
and the corrected output has been confirmed on a loaded save.

Raw native clock `{50,18,11,56,232,weekday=1,reserved=140,rate=12,anchor=53830}`
at native time 98767 produced exactly the captured getter value
`50 / 18:20:55.476 / rate=12 / anchor=98767`. This case now exercises the reused
pure clock arithmetic in a regression test. The reserved byte is native padding.

## Pinned native mutation paths

- Narrow authoritative clock publication: `0x100eaba18(transform, FieldTime*)`.
  CommonField setter is `0x100e94744`. Native nearby-target constructor
  `0x100c2f364`, ACK sender `0x10099ed7c`, target destructor `0x100c2f104`.
  Avoid whole set-time request `0x10086ed4c`: it also invokes progression and
  optional position/field changes. Server raw clock is `0x108a6ab00`, client is
  `0x108a6aae0`. FieldTime layout matches the existing Windows value type;
  nonlinear day/night arithmetic matches the captured Mac getter case above.
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

## Clock command path

`world/world_control.h` keeps pointer-free requests and receipts under a mutex.
It accepts only a fresh selected identity, current session/generation, increasing
request ID and a bounded deadline. Publication runs only from the verified server
actor callback. Each request is consumed before its single publication attempt.
A world change cancels queued commands; uncertain publication latches time only,
without disabling item/mount grants.

`world/time_native.h` reacquires the field lease, rechecks ownership, unchanged raw
clock and the loaded FieldInfo fixed-time flag, then calls the narrow publisher.
Both the global and transform clock must match before the native ACK sender runs.
A published receipt means server readback and ACK invocation. The live trial also
confirmed matching client/server clock fields and rate after freeze and resume. No command is retried after a failed/uncertain response.

Freeze captures the current rate; Resume time restores it at the frozen displayed
time with a fresh anchor. Resume changes only an exactly matching owned freeze.
Freeze ownership is process-local; persistence across saving/restarting while
frozen has not been checked. Resume before closing the game during development.

The `/tmp/crimson-workbench-UID/world-PID.sock` endpoint uses the same private
0700 directory, 0600 socket, same-UID server check and bounded transfers as items.
The Rust client verifies UID and peer PID before sending exactly one 96-byte
request and reading one 176-byte reply. The Mac dialog reuses the existing live
controls and pending-request history for both time and weather.

## Weather ownership and live verification

`world/weather_control.h` runs before the ordinary weather producer through the
verified non-executable vtable data slot. The original job signals downstream
consumers before returning, so writes must precede it. Receipts are confirmed on
the next producer callback, including derived rain/snow outputs. The original
job is always called, and its return value is preserved.

The module acquires an automatic manager only, refuses foreign manual overrides,
and relinquishes ownership after foreign changes. Requests share the clock's
session, generation, replay counter and bounded deadlines. Reset toggles native
automatic mode and waits for ordinary production. World changes request release
on the original manager; no retained pointer is dereferenced on a new manager.
Uncertain publication latches weather without resending the command.

Mac weather freeze holds the chosen scalar settings and precipitation type.
Climate calculations and animations continue. The frontend states this limit.

The live trial froze time at day 50, 22:55:22.056, applied light rain
(precipitation .15, cloudiness .8, humidity .5, wind 4), and observed the ordinary
producer retain it. The user confirmed rain appeared. Reset restored automatic
weather while the clock remained frozen; resume then restored rate 12. Native
client/server witnesses agreed at rates 0 and 12. Final status had both controls
available and neither frozen. Private evidence is in
`.local/macos-world/weather-trial.log` and the launch's `runtime.log`.

## Saved-position teleport (awaiting live validation)

The first Mac implementation reuses the existing 112-byte request / 14016-byte
reply, UTF-8 name checks, 16 slots and pending-command UI. It accepts no frontend
coordinates. Bookmarks are session-only; the Mac panel states that they disappear
when the game closes. Persistent bookmarks and map-pin travel are not included.
The Windows persistence and marker paths are unchanged.

`teleport/native.h` acquires the pinned field lease and checks field/stage keys,
component ownership and the native movement error bits. The eligibility function
`0x100eaaff8` takes transform, int output and byte output; both outputs start at
zero. It acquires/releases its own field lease and actor read lock. The separate
actor guard constructor `0x1000a4bdc` returns 24 bytes in X8, including its native
reference bookkeeping; `0x1000a4c68` releases it. Position and yaw getters run
inside this guard, then it is released before Begin acquires its own lock.

A move reacquires the field lease, checks the same field and character, idle
transition state, positive borrow count and empty pending descriptor, then checks
the current selected actor and deadline immediately before its single Begin
call. The lease defers transition while the copied descriptor is checked. Its
full destructor releases exactly once and allows native streaming to continue.
A successful queue receipt is distinct from observing the player within 0.5 world
units of the destination. No uncertain command is automatically resent.

The `teleport-PID.sock` endpoint uses the same private directory, same-user and
peer-PID checks as the existing connections. Fake native bindings cover owner,
field, eligibility and final-authorisation rejection without invoking Begin,
exact descriptor publication, lease/lock cleanup, replay suppression and arrival
after a temporary loading observation. Rust verifies the complete 14016-byte
socket transfer. Mac browser coverage checks the session limit, disabled markers
and no mutation on panel open.

Remaining: restart the game with this module, validate the native capture/guard
ABI and run one explicit captured-position return trial. Do not distribute the
teleport path as live-verified before that trial.

Run focused checks with `make -C runtime/macos-items all test`, Rust app tests and
Clippy, and the `mac-mods`/`world-controls` browser tests.
