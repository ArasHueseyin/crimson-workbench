# Adjustable Axiom Force reach

17 October 2026. The native adapter, its baseline guards and owned-memory tests
are implemented locally. **Live gameplay acceptance has not been observed or
claimed.** Research read the pinned executable and existing privately extracted
original data. It did not control a game, install a module, or edit game files,
archives, settings or saves.

`CrimsonAxiomReach.asi` scales the dedicated Axiom Force distance tuning at
startup. Normal **1x** changes nothing; **2x through 5x**, including intermediate
percentages, scale target reach, held-object distance clamps and the native
horizontal/vertical release margins. This extends the functional arm/tether,
not the character model or weapon hitboxes. Collision, target type, ownership,
skill, puzzle and other non-distance eligibility rules remain native.

## Identity and native distance evidence

Supported executable: **1.0.0.2976**, SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`,
PE timestamp **1790086912**, image size **389722112**. Addresses below are RVAs.

The original `commonactioninfo.xml` identifies the Axiom Force preset as
`RemoteCatchPreset Key="BlackWolf"`. Its actual thrown/IK arm uses
`CD_Abyss_Gauntlet_01`, `B_Att_Hand_L_00` and `B_Att_IK_L_Hand`.
The privately extracted original XML SHA256 is
`2e61dc12ee504d424caeb7322cf62896feb92bf1639eddc845b698610ad73f2c`.
`ClientRemoteCatchActorComponent` has vtable **55890d8**;
`RemoteCatchFunctions@gameClientScript` has vtable **56ffb48**. This separates
the requested ability from unrelated grapple, weapon-reach and render effects.

The four fixed-address float tuning records are:

| Meaning | Record | Current cell | Native key | Baseline |
| --- | --- | --- | --- | --- |
| Ordinary Axiom target/held reach | 6c97870 | 6c978a8 | 6dce9b68 | 20.0 |
| Special/type-5 Axiom reach | 6c978c0 | 6c978f8 | 8a220e76 | 32.0 |
| Horizontal release margin | 6c97960 | 6c97998 | d59ce7f6 | 3.0 |
| Vertical release margin | 6c979b0 | 6c979e8 | e3381094 | 3.0 |

Each record has float-tuning vtable **5715be8**, key at **28**, current float
at **38**, and original/default float at **3c**. The native equality method
**1363520** compares current/default cells. Only current cells are changed;
default values, registry bindings and adjacent fields are retained.

Native consumers establish the meanings beyond their positions in a table:

- **58d2e0**, including **58d776** and **58ea50**, selects ordinary/special
  target distance and compares actual candidate separation against that range.
- **58fa30**, at **58fe86**, computes the candidate's Euclidean separation,
  then `vcomiss xmm6,[6c978a8]`; candidates beyond it take the rejection branch.
- **5a9950**, at **5a9a8d**, uses the ordinary range when constraining the
  target's displacement along the camera direction.
- **117fda0**, at **1180270**, clamps the held distance field `+314` through
  `vminss xmm0,xmm3,[6c978a8]`, then retains its native lower bound.
- **1180ea0**, **11811ba..118122f**, chooses ordinary/special range from native
  target type 5, adds the horizontal and vertical margins, and compares actual
  horizontal separation and absolute vertical separation. The following
  collision/line-of-sight checks remain intact.
- **11a88b0**, at **11a891f..11a8937**, is another holding/stretch consumer of
  ordinary range plus the minimum of the two margins. This is a second native
  holding path, not evidence of a server-only hook or networking feature.

At 2x, ordinary acquisition changes from 20 to 40; its ordinary release envelope
changes from 23 to 46. Special reach changes from 32 to 64, with corresponding
release margins. At 5x, the current tuning is 100/160/15/15. No guessed global
float search or archive rewrite is performed.

These are **shared dedicated Axiom tuning values**, not per-selected-character
fields. The loaded-world guard controls when startup changes are allowed; it
does not imply that other native consumers of the Axiom tuning are excluded.
RTTI separately identifies `ServerRemoteCatchActorComponent` at vtable
**5b1f508**. No change to its methods is made or claimed.

## Initialization and ownership

Writing during ASI load would be wrong: the native constructors could reset the
values. Their actual constructors and C++ initializer-array entries are:

| Initializer entry | Constructor | Array cell |
| --- | --- | --- |
| 94430 | jump to 7df5ef0 | 51f3278 |
| 94510 | 94510 | 51f3280 |
| 946d0 | jump to 7dff9d0 | 51f3290 |
| 947b0 | jump to 7e013d0 | 51f3298 |

The constructors write the original current/default pairs, then call tuning
registration **1360a40**, which can apply an existing native tuning override.
They belong to the C++ initializer array **51ecb30..521b810**. CRT entry
**4895b38** invokes the array at **4895baf** and sets initialization state
**6c88690** to **2** only afterwards, at **4895bb4**.

The worker waits until CRT state is 2 and the client root **6d69190** resolves
through manager `+30` to a typed manager **55b62c8**, selected actor `+50` of type
**5585c40**, nonzero identity, alive flag and matching possessor backlink. The
selection is rechecked. Thus normal static construction and initial native
tuning-override loading finish before the adapter changes anything.

The worker is pinned before its thread starts and polls at 250 ms while waiting.
There is no menu timeout, so a late save load remains supported. Once the world
is ready, all four descriptor vtables, keys, current values and original/default
values must match the original baseline. An existing non-default tuning override
is a conflict, not an invitation to multiply it. No automatic retry follows a
conflict or attempted mutation. No actor pointer is kept or written.

## Transaction, coexistence and reset

The module validates the complete EXE hash, PE identity, **24 constructor/CRT/
consumer fingerprints** and the four relocated initializer pointers. It installs
no MinHook detour and writes no instruction bytes, avoiding shared hook-entry
collisions with the existing runtime modules.

All four current values are inspected before the first write. Each write is an
aligned 32-bit compare-exchange from the exact original bits to the scaled bits.
Descriptor identity is rechecked before each change, and a final read verifies
all four resulting cells. On partial failure, already changed cells roll back in
reverse order using compare-exchange from the adapter's own scaled bits. A later
foreign value is preserved and a rollback-conflict message is logged; rollback
is never reported complete when that happens. The module does not continuously
overwrite native developer-tuning edits made after activation.

Settings are exactly eight bytes: ASCII **CWA1**, then little-endian u32
`reach_percent`. Allowed values are **100** or **200..500**. The user file is
`%LOCALAPPDATA%\local.crimson.workbench\axiom-reach.dat`. A missing, malformed,
linked or incorrectly sized setting falls back to 1x with no memory writes.
Save settings/install/remove with the game closed; the file is read once on the
next game start. A 1x setting or removing the ASI restores native behavior on
the next start because process RAM is recreated. No save patch is involved.

## Verification and remaining acceptance

`tests/axiom_tests.cpp` passed with MSVC 19.44 x64. It exercises every valid
integer percentage, malformed/endian/range settings, delayed world availability,
missing/invalid context, each descriptor/current/default conflict, each of the
four possible write failures, complete rollback, concurrent foreign edits,
postflight interference and prevention of double multiplication.

The same test executes isolated copies of the original **58fe86 acquisition
comparison** and **1180270 held-distance clamp** against owned tuning memory.
Only RIP displacements are relocated; ABI wrappers preserve required registers.
They reject distances above 20/clamp to 20 at baseline and accept/clamp against
100 after the owned 5x transaction. No fixture calls an address in the game or
accesses another process.

Parent integration should run the combined package/loader checks. User gameplay
acceptance should then cover ordinary and special targets at 2x and 5x,
acquisition near each boundary, held-object dragging, walking away until native
release, world reload, and a 1x restart. Existing puzzle/type/collision refusals
are expected to remain refusals. The recorded tests do not replace this check.
