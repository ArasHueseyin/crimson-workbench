# Native format port for phase 1

Phase 2: `ItemRecord.stat_keys` collects actual status references from `enchant_data_list` and `sharpness_data.stat_data`. Key wrappers retain their type name in tracked fields (e.g. `ItemKey`) instead of just `u32`; Values and byte ranges remain unchanged. This creates links exclusively from typed references. The additional Stringinfo parser in the core is a proprietary implementation according to the MIT format documentation from the same original commit; 31,812 real strings and both files pass their round trip. Details under [DESKTOP.md](DESKTOP.md).

`vendor/crimson-format` is an internal Rust crate without Python, shipped C-ABI, GUI or Apply function. It provides only read file accesses and serialization in memory. The calling `cd-core` must check the known build/table hashes before use and must not equate parser success with a vanilla proof.

## Origin and decision

Format and item base: [bbfox0703/crimson-rs, Commit b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0), commit time 2026-09-18 16:51:44 +08:00. Copyright 2026 Tommy Tran, MIT; The full license text is in [vendor/crimson-format/LICENSE](../vendor/crimson-format/LICENSE).

The reference build and the current 2.03 itemparser are working. However, the unchanged Git-Dependency has no `rlib` output and no public Rust core API. Therefore, the limited MIT port documented in phase 0 was chosen, no new external fork/publish dependency was created. No foreign manager was involved and no game content was taken over.

Adopted/adapted were exclusively:

- `src/binary/{mod,primitives,arrays,types}.rs`: primitive types, structured
BinaryRead/BinaryWrite and full bytefield ranges.
- `src/item_info/{mod,keys,item,structs}.rs`: Typed 2.03 item structure.
- `src/crypto/checksum.rs`, `src/crypto/chacha20.rs`: Jenkins and PAZ-ChaCha20.
- The reading partial compression section from `src/binary/paz.rs`:
header LZ4 and DDS mip variants; its upstream format source notes are retained. No PAZ builder/file system writing functions were taken over.
- Addition of 03.10.2026: `src/save/` same MIT revision for headers,
ChaCha/HMAC, compression, TOC and structured save objects including serialization in memory. Additional license copy: [LICENSE-save](../vendor/crimson-format/LICENSE-save). HMAC errors are rejected before decompression, file and user data lengths are limited to 128 MiB; Surplus file bytes are rejected. The calling core must reject unknown areas and missing exact round trips before write operations. Permanent stable registration is a separate extension commissioned by the user; The planned general save tracker remains reading.

`archive.rs`, `items.rs`, `localization.rs` are own limited facades and container readers based on the formats occupied in [ARCHIVES.md](research/ARCHIVES.md). PAMT/PAPGT are read structurally and re-serialized with obtained name/trie buffers; `serialize_registry` and `serialize_group` are not returns of the read-in byte vector. The SHA-256 build/localehashes come from the Workbench/verifier layer, not the 32 bit game checksums.

## Public interfaces

All failing methods yield `std::io::Result`.

```rust
Archive::open(root: impl AsRef<Path>) -> Result<Archive>
Archive::groups() -> &[ArchiveGroup]
Archive::list_group(group: &str) -> Result<Vec<ArchiveEntry>>
Archive::extract(entry: &ArchiveEntry) -> Result<Vec<u8>>
Archive::serialize_registry() -> Result<Vec<u8>>
Archive::serialize_group(group: &str) -> Result<Vec<u8>>

ItemTable::parse(body: &[u8], header: &[u8]) -> Result<ItemTable<'_>>
ItemTable::entries() -> &[ItemRecord]
ItemTable::fields(key: u32) -> Result<Vec<RawField>>
ItemTable::serialize_body() -> Result<Vec<u8>>
ItemTable::serialize_header() -> Result<Vec<u8>>

Paloc::parse(bytes: &[u8]) -> Result<Paloc>
Paloc::entries() -> &[PalocEntry]
Paloc::decoded_payload() -> &[u8]
Paloc::original_bytes() -> &[u8]
Paloc::serialize_payload() -> Result<Vec<u8>>
Paloc::serialize() -> Result<Vec<u8>>
```

`ItemRecord` contains keys, internal name, type/tier/category, stack number, name/description references, first icon reference, ten inventory keys and body offset/length. `LocalizableText.index` is the direct numerical PALOC string key. For example, Item2200's name index `9448928051312 = (2200 << 32) | 0x70` and description index `9448928051313 = (2200 << 32) | 0x71` are local. Descriptions must not be filled with the namenamespace.

