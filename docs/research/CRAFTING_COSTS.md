# Crafting and upgrade costs

17 October 2026. Feature #13 is **locally implemented**. The requested
end state is independent material-cost and money-cost controls for crafting and
upgrades. The upgrade-material, ordinary-material and recipe-currency components are
locally implemented independently. The complete recipe/group/tool/wallet audit
and native transaction analysis found no additional recipe fee beyond the
covered Clockwork Insect input. See [Recipe money-cost coverage](CRAFTING_MONEY_COMPLETION.md).
Unrelated shop/service/quest charges are outside this crafting feature. No
expansion has been installed into the game; in-game acceptance is pending.

## Current build evidence

The immutable original `multichangeinfo` table for executable 1.0.0.2976 contains
**18,576** records: 1,507 consumption-type-1 recipes and 17,069 type-3 upgrade
recipes. All records parse and independently serialize back byte-for-byte.

- Body: 4,578,225 bytes, SHA256
  `b21dcbff4deb62a1c476158632d87b372694ca53e983e901e7cfe23f3a50e301`.
- Header: 148,610 bytes, SHA256
  `27b8f2b37f0cd90c114e34240c6fe606b17ffbeb7277477b571f6ba468179a80`.
- Every type-3 record starts with a one-count target item with zero coupon
  count. No type-3 record uses ingredient groups. Removing that first entry
  would remove the existing equipment input, rather than merely waive its
  improvement materials. Preserve the target, its refinement level and every
  unrelated recipe field.
- There are **4,032** coupon requirements. These are separate from ordinary
  item counts; native loader field names identify `_couponCount`. Treating all
  zero-item entries as unused data would incorrectly remove these requirements.
- Wallet item type 32 also contains oil, cogs, power sources and contribution
  balances. It is not sufficient evidence that an ingredient is a money fee.

Private extracted tables, their parsed contents and native disassembly remain
in ignored local research folders. This document contains identifiers and
fingerprints, not bundled game data.

## Equipment upgrade materials

The English **Free equipment upgrades** card removes all fixed requirements
after the first entry in every one of the **17,069** supported type-3 upgrade
recipes. That first entry is kept exactly, including the equipment key,
one-count quantity and existing refinement level. All other fields, including
the consumption mode, tools, knowledge/conditions, result dropsets and flags,
remain byte-identical. There is no invented target or zero-quantity input.
The removed requirements include ordinary materials, duplicate-material gear
and the 4,032 refinement-coupon entries. The original game still performs its
normal equipment-improvement operation on the owned input.

Native matching routine RVA `2428b40` checks the expected and provided fixed
requirements. Keeping one positive target requirement preserves that route;
it also retains the first-entry data used by the native upgrade checks at
`667510`. Existing source definitions and output levels are not raised by this
mod, and no recipe conditions are bypassed.

The complete source table/header are pinned before interpretation. The builder
refuses unknown target layouts and conflicting edits, requires the complete
17,069-row set and publishes its in-memory replacements together. Its reviewed
diff summarizes the complete set rather than rendering 17,069 nearly identical
UI rows. The currency component below is independent; combining the two edits
17,070 different records. Removing either from the next combined plan restores
that part while keeping the other selected component.

Local fixtures independently parse, serialize and rebuild every recipe. For
each changed upgrade, restoring its original non-target requirements must
recover the entire original record byte-for-byte. They also check unchanged
ordinary crafting recipes, unchanged currency requirements when the currency
toggle is off, idempotence, conflict rejection and actual generated overlay
archives. In-game checks must still verify one ordinary material upgrade, one
coupon upgrade and a duplicate-material upgrade, preserving the equipment's
customizations and normal refinement/quest limits. No module or archive has
been installed for these checks.

## Verified currency component

Recipe **1010292**, `Kuku_Drone_Backpack_0`, requires 300 units of native wallet
item **1000485**, `Money_Kuku_MachineBug`, displayed as Clockwork Insect. Its
other four fixed requirements are 1002130 ×1, 1003633 ×3, 1003827 ×1 and 1002198
×200. Knowledge 1003097 and result dropset 1007688 are preserved.

The local English Marketplace card **No Clockwork Insect crafting fee** removes
only that currency requirement. It leaves four positive material requirements,
all flags, conditions, tool selection, knowledge, descriptions and results
unchanged. It changes no wallet balances, item definitions, dropsets, shops or
other recipes. No input quantity is set to zero.

The recipe row is additionally pinned to SHA256
`0317fd6d9f82a6d72a4008bdcbaaa5dcf9702334b44614c39ffc6716d26a744f`;
the currency item row to
`e10e89aeef53a7f6769059058d06a7363d28635f88d8c364fd3b150670435bae`.
Unknown or conflicting edits fail before a plan is produced. It uses the
existing combined archive plan and restore mechanism. Removing it from a new
plan restores the original recipe through removal of the previous overlay.

This is the sole fixed requirement using the checked money identities 1, 2, 11
or 1000485 in the current recipe table. No copper/pearl/camp-money requirement
was found in those fixed arrays. This does not prove the absence of charges
outside that table, and the card does not claim to waive such charges.

## Native observations behind empty-input support

