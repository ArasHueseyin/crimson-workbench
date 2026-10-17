# Active mount protection

Status: **locally integrated and independently reviewed; game acceptance pending**. The local module protects the currently ridden character-based mount and the selected, summoned owned Vehicle-group main mount, including after dismount. It does not protect every owned or registered animal. No module was installed, no game process was changed or started, and no saves or game files were written during this work.

## Reviewed build and HP definition

The implementation is restricted to `CrimsonDesert.exe` build `1.0.0.2976`, SHA256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`, PE timestamp `1790086912`, image size `389722112`. It checks the full EXE hash and **23 native code spans** before hooking. Private audit metadata contains 33 complete native function ranges and their hashes.

Original StatusInfo, MercenaryInfo and MercenaryGroupInfo were extracted read-only through the immutable baseline registry. Their six body/header hashes are independently checked by `verify_mount.py` and recorded in `verified-audit.private.json`.

| Definition | Semantic key | Original ordinal | Kind |
| --- | --- | --- | --- |
| Hp | 1000000 | 0 | **1** |
| Stamina | 1000026 | 22 | **2** |

HP has kind 1, not stamina's kind 2. Native StatusInfo serializer `1513c80..1514335` reads the blocked byte into object `+10`, the next one-byte kind into `+11` at `1513d42`, and the following four-byte ordinal into `+14` at `1513d64`. Its SHA256 is `2d14c7f57253cde4852dcd9d02c52691902bb22f4a9153c56049d38db9be4b67`. The hook resolves the incoming ordinal and requires semantic HP key, kind and matching ordinal; ordinal zero is not assumed.

## Native active mount association

| RVA | Evidence |
| --- | --- |
| `214f680` | Main-selection mutator clears the old record's selected-character handle `+148` to `FFFF` at `214f773`, then assigns the new record at `214f80f`. |
| `214fad0` | Swaps main-selection handles between two full-serial records. |
| `214c190` | Native main-record lookup scans clan records `+18`, count `+20`, resolves CharacterInfo `+BE` to MercenaryInfo `+70` to its group, and requires record `+148` to match the current selected character's Status `+30`. |
| `2275180` | `CheckMainMercenarySummoned` obtains owner clan component `+110`, calls that lookup and reads returned record `+50` as the live full actor identity at `2275229`. |
| `2146170` | Clan serial map hashes the low 32 bits but compares **all 64 bits** of node `+8` before returning record pointer `+10`. |
| `2154b00` | Mount MercenaryComponent `+18` resolves its owner; `+20` is the full instance serial used in owner clan lookup. |
| `9db9c0` | Client clan virtual `+198` performs serial lookup, resolves record `+50` and checks spawned actor's MercenaryComponent `+18` against the clan owner. |
| `9e2dc0` / `21563d0` | Record update/copy preserves full serial `+28`, character handle `+20`, selected-character handle `+148` and spawned full actor identity `+50`. |
| `e4e35e0` | Reset sets serial to all ones, spawned identity to zero and selection to `FFFF`. |

Factory `8ab190` establishes Clan component slot 34 (`+110`) and Mercenary component slot 35 (`+118`). Exact Client/Server vtables are checked. Character, mercenary and group definitions are resolved by loaded handles, with key/count/blocked checks. Original MercenaryGroupInfo semantic key **65, Vehicle**, contains keys **78, 79, 80, 82, 81, 65**; these have MercenaryType **2**, loaded at MercenaryInfo `+20` by serializer `1510ca0`, and group handle `+70` read at `151134b`. Pet, Domestic, followers and Ship have separate groups. The original summon limit can exceed one; ownership or being summoned alone does not establish the active main mount.

The implementation requires exactly **one** Vehicle-group record selected for the current player character in each client/server clan. That record must be the serial lookup result, its full 64-bit serial and character must match the candidate, and its full spawned actor identity must round-trip to the authoritative registry actor including generation bits. Both MercenaryComponent owner identities and serials must agree. Inactive registered records, mismatched generations, duplicate selections or duplicate exact serials fail closed. Low-key hash collisions with different serial high bits remain valid.

This main-mount path does **not** require an occupied rider seat or Vehicle `+EB`. Native `CheckMainMercenarySummoned` establishes the selected summoned actor independently of those rider-seat conditions. Fixtures verify protection with zero riders and `+EB=0`. The separate currently ridden fallback retains `+EB` and reciprocal seat checks used by native rider lookup. No last-ridden cache is used.

## Damage consumer and conservative behavior

The single hook is owner-delta helper `c7e0860`, reached through `17b3230`. It resolves the incoming status entry and forwards the signed delta through `17b3e10 -> c7e7960`. Normal HP dispatcher `17ae080` routes to health helper `17ac040`; zero delta leaves initialized output and positive HP valid, avoiding its normal HP death branch. Complete owner-delta SHA256 is `940fe54602de0c88f9e8f4101fa623637e89904cd0a10a731e023db3bf901fb3`; health helper SHA256 is `0bebc01a812409d5deca19db6e2b406e92f4afe6948a7c4590843f2b0d5f7f97`.

Both paths start from fresh selected client/server player identities and validate actor, status, vital, control, vehicle and owner backlinks, exact vtables, character kind, positive HP and full authoritative identity. The ridden fallback additionally verifies matching rider mount/seat and reciprocal occupied seats. Every borrowed relationship is rebuilt before suppression; volatile integral reads prevent compiler reuse. Inspection exceptions preserve the original delta. These are conservative snapshots, not native synchronization.

Only negative HP deltas in the verified scope become zero. Player health, NPCs, inactive owned creatures, heals, stamina, other resources and already dead mounts keep native behavior. The original helper is called once; every other argument, output and full return register is preserved. Direct scripted death/despawn or separate HP setters are outside this hook's claim. Lion, Rokade and other character-based mounts use the structural path and still need game acceptance.

This hook is separate from Player Stamina's `c7efde0` commit hook. The module pins its lifetime before creating any hook. After an activation attempt it retains its published trampoline even on an activation error, because an entered callback might still use it.

Parent integration removed a loaded-entry guard at CharacterInfo getter
`389570`: bundled Blackstar legitimately hooks that entry. Mount protection
reads the manager rows directly and does not call this getter. The original
complete-function audit and the implementation-body pin at `87f8210` remain.
The coexistence fixture checks all 39 guards from the new mount/consumables
modules against the 31 packaged hook entries, and rejects a reproduction of
the old conflicting guard. This covers the specific code-guard conflict,
not full gameplay compatibility with every mod.

## Verification and handoff

Private CMake builds the ASI and fixtures with MSVC `/W4 /WX`. CTest passes **278 original-call forwarding cases** plus five injected lifecycle outcomes. Fixtures cover mounted and on-foot selected mounts, both actor representations, zero riders/disabled rider flag, inactive registrations, duplicate selections/serials, 64-bit hash collisions, stale actor generations, selection changes during inspection, reordered definitions, HP kind 1 versus stamina kind 2, ownership corruption, multiple seats, invalid memory, unchanged resources and full native forwarding.

Ignored `.local/marketplace-expansion-research/mount-protection/` contains:

- `archive/Cargo.toml`: reproducible original table extraction into the private folder.
- `verify_mount.py`: independent EXE, 23 code spans, 33 complete function hashes, six original table hashes, HP kind loader and Vehicle-group semantics audit; it does not regenerate pins.
- `full-functions.private.json`, `verified-audit.private.json`, native disassemblies and table records.
- `build/Release/CrimsonMountProtection.asi`: local build only.

Shared CMake, native staging, manifest allowlists, package lifecycle scripts and
English Marketplace controls are integrated. Parent review independently
rechecked the original EXE, 23 code pins, 33 complete function hashes and six
original table hashes. Combined native tests and the synthetic 12-ASI loader
host pass. The Marketplace browser fixture verifies independent install/remove,
the on-foot scope description and refusal to change modules during play.
The final temporary package lifecycle run also passes all ten optional modules,
including independent removal/reinstallation, running-game refusal and rollback.

Remaining game acceptance: ridden/on-foot combat damage for lion/Rokade,
switching the selected mount, inactive owned creatures taking normal damage,
despawn, reload, and coexistence with other runtime modules. This checkpoint
claims offline implementation, not an observed successful game test. Work is
paused at the user's request after this integration round.
