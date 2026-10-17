# Extended photo mode — supported native route

This component exposes the game's normal photo mode through a pointer-free local command API. It requests the normal movable PhotoCamera, can hide/show its HUD and controls, and uses the native photo UI's scene pause. Camera movement continues through the game's normal photo camera inputs. It does not change the time of day, grant a debug free camera, invent a gameplay phase number, write time-scale fields, or retain borrowed game pointers.

Implementation and owned offline tests are complete. Gameplay acceptance has **not** been performed. The running game was not accessed, injected into, controlled, installed into, or restarted during this work. Only the original executable/resources were read; native bindings in the tests point to owned mocks. The built ASI remains in the ignored private build folder.

## Supported image and evidence

- Build 1.0.0.2976, timestamp `1790086912`, image size `389722112`.
- EXE SHA-256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
- `photo_targets.h`: 32 audited code ranges and 6 vtable entries. Startup verifies the full EXE hash, PE identity, code hashes and relocated table targets before installing one hook.
- Original `ui/xml/gamemain/play/photomodepanel.html`: 24524 bytes, SHA-256 `efa4d783c2562a743942731516279ced2eaef393024dece74578355d2719adb9`.
- Original `photomodepanel.css`: 7220 bytes, SHA-256 `5391ba70a7e4b7240241446a4b96345f9fc0f1d79e37449260ef34fd2cff63db`.

These resource entries have flags `0x32`. The private reader uses the existing `crimson-format::Archive` API: registered PAMT metadata, pack-entry decryption and LZ4 decoding. It does not modify the original archives. Decoded resources and disassembly stay in ignored `.local/marketplace-expansion-research/photo-mode/`.

## Normal camera and UI lifecycle

| Native evidence | Meaning used by this adapter |
| --- | --- |
| `aab0e0`, RTTI/vtable `55ca780` | CameraManager creates and owns its `PhotoCamera` at `manager+10`. The separate debug `FreeCamCamera`, vtable `55ca438`, is at `manager+18` and is excluded. |
| `10ddad0(UIview, bool)` | Original normal photo request, called with `true` by the pause-menu handler at `108eea9`. The adapter calls this original entry. |
| `10ddb45..10ddb65` | Native fast path queues the request before looking up an instantiated panel/controller. The adapter therefore permits a missing photo panel before entry. |
| `63f530`, `64c3c0` | Native request creates `UIPhaseData_PhotoMode` (type byte 9), acquires its own `393650` guard and transfers the allocated request into the native phase queue. No mod-created request layout or ownership is passed to the engine. |
| `63e240`, `7c7401` | Native photo data dispatches to the registered `SetSubPhase_PhotoMode` action. The adapter does not substitute a phase enum. |
| Photo controller vtable `56f1968`, slot `80` â†’ `10debe0` | Native activation calls `aabf80(cameraManager, 1, options)`. Native mode 1 selects the normal PhotoCamera and invokes the engine's stack/reference/pose lifecycle. |
| Slot `90` â†’ `10ded40` â†’ `aac310(manager, 1)` | Native deactivation removes that camera through the matching native stack/descriptor lifecycle and restores the game camera pose. |
| `10de190(control, null, closeWidget, false, null)` | Original photo input handler with the freshly resolved normal close widget at `control+170`. It preserves native input/type/eligibility checks and performs native UI closure and phase-queue exit. The fifth argument is unused by this close branch. |

`aac310` has split unwind entries. Its logical body is `aac310..aac4f7`; a pin of only its first unwind fragment would be insufficient. The private audit and checked-in code hash cover the logical body.

The original UI body selects `UIGamePlayControlPhotoMode`, `inputGroups="CameraRotate"` and `timeScale="0"`. It binds PhotoModeCameraMove, Rotate, MoveAxis, PrecisionMove, FOV and other normal photo controls. `PhotoCamera` activation/update slots `320`/`2d8` point to `aaa5a0`/`aaa7a0`. Native movement, collision/range limits, controller configuration and camera constraints remain engine-owned. This component does not promise an unlimited debug flight camera.

## HUD hiding and restoration

The original panel binds `PhotoModeHideAll`. Its normal handler reaches `10df010`, which sets the controller's hidden flag at `218`, closes the photo widgets/controls through their normal virtual functions and sets the normal camera's corresponding UI/input state. The paired `10dee30` restores controls and calls the normal `10df1c0` option-state path. The mod calls these original functions; it does not patch a global HUD flag or hide unrelated UI roots.

The controller is looked up fresh via `35da2a0(UIview, "PhotoModePanel")`. The native single-instance path is reconstructed from `10ddad0`/`10de190`: panel metadata â†’ widget `120` â†’ instance array `40`/count `48` â†’ instance `d8` controller. The adapter requires exactly one ordinary instance, no override branch, the expected vtable, the controller/instance backlink, the body's script pointer at `118`, and its native time-scale value at `23c` equal to zero. A normal `3e72bb0` getter must resolve that controller back to the borrowed primary UI view.

A missing, loading, replaced or unsupported photo controller leaves `controls=false`; it does not destroy the stable context or block requesting entry. HUD and close commands wait for newly validated controls, within their execution deadline. No borrowed controller pointer is stored between frames.

## Scene pause, not a clock-only effect

`35cd990` computes active UI time scaling. `ad1300` consumes it, smooths the effective UI value through `UIview+312c0`, multiplies the simulation delta at `Frame+60 â†’ +64`, and passes that scaled delta into the client actor update at `ad258d`. The original photo UI requests zero through this existing path. The adapter does not write a delta, global time-scale, clock hour or debug freeze property.

The status flag `scene_frozen` requires all of:

