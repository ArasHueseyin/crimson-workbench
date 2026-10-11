# Repair 0.9.0 – capture from held references

Historical status. Own 2949 proofs of the reading fields still referred to here as legacy fixtures are now supplemented in [Modul 0.10.0](INVENTORY_2949.md).

Status: 22.09.2026. Development module, still **no installable mod**. Desktop app v0.5.9 and its locked repair option remain unchanged.

## Removed gap

The previous protected reader acquired both Actor references and owner locks, but subsequently called the general diagnostic reader. This searched the server registry again without their own lock. An actor lock protects neither registry node nor another actor registered there.

`capture_with_locks` now only captures the already held pair. A valid full identifier is mandatory; Alive/destructive state shall be checked before and after detection. The client selection must correspond to the held client. Another selection is rejected before reading their character data. The client anchor and all read item data remain part of the control read; A recognized change discards the entire output.

The general `capture` path remains for diagnostic/offline fixtures and is expressly not a live source of income. Both paths use the same item, UID, position, definition, and mirror check. No reading operation changes durability or triggers game events.

## Common native test

The sample from [0.8.0](REGISTRY_NATIVE_INTEGRATION.md) is extended with the full WindowsRWLock-Try function of the build 1.0.0.2949:

|Properties|Evidence|
|---|---|
|RVA/Size|`0x1371c30` / 174 Bytes|
|SHA-256|`f928b8b10862b8348823b946709b6d75d7eed1f2a6665350a0cfa8d9cecbf606`|
|pdata / Unwind|Exact limit up to `0x1371cde`; Header `11140800`|
|TLS read access|Only nine bytes at `0x1371c5f` redirected to private TLS vector|
|Dependencies|Real Windows TryAcquireSRW/ReleaseSRW features and GetCurrentThreadId; Exclusively own barrier objects|

In total, 16 code ranges, ten pdata boundaries and five TLS reading locations are now pinned. The previous original reference methods and releases remain part of the same sample. Neither game code is loaded into an ongoing process nor is its memory read. The full source file hash is checked before and after; Copies of code are on private RX pages, data on separate pages.

18 additional scenarios combine protected registry search, real actor references, actual non-waiting owner locks, readers, and repair planning:

- Normal/User actuators and direct/registered reference mode.
- Both owners have valid references and real exclusive
Windows locks; Control reading also fulfils this condition.
- A competing reader each blocks an owner lock. The registration
reports `lock_busy`, does not read items and returns already acquired funds.
- Lack of second possession, old generation identifier, death and state of destruction.
- Other/missing client selection and change of selection during control reading.
- Invalid base pointer, different item UID and impermissible durability.
- A registry entry replaced after reference acquisition is not read again.
The held object remains alive in this fixture; This is not proof that a logged-off object can still be changed.
- blocks are released before the references; No remaining property and no
Blocked Windows lock after success or error. Source data remains byte-identical.

**Important limit:**Inventory, equipment and client anchor layout of this combined sample are private fixtures of the documented 2944 reader. The native purchase and blocking page comes from 2949. The joint success proves the interaction of the components, **no live release of the inventory layout for 2949**.

## Results

- Six CTest suites, MSVC release with `/W4 /WX` passed.
- 362 Synthetic reader/reference conditions; of which expressly illegible
Registry areas and unobsessed replacement objects on which no read attempt is made.
- 48 native scenarios, 503 counted native calls, 3,867 test conditions.
The original 30 registry/reference cases are included.
- 1,481 Action, 25 Lock Group and 55 Source adapter conditions remain.
- The test without EXE checks 48 manifest conditions, but does not lead to native
Game code from. The native sample still requires the express EXE path.

Reproduction:

```powershell
./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

Evidence: `.local/repair-runtime-v9-build-test.log`, `-native-result.json`, `-reader-result.json`, `-validation.json` with the same prefix.

## Still open

Manager/context lifetime, actual current player selection, engine thread and TLS mode must be backed up by the game host. References on Actors do not automatically keep their managers alive. Comparison reading does not detect an unnoticed A→B→A change; Every charge/world change needs a real host signal and a new era, even with reused addresses and identifiers.

The MIT reference Trinity’s researched motion hook describes even its player assignment as a practical observation without a complete static chain. It was therefore not adopted as a proven dispatch point. Your own repair must not only run on any movement or render thread.

New inventory/event proofs, complete change transaction and confirmed effects, input, loader and B0 installation are still missing. The engine threadmap, parent references and final cleanup are still test dependencies. Manuals in-game tests follow as agreed only after the development of all phases; The cases are in the [Testcheckliste](../../TESTCHECKLISTE.md).
