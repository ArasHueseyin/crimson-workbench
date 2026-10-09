# Crimson Workbench – Build Spec

You are building **Crimson Workbench**, a standalone desktop GUI app for the single-player PC game **Crimson Desert** (Pearl Abyss). It combines a game-data browser, planning tools, a read-only save reader, and its **own mod engine** that applies changes to the game directly. It is fully self-contained: no CDUMM, DMM, or any other external mod manager is needed or used.

Work strictly **phase by phase** (see "Phases"). After each phase: run all tests, update `docs/PROGRESS.md`, and **stop for my review**. Do not start the next phase on your own.

---

## Hard rules

1. **Controlled writes only.** Save files are always read-only. Game files are modified exclusively by the Apply engine (B0), never by any other code path, never while the game is running, and only after a verified vanilla snapshot and backup exist. Leave original PAZ archives untouched wherever possible (overlay approach, see B0).
2. **Round-trip or it doesn't ship.** Every table parser must satisfy: parse → serialize == byte-identical output for the unmodified file. This is an automated test against the real game files. No table gets an editor before its round-trip test passes.
3. **Build detection.** Fingerprint the installed game build (exe version + hashes of the tables we use). Only known builds are supported; unknown builds are refused with a clear message instead of being mis-read. Schemas are versioned per build.
4. **Current format.** Since the 4 September 2026 update, data tables are named `*.staticinfobody` instead of `*.pabgb` (e.g. `iteminfo.staticinfobody`), and shop stock records gained 8 bytes. A lot of community documentation predates this. Always verify against the real files on this machine; treat all reference docs as possibly outdated.
5. **No game data in the repo.** No game files, extracted tables, textures, or saves get committed. Add them to `.gitignore`. Tests needing game data read paths from `.env` (`CD_GAME_DIR`, `CD_SAVE_DIR`) and are skipped when missing.
6. **Licenses.** Check the LICENSE of every reference repo before copying code. No license → use it only as format documentation and write your own implementation. Credit all sources in `CREDITS.md`. Mods generated with knowledge from GildyBoye's editor must follow its attribution terms.
7. **Unknown fields stay raw.** If a field's meaning is not confirmed, show it as raw (offset, type, value). Don't invent names. Mark guesses as guesses.

---

## Tech stack

- **Tauri 2** desktop app. Rust backend, **React + TypeScript + Vite** frontend.
- Rust workspace:
  - `cd-core` – all format/parsing/patching logic, no UI. Fully covered by `cargo test`.
  - `cd-cli` – thin CLI over `cd-core`: dump tables to JSON, run round-trip checks, diff two builds, export a mod. Use it to verify your own work without the GUI.
  - `app` – Tauri shell exposing `cd-core` via commands.
- Evaluate **bbfox0703/crimson-rs** as a dependency for PAZ/PAMT, decryption, save format, and iteminfo. If the license allows it, it builds, and it supports the current build, depend on it (git dependency, pinned commit). Otherwise port only what's needed.
- **SQLite** (`rusqlite`, bundled) with **FTS5** as a local index built from the extracted tables. Rebuild automatically when the build fingerprint changes.
- Frontend: TanStack Table + virtualization for large tables, Leaflet with `CRS.Simple` for the map, Zustand or similar for state. Keep it clean and fast; dark theme default.
- Windows is the primary target (game is installed here). Code should still compile on Linux for CI.

---

## Reference material – read before writing code

Summarize findings in `docs/FORMATS.md` (file layout, encryption, compression, table schemas, what changed in the September update).