1. The actual top camera descriptor is normal mode 1 and references the manager-owned PhotoCamera.
2. Native `35cd990(UIview)` returns zero.
3. The final native scaled simulation delta is zero while raw frame delta at `+6c` is positive.

This prevents a zero UI target during its fade, a zero wall-clock sample, or a paused day/night clock from being reported as an observed scene pause. Native UI/render/camera updates retain their normal unscaled paths. Complete visual acceptance, including animated environment effects and restore behavior, still requires a deliberate in-game test.

## Thread, lifetime and context ownership

The only hook is `35d8080`, the existing native UI update. The original runs first. Mod work is admitted **only** at return address `ad2502`:

`ad2499..ad24b5` enumerates the native UI view registry. For the primary view, `ad24de` reads its native lock at `31298`, virtual slot `8` acquires it, `ad24fd` calls the UI update, and slot `10` releases it after return at `ad2509`. The other call returns at `ad2516` and has no such lock; the mod always passes it through. Recursion is guarded and the callback never waits on the mod's own atomic flag.

Global `6d691e8` is the aggregate stored at **Frame+e8**, not an alias of Frame+18. `ad0fb0` proves its sources:

| Aggregate field | Native source |
| --- | --- |
| `50` | Frame+58 CameraManager |
| `58` | Frame+60 simulation/raw delta state |
| `68` | `[Frame+18]+88` UI view registry |
| `90` | Frame+28 ClientRoot |
| `98` | ClientRoot+1188 native UI phase manager |

Each snapshot validates that aggregate `68 â†’ 78[0]`, with a nonempty native count at `80`, is precisely the borrowed primary UI view. CameraManager and PhotoCamera RTTI/vtables, bounded native mode descriptors and context members are revalidated. UI lookup, native getters and native actions run on this leased UI callback; no game function is invoked from the pipe worker.

The policy retains only scalar state, command data and an opaque fingerprint of stable context members. Photo controller/body addresses do not participate in that fingerprint, so deferred panel creation does not masquerade as a context change. Borrowed Frame pointers are local to the current callback. Read faults from the adapter's own guarded reads fail closed; native calls are outside SEH and native C++ exceptions propagate. The module is process-pinned before its asynchronous initialization worker starts. Trampolines publish before hook enable; failed/uncertain activation retains published resources in passthrough.

## Command contract and asynchronous results

Pipe: `\\.\pipe\CrimsonWorkbench.Photo.v1.<PID>`, current-user ACL, local clients only, exact message sizes and bounded I/O. `photo_protocol.h` defines Request **56 bytes**, Reply **64 bytes**, protocol 1, magic `0x48505743`.

Operations: status=0, enter=1, hide_hud=2, show_hud=3, leave=4. State/reason numeric values are stable in the header. A status request discovers the random session and current context epoch. Commands require that session/epoch, an increasing nonzero ID, a recent observation and an absolute execution deadline no more than 3000 ms ahead. `hide_on_enter` is 0/1 for enter; other commands use 1. Default entry hides controls after activation is observed.

Transport receipt, native request entry and observed activation are distinct. `native_queued` does not claim that the engine accepted or displayed the mode. Ownership is established only after a requested normal photo mode and actual native scene pause are observed in the same stable context. An already active or covered/debug camera is not adopted. HUD/leave commands act only on an owned mode with fresh native controls. A normal in-game exit is observed and drops ownership without reopening anything.

Each accepted command is issued at most once. Reusing its ID returns the stored result; changed/reordered IDs are rejected. Temporary snapshot unavailability preserves the pointer-free receipt, epoch and outstanding observation state; it never replays an issued entry or exit. A positively observed different stable context drops ownership and pending work. The observation deadline is checked before every success branch. An unobserved result after the deadline becomes `uncertain`, without an automatic retry or speculative cleanup. If an entry appears only after its observation deadline, it is not adopted and cannot trigger deferred HUD hiding; normal in-game photo close remains available. A late HUD or exit observation updates the current native flags but cannot turn the expired receipt into a success. An actually closed camera still drops ownership. Manual and Workbench photo transitions should be exercised separately during acceptance because the native void API supplies no per-request completion token.

## Owned offline verification

The independent CMake project uses pinned MinHook 1.3.4 and MSVC x64. It builds `CrimsonPhotoMode.asi` and `marketplace-photo-mode-tests` under ignored `.local/marketplace-expansion-research/photo-mode/build`; nothing is installed or loaded.

Tests cover exact callback lease/passthrough, normal native-call argument ABIs, camera type and descriptor checks, effective versus requested pause, original getter membership, guard-page reads, optional/multiple/invalid panel data, complete entry/hide/show/leave flow, foreign camera refusal, session/epoch/deadline/ID checks, no automatic retries, propagated native exceptions, deferred first panel creation, snapshot gaps during entry/exit, first success observations after entry/hide/show/leave deadlines, expired deferred HUD hiding and hook resource lifetime.

```powershell
# From the repository root: download pinned dependencies, build and test all modules.
.\scripts\build-native.ps1 -OutputDirectory '.local/native-verification'
```

The original asset reader is `cargo run --offline --manifest-path .local/marketplace-expansion-research/photo-mode/extract/Cargo.toml -- 0012 photomode`. `pin_evidence.py` regenerates only owned research files and `photo_targets.h`; `verify_evidence.py` is read-only. Do not regenerate source after handoff freeze without coordinating with the integrating reviewer.

Remaining acceptance: first use without opening photo mode beforehand; keyboard and gamepad free-camera movement while actors are frozen; complete HUD disappearance and restoration; entry/exit while controls load; native in-game exit; save/world/menu transitions; and coexistence with the other packaged hooks. These are not inferred from passing mock tests.
