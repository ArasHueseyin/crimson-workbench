# SQL job path: no memory confirmation (0.18.0)

Status: 23.09.2026, Steam 25455892 / EXE 1.0.0.2949. All executions are carried out on private data in the own test program. The installed EXE is only read. No access to game process or saves, no installation change. The desktop app remains v0.5.9; Free repair remains blocked.

## Results

**The function previously known as SQL execution does not execute a database task itself.** Your complete functional body is now bound and tested in isolation. When the switch is set, it delivers 0 without reading the job at all. A zero pointer is also accepted in this branch.

The equipment slot consumer can use it to empty his markings and get a successful status even though no storage has taken place. An empty change inventory, a successful client corner and the return 0 are each no proof of persistence. This specifies the previous designation "Persistence Interface" in [0.16.0](PERSISTENCE_2949.md).

The production release remains unchanged: `engine_commit=false` and `live_installable=false`. In this step, no game connection was added.

## Original pinned functions

EXE-SHA-256: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.

|Function|RVA|Longitude|SHA-256|
|---|---|---|---|
|Total execution shim|`0x2776d00`|`0xc8`|`9cf1709ef75ad14106cdcd8e9f66fb55260612696421e28117e00f770b1ad5e3`|
|Acquisition of the common request pointer|`0x2c1da50`|`0x3f`|`6e744fe8978f3b0ae40d289c92e21a39779228870ac5ce901802e20ed8ba422d`|

The shim comprises five pdata fragments: `0x2776d00/0x37`, `0x2776d37/0x27`, `0x2776d5e/0x41`, `0x2776d9f/0x1e`, `0x2776dbd/0xb`. The concatenated wind data are checked. Examining only the first fragment would be incomplete.

### Execution shim

- Byte to RVA `0x6ceee08` not equal to 0: immediate return 0; no test,
no change to the request and in particular no deletion of an old error.
- Byte equals 0: Request `+0x50` is set to 0 and `+0x48` to 4.
The backend pointer comes from `+0x10`. Interval 0 to backend `+0x34` leads immediately to return 1.
- At interval > 0 the clock is called up and backend `+0x38` to
Current time plus interval set. If the previous deadline has expired or exactly reached and the switch continues 0, a diagnosis call follows. For this purpose, Request `+0x28` is temporarily switched to Request `+0xb0`, then reset and the status is emptied again. The return is 1.
- There is no other call in the whole body that
Database or save writing path implemented. The clock and diagnostic receiver are their own controlled functions in the test.

The actual switch value in the current game was not read. No active runtime mode is derived from the file investigation alone.

### Request acquisition

`0x2c1da50` builds a 16 byte wrapper around the pointer from context `+0x88`. There is neither a new independent request nor a reset error state. Repeated acquisition provides the same pointer; Zero also remains zero. The getter leaves R8 unchanged. This explains the register use of the previously examined socket helper, but does not replace a durability memory proof.

## Tests carried out

19 new native scenarios in [sql_native.inl](tests/sql_native.inl):

- Eight shim cases: switch 1, switch 255 with zero request, interval 0,
future/exact/past deadline, switch changed during the clock call and zero-jacketing. Entire request/backend images, diagnostic sequence and unwind with access exception are compared.
- Three cases of acquisition: valid, missing and repeatedly acquired request.
Old error -77 and the protection bytes behind the wrapper remain.
- Eight integrated consumer cases: four situations in both private
TLS allocation modes. Original purchase and complete shim replace the corresponding callbacks. Switch branch with status 0 is accepted; Return 1 and old errors -7 and +7 result in the expected error codes. UID 22 and main durability 100 are prepared correctly; the original list is cleaned up. References, blocks and allocations are balanced.

Nine CTest suites and MSVC release `/W4 /WX` passed. Total **241 native scenarios, 3,674 calls and 128,415 conditions**; Many conditions repeatedly compare tables. Manifest: 58 code ranges, 76 pdata fragments, 35 private TLS locations. Production suites unchanged: 43 Transaction scenarios/273 Conditions and 35 Writing scenarios/202 Conditions.

Capacity management, diagnostic/defect receivers and previous UI/transport dependencies remain partly test functions. The two new original bodies themselves do not receive a TLS replacement. No test result confirms real storage.

## Next required proof

The downstream result path `0x278a4a0` jumps to `0xfeb27d0` and calls up the virtual method `+0x30` of the transaction at switch value 1. This is his eighth argument; the FetchDropResult also passed to Request `+0x6e8` is the fifth argument and not the virtual receiver of this branch. The now assigned method of the `ItemPopInventory` also only writes a success status. A separate conversion path between Item and `ItemSaveData` takes over both durability types; So far, it has only been statically occupied. [Assignment and next native examination](ITEM_SAVE_2949.md). The regular storage/charging path must be followed further. The socket helper alone still transmits only an ItemInfo identifier, no proven durability value.

Server-side inventory messages, full events/UI, feedback assignment, host with valid player/manager/thread binding, input and loader/B0 are still missing. The later in-game validation R06 requires a comparison of the main item and each socket after saving, reloading and complete restart. [Testcheckliste](../../TESTCHECKLISTE.md), [Phasenstatus](../../PHASENSTATUS.md).
