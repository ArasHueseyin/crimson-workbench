# CLI und lesender Core

Die CLI liest die lokale Installation, prüft bekannte Dateihashes und macht Items
als JSON und über eine SQLite-FTS5-Suche zugänglich. Diese Anleitung beschreibt
die vorhandenen Befehle; konkrete Testresultate stehen in [PROGRESS.md](PROGRESS.md).
Die [Desktopoberfläche](DESKTOP.md) verwendet denselben Core. Modexport und
Apply/Restore sind noch nicht implementiert. Save-
Verzeichnisse werden erkannt, ihre Inhalte werden noch nicht ausgewertet.

## Bauen und konfigurieren

Die Beispiele werden in PowerShell **im Projektordner** ausgeführt. Benötigt
werden Rust 1.95 oder neuer und unter Windows C++-Buildwerkzeuge mit Windows SDK;
SQLite wird mitgebaut.

```powershell
cargo build --workspace --locked
$cli = ".\target\debug\cd-cli.exe"
& $cli --help
```

Alternativ lässt sich jeder Aufruf als `cargo run --locked -p cd-cli -- <Befehl>`
ausführen. Unter Linux liegt die gebaute CLI in `target/debug/cd-cli`.

Die lokale `.env` ist bereits eingerichtet. Für eine andere Installation kann
`.env.example` als Vorlage dienen:

```dotenv
CD_GAME_DIR=C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert
CD_SAVE_DIR=C:/Users/<Benutzer>/AppData/Local/Pearl Abyss/CD/save
```

Priorität bei der Spielauswahl: `--game`, Prozessumgebung, `.env`, automatische
Erkennung. `--project` wählt den Projektordner mit `.env`; Ausgaben und Caches sind
auf dessen Unterordner `.local/` und `exports/` begrenzt. Standard ist das aktuelle
Verzeichnis. Relative konfigurierte Pfade
beziehen sich auf diesen Projektordner. Bei mehreren automatisch gefundenen
Installationen ist eine Auswahl erforderlich. Ein ausdrücklich gesetzter, aber
ungültiger Spielpfad wird als Fehler gemeldet.

```powershell
& $cli detect
& $cli --game "D:\SteamLibrary\steamapps\common\Crimson Desert" detect
```

`detect` liefert Installationspfade, Plattform, verfügbare Build-ID und Save-
Verzeichnisse. Die Erkennung berücksichtigt Steam-Bibliotheken, Epic-Manifeste,
gängige Xbox-/Game-Pass-Verzeichnisse sowie Pearl-Abyss-Savepfade und Proton-
Bibliotheken. Unzugängliche Paketinstallationen benötigen einen lesbaren expliziten
Pfad; die CLI verändert keine Zugriffsrechte.

## Fingerprint und Roundtrip

```powershell
& $cli fingerprint
& $cli tables
& $cli roundtrip --all-fields
```

`fingerprint` prüft EXE-Version, erfasste Metadaten, verwendete Tabellen und die
gewählte Item-Lokalisierung gegen den hinterlegten Lesebuild. Aktuell ist das
Schema `steam-25381195-gamedata-2.3-v2` hinterlegt. Ein Update oder veränderte
Eingaben sperren die semantische Interpretation. Eine Diagnose bleibt möglich:

```powershell
& $cli fingerprint --metadata-only --output .local/build-diagnose.json
```

`--metadata-only` hasht EXE und erfasste Metadatendateien, ohne Tabellen semantisch
zu lesen. Dieser Modus bestätigt kein unterstütztes Leseschema. Auch ein vollständig
passender Fingerprint setzt `certified_vanilla` **nicht** auf wahr: bekannte Bytes
sind kein Nachweis einer sauberen Originalinstallation und keine Apply-Freigabe.

`roundtrip` vergleicht im Speicher Originalbytes mit der Serialisierung von
Tabellenbody/-header, Iteminfo, PALOC sowie Archivmetadaten. `--all-fields` prüft
zusätzlich die lückenlose Feldabdeckung aller Iteminfo-Datensätze. Das schreibt
keine Daten zurück ins Spiel. Ein Rohdaten-Roundtrip bestätigt den Erhalt der Bytes,
nicht automatisch die Bedeutung der enthaltenen Felder.

## Items und Rohfelder lesen

