# Phase 0 – Formats and Local Build

**Supplement phase 2:** Desktop item browsers and real Item icons are implemented. Additional `stringinfo` (31,812 Records) is SHA-certified and secured by typed body/header round trips; Details in [DESKTOP.md](DESKTOP.md). Dealer/drop/prescription relationships remain explicitly unclear.

**Supplement phase 1:** The native core is now implemented and tested; See [CORE.md](CORE.md), [FORMAT_PORT.md](FORMAT_PORT.md) and [PROGRESS.md](PROGRESS.md). The following sections remain the research report from phase 0. Re-verified: all 14 table indices with body/header roundtrip, typed item info, 15 item languages and PAPGT/PAMT in their own Rust core. Quest/Stage headers count with `u32`, the other recorded headers with `u16`. 13 Table bodies remain uninterpreted bytes, without semantic editor sharing.

Status: 19.09.2026. The basis is the updated `SPEC.md` with its own apply engine. **Observations are not vanilla certification.** The real installation has been read exclusively; Reference tools were built only in isolated project directories. Save contents have not been studied.

The complete audited revisions and licensing decisions are in [CREDITS.md](../CREDITS.md). Detailed documents: [Archive](research/ARCHIVES.md), [Tables](research/TABLES.md), [Apply/Overlay](research/APPLY.md). Any statement about older schemes remains tied to their source status.

## Installation monitoring

|Characteristic|In fact, it was established that:|
|---|---|
|Plattform|Steam, App ID `3321460`, display name "Crimson Desert Enhanced"|
|Installationsordner|`C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert`|
|Steam-Build-ID|`25381195`|
|Main programme|`bin64/CrimsonDesert.exe`, 397,242,776 Bytes|
|EXE file/product version|`1.0.0.2944`|
|EXE SHA-256|`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`|
|Meta-Registry|`meta/0.papgt`, 679 bytes; 39 Registered groups|
|Registry SHA-256|`c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea`|
|Registry-interner Jenkins-Hash|stored and calculated each `0x953ce06a`|
|Existing numerical groups|35; All registered in PAPGT|
|Registered, without folder|`0036`, `0038`, `0039`, `0040` – optional groups, no freely available IDs|
|Static tables|Gruppe `0008`, `gamedata/binarystaticinfo__/bin/`|
|German localisation|Gruppe `0027`, `gamedata/stringtable/binary__/ger/`, 39 Namespaces|
|Actual Save tribe|`%LOCALAPPDATA%/Pearl Abyss/CD/save`, 11 `.save` files found|

The save path under `CrimsonDesert/Saved/SaveGames` mentioned by way of example in the spec does not exist here. Steam was found directly via library/appetam data. Epic/Game Pass detection is not yet an implemented or verified feature.

