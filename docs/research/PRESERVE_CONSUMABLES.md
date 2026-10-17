# Preserve selected consumables

Status: the local native extension is implemented and its owned fixtures pass.
Root independently integrated and checked the 561-key Rust selection. The
expanded native implementation awaits parent/package review and controlled
in-game acceptance. Nothing was installed or exercised in a running game.
This is a selected-item feature, not a global inventory-cost bypass.

## Exact selection and protected actions

The original unblocked ItemInfo and ItemUseInfo tables define 561 complete,
audited families in `runtime/marketplace/consumables/consumables_catalog.h`.
A user selects items individually. The existing owned stack must contain enough
items for the original action. Protection does not grant items, grow stack
limits, skip eligibility, or bypass cooldowns.

| Family | Count | Item IDs / original family |
| --- | ---: | --- |
| Arrows | 9 | `2200`, `50001`, `50003`, `50011`, `1000070`, `1001314`, `1001315`, `1001316`, `1001321` |
| Stat potions | 15 | `1000028`, `1000030`, `1000034`, `1000085`, `1000112`, `1000174`, `1000175`, `1000228`, `1000229`, `1000230`, `1000304`, `1000320`, `1000377`, `1000392`, `1000448` |
| Healing | 2 | Holy Water `22009`, High Holy Water `1000476` |
| Food / edible ingredients | 535 | 403 food/plant definitions, 48 fish, 78 insects, 4 frogs, 2 other edible-material definitions |

Names come from Workbench's original localized game catalog. The arrow names
are Pyeonjeon, Arrow, Poison, Hyosi, Sleep, Ice, Lightning, Fire and Bomb.
The three-effect stat potions keep all three native effects; only the final
Skill record requests the self-item cost.

The 537 new healing/food definitions all share the original first eight native
types `16,0,16,8,8,0,0,0`. Their complete lists contain up to 17 use records.
They include reserve-slot setup, normal eating/drinking Skill actions, target
feeding and, for some ingredients, random-box conversions or caught-animal
release/summon handlers. The module protects the verified **Skill self-item
cost**, which covers normal eating/drinking, and the existing verified arrow
DestroyOnly cost. It leaves **feeding, conversion, releasing and summoning
costs native**, even when those actions belong to the same selected item.
This prevents a "preserve fish" selection from also making fish-release or
conversion operations free. No refund/add-item operation exists in this module.

MultiArrow `50057`, packages, recipes and families outside this exact catalog
remain unselectable. This catalog does not claim universal support for every
possible consumable action.

## Evidence source

Original local Windows build: `1.0.0.2976`; PE timestamp `1790086912`; SizeOfImage `389722112`; preferred image base `0x140000000`.

EXE SHA256:

```text
57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7
```

All addresses below are RVAs. Research only read the original EXE and archive tables. It did not execute native game functions or modify a game process, installation, or actual save.

ItemUseInfo table hashes:

| Original archive entry | SHA256 |
| --- | --- |
| `itemuseinfo.staticinfobody` | `d5ccba4f58e9f3412cd93f041b25a6fd321b54698cbef5f8ee5b06fafef9d340` |
| `itemuseinfo.staticinfoheader` | `1028844625b9062e17b861ae4f0f1402bb610647a33ae718407a0f9f71d0eaf2` |

The header contains 10,140 key/offset entries. The current original ItemInfo body round-trips through the existing project parser without changes. Each supported ItemInfo's entire use list is checked against the exact keys in `consumables_catalog.h`, rather than assuming that all items named "potion" have the same handler.

## Serialized tag, native type and payload mapping

The original outer Info decoder `0x1506c80` reads the blocked byte, then reads
one type byte at `0x1506cff..0x1506d16`. It passes that same byte in EDX to
factory `0x241df30` at `0x1506d30`. The factory jump table is `0x241ef60`;
each case writes the native tag at Data `+0x18` and installs the following
RTTI-verified vtable. There is no inferred enum shift.

