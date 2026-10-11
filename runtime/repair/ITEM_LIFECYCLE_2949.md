# Repair 0.12.0 – native items copies for equipment events

historical stage of development; supplemented by [Slot marking and held capture 0.13.0](DIRTY_SLOTS_2949.md).

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. Full EXE hash: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. further development module; App v0.5.9 and locked repair option unchanged.

## Newly implemented

`crimson_repair_item.lib` has native item values above `item::Value`. Construction, assignment and release are firmly bound for each object. Copying and moving the owner is excluded. Access and operations on other threads are rejected; a destruction on the wrong thread ends the process according to the same contract as the existing reference management.

Prior to assignment, zero/error pointers, overlap, alignment and the three vector headers are checked. An invalid header must not first delete the existing target value. In the event of an unexpected native return, the value becomes unusable; It can only be redesigned after release. Clean-up is guaranteed as far as the bound native functions fulfill their documented contract.

The binding must later come from a verified game module loaded during use. The host must run on the correct engine thread with valid TLS and protect the source value along with all nested data. The library does not grant these releases. `action::Image` is a separate image and must not be transferred as a native source with its stored pointerbytes.

## Documented copy layout

The actual assignment is not a complete `memcpy`: it deletes previous optional data and copies relevant fields and nested memory areas via its own helpers. Paddingbytes are partially not copied.

|Area|Header/Size|Behaviour|
|---|---|---|
|Base data|Pointer `+0x60`, number `+0x68`, memory capacity `+0x6c`; 6 Bytes per entry|own allocation; Number and contents are copied|
|Logical base boundary|Byte `+0x70`|Separate from storage capacity; It is also copied|
|Additional list|Pointer `+0x78`, number `+0x80`, capacity `+0x84`; Stride 16|13 occupied content bytes per entry, remaining bytes padding|
|Further list|Pointer `+0xa8`, number `+0xb0`, capacity `+0xb4`; Stride 6|Own allocation and content copy|
|Optional block|Pointer `+0xb8`, 24 Bytes|Recopied, previous block released|
|Optional block|Pointer `+0xc0`, 16 Bytes|Recopied, previous block released|

The technical significance of the two additional lists/blocks is not derived from their copy size. The new capacity check shall be part of the native copy release; The existing reader has not been changed in this step.

Native entries: constructor `0x2409750`, assignment `0x240b020` → `0xf300c80`, destructor `0x240af10` → `0xf2fe4a0`, empty `0x240b430` → `0xf312630`. All 17 new code ranges, including the full list/allocation helpers, are pinned with sizes and hashes in `tests/item_functions.inl`.

## Integration and testing

The 15 equipment event tests now use this owner with original construction, deep assignment, and destruction. The subsequent copy is still produced under the owner's locks. Once approved, the original server notifier receives the independent copy. Only after all callbacks will it be released; Actor references are maintained until then.

14 additional native copy cases check both TLS allocation paths: simple items, sockets, all additional data ranges, re-copying with larger numbers, replacement with empty data, self-assignment and invalid source capacity. The source and source memory remain unchanged. Own Windows heap with protection marks checks allocation limits, independent pointers and fully balanced releases.

**Seven CTest suites, MSVC release `/W4 /WX`, 30 new owner test conditions and 117 native scenarios with 851 counted calls and 13,639 Conditions passed.** The remaining suites: 1,537 Action, 385 Reader, 25 Lockgroup and 55 Source conditions. The test without EXE only checks 270 manifest conditions.

Total 45 code ranges, 49 pdata fragments and 24 private TLS redirects. In the case of a shifted list helper, the Unwind chain points to an older byteidentical prolog record. Only this pinned alias is normalized to the registered record; Even nested chains are explicitly tested. Original C++ cleanup handlers will still not be registered. Allocation errors and exceptions in foreign functions are thus not proven to be controllable.

## Still open

The native item copy is now implemented and tested in isolation. It still does not replace a complete repair commit: game host/manager/thread binding, actual item change, slot invalidation, inventory events, real effect and transport processing, order assignment, input, loader and B0 are missing. The private heap is not a proven engine allocator. Slot invalidation has been further statically confined, but not yet implemented or executed.

No game process/save accesses or installation changes. Reproduction: `./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"`. Evidence: `.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`. [Phasenstatus](../../PHASENSTATUS.md), [Subsequent tests R01–R11](../../TESTCHECKLISTE.md).
