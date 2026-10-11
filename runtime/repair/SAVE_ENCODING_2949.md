# Native storage buffer and compression – module 0.22.0

Status: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`. **66 passed additional native scenarios.** Original buffer preparation, allocation and compression run with private entries. The downstream processing step continues to be its own callback. file operations remain simulated; does not use save files or running game objects.

## New original functions

|Function|RVA / full length|
|---|---|
|Prepare storage buffers|`0x235c750 / 0x31a`|
|Byte buffer dimensions|`0x1326490 / 0x142`|
|Allokationswrapper|`0x13737c0 / 0x69`|
|Kompressionswrapper|`0x12c61e0 / 0x35`|
|Compression body|`0x12c4fe0 / 0x11fd`|
|Initialize the compression state|`0x12c6220 / 0x31`|
|Windows-Stackprobe|`0x4896270 / 0x4e`|
|16-Write bit offset|`0x12c4fc0 / 0x4`|
|Count matching bytes|`0x12c4fd0 / 0x9`|

Nine more feature hashes, seven pdata entries and three private TLS replacements. Total: **100 code ranges, 125 pdata fragments and 58 replaced TLS locations**. The stack sample reads unchanged `GS:[0x10]`, i.e. the stack limit of your own Windows test thread. She doesn't read game TLS.

The adjacent six-byte-long CRT hops for memcpy/memset are each given a six-byte-long private diversion with a separate function pointer. A longer trampoline would overwrite the next jump point. All code pages remain RX, all used pointer pages R; No RWX sites.

## Checked content and boundaries

- **40 Size/compression cases:** 0, 1, 12, 13, 255, 256, 4,096, 65,546,
65,547 and 131,072 bytes, each with/without compression and in both TLS modes. repetitions and deterministic random bytes; Both native branches around the limit `0x1000b` are reached.
- **Four reuse cases:** keep sufficient output allocation and
Replace too small an output allocation; Content, length, capacity and release checked.
- **Six error cases:** Compression errors with raw data still unchanged;
downstream error with/without prior compression. The output descriptor remains unchanged. Already successfully compressed input buffer remains compressed.
- **14 Integrations with the original file helper:** normal with/without compression,
Open, write, flush, compression and downstream processing errors in both TLS modes. Actual file operations are their own callbacks.
- **Two repeat cases:** same private entry again after opening error
Calling. The second pass compresses the already compressed content again and enters its length as a new original length.

Compressed bytes are independently translated back into the original byte sequence with a small, limited test decoder. The decoder uses the [official LZ4 block description](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md); It is not a copy of the native compressor and not a productive save reader. The fixture limits its allocations to 262,144 bytes. This does not result in a general game or save size limit.

The encoder sets the original and processed length when entering `+0xaa` and `+0xae`. The output consists of the 128 bytes from `+0x98` followed by useful data; the buffer has two additional zero bytes outside the useful length. Identity, other headerbytes and protected areas are preserved. Both allocation modes fully release all temporary and remaining buffers.

## Importance for repairs

The original raw data can be replaced by its compressed representation even before a file is opened. An opening error leaves this Zustand. Repetition of the same entry is therefore not a neutral repeat: the test confirms double compression and a changed size. A new repair/storage attempt must recreate its raw data or use the engine’s fully occupied repetitive path. No automatic repetition is introduced into the productive repair adapter.

The already checked [File helpers](SAVE_FILE_2949.md) can still report success after a failed flush and release the payload. A Boolean return or a emptied buffer alone therefore confirms neither the specific repair order nor a data carrier conclusion.

## Still open

`0x235c040 → 0xee36650` is replaced by a controlled, normally unchanging callback. Its original code runs through distributed and indirect jumps. The 44 target beginnings considered so far are **incomplete block windows**, not additional tested or executable functional bodies. Encryption, authentication and their key/nonce management are not proven.

Other limits: allocation errors, allocation tags except `0xff`, full object serialization, original queue/backup/renaming path, game host, time/thread, real event recipients, and unique completion feedback. The compression error sample enforces the error by a callback; it does not claim such an error with correctly dimensioned native compression.

## Results

MSVC Release `/W4 /WX`, nine CTest suites and a total of **418 native scenarios, 4,289 calls and 204,378 conditions** passed. Many conditions are repeated byte/manifesto tests. The unchanged production suites report 35 writing scenarios/202 conditions and 43 transaction scenarios/273 conditions. `save_postprocess=fixture_callback`, `save_compression=native_lz4`, `save_disk_commit=false`, `engine_commit=false`, `live_installable=false`.

The later in-game validation remains [R06/R09](../../TESTCHECKLISTE.md); technical errors and repetitions are simulated in advance on own data. Phase 5 remains open, app v0.5.9 and game installation remain unchanged.

Sources: [native Probe](tests/save_encode_native.inl), [Functional manifest](tests/encode_functions.inl), [Testarena](tests/registry_native.cpp). Evidence: `.local/inspect_encoder_v22.py`, `.local/repair-runtime-v22-encoder-dependencies.json`, `.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.
