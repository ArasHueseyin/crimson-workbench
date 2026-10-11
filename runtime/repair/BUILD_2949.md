# Native assignment for EXE 1.0.0.2949

Status: 22.09.2026. This page describes the pure reading research evidence. Based on this, the [Isolated sample 0.8.0](REGISTRY_NATIVE_INTEGRATION.md) and their [Extension to owner locks and readers 0.9.0](PINNED_CAPTURE_INTEGRATION.md) exist. Own proof of inventory/equipment/selection accesses adds [Modul 0.10.0](INVENTORY_2949.md). Event preparation and native server/client sample on private items added [Modul 0.11.0](EQUIPMENT_EVENTS_2949.md); Transport and effect/UI receivers are fixtures. Native copies of items and their lifetime are supplemented [Modul 0.12.0](ITEM_LIFECYCLE_2949.md), still with their own test allocator. Included capture and native equipment slot marking adds [Modul 0.13.0](DIRTY_SLOTS_2949.md); The later slot consumer is not yet connected. The full field recorder for both ranges/Zustandcopies adds [Modul 0.14.0](FIELD_WRITER_2949.md); The joint engine transaction remains open. Furthermore, no installable mod. The app supports the tables of this build since v0.5.9; A complete repair runtime is therefore not released.

`tools/observe_registry_build.py` checks the full EXE hash `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a` before and after reading. It resolves eight primary VTables via type names and self-referencing MSVC-RTTI locators, requires unique results and method addresses in executable PE sections. It does not read processes or saves and does not execute EXE functions. The report contains function sizes from pdata, hashes, unwind heads, and static jump/call targets. A missing pdata entry does not get an invented limit.

|Methode|RVA in the new build|Occupied size|
|---|---|---:|
|ClientActorManager, Slot `0x28`|`0x8ae140`|485 Bytes|
|ServerActorManager, Slot `0x28`|`0x2a82300`|542 Bytes|
|Actor release, slot `0x08`|`0x14366a0`|757 Bytes|
|WindowsRWLock Acquire, Slot `0x18`|`0x1371b60`|155 Bytes|
|WindowsRWLock Try, Slot `0x28`|`0x1371c30`|174 Bytes|

The specific Actor release matches in CommonActor as well as the four tested client/server normal/user types. Short acquire wrappers and the lock release method do not have their own pdata entry; their limits are not demonstrated by this sample.

The full lookup hashes:

- Client: `854519c59e00f271c9a5eb4635a29065aeb7ab3e07a89fc18c58eff34d762253`
- Server: `cda4603cc3a252f9d40f9fec9bd40882a0dd65eff422d612e457c8d3e9f29bcc`

The new server address was obtained from the current VTable. Previous preliminary v7 addresses from the update investigation may not be adopted. The client path calls the registry helper `0x464290` and the actor slot `0xc0`; The server path uses the Type Helper `0x466060` and Actor slot `0xb8`. The methods are therefore not replaceable by a common assumed wrapper. The protected reference acquisitions, thread/TLS requirements and release dependencies must be jointly bound and checked before a new execution sample.

Project call:

```powershell
python runtime/repair/tools/observe_registry_build.py
```

Local report: `.local/repair-build25455892-registry-observation.json`. The historical native host will not be relaxed and will continue to deny this EXE. The new registry host is pinned separately and runs the protected client/server lookups including reference acquisition and sharing on private test objects. Live resolution and repair transaction remain further tasks.
