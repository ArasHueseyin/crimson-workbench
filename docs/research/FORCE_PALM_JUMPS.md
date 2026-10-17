# Native Force Palm chain repetition

17 October 2026. Planned feature #25 now has a native runtime implementation
and passing isolated policy and copied-native fixtures. **Gameplay has not been
observed or claimed.** Parent-reviewed distribution/UI integration is now
complete locally; a later user-authorized gameplay check remains separate.

The adapter reopens the existing aerial Force Palm chain after its native
three-stage sequence. It retains native inputs, learned-skill requirements,
movement restrictions, cooldowns and spirit costs. It changes no jump height,
stamina value, shared-value instance, chart buffer or archive entry.

All new inspection used the pinned original executable and original archive
entries opened read-only. The copied-native fixture executes only isolated
copies with owned callbacks; it never calls an address in the game executable.
No module was installed or injected, and no game/process/save/archive was
changed or controlled.

## Supported executable and original evidence

- Build **1.0.0.2976**, PE timestamp **1790086912**, image size **389722112**.
- EXE SHA256:
  `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
- Original `fist_upper.paac` SHA256:
  `950c6059444aa672af95efe4fe1c074dfaef3a47f75e44a7f8ee68231c5a940d`.
- Original `common_upper_branchset.paac` SHA256:
  `bff3f06e0a2e1a4000148c2137ee68c3a2332221b42638065f0ab183c07d9fdc`.
- `commonsharevalue.pashv` SHA256:
  `d7b66ede03c9dc4f9d5aef92d2f5eb1b0863741de17638cfca5e88c0e3a4fb00`.
- `defaultsharevalue.pashv` SHA256:
  `2d14d27c65596d2920931761bdcdd4c9d368c6435903196352025d05fe4108b3`.

The bounded PAAC parser follows native animation reader **1f66560**, package
reader **1f68370..1f69040**, and branch reader **1f670b0**. Both Force Palm
packages parse exactly to EOF. The fist package contains 1732 animation records,
814 chart records of stride `114`, 1836 branch records of stride `34`, and 5809
branch-reference records of stride `1c`. Its condition buffer has 11212 bytes;
the common upper branch set has **20640** condition bytes, **11334** finder
entries and **1646** branch records.

## The native three-stage rule and reset

Skill **15035**, `Skill_JiJeongTa_Air`, is the aerial Force Palm skill. Its
developer description identifies releasing energy toward the ground to raise
the character in the air. Its ordinary cost is **5000 Mp**, key **1000027**;
Stamina is a separate key **1000026**. Skills 10070/10162 are ordinary double-jump
definitions and are unrelated to this repetition limit.

The limit is a finite chart sequence plus a consumed boolean entry latch. No
integer counter with a guessed value of three is patched:

| Native route | Fist chart / animation | Following Palm route |
| --- | --- | --- |
| Common upper branch 285, then chart 582 and branch 1202 | Chart 459, animation 1462 | Branch 1541 or 1542 |
| First follow-up | Chart 550, animation 1467 | Branch 1547 |
| Second follow-up | Chart 69, animation 1468 | No fourth Palm-stage branch |

The first animation uses `att_air_move_buddhahand_ing_00`; animations 1467 and
1468 use `jumpstep_tongbea_2_00` and `jumpstep_tongbea_3_00`. The other initial
Skill-15035 chart variants are 100, 356, 399 and 497, with animations 1465, 1463,
1469 and 1466. They join the same follow-up sequence. All seven skill charts
retain their original native definitions. The third-stage chart exits through
its native routes; allowing the initial route again permits another chain.

The entry latch is **resetairaction**, native name hash **d2603bc1**. The hash
was established through native lowercase lookup3, functions **22cc0e0** and
**1364780**, rather than guessed CRC/FNV hashes. Its common PASHV bool record is
at file `1311`, default true. `resetairaction_jump` (**8019ee84**) and
`resetairaction_lunge` (**79e01366**) are separate values.

Initial Palm animations 1462/1463/1465/1466/1469 all reference the same native
UpdateShareValue event at fist file offset **237e07**, global event-buffer word
offset **1c872**. It is event type 12, bool type 1, hash d2603bc1, value false,
with no condition program: the initial Palm consumes the entry latch. The two
follow-up animations do not reset that latch.

The native reset event at fist offset **1c5c3f** writes true under the five-node
program `AND(base34, AND(extension184(0,0), resetairaction==false))`.
Extension184, thunk **362980** to **8702d70**, compares the current layer record's
`40` word with its operands. Base34 is supplied by native cache producer
**386080**, including native state/physics predicates at **387719..387762**.
These reset predicates and the separate jump/lunge reset events remain native.
The implementation does not assert an unobserved landing transition from these
field names; the later gameplay check should exercise landing/reset explicitly.

## Exact admission program and native consumers

Common upper branch **285**, file offset **ed69**, targets state hash
**b81b2188**, which resolves to fist chart 582 and then first-stage chart 459.
The branch retains the input condition `key_skill_16_start`. Its admission
descriptor starts at finder index **4612**, count **5**:

```
AND(extension183,
    AND(sharevalue(resetairaction) == true,
        nativeSkillEligibility(15035, level 1)))
