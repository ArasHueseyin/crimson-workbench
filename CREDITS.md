# Credits, sources and licenses

**Sharing package 0.6.1 (10.10.2026):** The original project code is under [MIT](LICENSE). The full licensing of desktop dependencies and optional native modules including MinHook and Ultimate ASI Loader are available in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and `licenses/`. The following entries document the historical development; the earlier statements without loader dependency do not apply to the new optional runtime package. Game contents will still not be supplied.

Status: 19.09.2026, phase 2. The targeted native MIT port from crimson-rs is below [`vendor/crimson-format`](vendor/crimson-format/), with complete [MIT licence](vendor/crimson-format/LICENSE), pinned original commit and [Change/file directory](docs/FORMAT_PORT.md). It is integrated as a local Rust crate. There is no Python, CDUMM or save-writer runtime dependency. Other reference sources remain in `.research/` and are not adopted. License details were checked on files, not just GitHub labels or READMEs.

The desktop interface uses regular dependencies instead of copied community editors: Tauri 2, React, TypeScript, Vite, Zustand, TanStack Table/Virtual, Lucide, Vitest and Playwright. Exact versions/licenses are in `app/package-lock.json` and `Cargo.lock`; DDS/PNG decoding uses Rust `image` without its full standard feature package. The C app icon is its own SVG graphic; The PNG/ICO variants were generated with Tauri. Item icons are only read locally from the user installation and are not bundled.

