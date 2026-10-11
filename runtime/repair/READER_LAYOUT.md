# Reparatur: lesender Spieladapter 0.3.0

Current addition 0.9.0: The protected path captures only held owners and does not search the registry again. His mandatory exams and the common native sample are in [PINNED_CAPTURE_INTEGRATION](PINNED_CAPTURE_INTEGRATION.md). The following addresses continue to describe the historical build 2944. Current 2949 addresses and native comparison tests: [INVENTORY_2949](INVENTORY_2949.md).

Status: 21.09.2026, Steam 25381195 / EXE 1.0.0.2944. A separate reader for the current data structure is implemented. It delivers separate, owned item/socket images to repair planning. **No game transaction, no loader and no access to the current game.**

## Recording

`repair_reader.h`, `src/reader.cpp` and `src/reader_win.cpp` form the static library `crimson_repair_reader.lib`. `LocalMemory` only reads the calling process, checks side boundaries and intercepts errors of its own read operation. There is no PID selection, no process handle, and no write interface. The tests only use their own process/fixture memory image.

The client anchor leads to the currently selected character. The reader searches for its identifier specifically in the server registry; He does not search the heap or all the characters. The full identifier must match. Type testing, possessor reference and component ownership are additional conservative tests. A match of item contents is not sufficient as a player identity.

Only the character container and equipped items are recorded. Other containers are skipped by their type without reading their item data. Empty slots, quantities <= 0 and the exclusion list of the inventory getter are taken into account. Inventory slots are contiguous `0xc8` byte entries; Equipment entries include `0xd0` bytes including slot tag. base data shall be copied separately; saved pointers are never dereferenced by the action.

UID, position and durability/socket data shall fit together in both representations. Duplicate IDs/slots, unreadable areas, unknown definitions, inappropriate geometry or lack of game recognition discard the entire output. All read areas are read and compared again before publication. **This comparison does not replace any engine lock** and in particular does not detect any unnoticed A→B→A changes between the reading times.

Budgets: 128 inventory containers, 64 equipment entries, together 2,048 existing items per representation, 64 base slots per item, 65,536 registry buckets/indices, 31 entries per registry bucket, total 16 MiB and 65,536 read operations including control read. These are local processing limits, not claimed maximum game/save capacities.

## Static proof in the current build

Full EXE-SHA-256: `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. All code / global addresses are RVAs. The source EXE was only read.

|Path|Finding and limitation|
|---|---|
|Clientglobal `0x6d691b0` → `+0x30` → Manager `+0x50`|Static client context; Construction at `0xacf5f0`, access and a. above `0x8b4480`. The adjacent global address `0x6d691b8` is not an interchangeable server anchor.|
|Serverglobal `0x6d696c0` → `+0x48`|Server-side equipment caller `0x2a94c40` uses this manager for `0x2a82730`.|
|Server-Registry `0x2a82730`|Handling test, key `handle & 0xfffff`, bucket selection by modulo, 256-Byte buckets; Key/Index off `+8`, Node key `+4`, character pointer `+8`Acquires real engine accesses; represented in the test only by stubs.|
|Actor `+0x60`, `+0x68`, `+0xa0`|Full identifier, components and possessor; Possessor `+0xd0` must reject. Components: inventory `+0xb8`, equipment `+0x38`. Ownance examination additionally via component `+8`.|
|Inventargetter `0x212f4a0`|Holder `+0x18` Pointer array, `+0x20` Number; Bucket `+0x10` type, `+0x0c` signed slot limit, `+0` contiguous items. Exclusion list `+0x20/+0x28`, 12 byte records with itemkey at the beginning.|
|Holdergetter `0x212a0f0`|Actor/Possessor/Component path; for each character definition directly or via owned actor. Native tests check direct, diverted and missing owners.|
|Equipment-Wrapper `0x20c90e0`|Component `+0x90` → Table `+8/+0x10`; Stride `0xd0`, Slottag `+0xc8`.|

The selection of type tags 1/4/9 takes into account clues from the MIT reference [Trinity, Commit 70c9a00](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b) to alternative protagonists. The current type getter has been executed in isolation; Day alone does not identify an active player. Real change of character and the admissibility of the additional possession tests in all game states have not yet been proven. No foreign resolver/hook code has been adopted.

## Comparative native sample

|Function|Longitude|SHA-256|
|---|---:|---|
|Inventargetter `0x212f4a0`|226|`959437d3eff481d79aa0d6e9bb4ead3bc54ab2f408d80cddcf689c3b33506c9d`|
|Holdergetter `0x212a0f0`|127|`e4b46826485c932d72fb7c175b0e4c3943c8dee8e6cd01a5378ab170f17162c5`|
|Registrylookup `0x2a82730`|542|`df6ba99ef13614b5a8b8cbcd7c200127a176788905134ff9b445a91c6cf43c68`|
|Actor Type Getter with jump table `0x466060`|88|`5f31dba7c7e552dc6983e9288297f246d87d5fb9bfb90506e905b121ecc916a8`|

The unchanged inventory function and the own reader receive the same artificial containers. Valid, negative/too large, empty, used and filtered out slots give identical results after joint presence normalization. The native getter can still deliver its slot pointer for an empty item; the calling path must check the key and the set. The own reader already returns address 0 for such entries and rejects additional ambiguities.

The inventory getter consists of four `pdata` fragments. The test host registers all four and receives the checked `UNW_FLAG_CHAININFO` references to the first prologue. A deliberately inaccessible slot in the own test process also confirms the exception return through this chain. No fault was triggered in the game.

Twelve native registry cases check hash/index access, invalid handles, blank/false entries, type rejection, access rejection, and balanced lock calls. **The lock and actor access methods are non-throwing test stubs.** The test proves their called ABI, not the implementation of a real engine lease. The result can contain an actor pointer despite an invalid lease: The validity byte `+0x10` must be evaluated. The test host does not call methods of the VTable addresses written to the output by the game code.

## Supplement 0.6.0

`capture_with_references` has both native references during capture and returns them only after the inventory locks. Under the locks, it additionally checks full identifier as well as Alive/Destruction state. Protected sources of purchase and engine TLS must still be connected by the game host. [Reference contract and current audit](REFERENCE_INTEGRATION.md).

## Remaining integration

Add 0.5.0: `capture_with_locks` keeps both host-bound owner locks during capture. Original lock methods are tested separately with real Windows SRW locks on private test objects. The lifetime protection of the owners remains a prerequisite of the host and is not yet integrated. During the rebound, the locks are free. [Current Non-Proliferation Agreement](LOCK_INTEGRATION.md). The registry samples described above retain their test stubs.

Before use in the game, the reader needs a host that verifies the full build and the original/active tables, holds real engine leases, and delivers a new epoch number every time the load/world changes. `Frame` contains these host values; the reader does not invent them and does not check file hashes himself. The matching of the full client/server character recognition and the item IDs is conservatively required, but still unproven for all real transitions.

After that, the confirmed engine commit is still missing: change durability, update inventory/equipment events and derived effects, confirm result and leave persistence to the game. A comparison reading or a duplicate raw storage writing does not fulfill this contract. Operating input, loader and B0 installation are also still open; `installable` remains `false`.

Historical status 0.3.0: 124 Reader checks, 1,377 Action checks and total 116 native calls with 1,387 check conditions passed. Reproduction: [README](README.md). Full Commitment Contract: [ACTION_INTEGRATION.md](ACTION_INTEGRATION.md).
