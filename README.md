# Crimson Workbench

**Version 0.6.1 Preview:** Windows-x64-Setup und portables Paket für die eigene
Crimson-Desert-Installation. [Downloads auf GitHub](https://github.com/ArasHueseyin/crimson-workbench/releases)
und [Installationsanleitung](docs/INSTALLATION.md). Das Setup berücksichtigt
WebView2; für die fertige App sind keine Entwicklungswerkzeuge erforderlich.
Einstellungen und Arbeitsdaten liegen im Benutzerprofil. Live-Items und
Zusatzsockel werden als getrenntes optionales Mod-Paket bereitgestellt.

Der eigene Code steht unter der [MIT-Lizenz](LICENSE). Die Lizenztexte der
Abhängigkeiten werden mitgeliefert: [Fremdkomponenten](THIRD_PARTY_NOTICES.md).
Spielarchive, Saves und persönliche Konfigurationen werden nicht verteilt.

Die folgenden Entwicklungsberichte beschreiben auch frühere Versionen; für die
Installation und Kompatibilität von 0.6.1 gilt die obige Installationsanleitung.

[Phasenübersicht: implementiert, offen und nächste Schritte](PHASENSTATUS.md).
[Konkrete Testcheckliste](TESTCHECKLISTE.md).
[Weitergabe, Voraussetzungen und GitHub-Installation](docs/WEITERGEBEN.md).

**Buildhinweis 22.09.2026:** Die Installation wurde auf Steam 25455892 / EXE
1.0.0.2949 aktualisiert. Workbench v0.5.9 unterstützt dessen Tabellen zusätzlich
zum bisherigen Build 25381195 / 1.0.0.2944. Die native Reparaturanbindung ist
weiterhin gesondert offen. [Buildvergleich und Grenzen](docs/BUILD_SUPPORT.md).

Crimson Workbench hat eine **Tauri-2-Desktopoberfläche mit Itemdatenbank**:
6.816 Gegenstände durchsuchen, echte Icons und alle Felder ansehen, nach Typ,
Kategorie, Tier, Stapelbarkeit und Stat-ID filtern sowie einzelne Items als JSON
speichern. Deutsch ist Standard; 15 Itemsprachen sind verfügbar.

Der neue **Herstellungsplan** berechnet 1.108 Rezepte mit Materialbaum,
Materialalternativen und gemeinsamen Vorräten. Rezeptlinks führen aus den
Itemdetails direkt zum Rechner. [Bedienung und Formatnachweise](docs/CRAFTING.md).

Die **Modwerkstatt (v0.5.9)** plant Shopbestände, Zusatzartikel,
täglichen Refresh, Dropmengen/-chancen und positive Trust-Zuwächse.
Neu sind Spawn-Gruppen, Reittiere/Blackstar, Inventar und Lager, Stacks,
Haltbarkeit, Skillkosten/-cooldowns und Item-Stats/Buffs mit Vorlagen.
Neue Enchant-Stufen lassen sich aus Originalstufen kopieren. Die Buffmatrizen
aller 2.069 Skills sind mit Rohwerten und vollständigen Originalbytes inspizierbar.
Zusätzlich sind 87.697 Basis-/Ressourcenfelder benannt, nach Namen oder Werten
durchsuchbar und schreibgeschützt sichtbar. Der Leser folgt jetzt durchgehend
der Tabellenstruktur, ohne eine wiederholte Skillkennung zu suchen.
Neun Beschwörungseffekte und ein AddSubLevel-Effekt sind jetzt in einzelne
Felder aufgeteilt; die Suche findet auch Feldnamen. Bestätigte 64-Bit-Buffwerte
sind einzeln editierbar. Zwei Spawn-Patrouillen haben
Reset-Faktoren und Einzelwerte; die Stadtflug-Abstiegsregel wird gezielt aufgehoben.
108 Gebietswiederbesetzungsregeln lassen sich getrennt skalieren oder einzeln
bearbeiten. Inventar-/Lagerwerte sind vorläufig auf 1.460 Plätze begrenzt,
abgeleitet vom Standarddeckel eines bestätigten Inventar-Codepfads.
Neu in v0.5.8: zusätzliche Ausrüstungs- und Skill-Eigenkosten; sieben getrennte Kostenkategorien, Einzelwerte und gemeinsame Nullkosten-Schalter.
[Prüfung aller Phase-5-Restpunkte](docs/PHASE5_REMAINING.md).
Phase 5 ist teilweise umgesetzt: allgemeine NPC-Respawn-Timer,
Kostenfreie Reparatur und vollständige semantische Skill-Buffbearbeitung bleiben offen.
Ein als unsicher nachgewiesenes Nullsetzen von Reparaturmaterialkosten wurde
in v0.5.5 aus dem Writer entfernt; alte Vorlagen lassen sich gezielt korrigieren.
[B4–B11: Bedienung und genaue Grenzen](docs/ADVANCED_MODS.md).
Individuelle Artikelsets und Refresh-Ausnahmen pro Händler, Dropsetauswahl,
Einzelausnahmen und manuelle Dropgarantien sind verfügbar. Sie zeigt alle
Feldänderungen und geplanten Dateien, exportiert noch nicht angewendete Overlays
und probt Apply, Wiederanwendung, Restore und Abbruch-Recovery an einer Projektkopie.
Der Transaktionskern unterstützt mehrere Overlaygruppen; unterbrochene Projektproben
lassen sich per CLI und über die neue
[Sicherungs-/Wiederherstellungsansicht](docs/BACKUP_RECOVERY.md) prüfen und gezielt
zurücksetzen. Die Rücknahme verlangt eine aktuelle Vorschau und eine vorhandene,
geprüfte Sicherung. Neue Windows-Proben halten Start-/Quell-/
Backupsperren an eigenen Testdateien und verweigern simulierte Quellupdates.
Neu ist die lesende [Installationsprüfung](docs/INSTALLATION_CHECK.md): vollständige
Datei-/Größeninventur gegen die lokal installierten Steam-Depots, EXE-Liste und
Abweichungsbericht. Die großen Archive bleiben dabei ungeöffnet.
Separat verfügbar: [vollständige Inhaltsprüfung](docs/CONTENT_AUDIT.md) nach dem
Spielen, mit Fortschritt, Abbruch, SHA-1-/SHA-256-Vergleich und Berichtsexport.
Exportierte Prüfberichte lassen sich jetzt mit einer geprüften Registry-Kopie
als [beobachteter Ausgangsstand](docs/BASELINE.md) dauerhaft im Projekt speichern.
Beim Wiederöffnen werden die Sicherung und der aktuelle Metadatenstand geprüft.
Neu ist die [Live-Anwendung mit Restore](docs/LIVE_APPLY.md): explizite Einrichtung,
Dateivorschau, vollständiger Quellvergleich unter Startschutz, Apply/Reapply und
Wiederherstellung. **Für deine Installation noch nicht eingerichtet oder ausgeführt.**
Der explizite Basiswechsel nach Updates archiviert eigene Dateien und Historien;
die Inventur weist sie separat aus. [Fremde Zusatzdateien](docs/FOREIGN_FILES.md)
können nach exakter Vorschau ausdrücklich zum unveränderten Beibehalten bestätigt
werden. Fremde Originaländerungen bleiben gesperrt. Die Entwicklung von Phase 4
ist abgeschlossen; die manuelle Spielabnahme steht noch aus. Unterstützt
sind Shopergänzungen bei 208 Händlern und Chancenänderungen in 12.306 Dropsets.
Manuelle Spieltests sind auf Nutzerwunsch bis zum Ende aller Entwicklungsphasen
zurückgestellt; der Entwicklungsstand und die offene Spielabnahme werden getrennt geführt.
[Bedienung, Abdeckung und offene Freigaben](docs/MODS.md).

Außerhalb der ausdrücklich gestarteten B0-Live-Abläufe wird die Installation nur
gelesen. Konkrete Weltfundorte,
vollständige Händlerquellen und Save-Auswertung bleiben offen. Der CLI-Core
bleibt separat nutzbar.

## Desktop starten

Für Freunde sind die fertigen [Release-Downloads](https://github.com/ArasHueseyin/crimson-workbench/releases)
vorgesehen. Beim Selbstbau liegt die Windows-App unter
`target/release/crimson-workbench.exe`. Im Projektordner starten:

```powershell
.\Start-Workbench.ps1
```

Die App benötigt keinen laufenden Entwicklungsserver. Installierte und portable
Starts verwenden einen eigenen Benutzer-Datenordner; mit `--project "C:\Pfad\zu\Daten"`
lässt er sich überschreiben. Ein Entwicklungscheckout wird weiterhin erkannt.
Windows verwendet die installierte WebView2-Laufzeit.
[Bedienung, Grenzen und Architektur](docs/DESKTOP.md).

Selbst bauen: Node 22.22+, Rust 1.95+, Windows SDK und C++-Buildwerkzeuge:

```powershell
cd app
npm ci
npm run desktop:build
```

`npm run tauri dev` startet die Entwicklungsumgebung. `npm run dev` allein
startet nur das Frontend: ohne Tauri werden ausdrücklich keine Spieldaten angezeigt.

Die CLI liest Spielarchive und schreibt JSON-Ausgaben, Overlaykandidaten,
Probekopien und SQLite-Caches nur unter
`.local/` oder `exports/` im Projekt. Spiel- und Save-Verzeichnisse sind als
Ausgabeziele gesperrt. Ausschließlich die neuen `live-setup`/`live-execute`-Befehle
verwenden den getrennten B0-Schreibpfad; Save-Dateien sind weiterhin ausgeschlossen.
CDUMM bleibt eine technische Referenz; es wird nicht benötigt oder aufgerufen.

## Start in PowerShell

Im Projektordner, mit Rust 1.95 oder neuer und installierten C++-Buildwerkzeugen:

```powershell
cargo build --workspace --locked
cargo run --locked -p cd-cli -- detect
cargo run --locked -p cd-cli -- fingerprint
cargo run --locked -p cd-cli -- search Stumpfpfeil
```

Die vorhandene lokale `.env` enthält die gefundenen Spiel-/Save-Pfade. Auf einem
anderen Rechner dienen `.env.example`, `CD_GAME_DIR` und `CD_SAVE_DIR` zur
Konfiguration. Ohne expliziten Spielpfad versucht die CLI Steam, Epic und Game Pass.
Deutsch ist die Standardsprache; `languages` zeigt die 15 hinterlegten Itemsprachen.
Die Suche legt ihren Cache unter `.local/index/items.sqlite` an.

**Umfang:** `iteminfo` wird strukturell vollständig mit typisierten Feldern und
Rohbereichen gelesen. Bei 13 weiteren Tabellen werden zunächst nur Index und
uninterpretierte Datensatzblöcke geprüft. Das sind noch keine Shop-, Rezept- oder
Skill-Editoren. Ein passendes Leseschema bestätigt weder Vanilla-Herkunft noch die
Freigabe für Änderungen am Spiel. Unbekannte Builds werden nicht semantisch gelesen.

[CLI-Anleitung und Core-API](docs/CORE.md) erklärt Einzelitems, Felddumps,
Roundtrips, Buildvergleiche und die Grenzen der Implementierung.

## Prüfungen und Dokumentation

```powershell
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --nocapture
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

Der aktuelle Ausführungsnachweis steht im [Fortschrittsbericht](docs/PROGRESS.md).
Windows-/Linux-CI ist konfiguriert; Linux wurde lokal mangels laufendem
Docker-Daemon nicht ausgeführt. Frontend- und native UI-Prüfungen sind in
[TESTING.md](docs/TESTING.md) beschrieben.

- [Spezifikation mit eigener Apply-Engine](SPEC.md)
- [Formate und lokale Buildbeobachtung](docs/FORMATS.md)
- [Machbarkeit aller vorgesehenen Funktionen](docs/FEASIBILITY.md)
- [Entwurf der späteren Apply-Engine](docs/APPLY_DESIGN.md)
- [Prüfplan und manuelle Spieltests](docs/TESTING.md)
- [Quellen und Lizenzen](CREDITS.md)

`.env`, `.local/`, `exports/`, `.research/` und lokal bereitgestellte `reference/mods/`-Inhalte
bleiben außerhalb von Git. Spielarchive, extrahierte Tabellen, Texturen und Saves
werden nicht mitgeliefert. Nach der Phase folgt die vorgesehene Durchsicht; die
nächste Phase beginnt nicht automatisch.