| Native tag | Vtable RVA | Native type | Relevant derived payload |
| ---: | --- | --- | --- |
| 0 | `0x59bf6b0` | `ItemUseData_Skill` | Serialized count + (u32 skill key, u32 level); native borrowed array `+30`, count `+38` |
| 2 | `0x59bf788` | `ItemUseData_RandomBox` | Entire handler unchanged |
| 3 | `0x59bf7d0` | `ItemUseData_SummonGimmickWithCatch` | Entire handler unchanged |
| 4 | `0x59bf6f8` | `ItemUseData_SummonCharacterWithCatch` | Entire handler unchanged |
| 8 | `0x59bf1c8` | `ItemUseData_FeedToTarget` | u32 skill key + u32 level + u8 target kind; native `+30/+34/+38` |
| 9 | `0x59bf8a8` | `ItemUseData_DestroyOnly` | Empty derived payload |
| 12 | `0x59bf740` | `ItemUseData_Projectile` | Two u32 fields, 8 serialized bytes; not the arrow reserve-slot records |
| 16 | `0x59bf3c0` | `ItemUseData_RegisterReserveSlot` | u32 ItemUse key + u8 flag; native u16 handle `+30`, flag `+32` |
| 19 | `0x59bf408` | `ItemUseData_InventoryBuff` | Common fields only; not the type-8 feeding records |

The existing arrow guard previously combined type16 with the Projectile
vtable, an impossible combination in this build. This extension corrects it to
RegisterReserveSlot type16 / `0x59bf3c0`. That bug could leave the old arrow
policy permanently in passthrough even though synthetic fixtures passed.
Nine copied original constructor fragments established the native tag/vtable
mapping privately on owned byte buffers. The public synthetic regression
explicitly rejects the old combination. Projectile creation itself remains
native.

Original common decoder `0x241f230` maps serialized common bytes to self-cost
`+24`, gates `+25/+26`, frame `+19`, flags `+27/+28`, parameter `+20`, and the
condition vector `+8/+10`. The common prefix is 22 bytes including the condition
count, followed by nine bytes per serialized condition. Each condition becomes
six native bytes after wrapper-key conversion. The next bytes are the actual
derived payload, rather than treating those conditions as skill records.

Original Skill decoder `0x241f920` reads one skill per audited Skill record,
with its original level. Reserve-slot decoder `0x2422530` reads the five-byte
payload, while FeedToTarget decoder `0x2421690` reads the distinct nine-byte
payload. Projectile decoder `0x2420fe0` reads eight bytes. The five complete
copied decoders (including common) run only with owned streams, preallocated
owned arrays and mocked wrapper conversion/reservation. All external calls,
imports and dormant error/allocator targets are redirected to owned stubs;
none can call the original EXE. Twenty-two unchanged original raw records
cover every compiled common-field pattern and condition count (0,1,4).

These original bytes stay in an ignored private evidence header. Optional
verification sets `CRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER` to its absolute
local path. The default public suite uses owned synthetic structures and the
numeric semantic catalog; public CI requires no private header, and the release
build clears this option. A separate private read-only audit checks each byte
against the hash-pinned EXE, checks the actual factory table and RTTI, and checks
each raw record against the original archive body. These source witnesses
complement rather than replace the owned policy fixtures.

## Selected-player and existing-stack boundary

The ItemUse dispatcher at `0x2b49c10` gets a fresh per-call context. It resolves the selected client player and matching server actor through the normal managers, checks the full identity, actor/component ownership and registry stability, and requires the supplied inventory component to belong to that server actor. NPCs, mounts, other actors and another inventory owner do not qualify.

The inventory slot is resolved from the actual invocation's inventory handle and slot number. Native `0x212f500` uses the inventory component's `+0x18` pointer array, `+0x20` count, inventory handle `+0x10`, signed slot bound `+0x0c`, and slot stride `0xc8`. The policy checks the slot's UID, ItemInfo handle and positive signed amount at `+0x10`. The requested amount must be positive and no greater than the existing amount.