```

The exact finder word offsets are `[0, 73, 0, 2717, 2722]`. The two typed leaf
records at byte offset **10868** are exactly:

```
234, d2603bc1, 3f800000, 0, 0
237, 00003abb, 00000001, 0, 0
```

Native evaluator **242d370** returns **0 match**, **1 false**, **2 invalid**.
Thus op629 is AND, op630 OR, op631 NOT. Extension dispatcher **360be0** dispatches
131..627 through the registered method table. Opcode234's handler **366d20**
resolves the actor's shared component at `actor+68 -> components+58`, looks up
the hashed value through **1f69c90**, reads the bool through virtual slot `88`,
and compares through **242dbe0** / **f5644e0**. Opcode237's **362df0** handler
retains native eligibility **21ed170**, including learned level, resources,
cooldown and other skill conditions. The adapter never replaces that right leaf.

## Borrowed layout, ABI and selected-player scope

The sole hook is evaluator entry **242d370**. Its exact Windows x64 signature is
`uint8_t(void* descriptor, void* control, void* frame, uint8_t flag,
uint32_t* cursor, uint8_t* negated, void* diagnostic)`.

The native descriptor has start/count at `00/04`, condition owner at `08` and
string context at `10`. The current condition owner is native package `+110`:
buffer pointer `00`, buffer byte count `08`, finder count `10`, finder pointer
`18`, branch count `20`, branch pointer `28`. Loader **1f686ff..1f68781**
establishes these fields; callers including **232d8dc** and **232e5d5** create
the corresponding invocation descriptors. Included branch views can have
different condition owners; no global package address is assumed or cached.

Only the recursive invocation at cursor **3**, within that exact five-node
program, qualifies. The adapter validates both leaf records, both logic/operator
cells, all five finder cells, native sizes/counts, and all 52 bytes of branch
285. Every read is bounded or fault-guarded. A different program, relocation of
the relevant nodes, modified branch, truncated/unreadable buffer, or failed
native comparison validity remains passthrough.

RTTI identifies control vtable **55b5460** as
`ClientCharacterControlActorComponent`. Its slots `268/270` are native condition
methods **360b60/360be0**. The current selected actor comes from client root
**6d69190**, manager `+30`, selected actor `+50`; manager and actor types must
match **55b62c8/5585c40**. The callback receiver must be that actor's component
`40` and backlink through receiver `+08` to the same actor. Identity, alive flag,
metadata kind 1/4, component count and possessor backlink are checked.

The native evaluator always executes exactly once with all seven original
arguments. Only native false **1** can become match **0**. Invalid **2**, already
matching **0**, and unexpected results pass through. Fresh volatile byte reads
revalidate the complete borrowed snapshot after the original call. Selection,
identity, component, script-owner or buffer changes decline the override. No
borrowed pointer survives the call. Native diagnostic/cache and negation-scratch
side effects are retained, including diagnostic `+04` reflecting the original
comparison; only the evaluator's return value is adapted.

## Server admission remains native

RTTI identifies `ServerCharacterControlActorComponent` vtable **5b25468**, whose
condition slot `270` is **2a854a0**. Its switch tables at **2a891dc/2a89148** map
op183,184 and234 to default **2a89137**. That default writes the negation scratch
and returns `!flag`; it does not query resetairaction. Native server evaluators
**2a7c780**, **2a7cc90** and **2a896e0** pass evaluator flag **1** at call sites
**2a7c92f**, **2a7ce28** and **2a89d23**, so this client-only latch condition
matches there without an override. Opcode237 has dedicated server case
**2a8588d**, leading to native skill eligibility **17fbd00**. No server control,
actor, eligibility or resource check is widened by this module.

## Runtime lifetime and coexistence

`CrimsonForcePalm.asi` is pinned before its initialization worker starts, and
again before hook installation. It verifies the on-disk EXE hash, PE identity
and 14 narrow native code/table witnesses. The trampoline is atomically
published before activation; policy stays disabled until activation succeeds.
After any activation attempt, cleanup disables the entry but retains its
published trampoline and pinned module for process lifetime, including failed
activation/disable paths. An in-flight callback cannot lose its original.

There are no Force Palm settings files. Installation selects the module;
removal takes effect on the next game start. The only hook is **242d370**;
the dismount module's separate opcode404 handler does not overlap it. Witnesses
avoid other Marketplace hook entries, including Blackstar's CharacterInfo getter,
ordinary jump **88bf40**, mount jump, stamina, speed, consumables, active mount
protection and crafting. No shared CMake/API/UI/package source was edited for
this implementation; the parent integration owns those changes.

## Verification and integration contract

Owned production files are `runtime/marketplace/force-palm/force_palm_scope.h`,
`force_palm_policy.h`, `force_palm_targets.h`, `force_palm_hooks.h` and
`force_palm_mod.cpp`; the tracked test is
`runtime/marketplace/tests/force_palm_tests.cpp`. Add a Windows x64 MSVC C++20
shared target **CrimsonForcePalm**, suffix **.asi**, source
`force-palm/force_palm_mod.cpp`, linked against pinned MinHook and **bcrypt**.
Use `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, and `/W4 /WX`. The policy test uses
`/UNDEBUG` and requires no game data.

