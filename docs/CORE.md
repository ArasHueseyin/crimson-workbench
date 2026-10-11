# CLI and Reading Core

The CLI reads the local installation, checks known file hashes and makes items accessible as JSON and via a SQLite FTS5 search. This guide describes the existing commands; Concrete test results are in [PROGRESS.md](PROGRESS.md). The [Desktop interface](DESKTOP.md) uses the same core. Modexport and Apply/Restore are not yet implemented. Save directories are recognized, their contents are not yet evaluated.

## Build and configure

The examples are executed in PowerShell ** in the project folder**. Requires Rust 1.95 or newer and under Windows C++ build tools with Windows SDK; SQLite is being built.

```powershell
cargo build --workspace --locked
$cli = ".\target\debug\cd-cli.exe"
& $cli --help
```

Alternatively, each call can be executed as `cargo run --locked -p cd-cli -- <command>`. Under Linux, the built CLI is in `target/debug/cd-cli`.

The local `.env` is already set up. For another installation, `.env.example` can serve as a template:

```dotenv
CD_GAME_DIR=C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert
CD_SAVE_DIR=C:/Users/<User>/AppData/Local/Pearl Abyss/CD/save
```

Priority in game selection: `--game`, saved Workbench settings, process environment, `.env`, automatic detection. `--project` selects the project folder with `.env` and `settings.json`; Outputs and caches are limited to its subfolders `.local/` and `exports/`. Standard is the current directory. Relative configured paths refer to this project folder. For multiple automatically found installations, a selection is required. An explicitly set but invalid game path is reported as an error.

```powershell
& $cli detect
& $cli --game "D:\SteamLibrary\steamapps\common\Crimson Desert" detect
```

`detect` provides installation paths, platform, available build ID and save directories. The detection takes into account Steam libraries, epic manifests, popular Xbox/Game Pass directories, and Pearl Abyss save paths and Proton libraries. Inaccessible package installations require a readable explicit path; The CLI does not change access rights.

## Fingerprint and Roundtrip

```powershell
& $cli fingerprint
& $cli tables
& $cli roundtrip --all-fields
```

`fingerprint` checks EXE version, captured metadata, tables used and the selected item localization against the stored read build. Currently, the schema `steam-25381195-gamedata-2.3-v2` is stored. An update or altered input blocks the semantic interpretation. A diagnosis remains possible:

```powershell
& $cli fingerprint --metadata-only --output .local/build-diagnose.json
```

`--metadata-only` hashes EXE and captured metadata files without semantically reading tables. This mode does not confirm a supported reading schema. Even a fully matching fingerprint holds `certified_vanilla` **not**: known bytes are no proof of a clean original installation and no apply release.

`roundtrip` compares original bytes in memory with the serialization of table body/header, item info, PALOC and archive metadata. `--all-fields` additionally checks the seamless field coverage of all Iteminfo datasets. This doesn't write data back into the game. A raw data round trip confirms the receipt of the bytes, not automatically the meaning of the contained fields.

## Reading items and raw fields

```powershell
& $cli item 2200
& $cli item 2200 --fields
& $cli dump iteminfo --item 2200 --fields --output .local/item2200.json
```

`item` provides localized information and the typed item data set. `--fields` complements field areas and raw information. Unconfirmed meanings remain unnamed; Numerical categories are not provided with invented labels. Without `--output`, JSON is written to the standard output.

`dump iteminfo` outputs all item data sets. A full field dump with `--fields` requires `--item` to keep the output limited to one record.

There are **14 table indices**: `iteminfo` and 13 more tables:

```text
characterinfo       crafttoolgroupinfo  crafttoolinfo
dropsetinfo         fieldinfo           inventory
multichangeinfo     questinfo           regioninfo
skill               stageinfo           storeinfo
vehicleinfo
```

At `iteminfo`, the complete structure is typed read/serialized; Unknown areas are preserved. The other tables are treated as tested key/offset indices with **uninterpreted dataset bytes**:

```powershell
& $cli dump storeinfo
```

This call shows dataset limits and hashes. `dump <table> --item <key>` provides a selected raw data set as hexbytes. The actual keys come from the respective index. Recipes, shop inventory, trust values and skill costs are not yet offered as editable fields.

