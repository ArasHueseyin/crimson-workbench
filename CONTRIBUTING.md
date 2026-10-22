# Contributing to Crimson Workbench

Contributions are welcome: bug fixes, UI improvements, documentation, format research and mod ideas. You can work on the frontend and synthetic tests without a game installation.

## Before you start

Search [existing issues](https://github.com/ArasHueseyin/crimson-workbench/issues) and [pull requests](https://github.com/ArasHueseyin/crimson-workbench/pulls). Use the [issue forms](https://github.com/ArasHueseyin/crimson-workbench/issues/new/choose) for bugs, feature requests, mod requests or feedback. For a large change or a new native hook, discuss the intended behavior and compatibility limits in an issue first. Small fixes can go straight to a PR.

See [Development](docs/DEVELOPMENT.md) for requirements, setup, the code map and checks. The [documentation index](docs/README.md) separates user guides from older research notes.

## Pull request workflow

1. Fork the repository and create a focused branch from `master` in your fork. Maintainers can use a branch in this repository.
2. Make the smallest change that solves the problem. Keep unrelated formatting and generated files out of the diff.
3. Run checks for the affected layers using the [test guide](docs/DEVELOPMENT.md#checks). Add a regression test for a bug when it meaningfully protects the behavior. Documentation-only changes need a link and preview check, not a full native build.
4. Open a PR against `master`. Describe the problem, resulting behavior and validation. Include screenshots for visible UI changes, and distinguish automated checks from actual in-game tests.

`master` requires a PR and successful `test (windows-2022)` and `test (ubuntu-latest)` checks. The branch must be up to date before merging. Force pushes and deletion are blocked; a second maintainer's approval is not required.

## Code and documentation conventions

- Write new UI text, documentation, issues and PR descriptions in English. Preserve the user's selected item language when displaying game content.
- Follow the surrounding code and `.editorconfig`. Rust is formatted with `cargo fmt --all`. There is currently no repository-wide TypeScript/C++ formatter; avoid unrelated reformatting.
- Keep reusable parsing, validation and change planning in `cd-core`; keep Tauri commands and UI code focused on their respective boundaries. See the [code map](docs/DEVELOPMENT.md#code-map).
- Update the relevant feature guide when changing behavior, installation requirements or supported builds. Describe unverified behavior as unverified.
- Retain upstream credits and license notices. Document the source and license of any new dependency or borrowed code in `THIRD_PARTY_NOTICES.md` and `licenses/` as appropriate.

## Game data and write operations

Use synthetic fixtures for committed tests. Do not commit game archives, extracted tables, textures, saves, account information, personal paths, `.env`, `.local/`, `.research/`, `exports/`, or privately licensed reference mods. A small video demonstrating the app belongs under `docs/assets/`; it is not a source-data fixture.

Changes that write saves or game files must retain the existing compatibility checks, explicit user action, backup and restore behavior, conflict checks and closed-game requirements. Tests should cover relevant failure and recovery paths. Native runtime changes must also verify the supported EXE, object ownership/lifetime and failure behavior; automated tests alone do not establish in-game compatibility.

When reporting a bug, include Workbench/game versions, reproduction steps and the exact error. Redact personal information from logs and screenshots. Do not attach whole saves or game archives to public issues.

## License

By contributing original code, you agree to license it under this project's [MIT license](LICENSE). Third-party code keeps its applicable license and required notices.
