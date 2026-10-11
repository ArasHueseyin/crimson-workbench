# Complete skill trees for all three characters

Status: 10 October 2026. Written to the most recent save and independently read back afterward. In-game validation remains pending.

## Changes recorded

The three main skill trees were identified from the installed game data: 50 entries for Kliff, 48 for Oongka and 44 for Damiane. Two shared skills mean that these 142 tree entries use 140 distinct knowledge keys. All are at their respective maximum level in the edited save: 69 newly unlocked, 16 raised, and 55 already-maxed entries preserved. This includes the normal health, stamina and spirit levels in those trees.

The maximum level comes from each knowledge entry's level list. An individual `SkillInfo.max_level` is not a general limit for a tree entry with multiple skill variants.

| Character | Available points preserved | Additional learning costs recorded |
| --- | ---: | ---: |
| Kliff | 108 | 94 |
| Oongka | 236 | 205 |
| Damiane | 116 | 91 |

The total-points counter was credited by the same amount as the additional learning costs, keeping the existing accounting and refund counter consistent. Historical spending and skills learned through observation were preserved. Available points and inventory items were not consumed.

This was a direct edit of the most recent save, internally `slot0`, displayed as **Slot 1** in the game and Workbench. Inventory, equipment, extra sockets, quests and other saves were preserved. The game installation and Workbench files were unchanged.

## Technical checks completed

- All twelve existing save/lobby files were backed up before the edit.
- The private candidate passed a complete semantic comparison of all 1,179 blocks, including exact treatment of existing NaN values.
- All 140 target keys were unique, present and at the intended maximum level.
- The schema was unchanged, with no undecoded regions; 135,405 embedded position references were checked.
- The other 1,178 blocks retained their payloads: 891 were byte-identical, and 287 changed only where absolute position references required relocation.
- Reversing the changes in a private copy reproduced the complete original decrypted body byte for byte.
- Independent decryption, HMAC and decompression checks passed.
- One save file was replaced atomically while the game was closed and launch protection was held. Readback passed; the other eleven save/lobby files and twelve installation/mod files remained unchanged.

Private evidence and originals are under `.local/skill-tree-all-20261010/`. The pre-edit backup is in `originals/20261010T125106.513574Z/`; completion evidence is in `save-commit-result.json`, and launch-protection results are in `guard-result.json`.

## In-game checks still required

1. Start Crimson Desert and choose **Continue** to load the most recent save.
2. Open the skill trees for Kliff, Oongka and Damiane and confirm that the intended levels are fully learned.
3. Use several previously unavailable skills, including a character-specific combat skill and a movement skill for each character. Check maximum health, stamina and spirit.
4. Save normally, close the game and reload the same save. Verify that unlocks persist and inspect inventory, equipment and quests.

This personal save-editing task did not implement a general-purpose full-unlock button in Workbench.
