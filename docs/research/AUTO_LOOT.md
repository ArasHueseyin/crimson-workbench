# Auto-loot: native nearby free ground and corpse loot

Implemented locally for Crimson Desert **1.0.0.2976**. This module has been built and tested only in an owned fixture process. It has **not** been installed, loaded into the game, or gameplay-tested. The running game, its process, saves and archives were not changed.

The adapter automatically requests normal pickup for nearby existing ground items and eligible ordinary corpses. It uses the player's native field-radius results **before camera/focus selection**. Ground keys are `1000004` (`Gimmick_PickUp`) and `1000016` (`Gimmick_PickUp_NoAction`), retaining the normal animation/frame-event route for the former. Corpse keys are `12` (`Dead_Loot`), `1000009` (`Dead_Loot_Simple`) and `1000337` (`Dead_Loot_Swim`), with the strict ownership and native non-crime gates below. Buy, thief, pet/catch/inspect, dialog, housing, puzzle, custom-pivot and child-interaction routes are excluded. This does not grant an item by ID, synthesize a drop, skip native capacity checks, or call the server inventory-transfer function directly.

## Files and build pin

- Implementation: `runtime/marketplace/auto-loot/{auto_loot_mod.cpp,auto_loot_policy.h,auto_loot_native.h,auto_loot_hooks.h,auto_loot_targets.h,CMakeLists.txt}`.
- Owned tests: `runtime/marketplace/tests/auto_loot_tests.cpp`.
- Ignored evidence and build output: `.local/marketplace-expansion-research/auto-loot/`.
- Original EXE SHA-256: `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
- PE timestamp `1790086912`, image size `389722112`.
- `auto_loot_targets.h` contains **104 exact 32-byte pins** covering the hook, native caller, player lease, field query/join, distance/camera flags, target resolver/release, pickup/loot checks and requests, non-crime gates, resource layouts and downstream transactions. Runtime startup checks the full original EXE file hash, PE metadata and loaded-image byte pins before creating a hook.

All addresses below are RVAs for this exact build. The only hook is **`0x9af360`**. Its only authorized native caller return address is **`0xad26e1`**. There is no hook on LiveItems' `0x2774b20` and no dependency on its task-observer lease. A different caller receives the original function with no policy work.

## Nearby candidates and ownership

The native client frame `ad2380` calls `8b4570` at `ad26c3`, checks the resulting 32-byte selected-source actor lease at `ad26c9`, and calls `9af360(field, source)` at `ad26dc`. It releases that selected-source lease only at `ad26f1`. The hook calls the original first and performs its work synchronously before returning to this caller.

`9af360` reads the source position and bounds under the native actor guard. It queries field cells using `9abde0`, runs `9ae960`, joins the query workers, sorts the results with `9b2f70`, then sets `ClientField+318` ready. The worker thunk `9b39a0` jumps to `9aef70`. Each field result is `0x28` bytes: a native 32-byte actor lease, squared distance at `+20`, and classification flags at `+24..26`. The distance is `max(0, center distance - source bound radius - target bound radius)^2`, as shown at `9af062..9af0fb`.

The later native interaction selector `ac3920` reads the same list from `ClientField+308/+310` and compares its distances against the square of the native interaction radius at global `6cca8b8` (`ac3aa6..ac3b61`). Auto-loot uses that radius with finite/bounds checks; it does not invent a world-unit/metre conversion or widen the engine radius. The native cell query remains the candidate boundary.

The worker separately derives `result+25` from the camera frustum at `9af149..9af221`. Auto-loot deliberately does **not** apply that camera flag: an eligible item behind the camera can be considered without selecting it manually. It retains the active-lease and `+24` exclusion checks. Native pickup eligibility and position/collision checks still run afterward.

While snapshotting IDs, the adapter takes the native one-pointer read guard `393df0` on `field+8`. The same field lock node is used by field cleanup at `9a565f..9a5675` (the native cleanup's exclusive guard is `393650`). It verifies the actual field through the selected `ClientActorManager+70 -> first field`, exactly as the caller at `ad26d1/ad26d9`, and the reverse `field+218 -> ClientRoot+10f8 -> same manager` binding used by the native query at `9af4d3/9af4da`. The manager must have vtable `55b62c8`, and the field must have vtable `55bcea0` (`ClientField`), ready `+318`, and enabled `+31a`. The later UI reader's `root+c8` path is not used as an ownership proof; the fixture keeps it deliberately bound to an unrelated field. Counts must fit capacity and be at most 4096; the adapter snapshots at most 512 scalar actor identities. It does not retain the borrowed result-vector pointers.

Each snapshot ID is then independently resolved with `8ae940(ClientActorManager*, ActorLease*, uint32_t)` and released with `1434880(ActorLease*)`. `8ae940` takes the native registry lock and obtains a fresh actor reference before returning. This owned lease encloses all reads, eligibility checks and the request for that target. Cache updates or pointer reuse cannot substitute a stale cached actor pointer. **`8c4840` is a move into a lease vector, not a copy/retain function, and is not used.**

## Selected player and native execution context

The client-source lease returned by `8b4570` uses `manager+58` (the selected possessor), then **possessor+d8**. This differs from the read-only selected identity at `manager+50`, whose `ClientChildOnlyInGameActor` is at **possessor+d0**. Treating these as two players, or directly comparing the source pointer to `manager+50`, is incorrect.

The adapter revalidates this complete pair repeatedly:

1. Client TLS realm byte `[gs:58][0]+1ec` is zero and its native context/pool `+250` exists.
2. The root at `6d69190` has a `ClientActorManager` (`55b62c8`), and its `+50` selected child has vtable `5585c40`, active `+96`, actor-kind metadata 1, and the expected nonzero identity.
3. The child's `+a0` possessor equals manager `+58`; possessor `+d0` returns the child and `+d8` returns the leased source.
4. The source is `ClientNormalInGameActor` (`55857b0`), actor-kind 1, same identity, same possessor. Its component table at `+68` has a `+70` count of at least 20 before reading the `+98` slot. That interaction component is `ClientInteractionActorComponent` (`55bdb30`) and has `+8` owner equal to that source.
5. The component's busy state `+2a` is zero. The receiver equals root `+58`, and its native busy/request flags `+2c`, `+d8`, `+dc` are clear.

The client component-count layout is independently proven by the `ClientNormalInGameActor` constructor chain `898af80 -> 4f8db0 -> 8bfa030 -> 2067dd0 -> e0ba930`: `e0ba964/e0ba968` initialize table pointer `+68` and count `+70`. Its shared cleanup `e0d25d0` uses `+70 * 8` to iterate the `+68` pointer table at `e0d2615..e0d2643`. Bounds check only the required slot; the server-specific 53..57 count range is not assumed for client actors. A guardpage fixture also refuses a declared sufficient count whose actual table span is unreadable.

The `selected_pair()` reader exists for owned object-graph fixtures; it grants no execution permission. Production additionally requires client TLS and the exact native frame caller/lease above. No source/target/component/receiver pointer is kept between updates.

## Actual item, interaction and request path

Under the target's native actor read guard, the adapter requires actor-kind metadata **7**, a `ClientGimmickActorComponent` (`55b7898`) at `target+68 -> +30`, and its owner `+8` equal to the leased target. The target's component-table count at `+70` must be at least 7 before reading slots `+20/+30`; its state component must exist and not have subtype `1d`. Reading the target's interaction slot `+98` separately requires a count of at least 20. The actual item at gimmick `+c0` must exist, differ from the native empty-item singleton at `6cfcd78`, have a valid internal item handle at `+8`, and positive signed 64-bit quantity at `+10`.

`2123710` proves the interaction-list semantics. Its first branch copies current cached records from target interaction component `+88/+90`. Otherwise, for actor-kind 7, it constructs base records from `GimmickInfo+30/+38`, reading each override's internal interaction handle. The adapter copies only **plain default records** under the target lock; those have no borrowed serialized/name/pivot payload. The default record is 48 bytes, with the global empty-string sentinel `6a65940` at `+0/+18`, null `+8`, handle at `+10`, pivot `FFFFFFFF` at `+14`, zero `+20`, option `FFFFFFFF` at `+28`, state 2 and flag 0 at `+2c/+2d`. Custom records are never simplified into plain ones.

Interaction definitions resolve through `3888b0 -> 87e65f0`. `150ac00` proves the field layout. The whitelist matches **external key and native type together**: `1000004/type 0` or `1000016/type 0b`. It requires unblocked `+10`, exactly one `_interactionPivotList` entry (`+40`), no pop-item cost list (`+78`), undefined dialog set (`+80 == FFFFFFFF`), no child list (`+a0`), and no wait-for-interaction flag (`+c1`). Pivot count is not an animation/action-step count.

Gimmick definitions resolve through `3885b0 -> 38ba40 -> 1501e40`. The adapter rejects blocked `+18`, optional `_buyableDropItem` pointer `+288`, `_isBuyable` byte `+66d`, housing `+3c9`, and puzzle `+3ca`. `22cf3b0` proves that `+288` is an optional allocated object pointer, not a boolean.

For an accepted actual-item candidate, the adapter performs:

1. Native `9ba050(component, target, nullptr)` for the existing source/target conditions, native interaction list, mode-0 eligibility and collision/position validation. A null output is an existing native call mode (`ac2c5d`, `ac2ca1`); the engine owns and destroys its temporary vector.
2. Native `9b94b0(component, target, record, 3, Eligibility*)`, with the same 16-byte result initialization used at `abf827`. Only **state 2 and error 0** permit proceeding; conditional/confirmation states are refused. This retains common conditions through `211fcb0`, quest/object eligibility, client context and native range/pivot checks.
3. Native `8f5850(gimmick, int* error, source, 0)`, requiring its return to equal the supplied result pointer and error 0. This is the same existing-item pickup capacity route called by the client's normal PickUpItem frame event. Mode 0 invokes the native inventory-capacity check `2107f30`.
4. Fresh target/item identity, component, definition, selected-pair, cost flags and current plain interaction-list membership checks. No target lock is held while calling a pickup/request function.
5. Native **`abf770(receiver, int* error, target, record, bool* dialog, uint32_t* scratch)`**, initialized exactly as the ordinary UI request: error -1, dialog false, scratch 0. This reacquires the selected source lease, repeats native validity/eligibility and reaches normal client preemption `9b5eb0`. The adapter never supplies an affirmative answer to a native confirmation.

The engine owns completion. For ordinary animated pickup, the native client/server PickUpItem frame events (`8011d0`, `27ddf80`) keep their animation/target-lease checks. For NoAction type `0b`, `ServerPickUpItemInteractionProcessor` (`5b2aac0`, execute `2b39bd0`) resolves its current target, copies the **existing** gimmick item and checks capacity before reaching `2aa61c0`. That shared native dispatcher performs the actual pickup transaction. Auto-loot does not call it directly, and never calls the unrelated ItemPicker grant API `2b3ee50`.

## Ordinary corpse loot and the independent non-crime gate

Corpse candidates come from the same complete nearby field results and fresh target lease as ground items. There is no focus requirement and no additional hook. The target must be a `ClientNormalInGameActor` (`55857b0`), kind 1, with at least 26 component slots before reading `+c8`. Its state (`+20`), interaction (`+98`, `55bdb30`) and loot (`+c8`, `55be5d8`) components must belong to that exact leased target. Constructor `8ab190`, especially `8ac391..8ac3ff`, proves that `ClientLootDropActorComponent` occupies index 25. State `+273` must be **nonzero**: `ConditionData_CheckDead::evaluate` at `224f16d..224f17c` returns a condition status, with zero meaning success and `SETE` producing failure for a zero field. Independent `AICondition_CheckDead` (`ead9178..ead9192`) likewise returns success for a nonzero field. Most directly, the ordinary `ServerLootBuffProcessor` branch at `2acd4de..2acd4fe` checks `LootBuffData+90` (DeadBody) and requires nonzero target state `+273` before continuing to native loot eligibility at `2acd58c`; a zero field enters a separate possessor/serial path that this adapter does not automate.

Ordinary corpse interactions are obtained through **`2124060(component, true, const uint32_t* externalKey, Interaction* output)`**, the existing `FindInteraction` route. It takes the target's native actor guard at `21240b1`, constructs current interactions through `2123710`, resolves their definitions and compares the external key at `21240fe..2124100`. This includes native current caches, character defaults, activity and override handling. It copies a matching record into an owned output at `21241a6 -> 2126490`, releases its guard and destroys its temporary records/buffer. Auto-loot initializes the output with the native empty-string sentinels and defaults, keeps only plain records whose resolved key equals the requested key, and destroys each owned output with `9c0710` before returning. That clear routine destroys the three owned payloads and sets vector size to zero, without freeing the caller's stack buffer. Custom serialized/pivot payloads are discarded without converting them to plain defaults.

Each of the three corpse keys must have native type 0, unblocked `+10`, pivot count 1, no pop-item costs, no dialog, no child list and no wait flag. `Dead_Loot_Pet` (`1000076`), `Dead_Inspect` (`1000179`) and `Dead_Catch` (`21`) do not enter this family. Native mode-0 scan `9ba050` and full mode-3 eligibility `9b94b0` still require state 2/error 0 and preserve the existing condition, quest/object and range checks.

The additional loot gate is **`2144860(lootComponent, int* error, unusedThird, sourceActor, true)`**. The source is the fourth argument (`r9`); the fifth argument is the native distance-check flag. The original third argument is unused by the body. `ConditionData_IsLootable::evaluate` (`2251f60`) and `ServerLootBuffProcessor::check` (`2acd4b0`, call `2acd5ce`, vtable `5b2eae0`) establish this ABI. The function reads actual existing drop data through `2145600`/`2145520`, acquires the source inventory through `212a150`, checks target/source distance and bag availability, and returns the supplied error pointer. Only error 0 permits a request. Auto-loot revalidates the corpse, selected player, plain record membership and definition, then enters the same ordinary **`abf770`** request used for ground pickup. The normal animation, target-buff and inventory completion route is established below; the adapter does not call its execution/transaction entries directly.

Free interaction keys alone do not prove that looting a body is legal. The criminal-action frame event `27dd4f0` reaches registration `2959cb0` and common preflight `25d8570`. A negative result from the entire crime check is insufficient because it can depend on a temporary buff, timer or detection state. The adapter instead requires these independent, conservative facts:

1. State `+2e8` is the native custom criminal-action predicate. Its owner at `+18` (state `+300`) must be zero, its condition count at `+10` (state `+2f8`) zero, and its unconditional flag at `+1c` (state `+304`) false. `25d89bd..25d89e2` and predicate evaluator `178e5c0` prove that this empty, unowned predicate returns false. Even self-owned or conditionally exempt custom predicates are refused.
2. Additionally, either native **`2215750(source, target, true)` returns numeric 0**, or the target's valid, unblocked `TribeInfo` has `_wantedCrimeType == 7`. The first is an independent refusal at `25d8665/25d8667`, before any crime category is processed; no unproven name is assigned to that numeric relationship result. The second is an independent refusal for native categories 0/4/5/6 at `25d86aa..25d86b1`; the remaining categories reach the empty predicate above. Character definition handle state `+30` resolves through `389570`, its TribeInfo wrapper `+a4` through `3883d0`; loader `151d1e7/151d1fb` establishes TribeInfo field `+50` as `_wantedCrimeType`.

Both checks run again on a fresh snapshot immediately before request entry. Native functions take their own subsystem guards outside the adapter's guarded memory-read helpers. The source lease from `ad2380` and the freshly resolved target lease remain alive throughout; target actor guards protect snapshot reads but are released before calling `FindInteraction`, relationship, loot eligibility or the request. No corpse, state, component, interaction payload or definition pointer is queued or retained between updates. This accepts a proven free subset of ordinary corpse loot and deliberately refuses ambiguous ownership/crime data. It does not automate NPC inspection, pet collection, catches or crime confirmations.

## Native corpse completion and queued ownership

The ordinary request consumes the copied plain record safely. `2120cc0` reduces it to its definition handle at `2120d3a`. The complex-payload branch in `9b8b90` is unreachable for the admitted record flag `+2d == 0` (`9b8c09 -> 9b8fc7`). `9b5eb0` and `bb3ab0 -> a44a8d0` serialize actor IDs, options and scalar fields into owned native work. The deferred path at `abff79` uses `2126490` to make its own interaction copy. It does not retain the adapter's stack record or its borrowed target/component pointers. Actor leases remain local to the current synchronous query callback; the engine resolves queued actor IDs afresh.

Original `InteractionPivotInfo` reader `2404c60` maps each admitted interaction's pivot action to native field `+68`: `Dead_Loot` uses chart hash `d346144f`; `Dead_Loot_Simple` and `Dead_Loot_Swim` both use `5c093287`. All three pivot reward/output keys at `+a0` are zero. Their ordinary Type-0 interaction processor therefore does not substitute an unrelated direct reward operation.

The original `basic_upper_interactive.paac` resolves these action hashes to charts 279 and 129. Native `1f65a70` reads chart `+e0` as a **u16 start index into 28-byte descriptors**, with count at `+e4`; these are not physical animation indices. Descriptors 306..310 and 311..312 have flag 1 and resolve through `1f618c0` to 52-byte branch rows 112..118. Their `+14` hashes select child charts 266/181/165/66/195 and 84/23. Child chart `+54` is the u32 physical-animation index, and `+ea` is its count; native loader `1f68120..1f68143` and reader `1ec6eb0` access the actual animation-object table at root `+238` using those fields.

Every branch reaches one of the following seven physical animations and its genuine Attack frame event:

| Child chart | Animation | Attack name hash | Original PAATT base offset | Skill / level |
| --- | --- | --- | --- | --- |
| 266 | 80 | `ee979055` | `42d7` | 19 / 1 |
| 195 | 81 | `02dd1808` | `468d` | 19 / 1 |
| 66 | 82 | `4aac2275` | `4a43` | 19 / 1 |
| 165 | 83 | `c27feba8` | `4df9` | 19 / 1 |
| 181 | 84 | `4103df89` | `51af` | 19 / 1 |
| 84 | 85 | `ec37889d` | `52d9` | 19 / 1 |
| 23 | 86 | `5c166048` | `5403` | 19 / 1 |

Native `1f66a45..1f66a8a` reads each 20-byte animation event instance's **u32** index at `+8`, multiplies it by four and reads a **u16** event type from root `+1a0`. The original loader `1f689a8..1f689fd` places that shared buffer at resource offset `3f5bc`, length 27,352 bytes. All seven events are type 0, which the server factory `28f0210 -> 28f024e` constructs as `ServerFrameEventAttack` (vtable `5b1da60`, execute `27d8550`). Cosmetic LookAt/EffectSwitch/Sound events are not used as pickup proof.

`27d8550 -> 101a3f00` enters the native attack component and `1565970`. It reads the Attack name hash from event data `+8`, resolves the matching original PAATT row through `1ea94d0`, and stores the request's attack body at `rawBase+8`. All seven original version-0 PAATT rows contain external SkillInfo key 19 and level 1 at raw base `+108/+10c`, corresponding to body `+100/+104`. Original SkillInfo 19 is the unblocked `Skill_Looting`: one level, one non-null type-11 `LootBuffData`, target selector `+14 == 0`, DeadBody payload `+90 == 1`. The native common reader `1f28cc0..1f29284` and LootBuff payload reader `1f2d880` establish those positions. Skill 20 is the separate non-dead-body variant and is not substituted.

The attack's normal application wrapper `17f3e20 -> c89f080`, call `c89f11d`, invokes `ServerAttackActorComponent` vtable `+240 = 28b0f30`. That function retains the same request, iterates valid native target rows and calls **`28b21d0` at `28b130f`**. The target-side skill loop uses the body's SkillInfo references and level, resolves 19 through `3880a0`, and admits target buffs at `28b30f8` by skipping selector 1. Calls `28b3545`/`28b371c` invoke the target skill component's vtable `+178 = 2bee940` with an owned non-null buff context. The separate source-only method `21ef5a0`, which requires selector 1, is not the LootBuff application path.

`2bee940` reads the actual buff type at `2beea22` and selects the registered processor table at `2beea37`. Registration `2be9027` places `ServerLootBuffProcessor` (vtable `5b2eae0`) in slot 11. Its genuine apply call at `2bef855..2bef88b` invokes processor slot `+10` with non-null owned buff data and the native source/target actors, reaching **`2acd5f0`**. This is distinct from cleanup calls with null buff data.

`2acd5f0` queues a seven-byte payload through `27745f0`: timer type **`0x7e8`**, byte `ff`, and target actor ID. The original timer metadata at `6a7fe60` declares that type and payload size; initializer `230db5` installs vtable `5b34858`, whose handler is **`2c31430`** (`TrocTrProcessLootingDeadDropOnceTimer`). The handler reads target ID at payload `+3`, obtains a fresh native actor lease at `2c3146e`, and calls **`2b81410` at `2c31578`** on that target's loot component with the source actor. The native immediate fallback at `2acd736` reaches the same entry. That entry passes the existing corpse/drop items through `2b85870 -> 2b829a0 -> 2781490`, the inventory transaction path. Type `7e8` is not the separately registered `TrocTrLootingDropFromDeadBodyReq` (`bec`). Thus the admitted ordinary interaction has a concrete existing-item transfer route, rather than only opening a loot dialog. Native eligibility, animation interruption and engine outcomes can still refuse completion; offline provenance does not claim that a live pickup succeeded.

## Bounds, request outcomes and compatibility

Policy evaluation is limited to once per 250 ms, requests to once per 500 ms, and one request per update. The attempted set grows with successful request entries, stores full 32-bit native player/target identities packed into a 64-bit pair, and never evicts. There is no fixed request-count limit or unproven despawn garbage collection. A nonblocking atomic-flag guard serializes set access; contending calls skip that evaluation. A capacity or eligibility refusal before entering the request does not consume that identity. Once a request enters native code, a failed, exceptional or unknown outcome never authorizes automatic retry for that identity in this process. This favors avoiding duplicate requests over recovering a transient failed request automatically.

The set is constructed lazily, so its allocation cannot fail during DLL static initialization. Only construction/insertion of this owned container is inside the `std::bad_alloc` catch, before request entry. An allocation failure permanently disables further policy evaluations/requests for this process without forgetting any prior attempted identity. Native calls remain outside that catch; even a native `std::bad_alloc` propagates and cannot be mistaken for an owned-container failure. Memory use therefore grows with distinct requested identity pairs, with fail-closed behavior if allocation becomes unavailable.

TLS reentrancy guards enclose the original query as well as policy work; nested originals still run, while nested automation is skipped. C++ unwinding restores the policy/frame guards and releases freshly resolved target leases. Guarded memory-read helpers catch access violations. Normal engine-call and owned-record cleanup exceptions propagate. If cleanup itself throws while a primary native exception is already unwinding, only that secondary exception is contained, a permanent atomic failure latch disables further automation, and the original exception continues unchanged. Cleanup is marked entered before the call and is never retried after a partial failure. The fixture tests simultaneous lookup/cleanup failures and verifies the primary exception, balanced locks, one cleanup attempt and permanent refusal. Native races can still veto the request or enter the engine's ordinary confirmation UI; the adapter does not bypass or confirm those outcomes.

`start()` pins the module before creating the asynchronous hash/initialization worker, so an early ASI unload cannot invalidate the worker's code. A failed pin starts no worker. The hook transaction also verifies the pin before publishing a trampoline and enables the policy only after successful hook activation. On any activation attempt, cleanup disables policy/hooks but retains published trampolines and the pinned module for process lifetime. MinHook initialization ownership is respected. This is an exact-build adapter for the audited original resource semantics; other asset mods can change interaction behavior and are not gameplay-certified by the byte/field checks.

## Evidence and verification

The ignored `evidence-functions.json` records exact ranges and SHA-256 values for 90 native code/metadata ranges. Key ranges include:

| Native range | SHA-256 | Role |
| --- | --- | --- |
| `9af360..9af53a` | `fbd24f2a871e8254618f40ee5fee8fed8acaa4f86c2741f45a3ffb912f42aa40` | Nearby field update hook |
| `ad2380..ad2fe9` | `63e284f758589d40e054b7efefe8222d352b21528837de497e7c141f5bcfa7f0` | Client frame source lease and exact caller |
| `9ae960..9aef65` | `2c1cf5674f565787e13074e076130aa04969439467c64e207c7643eacfd6cc7c` | Native worker dispatch/join |
| `9aef70..9af354` | `e1fa81530fc9f4fcb0f54a2bf7dacbeea948c1929bf63272e1bc81a31e130a40` | Owned lease move, distance and camera flags |
| `8b4570..8b46f6` | `525f46ca27beb1350c7ba7a87f2187d1d7d52b795e1e8ae4b4b64b6988a7de3c` | Selected normal-source lease from possessor+d8 |
| `8ae940..8aeb25` | `a37adcce5cbfd78d78266cb0558201fa5b0a30183314bf33132385cfe170423e` | Fresh target actor lease |
| `1434880..143491c` | `58962428c45832afd42ef53dbda8feeef6d8bebb53c709f1d79c683bbdc49860` | Native actor-lease release |
| `9ba050..9ba50d` | `fb4477d244f99c29acfe30c0dadd36094732575b3bf81b9f65546416a948306d` | Existing candidate scan/check |
| `9b94b0..9b98f4` | `b5d452da98bae8139370c6dcb92a2747b3b7e2f0f3af6f3e266b9f969c279c0e` | Full interaction eligibility |
| `8f5850..8f6dd0` | `6fbe4904ecb71a34bcac0d7be0a264225e85367474347a149af9bd9bb2ea6697` | Actual-item pickup/capacity check |
| `abf770..abffda` | `923dfa7332b9ee4654005fd61021509a062e04e838220a92c410cb0b41aef452` | Ordinary UI/native pickup request |
| `28b0f30..28b1568` | `ed59c68cbc40402032c3256dd534d6934071a454f71c72e59450be552db0595f` | Native attack target loop |
| `28b21d0..28b3b70` | `585f23615434a0e7bd25bb3e3763c6ccfc09b48db6c4b7235891461157f98e55` | Target-side SkillInfo and buff application |
| `2bee940..2bf0e68` | `910cd424e2e048dcf7621bc7a67e6a3fc97acb908e06028ba524189571583b1b` | Actual type-11 processor dispatch |
| `2c31430..2c3167c` | `72f5cb6826273ca62a3a1d9acd11f87d5ffed1ed23c0d971093100f9d4e730f1` | Queued dead-drop timer resolves target and enters transfer |

Read-only extraction from original archive 0008 produced:

| Owned evidence copy | SHA-256 |
| --- | --- |
| `interactioninfo.staticinfoheader` | `5baca1ec6ca7cbdf0873d221ec76608e24657a1dbd47dfbc265dcc8ed75fdb27` |
| `interactioninfo.staticinfobody` | `24f20dddeecb1afde8c6d7e7a2172d21afdb84a505c26ba37d91b6e016987cb3` |
| `gimmickinfo.staticinfoheader` | `b910a67ff153071fc7366a5776b304b0738a3cbe7ab8e02591883ff73fae15db` |
| `gimmickinfo.staticinfobody` | `627de5cab68cf23856602498ca5fd0ed30e71e5faa8171368cc460ab4e7f7fed` |
| `tribeinfo.staticinfoheader` | `7262af0d6f590e3ffd508506727c4f734bd7b9528b31ad8203423920b4eca5a6` |
| `tribeinfo.staticinfobody` | `0610072a7608a8d96aa842b6044256101286f17f242025bceb7a3effada24767` |
| `skill.staticinfoheader` | `be0bce4a25cf8a977e6981eb333e83dcf7f94b5102e6620bdbd7b93d5eea95cb` |
| `skill.staticinfobody` | `1c62b93ea8ec32445d4078becd8e8b451e13ab519bf638721a0ed8258ac01f41` |

Read-only extraction from original archive 0010 also supplies:

| Owned evidence copy | SHA-256 |
| --- | --- |
| `basic_upper_interactive.paac` | `4af83367c99b2710cb7612281af54d19191225132e585132450519393d737250` |
| `basic_upper_interactive.paatt` | `4096ff9604001055348d339673ee5ef993f23bdeab89bc93dae6757963b84eab` |

The original interaction header has 437 entries. Ordinary free pickup key `1000004` is record 15, body `[6387,6821)`, SHA-256 `a7b235e90bf576a4fe2482ac75d35a5a7bc6a36b20e454aa22ee9a81e1cf0f71`; NoAction `1000016` is record 16, `[6821,7284)`, SHA-256 `a678fdf26eebb1c019afe24dd67680760f73079a8acb098c42b67bcbad110276`. Buy NoAction `1000138` has the same native type `0b` but a distinct key; accepting type alone would be unsafe.

The three normal corpse records all have native type 0: `12`, record 10, `[4009,4494)`, SHA-256 `bcfc1283e3445f621a17527055686d97753f1a84b574fd2eabe44dd8d02eb1d3`; `1000009`, record 11, `[4494,5004)`, `f032cdf12d1582f864e133bbfff5f0c5f69b26ef5b528613a43889b5464b175d`; `1000337`, record 12, `[5004,5524)`, `c105da50bc367ed27190e2e8a7716c505e3c1518f2a0478e6c098565cac396b5`. The original TribeInfo has 372 records, including 79 with wanted-crime type 7, such as animal, deer and wolf groups. The ignored `corpse/resource-evidence.json` records the exact parsed resource offsets/hashes and examples; the native category proof above determines eligibility, rather than interpreting tribe names as a legal exemption.

Commands from the repository root (PowerShell):

```powershell
# From the repository root: download pinned dependencies, build and test all modules.
.\scripts\build-native.ps1 -OutputDirectory '.local/native-verification'
```

MSVC Release `/W4 /WX` builds the ASI and fixture; CTest passes 1/1. `verify_evidence.py` verifies all 104 runtime pins, 90 native code/metadata ranges, EXE metadata/hash, ten original-resource hashes and exact pickup/buy/corpse records. It also executes `corpse_transfer_resources.py`, which consumes the complete original PAAC with native widths, verifies all nine interaction variants/seven distinct animations and Attack rows, and checks SkillInfo 19's target selector and DeadBody LootBuff. The derived output is `corpse/transfer-resources.json`; original bytes and typed-parser output stay ignored. The fixture exercises the actual native readers against owned virtual-memory objects (including source/child possessor round trips, actual-item checks, behind-camera candidates, native radius, plain cached records, native lock/release ABI, unrelated `root+c8` fields, manager/field backlinks, component-count bounds and unreadable table spans), plus mocked native function boundaries for refusals, capacity, stale identity/membership, request deduplication, rate limits, reentrancy, exceptions and hook failures. Corpse regressions cover all three keys; death-state polarity against the independent original condition-status and DeadBody-loot predicates for flags 0/1/2/255; native component type/owner/count and guardpage failures; owner/custom-crime rejection; both independent exemption paths; current native key matching; owned-record cleanup including primary-plus-secondary exceptions; loot capacity/distance ABI; pre-request failures and failed/exceptional request deduplication. Its growth regression accepts 20,000 distinct requested pairs, then refuses all duplicates while accepting new pairs with distinct high player/target bits. An injected owned allocator tests lazy construction, failure after earlier entries, permanent fail-closed behavior and no request entry; a separately thrown native allocation exception must propagate. It never executes original game code or uses a game-process handle.

## Remaining gameplay acceptance

Parent integration found that Blackstar already owns the five-byte entry at
`3885b0`. Auto-loot now pins the unchanged 32-byte getter body at `3885b5`
instead, retaining its full original EXE hash. Blackstar's replacement row is
restricted to its summon context and native return `9dd69f`; Auto-loot receives
the original row. The CharacterInfo binding likewise remains the normal getter
`389570`, while its pin starts at unchanged body `389575` after Blackstar's
five-byte resolver hook. Blackstar supplies the original row for Auto-loot's
caller, outside its authorized summon context. The combined coexistence check covers both modules and rejects
guards that overlap another packaged hook. The parent independently rebuilt
the ASI and passed the owned fixture and combined coexistence test, including
all 104 runtime pins. The field-manager and component-count fixes above were also reviewed
against the original caller/constructor/destructor instructions.

Gameplay validation requires a separately authorized next normal game startup. Verify ordinary animated and NoAction free ground items and normal `Dead_Loot`/`Simple`/`Swim` corpses around the selected player, including targets behind the camera and several nearby drops without manual focus. Confirm native range/obstruction limits, action completion and persisted inventory quantities; full-bag, ownership and quest refusals; unowned animal corpses and relationship-exempt enemies; refusal of owned/criminal NPC bodies and custom crime predicates; object despawn; selected-player changes; no purchases/theft/dialog acceptance; no repeated requests; and coexistence with the other locally built modules. Native plain-record restrictions and the conservative crime gate can exclude valid custom or ambiguous corpses. The same native actor identity is never requested twice in a process, including a later change of interaction family or replenished loot; this is intentional duplicate protection. Memory use grows with distinct attempted pairs and an owned allocation failure stops automation for that process. No in-game success is claimed by the owned tests.
