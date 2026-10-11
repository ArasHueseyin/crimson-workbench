# File Writer on Private Buffers – Module 0.21.0

Status: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **34 passed additional native scenarios.** All file operations including flush and close are callbacks of their own; The handle is just a private number. No save files or running game objects were used.

Historical evidence 0.21.0. The native buffer preparation and compression are now tested separately in [SAVE_ENCODING_2949.md](SAVE_ENCODING_2949.md). Downstream processing and actual file operations remain open.

## Original exported code

|Function|RVA / full length|Implementation|
|---|---|---|
|Write a pre-marked entry|`0x2358700 / 0x2c9`|Preparation, opening, writing, flushing, cleaning and branches of error|
|Empty/release byte buffer|`0x1373a40 / 0x8e`|Release original payload after success as well as remaining test buffer|
|Share file object string|`0x394db0 / 0x8e`|Only the empty sentinel branch actually used by the test|

Three additional functional hashes and pdata entries, four private TLS replacements. Total manifest: **91 code ranges, 118 pdata fragments, 55 TLS locations**. The TLS location of the string destructor and the heap path for long formatted names are bound, but in these cases do not pass through. Language-specific exception cleanup handlers of the game will still not be registered.

The write assistant gets `(Handler, Verzeichnis, Eintrag)`. The trader refers to his owner at `+0x10`; its string at `+0x78` is passed on to the encoder. Entry names and this string can be inline or external. The pre-marked byte buffer is at `+0x120` at the entry.

## Cases under review

**26 Direct cases:** 13 Variants in both private TLS allocation modes:

- regular flow with inline strings as well as with both external strings;
- preparation error without and with already applied temporary buffer;
- opening errors, writing errors and opening errors with simulated residual handle;
- failed flush, failed close and both together;
- empty output without allocation and empty output with reserved buffer;
- 4,096 Byte payload data, passed to the write receiver with byte precision.

**Eight integration cases:**Success, opening error, spelling error and flush error in both allocation modes with the original [Storage dispatchers](SAVE_BACKEND_2949.md). A separate bridge passes exactly one entry. It replaces the original queue/backup/renaming path.

It reviews arguments, call order, results, unchanged owners, buffer content and protection bytes, and full release. The original Clear resets the pointer, capacitance and length, giving the allocation tag `0xff`. The preparatory test function does not change the input buffer. The proof that it is retained in case of opening/writing errors applies to **this binding**; The real encoder can change the input buffer beforehand, see below.

## Why success does not yet confirm storage

In the case of successful opening and writing or empty output:

1. Flush is called; its outcome does not affect the process.
2. The original Clear releases the pre-registered payload.
3. Close is called; Its outcome does not affect success either.
4. The helper returns the truth. The associated dispatcher sets his
Success status and deletes dirty even in the simulated flush error.

In case of opening/writing errors, the helper delivers false and does not call a flush. The dispatcher then gets dirty and deletes a previous success status. Temporary output allocations are also released after preparation errors.

These are statements about original code with private recipients. Neither a real write process nor a disk closure was confirmed. As a result, the productive repair adapter does not receive approval to use this bool return as a correlated success message.

## Next bindings, previously only statically read

`0x235c750 / 0x31a` prepares the file buffer. A flag at entry `+0xa4` decides on an additional processing step. When the mask is set `0x02`, the code calls `0x12c61e0` and then replaces the original payload; `0x235c040` is another processing step. Only then are lengths set at `+0xaa`/`+0xae` and a 128 byte header plus useful data output. The exact coding and its error/possession semantics still have to be tested natively. In particular, an error before the file is opened is not a general proof for unchanged pre-recorded useful data.

`0x12b7dd0 / 0xe3` is the downstream write wrapper. After successful `WriteFile`, he compares the actually written with the requested byte number; a subset is treated as an error. Also this wrapper and its notification are so far only statically read.

Other open points: complete serialization from ItemSaveData, original queue/backup/renaming logic, valid time and thread, reliable proof of completion for the repair order, and game host and installation.

## Result and later in-game tests

MSVC Release `/W4 /WX` and **nine CTest Suites** passed. Total **352 native scenarios, 4,095 calls and 142,649 conditions**. Production suites unchanged: 35 Writing scenarios/202 Conditions, 43 Transaction scenarios/273 Conditions. The test result reports `save_file_io=private_callbacks`, `save_disk_commit=false`, `engine_commit=false`, `live_installable=false`.

Later [R06 and R14](../../TESTCHECKLISTE.md) apply: compare repaired main/socket values and colorings before/after repair as well as after regular saving and normal game restart. Technical error cases are simulated here in the test program; the user should not generate power outages or save damage. The repair remains locked in app v0.5.9.

Sources: [native Probe](tests/save_file_native.inl), [Functional manifest](tests/registry_native.cpp), [Wind test](src/pe.cpp). Evidence: `.local/verify_save_file_v21.py`, `.local/repair-runtime-v21-save-file.json`, `.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.
