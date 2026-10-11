# Own Repair Action – Integration Contract

Status: 22.09.2026, development module 0.10.0. The user has expressly released his own repair action for still existing, damaged items. The action requires no item repair rule and no repair material. Consumed/disappeared items are not generated. Direct save-write operations remain excluded; Later persistence must be done via the game.

## Developed scope

`repair_action.h` / `src/action.cpp` contain your own repair planning, a complete backend for **own memory images** and the command queue. The static library `crimson_repair_action.lib` is used in the isolated test host. She is not yet connected to the game.

- Individual items, items carried, items equipped or both.
- The main item and any socket insert available are independently attached to the
Repairs the respective occupied maximum, even with current durability 0.
- Identity, inventory position, positive amount and both must copy Zustand
fit. Empty slots, consumed items and inappropriate copies are rejected.
- Values without durability, true unlimited definitions and instance sentinels
remain intact. Finite maxima above 32,767 are not accepted because of the signed-16 bit engine arithmetic. This is not a proven overall limit.
- “No durability loss” can be combined: A verified original
Maximum and the known active 65,535 override allow the repair to the original maximum. An unexplained difference is rejected.
- Only the durability words `Item+0x40` and `Sockel+2` can change.
Pointer, set, ID, enchantment, base occupancy/number and flags are preserved.
- The entire batch is compared again before the first write access.
Changes to a later item also reject previous changes.
- Duplicate submissions shall be aggregated; each order shall not exceed:
one time. It expires after five seconds and must not survive a world/character change. Execution only on the specified dispatch thread.
- An unconfirmed commit permanently blocks the queue; No automatic
Repeat test. The host must create a new queue after a new, tested session has been set up.

The limits 2,048 Items and 64 Base slots are explicit processing budgets, not claimed engine/save capacities. An unknown layout is rejected.

Since 0.3.0, `crimson_repair_reader.lib` provides these images from the current client/server structure: targeted character identification search, owner verification, character inventory and equipment, UID/position matching and complete control reading. Native inventory getters match on common fixtures; The server registry is tested with artificial lock/access methods. This reading part is **not yet an engine lease or write transaction**. [Implemented Paths, Evidence and Boundaries](READER_LAYOUT.md).

Since 0.4.0 the transaction can report `pending`. The queue keeps the plan until the session-bound confirmation of all after values and engine events. Stranger/old feedback will be rejected. Partial error, world change and timeout block follow-up orders without repetition or resetting. A native client error path may have already removed sockets; Therefore, error != unchanged. [Implemented completion pathway and native findings](ACK_INTEGRATION.md).

Since 0.5.0, `capture_with_locks` collects the data under both owner locks bound by the host. These are released upon each return. Original lock methods are tested with real Windows SRW locks in the private test host. The host must continue to independently secure the lifetime of the owners and validate the method bindings. [Non-binding contract](LOCK_INTEGRATION.md).

Since 0.6.0 holds `capture_with_references` two native reference documents until after release of the inventory locks. Identifier and Alive/Destruction state are checked under the locks before and after detection. The host must still ensure the acquisition from protected sources and unchanged engine TLS mode. [Implemented Reference Management and Native Evidence](REFERENCE_INTEGRATION.md).

Since 0.7.0, `crimson_repair_registry.lib` connects manager lookup and document release as a source. Acquire/release use the same source context. [Contract and the former update limit](REGISTRY_INTEGRATION.md).

Since 0.8.0, the common native client/server search with specific reference methods and release exists on 1.0.0.2949. True Windows registry locks protect private test objects. Current manager/player/thread binding and other repair methods remain open; The old native host does not accept the new EXE. [Common Evidence and Borders](REGISTRY_NATIVE_INTEGRATION.md).

## Requirements for the missing game adapter

Since 0.9.0, the protected capture does not search a registry again, but reads directly from the owner pair. Valid full identifier and Alive/Destruction tests are also mandatory for `capture_with_locks`. Native acquisition/blocking methods of 2949 are tested together with the reader on private legacy layout fixtures. This is not proof of the new inventory layout. [Shared coverage and open hosting obligations](PINNED_CAPTURE_INTEGRATION.md).

The abstract `Transaction` contract is **not an implemented game adapter**. Since 0.10.0, the reading fields and native inventory/equipment accesses for 2949 are occupied separately; Reader views require an explicit known build. The real manager/thread binding and change transaction remains open. [Detection and range](INVENTORY_2949.md).

Its commit must perform all of the following tasks itself before the function becomes installable:

1. Connecting the developed read resolver with real engine access shares
and confirm his client/server assignment for actual game states. No NPC, dealer, warehouse or temporary inventory planner copies. world/character changes invalidate the session identifier and pending orders; the epoch number must be renewed by the host at each transition.
2. Complete EXE build, the called functions and current tables
check. The original item definitions must come from the vanilla stock approved above B0. The current local stock is not yet approved for this. No address transfer from an older community version.
3. Serialize access and changes in a busy engine transaction.
A C++-Mutex alone does not lock engine threads. The input/render thread is only allowed to set a job, not to make any inventory changes.
4. Check against freshly resolved item IDs and prevalues, both representations
correctly update and trigger the engine notifications/derived effects. A mere memory write or a later UI refresh is not enough.
5. `applied` Report only after confirmed completion. `rejected` does not promise any
Amendment. For unclear/partial result `unknown`; No speculative reset after a possibly already triggered engine event. Alternatively `pending` and then `Queue::confirm`: assign real messages with order number/session, collect fresh data under new engine lease and confirm all necessary events. This game event assignment is missing.
6. Connect your own operating input, activate loader only at the next game start
and process installation/return via B0 including conflict/update verification.

`apply_to_owned_snapshot()` must not be used as an alleged game adapter by reinterpreting external memory addresses. It mutates only its explicitly owned vectors/arrays and does not trigger engine events. The binaries in the private build folder do not belong in the game folder.

## Evidence in the current build

EXE 1.0.0.2944, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. All addresses as RVA:

|Function / Field|Evidence|
|---|---|
|Item-Updater `0x240d650`, 405 Bytes|SHA-256 `cdbf73177286a1465e808e462c5465c06756ab9910183eb09c2321ee5a195a3f`; Reads main value `+0x40`, definition `+0x400`, socket data `+0x60`, count `+0x68`, capacity `+0x70`, socket stride 6|
|Equipment-Wrapper `0x20c90e0`|Checks existing items; Subsequently, update further components via virtual methods. Table pointer `+0x90`, array `+8`, count `+0x10`, entry stride `0xd0`, slot tag `+0xc8`|
|Client callers `0x98fb70`|Processes durability delta and base list; further UI-/Zustandupdate depends on this|
|Alter Inventarreparaturpfad `0x2be41a0`|Material selection, transaction, `repairItemEndurance` database operation, then instancewrite at `0x2be47f5`|

The unchanged updater is **not a general RepairAll helper**: In the native test, Delta +65 repairs the main item from 35 to 100, leaves a 0/100 socket at 0 and sets a 10/100 socket to 0. The own action sets each value independently correctly to 100. After that, the original code with delta 0 unchanged values and with normal wear -1 confirms the value 99 for all three fields. With the well-known No-Wear-Sentinel, all three remain at 100. All calls are made only in the test host's own memory, with artificial definitions. This checks the field use, not the complete game transaction.

The MIT reference [Trinity, Commit 70c9a00](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b) provides indications of separate server/client copies. Your older layout values `0x88/0xc8/0xc0` match **not** to the proven equipment layout `0x90/0xd0/0xc8`. No code, hook or loader from it was adopted.

Testing and reproduction: [README](README.md).