The [Machine-readable observation report](builds/steam-25381195.observed.json) Contains 38 Original file hashes (EXE, PAPGT, PAVER and 35 PAMTs, 28 extracted table file hashes, localization data for integrity checking and the real round trip results. **No table content, assets or savebytes**The 28 Table files are Bodies and headers of `characterinfo`, `crafttoolinfo`, `crafttoolgroupinfo`, `dropsetinfo`, `fieldinfo`, `inventory`, `iteminfo`, `multichangeinfo`, `questinfo`, `regioninfo`, `skill`, `stageinfo`, `storeinfo` and `vehicleinfo`Further tables to be used in the future need to be supplemented.

The data matches the 2.03 layout described by crimson-rs. However, the EXE version is not the same as the publicly designated patch version; It will not be renamed accordingly. There is no released Workbench scheme registry yet. Unknown hash combinations must be rejected in phase 1.

## PAZ, PAMT and Group Registry

The numeric group folders contain `0.pamt` and numbered `.paz` chunks. PAMT performs paths/names, chunk mapping, offsets, compressed/uncompressed sizes and flags; PAZ contains the useful data. Metadata and Payload are read separately. Boundschecks must apply before allocation, seek, decryption and decompression.

The [Unpacker-Spezifikation](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/PAZ_DECRYPTION.md) and the current Rust reader describe ChaCha20. The older recipe uses a lowercase base file name and fixed derivative constants. The actual [Rust-Code](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/chacha20.rs) checked here uses the basename checksum **plus three PAMT-`encrypt_info`-bytes** and does not perform a lowercase normalization itself. Therefore, adopt the archive name unchanged; Remove directory portion. Counter/Nonce semantics must match the Rust ChaCha variant used. The historical fixed recipe is not a universal current key derivation.

The current Rust code assigns the lower flag nibble as 0=None, 1=Partial, 2=LZ4, 3=Zlib, 4=QuickLZ; The March documentation deviates from this at 3/4. None/LZ4/Zlib and some partial variants are implemented, QuickLZ is not. Encryption is in the upper nibble: None/ChaCha20 is supported, ICE/AES is not. Unknown flags must expressly reject Workbench instead of accepting the partial none-fallback of the reference parser. Do not assume an LZ4 frame and do not guess the codec from the same / unequal length alone. The existing Rust implementation supports multiple header/DDS mip partial compression cases, but this does not result in universal texture support. Local table extraction succeeded; A complete world map texture test has not been performed.

`meta/0.papgt` registers groups with an optional flag, voice mask and PAMT hash. The referenced setup has a 12 byte header, 12 byte group records and a size-defined name table. In the local build, the general voice mask is `0x7fff`; older sources and some builders use `0x3fff`. Neither this constant nor group-counting heuristics are blindly adopted.

For overlays, CDUMM`hashlittle` describes PAZ data and PAMT/PAPGT byte 12, seed `0xC5EDE`; the group registry references the correct PAMT hash. These 32-bit game checksums are **not** the SHA-256 build/backup signatures. Exact layouts and source lines are in [APPLY.md](research/APPLY.md).

## Tables and the September Changes

The old internal path `gamedata/binary__/client/bin/` has been replaced by `gamedata/binarystaticinfo__/bin/`. Body/Header are now called `.staticinfobody` and `.staticinfoheader` instead of `.pabgb/.pabgh`. This is confirmed locally. [CDUMMs table_ext.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/table_ext.py) dates the change to 04.09.2026; NattKhs`table_layout.py` is called 2.01.00.

Bodies contain differently constructed records with lengths, strings, arrays and optional/tagged substructures. Headers are key→bodyoffset indices; the key width is table-specific. For Iteminfo, the familiar pattern is `u16 count + N × (u32 key, u32 offset)`; Other tables use u16 keys. Memory layout offsets from IDA are not file offsets. Unknown fields, order and padding remain unchanged raw.

For size changes, the appropriate header must be set up from the ** newly serialized** records. A vanilla header next to a grown body can silently address false items even when the game starts. Therefore, body and header are treated as a related test and output item.

### Shops

For its pinned September build, the [current CDUMM store source](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/storeinfo_native_parser.py) describes eight new raw bytes between the optional `sub_data` and `effect_list` count as well as an additional byte in the store section. The eight bytes have no confirmed technical significance. Older NattKh parsers take 105 or 113 Bytes and contradict each other partially in read/write offsets. This is **not a universal fixed-size record contract**. The local store table is present/hated; A complete store round trip and the current stock/refresh semantics have not yet been proven.

### Iteminfo

The local body has 6,465,724 bytes and 6,816 items. Its SHA-256 is `87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce`. `parse_iteminfo_from_bytes` → `serialize_iteminfo` of the pinned crimson-rs resulted in **byteidentical output**. Each `inventory_info_list` read has ten entries, matching the 2.03 extension versus nine. The table header is hashed; its independent parser/serializer proof is still a Workbench test order.

Successful serialization does not prove the effect of all stat fields. In particular, B11 requires validated statname spaces and compatible buff/item combinations. Lossy parsers that skip unreadable records do not comply with the spec and may not create editors or build releases.

## Localisation

Since the current layout, language files are in namespace files per language. Local is German `ger` in group `0027`. The tested `item.paloc` has 649,912 bytes, a `paloc` header of `0x200` bytes and an LZ4 block of 649,400 bytes, which decompresses to 1,807,930 bytes. In it, 13,575 entries were read.

The **decompressed PALOC payload** can be parsed/serialized byte-identical. This is not evidence of identical LZ4 recompression or a round trip of the entire packaged file. For the planned reading localization, the checked decoding is first sufficient; The original container remains unchanged. Additional namespaces/languages and all item-key links require integration tests.

## Saves and Progress

The [Save-Formatquelle](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/mod.rs) describes a 0x80 byte header with `SAVE` magic, version/flags, size fields, 16 byte-nonce and HMAC-SHA256. The body is decoded ChaCha20 and decoded as LZ4 block; HMAC checks the decrypted compressed bytes. Version-dependent keys and other raw header fields must be observed. The reference can return a body with `hmac_ok=false` despite the wrong HMAC. Workbench shall not treat it as reliable read progress; the integrity check requires an explicit error state and repeat reading.

Quest/mission/stage links are known from sources. Which save states in the **build** installed here correspond to which 100% category was not checked. Also, a complete collectible coverage is not occupied. Local saved games have not been copied, decrypted or modified. For phase 7: Shared read-only opening, consistent repeat reading with parallel storage, no save-writer API from references.

## Own application engine and reference errors

CDUMM serves only as a source reference. own numerical PAZ/PAMT groups are registered in PAPGT; According to the current reference code, the first matching entry wins. That’s why its builder puts overlay entries before vanilla. The numerically highest group number alone is not a proof of priority. All active modules must be merged per table path before output; The game does not make a field merge.

NattKhs [MODDING_GUIDE](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/blob/96e7f78fcb00d6cb5615171a7f359f0c2de5b114/CrimsonGameMods/MODDING_GUIDE.md) reports older errors with non-numerical groups, old header offsets and commonly packed `iteminfo`/`equipslotinfo`. These indications become regression tests; his old paths, language masks and supposedly free group IDs are not current requirements. Other managers can remove unknown groups – this must later be recognized as a foreign Zustandchange.

The local registry is consistent, but the overlay rank was deliberately not checked by game directory writing attempts. [APPLY_DESIGN](APPLY_DESIGN.md) describes backup origin, journal, crash recovery, running game lock also for restorers as well as the limits of a mere tem file name. In-place patching is not an implemented fallback. `meta/0.pathc` does not need to be changed for pure table overlays after the reference.

## Integration decision and test limits

crimson-rs is MIT-licensed, built locally with Rust 1.95 and suitable for the tested Itembody/PALOC payload. However, it only provides `cdylib`/`staticlib` and private Rust modules. An unchanged pinned Git-Dependency therefore does not offer the desired public Rust-API. Phase 1 should decide a small maintained fork with `rlib`/public core modules or a limited, licensed porting. Python/C-ABI served here only the reference proof.

The reference tests reported 354 passing, 46 ignored, 0 failed. Some live tests report early success internally without fixtures; This suite run is therefore not a comprehensive real file proof. Separately actually checked Itembody, `0008/0.pamt`, `meta/0.papgt` as well as the German Item PALOC payload. No app, frontend, apply, restore or in-game tests were performed. A full PAZ backup snapshot was not created in phase 0 and is still required before each subsequent write authorization.
