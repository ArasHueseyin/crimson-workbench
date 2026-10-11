# Actor references and lifetime 0.6.0

Status: 22.09.2026. Separate development module; Workbench continues to v0.5.8. No installable mod, no access to the game process or saves.

## Implemented

`crimson_repair_reference.lib` manages two native 32 byte reference documents (`PaPtr`) as non-copiable/movable owners. A host must deliver them via verified Acquire/Release Callbacks. An invalid reference can contain a non-empty actor pointer: The validity byte `+0x10` is decisive. The sources shall have the same full character identifier; The same actor addresses for client and server are rejected. All sources are checked in advance.

If the second source fails, its result and the first reference are cleaned up in reverse order. Public calls on a foreign thread are rejected. Destroying a still holding owner on a foreign thread is a programming error and leads to `std::terminate`. In addition, the host must keep the TLS reference mode unchanged throughout ownership; The same thread ID alone does not prove this.

`capture_with_references` holds both references over full protected inventory capture. The inner reader returns the inventory locks first, after which the references are released; This also applies to rejection and C++ exception. Under the locks, the full identifier, `Alive+0x4a` and the destruction indicator `+0x4b` are checked before and after the acquisition. The result contains private data copies and no persistent write authorization.

## Native findings and functional limits

Pinned EXE 1.0.0.2944 / Steam 25381195, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. All addresses as RVA:

|Function|Bytes|SHA-256|
|---|---:|---|
|NormalActor access wrapper `0x4f9000`|12|`90521d0c1b64f081e463c6d47ce1ce830aadf6a417b7607269319860382e5eb6`|
|Identifier-related acquisition `0x1436bf0`|246|`30eec32cccb2b3447885935521b2c92e9c951f9a7d6fb729b09f81833beca0d9`|
|Check existing reference `0x1436990`|152|`17015ad6da39a35b7248364f8e9622fe0d6e221284e201acb38a08f3abc0386b`|
|Concrete actor release `0x1436690`|757|`0a7467851808949a1dddfbbda6f4d1270cefe1262ce80c796da571a171a7b8ad`|
|PaPtr release `0x1434900`|156|`58962428c45832afd42ef53dbda8feeef6d8bebb53c709f1d79c683bbdc49860`|

The checked Common-/Client-/Server-Actor-VTables point at `+8` to `0x1436690`. The simpler base path `0x14345c0` is **no replacement**: The specific path can also move Zustand `0x20` to `0x40` and call up a pending cleanup for server objects. The PaPtr destructor calls the virtual release and protects the final destruction with an atomic comparison of `Actor+0x4b`. The validity byte is then deleted. The separate PaPtr mode `+0x11` has its own release branch.

Depending on the TLS flag `+0x1d2`/`+0x1d4`, the purchase takes place via the direct reference counter `Actor+0x10` or via thread registration plus `Actor+0x48` under the embedded lock `Actor+0x18`. The direct path checks **not** the Alive field. The additional rejection in the reader is therefore necessary. NormalActor and UserActor purchase use various virtual wrappers; The tested purchase wrapper is expressly the NormalActor path.

## Isolated sample and boundaries

The five functions above are copied from the hash-checked file to private test process memory. Three nine-byte TLS read accesses are redirected to a private test TLS vector via register-maintaining `lea`. All other function bytes remain unchanged. No real TEB/TLS change. Game cleanup handlers are only removed from the private Unwind copies; Exceptions from native game dependencies are thus not claimed as tested.

13Scenarios check full identifiers, rejected access, existing external references, both direct TLS modes, thread registration branch, logged-out objects, delayed destruction, and the additional Zustandclean-up. The special document and re-release after disability are also checked. An invalid document with an address 1 confirms that there is no pointer access.

**Test dependencies:** Thread registration/lookup, embedded blocking methods, parent object resolution, re-purchase upon cleanup, cleanup and final destruction are expressly fixture callbacks. The tests prove the native decision paths and balanced calls, not the full engine threadmap, actual object destruction or real game events. The separate WindowsRWLock sample from 0.5.0 continues to use real Windows SRW imports.

## Supplement 0.7.0

The source adapter for manager lookup and appropriate release method has now been implemented and synthetically tested. The newly installed EXE 1.0.0.2949 prevents further detection with the previously pinned host on 1.0.0.2944. [Registry adapter and build change](REGISTRY_INTEGRATION.md).

## What is missing before use in the game

The host must purchase current client/server references **from protected sources**. Handing over an unprotected actor address to the Acquire Callback would be unsafe even before the reference purchase. This live source resolver, concrete method binding, initialized engine thread and constant TLS modes are not yet implemented. The existing reading layout resolver does not replace a registry lifetime lock. The C++ callbacks are not a finished game host.

After that, complete repair transaction, game events and order assignment, input, loader and B0 installation are still missing. The reference library is not connected to the desktop app; `installable` remains `false`.

## Test result

Four CTest suites and MSVC release `/W4 /WX` passed. 249 Reader/Reference Conditions, 25 Lock Group Conditions, 1,481 Action Conditions. Native host total 203 views and 3,859 conditions. The reader suite checks, among other things, share order, both failed acquisitions, blocking conflict, false/old identifier, incipient destruction during capture, thread change and exceptions. Evidence: `.local/repair-runtime-v6-build-test.log` and `.local/repair-runtime-v6-validation.json`.
