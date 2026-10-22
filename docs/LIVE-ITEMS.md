# Item selection and live spawner

Items whose default destination is Kuku storage, warehouses, vehicle/ship
storage, housing storage or another unverified special container are now
refused before native creation. Detection uses inventory IDs in the item data,
not translated names. Other placement-context entries do not establish that a
pot is interchangeable or owned. The UI explains the required storage and the
ASI independently checks the currently loaded definition. Such a refusal leaves
the session ready for ordinary items; it does not require a game restart.
Direct creation inside special containers is not implemented. It requires a
verified native capture/storage path and target identity, not merely a different
inventory number. Existing uncertain requests retain their no-retry protection.

Status: 03.10.2026. **Selection and automatic live output are implemented, tested outside the game and installed when the game is closed. Additional suspension is no longer provided for. The decrease in the real game is still pending.**

Open: Desktop shortcut **Crimson Workbench**, navigation **Item selection**.

## Extension from 03.10.2026 – installed on 04.10.2026

**Further update 04.10.2026, installed and technically tested; in-game validation open:** partial word search for all items, optional regex mode in item database and item selection, larger icons and clickable image preview. Search patterns are limited to 512 characters. Invalid or unsupported patterns show an error message; Without regex, inputs are searched as normal subwords. Categories, sorting and pages remain combinable. Pure numbers continue to search for an exact item ID.

Lightning arrow **1001315**, fire arrow **1001316** and cold arrow **1001314** are expressly marked as monster ammunition in the item data tested. They can be present in the inventory without functioning as regular player ammunition. The Workbench therefore shows a hint and blocks its output from a native call. Useful alternatives are **arrow 50001**, **poison arrow 50003** and **explosion arrow 1001321**; In the case of the latter, player use is expressly noted in the item data. Existing NPC arrows are not removed.

New ASI and desktop version with closed game/Workbench replaced and independently read back: 36 existing files including 13 current save files unchanged, 23 backups checked. The existing desktop shortcut opens the new version. Installing does not award items or animals. Detection: `.local/live-items-bag-mounts-20261003/items-final-verification.json`. in-game validation of the new functions remains open.

The general error for Extra Large Pocket **6003** and Lightning Arrow **1001315** comes from an optional definition field with value `65535` that was incorrectly treated as an invalid item. The confirmed native standard path will now be allowed. Unknown IDs, definition deviations and too many preset sockets receive separate messages. The actual native constructor and converter were tested with observed default values in their own process for both items; The inventory/storage acceptance in the game is pending.

The set field allows blanking, typing and pasting with checking for whole numbers from 1 to 10.000. An estimate uses the basic catalog stack limit: 100 pieces at limit 10 result in a maximum of ten new stacks, existing stacks can fill up the native allocation. Mods can change the limit. No additional allocation requests or automatic repetitions for batch splitting; an unclear or partial order remains blocked for examination.

The knowledge display reads the **selected stored save**. It uses typed knowledge rewards in item data and the stored knowledge level, not mere reading conditions of a book page. Conditions: already attained, not yet attained, partially, unknown. Documents without a clear assignment show unknown. Save after reading in the game and re-read **Knowledge**. In the case of damaged or incompletely readable files, no status is invented. The comparison with a read/unread font in the game is pending.

New selection **mounts** with search, family, description and ownership status: [Registration and borders](REITTIERE.md). Registration takes place when the game is closed with fresh backup and exact receipt of existing save areas. The installation of this feature does not award animals or items.

The follow-up extension for assigned healthy template animals and additional animal portraits is installed and independently tested; in-game validation remains pending. 299 Entries have an image (89 exact portraits, 210 labeled example images). The new copy does not adopt name or active character assignment; The original shall be preserved. Another technically tested extension offers basic entries without a family template; The desktop update is installed. The surface characterizes their as yet unconfirmed game initialization and riding function. Details and limits are in [mounts](REITTIERE.md).

