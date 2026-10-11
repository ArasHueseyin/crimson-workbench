# Repair 0.14.0 – complete writing step for durability fields

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE hash: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **Development module, not yet an installable repair action.** App v0.5.9 remains unchanged and the repair option locked.

## Implemented writing step

`crimson_repair_writer.lib` contains `writer::Batch` and the local memory access `writer::LocalAccess`. A batch takes an immutable repair plan and a still held `reader::HeldCapture`. It can process items carried and equipped individually, separately or together, each main item and socket in both Zustandcopies.

Addresses are derived exclusively from a fresh recording of the same held owners. A detached `Capture` or a purely reading `Memory` adapter does not grant write permission. The write adapter must be the same object that reads the sources for the held capture.

The preliminary check captures **the entire batch before the first write**:

1. Re-check current frame, selection, full identifiers and owners.
2. All pre-images, definitions, identities and positions with the plan
compare; Subsequent image initially calculated exclusively privately.
3. Actual memory areas, alignment, number of bases, logical limit and
Check allocation capacity. Additional vectors and optional data remain protected.
4. Overlaps with other items, sockets, empty/filtered slots, read
Reject structures, owner/block data and additional item data.
5. Check the spelling of all changed words and then the entire
Compare the reading stock together with addresses and source bytes again.

Only then does the adapter write only actually changed 16 bit values: main item `+0x40`, base dataset `+2`. Unchanged and unused values are not written. The actual loop does not create new C++ buffers. Each store also compares its previous value. A complete check-read then checks the expected postage image and the entire original reading stock with exactly these allowed changes, including empty slots and structural metadata.

`LocalAccess` works only in the calling process. There is no PID search, remote access or modification of site rights. Write-protected, executable, guard and inaccessible pages are rejected. OS memory errors are handled only in the limited own read/write frame.

## Income and Life Contract

- `rejected` from the preliminary check means: This call did not write anything.
- `fields_written` only confirms the checked field values. The
Owner locks and references are maintained for the following phases.
- Once a store has been attempted, its error will be `outcome_unknown`, too
at the first word. There is no blind withdrawal or automatic repetition. An old precedent is then no longer output as valid over `view()`.
- Each capture held allows only one write attempt. Subsequent control reading
does not reset this limit. Incorrect thread and shared owners do not allow storage operation. `already_attempted` is not proof that a previous call did not change anything.

These results may not be displayed lump-sum on `action::Commit::applied`. The full engine adapter must prepare native post-copies and event paths before the change, then execute slot markings and all inventory/equipment events, and check unequivocal feedback. An error after field changes must lead to the unclear overall order.

## Tests carried out

**Eight CTest Suites**, MSVC Release `/W4 /WX`, passed. The new writing suite contains **34 Scenarios and 197 Conditions**:

- All ranges/individuals, active no wear rule, sentinel, pure base change,
unchanged fields and empty orders.
- Invalid base/additional capacity, common item/base buffers, base within
an item or actor header and overlap with additional data.
- A later obsolete item, indescribable last field, modified session,
other player selection, invalid plan and a read-only adapter.
- Change in value, move a byte-like item list and change an empty slot
during the writeability test; Everything was rejected before the first store.
- Error before/after an actual store, partial cancellation, faked successful
Stores, unexpected change of an empty slot and change of choice when writing.
- foreign thread, repetition and closed ownership phase; Really own Windows-
Pages with read-only/executable protection, odd addresses and border overflow.
- Maximum implementation scope: 2,048 existing items and 4,096 changed
Words in both Zustandcopies. This is not a promised inventory/engine limit.

**Eight additional native integration cases** connect the production recorder to the original reference/lock functions of the current build and actual private memory addresses. Normal and registered reference mode, both Zustandcopies, all ranges and an independently created byte comparison exist. The native owner locks remain uninterrupted until after the control reading. Native and synthetic reading samples use the same source evidence.

Current overall status: **141 native scenarios, 1,408 counted calls and 120,069 conditions**. Furthermore, 51 pinned code ranges, 57 pdata fragments and 26 private TLS readers. The large condition number comes mainly from repeated checking of the slot table from 0.13.0, not from as many independent cases.

## Remaining integration

The new field recorder is not yet connected to the full sequence of events for an engine transaction. The previous 15 equipment event cases retain their separate private field setup. Post-copies must be fully prepared before the first source writing. For items carried on board, the event paths are still missing, for slot markings their consumers and native cleaning. True effect/transport/UI processing and order assignment remain open.

Also missing is confirmed player/manager/engine thread/world access, input, loader and B0 installation. The host must continue to secure context/catalog lifetime, world transitions and TLS; the write adapter does not itself generate these prerequisites. No write execution in the game process, no save accesses or changes to the game installation. App/Rust/UI code is unchanged and has not been retested for this C++ change.

Evidence: `.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`. [Source code](src/writer.cpp), [Windows access](src/writer_win.cpp), [Test cases](tests/writer_tests.cpp), [native Integration](tests/writer_native.inl). Predecessor: [Slot marking 0.13.0](DIRTY_SLOTS_2949.md). Manual in-game validation remains postponed to later as agreed: [TESTCHECKLISTE](../../TESTCHECKLISTE.md), in particular R01–R12.
