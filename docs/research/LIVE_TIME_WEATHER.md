# Live time and weather controls

Status: **implemented; the first live session exposed a client-phase validation bug, now corrected and covered by native regression tests**. Field-time selection, advancing future hours, independent time/weather freezes, weather parameters and restoration are implemented behind a pointer-free command queue. The ASI has been installed and loaded; visible weather application and restoration still need acceptance testing with the corrected module.

## Live client-phase correction (2026-10-17)

Read-only status requests showed a connected clock but unavailable weather.
Bounded read-only inspection found the active ClientField in the expected world
and FieldInfo, with a valid `ClientPhase_LivePlay` object in slot **5**. The
adapter incorrectly rejected every phase index greater than 2. The pinned native
dispatcher at `0xa0df38` instead accepts seven slots, indices **0 through 6**;
the pointer table ends immediately before the phase-index byte at `+0x48`.

The corrected bound retains the exact field, current-manager, active-object,
world, freshness and publication checks. The default owned fixture now uses
slot 5, so all existing weather operations exercise the observed gameplay state.
Additional tests cover all seven slots, out-of-range indices, missing phase
objects and inactive weather. All 19 Marketplace native test targets pass.
The known previous module hash is retained for an explicit Marketplace update.
No gameplay mutation was used to diagnose this issue. A game restart is needed
to load the corrected ASI; replacing a loaded module is not a live update.

The original investigation below was performed offline. Shared IPC/UI
integration was verified separately, and those checks do not establish visible
in-game weather acceptance.

## Requested behavior

Choose the actual time and weather while playing, advance by an entered number of future hours, and provide separate **Freeze time** and **Freeze weather** controls in Marketplace. Turning a freeze off must restore native progression. An implementation must not pause global simulation or unrelated quest timers, and must not substitute a visual tint for weather.

## Pinned source and reproducibility

All addresses are RVAs in original Windows build `1.0.0.2976`, PE timestamp `1790086912`, SizeOfImage `389722112`, SHA256:

```text
57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7
```

Private scripts, disassemblies and evidence manifests are under `.local/marketplace-expansion-research/world-controls/`. They read the pinned original and write only owned research files. `checkpoint.py` verifies the bounded witnesses, runs the owned fixture executable and invokes `phase_audit.py`. The phase audit checks 65 code spans, 17 relocated table slots, 67 instruction/import witnesses and three mandatory native call edges ending in the explicit weather-group wait.

## Established clock units

`getCurrentFieldTime` is registered from `0x143e237/0x143e27a`, pointing at native getter `0x20e55a0`. This getter returns a 32-byte FieldTime value. Native conversion helpers are `0x20d3460` (fields to encoded units) and `0x20d3360..0x20d3455` (encoded units to fields).

The verified fields are:

| Offset | Meaning | Native evidence |
| --- | --- | --- |
| `+0x00` u32 | Day | Encode `0x20d3496/34b1`; decode `0x20d3385` |
| `+0x04` u32 | Displayed hour | Encode `0x20d3460`; decode `0x20d3450` |
| `+0x08` u32 | Minute | Encode `0x20d3467`; decode `0x20d3440` |
| `+0x0c` u32 | Second | Encode `0x20d346d`; decode `0x20d342c` |
| `+0x10` u32 | Millisecond | Encode `0x20d3477`; decode `0x20d33fa` |
| `+0x14` u8 | Weekday, day modulo 7 | Decode `0x20d337f..339d` |
| `+0x16` u16 | Progression multiplier read by the getter | Getter `0x20e567a..568a` |
| `+0x18` u64 | Timestamp used to extrapolate elapsed time | Getter `0x20e5658..5685` |

The raw day is **75,600,000 encoded millisecond units**, not 86,400,000. The game compresses the nighttime portions of the displayed clock. With `D` the displayed milliseconds since midnight:

```text
00:00 <= time < 03:00 : raw-within-day = floor(D / 2)
03:00 <= time < 21:00 : raw-within-day = D - 5,400,000
21:00 <= time < 24:00 : raw-within-day = floor((D + 64,800,000) / 2)
raw = day * 75,600,000 + raw-within-day
```

| Displayed time | Raw within day |
| --- | ---: |
| 00:00 | 0 |
| 03:00 | 5,400,000 |
| 12:00 | 37,800,000 |
| 21:00 | 70,200,000 |
| Next midnight | 75,600,000 |

