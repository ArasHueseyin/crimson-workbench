# Save dispatchers and pre-marked data sets – module 0.20.0

Status: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **39 passed new native scenarios on private objects.** No save files, Steam interfaces or running game objects used. Not an installable mod. The [Item/storage object converter](ITEM_SAVE_2949.md) remain tested separately.

Historical evidence 0.20.0. The subsequent native check of the filewriter is documented in [SAVE_FILE_2949.md](SAVE_FILE_2949.md); Real file operations and complete storage closure remain open.

## Native Functions and Ties

|Function|Original pathway|Test test|
|---|---|---|
|Storage dispatchers|`0x23556b0 → 0xee09e40`, complete body `0x2e3`|Call order, Zustandsbytes, skip, error and repetition|
|Steam Approval test|`0x235adb0 → 0xee1cd90`, `0x55`|Original function in the dispatcher, private platform queries|
|Pre-notification of an entry|`0x235b5d0 → 0xee2a210`, complete body `0x13f`|Three queues, growth, duplicates, both modes of allocation|

Six additional code ranges including the three jumps. The marker has five pdata fragments; the first boundary alone comprises only `0x2f` bytes and is **not a complete function**. Four unwind chains as well as the byte-identical alias `0x62bd504 → 0x171512d8` are tested. A new TLS position will be privately replaced. Total manifest: 88 code ranges, 115 pdata fragments and 51 private TLS locations.

## Conditions actually tested

The dispatcher receives context and storage state. Its first backend call receives the same memory state in RDX; This is required for the load branch. The further order is `+0x48` with context `+0x18`, then `+0x28` and `+0x30` with the memory state. The receivers for loading, writing, deleting and diagnosis are their own test callbacks; They do not access any files.

- **16 Gate/status cases:** all combinations of `+0x698`, `+0x699`, `+0x69c`
with previous success byte 0/1. `+0x69c != 0`, `+0x699 != 0` or `+0x698 == 0` delivers true without backend access. An old success value remains unchanged. Skipping work even tolerates a missing context in the test.
- **8 Error cases:** Abort in each of the four steps, with inline or external
Deposited diagnostic name. Only the prefix actually reached is executed. The dispatcher sets `+0x69b = 0`, receives dirty `+0x698` and reports exactly once.
- **8 native Steam checks:** successful quota query, available bytes > 0
and file number <10,000 required. 9,999/10,000, missing capacity, query errors and large limits are checked. The total quota is not additionally compared by the native helper against the available bytes. Platform data and API object are private; There is no Steam cloud query.
- **1 Mistake in missing backend:** active work causes the expected
private access errors; Original Stack-Unwind returns without success message.
- **6 Pre-note cases:** three queue types in both TLS modes. Each nine entries
force growth; The last address is intentionally a duplicate. The marker does not deduplicate and only modifies queue headers and dirty bytes. Entry objects and other fields remain unchanged. The fixture releases its queue buffer.

For four successfully completed steps, the dispatcher sets `+0x69b = 1` and deletes `+0x698`. Another call skips the now clean Zustand; The two success cases explicitly check this. This status change only confirms the sequence executed here with test receivers, no data carrier termination.

The three queue headers are at `+0x6a0`, `+0x6b0`, `+0x6c0`. The following file analysis assigns them to load, write and delete. The species chooses the native marker unchecked; The sample only uses 0/1/2.

## Separate static file proof

These functions were read **only**, not executed natively:

- Constructor `0x2354f50` sets a `GamePlaySaveDataFileHandler_Steam`
(RTTI-VTable `0x5984070`) in context `+0x10`. Its platform pointer `+0x148` is initially zero; Its later initialization and life span remain open.
- VTable `+0x20` is `0x2356ef0 → 0xee13560` (Load), `+0x28` is `0x2357180`
(Save), `+0x30` is `0x2357c10` (Delete); `+0x48` is the natively tested Steam helper. The basic VTable `0x5983f90` uses the same file methods.
- The save path calls for entries of the write queue `0x2358700`. This prepares
preset an output buffer, check Open (`0x12b7a90`) and Write (`0x12b7dd0`), then call the real import `FlushFileBuffers` (`IAT 0x51eb420`). Its return value is checked before Payload cleanup and success **not**. `CloseHandle` follows over `IAT 0x51eb480`; Also this return value does not determine the success here. None of these file/handle calls were executed.
- Timer types are assigned: `TrocTrFlushPendingSaveDataOnceTimer`
(`0x5b34358`, handler `0x2c35750 → 0x10665180`), `TrocTrSaveGameDataAutoTimer` (`0x5b34658`, `0x2c35e40`) and `TrocTrGamePlaySaveDataRepeatTimer` (`0x5b34fd0`, `0x2c3a4e0 → 0x10690e60`). This does not yet result in a permissible call time or thread binding for a repair job.

## Result and remaining work

Nine CTest suites and MSVC release `/W4 /WX` passed. Total **318 native scenarios, 4,039 calls and 141,411 conditions**. Unchanged production suites: 35 Writing scenarios/202 Conditions, 43 Transaction scenarios/273 Conditions. The native test reports `save_backend=fixture_callbacks`, `save_disk_commit=false`, `engine_commit=false`, `live_installable=false`.

Next, check the original file helper with private buffer/handle receivers, connect the serializer and the temporal processing and assign real final documents to the specific repair job. Server-side inventory messages, full effects/UI, game host and installation remain open. The later in-game validation R06 remains necessary; No simulated power outages or intentionally damaged saves are required by the user.

Sources: [native Probe](tests/save_dispatch_native.inl), [Manifest/unwind registration](tests/registry_native.cpp), [Wind test](src/pe.cpp). Reproducible artifacts: `.local/verify_save_backend_v20.py`, `.local/repair-runtime-v20-save-backend.json`, `.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.