ItemInfo manager `0x6d69aa8` resolves the slot's definition. ItemUseInfo manager `0x6d6f6c8` resolves its loaded use handles. Registered array handles, native semantic keys, unblocked state, expected RTTI, type, frame, self-cost/gating flags, other common flags, parameter, original condition count/pointer and Skill array count/pointer must match. The complete compiled catalog has 22 common-field patterns. The source use vector must be a unique subset of this item's exact use list. Missing or lazily unloaded rows fail to the original action; the module never loads or invents definitions itself.

Parent review found and removed two unjustified guards inherited from a different
Info layout. **ItemInfo and ItemUseInfo `+0x11..0x17` are padding**, not a
ready-state/index pair. ItemInfo constructor `2412710` initializes key `+0`,
string `+8`, blocked `+10`, then fields starting at `+20`; its deserializer
`150ecb0` reads key/string/blocked followed by the eight bytes at `+18`.
ItemUseInfo loader `60ffd0` allocates `20` bytes; constructor `610078` and
deserializer `1506c80` initialize key `+0`, string `+8`, the single blocked byte
`+10` and Data pointer `+18`. They do not initialize padding as metadata.
The policy instead binds array registration, actual semantic keys, stack
definition handle and the complete verified use family. Fixtures deliberately
fill the padding with arbitrary `a5`/`5a` values, preventing false success from
seeding nonexistent fields. This is distinct from the actual kind and ordinal
fields of StatusInfo used by the separate stamina/health policies.

Every cost decision repeats the selected-player resolution and checks the same UID, slot, definition, use rows, requested count and source list. A changed selection, replaced stack, insufficient quantity, changed use family or unreadable address returns to the original path.

## Skill self-cost, effects and borrowed lifetime

The Skill use handler is `0x2b4a960`. Its call to ItemUseInfo accessor `0x609fe0` returns at `0x2b4aa10`. Only this exact accessor return site, inside a verified selected stat-potion/healing/food ItemUse context, may receive a private copy.

At `0x2b4aaa7` the handler checks SkillData byte `+0x24`. When set, `0x2b4aab1..0x2b4ab38` appends one 40-byte PopItemParameter for the self item to the cost request. When clear, the native branch goes directly to `0x2b4ab3c`. The module copies the 0x20-byte Info and 0x40-byte SkillData to the owning invocation's stack and changes only this byte from 1 to 0. Global Info/Data bytes are never written.

The original ItemUse dispatcher still obtains the original Info at its other accessor sites (`0x2b49dac`, `0x2b49df2`, `0x2b49e0c`). Its eligibility checks, virtual conditions, auxiliary use state, and notifications see the original records. The Skill handler still validates the original skill references/levels through `0x17fb990`, runs the native cost transaction, and applies the original effects. All other Data bytes, skill-array pointers, conditions and gating flags are preserved. ItemInfo cooltime and native scheduling are unchanged.

The copied pointers are borrowed, with the following complete native retention witness for this pinned handler:

1. `0x2b4aa10` reads `Info+0x18`. `0x2b4aa1e` selects that Data pointer by tag. `0x2b4aa22` stores it in the handler's stack slot `[rbp-0x28]`. The Info pointer has no further use before its register is overwritten.
2. `0x2b4aa5f` and `0x2b4aa63` read Data's skill-array pointer/count at `+0x30/+0x38`. Before the validation call at `0x2b4aa82`, `0x2b4aa7c` sets RDX to an element of that original skill array. The copied Data pointer is not passed to validation.
3. `0x2b4aa98` reloads `[rbp-0x28]` solely to check the self-cost byte at `0x2b4aaa7`. The next transaction call receives an independent request vector and stack result, not the Data pointer.
4. `0x2b4aee0` reloads `[rbp-0x28]`, then `0x2b4aee4/+ee8` read the original skill array/count for effect application. Calls receive those original skill elements and native actors. The copied Data pointer is not passed or stored elsewhere.
5. The entire handler has exactly these three references to `[rbp-0x28]`: one store and the two reloads above. No address of that local is passed to another function. The native handler returns before the outer ItemUse invocation returns.

