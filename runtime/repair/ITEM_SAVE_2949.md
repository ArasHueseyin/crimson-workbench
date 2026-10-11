# Item memory image and reconversion – native private sample

Proof of 0.19.0. The subsequent native dispatcher/queue check and static backend assignment are in [SAVE_BACKEND_2949](SAVE_BACKEND_2949.md).

Status: 23.09.2026, development module 0.19.0. Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **38 native scenarios existed:** Original functional copies only convert their own private items and storage objects. 14 Cases link the actual repair plan and native aftercopies to both converters. No access to game process or save files; no proof of the letter on data carriers.

## Alternative SQL results processing

The in [SQL_DISPATCH_2949](SQL_DISPATCH_2949.md) Methodology used `+0x30` belongs to the transaction, not to the FetchDropResult:

1. Repair `0x2be41b0` Acquisition over `0x2bba9d0 → 0x10515670` and
`0x2bbdde0` a `ItemPopInventory` from the context pool.
2. The design sets VTable `0x5b05818`; Your RTTI confirms the type.
3. `0x27b0950 → 0xff3c350` passes this transaction as an eighth argument
an `0x278a4a0 → 0xfeb27d0`. The fifth argument is the separate `FetchDropResult` to request `+0x6e8`.
4. At switch value 1, the result path calls the transaction method `+0x30`
to: `0x35ea90 → 0x873e140`. This complete 10 byte body only writes 0 into the result pointer and returns it.

This specific branch is therefore also not a storage jaw. The actual switch value and possible other transaction types are thus not determined.

## Item → ItemSaveData

`0x188ccc0`, length `0x709`, SHA-256 `584f5202a37f1bc2d08596bddb9c757bb21ff61c56a3f5909b6354daaf016da6`.

The receiver is a `ItemSaveData` (RTTI-VTable `0x58b8308`, size in the calling vector `0x118`). Source is the well-known `0xc8` byte item. ABI: `(save_object, item, slot_u16)`. Invalid/empty items go into the reset path. For valid items, the following takeovers are used:

|Wert|Itemquelle|Memory image|
|---|---|---|
|UID|`+0x0`, u64|`+0x30`, u64|
|Item identifier|internal key `+0x8` over `0x38ab60`|External identifier `+0x38`, u32|
|Slot|drittes Argument|`+0x3c`, u16|
|Menge|`+0x10`, i64|`+0x40`, i64|
|Main durability|`+0x40`, u16|`+0x60`, u16|
|Stored number of bases|Low-order byte of `+0x68`|`+0x78`, u8|
|Logical base boundary|`+0x70`, u8|`+0x79`, u8|

The base vector of the memory image is at `+0x80`. Its elements are `ItemSocketSaveData` (RTTI-VTable `0x58b7b28`), Stride `0x30`:

- Current durability `+0x28` gets the word `+0x2` of the 6-Byte-ItemSocket.
- External item identifier `+0x2c` arises from its internal key over `0x38ab60`.
- The conversion produces **five** base data sets. For positions outside
The logical boundary, i.e. In the stored number, it uses a global substitute data set. The native sample binds this as key `0xffff`, durability 0.

The `0x212cd60` inventory function calls this converter to `0x212d778` and `0x212dc7b` and takes over `0x118` byte storage objects. The two reflection setters for `_endurance` (`0x1886c10`, field `+0x60`) and `_currentEndurance` (`0x187ef50`, field `+0x28`) match this assignment. These fields had previously been found without secured object allocation.

## ItemSaveData → Item

`0x188c440`, length `0x879`, SHA-256 `b97e21b13592fe436d6c1165f5165faea44f5a6b15fe0141d8d53a4d05c096af`. ABI: `(save_object, destination_item, slot_out_u16)`.

The opposite path translates external identifiers via `0x38aac0`, takes over the stored main value and up to five base values from the `0x30` byte records. He first builds up initialization data, calls `0x240a040` and assigns the resulting native item copy via `0x240b020`.

The main value is **no unchecked byte-identicalreturn path**: stored 0 can first be set to 1 at finite maximum; This is followed by a signed comparison with the ItemInfo maximum. After construction, the no-wear sentinel is set at maximum `0xffff`; a negative i16 main value at finite maximum is replaced by this maximum. The native sample confirms 0 → 1, via maximum → maximum, stored `0xffff` at finite maximum → maximum and active no-wear maximum → `0xffff`. A prepared repair continues to use the verified finite vanilla maximum; Only the loader applies the no-wear rule.