Odd displayed milliseconds in either nighttime interval quantize down by one millisecond. The pure helper in `runtime/marketplace/world-controls/time_units.h` mirrors this conversion with strict field bounds and rejects a decoded day exceeding u32. It does not change progression multipliers, timestamps or game memory.

The native getter selects 32-byte realm clock globals at `0x6a6d238` and `0x6a6d218` using TLS selector `0x1ec`. It can instead return a stage-defined fixed time: `0x20e55c7..55e6` checks the current stage's fixed-time flag and copies its FieldTime. Blindly modifying only one realm or the rendering clock would therefore not implement a consistent world-time command.

The primary getter extrapolates as `encoded_base + (native_now - anchor) * rate`; it has no denominator involving the rate (`0x20e5668..5696`). Rate zero therefore freezes this field calendar without changing global simulation speed. `field_time.h` mirrors the arithmetic, including future anchors, night quantization and overflow rejection. A selection preserves the current day and progression rate. Freeze captures the current native rate, publishes zero once, and resumes at the held time with a fresh native anchor, avoiding catch-up over the frozen interval.

**Advance time** accepts an integer from **1 through 10,000 future displayed hours**. It first obtains the current extrapolated displayed clock, adds whole hours on a 24-hour calendar with checked u64 day carry, then re-encodes the compressed native calendar. For example, 22:15 plus six hours is 04:15 on the following day; plus 48 hours retains the displayed time and advances the day by two. Weekday is recalculated as day modulo seven. Minutes, seconds, progression rate and the current native millisecond resolution are preserved, with a fresh native anchor. Crossing into nighttime can quantize an odd millisecond down by one, exactly as ordinary native encoding does. A target beyond the u32 native day limit is rejected before publication. Ten thousand hours is 416 days and 16 hours; the day-index carry can be 417 when starting late in a day.

Advancing during a Workbench time freeze leaves rate zero and updates the exact owned frozen value, so later observations retain the freeze and Resume starts from the advanced time. Weather selection/freeze and weather receipts are unchanged. The operation uses the same narrow native publication/Ack path as time selection; its field-calendar effects and acceptance boundary are the same as documented below.

## Authoritative clock publication and ownership

`TrocTrSetFieldTimeReq` at `0x29bfd70 -> 0x1049d430` parses FieldTime and reaches `0x2bc8970` through the server actor's transform component. Calling that complete request is inappropriate: it also calls `0x2842c70`, which iterates gameplay records through `0x285be90`, and an optional branch performs stage/position work.

The implemented path uses the narrower publication routine `0x21a1cc0 -> 0xe52e900`. It writes the transform component's FieldTime at `+0x450`, acquires its CommonField, verifies the current realm/world and publishes the corresponding realm clock. The adapter checks both server readbacks, then calls the same native Ack sender used by the original request, `0x299aed0`, with a correctly initialized `ToNearSendTargetFinder` (vtable `0x5b25418`, enabled byte `+8`, actor `+0x10`). The Ack's client handler reaches `0xa38b00` and the same native publication function. A clock `published` receipt means matching authoritative server readback and entry into the native Ack sender; it does not assert client delivery or observed in-game acceptance.

`clock_native.h` independently validates the actor, component count `53..57`, actor identity, possessor backlink, exact ServerTransformSync vtable `0x5b2cf60`, component owner and transition flag before acquiring the field. Component index 52 is `components+0x1a0`. Virtual `+0x140 -> 0x2bc4280` acquires a 0x30-byte FieldReference; virtual `+0x148 -> 0x2bc42c0` releases its transform borrow before `0x1434880` releases its retained field reference. Native borrow count/pending-transition logic is at `0x2bbf340/0x2bbf140`; it is not inferred from an arbitrary critical-section field.

The retained field must be ServerField (`0x5b29a60`). `field+0x180` must match the actor possessor's world ID. Native FieldInfo index `0x2af7d10` and component index `+0x28` must agree. Fixed-field time (`FieldInfo+0x6a`) and a natively stopped field (`CommonField+0x17c`) refuse a new time command with distinct reasons.

Terminology correction from independent original-image review: the getter at
`0x4a2010` names its table `fieldinfo` (`0x5971d78`); its `0x4a5580` loader
allocates `0x78`-byte records and calls `0x1505ab0`. Historical `stage`,
`stage_info` and stage-related reason names in the native/wire client mean this
FieldInfo index, not the separate StageInfo table. The wire layout and checked
fixed-time flag remain unchanged.

