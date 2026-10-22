# Install Crimson Workbench

This guide describes the expanded **v0.10.0** feature set. Check GitHub Releases
for published installers; older releases contain their earlier feature sets.

## Windows setup

1. Open [GitHub Releases](https://github.com/ArasHueseyin/crimson-workbench/releases) and download `Crimson-Workbench_0.10.0_x64-setup.exe`. The automatically generated source-code ZIPs are for developers.
2. Close Crimson Desert and Workbench, then run the installer. **Live Items, Extra Sockets and their loader are included and selected by default.** Setup detects Steam game folders. If necessary, select your game installation folder: the folder that **contains `bin64`**. Windows may request administrator permission to install the game modules. You can deselect them to install only Workbench. No separate ZIP or PowerShell commands are needed. Setup and the app interface are in English. If WebView2 is missing, setup downloads it from Microsoft and needs an internet connection.
3. Open Crimson Workbench from the Start menu. If the game is not detected, open **Data sources**, select your installation and, if necessary, your save folder, then choose **Save & connect**.

You need Windows x64 and your own compatible game installation. The finished app does not require Rust, Node.js, a GitHub account or the GitHub CLI. The packages are currently unsigned, so Windows may show an unknown publisher. Compare the download with the supplied `SHA256SUMS.txt` before running it.

## Automatic item language

The interface stays in English. With the default **Automatic** setting, item names and descriptions follow the installed language recorded in the selected game's Steam manifest. Workbench reads `MountedConfig.language`, or `UserConfig.language` when the mounted section is absent. It falls back to English when detection is unavailable and never changes your game or Steam settings. Reopening the catalog picks up changes.

Separate in-game language overrides and Epic/Game Pass language settings are not detected yet. You can choose a catalog language manually in the top bar. Existing manual preferences are preserved during an update; choose **Automatic** once if you want an older installation to follow Steam.

## Portable package and personal data

Alternatively, extract `Crimson-Workbench_0.10.0_windows-x64-portable.zip` into a folder and run `crimson-workbench.exe`. WebView2 must already be installed. The portable package does not automatically install the game modules.

By default, settings, caches, backups and exports are stored under `%LOCALAPPDATA%\CrimsonWorkbench`. No source-code checkout is needed. Use `--project "D:\My Workbench Data"` to select another writable data folder. Existing development checkouts and their `.env` files remain supported.

Uninstalling Workbench removes the app and preserves your user data and game installation. Close Workbench before updating; a newer installer updates it in the existing location.

## Marketplace runtime mods

The expanded setup and portable package bundle all **20 optional Marketplace runtime mods**, including time/weather, photo mode and teleport. Time/weather and teleport require the matching bundled Live Items module; their live panels work while playing after setup. With the bundled loader installed, close the game and use **Install mod** on the desired Marketplace card. Windows may request administrator permission. Workbench stays open. Start the game afterward. **Remove mod** disables only that card's managed ASI; saves, other mods and personal socket data are preserved. Recognized personal versions offer **Import & update mod**, which backs up the original module first. See the [marketplace guide](MARKETPLACE.md) for behavior and remaining Blackstar restrictions.

## Compatibility

The catalog supports these observed Steam builds, with matching checked table and file sizes:

| Steam build | EXE version |
| --- | --- |
| 25381195 | 1.0.0.2944 |
| 25455892 | 1.0.0.2949 |
| 25477059 | 1.0.0.2976 |

After a game update, affected features may remain unavailable until the new build is checked. Detecting an Epic or Game Pass installation does not establish full compatibility with that build.

## Live Items and Extra Sockets

Since v0.6.2, the same setup EXE installs these optional modules. Reinstalling recognizes identical modules and preserves their configuration. Different or modified mods are not overwritten: setup reports the conflict and stops the module installation while leaving the Workbench app installed.

Uninstalling the app does not remove game files. For targeted module removal, the app's `runtime-mods` folder contains the original `Uninstall-RuntimeMods.ps1`. Portable users can also download `Crimson-Workbench_0.10.0_optional-runtime-mods.zip`, which includes both modules, install/uninstall scripts, licenses and a neutral starting configuration. Follow its `README.md`. The scripts check the supported EXE, package files and existing mods before writing. Do not replace an existing loader with an unrelated file.

These modules support only the tested **EXE 1.0.0.2976**, SHA-256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`. Compatibility checks still apply to each game installation and feature. Save and close the game before installation, socket changes or module removal. Do not replace your saves or configuration with another user's files.

### Item picker blocked after an uncertain result

If the game reports an error during an item grant or its additional quantity cannot be confirmed, the Live Items module blocks further grants for that game session. The module can still be connected while **Add to inventory** is disabled. Check the inventory before retrying, then save if appropriate and restart Crimson Desert to reset the session block. If Workbench still shows the old request afterward, choose **Result checked in game** only after checking its outcome. Workbench never automatically repeats an uncertain grant.

## License

The original Workbench code is under the included MIT license. You may use, modify and redistribute it while retaining the copyright and license notices. Third-party components keep their own licenses in `THIRD_PARTY_NOTICES.md` and `licenses/`. Game content is read from your own installation and is not part of the package. No additional Workbench license purchase is needed.

## Optional GitHub CLI download

With the GitHub CLI installed:

```powershell
gh release download v0.10.0 --repo ArasHueseyin/crimson-workbench --pattern '*-setup.exe'
```

Run the downloaded setup file afterward; downloading alone does not install it. To check a file's SHA-256 checksum, use `Get-FileHash .\<Filename> -Algorithm SHA256`.
