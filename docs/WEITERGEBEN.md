# Distributing Crimson Workbench

Since **v0.6.2 Preview**, releases provide a Windows x64 installer, a portable ZIP and a separate optional runtime-mod package. The installer already includes the live modules. Packages include the MIT license and third-party license texts, but no game archives, saves, personal backups, `.env` or developer settings.

For friends, use the [installation guide](INSTALLATION.md) and public [GitHub releases](https://github.com/ArasHueseyin/crimson-workbench/releases). Downloads require neither an invitation nor a GitHub account.

## What setup does

The NSIS setup installs Workbench for the current Windows user, creates a Start menu shortcut and downloads WebView2 from Microsoft if needed. The app stores data under `%LOCALAPPDATA%\CrimsonWorkbench`, including game/save paths and the catalog language preference. A `--project` argument overrides that data folder. The source checkout is not needed by normal users.

The same installer sets up both optional runtime modules. Its game-folder page detects Steam installations or accepts a folder containing `bin64`. Modules are selected by default and can be deselected. Setup checks the game EXE, package hashes and existing files, and requests administrator permission when necessary. Users do not need to run commands or extract another ZIP. Unattended `/S` installs only the app; `/INSTALLMODS` together with `/GAMEDIR="..."` explicitly enables modules for automated deployment. A new user receives an empty version 2 configuration, never another user's personal socket configuration.

## Build a release

Requirements: Windows x64, Node 22.22+, Rust 1.95+, Visual Studio 2022 C++ tools, the Windows SDK and CMake. From the repository root:

```powershell
.\scripts\build-release.ps1
```

The script downloads native dependencies pinned by SHA-256, builds LiveItems and then the matching Extra Sockets module, and embeds the module hash in the Rust build. It runs checks and creates the installer, portable ZIP, runtime-mod ZIP and `SHA256SUMS.txt` under `.local/release/<version>/assets`, using the version from `app/package.json`. Use a fresh `-OutputDirectory` for a subsequent run; existing release artifacts are not overwritten.

The ordinary desktop build remains available:

```powershell
Set-Location app
npm ci
npm run desktop:build
```

It uses the existing approved native hash and does not rebuild the native modules. Use `build-release.ps1` for a complete distribution.

## GitHub pipeline and testing

`.github/workflows/release.yml` runs manually or for a `v*` tag. A fresh Windows 2022 runner builds and checks the packages. The package test installs setup, checks the packaged WebView interface with isolated user data, verifies Marketplace module lifecycles in synthetic folders and uninstalls the test app. Assets are published as a regular **Latest** release only after successful checks for a matching version tag. Manual runs retain build artifacts. The tag, npm, Tauri and Cargo versions must agree.

The native loader test runs in its own synthetic process and verifies that the packaged ASIs reject an unrelated EXE. Automated evidence does not replace testing a friend's particular installation. Compatibility guards and documented limitations remain active in regular releases.

## Licensing and requirements

The [MIT license](../LICENSE) covers the original project code. It does not need to be purchased or applied for: include the standard license text with the copyright notice. Redistributors must retain those notices. Dependencies keep their separate licenses in `licenses/` and [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

The [installation guide](INSTALLATION.md) describes supported game builds, WebView2, the English interface, automatic item language, and the stricter EXE requirement of the optional modules. The installer is currently unsigned.