Since phase 3, the additional module `crafting` interprets recipe/group records and simple item dropsets. The existing `dump` outputs remain unchanged. CLI usage and limits: [CRAFTING.md](CRAFTING.md).

## German, other languages and search

```powershell
& $cli languages
& $cli index
& $cli search Stumpfpfeil
& $cli search Stumpfpfeil --limit 20 --offset 0
& $cli --language eng item 2200
```

Standard is `ger`; `de` is an alias. English accepted `eng` or `en`. The stored item languages are:

```text
ara eng fre ger ita jpn kor pol por-br rus spa-es spa-mx tur zho-cn zho-tw
```

The item PALOC file of the selected language is checked. If the translation of a single item is missing, existing standard text or internal key as a fallback. `name_resolved` distinguishes a name from a pure key fallback; it does not prove that the text came from the chosen PALOC. This is not a localization of all game texts or the pending GUI.

FTS5 searches in names, descriptions and internal keys. `--item-type` takes a numeric type, `--category` the category code visible in the dataset:

```powershell
& $cli search --item-type 1 --limit 20
```

The local cache is in `.local/index/items.sqlite` by default. `index` and `search` build it up as needed; a changed fingerprint, language change or schema change triggers a new setup. `--cache .local/index/anderer-name.sqlite` can choose a separate cache.

## Compare two builds

Two manifest files must be created first, for example before and after a game update. For comparable metadata reports, the same mode is used in both calls:

```powershell
& $cli fingerprint --metadata-only --output .local/before.json
# Create the following snapshot only at the later comparison time.
& $cli fingerprint --metadata-only --output .local/after.json
& $cli diff .local/before.json .local/after.json
```

`diff` compares the saved hashes and EXE versions without interpreting a newer build. A complete fingerprint additionally contains table/localization hashes; For such comparisons, both builds must have a supported reading schema. An empty diff only means that the values recorded in the manifest match; It is not an audit of all game archives.

## Local writing limit

`--output` only creates **new files under `.local/` or `exports/`** within the project. Other goals are also rejected if they are in the project. The same limitation applies to SQLite caches. Both output folders are excluded from Git. Existing JSON files are not overwritten; For another snapshot, a new name is required. Use `--output` instead of shell redirection so that the program's internal path protection is applied to the output target.

All recognized and explicitly configured game/save roots are protected, even if a configured directory does not yet exist. Existing directory links are dissolved. Existing hardlinks and linked SQLite files/additional files are rejected. Your own regular SQLite cache may be reused; This exemption does not apply to JSON dumps.

## Rust interfaces

|Entry|Zweck|
|---|---|
|`discover_project(project, game_override)`|Combine configuration and reading installation recognition.|
|`Workspace::open(project, game_override, language)`|Open verified game data and local output rules together.|
|`workspace.data()`|Immutable `GameData`; `tables`, `item_detail`, `dump_table`, `roundtrip` and `fingerprint`.|
|`workspace.build_index(cache)` / `workspace.search(query, cache)`|Build/request derived local index.|
|`workspace.write_json(path, value)`|Protected new JSON edition; No overwriting.|
|`fingerprint::Fingerprint::inspect(game)` / `fingerprint::diff(before, after)`|Diagnosis without semantic interpretation and comparison of stored fingerprints.|
|`PathPolicy::from_roots(roots, output_root)`|Protect all game/save roots against local spending.|
|`browser::BrowserSession`|Permanent desktop session: checked snapshot, language/build related index, paginated search, raw fields, real icons, and protected item exports.|

`GameData` does not provide a write operation to the installation. index intranas are private; Public workspace methods check the output paths before opening. The selectively integrated MIT format library is below `vendor/crimson-format`; Origin and boundaries are documented in [CREDITS.md](../CREDITS.md).

## Tests

```powershell
cargo test --workspace --locked
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

Synthetic tests do not require a game installation. The live round trip test reads `CD_GAME_DIR` from the environment or `.env`: without a configured path, it is explicitly skipped; A configured but missing, altered or unsupported build makes it fail. Live exams read exclusively; They do not generate a search cache and do not analyze save content.

[CI](../.github/workflows/core.yml) is set up for Windows and Linux. Linux did not run locally because there was no Docker daemon available. A configured workflow is not yet a successful Linux test run. Executions performed and remaining work are in [PROGRESS.md](PROGRESS.md); The current GUI is described in [DESKTOP.md](DESKTOP.md).