- https://github.com/bbfox0703/crimson-rs – Rust lib: PAZ/PAMT, save format, iteminfo round-trip, stageinfo/questinfo loaders
- https://github.com/lazorr410/crimson-desert-unpacker – PAZ/PAMT extraction, ChaCha20 key derivation, LZ4, repacking spec (`PAZ_DECRYPTION.md`)
- https://github.com/NattKh/CrimsonDesertModdingTools – Python parsers for pabgb/pabgh tables (iteminfo, storeinfo, regioninfo, characterinfo mounts, vehicleinfo, fieldinfo, questinfo, stageinfo), universal dumper
- https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS – save editor, ItemBuffs, Stores, DropSets, SpawnEdit, FieldEdit; read `CrimsonGameMods/MODDING_GUIDE.md` carefully (empirically tested failure modes)
- https://github.com/GildyBoye/CrimsonDesertShopEditor – shop, price, dropset (incl. trust gain), skill editors
- https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager – CDUMM source. **Reference only, not a dependency.** Study how it applies mods to the game: vanilla snapshot, delta patching, PAZ/PAMT rewriting, checksum updates, `meta/0.papgt` group registration, overlays, recovery after game updates. Our Apply engine (B0) must solve the same problems itself.

**Reference mods:** I will put existing Nexus mods into `reference/mods/` (inventory slots, stack size, dragon cooldown, stamina/durability/cooldown packs, shop mods). They show exactly which files and bytes the community changes. Use them to locate fields. Never ship their content.

---

## Features

### A. Read-only tools

**A1 – Item database**
- All items with localized names (language selectable, default German; `.paloc` files, LZ4-wrapped since 2.03), category, icon if extractable, and every field including unnamed ones.
- Full-text search over names, descriptions, and internal keys (FTS5). Filters by category, type, stats.
- Item detail view with cross-links: sold by (store, city/region, price, stock), dropped from (dropset → source), used in recipes, recipe to craft it.

**A2 – Crafting calculator**
- First find where recipes live (which table/fields) and document it.
- Pick target item + quantity → recursive multi-level material tree + aggregated list of base materials.
- For each material: vendors (store, region, price, stock), drop sources (source, chance, quantity), craftable yes/no.
- "Already have" amounts to subtract. Later (after A4): optionally import owned quantities from the save.