Evidence: `.local/live-items-bag-mounts-20261003/`. 155 Core tests, 14 Codec tests, 11 Frontend unit tests and ten targeted browser tests passed. Native Bag/Arrow standard paths, queue/context, automatic connection, real Rust/C++ pipe and common loader tested in own processes. True private save copies consist of knowledge reading, a bear/horse registration, fresh animal/item numbers, encrypted readback and complete back-testing. These tests do not replace in-game validation.

## Implemented

- 6,816 Items from the tested local installation, without external Item API.
- icons from the game archives; a placeholder in the absence of an image reference.
- Description in list and detail view, numerical table values and selection of refinement level.
- Five main categories and named subgroups from `ItemGroupInfo`. The group assignment uses its item lists, unadvanced item categories.
- search by name, description, internal ID and exact numeric item ID; Combination with categories.
- Virtual list and pages with 200 results each. Outdated search and detail responses cannot overwrite more recent results.
- Quantity input and real command **“Give to inventory”**. The button is only active when the live module is connected; Without a suitable runtime, it remains locked.
- Reading data matching for Build 2976: 38 Metafiles, 28 Core table files and all additional tables and localizations used are checked. No credentials and no vanilla certification.

The selection reads the **basic data**. Installed table changes, individual sockets, character bonuses and current inventory instances are not yet included. In the case of a registry that has been modified outside the Workbench journal, only the fully hash-checked original basic catalogue is accessible. The file writing paths retain their previous possession checks.

## Diagnose-Mod

`CrimsonLiveItemsProbe.asi` is installed when the game is closed. He checks the full EXE and the full observed inventory routine of the build 2976 before installing a hook. The hook only observes the call context and forwards each original call exactly once. It does not hold any game object pointers beyond the call and does not have an item-give command.

At the next normal game start with loaded save, the mod can write context observations to `bin64/CrimsonLiveItemsProbe.log`. A `READY` only means that the reading hook has been set up. Even a successful observation does not yet give write authorization.

During installation, 32 existing game/save files were checked unchanged and 18 files were freshly backed up. Registry, Game EXE and saved games have not been edited. Installation and testing are under `.local/live-items-20261003/`.

## Installed live module

`CrimsonLiveItems.asi` uses the native generation and inventory transaction routines of the 2976 build. The full EXE and the central functions used are checked before the hook. New item data is created with the native constructor and its complete InitData conversion; Durability and preset properties come from the currently loaded definitions.

The first live module was installed and read back when the game was completely closed. The automatic version now replaces this module and `target/release/crimson-workbench-live.exe`, also with the game closed and Workbench closed. The existing desktop link thus opens the new version. 36 existing files including all 13 current save files and the link remained unchanged; 23 Files have been backed up. Both replaced files are received as `auto-install-originals/replaced-0.bin` (ASI) and `replaced-1.bin` (Workbench). Current installation proofs and backups: `.local/live-items-autopump-20261003/`; First version: `.local/live-items-20261003/`.

Requests are automatically processed after a suitable server character order. The two checked call channels maintain a native reference to its owner until after the output. The original order runs exactly once; his arguments, his error code and his result are preserved. Checked are server realm, context pool, native object class, active player, full character identifier and references between owner, control and inventory. No object pointers are saved for later calls and no TLS values are changed in the game. Recursion protection prevents further output within a nested game task.

The status query checks the selected figure only by reading. Thus, the selection remains usable even with Alt-Tab/Pause, if the same previously confirmed figure is still active. An order is only executed on the game thread with a valid native reference. If the character identifier is changed or missing, waiting orders are rejected. A longer context termination without reading confirmation makes the connection unready again. This version supports the tested SelfPlayer context; Other types of players will not be allowed unchecked.

The Workbench addresses a local named pipe of the confirmed game process. Only the same Windows user may connect; Remote connections are rejected. A request contains Item ID, Quantity, UUID and Game Instance. Runtime prevents re-entering the same UUID, manipulated requests, orders for another game instance, and parallel write orders. The Workbench stores open IDs before sending and requests the same ID for a connection error, instead of giving it again.

