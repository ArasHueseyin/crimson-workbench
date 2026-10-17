# Ordinary crafting runtime path

17 October 2026. Locally integrated native ordinary-crafting material support
for feature #13. It uses immutable recipe views and retains the native
prerequisite, output and inventory transaction paths. Automated checks pass;
in-game acceptance remains pending. No game
process was controlled or written, no native game function was called, and no
game files or saves were modified for this investigation.

## Evidence boundary

Executable 1.0.0.2976 is pinned to SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
All addresses below are RVAs. Native assembly and the private manifest remain
under ignored `.local/marketplace-expansion-research/crafting-runtime/`.
The manifest independently fingerprints 13 complete functions and 15
instruction-aligned critical spans read directly from that executable.

The archived-empty-input route below was investigated first and is not enabled.
The current module keeps original archive and recipe-manager rows unchanged,
so database initialization always sees its original valid positive costs.

Any ordinary-crafting transform must preserve consumption type 1,
all conditions, actual knowledge prerequisites, tools, flags, outputs and any
independently retained currency. Removing positive ingredient entries is different
from setting their quantities to zero. No original ordinary recipe has both
requirement vectors empty; the runtime needs explicit support for that state.

## Confirmed barriers

1. **Database initialization/validation:** `2429700` visits the recipes grouped
   by their craft tool. At `2429dec` it checks loaded fixed count `+48`, then
   `2429dfa` checks group count `+58`. If both are zero, `2429e00..2429e15`
   accepts only a special parent-recipe/consumption-type-2 case. Other recipes
   reach an error at `2429e4b`. An ordinary type-1 recipe needs a scoped route
   to the existing continuation `2429e17`. Do not change it to type 2: the UI
   enumeration independently excludes that mode.
2. **Client validation:** `213b640`, reached from `49ea90` and several recipe UI
   builders, creates fixed/group input vectors. At `213b793..213b799` it
   rejects when **both supplied counts are zero**, before ordinary recipe
   validation. The failure at `213b79b` returns a native error. The normal
   continuation `213b812` loads the recipe and still calls `2429370` at
   `213b852`. This is an additional necessary gate; adjusting only database
   initialization and the quantity dialog cannot make free crafting work.
3. **Quantity dialog:** `1074800` rejects material-model count zero at
   `1074846..107484d`. Its ordinary calculation initializes maximum quantity
   to `UINT32_MAX` at `1074853`, then takes minima from positive material
   availability. Empty inputs require an explicit finite maximum, not just
   deleting the zero-count branch. Material quantities must never be zero:
   `10749cb` performs an unguarded division by the required amount.
4. **Two UI-only knowledge fallbacks:** `e27040` and `107d6e0` read the actual
   recipe knowledge at `+30`. If invalid, they fall back to the first fixed
   item's knowledge, or the first member of the first ingredient group.
   After both input lists are emptied these dereferences are invalid. Exactly
   nine supported ordinary recipes originally have no explicit knowledge.
   Preserve their recipe knowledge field; supply the original display fallback
   only to the local UI register before its comparison, never to the recipe.

Changing the actual recipe knowledge to a display fallback would add a gameplay
prerequisite: `2429370` checks any valid knowledge at `242947b..2429509`, whereas
an invalid handle follows the native skip at `242950e`. The UI fallback and
actual prerequisite therefore cannot be represented by the same archive edit.

The nine original fallbacks are:

| Recipe keys | Original first item | Display knowledge key |
| --- | --- | --- |
| 1000085, 1000087, 1000089, 1000091, 1000093 | 1002870 | 1000689 |
| 1000160 | 720001 | 17047 |
| 1000415, 1000416, 1000417 | 75001 | 1000165 |

Possible narrow UI hook boundaries are `e27114` and `107d7c2`, after the actual
knowledge load but before its `FFFF` comparison. For a verified supported,
modified, empty-input recipe, replace only the local `BX` display handle. All
other recipes continue unchanged. These are proposed boundaries, not installed
or executed hooks. Resolve semantic keys to current loaded handles; do not
hardcode a table index as a knowledge key.

## Existing validation to keep

`2429370` evaluates conditions, knowledge and supplied ingredients. Empty groups
with empty supplied group data take `2429610..242961c -> 24296d5`; that invokes
fixed matcher `2428b40` at `24296df`. Expected fixed count zero and supplied fixed
count zero take `2428b65..2428b75` and return true. Therefore there is no evidence
that this recipe validator itself needs a free-crafting bypass. In particular,
returning unconditional success from it would incorrectly skip prerequisites.

Recipe-selection UI `10761e0` clears previous crafting state using `49fec0`,
assigns the selected recipe handle, and builds the material model using
`106ec30`. Both fixed and group model builders have count-based empty loops.
The reset `49ff7e/49ff85` clears both previous supplied-input counts and
`49ff95` restores requested quantity one, preventing a stale prior recipe's
materials from becoming the empty recipe's request.