```powershell
& $cli item 2200
& $cli item 2200 --fields
& $cli dump iteminfo --item 2200 --fields --output .local/item2200.json
```

`item` liefert lokalisierte Angaben und den typisierten Itemdatensatz. `--fields`
ergänzt Feldbereiche und Rohinformationen. Unbestätigte Bedeutungen bleiben
unbenannt; numerische Kategorien werden nicht mit erfundenen Labels versehen.
Ohne `--output` wird JSON auf die Standardausgabe geschrieben.

`dump iteminfo` gibt alle Itemdatensätze aus. Ein vollständiger Felddump mit
`--fields` verlangt `--item`, damit die Ausgabe auf einen Datensatz begrenzt bleibt.

Es gibt **14 Tabellenindizes**: `iteminfo` und 13 weitere Tabellen:

```text
characterinfo       crafttoolgroupinfo  crafttoolinfo
dropsetinfo         fieldinfo           inventory
multichangeinfo     questinfo           regioninfo
skill               stageinfo           storeinfo
vehicleinfo
```

Bei `iteminfo` wird die vollständige Struktur typisiert gelesen/serialisiert;
unbekannte Bereiche bleiben erhalten. Die anderen Tabellen werden als geprüfte
Schlüssel-/Offsetindizes mit **uninterpretierten Datensatzbytes** behandelt:

```powershell
& $cli dump storeinfo
```

Dieser Aufruf zeigt Datensatzgrenzen und Hashes. `dump <Tabelle> --item <Schlüssel>`
liefert einen ausgewählten Rohdatensatz als Hexbytes. Die tatsächlichen Schlüssel
kommen aus dem jeweiligen Index. Rezepte, Shopbestand, Trustwerte und Skillkosten
werden dadurch noch nicht als editierbare Felder angeboten.

Seit Phase 3 interpretiert das zusätzliche Modul `crafting` Rezept-/Gruppen-
records und einfache Item-Dropsets. Die bestehenden `dump`-Ausgaben bleiben
unverändert. CLI-Nutzung und Grenzen: [CRAFTING.md](CRAFTING.md).

## Deutsch, weitere Sprachen und Suche

```powershell
& $cli languages
& $cli index
& $cli search Stumpfpfeil
& $cli search Stumpfpfeil --limit 20 --offset 0
& $cli --language eng item 2200
```

Standard ist `ger`; `de` ist ein Alias. Englisch akzeptiert `eng` oder `en`.
Die hinterlegten Itemsprachen sind:

```text
ara eng fre ger ita jpn kor pol por-br rus spa-es spa-mx tur zho-cn zho-tw
```

Geprüft wird jeweils die Item-PALOC-Datei der gewählten Sprache. Fehlt die
Übersetzung eines einzelnen Items, dienen vorhandener Standardtext bzw. interner
Schlüssel als Fallback. `name_resolved` unterscheidet eine Namensangabe vom reinen
Schlüsselfallback; es beweist nicht, dass der Text aus der gewählten PALOC stammt.
Das ist keine Lokalisierung sämtlicher Spieltexte oder der noch ausstehenden GUI.

FTS5 sucht in Namen, Beschreibungen und internen Schlüsseln. `--item-type` nimmt
einen numerischen Typ, `--category` den im Datensatz sichtbaren Kategoriecode:

```powershell
& $cli search --item-type 1 --limit 20
```

Der lokale Cache liegt standardmäßig in `.local/index/items.sqlite`. `index` und
`search` bauen ihn bei Bedarf auf; ein geänderter Fingerprint, Sprachwechsel oder
Schemawechsel löst einen Neuaufbau aus. `--cache .local/index/anderer-name.sqlite`
kann einen getrennten Cache wählen.

## Zwei Builds vergleichen

Zwei Manifestdateien müssen zuerst erzeugt werden, beispielsweise jeweils vor
und nach einem Spielupdate. Für vergleichbare Metadatenberichte wird in beiden
Aufrufen derselbe Modus verwendet:

```powershell
& $cli fingerprint --metadata-only --output .local/before.json
# Den folgenden Snapshot erst zum späteren Vergleichszeitpunkt erzeugen.
& $cli fingerprint --metadata-only --output .local/after.json
& $cli diff .local/before.json .local/after.json
```