### Operation

1. Start the game normally, load save and return to the normal game briefly. The module automatically confirms the character; You don't have to pick anything up.
2. In **Item Select**Choose Item and Quantity, click **Choose Enter Into Inventory**.
3. If the game is paused or does not process character tasks in the background: return to the game briefly. The request is automatically processed at the next matching character task, without additional lifting or an inventory process.
4. The runtime confirms the additional amount based on the inventory. After that, save normally in the game. A confirmed amount is not yet proof that later storage and reloading was successful.

Waiting requests can be canceled and expire without execution after 30 seconds. In the case of a native error or a different set, the result is marked as unclear and further giving is blocked in this game instance. No automatic repetition.

Limits: 1–10,000 piece per request; for item types without the confirmed native set flag, a limit of 100 remains. This flag is not a reliable proof of lack of stackability. Five preset bases when created; Items with further modified base definitions are rejected before being generated. The selection also shows base table values; The runtime actually uses loaded definitions. The protocol keeps a maximum of 512 request IDs until the game restart, without forgetting old IDs.

## Still open

1. Confirm automatic character context in the real game: In `CrimsonLiveItems.log` the new `ARMED` line and a `CONTEXT` line must appear. The old diagnosis alone does not prove a working grant hook.
2. Decrease with **an iron ore**: without additional pickup, see additional amount in the inventory, save normally, close game completely and reload the same save.
3. After that, stack, check full inventory, pause/alt tab, load/character change, cancellation, 30-second decay and other item types. Only use higher quantities after the storage/load detection.

## Already checked

- 150 Passed core tests, ignored an existing test.
- Real catalog test: all 6,816 items, DDS icon, categories, category/text/ID search, additional equipment values and export.
- Eight browser tests: existing catalog and new selection, icons/description, refinements, pagination, refurbished search responses and layout at 1024 pixels.
- Production frontend and desktop EXE built; Desktop shortcut updated.
- Sample: reading object check including invalid pointers, exact one-time forwarding and unchanged observed bytes. Existing ASI loader loads all five mods; A foreign EXE is rejected before the hook.
- New Runtime: Exactly one-time forwarding of all 13 transaction arguments, competing dequeue, UUID repetition, deviant content, wrong game instance, cancellation, expiration and lock checked for unclear result.
- Twelve cases of actual, hash-tested build2976 code in a dedicated test process: constructor, full InitData conversion, and destructor; 0–5 Socket, Quantity 1/1,000, native standard durability, new item ID and memory limits. Definitions/Allocator are test data; This is not a test of the real inventory/storage routine.
- Real Windows pipe between Rust and C++ in your own test process: status, type/repeat, result query, cancellation and rejection of another game instance. No connection to the current game.
- Eleven browser tests, seven front-end unit tests and five desktop unit tests. Common loader checked with all six ASIs and rejection of a foreign EXE.
- Installation of the new runtime and desktop version read back; existing saved games, registry, mods and EXE unchanged tested. Desktop shortcut points to the new version.
- Automatic output: Actual build 2976 task dispatcher, its wrapper and the MinHook trampoline executed in its own test process. Native reference is preserved via the callback; Arguments/error/original call, nested job, UUID repetition, wrong figure/realm, missing pool, context break, pause and error lock checked. Registry/reference release and inventory are test data; This is not yet a real game inventory/storage test.
- All 13 current code/call path pins checked; Windows Pipe with protocol 2 tested. Passed five desktop and seven frontend unit tests; Production build and common loader tested with all six ASIs.
- Update performed with closed game/Workbench, locked game start, current backups and read back. The first exchange was denied by Windows due to open target handles; The old files remained unchanged. The corrected exchange keeps the game EXE exclusively blocked until completion.

## Dismantling of diagnosis

When the game is completely closed, only `bin64/CrimsonLiveItemsProbe.asi` can be removed. The four existing mods and the registry remain in place. The diagnostic log can be kept for further analysis.
