# Free mod marketplace

Version 0.7.0 introduces a built-in catalog of four free recipes. It works locally without an account, server, payment or separate mod downloads. The interface is English. Recipes contain our editing instructions, identifiers and fingerprints; game assets and personal saves are not distributed.

| Mod | Scope | Effect |
| --- | --- | --- |
| Larger lion | Shared lion appearance model; lionesses excluded | CharacterScale 0.90 → 1.17 (+30%) |
| Faster mounts | Circus lion and all five Rokade stages | Movement speed stat ×1.30 |
| No mount stamina cost | Circus lion and Rokade | Stamina cost reduction to 100%, boosted regeneration; player stamina unchanged |
| All skills | One selected save slot | All 140 abilities in the Kliff, Oongka and Damiane trees reach their verified maximum |

These recipes require game EXE **1.0.0.2976** and matching verified source data. A matching EXE version alone does not authorize writes. Existing unknown or conflicting game modifications remain blocked by the normal installation checks.

If the current registry contains unmanaged modifications, an in-memory preview or workspace export can still use the verified original snapshot. It shows a warning and grants no write permission. Live apply remains blocked until the registry is reconciled through the existing recovery procedures.

## Apply game mods

1. Open **Mod marketplace** and use the **Select mod** switches for the desired game mods. **Deselect mod** removes a recipe from the draft; neither action installs or removes game files on its own.
2. Choose **Review combined plan**. This prepares files in memory and changes no game files.
3. Review the changes and use **Live apply & restore**. Its existing setup requires a saved content baseline and Steam verification confirmation. Setup and source checks are performed in the application; no PowerShell commands are needed.
4. Close the game, calculate the live file preview and apply the reviewed plan.

The three recipes combine into one plan with any current Workshop settings. Applying replaces the previous Workbench overlay. Remove a recipe from the plan and apply again to disable that recipe; **Check live restore** restores the original registry and removes the complete managed overlay. Existing unmanaged mods are preserved, not adopted or deleted. Model and table overlays are separate archive groups, with the original source language bits retained for each.

“Selected in draft” describes the plan, not the installed state. Selecting a card never installs a mod automatically. **Export local package** writes an unapplied package into the workspace.

## Unlock skills

Choose **Choose save & review** on All skills, select the exact save slot, and inspect the counts of new, upgraded and already-maxed abilities. Save and fully close the game. Confirm the progression change and click **Back up & unlock all skills**.

Workbench preserves existing available skill points, historical expenses, inventory, observed learning and unrelated save data. Missing upgrade costs are added to both the spent ledger and the earned-point total as a gift, keeping refund accounting consistent. Unknown save fields, duplicate entries, inconsistent ledgers, changed saves and overflowing counters cause refusal before writing. A save with no usable skill-entry template must learn a skill normally first.

Backups and transaction receipts are stored in the workspace under `.local/skill-unlocks/<request-id>`. `original.save` and `original.lobby` retain the original files; lobby bytes are unchanged. Retrying a pending request keeps its identifier and checks completion or rolls back an interrupted commit. Fully unlocked saves receive no second gift.

This is a progression edit, not a toggle. There is no automatic “off” switch: restoring an older backup later would also discard progress made since that backup. Only the selected slot is edited. Existing game-version and save-path restrictions still apply.

## Validation

The automated checks cover the three mount combinations, unchanged records and inverse edits, the exact lion appearance path and scale, combined preview/export, unsupported builds, running-game refusal, stale saves, explicit skill confirmation and retry identifiers. An opt-in private save test checks encrypted roundtrip, preserved points and repeated-unlock idempotence without writing the source fixture. The shared archive transaction and paired-save recovery kernels retain their existing interruption tests.

Manual game tests should use the locally installed build: compare lion size, mount speed and sustained sprinting, then load a deliberately selected save and check all three skill trees and point balances. The user previously tested the underlying custom mods; testing the new marketplace workflow is separate.
