# Optional: Live Items und Zusatzsockel

Dieses separate Paket ist experimentell und nur fuer **Windows x64 / Crimson
Desert 1.0.0.2976** mit passender EXE-Pruefsumme. Die Workbench funktioniert
auch ohne dieses Paket. Echte Spieltests auf einem zweiten PC stehen noch aus.
Das Paket enthaelt keine Spieldaten und keine persoenliche Sockelkonfiguration.

1. Workbench und Spiel schliessen. Vor Verwendung eigene Spielstaende sichern.
2. ZIP vollstaendig entpacken. PowerShell in diesem Ordner oeffnen.
3. Den **Spiel-Hauptordner** (enthaelt `bin64`) explizit einsetzen:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert' -InstallLoader
```

`-InstallLoader` installiert zusaetzlich den mitgelieferten Ultimate ASI Loader
9.7.4 x64 als `winmm.dll`. Eine bereits vorhandene identische Loader-Datei wird
nur verwendet und bleibt bei der Deinstallation bestehen. Andere vorhandene
Loader oder ASI-Dateien werden nie ersetzt. Ohne den Schalter wird ein bereits
vorhandener, genau passender Loader verlangt. Ein abweichender Loader muss
zuerst vom Nutzer geklaert werden. Das Paket laedt bei der Installation nichts
aus dem Internet nach.

Ist noch kein `winmm.dll` vorhanden, aber z. B. `dinput8.dll`, `version.dll`
oder ein anderer bekannter Proxyname, bricht die Loader-Installation zur
manuellen Kompatibilitaetspruefung ab. Entpack- und Spielordner duerfen keine
Symlinks oder Junctions enthalten.

Eine fehlende `CrimsonExtraSockets.dat` wird als gueltige leere V2-Konfiguration
angelegt. Eine vorhandene persoenliche Datei bleibt unveraendert. Gegenstaende
werden spaeter in Workbench ausgewaehlt. Live Items benoetigt einen gestarteten,
unterstuetzten Spielprozess; Aenderungen an Zusatzsockeln erfolgen bei
geschlossenem Spiel.

Deinstallation mit dem **urspruenglichen Paket** bei geschlossenem Spiel:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Uninstall-RuntimeMods.ps1 -GameDirectory 'D:\SteamLibrary\steamapps\common\Crimson Desert'
```

Nur selbst installierte Dateien mit unveraenderten Pruefsummen werden entfernt.
Persoenliche `.dat`-Dateien, Logs, fremde Mods und Spielstaende bleiben erhalten.
Eine geaenderte Datei fuehrt vor dem Entfernen zum Abbruch. Ein Spielupdate
verhindert die Neuinstallation, aber nicht die sichere Deinstallation.

Lizenzhinweise stehen in `LICENSE`, `THIRD_PARTY_NOTICES.md` und `licenses/`.
Downloadquellen und feste Pruefsummen stehen in `dependencies.lock.json`;
`runtime-manifest.json` prueft die zusammen gebauten ASIs und den Loader.
