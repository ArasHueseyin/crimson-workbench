# Native weapon effect transfers

Local implementation checkpoint: 17 October 2026, game build 1.0.0.2976.
The running game has not been changed. Combat and visual acceptance remain
pending; table roundtrips alone do not prove the transferred effect works in play.

## Identified effects

The user's Elektro-Mecha-Langschwert is **Electro-Mecha Longsword, item 1001062**
in the English catalog. Its enchant-level buff lists are empty. Its equipped
passive is **91101, level 3**, `Equip_Passive_LightningWeapon`. Copying an ordinary
equipment buff would miss the requested effect.

This skill contains `RegisterConditionSkillBuffData` entries registering **92011**,
`Repeat_Increase_Lightning`, at the corresponding level. That child uses native
projectile reference **720417726**. Reusing the complete equipped-passive reference
retains the native trigger chain instead of synthesizing a cosmetic effect.

Original items already use passive 91101, level 3 on these weapon classes:

| Native source/witness | Item key | Equipment type | Category |
| --- | --- | --- | --- |
| Electro-Mecha Longsword | 1001062 | 1086980073 | 202 |
| Lightning Spear | 1001163 | 2914941932 | 205 |
| Lightning Greathammer | 1002741 | 604374103 | 212 |

**Flame Spear, item 1000451**, provides **91105, level 3**,
`Equip_Passive_FireWeapon`. It registers **92013** and includes the original
material-effect behavior. Blazing Spear 1000176 and Kuku Flame Spear 1002176 also
use this passive. Fire transfers are offered only for the observed spear pair.
The skill table contains an ice-weapon skill, but a name alone does not establish
a source item or compatible target; it is not offered yet.

## Selection and compatibility

The **Native weapon effects** Marketplace card opens an English editor with
source selection, compatible-target name/ID search and removal of draft transfers.

- Lightning accepts the three observed equipment-type/category pairs above.
  Fire accepts only the spear pair. The backend refuses masks, armor, consumables,
  unknown sources and unverified weapon classes too.
- Both primary source items are protected from becoming targets.
- Empty equipped-passive lists can receive the selected native reference.
- A different existing native ability requires an explicit **Replace existing
  native weapon abilities** choice. The editor lists skill IDs/levels, and the
  plan shows the exact list replacement. All equipped passives on that target
  definition are replaced; enchant-level buffs and sockets are retained.
- Changing the source or target revokes replacement consent. A weapon already
  using the exact selected passive produces no change.
- Workshop edits to the transferred passive or its child skill are refused,
  rather than silently changing the selected native effect.

This edits an archive definition and affects **all copies of the selected weapon**
after the game reloads it. It is not an inventory-instance or running-game editor.
Identity, upgrade rows, socket configuration, ordinary enchant buffs and other
fields are retained. Independent Workshop edits compose with the transfer and
remain separately visible in the preview.

## Restoration and checks

Every plan starts from validated originals. Remove a transfer and apply the newly
reviewed plan to restore that weapon's original equipped-passive list while
retaining independent Workshop edits. Restore removes the complete Workbench
overlay through the existing transaction engine. Both operations require the
game to be closed.

The recipe fingerprints the full original skill body and each source/witness
item row. No original game asset bodies are checked into the repository.

The opt-in original-data test verifies lightning on another greatsword, fire on
a spear, explicit replacement, rejection of unsupported targets/sources, and
unchanged source/unselected item bytes. An inverse byte comparison covers every
field outside the selected passive list, including upgrades and sockets, while
retaining an independent Workshop stat edit. Removing transfers reconstructs the
original abilities. A private-fixture test checks conflicts with Workshop skill
edits. UI tests cover selection, consent invalidation and removal without
automatic installation.

Pending game checks: real attack triggers, lightning/fire appearance, unequipping
and restoration. Installing the local expansion for those tests must wait until
the user closes the game.
