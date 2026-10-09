# Desktop-App – Itemdatenbank, Herstellung und Modvorschau

Die Anwendung verbindet Tauri 2, React/TypeScript/Vite, Zustand, TanStack Table
und TanStack Virtual mit dem lesenden Rust-Core. Sie lädt keine Spielinhalte aus
dem Netz. Spieldateien, Texturen, Testexporte und Screenshots bleiben lokal und
sind aus Git ausgeschlossen.

Die neue Seite **Herstellungsplan** und die Rezeptverknüpfungen sind in
[CRAFTING.md](CRAFTING.md) beschrieben. Die **Modwerkstatt** mit ihren bewussten
Freigabegrenzen steht in [MODS.md](MODS.md). Insgesamt 31 native Kommandos sind erlaubt;
`craft_info`, `craft_plan` und `craft_item` laden Rezepte bei Bedarf. Ein
Installations-/Sprachwechsel verwirft auch den manuellen Herstellungsplan und die
Modkonfiguration. `mod_info`, `mod_preview`, `mod_export` und `mod_rehearse`
interpretieren geprüfte Tabellen und schreiben gegebenenfalls nur Projektdateien.
Sechs neue Live-Kommandos bedienen Status, Einrichtung, Dateivorschau und einen
abbrechbaren Auftrag. Nur B0 darf kontrolliert Spielpfade schreiben. Es gibt
weiterhin kein allgemeines FS-/Shell-Plugin. [Live-Bedienung und Grenzen](LIVE_APPLY.md).

## Bedienung

1. `Start-Workbench.ps1` oder `target/release/crimson-workbench.exe` starten.
   Der Projektordner enthält die bereits eingerichtete `.env`.
2. Bei einer eindeutigen oder konfigurierten Installation lädt die Datenbank
   automatisch. Unter **Datenquellen** lassen sich gefundene Installationen
   auswählen oder ein Pfad eintragen. **Neu einlesen** prüft die Dateien erneut.
3. **Ctrl+K** fokussiert die Suche. FTS5 sucht ganze Begriffe in Namen,
   Beschreibungen und internen Schlüsseln. Beispielsweise `Stumpfpfeil` oder
   `Arrow`. Mehrere Begriffe werden mit UND verknüpft; keine FTS-/SQL-Operatoren.
4. Typ, Kategorie und Tier kombinieren. **Mehr Filter** bietet Stapelbarkeit
   und Stat-ID. Klick auf einen sortierbaren Spaltenkopf ändert die Reihenfolge
   über die gesamten Treffer. Die Liste lädt 200 Treffer pro Seite nach und
   rendert nur den sichtbaren Bereich. Eine Schaltfläche lädt ebenfalls nach.
5. Ein Item auswählen: Übersicht, alle Rohfelder und direkte Itemreferenzen.
   Enter/Leertaste öffnet eine fokussierte Zeile; Pfeiltasten wechseln die Zeile.
   Feldsuche findet Pfad, Typ, Wert oder Hexoffset; unbekannte Felder lassen sich
   gesondert anzeigen. Angaben sind nicht editierbar.
6. **Item als JSON speichern** erzeugt eine neue Datei unter `exports/` im
   Projekt. Der vollständige Pfad erscheint als Rückmeldung. Keine Zieldatei
   wird überschrieben; Spiel-/Save-Verzeichnisse bleiben gesperrt.
7. Die Sprache oben rechts wechseln. Dabei werden Sitzung und Auswahl ersetzt;
   Indexe verschiedener Sprachen/Builds sind voneinander getrennt.

Unbekannte/veränderte Builds zeigen eine klare Sperre mit Diagnosedetails.
Bei fehlender Installation lässt sich der Pfad korrigieren. Fehlende Suchtreffer
sind ein normaler Leerzustand. Ein Browser ohne Tauri zeigt eine Erklärung statt
erfundener Beispieldaten.

## Was A1 bereits abdeckt und was noch fehlt

- Alle 6.816 Items mit lokalisiertem Namen/Beschreibung und lesbaren Feldbereichen.
  Der Detailname nutzt bei fehlender Übersetzung den dokumentierten Fallback.
- Echte Itemicons über SHA-geprüftes `stringinfo` und die registrierte Gruppe
  `0012/ui/texture/icon`. Ein fehlendes oder nicht unterstütztes Icon zeigt einen
  neutralen Platzhalter mit Begründung, kein erfundenes Ersatzbild.
- Typ und Kategorie sind **Spiel-IDs**, keine verifizierten deutschen Kategorienamen.
  Tier ist die rohe Zahl. Statfilter beziehen sich auf die bestätigten typisierten
  Hashreferenzen in `enchant_data_list` und `sharpness_data.stat_data`; Namen und
  Gameplayeinheiten dieser Stats sind nicht bestätigt.
- Direkte Felder vom Typ `ItemKey` führen zu existierenden Items. Unaufgelöste
  IDs bleiben sichtbar, aber ohne falsche Verknüpfung. Bloße u32-Zahlen werden
  niemals zu Itemlinks umgedeutet.