The UI subsequently calls `49ea90` with availability checking disabled at
`107694c`. Its success is stored at context `+28b`; together with ordinary
craft-tool permission `+28c`, it controls the quantity button at
`107699e..10769c7`. Fixing the scoped client empty-vector gate should let this
existing permission logic operate; no unconditional button-enable patch is
justified. `1074800` has one verified direct caller, `1070abe` in the event
dispatcher `1070200`, for that quantity button. Normal craft action `1071c60`
still checks the selected recipe knowledge at `1071f76..1071f93`, calls
`49ea90` with normal availability checking, and retains its ordinary action path.

## Native request and transaction observations

The item-exchange request execution method `2a0c950` leads to `2b6c050`.
Its parser `2b71b10` visits 50 fixed slots and 50 group slots. Unused fixed
slots with invalid handles/zero identifiers and unused group item key zero
are naturally skipped; both resulting vectors may stay empty. It returns
success at `2b71fd1`; there is no additional zero-vector rejection in this parser.
`2b6c050` still calls `2429370` at `2b6cb58` before the transaction.

Within transaction `2b6d620`:

- `2b6d82d..2b6d847` gets the supplied material begin/end. Equal pointers take
  the native empty path to `2b6dcfb`, skipping material availability collection.
- The optional target-item path at `2b6dcfb` is skipped for invalid target item,
  as in ordinary crafting; it is not a mandatory synthetic input.
- Empty collected consumption entries skip at `2b6ddbf..2b6ddc2 -> 2b6df29`.
  The requested positive iteration count is still incremented/compared in
  `2b6df29..2b6df33`; it is not derived from the material-vector length.
- Result dropsets at loaded recipe `+e0/+e8` and additional dropsets `+f0/+f8`
  are processed separately at `2b6df7f..2b6e282`. Their definitions and native
  random/result paths must remain untouched.
- The later consumption-vector scan at `2b6e91c` skips naturally when empty.
  Inventory transaction setup `277e080` passes its consumption list to
  `27b0540`. That helper explicitly takes an empty list from `27b05b5` to
  `27b0629` and returns success. Output handling remains a separate path.
- The normal general inventory and result validation/commit routines remain
  in place (`277fe00`, `277ffb0`, `2780450`, `2780570`, `2780820` and their
  normal children). The bounded review found no reason to bypass them.
  Static loop acceptance is not proof of a completed native transaction.

The native material-reduction buff is not an alternative: its handling occurs
only after material selection/availability and probabilistically saves one unit
per iteration. It does not satisfy crafting with no materials owned.

## Immutable recipe views remove the startup dependency

The runtime leaves original recipe files and manager row pointers unchanged.
It hooks the standard handle getter at `4a21f0`, calls the original getter once,
and returns an immutable material-free copy only at **50 instruction-verified
post-call addresses** in ten reviewed UI, client-validation and server-transaction
functions. Database-initialization callers are excluded. Unknown callers,
unknown layouts, invalid registrations, resource exhaustion and incomplete
activation retain the original returned row. A late or failed worker therefore
cannot leave an invalid zero-input archive database behind.

Native allocation sites `4a6e95..4a6ee9` and `f5420c2..f54210f` allocate
**0x100 bytes aligned to 0x10**, then call constructor `2428070`.
`crafting_shadow.h` copies that complete representation and replaces only the
fixed/group input-vector region `+40..+5f`. All condition, knowledge, tool,
flag, output and additional fields remain exact. `crafting_source_costs.h`
contains fixed/group count contracts for all 1,507 original ordinary recipes;
an independent Rust parser checks these against the full pinned original table.

The native fixed-input reader `15119f0` establishes the 0x20-byte input layout:
item handle +0, character/gimmick handles +2/+4, u64 quantity +8, u64 coupon
count +10, refinement u16 +18. Container reader `1524600..1524707` appends
those complete 0x20-byte records. The runtime resolves loaded ItemInfo handles
to semantic keys rather than assuming a fixed table index.

For recipe 1010292, the four material entries disappear from the copy, while
its exact 300-Clockwork-Insect entry is kept in an owned, aligned single-entry
array. If the separate existing currency archive component already removed
that fee, the known four-entry original layout is also accepted and the view
has no costs. The other 17,069 type-3 upgrade definitions are never virtualized;
the separate upgrade-material component retains its owned-equipment input.

Views are stable allocations, retained until the pinned module's process exit.
They are not stack/TLS scratch rows and are never passed to native destructors.
A later original row/table generation produces a separate view; old views are
not recycled while a native reader might still retain them. The bounded pool
holds at most 8,192 versions and returns the original row if allocation or the
bound fails. Original native condition/result pointers remain borrowed, with
the same table lifetime boundary as the original getter's row.

