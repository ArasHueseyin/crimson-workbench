# Owner lock and protected capture 0.5.0

Historical status. The direct capture of held owners and the common sample with the lock functions of the new build are in [Modul 0.9.0](PINNED_CAPTURE_INTEGRATION.md).

Status: 22.09.2026. Separate development module; Not an installable mod. The Workbench remains v0.5.8. All execution samples use their own test objects.

## Implemented

`crimson_repair_lease.lib` combines up to two locks bound by the host. It checks all bindings before the first call, sorts by object address and tries exclusive access without waiting. If the second lock is occupied, the first lock is released immediately. Identical bonds are held once; Contradictory ties of the same object are rejected. Release takes place in the reverse order and receives an already existing external recursion.

The group is neither copyable nor displaceable and belongs to its creation thread. Public calls on another thread will be rejected. Destroying a still held group on a foreign thread is a programming error and terminates the host with `std::terminate`; Foreign Windows locks must not be released there.

`capture_with_locks` combines this with the existing inventory reader. The host already delivers client/server owners protected against release and tested method bindings. The reader compares their blocking pointers to `Actor+8` before and under the blocks, records both inventories together with control reading and checks the currently dissolved owners again. Mistakes discard the private outcome. When you jump back, both locks are free: the result is a data copy, not a continuing write permission.

## Proof of pinned build

Steam 25381195, EXE 1.0.0.2944, SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`. Addresses are RVAs. `WindowsRWLockBase`-VTable `0x5719fe0` and `WindowsRWLock<2,0>`-VTable `0x55856e8`: Release at `+0x20`, Try at `+0x28`. `false` selects exclusive access, `true` shared read access. The native structure contains SRWLOCK at `+0x10`, owner thread at `+0x28`, recursion at `+0x2c`. A `NullRWLock` bond would not be proof of mutual exclusion.

|Function|Bytes|SHA-256|
|---|---:|---|
|Try `0x1371c20`|174|`45f3b9499ce28442b126fd0941aa5711d1cc1a58e19a7d3893330d83ac430c90`|
|Release `0x1371bf0`|40|`b43c0aebed083eebd4ce7e824ed7acb44bec3f81d92a8ab52cd1be3981a5740a`|

In the private try function image, only the nine-byte TLS read access at `0x1371c4f` is redirected to private data per test thread. All remaining try-bytes and the full release code remain byte-identical. The test host does not change TEB or real engine TLS data. The original import calls use real Windows SRW features and `GetCurrentThreadId` on their own lock objects. Unwind registration removes the unusable game cleanup handler of the private copy. This is not an exceptional check of a fully loaded game module.

Tested are exclusive access with actual Windows countertest, recursion, a competing thread with revocation of the first lock, and an existing reader who rejects the writing attempt. Older registry/ack samples continue to use their documented lock stubs.

## Supplement since 0.6.0

The reference management and its integration into the reader have now been developed. Protected live sources and specific host bindings are still missing. [Current reference agreement](REFERENCE_INTEGRATION.md).

## Limit of the original level 0.5.0

A held inventory lock does not replace a valid actor reference. The separate actor access path `0x1436bf0` checks the full identifier and uses direct reference counters or a thread registration depending on the engine TLS. In the normal path, the embedded lock `Actor+0x18`, Zustandsflags, Alive field and reference registration are added. This lifetime management is not yet integrated into the host. A pointer comparison or self-invented counter is not enough.

The future host must itself occupy lifetime, current method tables, initialized engine thread, session and owner suitability. It also lacks the complete change path including game events, their assignment to the job, operator input, loader and B0 installation. The libraries do not provide a live resolver for lock bindings and do not change items in the game.

## Test test

Four CTest suites, MSVC Release `/W4 /WX`: passed. 1,481 Action Conditions, 163 Reader Conditions, 25 Group Conditions; native host total 140 views and 2,408 conditions. Reader samples prevent item access without both locks and also check for changed owner anchors, false bindings and errors when captured. Evidence: `.local/repair-runtime-v5-build-test.log` and `.local/repair-runtime-v5-validation.json`. No game process/save access.
