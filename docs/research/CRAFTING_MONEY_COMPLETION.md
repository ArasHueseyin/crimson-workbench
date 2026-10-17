# Recipe money-cost coverage

17 October 2026. Feature #13's current-build recipe costs are covered by the
three existing independent Marketplace components. The expanded audit found
**no additional money fee to patch in the normal crafting/upgrade transaction**.
The sole currency ingredient is the existing 300-Clockwork-Insect fee. A new
money ASI or a global wallet/inventory bypass would be redundant and broader
than the requested recipe-cost feature.

This conclusion covers the pinned **18,576 MultiChange recipes** in executable
1.0.0.2976, not every purchase, repair, service, quest or other transaction in
the game. Local implementation coverage is separate from in-game acceptance,
which remains pending. No game process, installed module, game archive or save
was changed or called during this audit.

## Complete source-data check

Original archive entries were read into the ignored owned directory
`.local/marketplace-expansion-research/crafting-money/`. The table fingerprints
are checked before semantic interpretation. Independent readers consume every
recipe and group record through EOF and serialize every field back exactly;
the existing typed ItemInfo reader additionally roundtrips all 6,816 items.

| Table | Rows | Body SHA256 |
| --- | ---: | --- |
| MultiChangeInfo | 18,576 | `b21dcbff4deb62a1c476158632d87b372694ca53e983e901e7cfe23f3a50e301` |
| ItemGroupInfo | 1,602 | `719b9f91c0bffa1b676563739e0378a72b659a870e936093accc8a9cb046e45c` |
| ItemInfo | 6,816 | `87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce` |
| CraftToolInfo | 19 | `21dcb1a5288038fd58fab4053c54a28a617d7056526c3ab1078f644d9a7ed1fe` |
| CraftToolGroupInfo | 12 | `1c4142063a612dc46e58ed02aa0b8c80475972bb1de4e64a4a9b776899b37b85` |

All 1,507 ordinary recipes and 17,069 upgrade recipes were checked, including
**717 recipes with ingredient groups**. Expanding all 56 directly referenced
groups reaches 289 groups. Required roots are nonempty; cycles, blocked groups,
unknown group selectors and missing members are rejected rather than silently
excluded. No expanded recipe group contains a wallet item.

Item type 32 and the optional money-format definition identify **19 wallet
items**, not 19 currencies: the set also includes contribution balances, camp
resources, cogs, oil and power plants. Checking the entire set prevents an
unrecognized wallet denomination from being overlooked. Only four identities
occur in recipe fixed inputs:

| Item | Fixed entries | Existing cost category |
| --- | ---: | --- |
| Cog 1000547 | 16 | Material |
| Small Mechanical Powerplant 1002198 | 14 | Material |
| Specialty Oil 30022 | 5 | Material |
| Clockwork Insect 1000485 | 1 | Currency fee |

These are 36 entries across 22 ordinary recipes. Copper, Pearl, camp money,
contribution balances, pinball coins and Hernand bonds occur in no recipe input.
All eight `Money_` internal-name candidates were also checked as a separate,
conservative cross-check. Their sole recipe entry is recipe **1010292**, fixed
index 3, Clockwork Insect **300**, coupon count zero. Its exact original row
and currency-item fingerprints remain the guards used by the existing edit.

The native tool readers at RVAs `14f6450` and `14fb340`, together with their
typed handle-array readers, reconcile the complete 19 tool and 12 tool-group
rows. Their remaining arrays contain handles, not a hidden monetary amount.
No additional fee field is present in these complete table records or in the
complete MultiChange record layout.

## Native cost source and transaction boundary

The original executable is pinned to SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
All addresses below are RVAs. The private audit fingerprints 17 complete
functions and 12 instruction-aligned critical spans directly from that file.

The normal request remains `2a0c950 -> 2b6c050 -> 2b6d620`; it retains native
recipe/prerequisite validation. Within `2b6d620`, the inventory-consumption
vector is at local `rbp-70`, with its count at `rbp-68`. Before its sole transfer
to general inventory setup, the complete native instruction walk finds exactly
these six writes to that vector's metadata:

| RVA | Purpose |
| --- | --- |
| `2b6df4b`, `2b6df4f` | Initialize pointer, count and capacity to zero |
| `2b6e410`, `2b6e417` | Replace grown storage and capacity, preserving the count |
| `2b6e4fc` | Append one copied selected-material consumption record |
| `2b6e864` | Remove an entry after the existing material-saving buff |

The only addresses taken of that vector before setup are the allocator at
`2b6e4c9` and the inventory handoff at `2b6ed2f`. It is not passed to another
fee-populating callback. The append loop `2b6e424..2b6e523` iterates the already
collected recipe-material entries, copies their 40-byte consumption records and
bounds each amount by the recipe's requested quantity/available selection.
There is no second append of a flat or tool-dependent monetary charge.