Freeze ownership consists only of the exact pointer-free FieldTime last published and the captured positive rate. On a stage/world/identity change or context expiry, the adapter restores the rate only if the raw server clock still exactly equals that owned zero-rate value. A foreign zero rate, changed anchor or other clock change is never overwritten. A fixed/native-paused field may ignore the realm clock; restoration of an exactly owned zero still prevents it leaking into the following ordinary field, without changing the stage/pause flags. Each frame permits one publication attempt. A mutation mismatch or native fault permanently latches that channel against further native acquisition or dispatch.

## Established weather choices and actual state read

The native weather display path `0xe5e6f0` calls `0x17fa700`, receives a u32 bit mask, and chooses its lowest set bit from indices 0 through 5 (`0xe5e770..77d`). At `0xe5e823` it indexes the six-pointer name table at `0x5912e88`:

| Bit | Mask | Native name | String RVA |
| --- | ---: | --- | --- |
| 0 | `0x01` | Rainy | `0x5913448` |
| 1 | `0x02` | Snowy | `0x5913450` |
| 2 | `0x04` | Windy | `0x5913458` |
| 3 | `0x08` | Fog | `0x57b3820` |
| 4 | `0x10` | Cloudy | `0x5913460` |
| 5 | `0x20` | Sunny | `0x5913468` |

These are **state-mask meanings**, not preset resources. Multiple bits may be present. The adapter exposes verified native scalar controls rather than inventing six resource filenames or assigning the mask directly.

The actor getter `0x17fa700` obtains a field through the actor's component at `components+0x1a0`, then reads actual field state at `CommonField+0x178` (`0x17fa790..79b`). A separate native condition can override its returned state to Sunny (`0x17fa7e2..7e5`). The native `ServerWeatherManager` type is identified by RTTI at vtable `0x5b2fe08`. Its constructor site `0x27bf56e..59d` stores the common-field owner at manager `+0x40`.

`ServerWeatherManager+0x38` is a climate-texture readiness flag, set by field initialization after loading `texture/climate_texture_1.dds` (`0x27c18ab..18ba`). It is not a freeze flag or an owning tick.

## Weather producers, owned resources and parameters

The actual weather result belongs to `PAWeatherManager` (primary vtable `0x5d51268`). Its constructor `0x3dbb5b0` creates a fresh WeatherResource via `0x38294b0` and retains its interface at manager `+0x60`. This result is distinct from the input/source influencer vectors at `+0x70/+0x80` and active input at `+0x90`. WeatherResource separately allocates WeatherConstant and AtmosphereConstant. Their primary `+0x10` parent is that result resource, as set by `0x3829637..9674`; the manager registers as a listener and does not replace their resource parent.

The manager's `+0xe8` is the PAWorld constructor argument (`0x3b412ef/0x3dbb6cc`), and that PAWorld owns the manager at `+0xf68`. The adapter validates its exact primary vtable `0x5d20718` (constructor `0x3b3e20a/211`), this backlink, the current manager chain used by the native field consumer (`0x6d69198 -> +0x38 -> +0x38`), the result vtable `0x5ce9df0`, WeatherConstant primary vtable `0x5ce8e58`, AtmosphereConstant primary vtable `0x5cea328`, and both child parent links.

Two proven producer boundaries run the original first:

| Producer | Native caller | Work retained |
| --- | --- | --- |
| `0x3dc4730` | PAWorld job `0x3b106d0`, return `0x3b1073c` | Weather blend, climate sample, precipitation derivation, wind animation and wetness work |
| `0x3dc50e0` | PAWorld update `0x3b4bdc0`, return `0x3b4c180` | Influencer/resource/texture updates, five owned climate outputs via `0x3dc08d0`, and native wind progression |

Native automatic blending at `0x3dc0340` skips when manager `_isManualWeather` byte `+0x31` is true. Merely setting this byte does not freeze weather: the main producer still writes five climate outputs to WeatherConstant data `+0x198..0x1a8`, and precipitation is derived separately. The adapter uses the native reflected manual setter `0x3db7460`, native reflected scalar setters, and native precipitation derivation `0x382a1a0`.

