# Repair 0.13.0 – Slot marking and held capture

historical stage of development; supplemented by the [Field recorder 0.14.0](FIELD_WRITER_2949.md).

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE hash: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **Development module; still no installed repair.** App v0.5.9 remains unchanged. The repair option is still blocked.

## Implementation

`reader::HeldCapture` keeps both native owner references and exclusive owner locks after capture. Planning and re-examination can thus take place within the same uninterrupted blocking phase. The previous short `capture_with_references` calls use the same path, but as before, they release everything before returning.

`refresh` requires the current frame independently observed by the host. Other world generation, catalogue revision, image base or build are rejected. Each refresh checks again selection, complete identifier, living owners, lock ties and the complete item image together with control reading. Errors delete old results and close the ownership phase. There is no second unprotected registry search. `release_locks` ends the reading phase finally, but keeps the references until `close`/Destruction for subsequent notifications. Re-acquisition and use on another thread are excluded.

This is a synchronous capture with held owners, **not yet a native scribe**. The host must keep the manager, context and catalog valid, exclude world transitions and run on the confirmed engine thread with matching TLS. The class does not recognize such transitions itself and must not span any frames. A prepared plan will continue to be fully verified against the fresh data prior to application; This is tested on the existing private backend.

## New native evidence

The server equipment VTable `0x5b28f80`, entry `+0x198`, refers to `0x2ae0650`. This function enters the slot into the table of component `+0x1e8`. The complete path, including collisions, shifted buckets and rebuilding of the table, now runs on private components in the own test process.

|Function|RVA|Complete code area|
|---|---|---|
|Slot marking|`0x2ae0650`|`0x32` Bytes|
|Insert jump|`0x20cd620`|`0x5` Bytes|
|Insert|`0xe1db910`|`0x20c` Bytes, three pdata fragments|
|Place search / collision displacement|`0x20cd9e0`|`0x287` Bytes, three pdata fragments|
|Enlarge / Rehash|`0x20cdc70`|`0x4be` Bytes|
|Bucket linkage|`0x411bc0`|`0x77` Bytes|

All ranges have their own SHA-256 values in [Manifest](tests/dirty_functions.inl). The place search is completely copied, including its two rear fragments. The byte-identical Unwind alias `0x6106998` → `0x17147c54` is only normalized for the exactly tested insert parent chain. The original TLS reading locations `0xe1dba5f` and `0x20cdcb6` are redirected exclusively in the private functional copies.

The test heap now supports the original 32 byte orientation of the nodes. Protective markings, unique possession of the payloads, balanced releases, slot/series values and all forward/reverse chains are checked. Copy buffers and slot payloads remain separate allocations.

## Tests carried out

- Seven CTest suites, MSVC release with `/W4 /WX` passed.
- 519 reader/reference conditions, including new cases for held capture,
Reacquisition, wrong thread, error/exception release and Zustandchange. An outdated batch with a later changed item does not change the previous items in the private backend.
- 16 new native slot cases: empty, single entry, duplicates, moved
Collision groups, 64 entries ascending/descending, strongly colliding and high-bit slot values, both TLS allocation paths in each case. High-bit values are a hash/ABI test, not a release of such equipment slots.
- All 15 existing event cases now mark the modified slot under
Both owner locks. The original server notification then runs on the same private component with the native aftercopy. The consignees check that the entry is present, the blocks are clear and the references are still held.
- A competing thread cannot return to `HeldCapture::open`
of the two real Windows owner locks. Refresh does not require re-blocking/reference acquisition; Unlock keeps the actor references.

Total **133 native scenarios, 1,296 counted calls and 119,997 conditions**. The larger condition counter is produced mainly by re-checking all table entries after each insertion. It does not correspond to this number of independent scenarios. Manifest: 51 code ranges, 57 pdata fragments and 26 private TLS readers; without EXE only 306 manifest conditions.

## Continued open integration

The slot marking alone confirms neither UI update nor persistence. The subsequent consumer of this list and its native cleanup are not yet connected; The sample itself cleans up its private allocations. Engine Allocator and its error paths, effect processing, transport, UI and parts of the child object management remain test dependencies. The sample does not cause any allocation errors in a foreign code.

There is no complete write/event path for ** and ** equipped items, clear real feedback, player/manager/thread/world binding, input, loader and B0 installation. `HeldCapture` is not such release. No execution in the game process, no save accesses, no game installation described. Rust-/Frontendcode has not been changed or re-tested.

Evidence: `.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`. Predecessor: [Item copies 0.12.0](ITEM_LIFECYCLE_2949.md). Later manual acceptance: [TESTCHECKLISTE](../../TESTCHECKLISTE.md), in particular R02, R05, R06, R10 and R12; Only after full implementation.