Implementation references: [Tauri commands](https://v2.tauri.app/develop/calling-rust/), [Tauri-Capabilities](https://v2.tauri.app/security/capabilities/), [Vite configuration](https://v2.tauri.app/start/frontend/vite/), [TanStack virtualization](https://tanstack.com/table/latest/docs/framework/react/guide/virtualization), [DDS-Header](https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header). Stringinfo format knowledge comes from `src/string_info/mod.rs` of the MIT crimson-rs revision pinned below; `cd-core/src/icons.rs` implements the limited parser itself and checks the current body/header round trip.

|Source/Revision|Established licence and intended handling|
|---|---|
|[bbfox0703/crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0), `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0` (18.09.2026)|[MIT](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/LICENSE), Copyright Tommy Tran. archive, save, PALOC and table reference; Built/tested locally for reference. When taking over, provide license text/notices.|
|[lazorr410/crimson-desert-unpacker](https://github.com/lazorr410/crimson-desert-unpacker/tree/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e), `f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e` (23.03.2026)|[MIT](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/LICENSE), copyright lazorr410. PAZ/PAMT, Crypto, Compression and Repacking. In principle, licence-compliant selective takeover is possible; It has not yet been done.|
|[NattKh/CrimsonDesertModdingTools](https://github.com/NattKh/CrimsonDesertModdingTools/tree/1a9c3f10440713ee1b8f0bb88f66091376b433b5), `1a9c3f10440713ee1b8f0bb88f66091376b433b5` (11.04.2026)|README calls MIT, but a LICENSE file is missing in the tested Git tree. According to Spec only format documentation, no copying of parsers or schema files.|
|[NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/tree/96e7f78fcb00d6cb5615171a7f359f0c2de5b114), `96e7f78fcb00d6cb5615171a7f359f0c2de5b114` (06.09.2026, author time zone)|[MPL-2.0](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/blob/96e7f78fcb00d6cb5615171a7f359f0c2de5b114/LICENSE.txt), copyright RicePaddySoftware. Here format/error mode reference. Adapted/adapted MPL files would have to meet their license/source code requirements.|
|[GildyBoye/CrimsonDesertShopEditor](https://github.com/GildyBoye/CrimsonDesertShopEditor/tree/c92d9153055c7a371c2c0cca51fc49d7c1c23d53), `c92d9153055c7a371c2c0cca51fc49d7c1c23d53` (16.08.2026)|No LICENSE file found; [TermsOfService.md](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/TermsOfService.md) contains mod attribution conditions. Only format/behavioural reference, no code adoption.|
|[faisalkindi/CrimsonDesert-UltimateModsManager](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/tree/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6), `b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6` (13.09.2026)|Root [MIT](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/LICENSE), copyright faisalkindi, but individual parsers/schema and DDS parts carry MPL 2.0 hints. No blanket MIT release for each file. Only reference for B0, no manager dependency.|

The remaining lines of the above search decisions describe their phase 0 status; In fact, only the MIT subset of crimson-rs listed in `FORMAT_PORT.md` was ported. The versions of the Rust dependencies are recorded by `Cargo.lock`. SQLite is integrated via rusqlite with the `bundled` option ([rusqlite documentation](https://docs.rs/rusqlite/0.40.2/rusqlite/)).

03.10.2026: The save codec from the same MIT crimson-rs revision was ported for stored knowledge displays and separate stable registration. Copyright and MIT text are also included in `vendor/crimson-format/LICENSE-save`. Authentication and sizing checks have been tightened; the transaction, template verification and maintenance of existing save areas are own core implementations. Details: [FORMAT_PORT](docs/FORMAT_PORT.md) and [mounts](docs/REITTIERE.md).

## Who is Tommy Tran?

Tommy Tran is the copyright holder named in the upstream crimson-rs MIT license. Workbench includes selected archive, localization, table and save-codec components derived from that project. The original notices are retained in `vendor/crimson-format/LICENSE` and `vendor/crimson-format/LICENSE-save`. This credit does not identify him as the author of the Workbench application; the original Workbench code has its own copyright notice in [LICENSE](LICENSE).

## Attribution in generated mods

GildyBoyes Conditions require traceable authorship and an open description of the changes; Edited foreign mods must remain recognizable as such and retain their original credits. The spec extends this requirement to mods that arise from insights from this editor.

Workbench must therefore identify the sources used later in the change manifest and in included mod descriptions, in particular GildyBoye for corresponding shop/drop/trust/skill findings. Changes, including relevant values, are described; Third-party mods are not presented as original work. This does not replace permission to copy the editor code.

## Game and reference mods

Crimson Desert and its contents belong to Pearl Abyss or the respective rightholders. This project is independent. Game archives, tables, textures, saved games and content of foreign Nexus mods are not included. The local folder `reference/mods/` did not contain any reference mods at the start of the search.

Before each actual code transfer, the license of the specific file, its origin and the required notifications are checked again. The implementation decision made here is: own core facades from documented format insights plus the documented MIT partial porting of crimson-rs.

## Phase 3: Recipe calculator

New original readers/writers in `crates/cd-core/src/crafting/formats.rs` and material planners in `planner.rs`. format candidates from the NattKh documents pinned above and MIT-crimson-rs; Dropset format comparison with NattKhs MPL editor. No code transfer from these python files. Current byte widths were measured at the local installation and fully roundtrip-tested; known deviations and limits are in [docs/CRAFTING.md](docs/CRAFTING.md).

## Phase 4: mod preview and transaction sample

Original Rust implementations in `mods/`, `apply/` and `vendor/crimson-format/src/overlay.rs`. Archive format knowledge and existing MIT cryptoportation from crimson-rs, after re-reading the MIT license. Store/Friendly format notes from the references pinned above, no adoption of CDUMM, NattKh or Gildy source code. The proprietary `dmm_parser` implementation is not integrated or replicated; for the licence indication, further investigation of this sub-package has been terminated. Exports expressly contain GildyBoye with source link and name Workbench as their own producer. All record/layout documents are separated in [MODS.md](docs/MODS.md) from still unconfirmed field meanings.

For v0.4.2, refresh/stock indices and independent drop chances were additionally determined by own static analysis of the local EXE and the bound tables. No game bytes or disassembly were transferred to the repository. The SteamKit reader was used only for comparing the manifest section structure, the Daily Refresh mod only for plausibility checking the number. Sources, addresses and limits: [B1/B2 field evidence](docs/research/PHASE4_FIELDS.md).

## Steam Depot Inventory (v0.4.4)

Own limited Rust parser, only format knowledge from [SteamKit DepotManifest](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs) and the [Protobuf definition](https://github.com/SteamDatabase/Protobufs/blob/master/steam/content_manifest.proto). No SteamKit/DepotDownloader code adopted, no new runtime dependency, no authentication or downloads. See [Installation test](docs/INSTALLATION_CHECK.md).

## Complete content comparison (v0.4.5)

[RustCrypto `sha1` 0.10.6](https://docs.rs/sha1/0.10.6/sha1/) as a regular cargo dependency, `MIT OR Apache-2.0` according to the tested package manifest. SHA-1 exclusively for the existing Steam hash comparison; SHA-256 continues over `sha2`. No cryptographic self-implementation and no origin certification. The own reader/worker code is described in [CONTENT_AUDIT.md](docs/CONTENT_AUDIT.md).

## Phase 5 (v0.5.0)

Own limited readers for confirmed world/skill/equip fields and own in-memory facade via the existing MIT item port. Format knowledge from the references pinned above and static testing of local EXE serialization; No accepted NattKh parser code. The item port keeps its MIT notes. No new libraries, game content or disassemblies in the repository. [Field evidence and restrictions](docs/research/ADVANCED_TABLES.md).

v0.5.1 complements own Enchant line copies and read-only skill buff inspection. The common buff structure was compared to the pinned MIT source and the local serializer; variable payloads were checked on the local EXE reader. No adoption of the heuristic upstream tail search algorithm. RTTI names and derived format widths are structural metadata; private values/disassembly remain exclusively in ignored research files.

v0.5.3 adds its own full reader for the 108 reoccupation rules and an inventory test limit from statically read EXE default values. No new external implementation, no EXE or runtime patches; Evidence in `docs/research/ADVANCED_TABLES.md`.

v0.5.2 adds its own fixed skill number changes, two stage reset readers and a single city flight condition. The specific reference to Condition 1011130 provides [Flight Freedom](https://github.com/shin2344234/flight-freedom/blob/d61a9c37fc2fc0970d155329ddbf143038f0910d/mod/src/game/signatures.h), Commit `d61a9c37fc2fc0970d155329ddbf143038f0910d`, MIT. No plugin, hook or patch code adoption; The data serialization and Boolean replacement condition were verified separately on the local EXE file. Source and document addresses are in the field proof linked above.


v0.5.4 adds its own named division of the Summon/AddSubLevel payloads from static reader/serializer analysis of the pinned EXE file. Only structural metadata and own readers in the project; No new external implementation, no game values or disassemblies in the repository, no process access.


v0.5.5 removes its own previously unused repair zeroing path after static detection of unprotected divisions in the local engine code. No external implementation or change in maturity; Sources remain the pinned EXE and the already ported item structure. New regression tests and template correction are own implementations.


v0.5.6 adds its own complete skill header/suffix inspection based on the statically read pinned EXE and removes the previous search for the same identifier bytes. No new external implementation or source data adoption; Addresses, field names and boundaries are in `docs/research/ADVANCED_TABLES.md`.


v0.5.7 adds its own Sentinel change for item durability and its own inventory move reader. The reference to 65,535 is provided by the documentation of [NattKh/RicePaddySoftware](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS) (`FIELD_JSON_V3_SPEC.md`, `ITEMBUFFS_FEATURE_AUDIT.md`, MPL-2.0). No MPL code. Semantics and byte widths were independently tested on the pinned local EXE; Implemented via the existing MIT item port. Stranger mods were neither taken over nor delivered.


v0.5.8 adds its own BuffInfo reader and additional self-consumption costs. The [DMM-Author-Changelog](https://www.nexusmods.com/crimsondesert/mods/633) provided the indication of previously missing equipment costs. No dependency, no download/installation of DMM or mod files; Format and fields were detected separately on the pinned local EXE and tables. Already known reference schemes were used exclusively as a format comparison; No foreign code is copied. Game content and disassembly remain in ignored local files.


Repair runtime prototype 0.1.0: own C++ implementation and own native test host; no acceptance of ASI loader, hook or modcode. The interfaces and computing paths were derived from the pinned local EXE. Original game code is only read dynamically during private tests and not embedded/delivered. Win32 usage after [Microsoft VirtualProtect](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect) and [DLL Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices). The optional reading audit uses locally installed pefile 2024.8.26 (MIT) and capstone 5.0.9 (BSD license); No package code has been copied or built into the DLL.

Own repair action 0.2.0: new own C++ implementation, queue and tests. [Trinity](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b), Commit `70c9a00dd6e10b2081d706a837756844c11f5c2b`, MIT, Copyright 2026 XeTrinityz, served as a licensing reference for separate client/server items. No code, hook, loader or binary adoption. The older equipment offsets do not fit the local build and have not been adopted; The item/socket fields used were independently verified from the local EXE and tested by isolated native calls. Research checkout only under `.research/`.

Reading repair adapter 0.3.0: own implementation for the current client/server registry, inventory/equipment structures and limited local read accesses. The same MIT reference Trinity gave structure/player clues; No implementation is undertaken. Current globals, slot boundaries and strides have been re-examined at the pinned EXE. Inventory/Registry functional copies are only read in your own test host from the local EXE; No original bytes will be delivered. Native Lock/Actor leases are custom test stubs and are not issued as proven engine synchronization.

Repair control 0.4.0: own asynchronous final control and after-value test. Server/client event paths and destructive socket function independently investigated at the same pinned EXE. The private test host replaces only a TLS read access of the client copy with artificial data; UI, Allocator and virtual locks are stubs of their own. No original code delivery, no new foreign code and no acceptance of a loader or hook.

Owner lock 0.5.0: own lock group and extension of the reading adapter. WindowsRWLock methods, Imports, Actor access and TLS branches independently tested on the pinned EXE; Trinity remained the same MIT structural reference without code adoption. Native samples call real Windows SRW features on their own lock objects. A TLS access of the private try copy is redirected. No original bytes delivered; no new foreign code dependency.

Reference management 0.6.1: own PaPtr owner and integration into the reader. Native acquisition/release paths including concrete actor override and method tables independently checked on the same pinned EXE. Threadmap, embedded locks, cleanup and destruction are their own test dependencies. Three TLS read accesses are redirected exclusively in private functional copies. No new third-party code takeover, original code delivery or modmanager dependency.

Registry source adapter 0.7.0: original C++ implementation and synthetic testing. The installed EXE 1.0.0.2949 was recorded only by reading. Previous native proofs for 1.0.0.2944 will not be transferred to the new build. No new foreign code dependency and no delivered game bytes.

Registry/reference integration 0.8.0: own separately pinned test host for EXE 1.0.0.2949. The 15 functional areas come exclusively from the local installation and are dynamically read only for the private test; No original code delivery. Real Windows SRW imports are working on their own test objects. Four TLS read accesses are redirected to private data; Threadmap and final object cleanup are separate fixture dependencies. No new foreign code adoption or loader dependency.

Protected capture 0.9.0: own reader conversion, own regression tests and extension of the private sample with the original 2949 WindowsRWLock-Try path. Total 16 dynamically read functional areas; No delivered game bytes. The inventory layout of the combined sample remains its own legacy fixture. Trinity’s motion driver was read as a structure clue, not adopted, or released as a proven player/engine dispatch. No new foreign code dependency.

Inventory accesses 0.10.0: own new build assignment and private comparison samples for inventory, owners, current selection, equipment and durability fields. 24 selected code ranges are read exclusively from the local EXE; No game bytes are delivered. Six private TLS redirects. Table resolvers and empty temporary items are their own test dependencies. No new foreign code, loader or access to the ongoing game process.

Equipment events 0.11.0: own preparation from repair plans and own private integration fixtures. Four other original functions are only read from the local EXE, no game bytes shipped. Server Notifier and Client Ack each have a private TLS redirection. Effect/transport/UI receivers, child list, clock and memory release are their own test dependencies. No new foreign code, loader or modmanager dependency.

Item copies 0.12.0: own property management and tests. 17 additional features are read from the local EXE to the private test host only, including construction, assignment, destruction, and list helpers. Own Windows heap with protection markings only replaces the allocation interfaces. 16 Additional TLS Reading Points Privately Diverted; normalized a byteidentical Unwind alias for its own chain. No original code delivery or new third-party code dependency, no change to the game process or installation.

Slot marking 0.13.0: six additional original functions executed exclusively from the local EXE in the private test host, no game bytes shipped. Two other private TLS diversions, one byte-identical tested unwind alias and its own aligned test heap. The capture and test fixtures held are their own implementations; No new foreign code or loader dependency.

Field recorder 0.14.0: own C++ implementation for entire repair batches and own local memory access, no other original function added. Native reference/lock functions are only used for private integration fixtures. No new third-party code/loader dependency, no original code delivery and no write execution in the game process.

Transaction adapter 0.15.0: original C++ connection of planning, native copies, field recorders and message adapters with custom error/lifetime tests. No additional original functions or foreign code dependencies. The existing pinned functional copies run only on private test objects. Effect/transport/UI receivers remain their own test callbacks; No game bytes are delivered.

Slot processing 0.16.0: own batch termination and private persistence fixtures. Four additional original functions (consumer, iterator, clear, destructor) are read exclusively from the local EXE to the private test host; No original code delivery. Six other private TLS diversions and ten separately tested wind fragments of the cleanup functions. Persistence recipients and error diagnosis are custom test callbacks. No new third-party code/loader dependency, no game process, installation or save change.

Inventory Client Ack 0.17.0: an additional original function executed exclusively from the local EXE in the private test host; No game bytes are delivered. Three other private TLS offices. Own inventory/UI/name fixtures and integration tests; Packet distributor and socket SQL caller only statically read. No new foreign code or loader dependency, no game process/save accesses.

SQL job path 0.18.0: two additional full original functions from the local EXE run exclusively on private requests. Five concatenated wind fragments of the execution shim tested; No additional TLS replacements. Clock and diagnosis as well as parts of the combined consumer test are their own test callbacks. RTTI and alternative results processing are only read-only examined. No original code delivery, new third-party code dependency or game/save access.

Item/Storage Object Converter 0.19.0: 24 additional full functions are copied from the local, hash-tested EXE to private test objects only. 32 additional pdata fragments and 15 private TLS redirects, including an RCX charging point. Own catalogue, reflection, watch, policy and heap fixtures; no new third-party code/loader dependency or original code delivery. No game process, installation or save accesses.

Save dispatcher 0.20.0: six additional code ranges run exclusively from the local, hash-tested EXE on private objects. Seven more pdata fragments, four tested unwind chains with byte-identical aliases and an additional private TLS redirection. File, platform and diagnostic recipients are custom test callbacks; Concrete file helpers and timers were only statically read. No original code delivery, new third-party code dependency or game/save access.


File Writer 0.21.0: three additional original functions from the locally hash-tested EXE run on private test objects, three pdata entries and four private TLS locations. Own formatting, encoder, diagnostic and file callbacks as well as an entry bridge to the dispatcher. The test replaces Flush/Close only in the private test address room; no real file handle, no original code delivery, new third-party code dependency or game/save accesses.


Memory buffer 0.22.0: nine more original functions copied from the hash-tested local EXE to private objects, seven pdata entries and three other private TLS replacements. The Stack Probe uses its own Windows test thread. own post-processing/file callbacks; own limited test decoder after the [LZ4 block description](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md). No LZ4 source code adopted, no new runtime dependency or game bytes delivered. No game process, installation write or save accesses.

Inventory package serializer 0.23.0: two additional original functions copied exclusively from the local, hash-tested EXE to private objects. Two pdata records and two other private TLS replacements. Own receiver/comparison/stream and diagnostic callbacks as well as limited own decoder to the previous client. No additional foreign code dependency or delivery of game bytes.

Separate offline inventory order from 23.09.2026: the already pinned MIT reference crimson-rs was used for reading/schema insertion/relocation and save packaging. Our own additional tests compare existing field bytes, pointers, UIDs, standard values and integrity. No new save writer feature in the published Workbench.

Supplement 04.10.2026, base entries for mounts: Own construction from the field name/type scheme of the local private save, without copied byte templates. NattKhs above pinned editor serves as a behavioral/format reference for simple animal registration; the [Analysis of Mount Repair](https://github.com/Atomic-Flip/crimson-mount-repair/blob/main/docs/technical.md) and [CrimsonAtomtics Mount-Katalog](https://github.com/bbfox0703/CrimsonAtomtic/blob/main/src/CrimsonAtomtic.Ui/Services/MountCatalog.cs) document schema changes and ownership/convocation fields. This does not result in confirmation of the newly built initialization path in the current game. No code, no foreign Savebytes and no knowledge/quest packages of these sources.