Offsets below are relative to a constant's `+0x28` data/interface pointer; reflected setters receive its primary pointer.

| UI control | Native field | Exposed range | Native evidence |
| --- | --- | --- | --- |
| Precipitation | WeatherConstant `+0x12c` | `0..1` | `_precipitation`, setter `0x37f1ba0`; reflection `0x13a3ab44..ab64`, max 1, step .01 |
| Cloudiness | WeatherConstant `+0x130` | `-1..1` | `_cloudiness`, setter `0x37f2570`; reflection `0x13a3e5b3..e5db` |
| Humidity | WeatherConstant `+0x134` | `0..1` | `_humidity`, setter `0x37f2f20`; native reflection range `-1..1` at `0x13a437b4..37dc`; exposed conservative subset |
| Base wind | WeatherConstant `+0x138` | `0..60` | `_windSpeed`, setter `0x37f38e0`; manual output clamps at 60 (`0x3dc3bb8`, constant `0x5d73554`) |
| Height fog | AtmosphereConstant `+0x9c` | `0..10` | `_heightFogDensity`, setter `0x3809230`; reflection `0x13b79a07..9a27`, step .001 |
| Rain/snow phase | Sign of WeatherConstant `+0x198` | Rain / snow | Deriver `0x382a211..a262`: positive selects rain; nonpositive selects snow |

Base wind and fog are native parameter units; no unproved physical unit such as m/s is assigned. Cloudiness also sets AtmosphereConstant `_cloudBaseDensity` data `+0xa8` to `max(0, cloudiness)` using native setter `0x380af40`; its native reflected maximum is 1 (`0x13b8f288`). Native fog output is `max(heightFog, scaled/clamped aerosol)`; selecting fog sets only the owned aerosol scalar at AtmosphereConstant `+0x88` to zero so that the selected height-fog value actually determines that output. Rain/snow phase changes the native precipitation selector, not the world's calendar or global temperature model.

Choose weather keeps those chosen parameters in the native manual blend mode. Native animation and unheld climate outputs continue. Freeze weather additionally captures/restores the pointer-free WeatherConstant scalar/byte-flag tail `+0x12c..0x1ab` and the three AtmosphereConstant fog/cloud scalars after both producer boundaries. Constructor `0x13ca2801..28d0` proves those fields follow all texture references. Six padding bytes after primary offsets `+0x184/+0x1a4` are excluded from capture, comparison and restore; unproved trailing alignment bytes are excluded too. The 27 WeatherConstant floats and three AtmosphereConstant floats must be finite before any snapshot is accepted. Packed byte flags and padding are never interpreted as floats. No texture/resource pointer, reference count, header, input preset, wind animation coordinate or global time delta is copied.

Unfreezing after a weather selection returns to that selected manual parameter mode. Unfreezing natural weather restores the prior native manual flag; when it was false, a release receipt waits until an ordinary native main producer has actually run with manual=false. **Reset weather** sends the existing explicit weather-release operation: it ends the Workbench weather override and restores the prior native manual policy. If another tool originally owned manual=true, the original scalar snapshot and true flag are restored instead of overwriting that policy. Time selection and time freeze remain independent. Foreign changes to owned scalar inputs or the manual flag relinquish control. A new manager keeps its own native policy; no stale manager pointer is stored or dereferenced.

## Native weather write phases

The two posthooks are justified by the pinned native frame/job graph, not by the queue's SRW lock. That lock serializes only our own callbacks and IPC values.