Raw fields are decoded for each item requested so that the normal index does not hold more than one million field objects in memory. Each `RawField` carries path, absolute body offsets, type designation, actual hexbytes, and decoded value. Large u64/i64 values are output as a decimal string in the raw field value to prevent JavaScript precision loss. Non-final f32s are displayed as text; The exact bits remain in `raw_hex`. `interpretation="unknown"` identifies unknown fields and the known upstream conjecture of the Quickslot flag. `upstream_named` expressly does not mean confirmed gameplay semantics.

The optional `RawTable::parse(body, header, key_width, count_width)` facade only checks explicitly given index widths, keys/offsets and obtained opaque records. Your byte content is **no** proof of a understood body scheme and no editor release. cd-core also has its own build versioned index description for the other recorded tables.

## Hardening compared to the original

- Tested addition before slice accesses; Count, offset, header, and actually
Existing PAZ file lengths are validated.
- A file/unpacked payload is limited to 128 MiB, PAMT/PAPGT to 16 MiB,
an item record on 4 MiB; Generic array reservations on 16 MiB per array. Together, resolved archive paths have a maximum of 16 MiB, one path has a maximum of 4096 bytes/256 trie steps. PALOC takes a maximum of 250,000 entries.
- Tries are read iteratively with cycle recognition. directory crossing,
absolute virtual paths, NUL/control characters and Windows drive syntax are rejected; Canonized real reading paths must be below the game root.
- `extract` does not rely on `ArchiveEntry` offsets manipulated by the caller:
Metadata is read/compared again from the PAMT checked against PAPGT.
- Codecs 0/1/2/3 are explicitly treated; QuickLZ and unknown codecs as well as
ICE/AES are rejected on extraction. Enumeration may display it diagnostically. Zlib output is limited to expected length plus one test byte.
- ChaCha20 uses fallible seek/keystream functions. DDS dimensions;
Mip number and size multiplications are limited/tested.
- Upstream’s allocated/leaked replacement strings were replaced by `Cow<str>`.
Non-UTF 8 bytes are preserved for the writer. Optional tags are kept original instead of normalizing each tag from zero to 1.
- Each item record must exactly match the key and area recorded in the header.
and already pass a real typed writer round trip when loading. No anchor search, no discarded records, no restbytes.
- The tracker of the item fields must cover each byte exactly once. One
Gap/overlap errors are reported instead of an incomplete export.
- PALOC checks Magic, header padding, sizes, LZ4 and the entire list area.
His payload is actually being re-serialized. For unchanged containers, the original LZ4 block is kept; Identical recompression is not claimed. In phase 1, there is no PALOC mutation interface.

At the file level, only `File::open`, `metadata`, `seek`, `read` and `canonicalize` are used. Production code does not contain `File::create` or file writing. Synthetic tests produce only temporary test archives. No save reader/writer was imported into this crate.

## Verifikation

`cargo clippy -p crimson-format --all-targets -- -D warnings` passed without warnings. 15 passed own synthetic tests: encrypted LZ4 archive by PAPGT/PAMT/PAZ up to plain text, real metadata round trip, manipulated entry offsets, truncated real chunk length, incorrect checksums, trie cycles, path/codec errors, header limits, unreadable items, PALOC-LZ4 and Raw-UTF-8 retention, optional tags, huge counts and DDS dimensions. No adopted live tests, which count silently as a success in the absence of fixtures.

Explicit native real file verification at 19.09.2026, only reading:

- 6,816 Items; 6,465,724 Bodybytes and 54,530 Headerbytes byte-identical by
Parser/Writer.
- All **1,763,837 field ranges** of all items completely and without overlap.
- `meta/0.papgt` and `0008/0.pamt` by structured writers byte-identical.
- German Item PALOC: 13,575 Entries; Payload and original container received
byte-identical.
- All 15 registered item language namespaces successfully with the native
Archive/PALOC facade read, checked and recorded as pure hash metadata in `crates/cd-core/schemas/steam-25381195.locales.json`.

`cargo run -p crimson-format --example verify_formats` explicitly requests `CD_GAME_DIR` and checks the current files; If the variable is missing, the explicit verifier fails. `fingerprint_locales` only outputs metadata on stdout. The actual optional real file tests and `.env` loading are coordinated by cd-core. No example writes into the installation or opens saves.

Not proven: other item schemas/builds, all partial texture variants, QuickLZ/AES/ICE, complete PAZ chunk checksums over all large archives, Apply or Save progress. Linux source code is platform neutral; An actual Linux run is not proven by Windows builds.

Phase 5 adds `item_mods.rs` as its own limited facade over the existing item reader/writer (Stats/Buffs, since v0.5.1 new enchant lines from original stages, stacks and repair lists). Copies receive complete line contents and renew the zero-valued separators. The facade does not have a file system writing API. `cd-core` checks status/buff references, build, experimental stack release and B0. Other readers are their own implementations; [Evidence](research/ADVANCED_TABLES.md).
