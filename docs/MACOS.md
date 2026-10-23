# Native macOS preview

This port retains the upstream Tauri app and Rust data readers. It builds an
Apple Silicon `.app` and `.dmg` on macOS 15 or later.

## Available

- Native Steam installation detection, including additional Steam libraries.
- Select the Steam title directory, `CrimsonDesert_Steam.app`, or its
  `Contents/Resources/packages` directory.
- Save-directory discovery under `~/Library/Application Support/Pearl Abyss/CD/save`.
- Item browser, search, descriptions, icons, and crafting planner.
- Experimental native item spawning on the pinned Steam executable: launch from
  Item picker with a hash-verified save backup, then submit an explicit request.
  Receipts verify the inventory delta and prevent automatic retries.
  See [native inventory work](MACOS_ITEMS.md) for limits and live verification.
- Settings and local caches; installed apps use
  `~/Library/Application Support/CrimsonWorkbench`.

The catalog admits only exact matches for the checked-in table and localization
hashes. Every base table and the selected item localization are verified before
semantic parsing; crafting and icon tables have their own existing hash checks.
The Mac data reference is Windows build 1.0.0.2976. No Windows executable version
is invented, and a matching catalog does not certify a vanilla installation.
The Mac schema has its own cache identity. Future game updates may be rejected.

## Unsupported

Game-file application/restoration, save edits, runtime marketplace modules,
Extra Sockets, teleport, photo controls, and live world controls.
The existing process and executable guards continue to reject these operations
on macOS. Some upstream controls remain visible but cannot perform these actions.
Mod previews and exports are development tools; they have not been validated for
application to the Mac game. Normal catalog use does not change the game bundle,
signature, installed mods, or saves. Explicit inventory grants can be autosaved
by the game. The first one-Timber experiment passed independent UI confirmation
and save/reload verification; other item types have not been verified live.

## Build

Install Apple command-line developer tools, Node 22.22+ and Rust 1.95+.
From `app/`:

```sh
npm ci
npm run desktop:mac
```

The outputs are under `target/release/bundle/macos/` and
`target/release/bundle/dmg/`. The platform-specific Tauri configuration preserves
the upstream Windows installer configuration. Local builds are not Developer ID
signed or notarized; public distribution requires those separate release steps.
See [Tauri's macOS packaging documentation](https://tauri.app/distribute/macos-application-bundle/).

## Verification

```sh
cargo test --workspace --locked
cd app
npm test
npm run test:e2e -- tests/setup.spec.ts
```

An explicit read-only integration test opens the installed Mac archives and checks
the 6,816-item catalog and 1,108 usable crafting recipes. It writes a search cache
only in a temporary project:

```sh
CD_MAC_GAME_DIR="/path/to/CrimsonDesert_Steam.app/Contents/Resources/packages" \
  cargo test --locked -p cd-core --test macos_live -- --ignored --nocapture
```

The diagnostic example compares all 61 referenced base, advanced, crafting, icon,
and item-localization files without admitting a build or parsing game tables:

```sh
cargo run --locked -p cd-core --example inspect_mac_compatibility -- \
  "/path/to/CrimsonDesert_Steam.app/Contents/Resources/packages"
```

The original live tests requiring private Windows fixtures remain ignored by
default. Their status is separate from the Mac integration test above.
