# Protected reference purchase on Build 2949 – Module 0.8.0

Historical status of this document. The extension with real owner locks and direct capture of held objects is documented in [Modul 0.9.0](PINNED_CAPTURE_INTEGRATION.md).

Status: 22.09.2026. New isolated proof of execution for Steam 25455892 / EXE 1.0.0.2949. **Further no installable repair mod.** The Workbench remains v0.5.9; The repair remains locked in the app.

## Joint pathway

`repair-registry-native.exe` combines the existing `registry::Source` adapter and `reference::Pair` with copies of the current client/server lookups, the concrete normal/user-actor reference methods and the PaPtr release. All managers, actors, buckets, nodes and receipts belong exclusively to the own test process.

The registry lookups will no longer be replaced by artificial purchase callbacks. The test performs both protected searches with full identifier. Your virtual reference calls are measured and forwarded to the unchanged native normal/user wrappers. These select different parameters: NormalActor uses the Zustandsmaske `0x10`, UserActor uses the type `4`. The appropriate reference methods and the actual Actor release run together.

The original WindowsRWLock Acquire/Release implementation uses real Windows SRW imports on private lock objects. The registry requests read locks. In the test, a write attempt must fail during the acquisition of reference and then be possible again. Four additional thread cases keep the registry exclusive: The lookup is waiting; a distance during this lock is then observed. For the second missing owner, the first reference is returned.

## Build and execution limits

The file reader has two explicitly separate build selectors. The old host continues to use only the original hash and rejects the new EXE. Only the new registry test selects 2949. Unknown EXEs remain blocked.

15 full code ranges are pinned in addition to the full EXE hash. Nine of them have exactly tested pdata boundaries and receive their original Stack Unwind codes. Known language-specific cleanup handlers are removed only in the private test host; The tested dependencies do not throw. The short Leaf/Tail Wrapper and the Type Helper including the jump table have separately tested sizes. The specific Actor release slots are controlled on five VTables.

Four nine-byte long GS reading accesses of the reference methods are redirected to a private TLS vector by register- and flag-maintaining LEA instructions. All other function bytes remain unchanged. The registry uses `shared=true`; the exclusive TLS branch of the lock-acquire function is not executed. Host-TEB/TLS is not changed. code is on private RX pages, data on read-only pages; No page can be written and executed at the same time.

## Passed tests

- Six CTest suites and MSVC release with `/W4 /WX`.
- **30 native scenarios, 263 native calls, 3,490 test conditions.**
- normal/user variants; both direct reference modes and registered mode;
full identifier including generation bits; valid high type bits.
- missing first/second registry entry, empty registries, incorrect node keys,
Inappropriate actor type, lost Zustandsmaske, different user/normal masks.
- A failed native reference acquisition may include a pointer in the invalid
Keep the document. This does not give rise to possession; The release does not de-reference it.
- sub-error, precise source allocation and reverse release order;
nested possessions remain. Registry entries are not changed.
- Alive=0 is actually accepted in the direct native path. This possession
is not a repair clearance; The separate inventory reader additionally checks Alive/Destruction under the owner locks. Registered acquisition rejects Alive=0.
- Last reference and delayed server-side cleanup: Zustandchange
`0x20 → 0x40`, release of the embedded lock before cleanup/destruction, exactly one destructor dispatch. Test objects remain allocated for error detection.
- Actual competing registry write locks for client and server,
each with unchanged or meanwhile removed entry.
- Old Host rejects new game EXE; new host rejects another own PE test file
before native execution. The old release will not be extended.

The previous standalone suites remain successful: 1,481 Action Conditions, 249 Reader/Reference Conditions, 25 Lock Group Conditions, 55 Adapter Conditions. The 45 manifest conditions of the new host also run without game EXE; They are not a native proof of success. The 3,490 conditions come from the separate call with exactly pinned EXE.

## Still to be implemented

The managers will continue to be provided in the test. Current player, secured manager lifetime as well as suitable engine thread and its TLS mode must be resolved in the real host. The inventory reader and other native change/event methods require their new layout/build proofs.

Thread reference map, higher-level references and final cleanup/destruction are clearly limited fixture dependencies. This sample does not claim full engine lifetime management. The complete repair transaction, event allocation, input, loader and B0 installation are still missing. No lost or destroyed items are created.

## Reproduced

```powershell
# Run all synthetic/manifest tests; does not read a game EXE.
./runtime/repair/test.ps1

# Also run the native registry/reference test on private test objects.
./runtime/repair/test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

`-GameExe` unchanged means the historical 2944 host. For the new registry sample use only `-RegistryGameExe`. Evidence: `.local/repair-runtime-v8-build-test.log`, `.local/repair-runtime-v8-native-result.json`, `.local/repair-runtime-v8-build-refusals.json`, `.local/repair-runtime-v8-validation.json`.

No changes to the installation or saves, no access to the ongoing game process. [Open play decreases](../../TESTCHECKLISTE.md).
