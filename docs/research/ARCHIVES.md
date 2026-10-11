# Archive, parser and dependency research

Status: 19. September 2026. phase 0; No application code, no apply, no change to game or saves. The samples executed during play read small metadata and selected PAZ areas. Extracted content is exclusively below `.local/archive-probe/extracted/`; public reference sources, builds and research aids under `.research/`. Both directories must be ignored.

## Pinned Sources and Licence

|Source|Tested Commit|Commit time|Licence|
|---|---|---|---|
|[bbfox0703/crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0)|`b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0`|2026-09-18 16:51:44 +08:00|[MIT, Copyright 2026 Tommy Tran](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/LICENSE)|
|[lazorr410/crimson-desert-unpacker](https://github.com/lazorr410/crimson-desert-unpacker/tree/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e)|`f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e`|2026-03-23 15:30:39 -04:00|[MIT, Copyright 2026 lazorr410](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/LICENSE)|

Both LICENSE files were read in the pinned checkout. Upon acquisition, receive the full copyright and MIT license text and guide the sources in `CREDITS.md`. The Software License is not proof of sharing game data; They do not become part of the project repo.

## Dependency decision

**crimson-rs is usable as a current format base, but no immediately importable Rust crate for cd-core.** [Cargo.toml](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/Cargo.toml) creates only `cdylib` and `staticlib`, no `rlib`. [src/lib.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/lib.rs) holds `binary`, `item_info` and `save` privately. A simple pinned Cargo-Git entry therefore does not provide the required public Rust API.

The build supports three configurations: empty default features; `c_abi`; Optional `python` with PyO3 0.27.2/abi3 for Python from 3.12. Direct dependencies are `chacha20` 0.10, `lz4_flex` 0.11, `flate2` 1, `hmac` 0.12 and `sha2` 0.10; `tempfile` 3 is used for tests. Python is not required for a native C-ABI DLL. A complete transitive license inventory has not yet been completed: `cargo metadata --offline --locked` wanted to load a non-cache platform-specific lockfile entry (`bitflags` 2.11.0). This is not a build error of the tested Windows configuration.

Recommendation for phase 1: minimal, comprehensible adjustment of a pinned fork with `rlib` and limited public facade, if a separate git-dependency should be provided. Alternatively, port only required MIT modules with source documents. C-ABI is a really working integration option, but it entails additional FFI/artifact management for a Rust back-end core. Do not expose the existing extensive save-write API unfiltered as a backend API.

## Executions performed locally

Windows; `cargo 1.95.0`, `rustc 1.95.0`; Python 3.12. Reference checkout unchanged. Build target `.research/target-crimson-rs/`.

- `cargo test --locked --lib --features c_abi --no-run`: Successful.
- `cargo test --locked --lib --features c_abi -- --test-threads=2`: **354 passed, 0 failed, 46 ignored**. `CRIMSON_GAME_ROOT` and `LOCALAPPDATA` were intentionally placed on non-existent research folders. Many live tests skip internally in case of missing data and still count as “passed”; This result alone does not confirm game compatibility.
- `cargo build --locked --features c_abi`: successful; 51 Warnings, no errors.
- `cargo build --locked --features c_abi,python`: Successful. The Python facade was loaded from the generated DLL in the research folder, without installation into the system python.
- The real file samples mentioned below were then executed separately with the unchanged reference API. No save file was read or written in this research. Linux was not built/tested.

Logs: `.research/crimson-rs-build.log`, `crimson-rs-tests.log`, `crimson-rs-dll-build.log`, `crimson-rs-python-build.log`. Research assistants: `.research/probe_archives.py`, `extract_selected.py`, `roundtrip_probe.py`, `locale_probe.py`, `registry_diff.py`. These are reproducible phase 0 samples, not application code.

## On this installation confirmed results

Installation base: `C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert`. The `0.paver` bytes `02 00 03 00 00 00 38 51 04 03` observed by the main search mean [paver.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paver.rs)`major=2, minor=3, patch=0, build=0x03045138`. This matches the parser target 2.03. Steam-Build ID and EXE version are separate fingerprint components and do not equate to this version stamp.

|Probe|Real local result|
|---|---|
|`iteminfo.staticinfobody`, from group 0008|6,816 Items; 6,465,724 Bytes fully parsed and serialized; **byte-identical**|
|Item-SHA256, input and output|`87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce`|
|Item-`inventory_info_list`|Length 10 for all 6,816 items|
|`meta/0.papgt`|679 Bytes; Reference parse → serialize **byte-identical**|
|PAPGT-SHA256|`c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea`|
|`0008/0.pamt`|1,109,183 Bytes; Reference parse → serialize **byte-identical**|
|PAMT-SHA256|`3723c5d61f6a0e8a9682b549112bbf072c5bb10e1eb8f87155668f65a79cfc90`|
|German `item.paloc`|group 0027, `gamedata/stringtable/binary__/ger`; 649,912 Bytes, 13,575 Entries|
|PALOC-SHA256, extracted container|`d8fd26ad4a845e4c6434f4c393fbe64866dc1f63f2a07df3be7d61fc9d734e5c`|
|PALOC compression|`paloc` magic; Header 0x200; LZ4 block 649,400 bytes → 1,807,930 bytes; **Payload parse → serialize byte-identical**|

28 Files were selectively extracted and hashed: Body and header of `characterinfo`, `crafttoolgroupinfo`, `crafttoolinfo`, `dropsetinfo`, `fieldinfo`, `iteminfo`, `questinfo`, `regioninfo`, `skill`, `stageinfo`, `storeinfo`, `vehicleinfo`, `inventory`, `multichangeinfo` (*)14 Couples. The file `.local/archive-probe/table-fingerprints.json` The sample extracts only selected file ranges, not complete PAZ archives. Table hashes confirm identity, not the correct interpretation of all fields and not the vanilla origin.

Measurement results: `.local/archive-probe/item-roundtrip.json`, `container-roundtrip.json`, `localization-fingerprint.json`, `table-fingerprints.json`, `metadata.json`, `registry-observation.json`. Do not copy extracted content into public documentation.

## PAZ/PAMT: actually implemented formats

[Archive documentation](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/docs/archive-format.md) and [PAMT source code](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/pamt.rs): `meta/0.papgt` registers groups, each group has `0.pamt` and numerical PAZ chunks. PAZ does not have its own header; PAMT contains directory/file tries, file sizes, offsets, chunk IDs, flags and checksums.

PAMT header according to executable source code: `checksum:u32`, `count:u16`, `unknown0:u16`, then `unknown0:u8` plus `encrypt_info:[u8;3]`; together 12 bytes. The simplified header offset text in `archive-format.md` is therefore not consistent throughout. Raw file entry: four `u32` (nameoffset, chunkoffset, compressed size, original size), `chunk_id:u16`, `flags:u8`, `unknown:u8` = 20 bytes. A new parser must obtain raw values and validate limits.

Lower flag nibble: 0 no compression, 1 Partial, 2 LZ4, 3 Zlib, 4 QuickLZ. Upper nibble: 0 no encryption, 1 ICE, 2 AES, 3 ChaCha20. [Paz.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paz.rs) implements None/LZ4/Zlib as well as several partial reading variants; QuickLZ is not implemented. Writing supports None/LZ4/Zlib. Actually implemented cryptography is None/ChaCha20; AES/ICE deliver “unsupported” when extracted. Unknown nibbles are partly displayed as none in the reference parser: cd-core should instead reject unknown values and display raw flags.

[ChaCha20-Code](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/chacha20.rs): Key/Nonce deterministic from basename checksum and three `encrypt_info` bytes of the respective group. No runtime hook/key tap needed. directory portion is removed; The code itself does not make lowercase normalization. The read archive names are therefore to be used unchanged. Decryption takes place before decompression.

[PAMT-Parser](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/pamt.rs) checks the checksum over bytes for the 12 byte header and optionally the group checksum transferred from PAPGT. The used [Jenkins-Hash](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/checksum.rs) initializes with file length plus `0xDEBA1DCD`. The fact that the sample understands the checksums is proven by real PAMT/PAPGT parse results; Complete PAZ chunkhashes were not calculated while playing.

## September compatibility and table limits

The current [Layoutresolver](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/gamedata_layout.rs) knows from 2.01 `gamedata/binarystaticinfo__/bin/<table>.staticinfobody` and `.staticinfoheader`; Fallback is `gamedata/binary__/client/bin/*.pabgb`/`.pabgh`. The new layout is actually present on this installation. The Rust resolver is `cfg(test)`/crate-private and not a directly usable public discovery API.

Do not treat "September" as a single schema version: 2.01 renames container, 2.03 expands the item inventory list from 9 to 10 entries. The pinned [Versionsgate](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paver.rs) currently only allows `(2,3)`. The exact historical conversion at 4. September was not compared here against old local binaries. The current 2.03 item compatibility has been verified in real terms.

There is no universal PABGB body scheme. Index forms differ: `u16 count + (u32 key,u32 offset)` for example at skill; `u16 count + (u16 key,u32 offset)` for stores; Other tables have broader keys. Stage/Quest can be loaded via PABGH-based C-ABI loaders with full key sets; Name anchor scans are potentially incomplete. Header with extract, keep raw sections.

**crimson-rs alone does not meet B1.** [store_info/mod.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/store_info/mod.rs) extracts only key and internal names; Purchase price/sale price/limits and variable stock records are not fully decoded. The claim “Stock-Record +8 Bytes” from the specification was not independently proven with this reference. No blanket fixed stock stride and no editor sharing derived from a mere store name lookup.

## Localization and save-reader

[paloc.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paloc.rs) accepts naked entry lists and since 2.03 LZ4 containers. An entry contains `u64 unk_id`, length-prefixed string key and string value; At the end there is a `u32 count`. The name `CString` in the reference code does not necessarily mean NUL-terminated C string here. Namespaces are partly in numeric string keys `(Jenkins(internal_name) << 32) | namespace`, such as item names `0x70`, quest headings `0x100`, mission/stage titles `0x101`.

German was found locally in group 0027 with 39 namespace files. `item.paloc` has PAZ flags `0x30`: ChaCha20, no PAZ compression; The LZ4 compression lies within the PALOC file. Container round trip must be explicitly distinguished from payload round trip: re-compression of LZ4 does not have to result in the same compressed bytes. For a hard unchanged-byte-identical container request, keep the original compressed bytes as long as there is no change. Here, the decompressed entry list after real serialization is identical; No equality of newly compressed containers is claimed.

[Save-Modul](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/mod.rs): 128 byte header with `SAVE`, version/flags, sizes, 16 byte-nonce and HMAC-SHA256. ChaCha20 decryption, HMAC via compressed plain text bytes, LZ4 block decompression, body scheme/TOC/object decoder. [Save::parse](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/io.rs) delivers the body even in the wrong HMAC and sets only `hmac_ok=false`; CD-core must explicitly treat this Zustand. Full save progress/100% tracker has not yet been proven. The existing Save Mutation API is not part of the authorized product scope: Saves remain read-only. No current user saves opened in this sample.

## B0: own overlay writer

[PackGroupBuilder](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paz.rs) implements the required basic operations: compress/encrypt plain text, write PAZ chunk, capture offsets/flags/sizes, set up trie indices, generate chunk checksums and PAMT checksum. [PAPGT add_entry/to_bytes](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/papgt.rs) registers/upsets a group in front, moves name offsets and recalculates the PAPGT checksum. The claimed engine search order “first hit wins” is documented upstream, but not confirmed here by a local application/start attempt. **Overlay base format green; local engine acceptance for all planned tables yellow.**

PAPGT: 12 byte headers; `entry_count:u8`; per entry `is_optional:u8`, `language:u16`, `always_zero:u8`, `name_offset:u32`, `pamt_checksum:u32`; then `i32` length and NUL-terminated name buffer. The real registry contains **39 entries**, Jenkins-Checksum `0x953ce06a`. Existing are **35** numeric group folders, all registered. `0036`, `0038`, `0039`, `0040` are already registered optional groups without folders. Therefore, missing folders are not free IDs. The real general speech mask is **0x7fff**, while older documentation/default values partly call `0x3fff`. Group selection and language mask derived from registry and actually supported languages.

The reference is not a finished secure apply manager: PackGroupBuilder writes directly into its target, has no product-wide transaction, process verification, vanilla certification, backup/recoveryZustandsmach or third-party mode property management. B0 must implement these tasks itself. Registry-Upsert on an already modified PAPGT leaves old names in the Raw name buffer; rebuilding from the unchanged snapshot meets "rebuild, don't stack". `u8`-Entrycount, `u16`-Chunk ID and `u32`-Offsets/sizes.

## Why the March Unpacker is just a supplement reference

[PAZ_DECRYPTION.md](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/PAZ_DECRYPTION.md) describes deterministic basename keys, ChaCha20 and LZ4 block data as well as historical size-fixed in-place repacking strategies and timestamp conservation. These constraints were neither adopted nor tested here as a general engine rule; they are not a proven compulsion for B0’s own overlay group.

Concrete inconsistencies/limitations: The document calls compression 3 "custom" and 4 "zlib", while the newer crimson-rs source code uses 3 Zlib/4 QuickLZ. [paz_parse.py](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/python/paz_parse.py) considers only `.xml` encrypted; However, the locally confirmed `.paloc` entry is also ChaCha20 encrypted. Its basename key recipe does not have the general PAMT-`encrypt_info` parameterization of the current Rust reference. Therefore, do not adopt the Unpacker as the current universal extractor/writer in cd-core. Its C++/Python programs were not built or applied to current game files; the current extraction was carried out via the tested Rust reference.
