# Contribution balance editing

Scope: game executable **1.0.0.2976**, SHA-256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
All investigation below reads original archives, the executable, or an existing
private historical save fixture. It does not modify a live save or installation.

## What the requested values represent

The six supported balances are native contribution currencies in the **Money**
inventory. They are not physical backpack items. Contribution experience and
sublevel progression are separate systems and are not changed by this feature.

| Display name | Item key | Native definition |
| --- | ---: | --- |
| Graymane | 1000658 | `Contribution_Graymane` |
| Demeniss | 1001753 | `Contribution_Demenissian` |
| Hernand | 1001754 | `Contribution_Hernandian` |
| Pailune | 1001755 | `Contribution_Pailunese` |
| Delesyia | 1001759 | `Contribution_Delesyian` |
| Tashkalp | 1003749 | `Contribution_Tashkalpan` |

Each original ItemInfo record has type 32, money denomination key 1 with minimum
1, and `max_stack_count = 9000000000000000000`. Its configured inventory entries
resolve to inventory key 1. The original Inventory table identifies key 1 as
`Money`. This is independent evidence for the namespace; matching names alone
would not establish where a balance belongs.

The exposed target range is **1–9000000000000000000**, inclusive, and a target
must be at least the current balance. All UI/API amounts use exact decimal
**strings**. For example, `9007199254740993` must not travel through a JavaScript
number. Zero, negative values, leading zeros, exponent notation, fractions,
overflow, unknown types and duplicate selections are refused.

The native static maximum is below signed 64-bit maximum. The native balance
consumer treats a nonpositive signed stack count as absent. This is a reason to
reject the unsigned 64-bit maximum rather than accepting every eight-byte value.
The maximum has not been tested in the live game and is not a claim that every
game UI formats extremely large balances attractively.

## Native UI corroboration

All addresses are relative to the pinned executable image base.

1. The string `contribution-item-key` is at RVA `5661890`. Initialization at
   `14cb40` stores its hashed property identifier at `6ccfc68`.
2. UI routine `d6ce00` loads that property at `d6ce3d` and passes it to
   `d6d360` at `d6ce5f`. The latter parses configured native ItemKeys, resolves
   their ItemInfo handles at `d6d65c`, and asks `21361b0` for the amount at
   `d6d685`.
3. `21361b0` selects the item inventory through ItemInfo offset `428` and finds
   the corresponding inventory object before calling `2408990` at `2136290`.
4. `2408990` iterates inventory slots of stride `c8`, resolves ItemInfo handle
   `slot+8`, compares the requested definition key, and sums positive signed
   counts from `slot+10` at `2408a49`. Thus the contribution UI reads an actual
   wallet stack amount, rather than an unrelated cosmetic display scalar.

The final routine spans more than one exception-directory range. Its complete
observed body is `2408990..2408a7f`; stopping at the first range end `24089c2`
would miss the amount lookup and summation.

Useful function hashes:

| Native span | SHA-256 |
| --- | --- |
| `d6ce00..d6d177` | `ecee9eeb101ea5695661a6a10f376d7e85f47efdfe2dffecf6aacedbda098e74` |
| `d6d360..d6d9ad` | `7c0e72bb59a55b6882225a6095ff1578adb3005bdf0de027a4714f0768514c44` |
| `21361b0..21362b7` | `4327b70e2e30b1ca2836cf20bbd86846a5b71d7c768a0853640f71df5bafe07e` |
| `2408990..2408a7f` | `2b1f39632639294d4160eb91a5e19f5c3c5c2e5e43fabef5c91b7f53faa9e0c8` |

No native hooks, functions or processes are invoked by the editor.

## Save representation and exact edit boundary

The embedded save schema identifies the relevant path as:

```text
InventorySaveData
  _inventorylist: InventoryElementSaveData[]
    _inventoryKey: InventoryKey = 1
    _itemList: ItemSaveData[]
      _itemKey: ItemKey = one of the six supported keys
      _stackCount: TStackCount (8 bytes, little endian)
```