Ignored audit/build root:
`.local/marketplace-expansion-research/force-palm/`. Its `CMakeLists.txt` builds
the module, tracked fixture suite and `native_eval_fixture.cpp`. Run there:

```
python make_native_fixture.py
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Release MSVC builds passed **2/2 CTest tests** with assertions active. Coverage
includes full original argument/result forwarding, every typed leaf/branch/finder
byte guard, wrong actor/move, unreadable/truncated buffers, script reload,
selection/identity changes, exception propagation, lifecycle failures and sibling
hook/guard coexistence. A 24-move/8-chain fixture retains native resource and
learned-skill failures.

The stronger ignored fixture copies **242d370..242d96c**, including its native
skip helper/tables. Internal calls remain in owned memory; recursive entries
route to the adapter, virtual handlers are owned stubs, cache flags are owned
zero bytes, and an unexpected external cache call traps. It proves native
recursion, all seven arguments, AL results, cursor and diagnostic/negation side
effects, continued evaluation of the right-hand skill gate, and the same
24-move fixture. A copied server default-case slice independently checks flag1
semantics. It does not execute a live engine call or reproduce gameplay physics.

A later gameplay check still needs more than three successive aerial uses,
spirit exhaustion, landing/reset, alternate initial variants, character changes,
and removal. These fixtures establish the native admission adaptation and its
scope; they do not establish observed gameplay or unlimited spirit.

## Parent integration verification

The parent independently checked the pinned EXE, all 14 native witnesses, the
complete original PAAC parse, exact branch285/five-node AST and typed leaves,
and server switch/default-case instructions. Both private CTest fixtures pass
under parent execution. The shared build includes `CrimsonForcePalm.asi`, the
native policy target, ID `force_palm`, and an English Marketplace card explicitly
retaining normal spirit costs. The full guard audit checks 76 new-module
witnesses against 38 distinct packaged hook entries.

The current ignored stage is
`.local/marketplace-expansion-runtime-20261017-force-palm-reviewed/`. It passes
all 12 Marketplace native targets, three LiveItems native targets and the real
pinned loader's synthetic host with 14 ASIs. Temporary package lifecycle,
independent removal, running-game/modified/foreign refusal and recovery checks
pass; all 38 Marketplace browser tests and the frontend build also pass.
The screenshot is a mocked browser fixture, not an installed-game result.
Nothing has been installed, committed, pushed or released for this expansion.
