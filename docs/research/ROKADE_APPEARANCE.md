# Dedicated Rokade appearance

Local implementation checkpoint: 17 October 2026. Build 1.0.0.2976 only.
This expansion has not been installed into the running game. Visual acceptance
in the game remains pending.

## Scope

CharacterInfo **31378**, `Animal_Black_Horse_Wild_31378`, references StringInfo
**3219883613** at byte 251 of its pinned character record. That string resolves to
`character/appearance/4_riding/cd_r0002_00_horse/cd_r0002_00_horse_01007.app_xml`.
Its character scale is **1.13**. A 30% increase gives **1.469**.

Replacing that existing file could affect other characters using the same
appearance. The recipe instead derives a separate appearance from the user's
original file and changes only Rokade's reference:

- New file: `cd_r0002_00_horse_workbench_rokade_130.app_xml`, in the same directory.
- New StringInfo key: **2357531225**, verified absent from the pinned source table.
- Preserve every original StringInfo record and index entry. Append one record,
  increment the 16-bit header count and append its key/offset pair.
- Preserve the original shared appearance file. Change only its cloned
  `CharacterScale` attribute. Head scale, prefabs, audio and other XML remain
  identical when the scale replacement is reversed.
- Redirect only CharacterInfo 31378. Existing reviewed speed/stamina changes to
  that row compose with this change; other character rows remain unchanged.

No original game asset bodies are checked into the repository. The recipe pins
the character row, string table body/header and appearance by SHA256 and refuses
changed sources. It derives the result from the user's own game archive.

## Native format evidence

Read-only static inspection used the same executable fingerprint as the other
2976 recipes:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
Addresses below are RVAs in that executable, not portable addresses.

- StringInfo manager global: **0x6d69a80**. The indexed resolver at **0x388e00**
  reads its 16-bit resolved index and accesses the corresponding record.
- Header loader **0x25cbfa0**, especially **0x25cc210–0x25cc2db**, reads serialized
  32-bit keys and 32-bit offsets, retains their order and inserts the keys into
  the manager's key-to-index map. It does not recompute keys from asset paths.
- A native consumer at **0x20c1cde–0x20c1dd8** accesses the same manager, looks up
  a serialized 32-bit key in its hash buckets and obtains the resolved index.

This supports allocating a new, non-colliding serialized StringInfo key. It
does not establish that the new appearance has been rendered successfully in
the game; that remains an acceptance test.

## Packaging and checks

The character/string table pair overlay and the dedicated appearance overlay
share one reviewed plan. Appearance registration preserves source group 0009's
language, while the table overlay uses group 0008's language. The existing
transaction engine applies and restores them together, with the game closed.
The raw appearance allowlist accepts only the dedicated filename, not arbitrary
horse appearance replacements.

Automated checks cover source rejection, byte-preserving string append, XML
inverse comparison, composition with both mounts' speed/stamina recipes,
unrelated-character preservation, archive reopening, combined transaction
apply/restore and explicit Marketplace selection without automatic writes.

Pending game checks: visible size increase on Rokade, normal mounting/riding
and equipment appearance, other horses retaining their size, and restoration.
Jump height and forced-dismount height are separate mods and remain unimplemented.