The historical fixture contains exactly one existing wallet object for every
supported key. Each has `_saveVersion = 1`, a valid existing item number and slot,
and `_transferredItemKey` equal to its own key. These existing IDs, slots,
timestamps, sockets, marks, ancillary data and all unrelated save records are
preserved. No physical item is spawned, cloned, transferred or assigned a new UID.

The byte-only helper in `crates/cd-core/src/contribution_values.rs`:

- Authenticates both files and strictly decodes the save. Unknown ranges and
  incomplete or nonidentical roundtrips are refused.
- Requires a unique InventorySaveData block and unique money container. Duplicate
  wallet IDs, slots or contribution objects are refused.
- Resolves fields by embedded schema names and verifies scalar types, widths,
  kinds, metadata and raw values. It never searches for a bare item ID and edits
  adjacent bytes.
- Returns all six balance choices. A missing existing contribution object is
  represented by `amount: null` and cannot be edited. Missing entries are not
  silently created or replaced with unrelated items.
- Binds a review ID to both original file hashes and the sorted exact targets.
  Changed saves, changed lobby files or changed target selections require a new
  preview.
- Patches only the selected eight-byte stack counters in the decompressed body,
  reauthenticates/reencodes a private candidate, then decodes it again. Restoring
  just the old selected counters must reproduce the entire original body exactly.
- Returns the original encrypted file bytes for an unchanged target selection.
  The helper itself never writes files.

The application service verifies the supported build and selected save path,
refuses a running game and uses the existing locked, backed-up save-pair
transaction. The English Marketplace card provides a save selector, current
balances, independent exact target fields, review and explicit application.
Targets must keep or increase the current amount. Moving between saves or
changing a target revokes the previous review. Missing entries stay disabled.

The durable request binds both original hashes, the target selection, save root
and slot. A matching completed request returns its receipt without reapplying.
Interrupted receipt publication is recovered; an uncommitted transaction rolls
back only its known generations. Foreign/stale saves are refused. Lobby bytes
remain exact throughout. No real save or installed game has been written.

## Evidence and verification

Original table hashes:

| Table | SHA-256 |
| --- | --- |
| ItemInfo body | `87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce` |
| Inventory body | `ee841f4ba922538b32eccb9665f269861099502c66384ce1745503bbce70aaa6` |
| Inventory header | `9fab547265374c3052be71f1c1cf2da80af641e338f0ab80cfe0778f45b70d32` |
| Money inventory row | `336202252f8d74eaaeffadadaa0c20555a993a334d924b86c9d27120842c3a3d` |

The full 6,816-record original ItemInfo body passed a byte-identical parser and
serializer roundtrip. The private historical save has a valid HMAC, a complete
embedded schema and no recursively undecoded ranges. Its six known wallet
objects are present exactly once. Private data and binary extracts remain under
the ignored `.local/marketplace-expansion-research/contribution/` directory.

Focused tests cover exact decimal limits and JSON strings, all-six and selected
edits, body-level inverse identity, unchanged-target idempotence, stale save and
lobby binding, changed selections, malformed authentication, out-of-range stored
counts, missing earned balances and duplicate contribution entries.

Parent verification independently rehashed all four native spans above against
the original EXE and reviewed the field/transaction code. The full transaction
test copies the historical save pair and original EXE into an owned temporary
fixture, never executes that EXE, and checks backup contents, exact readback,
idempotent retry, conflicting request IDs, stale-source refusal, interrupted
receipt publication and rollback. The original fixture and EXE are rechecked
afterwards. The full helper/transaction test run passed four tests; the
Marketplace browser suite passed 28 tests, including contribution review,
precise large amounts, changed-target invalidation, pending-request recovery
and running-game refusal.

In-game display, purchase/use, saving and reload acceptance remain pending. This
document does not claim a new game session or a live contribution edit was tested.
