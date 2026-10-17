# World map fog reveal

Local implementation for game build **1.0.0.2976**, 17 October 2026. No actual
game, save, archive or installation was modified. In-game acceptance is pending.

## Evidence

The pinned executable SHA256 is
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
A private authenticated historical save has a unique `FogSaveData` block with
one `_fogRLESaveData` reflected child, `SimpleRunLengthEncodingSaveData`.
Its fields are `_jumpingTable` (u32), `_codeList` (u8) and `_lengthList` (u8).
All have the observed generic dynamic-array encoding. The save's 30,301 runs
decode to exactly **524,288 bytes**, or **4,194,304 bits**.

The following native paths establish the representation and its meaning:

- Save writer `28bb540`, particularly `28bcbb9..28bcecb`, reads the component's
  `+260` bitmap and run-encodes all `80000` bytes. A run stores a code byte and
  a positive one-byte length, splitting equal runs at 255. It builds
  `min(run_count / 64, 1024)` index entries with stride
  `run_count / index_count`, then sets the final index to `80000`.
- Save loader `28b7360`, particularly `28b8735..28b88a5`, pairs the saved code
  and length arrays, expands the runs into an `80000`-byte buffer and copies
  it into the component's `+260` bitmap. It does not require neighboring runs
  with equal codes to have been merged.
- Client fog update `495ca0..496300` tests each incoming bitmap bit. Set bits
  lower the corresponding rendered fog value toward zero. The client image
  has `400000` bytes. This establishes that setting a saved bit reveals it.
- The native fog-reset request `29799c7..2979a08` clears the server bitmap to
  zero; its client acknowledgment `b63cfe..b63d56` fills the rendered image
  with `ff`, confirming the opposite, unrevealed state.

Bounded witness digests, using RVAs in the pinned EXE:

| Start | Length | SHA256 |
| --- | ---: | --- |
| `28bcbb9` | 786 | `45a59f8dc2d78883a111216e772ff785ae09fe2f76656c636339170922ee9dd5` |
| `28b8735` | 368 | `b0e2f98646e4d5cc7341603cf7d26ac38c65b056245077fccce2c025ccffbac3` |
| `495ca0` | 1632 | `4a46f5af3b5f994f3c62a8fe61714a0970cf5e4c38c776aabb6c7097522149de` |
| `29799c7` | 65 | `148379ae13f6b68533247443f2d591f231ec398db3770a343f3037f746234d18` |
| `b63cfe` | 88 | `51203ccf754892e1e1c0fc5c110a2a5281eb63ab5cc42e1380f20eb9fd529896` |

The disassemblies and original game/save bytes remain in ignored private
research directories; they are not application resources.

## Transformation and transaction

`crates/cd-core/src/map_fog.rs` authenticates and completely decodes the save,
checks its byte-identical structured roundtrip and the exact fog schema,
positive run lengths, decoded size and native jump-index construction.
It fills only the existing `_codeList` payload with `ff`. Lengths, jump
indices, field headers and all other save-body bytes retain their original
values. A second authenticated decode checks full reveal; restoring only the
original code payload must reproduce the complete original body byte for byte.
Keeping the original partitions avoids shifting any other save data.

The English Marketplace save action selects one slot, reviews the revealed fraction
and requires explicit confirmation. Its plan binds both authenticated save-file
hashes. Contribution edits and map reveal share the existing locked save-pair
transaction kernel: pinned closed-game check, guarded paths, exclusive original
files, backups, intent/receipt recovery and idempotent request IDs. The lobby
bytes remain unchanged. Map backups use the separate `.local/map-reveals`
history. A saved frontend request retains its original slot and hashes during
recovery; changing the selected save invalidates the displayed review.

Quest, knowledge, discovered-location, travel and other blocks are not edited.
Removing fog does not promise to unlock map markers or fast travel. The game's
normal behavior when loading a fully revealed bitmap still needs acceptance
testing. No runtime module or restart was needed to develop this save action.

## Local validation

- Both native RLE boundary tests pass, including the 1,024-entry index cap.
- The authenticated historical-save fixture passes: reveal, complete inverse
  body comparison, repeat/no-op, stale save binding, altered authenticated lobby
  binding and malformed lobby refusal. Original fixture files are unchanged.
- The temporary save-pair test passes: backup contents, unchanged lobby,
  idempotent retry, missing-receipt completion recovery, stale/conflicting request
  refusal and rollback to the original pair. Only temporary copies were written.
- The existing contribution transaction fixture still passes after sharing the
  transaction kernel. Its serialized request, intent and receipt formats remain
  compatible with the earlier local implementation.
- Core library suite: **194 passed, 17 ignored**. The private save and transaction
  fixtures listed above were also run explicitly. App Rust checking and the
  frontend production build pass; the existing large-chunk warning remains.
- **33 Marketplace browser tests** pass. The three map tests were rerun after
  simplifying the displayed percentage, and the dialog was visually inspected.

Game acceptance still needs the full map, normal markers/travel/quest behavior,
save/reload persistence and normal continued exploration after the change.
