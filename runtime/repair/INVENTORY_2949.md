# Repair 0.10.0 – Inventory and equipment accesses for build 2949

Historical layout proof. Additional event testing: [Development module 0.11.0](EQUIPMENT_EVENTS_2949.md).

Status: 22.09.2026. Steam 25455892 / EXE 1.0.0.2949, full SHA-256: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. Further development module without game host, loader or installable repair. The desktop app remains v0.5.9, the repair option locked.

## Newly implemented and occupied

The reader now explicitly requests a known build in `Frame`. Unset and unknown selectors are rejected before reference acquisition, blocking or memory reading. The host must check the full EXE and tables itself; Setting an enum does not replace build approval.

The field structure is tested separately for 2949. The known global addresses and field offsets remain the same; Multiple function addresses and hashes change. No blanket shift or takeover of old function bytes was used.

|Function/Observation|RVA|Umfang|
|---|---|---|
|Inventarslot|`0x212f4b0`|226 Bytes, four completely obtained pdata/unwind fragments|
|Inventory owners|`0x212a100`|127 Bytes; direct, diverted and missing owners|
|Current client selection|`0x8b4480`|140 Bytes; own native reference document, no registry lookup|
|Durability updater|`0x240d660`|405 Bytes; Item and base panels|
|Owner's blocking document|`0x393650`|104 Bytes; Lock reference and acquisition|
|Change of equipment|`0x20c90f0`|840 Bytes; Slot selection and only the branch checked with delta 0|
|Unlimited durability test|`0x240d4c0` → `0xf31ffd0`|Original 5 byte jump and complete 297 byte target|
|Two client anchor callers|`0x39b670`, `0x39ec90`|whole functions static hashed; Same global/manager path|
|Server/equipment callers|`0x2a94c50`|whole function static hashed; Server Manager and Equipment Path|

The complete functional hashes are in the test manifest and in `tests/inventory_native.inl`. The three anchor callers are only checked statically. The remaining selected functions are executed as private copies. Overall, the manifest comprises 24 code ranges and 20 pdata fragments; six TLS readers are redirected to private data. The original WindowsRWLock Acquire feature is now also running in the exclusive branch.

## Confirmed fields

- Clientglobal `0x6d691b0` → Kontext `+0x30` → Manager `+0x50`.
Two independent callers agree. The current selection acquires a reference document via the actor slot `+0xc0`; Pointers without validity bytes are not enough.
- Serverglobal `0x6d696c0` → Kontext `+0x48`; Actor `+0x68` → Equipment `+0x38`.
Manager/Actor identifier and reference layout are already tested separately natively.
- Holdergetter: actor `+0x68`, components `+0xb8`; Per character definition
Direct use or redirection via Actor `+0xa0` → Possessor `+0xd0`. The reader reference check remains deliberately conservative.
- Holder `+0x18/+0x20` → Containerarray; Containertyp `+0x10`, signed slot boundary
`+0x0c`, Itemarray `+0`, Itemstride `0xc8`; Exclusion list `+0x20/+0x28` with 12 byte entries. Key `+8` and positive set `+0x10` identify existing items.
- Equipment `+0x90` → Table with array `+8` and number `+0x10`;
Entries `0xd0` Bytes, Slottag `+0xc8`. The native path actually accesses the second matching slot in the test; Definition queries log its address.
- Main durability `+0x40`; Base pointer `+0x60`, number `+0x68`, capacity
`+0x70`; Base stride 6 bytes, durability `+2`.

## Automated proof

**Six CTest suites, MSVC release `/W4 /WX`, 385 reader conditions and 88 native scenarios with 578 counted calls and 6,219 test conditions passed.**

In addition to the 48 previous occupation/recording cases:

- 21 Inventory/owner cases: negative and too large slots, wrong container type,
empty/used items, second filter line, direct and redirected owners. The own reader and the original Getter agree after Presence normalization. A deliberately inaccessible slot in your own process checks the complete native Unwind chain and the clean rejection in the reader.
- Eight cases of current client selection: normal/user variants, direct and
Registered reference mode, missing selection, Zustandmasks and Alive-Zustand. Direct native ownership can accept Alive=0; the reader further separately rejects repair registration for such owners.
- Eight equipment cases: actual selection of the second slot day, missing
Slot, empty table, empty/used item, unlimited item, base and various durability. Original lock record and Acquire/Release hold a real Windows lock and return reference number and recursion completely.
- Three update cases: Delta 0 and Sentinel remain stable. **Positive delta 65
increases the main durability 35 → 100, but sets the damaged base 5 to 0.** This original path therefore remains unsuitable for our own base repair.
- The successful combined recordings are additionally combined with the current
Native Holder/Slotgetters on exactly the same private sources.

The action, block group and source adapter suites still exist with 1,481, 25 and 55 conditions. Without EXE, the native host only executes 72 manifest conditions; No native success is derived from this.

## Boundaries and the next step

The reading fields, which were previously only classified as legacy fixtures, now have their own 2949 documents. **This is not yet a fully connected game host.** Manager life, current engine thread/TLS mode, world change and real session eras are missing. The current client selection function does not synchronize its manager pointer itself; it is not a substitute for these hosting obligations.

The equipment sample only performs delta 0 or early rejected items. The empty temporary item construction/destruction and table resolvers are expressly fixture dependencies. Non-zero delta change, item copy, base removal, events and derived effects are thus proven **not**. Threadmap and final actor cleanup also remain test dependencies. No game/save accesses or installation changes were made.

The next integration step must prove its own repair writing path along with inventory/equipment events and tie it to a proven game sequence. Simply overwriting original fields twice is not enough for this.

Reproduction:

```powershell
./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

Evidence: `.local/repair-runtime-v10-build-test.log`, `-native-result.json`, `-reader-result.json`, `-validation.json` under the same prefix. Manual tests remain deferred until all phases have been developed: [Testcheckliste](../../TESTCHECKLISTE.md), [Phasenstatus](../../PHASENSTATUS.md).