The original initialization constructor materializes five base data sets and obtains the stored logical limit. Key and durability vote for all five records, even for free seats. The private policy switches at `0x6cf7248/+4` are at 0 in this sample. Their value in the game is not read. In particular, no general approval of an initialized number greater than the logical limit for the repair adapter has been proven.

## Native testing 0.19.0

Each 19 scenarios in both TLS allocation modes:

|Cases per mode|Test test|
|---|---|
|12 Direct conversions|Principal value 0/35/100/150/65535, active no-wear, 0/1/2/5 initialized socket, logical limit 0/5, free socket and socket value 0|
|6 Prepared repairs|Principal item and socket damaged, socket damaged only, No-Wear, principal/socket value 0, no initialized socket, socket at logic limit 1|
|1 Repair with additional data and reuse|Three coloring data sets, invalid item resets filled memory object; new item then replaces memory object and already filled load target instance|

UID, Item ID, Quantity, Enhancement, Slot, Main Value and all Base Values are checked. For the three dye data sets, all 13 meaning-bearing bytes are retained; their item/save strides differ (`0x10`/`0x38`). Sources and protective markings remain unchanged. The loader does not change the storage object or its base/dyeing vectors. Reset removes the valid identity and vector lengths; The reserved capacity shall be retained for reuse. No old sockets or colors get into the replacement item. Original destructors release all private allocations in a balanced manner.

24 additional full functional areas are pinned via SHA-256, including separate dust fragments and 15 additional TLS reading points. `0x188c50a` originally loads RCX instead of RAX; only exactly this tested instruction receives the corresponding private TLS redirection. The entire manifest contains 82 ranges, 108 pdata fragments and 50 TLS locations. code is executable and read-only; The original EXE is only read.

Sources: [Manifest](tests/save_functions.inl), [native Probe](tests/save_native.inl). Catalog/key translation, reflection keys/notifications, watch, guidelines, empty substitute data and allocator are their own test dependencies. Non-empty reflection metadata, optional item group/character conversion data and failed native allocation errors are not listed here. Language specific C++ clearup handlers of the game will not be registered; Checked stack wind data is retained. The test callbacks do not throw.

## Evidence and next examination

`.local/verify_save_route_v18.py` checks seven full function hashes, four RTTI types, the jump and the VTable method, and 15 instruction anchors. Historical result: `.local/repair-runtime-v18-save-route.json`, expressly `static_only=true`, `native_execution=false`.

Currently: passed nine CTest suites; Total **279 native scenarios, 3,950 calls and 139,784 conditions**. 38 of which are these conversion cases, 14 of which use prepared repairs. Result and validation: `.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`. `save_disk_commit`, `engine_commit` and `live_installable` remain explicitly false.

Next, track regular creation, serialization and completion of these objects, starting with the `0x212cd60` inventory caller. Further optional additional data and the actual baseline guidelines remain separate checks. This does not require reading existing saves.

### Static connection to the higher-order storage path

After the native sample, six more complete functions, three VTables, three jump targets and 14 instruction anchors were read-tested. This connection has been executed **not yet native**:

- Client/Common/ServerInventoryActorComponent use `0x212cd60` to VTable
`+0xd0`, the loader `0x212a880` to `+0x50` and the name getter `0x9c19a0` to `+0xd8`. The latter delivers exactly `inventory`.
- `0x20695f0` goes through a list of components (`+0x78`, number `+0x80`, stride 16),
queries the name via `+0xd8` and calls `+0xd0` with result pointer and own collection for storage objects. An error ends the further processing.
- `0x2069c73` calls `0x1296fa0` with the object collection and an output buffer.
Then `0x235af90 → 0xee28130` takes possession of it in a named entry. The serializer itself has not yet been semantically tested here.
- `0x23556b0 → 0xee09e40` calls the virtual slots at the backend (`Kontext +0x10`)
`+0x20`, `+0x48`, `+0x28`, `+0x30`. The respective fault diagnoses are called Load Fail, Valid Fail, Save Fail and Delete Fail. The specific backend type is missing.
- This dispatcher also has a branch of success without a backend call: set
Byte `+0x69c`/`+0x699` or deleted `+0x698` skips the sequence. Only after successfully completed backend calls does it set `+0x69b = 1` and `+0x698 = 0`. A mere true must therefore in turn not confirm a repair order.

Reproducible with `.local/verify_save_dispatch_v19.py`; Result `.local/repair-runtime-v19-save-dispatch.json`. Component ownership, concrete backend methods, scheduling and closing assignment are the next steps.

The conversion path found proves that both durability types are represented in the storage object. It does not yet prove when this object is created, written on data carriers or confirmed for a specific repair order. The later in-game validation R06 remains open: [TESTCHECKLISTE](../../TESTCHECKLISTE.md).