`ShadowRead` lets the four existing client/quantity/display bridge policies
resolve the same current view while preserving every unrelated native read.
The database bridge is always passthrough. Empty-cost quantity selection has
a finite **40-per-batch** limit; a retained fee stays on the normal positive-input
availability/quantity path. UI-only fallback knowledge never changes the
recipe's actual knowledge prerequisite. Fresh scalar reads are volatile and
aggregate snapshots use volatile byte reads; repeated checks are conservative
snapshots, not a claim of native locking or synchronization.

## Hook lifecycle and packaging

The full EXE hash, x64 PE identity and **23 loaded native witnesses** are checked
before creating hooks. Five existing bridge entries plus the getter hook are
created, all six trampolines published, then queued and applied together.
Policies remain passthrough until complete success. The getter trampoline is
published through an atomic pointer. Native getter exceptions are not swallowed;
only the subsequent read-only adaptation has a fault fallback.

After any activation attempt, all published trampolines remain until process
exit, including on an activation error or successful disabling: an entered
callback might still require one. Pre-activation cleanup can remove resources;
failed removals remain owned for retry. The module pins itself before hook
creation and does not register a static pool/hook destructor during loader-lock
teardown.

Production CMake enables `ASM_MASM`, compiles `crafting_mod.cpp`,
`crafting_bridge.cpp` and `crafting_bridge.asm`, and links MinHook/bcrypt.
The synthetic `crafting_bridge_fixture.asm` belongs only to the fixture target.
MASM objects have explicit unique filenames for directory-containing paths.
The existing state bridge preserves GPRs, flags and enabled architectural
XSAVE/FXSAVE state, and does not have general Windows unwind metadata. No
exception may cross that assembly bridge; policy read faults are caught inside
its C++ boundary. The getter uses a normal compiler-generated x64 wrapper.

`CrimsonFreeCrafting.asi` is integrated with the manifest, module allowlists,
independent package lifecycle and the English **Free crafting materials** card.
Installation/removal still requires the game to be closed, and the module loads
at the next game start. No package or module was installed for this work.

The pinned upstream loader was inspected at commit
[6b440669](https://github.com/ThirteenAG/Ultimate-ASI-Loader/blob/6b440669144c4a0bef5718ab155df160d231cd42/source/dllmain.cpp).
Its synchronous exported initialization callback alone does not prove game
startup ordering, because loading can be triggered through intercepted APIs.
The current immutable-view design does not depend on that ordering and does
not install MinHook from DllMain.

## Verification and remaining acceptance

- The original full recipe-table Rust fixture passes, including all 1,507 native
  fixed/group count contracts and byte-identical non-cost restoration.
- Native recipe policies pass **4,539 cases**, plus current-row view composition
  with client, quantity and both UI display bridges. Original rows and registry
  entries remain unchanged; stale/blocked sources are rejected.
- Immutable-view fixtures cover every original ordinary recipe, exact retained
  fee and independent fee removal, all non-cost bytes, missing knowledge,
  malformed layouts, stable old generations, allocation bounds and 800
  concurrent lookups.
- Lifecycle tests cover each of 15 setup failure positions, all-or-none policy
  activation and retained ownership after potentially entered callbacks.
- **80 owned x64 bridge probes** compare GPRs, defined flags, x87/MXCSR/XMM and
  enabled YMM state, stack pointer, callback alignment, nesting, a caught read
  fault and concurrent calls. This host uses 832-byte XSAVE and AVX. A known
  high-FIP diagnostic quirk also occurs in the no-bridge baseline; stable low
  diagnostic FIP values are used, without changing production XSAVE restoration.
- The combined build passes 11 Marketplace native targets, two LiveItems
  targets and a synthetic loader host with 13 ASIs. Temporary package lifecycle,
  rollback, interruption recovery and foreign/running-game refusals pass.
- Frontend build, 37 Marketplace browser tests and application Cargo check pass.
  The new browser fixture proves the independent native toggle, running-game
  refusal and preservation of the separate currency selection. The three
  crafting-related browser tests pass again after final wording adjustments.

Final local native stage:
`.local/marketplace-expansion-runtime-20261017-crafting-reviewed/`.
Logs: `.local/marketplace-crafting-reviewed-native.log`,
`.local/marketplace-crafting-reviewed-package.log`, and
`.local/marketplace-crafting-browser-tests.log`. No desktop installer was built.

Game acceptance remains unperformed: fixed/grouped/mixed recipes with no
materials owned, the nine display fallbacks, knowledge/tool/quest gates,
retained/waived fee, quantity selection, full-inventory refusal, normal results,
and restoration after module removal. Charges outside the reviewed recipe
path remain an open part of feature #13; this card does not waive unrelated
shop, service or quest costs.
