# Live world-control client

17 October 2026. The client, native module and Marketplace integration are
locally implemented. Native weather-phase review, original-image checks,
owned native/pipe fixtures and browser scenarios passed. In-game acceptance
remains for the user's test installation; automated checks do not claim it.

`MarketplaceWorldControls.tsx` provides explicit time selection, independent
clock freeze/resume, forward travel by 1–10,000 game hours across day changes,
precipitation phase and intensity, cloudiness, humidity,
wind strength, height fog, weather freeze/unfreeze and restoration of previous
weather control. **Reset weather** disables the Workbench weather override while
keeping the time setting/freeze; it does not require removing the ASI. Changing
a draft field sends nothing. The normal install/remove
actions remain in the parent runtime card and require the game to be closed.

The controller uses CWW1: a 96-byte request and a 176-byte reply. The first
160 reply bytes retain their original layout; the final 16 bytes hold separate
time/weather result codes and reasons. Each result belongs to its own receipt
ID. An unsolicited status change or an outcome from the other control lane
cannot prove a command completed. Native 64-bit session, generation, request,
deadline and receipt values cross JSON as canonical decimal strings and use
BigInt for client arithmetic.

The Tauri commands `world_status` and `world_change` use `world_service.rs`.
The selected game root is copied while holding the catalog session mutex, which
is released before bounded IPC. Mutations require supported version 1.0.0.2976.
The running process must have the selected canonical executable path, and the
named-pipe server PID must match that process. The pipe permits only its current
Windows user and rejects remote clients. Message boundaries, lengths and ABI
fields are checked in both directions. A bounded acknowledgement lets the
server disconnect after the client consumed its reply.

Before transmitting, the client saves the request identity and deadline to
local history. A transport failure or panel reload only queries status; it never
reconstructs or repeats the mutation. New changes stay blocked while the command
is pending. Process/world changes, uncertain outcomes and overwritten receipts
are shown explicitly. A user can dismiss an uncertain command after checking the
game; the monotonic local ID counter is kept. Native context/generation/deadline
checks still apply to every subsequent explicit action.

## Owned verification

- MSVC `/W4 /WX` builds the pipe worker and standalone
  `marketplace-world-pipe-host.exe`; all host values are synthetic.
- Five Rust tests pass, including the real Windows pipe against that owned host,
  exact large integers, payload validation, malformed/truncated/oversized frames,
  reconnection, distinct result fields and the bounded hour-advance payload.
- Five TypeScript tests cover identity arithmetic, unavailable lanes,
  operation-specific values, persisted pending data, per-lane results despite an
  unrelated global status, and process/world changes.
- Six browser tests execute the real component against a synthetic Tauri
  endpoint: explicit changes and independent freezes; lost-response/reload
  without retransmission; disabled setup/unavailable context/broken history;
  a world change that preserves pending-result acknowledgement; and weather reset
  that keeps a simultaneous time freeze; and a 48-hour advance that preserves
  both active freezes and weather values.
- The frontend build passes. The parent inspected the rendered controls.

The browser fixture is `app/tests/world-controls-fixture.html`; it is a test
entry rather than an application route. Logs are ignored under `.local/` as
`world-control-pipe-tests.log`, `world-controls-policy-tests.log`,
`world-controls-browser-tests.log` and `marketplace-world-controls-ui-build.log`.
No game process receives any request during these tests. The native clock and
weather adapter's original-code evidence is documented separately in
[Live time and weather controls](LIVE_TIME_WEATHER.md).

## Marketplace integration and parent review

The **Live time & weather** card installs `CrimsonWorldControls.asi` separately
and opens the live panel while the game is running. Its availability label is
**Adjust while playing** with **Close game for setup**. Closing the panel keeps
native settings active. **Reset weather** releases weather only; **Resume time**
releases the owned clock freeze. Installation checks the exact bundled LiveItems
hash, because older runtimes do not export the borrowed-task observer broker.
Missing or mismatched brokers are refused before module publication; removal
preserves the broker and the other mods.

The parent reviewed the original weather job order and the mandatory group23
wait, reran the 65-code/17-table phase audit and 86,071 native fixture checks,
and independently tested the real Windows pipe against the owned C++ host.
The combined package passes all 15 Marketplace native targets, the 17-ASI
synthetic loader host and temporary installation/removal/recovery tests.
The Marketplace modal browser test covers setup/running/version gates and
ensures opening or closing the panel sends no mutation.

The time section also offers **Hours to advance** and **Advance time**. It
accepts whole numbers from 1 to 10,000. This moves the displayed calendar forward
across days, preserving minutes, seconds, weather and any active clock freeze.
The current native day is displayed beside the time so a 24/48-hour jump is
visible even when the hour of day stays the same. The distinct JSON operation
`advance_time` has only an `hours` payload; Rust encodes operation7 with the delta
at word 48 of the unchanged 96-byte request. All other time/weather words remain
zero. No request is generated while editing the number.

Five Rust codec/owned-pipe tests and six world-control browser scenarios pass,
including a 48-hour jump with both freezes active. The same receipt and pending
history rules prevent automatic repetition after a lost response. Calendar
advancement can affect game rules based on time; the native sleep/wait action is
not invoked. In-game acceptance remains pending.
