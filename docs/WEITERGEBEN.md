# Crimson Workbench weitergeben

Seit Version **0.6.1 Preview** gibt es einen Windows-x64-Installer, ein portables
ZIP und ein getrenntes Paket für die optionalen Live-Mods. Die Releases enthalten
MIT-Lizenz und Fremdlizenztexte, keine Spielarchive, Saves, persönlichen Backups,
`.env` oder Einstellungen des Entwicklers.

Für Freunde: [Installation und Voraussetzungen](INSTALLATION.md).
Die Downloads stehen im öffentlichen
[GitHub-Repository](https://github.com/ArasHueseyin/crimson-workbench/releases).
Zum Herunterladen ist keine Einladung und kein GitHub-Konto erforderlich.

## Was das Setup erledigt

Das NSIS-Setup installiert die Workbench für das eigene Windows-Benutzerkonto,
legt einen Startmenüeintrag an und lädt fehlendes WebView2 bei Bedarf von Microsoft.
Die Workbench verwendet `%LOCALAPPDATA%\CrimsonWorkbench` für ihre Daten und
speichert Spiel-/Savepfade und Sprache. Ein `--project`-Ordner kann diesen
Standard überschreiben. Der Projektcheckout ist für normale Nutzer nicht nötig.

Die beiden optionalen Laufzeitmods werden nur mit ihrem gesonderten Skript
installiert. Dieses prüft die eigene EXE, die Paket-Hashes und vorhandene Dateien.
Persönliche Sockelkonfigurationen anderer Personen sind keine gültige Vorlage.
Ein neuer Nutzer beginnt mit einer leeren Version-2-Konfiguration.

## Release selbst bauen

Benötigt werden Windows x64, Node 22.22+, Rust 1.95+, Visual Studio 2022 mit
C++-Werkzeugen/Windows SDK und CMake. Im Repository:

```powershell
.\scripts\build-release.ps1
```

Das Skript lädt SHA-256-gepinnte native Abhängigkeiten, baut zuerst LiveItems und
anschließend das dazu passende Zusatzsockel-Modul. Dessen Hash wird beim
anschließenden Rust-Build eingebettet. Es führt Codeprüfungen aus und erstellt
unter `.local/release/0.6.1/assets` Setup, portables ZIP, optionales Mod-ZIP und
`SHA256SUMS.txt`. Für einen weiteren Durchlauf einen frischen
`-OutputDirectory` angeben; vorhandene Release-Artefakte werden nicht überschrieben.

Der separate normale Desktop-Build bleibt verfügbar:

```powershell
Set-Location app
npm ci
npm run desktop:build
```

Dieser enthält die bisherige native Hashfreigabe, aber keine neu gebauten Mods.
Für die gemeinsame Verteilung immer `build-release.ps1` verwenden.

## GitHub-Pipeline und Prüfung

`.github/workflows/release.yml` läuft manuell oder bei einem `v*`-Tag. Ein frischer
Windows-2022-Runner baut und prüft die Pakete. Der Pakettest installiert das Setup,
prüft die echte gepackte WebView-Oberfläche in einem isolierten Benutzer-Datenordner
und deinstalliert die Testinstallation. Erst nach erfolgreicher Prüfung werden
bei einem passenden Versionstag die Assets als öffentliches Preview-Release
bereitgestellt. Bei manuellen Workflow-Läufen bleiben sie als Build-Artefakte
verfügbar. Tag, npm-, Tauri- und Cargo-Version müssen übereinstimmen.

Die native Loader-Prüfung läuft ausschließlich in einem eigenen Testprozess.
Sie prüft auch, dass die Module eine fremde EXE ablehnen. Sie ist kein Ersatz für
die noch ausstehende Spielabnahme der Live-Items-/Zusatzsockel-Funktionen.
Der Pakettest auf GitHub ersetzt keinen Test mit der konkreten Installation eines
Freundes. Entsprechend wird das erste Paket als **Preview** verteilt.

## Lizenz und Grenzen

Die [MIT-Lizenz](../LICENSE) gilt für den eigenen Projektcode. Sie muss nicht
gekauft oder beantragt werden; die standardisierte Lizenz wird dem Projekt mit
Copyright-Hinweis beigelegt. Die eigenen Copyright- und Lizenzhinweise bleiben
beim Weitergeben erhalten. Fremdkomponenten behalten ihre separaten Lizenztexte
unter `licenses/` und in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

Unterstützte Spielstände, notwendige WebView2-Laufzeit und die engere EXE-Bindung
der optionalen Mods stehen in der [Installationsanleitung](INSTALLATION.md).
Die Setup-Datei ist derzeit nicht code-signiert.