Consequently, neither copied Info nor copied Data is retained by native code after the containing call. The shallow copies keep the skill-array pointer directed at the original engine-owned array. Each ItemUse has its own stable shadow storage; repeated lookup of the same record cannot overwrite another pending record. Byte changes to an already copied original record cause later lookups to return the original pointer.

## Arrow self-cost and complete native transaction

Arrows schedule RegisterReserveSlot records at frames 0 and 8 and a DestroyOnly use at frame 7. Their Data types are 16, 16 and 9. The module does not modify either reserve-slot record or any actual Projectile handler.

Only the DestroyOnly call at `0x2b4a2bf`, returning to `0x2b4a2c4`, may forward an empty self-cost vector. Its original 40-byte request must be the single request for the verified same owned stack:

| Request field | Required value |
| --- | --- |
| `+0x00`, u16 definition selector | `0xffff` |
| `+0x08`, u64 count | Exact requested amount |
| `+0x10`, u16 inventory handle | Same invocation inventory handle |
| `+0x12`, u16 slot | Same invocation slot |
| `+0x18`, u64 extra | Exact original extra argument |
| `+0x22`, u32 native flags | `0xffff0000` |

The native pop wrapper's other arguments must be the selected server actor, type 2, reason 8 and flag 1. If native default-inventory resolution chooses a different inventory handle, the policy deliberately falls back to the original cost.

The module calls the original wrapper with a borrowed zero-entry vector. It does not skip the wrapper, manufacture success, edit a prepared pop list, or restore spent items afterward. The original wrapper still performs preparation, validation, commit, finalization and notification stages and returns its actual result/error.

`0x27b1d20` is a jump to `0x101310c0`. Its empty request path was inspected through all stages:

| Stage | Native evidence |
| --- | --- |
| Prepare `0x27b0540` | Copies 40-byte requests; zero entries produce result 0 at `0x27b0629` |
| Plan `0x27b0660` | Request count `+0x58` zero branches from `0x27b0698` to native success `0x27b085c` |
| Validate `0x27b08e0 -> 0x1012aa30` | Resolved count `+0x20` zero takes the empty path |
| Commit `0x27b0b70` | Resolved 0xe0-byte record loop at `0x27b0b9d..0x27b0bb1` is empty; no `0x212df30` item pop |
| Finalize `0x27b0c90` | Zero resolved count at `0x27b0cbf` returns native success |
| Pop notifications `0x27b0fb0` | Zero count branches from `0x27b0fcf` to `0x27b11ce`; no fabricated pop notification |
| Post-pop `0x27b1540` | Native paths still run; per-record work and spent-item event `0x1804360` have no resolved entries |

Other inventory transactions, including crafting, shop payments and quest hand-ins, cannot match this exact ItemUse return site and self-cost request. Unselected items use the original pointers, vectors, effects and costs.

## ABI, recursion, threads and activation failures

The ItemUse callback forwards all ten native arguments, including the 64-bit extra argument, and returns the exact original result pointer. It never replaces native errors with success. The accessor forwards to its original implementation first, preserving lazy loading and native failure behavior.

The relocated pop wrapper writes entry `RSP+0x48` as a scratch result home. The forwarding typedef explicitly reserves a ninth 64-bit stack argument so that this native write cannot overwrite a live forwarding-frame local. The DestroyOnly caller only writes its corresponding outgoing `RSP+0x40` home; it never reads that scratch as an output. Its actual native result is the returned result object. A fixture writes the same ninth home through `_AddressOfReturnAddress()` and verifies all arguments and return/error values.

The active context is thread-local and points only to the current ItemUse stack frame. Every nested ItemUse pushes a context, including ineligible calls, so an unselected nested call cannot inherit protection. A `__finally` restores the previous context on ordinary return, C++ exceptions and native structured exception unwind. Guarded graph reads return to the original path on access faults. Different threads have separate contexts and shadow storage.

