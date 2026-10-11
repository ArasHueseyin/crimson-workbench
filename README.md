# Crimson Workbench

The current release is **Version 0.7.0**, including the free mod marketplace. [Download a release](https://github.com/ArasHueseyin/crimson-workbench/releases) or read the [installation guide](docs/INSTALLATION.md). The setup includes Live Items, Extra Sockets and their loader, detects the Steam game folder, and installs WebView2 if needed. No development tools, separate mod ZIP or PowerShell commands are required for the normal installation.

The interface and installer are in **English**. Item names and descriptions automatically follow the installed language in the selected game's Steam manifest. If it cannot be detected, the catalog uses English. You can still choose any of the 15 item languages manually. Separate in-game language overrides and Epic/Game Pass language settings are not detected yet.

The original Workbench code uses the [MIT license](LICENSE). Dependencies retain their own licenses, listed in [third-party notices](THIRD_PARTY_NOTICES.md). Game archives, saves and personal configurations are not distributed.

Development reports also describe earlier versions. For current installation and compatibility requirements, use the installation guide above. See the [phase overview](PHASENSTATUS.md), [test checklist](TESTCHECKLISTE.md) and [distribution guide](docs/WEITERGEBEN.md) for implementation status and remaining validation.

## Features and limits

The Tauri 2 desktop app includes an **item database** with 6,816 items, original icons, typed and raw fields, filters for type, category, tier, stackability and stat ID, and JSON export. Catalog support covers the observed Steam builds 25381195 / EXE 1.0.0.2944, 25455892 / EXE 1.0.0.2949 and 25477059 / EXE 1.0.0.2976, with matching data files. See [build support](docs/BUILD_SUPPORT.md).

The **crafting planner** covers 1,108 recipes, material trees, alternative ingredients and shared stock. Recipe links in item details open the planner directly. See [crafting](docs/CRAFTING.md).

The **Mod Workshop** previews shop inventories, additional items, daily refresh, drop quantities and chances, and positive trust gains. It supports 208 vendors and 12,306 drop sets, including individual vendor overrides, refresh exceptions, drop-set selection and explicit guaranteed drops. It exports unapplied overlays and rehearses apply, reapply, restore and interruption recovery on workspace copies. See [mod editing](docs/MODS.md).

The **free mod marketplace** bundles four curated recipes: a 30% larger lion model, 30% faster circus lion and Rokade mounts, no stamina cost for those mounts, and all 140 main-character abilities at maximum. Game mods share the Workshop preview and Live apply/restore system. All skills is a separate, explicitly confirmed save edit with a backup and preserved point accounting. These recipes support game EXE 1.0.0.2976. See [the marketplace guide](docs/MARKETPLACE.md).

Advanced editing covers spawn groups, mounts and Blackstar, inventory and storage, item stacks, durability, skill costs and cooldowns, item stats and buffs, and new enchantment levels copied from original levels. All 2,069 skill buff matrices expose raw values and original bytes. Another 87,697 named base/resource fields can be searched and inspected. Nine summon effects and one AddSubLevel effect have individual decoded fields; confirmed 64-bit buff values can be edited separately. Two spawn patrols expose reset factors, and 108 territory reoccupation rules can be edited or scaled. The city flight departure rule remains excluded. Inventory and storage values are provisionally capped at 1,460 slots, based on a confirmed inventory code path. Seven separate cost categories expose individual values and zero-cost switches. See [advanced mods](docs/ADVANCED_MODS.md) and [remaining phase 5 work](docs/PHASE5_REMAINING.md).

General NPC respawn timers, free repair and complete semantic skill-buff editing remain unfinished. An unsafe repair-material zeroing operation was removed in v0.5.5; older presets can be corrected. Development checks and manual in-game validation are tracked separately. The latter remains pending where noted in the feature documentation.

The transaction system provides these separate, explicit operations:

- [Backup and recovery](docs/BACKUP_RECOVERY.md): check a workspace rehearsal and preview its rollback before restoring a verified backup.
- [Installation check](docs/INSTALLATION_CHECK.md): compare file names and sizes with locally installed Steam depot metadata, without opening large archives.
- [Content audit](docs/CONTENT_AUDIT.md): compare SHA-1/SHA-256 hashes with progress, cancellation and report export while the game is closed.
- [Observed baseline](docs/BASELINE.md): retain an exported audit and verified registry copy, and recheck the backup and current metadata when reopening it.
- [Live apply and restore](docs/LIVE_APPLY.md): review files, check sources under launch protection, then explicitly apply, reapply or restore. A matching reading schema alone does not establish vanilla origin or authorize a write.
- [Foreign files](docs/FOREIGN_FILES.md): review and explicitly retain unchanged additional files. Conflicting modifications to original files remain blocked.

Workspace rehearsals test launch, source and backup locks on their own test files and reject simulated source updates. Changing a live baseline after an update archives owned files and history. Outside explicitly requested edit operations, the app reads the installation. Comprehensive world locations, merchant-source mapping and save analysis remain unfinished; supported save features are documented separately.

The optional Live Items and Extra Sockets modules have a narrower compatibility requirement: **the exact supported EXE 1.0.0.2976**. Their in-game validation remains pending. See [Live Items](docs/LIVE-ITEMS.md), [mount editing](docs/REITTIERE.md) and the installation guide for feature-specific requirements.

## Run or build the desktop app

For normal users, use the [release downloads](https://github.com/ArasHueseyin/crimson-workbench/releases). A local build produces `target/release/crimson-workbench.exe`. From a development checkout:

```powershell
.\Start-Workbench.ps1
```

The app runs without a development server. Installed and portable versions store settings, caches, backups and exports in `%LOCALAPPDATA%\CrimsonWorkbench`. Override that folder with `--project "C:\Path\To\Data"`. Development checkouts remain supported. Windows uses WebView2. See [desktop architecture and usage](docs/DESKTOP.md).

To build locally, install Node 22.22+, Rust 1.95+, the Windows SDK and C++ build tools:

```powershell
cd app
npm ci
npm run desktop:build
```

For a complete installer with freshly built native modules, follow the [distribution guide](docs/WEITERGEBEN.md). `npm run tauri dev` starts the development app; `npm run dev` starts only the frontend, which cannot load game data without Tauri.

## CLI

From the repository root, with Rust 1.95+ and the C++ build tools installed:

```powershell
cargo build --workspace --locked
cargo run --locked -p cd-cli -- detect
cargo run --locked -p cd-cli -- fingerprint
cargo run --locked -p cd-cli -- search Stumpfpfeil
```

Use `.env.example`, `CD_GAME_DIR` and `CD_SAVE_DIR` to configure paths on another computer. Without an explicit game path, the CLI tries Steam, Epic and Game Pass discovery. The CLI still defaults to German; `languages` lists the 15 catalog languages. Search caches are created under `.local/index/items.sqlite`.

The CLI writes ordinary JSON exports, overlay candidates, rehearsal copies and SQLite caches under `.local/` or `exports/`. Game and save folders are rejected as ordinary output destinations. The separate `live-setup`/`live-execute` path handles explicit B0 live transactions. CDUMM is a technical reference and is neither required nor invoked.

`iteminfo` has a complete structural reader with typed fields and raw regions. The initial generic reader for 13 other tables validates indexes and uninterpreted records; dedicated feature readers build on that foundation. A matching schema does not confirm vanilla provenance or permission to edit. Unknown builds are not read semantically. See the [CLI and core API guide](docs/CORE.md).

## Tests and documentation

```powershell
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --nocapture
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

Execution evidence is recorded in the [progress report](docs/PROGRESS.md). CI checks Windows and Linux; release packaging runs on Windows. Frontend and packaged-app checks are described in [testing](docs/TESTING.md).

- [Specification](SPEC.md)
- [Formats and observed builds](docs/FORMATS.md)
- [Feature feasibility](docs/FEASIBILITY.md)
- [Apply-engine design](docs/APPLY_DESIGN.md)
- [Credits and licenses](CREDITS.md)

`.env`, `.local/`, `exports/`, `.research/` and local contents of `reference/mods/` are excluded from Git. Game archives, extracted tables, textures and saves are not included.
