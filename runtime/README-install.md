# Optional: Live items and extra sockets

The package bundles optional Marketplace ASIs under `addons/`. Use their **Install mod / Remove mod** buttons in the desktop app with the game closed. These optional modules are not automatically installed by the base scripts below. Each Marketplace card states whether it works while playing, offers live controls, or requires a closed game to apply changes. Live time/weather and teleport require the matching bundled LiveItems runtime. Photo mode and teleport have their own live panels. Teleport supports saved positions and eligible mainland markers; missing geometry and unsupported layers are refused. The time/weather card's **Reset weather** button releases the weather override independently of the clock. The 30-hour sleep mod uses the existing 12-hour option; Blackstar's placement, indoor and quest/state restrictions can remain.

This separate package is experimental and only for **Windows x64 / Crimson Desert 1.0.0.2976** with matching EXE checksum. The Workbench also works without this package. Real in-game tests on a second PC are still pending. The package does not contain any game data and no personal configuration.

1. Close Workbench and the game. Back up your own saves before use.
2. Fully unpack ZIP. Open PowerShell in this folder.
3. Explicitly use the **main game folder** (contains `bin64`):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert' -InstallLoader
```

`-InstallLoader` additionally installs the included Ultimate ASI Loader 9.7.4 x64 as `winmm.dll`. An already existing identical loader file is only used and is preserved during removal. Other existing loaders or ASI files are never replaced. Without the switch, an already existing, exactly matching loader is required. A deviating loader must first be cleared by the user. The package does not load anything from the Internet during installation.

If there is no `winmm.dll` yet, but e.g. `dinput8.dll`, `version.dll` or another known proxy name, loader installation stops for manual compatibility testing. Unpacking and game folders may not contain symlinks or junctions.

A missing `CrimsonExtraSockets.dat` is created as a valid empty V2 configuration. An existing personal file remains unchanged. Objects are selected later in Workbench. Live Items requires a running, supported game process; changes to additional sockets are made when the game is closed.

Uninstall with the **original package** when the game is closed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Uninstall-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert'
```

Only self-installed files with matching checksums are removed. Personal `.dat` files, logs, foreign mods and game saves are preserved. A modified file leads to cancellation before removal. A game update prevents reinstalling, but not safe uninstalling.

License notices are in `LICENSE`, `THIRD_PARTY_NOTICES.md` and `licenses/`. Download sources and fixed sums are in `dependencies.lock.json`; `runtime-manifest.json` checks the assembled ASIs and the loader.