**A3 – Interactive map**
- Most uncertain feature. **Deliver a feasibility report first** (which data exists, what's missing) before building UI.
- Extract world map texture(s) from PAZ, tile them, render in Leaflet `CRS.Simple`.
- Find tables with world positions of collectibles / gimmicks / points of interest. Calibrate game coords → map coords using known reference points; document the transform.
- Layers per category, search, click marker → details. Mark collected ones if the save reader supports it.

**A4 – 100% tracker (save reader, read-only)**
- Auto-detect saves (Steam, Epic, Game Pass; default `%LOCALAPPDATA%\CrimsonDesert\Saved\SaveGames`), select slot.
- Parse quest/stage state (questinfo + stageinfo + save) → done / in progress / missing, grouped by region and category, with progress bars.
- Same for collectibles if the save tracks them.
- Watch the save file and refresh when the game saves. Open saves read-only, never lock them.

### B. Mod builder

Each module produces a set of edits on parsed tables. The Apply engine (B0) merges all active modules and writes the result into the game. Every module shows a diff preview (field, old → new) before applying.

**B0 – Apply engine (the core of the mod builder)**
- **Vanilla snapshot:** on first run, hash all files we might touch (`meta/0.papgt`, relevant `.pamt`/`.paz`). Only apply onto a verified vanilla state; otherwise refuse and tell me to run Steam "Verify integrity of game files".
- **Overlay first:** write modified tables into our own PAZ group owned by Crimson Workbench (own `.paz` + `.pamt` with correct offsets and checksums) and register it in `meta/0.papgt`, so original archives stay untouched. Determine in Phase 0 whether the engine accepts overlays for our tables (check how CDUMM/DMM do it). Fallback only if overlays don't work: in-place patching with full per-file backups.
- **Rebuild, don't stack:** every Apply regenerates our overlay from vanilla + current settings. Never patch on top of a previous patch.
- **Restore vanilla:** one click removes our group, restores `0.papgt` from backup, and verifies all hashes match the snapshot.
- **Game running check:** refuse to write while the game process is running.
- **Atomic writes:** temp file + rename, plus an apply log (what was written, when, hashes before/after).
- **Game update handling:** on build fingerprint change, check whether the update overwrote our changes. If schemas for the new build are known, offer to re-apply; otherwise stay vanilla and say why.
- **Foreign mods:** detect modifications not made by us (hash mismatches, unknown PAZ groups, other managers' leftovers). Warn clearly and don't apply over them unless I confirm.
- **Dry run:** full preview of all file-level changes before writing.

- **B1 – Shops:** every vendor sells everything (or chosen item sets), stock 999 per slot, daily refresh. Per-vendor overrides. Respect the new 8-byte stock record layout.
- **B2 – Drops:** global multipliers for drop chance and quantity, per-dropset overrides, "guaranteed boss drops" (chance 100% for boss dropsets, if bosses are identifiable from the data; otherwise manual selection).
- **B3 – Trust:** multiplier for NPC trust gain values (stored in dropsetinfo, see GildyBoye's Dropset Editor).
- **B4 – World:** creature/NPC spawn counts and respawn cooldowns (multipliers + per-spawn overrides), allow mounts in towns (region/field flags), mount ride duration and cooldown.
- **B5 – Dragon:** remove the Blackstar cooldown, no forced dismount when flying over region borders, ride duration presets + custom value.
- **B6 – Inventory:** inventory and storage slot counts as free numeric input, not presets. Locate the field via reference mods; if there's a hard engine maximum, find it, enforce it, document it.
- **B7 – Stacks:** global stack size (free input up to the discovered max) and "make stackable" for selected categories. Equipment and anything with per-instance state (durability, enchants, sockets) sits behind an **experimental** toggle with a clear warning. Document test results.
- **B8 – Durability:** no durability loss, repair cost 0.
- **B9 – Stamina & Spirit:** cost multiplier per action category (0 = free), or unlimited.
- **B10 – Skills:** skill browser with all values, cooldown multiplier (global + per skill), edit individual numeric values.
- **B11 – God-item editor:** pick an item, add/edit stats, buffs, enchants (iteminfo stat hashes, see NattKh's ItemBuffs). Save reusable templates.

### C. Profiles

- Named profiles = a saved set of module settings. Import/export as JSON.
- Built-in **"Farm mode"** preset: drops ×10, spawns ×3 (editable).
- One-click switch between profiles (e.g. Normal ↔ Farm) → applies directly via B0. If the game is running, show that it must be closed first; changes take effect on the next game start.

### D. Optional – separate later phase, do not start without my explicit go

- ASI plugin (C++) with an in-game hotkey that toggles farm mode at runtime. Requires runtime hooks and reverse engineering; only after everything else works.

---

## Phases

0. **Recon & feasibility.** Read all references, fingerprint my build, map every feature to concrete file/table/field or "unknown". Deliver `docs/FORMATS.md` and `docs/FEASIBILITY.md` with a traffic light per feature (green = field known, yellow = partially known, red = unknown / needs RE). **No app code in this phase.**
1. **Core.** Game dir detection (Steam/Epic/Game Pass), PAZ/PAMT read + decrypt + decompress, table parsers with round-trip tests, localization, SQLite index, CLI.
2. **App shell + A1** (item database).
3. **A2** (crafting calculator).
4. **Apply engine (B0) + B1–B3** (best documented modules first). B0 must pass its tests before any module writes to the game.
5. **B4–B11.**
6. **C** (profiles + farm mode).
7. **A4** (save reader + tracker).
8. **A3** (map).

## Definition of done (every phase)

- `cargo test` and frontend tests green; round-trip tests pass against the real game files.
- Apply engine: apply → game starts without errors; Restore vanilla → all touched files byte-identical to the snapshot (automated hash check). Test apply → restore → apply cycles.
- Mod modules: applied via B0, change visible in game. Write a manual in-game test checklist in `docs/TESTING.md` for each module; I verify in game.
- `docs/PROGRESS.md` updated: what works, what's still unknown, what broke, and anything you had to guess.
