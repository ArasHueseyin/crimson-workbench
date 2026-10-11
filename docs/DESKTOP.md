# Desktop app: item database, crafting and mod previews

The application connects Tauri 2, React/TypeScript/Vite, Zustand, TanStack Table and TanStack Virtual with the reading Rust core. It does not download game content from the network. Game files, textures, test exports and screenshots remain local and are excluded from Git.

The new page **Crafting planner** and the recipe shortcuts are described in [CRAFTING.md](CRAFTING.md). The **Mod Workshop** with its conscious release limits is in [MODS.md](MODS.md). Total 31 native commands are allowed; `craft_info`, `craft_plan` and `craft_item` load recipes as needed. An installation/language change also discards the manual manufacturing plan and mod configuration. `mod_info`, `mod_preview`, `mod_export` and `mod_rehearse` interpret verified tables and write only project files if necessary. Six new live commands serve status, setup, file preview and a cancelable task. Only B0 is allowed to write controlled game paths. There is still no general FS/Shell plugin. [Live operation and limits](LIVE_APPLY.md).

## Current language behavior (v0.7.0)

The interface and installer use English. The catalog defaults to **Automatic** and reads the installed game language from the selected installation's Steam manifest. When detection is unavailable, it uses English. Existing manual preferences are retained; choose **Automatic** in the top bar to enable detection. Separate in-game overrides and Epic/Game Pass language settings are not detected yet. The optional selector still supports all 15 catalog languages. Changing the language reloads the catalog and discards the current crafting/mod draft.

The architecture and A1 coverage notes below describe earlier development stages; use [README](../README.md) for current feature coverage and [installation](INSTALLATION.md) for packaged-app setup.

## Operation

1. Start the installed app, or run `Start-Workbench.ps1` from a development checkout.
2. A unique or configured installation loads automatically. **Data sources** lets you select a detected installation or enter a path; **Reload** checks the files again.
3. **Ctrl+K** focuses search. Search names, descriptions and internal keys. The current app also supports substring and explicit regular-expression search.
4. Combine type, category and tier filters. **More filters** exposes stackability and stat IDs. Sorting applies to the entire result before pagination. The list loads 200 results per page and renders only the visible rows.
5. Select an item to view its overview, raw fields and direct item references. Enter/Space opens a focused row; arrow keys move between rows. Field search matches path, type, value or hex offset. Unknown fields can be shown separately. These detail fields are read-only.
6. **Save item as JSON** creates a new file under `exports/` in the data folder and displays its path. Existing files are not overwritten; game/save folders are rejected as export destinations.
7. Item language is automatic by default. A manual change clears the previous results and selection; indexes remain separate for each language and build.

Unknown/modified builds show a clear lock with diagnostic details. In the absence of installation, the path can be corrected. Missing search hits are a normal empty state. A browser without Tauri shows an explanation instead of invented example data.

## What A1 already covers and what is still missing

- All 6,816 items with localized name/description and readable field ranges.
The detail name uses the documented fallback in the absence of a translation.
- Real Item icons about SHA-tested `stringinfo` and the registered group
`0012/ui/texture/icon`. A missing or unsupported icon shows a neutral placeholder with justification, not an invented replacement image.
- Type and category are **Game IDs**, not verified category names.
Tier is the raw number. State filters refer to the confirmed typed hash references in `enchant_data_list` and `sharpness_data.stat_data`; Names and gameplay units of these stats are not confirmed.
- Direct fields of type `ItemKey` lead to existing items. Unresolved
IDs remain visible, but without a wrong link. Mere u32 numbers are never re-interpreted as item links.
- **Open from A1:** Dealer → City/Region → Price/Stock, DropSet → Source and
Complete recipe relationships. The existing MIT store source only reads keys/names, leaves the variable body untouched and labels its historical fixed stock size itself as incorrect. Therefore, no claimed sources from byteheuristics. This gap remains explicitly in progress; Phase 2 does not provide full A1 source coverage.
- No mod/profile/save/card operation. The later phases have not started.

## Technical limits

The current reading schema is `steam-25381195-gamedata-2.3-v2`. The version has been increased due to additional typed references/stat fields and invalidates old index details. The archive/itembytes are not changed. The additional icon schema dataset is in `crates/cd-core/schemas/steam-25381195.icons.json` and contains only hashes.

`stringinfo` has 31,812 entries: u32 key, four unknown bytes as u32, one unknown byte, u32 length and original stringbytes. The header has u16-count and u32-key/offset. Body and header are reconstructed and completely compared byte-identical. Also unknown flag values and non-UTF 8 bytes remain. The observed item 2200 string `ItemIcon_Prefab_cd_phm_04_arw_0020` receives the archive extension `.dds` for the assignment. The real DXT5 texture was decoded as PNG. DDS are limited to 2 MiB, 512×512 pixels, one level and a maximum of ten Mips; The DDS variants of the integrated `image` decoder are supported.

`BrowserSession` keeps the checked snapshot and an FTS index open. Search filters and sorting run before page selection. u64/i64 values outside the secure JavaScript range are transmitted as a decimal string. Raw bytes remain additionally visible. icons have limited memory caches; No texture atlas is exported.

Native commands run on Tauri worker threads. Session IDs and request counters prevent an old search or language change from replacing newer results. After a game update, the already loaded session remains a snapshot; **Read in** or App restart checks the new build. No filewatcher in phase 2.

The main local view receives only nine commands of its own: `bootstrap`, `open_catalog`, `search_items`, `item_detail`, `item_icon`, `export_item`, `craft_info`, `craft_plan`, `craft_item`. No shell, FS, HTTP, Apply or Save plugins. CSP allows for local app resources, IPC and local PNG data; Game strings are rendered as text. Tauri/WebView2 separately manages its own Webview runtime data under the Windows app data directory; Indexes and JSON remain in the project.

## Building and testing

From the repository root:

```powershell
npm ci --prefix app
npm run build --prefix app
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --nocapture
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

In the folder `app`: `npm run desktop:build` creates the self-startable release EXE without an installer. The build contains the surface; Vite does not have to run for use. For a fast embedded debug build: `cargo build -p app --features custom-protocol --locked` after the frontendbuild.

An invisible native UI test is possible with `--background-test`. Only in the case of this explicit argument does the window remain hidden; Regular launches show the surface. `scripts/native-smoke.mjs` only connects to a specially started test instance on port 9225. The port is activated exclusively via their process environment, not in the product or system-wide.

Windows Junctions: The Vite launcher normalizes its working directory to the physical path so that document folders redirected to another drive do not generate false absolute chunk names. `.npmrc` fixes the peer resolver mode used; Versions and integritys are in `package-lock.json`.

Linux requires the Tauri system libraries from the CI configuration. Linux-CI is set up, but not running here. No cross-platform test success is derived from the Windows build alone.
