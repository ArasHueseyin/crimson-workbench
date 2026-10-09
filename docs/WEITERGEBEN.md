# Crimson Workbench weitergeben

Die aktuelle EXE allein ist kein vollständig eingerichtetes Paket. Auf einem
anderen Windows-Rechner kann der Katalog mit der eigenen Spielinstallation
funktionieren. Schreibfunktionen und Laufzeitmods benötigen ihre eigene
Installation. Ein Test auf einem zweiten Rechner steht noch aus.

## Voraussetzungen

- Windows x64 mit WebView2; Workbench enthält die gebaute Oberfläche, benötigt
  keinen laufenden Entwicklungsserver.
- Eigene Crimson-Desert-Installation mit einem unterstützten Datenstand.
  Die aktuellen Zusatzsockel unterstützen ausschließlich die geprüfte
  Steam-EXE Build **1.0.0.2976**, SHA-256
  `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
  Eine andere EXE wird abgewiesen, auch bei ähnlich angezeigter Versionsnummer.
- Eigener Projekt-/Ausgabeordner. Bei verschobener EXE beispielsweise
  `crimson-workbench-live.exe --project "C:\CrimsonWorkbench"` verwenden.
  Steam-Pfade werden gesucht; einen erforderlichen eigenen Spielpfad über die
  Auswahl oder `CD_GAME_DIR` in der lokalen `.env` angeben.

## Zusatzsockel

Benötigt werden ein kompatibler ASI-Loader, der geprüfte
`CrimsonLiveItems.asi` und der aktuelle `CrimsonExtraSockets.asi` im Spielordner
`bin64`. Die Zusatzsockel-Mod wartet auf die abgeschlossene Prüfung des
LiveItems-Mods, damit dessen Erzeugungsweg kompatibel bleibt. Vorhandene Loader
und Mods müssen auf dem Zielrechner berücksichtigt werden; kein blindes
Überschreiben.

Die neue Konfiguration Version 2 unterstützt eigene Ausrüstungsinstanzen mit
null bis fünf offenen Basissockeln und fügt jeweils zehn zusätzliche hinzu.
Sie kann leer angelegt werden und muss anschließend mit **den Gegenständen
des Empfängers** über Workbench → Zusatzsockel eingerichtet werden. Deine
aktuelle `.dat` enthält deine konkreten Instanz-IDs und sollte nicht als
Konfiguration des Empfängers verteilt werden. Das Format hat SHA-256 und bis
zu 256 Instanzen; die vorhandene Version 1 mit deinen drei Gegenständen bleibt
lesbar und wird beim ersten weiteren Gegenstand auf Version 2 erweitert.

Das Spiel muss vor jeder Sockeländerung gespeichert und geschlossen sein.
Die normalen Saves behalten ihr ursprüngliches Fünf-Sockel-Format. Zusätzliche
Plätze und Steine bleiben in der separaten `.dat`; die Spieloberfläche zeigt
höchstens fünf Sockel, die weiteren werden in Workbench verwaltet.

## Was nicht mitgeschickt werden sollte

Eigene Saves, Backups, Exporte, Logs, `.env`, Mod-Konfigurationen mit eigenen
Instanz-IDs oder private Cache-Ordner gehören nicht in ein Weitergabepaket.
Auch Spielarchive werden nicht benötigt: Workbench liest die eigene lokale
Installation des Empfängers.

Ein portabler Installer, automatische Ersteinrichtung der Zusatzsockel und
ein vollständiges geprüftes Weitergabepaket sind aktuell **noch nicht gebaut**.
Alle anderen persönlichen Änderungen an Spielarchiven oder Reittieren werden
durch das Weitergeben der Workbench-EXE nicht übertragen.

## GitHub: aktueller Stand und Installation

Stand der Prüfung: 10.10.2026. Ein Git-Push stellt zunächst den Quellcode bereit.
Das Repository [ArasHueseyin/crimson-workbench](https://github.com/ArasHueseyin/crimson-workbench)
ist öffentlich zugänglich; zum Herunterladen ist keine Einladung erforderlich.
Ein fertiger Windows-Installer oder Release-Workflow ist noch nicht vorhanden:
`app/package.json` baut mit `--no-bundle`, in
`app/src-tauri/tauri.conf.json` ist `bundle.active` ausgeschaltet.

Mit Repository-Zugriff können technisch erfahrene Freunde bereits selbst bauen.
Voraussetzungen: Windows x64, WebView2, Git, Node 22.22+, Rust 1.95+ sowie
Visual-Studio-C++-Buildwerkzeuge mit Windows SDK. Bei einem privaten Repository
muss das eigene GitHub-Konto vorher eingeladen und `gh auth login` ausgeführt
worden sein. In PowerShell:

```powershell
gh repo clone ArasHueseyin/crimson-workbench
Set-Location crimson-workbench
npm ci --prefix app
npm run desktop:build --prefix app
.\Start-Workbench.ps1
```

Die automatische Erkennung sucht die eigene Installation. Die Spielpfadauswahl
in der Oberfläche gilt derzeit nur für die laufende Sitzung. Für dauerhafte
Pfade `.env.example` nach `.env` kopieren und die eigenen vollständigen Pfade
eintragen; Umgebungsvariablen werden innerhalb der `.env` nicht expandiert.
Spielarchive, Saves, Caches und persönliche Mod-Konfigurationen werden nicht
mit dem Repository verteilt.

Der Katalog kennt aktuell Steam 25381195 / EXE 1.0.0.2944,
25455892 / 1.0.0.2949 und 25477059 / 1.0.0.2976. Die jeweiligen
Datei-/Tabellenprüfungen müssen ebenfalls passen. Daraus folgt keine allgemeine
Unterstützung künftiger Updates. Die Erkennung von Epic und Game Pass bedeutet
auch keine bestätigte Unterstützung aller Funktionen: Reittiere und Zusatzsockel
verwenden derzeit fest den Steam-Save-Stamm unter
`%LOCALAPPDATA%\Pearl Abyss\CD\save` und berücksichtigen dort `CD_SAVE_DIR` nicht.

## Was für eine einfache Installation durch Freunde fehlt

1. **Datenordner und Ersteinrichtung:** Einen beschreibbaren Benutzerordner
   automatisch anlegen, Spiel-/Save-Auswahl speichern und unbekannte Builds
   verständlich anzeigen. `project_root()` sucht derzeit den Quellcodeordner
   oder verwendet `--project` beziehungsweise das Arbeitsverzeichnis.
   Eine Installation unter `Program Files` darf dort keine Caches anlegen.
2. **Windows-Paket:** Tauri-Bundling aktivieren, einen NSIS-Setup-Installer oder
   ein geprüftes portables ZIP bauen und WebView2 berücksichtigen. Die normalen
   Schemas sind bereits in die EXE eingebettet; private Entwicklungsordner sind
   keine notwendige Beigabe. Für ein portables Paket muss der Launcher explizit
   einen beschreibbaren `--project`-Ordner wählen.
3. **Optionale Laufzeitmods:** `CrimsonLiveItems.asi` und
   `CrimsonExtraSockets.asi` getrennt und reproduzierbar bauen. Beide benötigen
   derzeit eine externe MinHook-1.3.4-Quelle über `MINHOOK_ROOT` und einen
   kompatiblen ASI-Loader. Version und vorhandene Mods prüfen, Installation und
   Rückbau erklären und mit einer leeren Konfiguration beginnen. Der normale
   Desktop-Build baut oder installiert diese Komponenten nicht.
   Die Binärhashes sind gegenseitig fest gepinnt: zuerst LiveItems bauen,
   dessen Hash im Zusatzsockel-Modul aktualisieren, Zusatzsockel bauen und
   dessen Hash im Rust-Core aktualisieren. Eine gültige leere V2-Sockeldatei
   muss beim Erststart erzeugt werden; aktuell wird eine vorhandene Datei
   erwartet. Die echte Spielabnahme der nativen Module steht noch aus.
4. **GitHub Release:** Einen Windows-Release-Workflow ergänzen, der eine
   festgelegte Version prüft, das Setup baut und zusammen mit Prüfsummen und
   Versionshinweisen als Release-Anhang bereitstellt. Die bestehende
   `.github/workflows/core.yml` prüft Code, erzeugt aber keine Release-Downloads.
5. **Weitergabe und Abnahme:** Eine Lizenz für den eigenen Projektcode festlegen,
   die benötigten Fremdlizenzen einschließlich MinHook ins Paket aufnehmen und
   das Paket auf einem frischen zweiten Windows-PC ohne Entwicklungswerkzeuge
   testen. Dabei Start, eigene Pfade, unterstützte/abgelehnte Builds und bei
   optionalen Schreibfunktionen Sicherung und Rückbau prüfen.

Tauri dokumentiert [Windows-Installer und WebView2](https://v2.tauri.app/distribute/windows-installer/).
Nach Veröffentlichung eines passenden Setup-Anhangs wäre der Download etwa:

```powershell
gh release download --repo ArasHueseyin/crimson-workbench --pattern '*-setup.exe'
```

Dieser Befehl ist für das künftige Release gedacht und installiert nichts
automatisch. Anschließend die heruntergeladene Setup-Datei starten. Freunde
können sie ebenso direkt auf der GitHub-Releases-Seite herunterladen; `gh` ist
keine Voraussetzung für die fertige App.
Siehe [GitHub-CLI-Dokumentation](https://cli.github.com/manual/gh_release_download).

Ein öffentliches Repository macht die späteren Release-Downloads allgemein
zugänglich. Bei einem privaten Repository benötigen Freunde Zugriff auf das
Repository. Technisch bleibt die App auf die jeweils unterstützten Windows-
und Spielversionen beschränkt. Die
[GitHub-Sichtbarkeit](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/managing-repository-settings/setting-repository-visibility)
und die technische Installierbarkeit sind getrennte Voraussetzungen.
