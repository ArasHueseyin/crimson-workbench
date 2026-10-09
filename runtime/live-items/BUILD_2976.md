# Native Live-Items candidate — build 1.0.0.2976

Status: automatic task execution implemented, tested outside the game and installed; real task observation, grant,
client UI and save/reload acceptance still pending. This is independent of the
file Live-Apply feature and does not relax its ownership or build checks.

The runtime admits only EXE SHA-256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`,
PE timestamp 1790086912 and image size 389722112. Function hashes and complete
sizes are in `src/runtime.cpp`. Game bytes are read only by the private native
fixture at test time, never committed to this project.

## Native path

| Operation | Current RVA | Evidence |
| --- | --- | --- |
| Automatic task wrapper (hook) | `2774b20` | Three arguments: error buffer, task object, context; original virtual task result is copied to the error buffer |
| Leased server task caller | `2774730` | Holds an acquired native actor receipt until after the wrapper returns; context[0] is the held owner |
| Leased decoded task caller | `2773c20` | Acquires the actor reference before invoking the wrapper and releases it afterward |
| Inventory dispatch | `2781780` | Thirteen-argument wrapper around `2781490`, with preflight, owner locking, commit, inventory events and persistence notifications |
| External item key lookup | `8802fb0` | Target of the `38aac0` thunk; outputs current internal u16 item type |
| Runtime item definition | `38ab60` | Realm-specific definition resolver |
| ItemVal quantity constructor | `2409970` | Complete 0x3ac-byte routine; current value size 0xc8 |
| ItemVal → complete InitData | `2449730` | Default construction plus `240cac0` serialization; output reaches 0x1b4 bytes, allocation is aligned 0x1c0 |
| ItemVal destructor | `f493f00` | Target of `240af60`; current socket destructor `3858c0` |
| Native grant transaction | `2b3ee50` | Used by the server create-item handler; obtains user reference, constructs native requests, acquires TLS context and calls `2781780` |

`22ccd40`/`ea1b360` resets new request ItemVal UIDs to -1 as part of the native
grant path. The runtime leaves allocation, placement, stack splitting,
notifications and persistence updates to the game. It never manually allocates
server UIDs or writes client/server slot mirrors.

The task hook executes the original exactly once before considering a request.
Eligibility requires a return address from one of the two fully pinned callers,
a successful original result, a held `context[0]` owner, TLS realm
1, an existing TLS+0x250 context pool, the pinned ServerChildOnlyInGameActor
vtable, a live SelfPlayer tag and owner/component/holder/possessor round-trips.
Its full generation-bearing handle must match the freshly read client selector.
The owner is
borrowed only until dispatch returns. There is no cached actor pointer,
cross-thread invocation, heap scan, realm switch or movement hook.

The native conversion has five inline socket records. Runtime definitions and
constructed values exceeding those bounds are refused before grant. A new
ItemVal must serialize UID -1, the requested external key and quantity. Before
and after grant, the holder's bounded bucket walk sums quantities for that type;
success requires native error zero and exactly the requested increase. Failure
after entering the transaction is uncertain and latches further requests off.
This check does not prove client acknowledgement or completed save-file output.

## Transport and execution

Pipe: `\\.\pipe\CrimsonWorkbench.Items.v2.<game PID>`, protocol version 2, protected with the current
user's SID and remote-client rejection. Workbench verifies the pipe-server PID
and process executable path. Requests are fixed 40-byte messages; replies are
fixed 48-byte messages followed by one client ACK byte. Read/write and ACK waits
are bounded. No command strings, paths, native pointers or scripts are accepted.

One queued/executing grant is allowed. Queued grants expire in 30 seconds and can
be cancelled. UUIDs and their results are retained until restart (max 512 without
eviction). Changing key/quantity under the same ID is refused. A boot epoch in
each request prevents replay into a restarted process, including reused PIDs.
Open IDs persist in Workbench local storage before transmission; uncertain I/O
causes receipt queries, never automatic re-grants.

Requests now run after a matching character task, independently of item pickup.
No raw character pointer crosses the queue. Requests bind to the observed full
character handle; a changed/missing client selection rejects queued work. Pure
pipe-side reads can keep the same previously confirmed selection ready during
pause/Alt-Tab, but cannot admit a new owner or perform a grant. Without that
confirmation, a context gap exceeding five seconds returns readiness to starting
and invalidates queued work when the next valid task arrives. Thirty-second
expiry still applies. Execution always requires the native lease and server TLS
checks again. This is a task callback, not a fabricated server thread or TLS pool.

## Tests

- C++: all 13 original arguments, return and error preserved; concurrent single
  dequeue; replay, payload tampering, wrong boot epoch, cancellation, expiry,
  quantity/socket limits, cleanup and post-grant delta.
- Actual build code in an own-process fixture: twelve constructor, complete
  conversion and destructor cases with quantities 1/1000 and 0–5 sockets,
  endurance, UID and allocation guards. Definition/allocator dependencies are
  fixtures. The game process is not opened and its EXE entry point never runs.
- Actual task dispatcher/wrapper plus MinHook in that same private arena:
  original ABI/result/error, native receipt lifetime through the callback,
  automatic dequeue/grant, nested task forwarding, replay, wrong owner/realm,
  missing context pool, context gaps, pause selection and fault latch. Lookup,
  reference release and inventory mutation dependencies are fixtures. This does
  not establish real in-game task cadence, synchronization or persistence.
- Rust/C++ own-process real Windows pipe: status, replay, receipt, cancellation
  and wrong-epoch refusal. The game is not a test target.
- Loader: existing winmm loader and six ASIs load together; an unsupported EXE
  is refused by the new runtime before hook installation.
- UI: ready-state gating, quantity validation, single submission, cancellation
  and pending-ID recovery after a lost reply and window reload.

Required real acceptance: one ordinary material, inventory UI, save and reload;
then stack limits, full inventory, character changes and other item types.
Installed on 2026-10-03 with both the game and Workbench closed, per the user's
standing request. The guarded installer verified 33 existing game/save files
unchanged and backed up 19 files. The desktop shortcut now targets the tested
`target/release/crimson-workbench-live.exe`. Actual in-game acceptance remains
pending. The automatic revision then replaced the ASI and that same desktop EXE
with both applications closed. Thirty-six existing files (including all thirteen
current save files and the shortcut) stayed unchanged; twenty-three fresh backups
were verified. Evidence: `.local/live-items-autopump-20261003/`. The main game EXE
was exclusively locked throughout the replacements/readbacks. Older versions
are retained in `auto-install-originals/replaced-0.bin` and `replaced-1.bin`.