The DLL is pinned before creating hooks. Trampoline pointers publish with release/acquire atomics. A separate atomic policy gate stays false until all three hooks activate successfully. Partial activation therefore exposes passthrough callbacks only. Every failure cleanup attempts to disable every created target before freeing any trampoline. A failed disable retains all resources. After any activation attempt, published trampolines are retained for the pinned module's process lifetime even after successful disabling, because an already entered passthrough callback may still need one. Pre-activation failures may remove disabled resources safely.

## Workbench integration contract

Module: `CrimsonPreserveConsumables.asi`.

Setting: `%LOCALAPPDATA%\local.crimson.workbench\preserve-consumables.dat`.

Binary format: ASCII `CWC1`, little-endian u32 count, then exactly count little-endian u32 item keys. Keys must be strictly increasing, unique, and drawn from the 561-key catalog. Count is 0..561; exact length is `8+4*count` bytes. Missing, malformed, reparse-point, multiply linked or oversized setting files disable the module. No selection installs no hooks. Settings apply at the next game start.

The module requires both the pinned EXE file hash/PE identity and 16 native memory signatures before hook installation. It does not provide an automatic compatibility guess for another game build.

## Verification and remaining acceptance

The extension builds as a Windows x64 Release ASI with `/W4 /WX`. The focused
owned CTest passes with 9,651 callback invocations. It covers all 561 supported
families, all 3,537 healing/food Skill records and all 2,396 other mixed-family
records, selected/unselected costs, unchanged Info/Data bytes and original
skill-array pointers, effects, cooldown scheduling, native results/errors and
the ninth pop stack-home write. Mixed families are exercised one native row
at a time; fixtures do not pretend that eating, feeding and releasing all
execute in the same real request. Both normal and rejected contexts preserve
the original dispatcher behavior.

Further regressions cover wrong native type/vtable pairs, all common flags,
condition/skill counts and pointers, source overflow/duplicates, changed
unconsumed family rows, more than three simultaneous stable per-record
shadows (Wild Insam has four cost-bearing Skill records), owner/UID/selection
changes, unreadable pointers, nested calls, C++ exception cleanup, concurrent
threads and hook publication/activation/cleanup failures. The optional private
constructor/decoder fixtures additionally verify the actual native resource
mapping independently of seeded policy objects; that run passes **9,652
callback invocations**.

Private read-only evidence and reproducible harnesses are in
`.local/marketplace-expansion-research/consumables/`:

- `generate_catalog.py`, `selected-items.private.json`, `selected-patterns.private.json`;
- `generate_decoder_evidence.py`, `factory-types.private.json`, `factory-decoder-evidence.private.json`, `raw-decoder-evidence.private.json`, `decoder-bodies.private.json`;
- `verify_evidence.py`, existing `proof.private.json` / `pins.private.json`, disassemblies and `verification.private.json`.

The public fixture uses synthetic structures and owned mocks. Optional original
decoder execution uses owned copies with owned mocks. Neither suite calls code
from the original game image or accesses its process. The private ASI is a build
result, not an installed module.

Parent integration now adds the independent runtime card, item names from the
game catalog, name/ID search, exact individual selection, explicit saving and
clearing, and a separate install/remove lifecycle. Installation requires a saved
nonempty selection; removing or clearing protection remains possible. Settings
are stored atomically with a second running-game guard and never change the
user's game files themselves. Native and Rust eligible-key tables are checked
against one another. Malformed, duplicate, unknown and oversized selections are
rejected; invalid native settings leave ordinary consumption unchanged.

For this extension, Root independently checked all 561 native/Rust keys and
kinds and passed three configuration tests, including a complete selection
roundtrip and old CWC1 profile compatibility. Existing parent package/browser
results applied to the initial 24-item implementation. A new full native
package review is still required before claiming that coverage for this
expanded implementation.

A controlled in-game acceptance check is still required. It should confirm
native item counts, Holy Water healing, food effects/buffs and all potion effects, normal cooldowns, each arrow's actual
projectile behavior, feeding/releasing/conversion costs, depletion eligibility, unselected costs, quest/shop/crafting
costs, world transitions and coexistence with other modules. No live acceptance
result is asserted here.