At `2b6ed41`, `277e080` receives that vector as argument four. Its native
`277e288..277e2ed` path forwards the explicit entries to `27b0540`. That method
copies each complete 40-byte record into its own requirement vector at `+50/+58`,
retaining the ordinary unremovable-item check. Equal begin/end pointers return
success at `27b0629`; it does not invent a default fee.

Validation `27b0660` exits successfully when that requirement count is zero at
`27b0698`. For positive inputs it forwards the exact same list to the normal
inventory resolver `212ecc0`. After the resolver's normal ownership/count
checks, consumption `27b0b70` iterates only the resulting `0xe0`-byte removal
entries. The amount passed to `212df30` at `27b0c46` comes from each entry's
`+10` count; no separate debit occurs before or after that loop. Notification
`27b0c90` likewise returns before its actor/event work for an empty removal list.
The outer validation, locking, equipment-customization transfer, output and
commit phases remain unchanged.

These source and dataflow checks resolve the earlier open question about a
second money charge on the **reviewed normal recipe path**. They do not justify
changing global wallet operations or unrelated transactions.

## Independent choices and resulting behavior

- **Free equipment upgrades** keeps the exact owned equipment input and removes
  subsequent materials/coupons from all 17,069 upgrade recipes. No native
  currency ingredient exists in those recipes.
- **Free crafting materials** removes the materials/groups from all 1,507
  ordinary recipes through the existing immutable views. The 300-unit fee stays
  when its independent currency component is off.
- **No Clockwork Insect crafting fee** removes only the verified 300-unit fee.
  The other four ingredients stay when ordinary material removal is off.

Combining the last two gives a cost-free ordinary recipe view for 1010292.
Normal learned-recipe, knowledge, tool, quest, equipment and result/inventory
rules remain in force. No wallet balance, unrelated item definition or reward
is increased. The current cards' precise names should be kept; a claim such as
"all money costs in the entire game" would be inaccurate.

## Verification and integration result

The isolated native fixture copies the four original methods `27b0540`,
`27b0660`, `27b0b70` and `27b0c90` into memory owned by its own executable.
Every external call and RIP-relative source is rebound to owned stubs/data;
unexpected calls abort. It neither loads the game EXE as a module nor calls a
game address. **505 cases pass** for zero through 100 explicit requirements,
exact 40-byte copies, exact positive removal amounts/slots, absence of implicit
empty-list fees and preservation of native unremovable-item refusal.

The inventory resolver/debit endpoints in that fixture are deliberately owned
stubs. Consequently it verifies native argument/control-flow behavior, not a
real wallet debit, affordability or complete game transaction. The native
resolver itself, result generation and real save/reload still require normal
authorized game acceptance, along with the material/runtime tests already
listed in [the ordinary crafting note](CRAFTING_RUNTIME_PATH.md).

The existing Rust test
`mods::marketplace::crafting::tests::current_build_recipe_costs_roundtrip_and_currency_edit_is_isolated`
passes again against the freshly extracted pinned source tables. It exercises
the actual currency and upgrade implementation, full-table restoration,
independent composition, conflicts and unknown-source refusal.

Private reproducible evidence lives in the owned `crafting-money` directory:

- `data_audit.py`, `data-audit.private.json`: five source tables, all recipe/group
  roundtrips, recursive groups and known monetary identities.
- `rust/` and `typed-money-items.private.json`: all 6,816 complete typed ItemInfo
  roundtrips and the full 19-item wallet candidate set.
- `crosscheck_typed.py`, `typed-crosscheck.private.json`: every recipe input
  against that full typed set, including all ingredient groups.
- `native_audit.py`, `native-audit.private.json`: original function/span hashes,
  the complete consumption-vector metadata writes and address escapes.
- `generate_native_fixture.py`, `native_fixture.cpp`, `CMakeLists.txt`,
  `native-fixture-manifest.private.json`: the owned copied-native fixture;
  Release build with `/W4 /WX` and its single CTest target pass.

**Integration needs no new ASI, hook, installer resource or Marketplace card.**
The parent independently reviewed the typed source audit and native vector
construction/consumption boundary, reran the five-table source extraction and
roundtrips, all 6,816 typed ItemInfo roundtrips, native span audit, regenerated
copied-native fixture and its CTest, and the actual full-table Rust cost test.
The parent additionally checked that all conditional branches in the four
copied methods stay inside their owned spans; external direct calls/jumps and
RIP sources use fixture bindings. These checks passed. The existing three
Marketplace components therefore complete #13 locally; the game-acceptance
requirement stays open.
Nothing was committed, pushed, released or installed.