- Native row deserializer RVA `1511b20` resolves the recipe's tool and reads its
  consumption type, condition list and requirements. The loaded fixed-material
  vector is at `+40`, its count at `+48`; ingredient groups are at `+50/+58`.
- Native row accessor RVA `4a21f0` resolves a current MultiChange handle.
- Native validation at `2429dec` checks both fixed and group counts. An empty
  pair is rejected along its normal crafting path unless a specific native
  consumption-mode case applies. Merely deleting all ordinary ingredients is
  not a supported implementation of free crafting.
- The item-exchange request type has vtable RVA `5b22e18`; its execution method
  at `2a0c950` deserializes the request and reaches `2b6c050`, then the larger
  transaction path at `2b6d620`. A table roundtrip does not prove that every
  empty-cost recipe will be accepted by these paths.
- UI routine RVA `1074800` divides the available material count by a required
  count at `10749cb`. It also skips its quantity-dialog path when its material
  model count is zero. Setting quantities to zero is unsafe, and deleting every
  ingredient still needs a verified UI/request path. The removed currency row
  above leaves other positive inputs and does not take either empty-input path.
- Craft-tool groups have a native `_ignoreReduceMaterialCountBuff` field. Its
  presence is not proof that an existing reduction buff can make every recipe
  free; the upgrade group ignores that reduction.
- In UI routines `e27040` and `107d6e0`, a missing recipe knowledge handle can
  trigger a fallback to the first fixed material, or the first member of the
  first ingredient group. Emptying both lists can make that fallback invalid.
  The upgrade implementation retains its equipment entry and avoids this
  empty-input case; the ordinary runtime path supplies separate UI-only fallbacks.

## Original empty-archive preparation, not enabled

The staged `crafting_materials` helper now prepares all **1,507** ordinary
recipes in memory and rebuilds the complete table. It removes positive material
requirements and ingredient groups instead of writing zero counts. The verified
300-Clockwork-Insect fee remains unless the separate currency choice is enabled.
The resulting native-support contract therefore contains 1,506 empty recipes
with the fee retained, or 1,507 with both costs waived. The 17,069 equipment
upgrade recipes stay unchanged by this preparation.

All original recipe identity, tool, consumption mode, flags, conditions,
knowledge, result and additional fields remain exact. Nine ordinary recipes
have no explicit knowledge prerequisite. Their current UI obtains a display
category from the first fixed material's ItemInfo knowledge key. Those exact
fallbacks have been checked against the independently roundtripped original
ItemInfo table. They are returned as **separate UI-only metadata**. Writing them
into the recipe's knowledge field would add an unlock requirement in native
`2429370`; the helper explicitly preserves zero there.

This preparation stays absent from Marketplace Selection and the archive-install
path. The new native Marketplace card uses immutable views instead. These
archived-empty-input helpers are retained only as research fixtures. Full-table tests
check both currency choices, all 717 ingredient-group recipes, byte-identical
inverse restoration, the nine native display fallbacks, unchanged upgrades and
rejection of unknown table data. They do not establish in-game acceptance.

Further native inspection distinguishes the existing reduction buff from free
crafting. `ReduceCraftMaterialCount` is native enum value 5. The transaction
checks craft-tool-group byte `+29` at `2b6e675`, then reads that buff at
`2b6e6ad..2b6e6b9`. The `2b6e7b0..2b6e893` loop rolls a probability and removes
or decrements one selected material unit after availability collection. Merely
raising that buff does not let the player craft without owning the materials;
it is not the implementation of the requested material-cost toggle.

The client exchange function also rejects two empty input vectors at
`213b793..213b799`, before its subsequent `2429370` check. Fixing only database
validation and the quantity dialog would therefore still leave a client-side
rejection. A scoped native extension must preserve the later condition and
knowledge checks. The continuing native UI/request audit is recorded separately
in `CRAFTING_RUNTIME_PATH.md` when available.

The empty-archive preparation is superseded by the native immutable-view route
documented in [Ordinary crafting runtime path](CRAFTING_RUNTIME_PATH.md). The
English **Free crafting materials** card supports all 1,507 ordinary recipes
without changing their archive or manager rows. It preserves the separate
300-unit fee unless the existing currency component removed it, and keeps
recipe prerequisites, output logic and normal inventory validation. Empty-cost
recipes use a finite 40-per-batch quantity. Native view/policy/ABI fixtures, the
full original table audit, combined packaging and Marketplace browser tests pass.

Remaining: authorized in-game acceptance of ordinary crafting and upgrades.
The expanded [recipe money audit](CRAFTING_MONEY_COMPLETION.md) resolves the
earlier question about additional current-build recipe charges; it does not
claim free shops, services or other unrelated game transactions. No game test
or installation has been performed.

Local checks cover exact record roundtrips, isolated fee removal, rejection of
ambiguous/malformed fees, byte-identical inverse restoration, preservation of
all 18,575 other recipes for the currency-only component, all upgrade target
inputs and non-cost fields for the material component, a real archive plan, composition/removal with other
Marketplace settings and explicit reviewed UI selection. The native transaction
and in-game crafted result still need acceptance testing.
