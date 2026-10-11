# Repair: Confirmation and notification 0.4.0

Status: 22.09.2026. The action control now supports an asynchronous confirmation. It represents **not an installed game connection**. The native client feedback path is only examined in its own test host.

## Developed termination path

`Transaction::commit` can also return `pending`. `Queue::execute` then reports `awaiting_confirmation` without counting a repaired item. The plan shall be maintained until confirmation. Re-entering, cancellation and a second execution are blocked during this time; There is no repetition. The engine host must not hold a lock over this waiting period.

`Queue::confirm` requires order number, original feedback session, a freshly captured session with item images, and the explicit confirmation of all required engine events. The session includes world generation, character and catalogue revision. An external/old response cannot complete a new order even with the same numeric order number.

`verify_applied` checks all planned post values of both representations including IDs, positions, sets, sockets and other item bytes. The order of the items in the detection array is insignificant. Only matching data **and** confirmed notifications lead to `applied` and a success amount count. The test only reads its own images and does not write any repair values.

Error, partial change, missing item, changed session/definition or a five-second confirmation period will result in `outcome_unknown` after acceptance. The queue is permanently locked. No speculative reset and no automatic repetition attempt. A later signal of success does not lift the lock. The host must recapture the actual Zustand and start a new verified session before creating a new queue.

`Queue::poll` must be accessed on every dispatch tick, even when loading or in menus. Before the start of a transaction, an old/expired order can be discarded without side effects. After acceptance, a world change or timeout is treated as an unclear outcome. Confirmation and polling may only run on the specified thread. During `commit` incoming feedback, the host must reset until after returning from `execute`.

Synchrones `Commit::applied` retains its previous contract: The adapter must have already confirmed all changes and events. The new `pending` is not a success and is not a release to issue a parcel shipment as a completed repair. The assignment of real game events to these orders is still missing.

## Findings from pinned EXE

Steam 25381195 / EXE 1.0.0.2944, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. All addresses are RVAs; Only files on the data carrier were read.

|Path|Finding|
|---|---|
|Common Equipment Wrapper `0x20c90e0`|Lock area, updater, item copy and virtual slotacticization; After that, release and event method with Item, Slottag, Delta, previous BrokenZustand and List of Removed Bases.|
|Server-VTable `0x5b28f80`, Slot `+0x190` → `0x2adfd90`|Treats broken/repaired transitions and other effects, processes remote sockets and calls the `0x29ead60` message path. Only statically examined, not fully emulated or tied.|
|Server-VTable Slot `+0x198` → `0x2ae0640`|Gives the slot tag to the path at `0x20cd610`. This alone is not a complete repair notification.|
|Client-Ack `0x98fb70`|Apply the delta first, then remove used sockets and only then compare the list with the server message. UI calls follow successful comparison.|
|Base function `0x240e3e0`|**No pure reader:** Collect existing finite bases with durability <= 0, replace them in the item with empty entries and get their slot index.|
|Lock constructor `0x393650`|Reference count, optional flags and virtual lock calls. In the test original constructor logic, but artificial lock implementation. No proof of an exclusive engine transaction.|

The earlier rough description of a “socket list” did not mean that this function is read-only. An unthinking call of the wear/rear path can change the base occupation even before a feedback error. Therefore, an error code cannot be treated as a “no change” promise after the call. An existing damaged insert must be treated before such paths via a confirmed repair path; An already remote deployment is not restored.

## Native sample and its limits

|Function|Size size|SHA-256 of the original|
|---|---:|---|
|Client-Ack `0x98fb70`|773|`d8221b3537464395a14ebbaf63b2bf124d15bbe0167c05e9a585f7338eb87aa1`|
|Lock constructor `0x393650`|104|`9361a1820340878d8bc677ec8cd21654fbab72c61102b5529939f23533e18e2f`|
|Base Distance `0x240e3e0`|465, three unwind fragments|`88087dcf346127a076e9c715e91d78e3ce5b7ef5e8a10e3ca6a71a9afc8f759b`|

The client copy contains exactly one test adjustment: The nine-byte TLS read access at `0x98fe2a` is redirected to artificial TLS data in its own arena memory. The existing TLS/TEB of the test process is neither converted nor interpreted as a game TLS. All other Ack bytes are unchanged and compared individually. The original socket function and the original lock constructor remain unchanged. The three base-unwind fragments together with the tested prologue chain are registered.

Item definitions, UI recipients, list attachments, sharing and virtual lock methods are test stubs. The error number 902 is an artificial constant on the original error path. Native C++ cleanup handlers for these foreign functions are not registered in the test host; the stubs used do not throw exceptions. No engine message was actually sent, no persistence triggered.

Six additional native cases:

1. Positive delta repairs the main item, consumes the bases and guides
then due to a different distance list for the error. The partial change remains visible and is rejected by the post-downgrade test.
2. Already correctly repaired private images plus delta 0 are preserved;
empty distance list fits, UI callback takes place, after values fit.
3. The same correct images with an incorrect distance list lead to an error;
Appropriate fields alone are therefore not sufficient to confirm success.
4. Normal wear -1 delivers success and UI callback, but fulfills the
No repair plan. Even error code 0 alone is not sufficient as proof.
5. Existing bases with durability 0 are removed; A suitable
Server list leads to two socket and one panel callback. No repair success.
6. A missing equipment slot can cause error code 0 in the tested debug flag path
Deliver without repair. The after-down-value test prevents a false success.

Passed a total of 122 native calls with 2,221 test conditions, 1,481 action conditions and 124 reader conditions; three CTest suites and MSVC `/W4 /WX` green. Reproduction: [README](README.md). The game connection remains technically open: real engine access releases, complete change/event path for inventory and equipment, operator input, loader and B0. `installable` remains `false`.
