# Repair 0.15.0 – prepared copies and common process

Status: 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE hash: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **Development module; the repair is still not usable in the app.** Workbench v0.5.9, game installation and saves remain unchanged.

## Implemented

`crimson_repair_transaction.lib` combines the existing action interface with capture, native item copies, field recorders and area-dependent messages. The host must already have secured build, engine thread, TLS, manager/catalog lifetime, and session. The new adapter does not make this game connection.

The common procedure is as follows:

1. Acquire both owner references and locks; This is the repair plan.
2. Allow the entire plan and reporting channels of all affected areas.
A lack of inventory path also completely discards a mixed order.
3. Read again; Additional vectors and optional item data before native copying
Check for readable initialized areas and a common copy budget. The budget of 16 MiB is an implementation limit, not an engine maximum.
4. Completely prepare both native aftercopies for each affected item.
`item::Value::prepare_repair` checks prior data, uses the native deep assignment and only changes durability values of your own copy.
5. Only then execute and check the complete field step.
6. Mark all changes under the locks. Then release both locks,
while owner references and prepared copies remain valid.
7. Send messages synchronously, destroy copies, return references.
The result remains `pending` until new read data and associated feedback together confirm the job.

Errors before a write attempt result `rejected`; Errors from the first write attempt result in `unknown`. There is no repetition or automatic withdrawal. A foreign thread may not use native methods or ownership. The message adapters shall not hold borrowed pointers beyond their call.

## Two corrected marginal cases

The number of logical base locations (`+0x70`) may be greater than the stored number (`+0x68`) and allocation capacity (`+0x6C`). Original copy/access functions handle missing records separately. Readers, plan and writers are now processing exactly the stored data sets. Empty vectors with available base spaces do not require a buffer; Uninitialized capacitance residues are not read. This is tested on the basis of the already pinned functions and new native cases.

The additional vector strides in the overlap test were reversed: `+0x78` has 16-byte data sets, `+0xA8` has 6-byte data sets. Corrected and secured by a targeted case whose overlap lies only in the rear part of the 16 byte data set. Previous module reports are historical stands.

## Tests carried out

**Nine CTest suites**, MSVC release with `/W4 /WX`, passed:

|Test test|Umfang|
|---|---|
|Action|1,529 Conditions|
|Readers / Blocking Group / Registry|519 / 25 / 55 Conditions|
|Item owners|29 Conditions|
|Field recorders|35 Scenarios, 202 Conditions|
|Common procedure|40 Scenarios, 247 Conditions|
|Total Native Sample|159 Scenarios, 2,023 Counted calls, 119,953 Conditions|

The new expiration check includes both ranges and Zustandcopies, single item, no-wear, empty and partially stored sockets, missing message paths, later copy errors, incorrect copy results, invalid additional data, budget overrun, write-offs, marker/message errors, session change, foreign thread, repetition and independent final check. Before the first store, all copies must be available; Messages only occur after the release of both locks.

The **15 native equipment event cases** now use this common adapter and the actual field recorder. The former manual field setup is removed. Original Item Copies, References, Windows Locks, Slot Marking, Server Notifier and Client Ack converge on private objects of their own. The confirmation then acquires new owner references and reads again. Intentionally modified slots or Conflicting Zustandcopies are already discarded by the reader and cannot successfully complete an order.

**32 native item cases** additionally check empty/partially stored sockets and prepared subsequent copies of both Zustand pages with both TLS allocation paths. No source change or third-party release; My own test heap is completely cleaned up. This leaves 51 pinned code ranges, 57 pdata fragments and 26 private TLS locations. The condition number is not a case number; It contains many repeated table checks. Compared to 0.14.0, there are no comparisons of unstored sockets and double checks from the replaced manual event setup. The smaller sums are not proof of less covered error cases.

## Still open

Concrete inventory events for items carried, consumers and native cleaning of slot messages, real effect/transport/UI processing and clear assignment of actual feedback to the repair order are missing. These recipients remain artificial test dependencies. Also missing game host, input, loader and B0-installation. There is no confirmed live engine commit.

The tests only perform functional copies in their own test process. No access to the ongoing game process, no game/savewrite operations. Unchanged Rust/UI parts were not retested. Manual decrease later, especially [R01–R13](../../TESTCHECKLISTE.md).

Evidence: `.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`. [Adapter](src/transaction.cpp), [Interface](include/repair_transaction.h), [Kopien](src/item.cpp), [Sequence tests](tests/transaction_tests.cpp), [native equipment sample](tests/equipment_events_native.inl), [native Itemproben](tests/item_native.inl). Predecessor: [Field recorder 0.14.0](FIELD_WRITER_2949.md).