- **Offen aus A1:** Händler → Stadt/Region → Preis/Bestand, DropSet → Quelle und
  vollständige Rezeptbeziehungen. Die vorhandene MIT-Storequelle liest nur
  Schlüssel/Namen, lässt den variablen Body unangetastet und bezeichnet ihre
  historische feste Stockgröße selbst als falsch. Deshalb keine behaupteten
  Quellen aus Byteheuristiken. Diese Lücke bleibt ausdrücklich im Fortschritt;
  Phase 2 liefert nicht die vollständige A1-Quellenabdeckung.
- Keine Mod-/Profil-/Save-/Kartenbedienung. Die späteren Phasen sind nicht gestartet.

## Technische Grenzen

Das aktuelle Leseschema ist `steam-25381195-gamedata-2.3-v2`. Die Version wurde
wegen zusätzlicher typisierter Referenzen/Statfelder erhöht und invalidiert alte
Indexdetails. Die Archiv-/Itembytes werden nicht verändert. Der zusätzliche
Icon-Schemadatensatz steht in
`crates/cd-core/schemas/steam-25381195.icons.json` und enthält ausschließlich Hashes.

`stringinfo` hat 31.812 Einträge: u32-Key, vier unbekannte Bytes als u32, ein
unbekanntes Byte, u32-Länge und originale Stringbytes. Der Header hat u16-Count
und u32-Key/Offset. Body und Header werden rekonstruiert und vollständig bytegleich
verglichen. Auch unbekannte Flagwerte und nicht-UTF-8-Bytes bleiben erhalten.
Der beobachtete Item-2200-String `ItemIcon_Prefab_cd_phm_04_arw_0020` erhält für
die Zuordnung die Archivendung `.dds`. Die reale DXT5-Textur wurde als PNG dekodiert.
DDS sind auf 2 MiB, 512×512 Pixel, eine Ebene und höchstens zehn Mips begrenzt;
unterstützt werden die DDS-Varianten des eingebundenen `image`-Decoders.

`BrowserSession` hält den geprüften Snapshot und einen FTS-Index offen. Suchfilter
und Sortierung laufen vor der Seitenauswahl. u64/i64-Werte außerhalb des sicheren
JavaScript-Bereichs werden als Dezimalstring übertragen. Raw-Bytes bleiben zusätzlich
sichtbar. Icons haben begrenzte Speicher-Caches; es wird kein Texturatlas exportiert.

Native Kommandos laufen auf Tauri-Workerthreads. Sitzungs-IDs und Anfragezähler
verhindern, dass eine alte Suche oder Sprachänderung neuere Ergebnisse ersetzt.
Nach einem Spielupdate bleibt die bereits geladene Sitzung eine Momentaufnahme;
**Neu einlesen** bzw. App-Neustart prüft den neuen Build. Kein Dateiwatcher in Phase 2.

Die lokale Hauptansicht erhält ausschließlich neun eigene Kommandos:
`bootstrap`, `open_catalog`, `search_items`, `item_detail`, `item_icon`, `export_item`,
`craft_info`, `craft_plan`, `craft_item`.
Keine Shell-, FS-, HTTP-, Apply- oder Save-Plugins. CSP erlaubt lokale Appressourcen,
IPC und lokale PNG-Daten; Spielstrings werden als Text gerendert.
Tauri/WebView2 verwaltet separat seine eigenen Webview-Laufzeitdaten unter dem
Windows-Appdatenverzeichnis; Indexe und JSON bleiben im Projekt.

## Bauen und prüfen

Im Projektroot:

```powershell
npm ci --prefix app
npm run build --prefix app
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --nocapture
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
```

Im Ordner `app`: `npm run desktop:build` erzeugt die eigenständig startbare
Release-EXE ohne Installer. Der Build enthält die Oberfläche; Vite muss zum
Benutzen nicht laufen. Für einen schnellen eingebetteten Debug-Build:
`cargo build -p app --features custom-protocol --locked` nach dem Frontendbuild.

Ein unsichtbarer nativer UI-Test ist mit `--background-test` möglich. Nur bei
diesem ausdrücklich gesetzten Argument bleibt das Fenster verborgen; reguläre
Starts zeigen die Oberfläche. `scripts/native-smoke.mjs` verbindet sich nur mit
einer eigens gestarteten Testinstanz auf Port 9225. Der Port wird ausschließlich
über deren Prozessumgebung aktiviert, nicht im Produkt oder systemweit.

Windows-Junctions: Der Vite-Launcher normalisiert sein Arbeitsverzeichnis auf
den physischen Pfad, damit auf ein anderes Laufwerk umgeleitete Documents-Ordner
keine falschen absoluten Chunknamen erzeugen. `.npmrc` fixiert den verwendeten
Peer-Resolvermodus; Versionen und Integritäten liegen in `package-lock.json`.

Linux benötigt die Tauri-Systembibliotheken aus der CI-Konfiguration. Linux-CI
ist eingerichtet, hier aber nicht ausgeführt. Es wird kein plattformübergreifender
Testerfolg allein aus dem Windows-Build abgeleitet.
