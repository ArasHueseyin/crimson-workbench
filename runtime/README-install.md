# Optional: Live items and extra sockets

This separate package is experimental and only for **Windows x64 / Crimson Desert 1.0.0.2976** with matching EXE point sum. The Workbench also works without this package. Real in-game tests on a second PC are still pending. The package does not contain any game data and no personal base configuration.

1. Workbench and close the game. Secure your own game stadiums before use.
2. Fully unpack ZIP. Open PowerShell in this folder.
3. Explicitly use the ** main game folder** (contains `bin64`):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert' -InstallLoader
```

`-InstallLoader` additionally installs the included Ultimate ASI Loader 9.7.4 x64 as `winmm.dll`. An already existing identical loader file is only used and remains uninstalled. Other existing loaders or ASI files are never replaced. Without the switch, an already existing, exactly matching loader is required. A deviating loader must first be cleared by the user. The package does not load anything from the Internet during installation.

If there is no `winmm.dll` yet, but e.g. `dinput8.dll`, `version.dll` or another known proxy name, cancels loader installation for manual compatibility testing. Unpacking and game folders may not contain symlinks or junctions.

A missing `CrimsonExtraSockets.dat` is created as a valid empty V2 configuration. An existing personal file remains unchanged. Objects are selected later in Workbench. Live Items requires a launched, supported gaming process; Changes to additional sockets are made when the game is closed.

Uninstall with the **original package** when the game is closed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Uninstall-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert'
```

Only self-installed files with unmodified purchase sums are removed. Personal `.dat` files, logs, foreign mods and game stadiums are preserved. A modified file leads to cancellation before removal. A game update prevents reinstalling, but not safe uninstalling.

License notices are in `LICENSE`, `THIRD_PARTY_NOTICES.md` and `licenses/`. Download sources and fixed sums are in `dependencies.lock.json`; `runtime-manifest.json` checks the assembled ASIs and the loader.