1. `WinAppFrame 0x617290` calls PAWorld virtual `+0x98` at `0x6173c5`; the exact World table maps it to `0x3b4bdc0`. That update calls secondary producer `0x3dc50e0`. Its call `0x3dc5497 -> 0x3dc08d0` obtains manager `+0x60` and writes five owned climate outputs at `0x3dc095d/961`. The posthook must restore held outputs before the following client read. A main-producer-only freeze would leak these changes.
2. The frame calls client update `0xad2380` at `0x6173ed`. Its retained field update reaches `0x9adf20` through `0xa8acd0`, then dispatches the current client phase through `0xa0dd00`. Live/CommonPlay virtual `+0x18` maps to `0xa0ada0`; Terminate maps to `0xa0c860`. Those phases call `0x3dccaf0`, which calls World virtual `+0xa0 -> 0x3b4cfd0 -> 0x3b10470`.
3. The weather context has exact vtable `0x5d1e820`, whose `+0x10` body is `0x3b106d0`. At `0x3b10648/650`, the fifth argument is destination group **23** (`0x17`), while `r9d=15` is its dependency. `0x1409c50` stores the job/destination tuple under dependency group 15. The pipeline waits dependency 15 through `0x3d1e890`; the wait drains that tuple via `0x1408960 -> 0x1409b90`, launching weather group 23. When native scheduling is disabled, `0x1409c85` instead invokes the job body synchronously before enqueue returns.
4. Within weather job 23, `0x3b10737` calls main producer `0x3dc4730`; our posthook runs at its return, still inside that job. The dispatcher calls job-body virtual `+0x10` at `0x140a039`, then completion virtual `+0x20` at `0x140a044`, then completion publisher `0x1408f10` at `0x140a05d`. The latter releases the group's semaphore at `0x1408fec`. The producer's internal reference releases therefore do not publish job completion before our posthook.
5. Before the next frame, the ordinary launcher path `0x617690 -> 0x617995 -> 0x3dcb940 -> 0x3dcbc71 -> 0x3dcd950` must pass the explicit **Wait group 23** at `0x3dcdb38/41`. The audit checks that each of those three call edges dominates every normal return. The launcher's separate all-group loop is conditional and is not the sole barrier. The alternate Frame path waits groups 0 through 34 at `0x617500..510`. Thus prior main-weather work completes before the next secondary producer/client read. Render group 28 is separate scene work and is not misidentified as weather.

The four gameplay weather-buff setters at `0x3dbc930/970/9b0/9f0` receive manager interface `+0x28`: their `+0x68 -> +0x70` path resolves the active source influencer at manager `+0x90`, rather than the separately owned result at manager `+0x60`. Source/editor resource changes can therefore feed the ordinary producer without directly owning our output snapshot. Generic reflected scalar setters operate on the explicit object passed to them; this audit does not assert synchronization of arbitrary external editor or other-mod writes. Observed foreign manual/input changes cause relinquishment, never automatic reclamation. These static phase witnesses cover the pinned normal gameplay path and do not amount to observed in-game thread acceptance.

## Native weather publication witness

`ClientField::updateWeather` at `0x9adf20`, called under the retained current field by `0xa8acd0` (return `0xa8aed9`), reads the owned manager result. It samples actual rain, snow, an additional climate value, cloud-base density, fog and wind. It writes CommonField `+0x178` under the native field lock and sends the native six-float weather request when the classified mask changes. The server `TrocTrSyncWeatherStateReq` handler (`0x29c5df0`) reaches field classifier `0x20e6fe0`. This path changes gameplay field state as well as graphics.

The third hook observes `0x9adf20` after the original; it never assigns the mask. It validates exact ClientField vtable `0x55bcea0`, the active native buffer, world ID `+0x180` and stage key `+0x2b2` (native getter `0x9a64e0`). A pointer-free fingerprint of the freshly validated current manager/result ties this leased client-field witness to a producer. A weather command requires the selected server world/stage, a fresh server observation and a matching client witness within one second. A weather publication receipt waits for the matching native client-field consumer, including the actual rain/snow classification for a selected precipitation phase. It is an implementation witness; visible in-game acceptance remains unmeasured.

## Rendering controls and rejected shortcuts

The current native console registrations `/settimeofdaylowerlimit` and `/settimeofdayupperlimit` at `0x3984a12/0x3984aa1` bind handlers `0x39859e0` and `0x3985a30`. They obtain a manager through virtual getter `+0x60` and write float limits at manager `+0x3b4/+0x3b8`. They prove render-layer controls, not a timer-safe authoritative world-clock edit.

The separately inspected MIT Trinity reference documents older render offsets `+0x3d4/+0x3d8` and older time/tick signatures. Its exact field-clock and tick patterns have **zero matches** in this pinned EXE. The broad singleton-init pattern happens to match an unrelated-looking initializer at `0x2ce5380`; that match is not accepted as engine ownership evidence. No reference code or guessed historical address is installed by this work.

The implemented clock path changes the authoritative field calendar. Day/night consumers and any quest or environmental rules intentionally based on that calendar can therefore change or stop with it. The adapter avoids the full time request's additional gameplay-record cleanup and never changes global simulation speed or a separate real-time timer. This does not establish that every quest uses a separate deadline; such an assertion would exceed the evidence.

