# Developer guide

This is the current entry point for building and contributing. Packaged-app users should use [Installation](INSTALLATION.md). Older architecture and test reports are linked in the [documentation index](README.md).

## Requirements

| Work area | Requirements |
| --- | --- |
| Frontend, UI tests and documentation | Git; Node.js 22.22+ in the 22.x line; npm |
| Rust core, CLI and desktop | Stable Rust 1.95+ with `rustfmt` and `clippy`; a native C/C++ toolchain |
| Windows desktop | Visual Studio 2022 Build Tools with Desktop development with C++, Windows SDK and WebView2; use the `x86_64-pc-windows-msvc` Rust toolchain |
| Native game modules and release packaging | Windows x64, the Windows tools above, and CMake |
| Linux Rust/desktop checks | Tauri's Linux system dependencies; see below |

CI uses Node 22 and stable Rust on Windows, Linux and Apple Silicon macOS. `Cargo.lock` and `app/package-lock.json` are committed; use `--locked` and `npm ci`. No game installation, private fixture or `.env` is required for the default synthetic checks. Linux CI validates source portability; it does not provide a supported Linux game-module release. See [native macOS](MACOS.md) for Mac build requirements and validation limits.

For OS-specific setup, use the official [Tauri prerequisites](https://v2.tauri.app/start/prerequisites/). On Ubuntu/Debian, the dependency list used by this project's CI is:

```sh
sudo apt-get update
sudo apt-get install -y libwebkit2gtk-4.1-dev build-essential libxdo-dev libssl-dev librsvg2-dev libayatana-appindicator3-dev patchelf
```

## Get a checkout

Fork the repository on GitHub if you intend to submit changes, then clone your fork. For a read-only checkout:

```sh
git clone https://github.com/ArasHueseyin/crimson-workbench.git
cd crimson-workbench
npm ci --prefix app
```

Commands below run from the repository root unless a section says otherwise.

## Frontend without the game

```sh
npm run dev --prefix app
```

Open `http://127.0.0.1:1420`. A plain browser has no Tauri backend and shows a desktop-runtime explanation. It does not load game files or invent catalog data. The Playwright tests inject synthetic command responses to exercise the actual UI; fixture pages under `app/tests/` cover individual live-control panels.

```sh
npm run build --prefix app
npm test --prefix app
```

The build includes TypeScript checking and writes frontend assets to `app/dist/`.

## Desktop and CLI

Install the platform prerequisites first, then start the desktop development app:

```sh
rustup component add rustfmt clippy
npm run tauri --prefix app -- dev
```

Tauri starts the frontend server itself. Keep port 1420 free. To create the Windows desktop executable without an installer:

```sh
npm run desktop:build --prefix app
```

The executable is `target/release/crimson-workbench.exe`. This command does not rebuild or install game modules. For a complete installer with fresh, matching modules and package checks, use the [distribution guide](WEITERGEBEN.md).

The CLI is a separate Rust binary; help works without a game installation:

```sh
cargo run --locked -p cd-cli -- --help
```

See [Core and CLI](CORE.md) for commands. The CLI currently defaults to German item language; pass `--language eng` when appropriate.

## Checks

Choose checks for your change. The full PR matrix is defined in [Core CI](../.github/workflows/core.yml); required job names are `test (windows-2022)` and `test (ubuntu-latest)`.

Before workspace Rust checks, run `npm run build --prefix app` once to produce the frontend assets, as CI does.

| Change | Local checks |
| --- | --- |
| Documentation | Preview Markdown; check relative links, images and command paths |
| React/TypeScript | `npm run build --prefix app` and `npm test --prefix app`; run affected Playwright flows for UI behavior |
| Rust | `cargo fmt --all -- --check`, `cargo test --workspace --locked -- --nocapture`, `cargo clippy --workspace --all-targets --locked -- -D warnings` |
| Tauri bridge or permissions | Rust/frontend checks above, affected UI flows and `cargo build -p app --features custom-protocol --locked` after building the frontend |
| Native modules or packaging | Windows native/package checks below; record any separate in-game validation |

### Browser flows

Run the commands in this subsection from `app/` (use `cd app` from the repository root):

```sh
npm run test:e2e
```

Windows uses installed Microsoft Edge by default, matching CI. Other platforms use Playwright's Chromium. To install that browser and its Linux dependencies, use the official [Playwright browser setup](https://playwright.dev/docs/browsers):

```sh
npx playwright install --with-deps chromium
npm run test:e2e
```

On Windows without Edge, install Chromium with `npx playwright install chromium`, then set `PLAYWRIGHT_CHANNEL=chromium` for the test command. In PowerShell, from `app/`:

```powershell
$env:PLAYWRIGHT_CHANNEL = 'chromium'
npm run test:e2e
Remove-Item Env:\PLAYWRIGHT_CHANNEL
```

Tests run headlessly and start their own Vite server on port 1420. Stop any existing development server first. Results and failure screenshots go to `.local/playwright-results/`. To run a single flow from `app/`, use `npm run test:e2e -- tests/item-picker.spec.ts`.

### Windows native and package checks

From the repository root in PowerShell, with Visual Studio 2022 C++ tools and CMake installed:

```powershell
.\runtime\repair\test.ps1
.\scripts\build-native.ps1 -OutputDirectory .local/contributor-runtime
.\scripts\test-runtime-package.ps1 -PackageDirectory .local/contributor-runtime
.\scripts\test-marketplace-package.ps1 -PackageDirectory .local/contributor-runtime
```

Use a fresh output directory for each native build. The build script downloads dependencies pinned by SHA-256, builds the modules and runs their CTest contracts. These checks use synthetic hosts and folders; they do not install modules into your game. Full installer smoke checks belong to the Windows release pipeline.

### Optional real-installation tests

Copy `.env.example` to `.env` and configure `CD_GAME_DIR` and, if needed, `CD_SAVE_DIR` only when deliberately testing your own installation. Process environment values take precedence. Leave them unset for a synthetic-only run.

Tests such as `crates/cd-core/tests/live.rs` skip explicitly when no game path is configured. A configured but unsupported, missing or changed installation is an error. Some private/long-running checks are explicitly ignored; do not enable all ignored tests as a routine prerequisite. Read each test's requirements first. No game data belongs in the PR.

## Code map

| Location | Responsibility |
| --- | --- |
| `app/src/App.tsx`, `store.ts` | Page composition and frontend state |
| `app/src/api.ts`, `*-types.ts` | Typed frontend command boundary and payloads |
| `app/src/Marketplace*.tsx`, `ItemPicker.tsx`, `MountPicker.tsx` | Feature UI; adjacent `*.test.ts` files cover frontend logic |
| `app/tests/` | Playwright flows, synthetic Tauri responses and panel fixtures |
| `app/src-tauri/src/lib.rs`, `*_service.rs` | Tauri commands, service orchestration and native pipes |
| `app/src-tauri/build.rs`, `capabilities/` | Command declarations and desktop permissions; generated metadata lives under ignored `gen/` |
| `crates/cd-core/src/` | Discovery, supported-build validation, catalog/search, save operations and reusable domain logic |
| `crates/cd-core/src/mods/`, `apply/` | Mod plans and guarded apply/backup/recovery transactions |
| `crates/cd-core/schemas/`, `tests/` | Build-specific schema metadata and Rust integration tests |
| `crates/cd-cli/` | CLI entry point and commands |
| `runtime/live-items/`, `extra-sockets/`, `marketplace/`, `repair/` | Windows native modules, hooks, protocols and isolated native tests |
| `scripts/`, `.github/workflows/` | Module lifecycle, package validation, CI and release assembly |
| `vendor/crimson-format/` | Vendored format implementation; preserve attribution and review upstream changes separately |

For a new feature, trace the existing UI → typed API → Tauri command/service → core or native module path. New commands also need matching registration and permissions. Keep build-specific assumptions explicit, and follow the contribution guide's [write-operation requirements](../CONTRIBUTING.md#game-data-and-write-operations).

## Local files and troubleshooting

- Keep generated data in ignored `.local/`, `target/` or `exports/`. Installed-app data lives under `%LOCALAPPDATA%\CrimsonWorkbench`; `--project` selects another data folder. See [Installation](INSTALLATION.md).
- Use `npm ci` from this checkout. The committed `app/.npmrc` supplies the peer-dependency resolution setting used by the lockfile.
- Missing linker, Windows SDK or WebKit errors usually mean a platform prerequisite is missing. Frontend-only work does not need those desktop dependencies.
- A browser-only preview cannot establish gameplay behavior. Record the game build, mod configuration and actual result separately when a change needs an in-game test.