`diff` vergleicht die gespeicherten Hashes und EXE-Versionen, ohne einen neueren
Build zu interpretieren. Ein vollständiger Fingerprint enthält zusätzlich
Tabellen-/Lokalisierungshashes; für solche Vergleiche müssen beide Builds ein
unterstütztes Leseschema haben. Ein leerer Diff bedeutet nur, dass die im Manifest
erfassten Werte übereinstimmen; es ist keine Prüfung sämtlicher Spielarchive.

## Lokale Schreibgrenze

`--output` erzeugt ausschließlich **neue Dateien unter `.local/` oder `exports/`**
innerhalb des Projekts. Andere Ziele werden auch dann abgelehnt, wenn sie im
Projekt liegen. Dieselbe Beschränkung gilt für SQLite-Caches. Beide Ausgabeordner
sind von Git ausgeschlossen. Bestehende JSON-Dateien werden nicht überschrieben;
für einen weiteren Snapshot ist ein neuer Name erforderlich. Verwende `--output` statt Shellumleitung, damit der programminterne
Pfadschutz auf das Ausgabeziel angewendet wird.

Alle erkannten und ausdrücklich konfigurierten Spiel-/Save-Wurzeln sind geschützt,
auch wenn ein konfiguriertes Verzeichnis noch nicht existiert. Vorhandene
Verzeichnisverknüpfungen werden aufgelöst. Bestehende Hardlinks sowie verknüpfte
SQLite-Dateien/-Nebendateien werden abgelehnt. Der eigene reguläre SQLite-Cache darf
wiederverwendet werden; diese Ausnahme gilt nicht für JSON-Dumps.

## Rust-Schnittstellen

| Einstieg | Zweck |
|---|---|
| `discover_project(project, game_override)` | Konfiguration und lesende Installationserkennung zusammenführen. |
| `Workspace::open(project, game_override, language)` | Geprüfte Spieldaten und lokale Ausgaberegeln gemeinsam öffnen. |
| `workspace.data()` | Unveränderliche `GameData`; `tables`, `item_detail`, `dump_table`, `roundtrip` und `fingerprint`. |
| `workspace.build_index(cache)` / `workspace.search(query, cache)` | Abgeleiteten lokalen Index aufbauen/abfragen. |
| `workspace.write_json(path, value)` | Geschützte neue JSON-Ausgabe; kein Überschreiben. |
| `fingerprint::Fingerprint::inspect(game)` / `fingerprint::diff(before, after)` | Diagnose ohne semantische Interpretation und Vergleich gespeicherter Fingerprints. |
| `PathPolicy::from_roots(roots, output_root)` | Alle Spiel-/Save-Wurzeln gegen lokale Ausgaben schützen. |
| `browser::BrowserSession` | Dauerhafte Desktop-Sitzung: geprüfter Snapshot, sprach-/buildbezogener Index, paginierte Suche, Rohfelder, echte Icons und geschützte Itemexporte. |

`GameData` bietet keine Schreiboperation auf die Installation. Indexinternas sind
privat; öffentliche Workspace-Methoden prüfen die Ausgabepfade vor dem Öffnen.
Die selektiv integrierte MIT-Formatbibliothek liegt unter `vendor/crimson-format`;
Herkunft und Grenzen sind in [CREDITS.md](../CREDITS.md) dokumentiert.

## Tests

```powershell
cargo test --workspace --locked
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

Synthetische Tests brauchen keine Spielinstallation. Der Live-Roundtrip-Test liest
`CD_GAME_DIR` aus Umgebung oder `.env`: ohne konfigurierten Pfad wird er ausdrücklich
übersprungen; ein konfigurierter, aber fehlender, veränderter oder nicht unterstützter
Build lässt ihn fehlschlagen. Live-Prüfungen lesen ausschließlich; sie erzeugen
keinen Suchcache und analysieren keine Save-Inhalte.

[CI](../.github/workflows/core.yml) ist für Windows und Linux eingerichtet. Linux
wurde lokal nicht ausgeführt, weil kein Docker-Daemon verfügbar war. Ein konfigurierter
Workflow ist noch kein erfolgreicher Linux-Testlauf. Ausgeführte Prüfungen und
verbleibende Arbeit stehen in [PROGRESS.md](PROGRESS.md); die aktuelle GUI ist in
[DESKTOP.md](DESKTOP.md) beschrieben.