## Dispatch, receipts and lifecycle

The root-owned `runtime/live-items/src/task_observer.h` broker supplies a synchronous observation after its original task and item pump, within its retained actor lease. It revalidates the selected server actor for each callback. The world adapter acquires the independent field lease described above. It installs no second hook on Item Picker's `0x2774b20`.

`control_protocol.h` defines CWW1/version 1: a 96-byte pointer-free Request and 176-byte Reply. Reply offsets 160/164 and 168/172 contain separate time/weather result codes and reasons correlated with their corresponding receipt IDs. Acceptance immediately records queued on that lane. Unsolicited context status has global id zero and does not change an earlier lane receipt. This prevents a weather release from completing a pending time command accidentally.

Operation **7 (`advance_time`)** belongs to the time lane. Its u32 word at Request offset **48** (the existing `time.hour` member) carries the whole-hour delta `1..10000`. Words 52/56/60, all six weather words at 64..84, and enabled at 88 must be bitwise zero; reserved at 92 remains zero. The existing member is read directly, without a union or type punning. Only operations 4 through 6 belong to the weather lane. Session, generation, deadline, strictly increasing IDs and consume-before-publication rules also apply to operation 7; an accepted request is never replayed.

`ControlQueue::handle` inspects/enqueues values only; it calls no game function on the IPC thread. Mutations require a random process session, current world generation, strictly increasing nonzero ID and a monotonic deadline no more than ten seconds away. Expiry can consume pending commands from the status/worker path without game access. World/stage/identity changes cancel queued work. Separate time/weather native faults and mutation mismatches latch the affected lane; there is no retry/replay. Observations become unavailable after ten seconds; weather additionally requires the recent matching client witness. Requests and retained policy contain no borrowed game pointer.

`world_mod.cpp` checks the full original EXE SHA256, PE identity, 65 bounded routine witnesses and 17 relocation-aware virtual-table witnesses before creating bindings. It pins the module before a worker or callback can outlive it. `world_hooks.h` creates all three weather hooks, rejects any null trampoline, publishes all originals before enabling the first hook, and retains module/trampoline lifetime after any activation attempt, including partial activation/disable failure. Nested calls use native passthrough; RAII resets the thread-local guard during C++ exception propagation. The existing actor observer registration pins the callback module for process lifetime. There is no unregister/unload operation.

The root-owned pipe worker uses same-user authorization and denies remote clients; the Rust client authenticates the actual server PID and transmits no command twice. That shared IPC/UI integration is reviewed separately by the parent task.

## Bounded verification result

The Windows x64 Release module and owned fixtures build with `/W4 /WX`. **86,292 offline checks pass**: the original 85,917 conversion checks plus anchored native-rate arithmetic, guarded field acquisition/publication, independent time/weather freeze and release, exact zero-rate ownership, fixed-stage/world-change handling, expiry, foreign changes, weather scalar snapshots with continued animation, native-auto continuation, finite-scalar and padding checks, per-lane receipt interleaving and hook lifecycle failures. Hour-advance checks cover all 24 start hours at +6/+24/+48/+10000 for running/frozen rates, day/week rollover, night compression, u32 day overflow, strict payload validation, time-only availability, native publication and duplicate rejection. Dedicated sequences retain time and weather freezes across a 48-hour advance, reset weather in the secondary producer, wait for an actual ordinary main producer, and verify that time's advanced held value/rate-zero ownership and receipt remain unchanged before later resuming its saved positive rate. The 65 code/17 table witnesses are re-derived from the original hash-pinned EXE by the private checkpoint.

These tests execute only owned code and synthetic native-function fixtures. The ASI artifact exists only in the owned temporary build. It has not been installed, injected into the game or used to call a game function. No real process, save or game archive was changed. Original archive reads used to inspect names were optional research; the implemented scalar adapter has no archive-startup dependency.

## Remaining validation boundary

The parent reviewed the native adapter, original phase/ownership evidence and shared integration, then independently reran the audits, native fixtures, pipe tests, browser scenarios and combined package lifecycle tests. Controlled in-game acceptance, actual client delivery of time Ack, visible weather, world-reload behavior and calendar-dependent quest effects have not been observed. Local build success and fixture publication receipts must not be described as successful game acceptance. Live execution remains outside the current authorization.
