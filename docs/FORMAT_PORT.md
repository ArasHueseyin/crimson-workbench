# Native Formatportierung für Phase 1

Ergänzung Phase 2: `ItemRecord.stat_keys` sammelt tatsächliche Statusreferenzen
aus `enchant_data_list` und `sharpness_data.stat_data`. Key-Wrapper behalten in
getrackten Feldern ihren Typnamen (z. B. `ItemKey`) statt nur `u32`; Werte und
Bytebereiche bleiben unverändert. So entstehen Links ausschließlich aus
typisierten Referenzen. Der zusätzliche Stringinfo-Parser im Core ist eine eigene
Implementierung nach der MIT-Formatdokumentation aus dem gleichen Ursprungscommit;
31.812 reale Strings und beide Dateien bestehen ihren Roundtrip. Details unter
[DESKTOP.md](DESKTOP.md).

`vendor/crimson-format` ist eine interne Rust-Crate ohne Python, ausgelieferte C-ABI,
GUI oder Apply-Funktion. Sie stellt ausschließlich lesende Dateizugriffe und
Serialisierung in Speicher bereit. Der aufrufende `cd-core` muss vor Benutzung
die bekannten Build-/Tabellenhashes prüfen und darf Parsererfolg nicht mit einem
Vanilla-Nachweis gleichsetzen.

## Herkunft und Entscheidung

Format- und Itembasis: [bbfox0703/crimson-rs, Commit
b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0),
Commitzeit 2026-09-18 16:51:44 +08:00. Copyright 2026 Tommy Tran, MIT;
der vollständige Lizenztext steht in
[vendor/crimson-format/LICENSE](../vendor/crimson-format/LICENSE).

Der Referenzbuild und der aktuelle 2.03-Itemparser funktionieren. Die unveränderte
Git-Dependency besitzt jedoch keine `rlib`-Ausgabe und keine öffentliche Rust-
Core-API. Deshalb wurde die in Phase 0 dokumentierte begrenzte MIT-Portierung
gewählt, keine neue externe Fork-/Veröffentlichungsabhängigkeit geschaffen.
Es wurde kein fremder Manager eingebunden und kein Spielinhalt übernommen.

Übernommen/adaptiert wurden ausschließlich:

- `src/binary/{mod,primitives,arrays,types}.rs`: primitive Typen, strukturierte
  BinaryRead/BinaryWrite und vollständige Bytefeldbereiche.
- `src/item_info/{mod,keys,item,structs}.rs`: typisierte 2.03-Itemstruktur.
- `src/crypto/checksum.rs`, `src/crypto/chacha20.rs`: Jenkins und PAZ-ChaCha20.
- Der lesende Partial-Kompressionsabschnitt aus `src/binary/paz.rs`:
  Header-LZ4 und DDS-Mip-Varianten; seine upstream Formatquellenhinweise bleiben
  erhalten. Keine PAZ-Builder-/Filesystem-Schreibfunktionen wurden übernommen.
- Ergänzung vom 03.10.2026: `src/save/` derselben MIT-Revision für Header,
  ChaCha/HMAC, Kompression, TOC und strukturierte Save-Objekte einschließlich
  Serialisierung in Speicher. Zusätzliche Lizenzkopie:
  [LICENSE-save](../vendor/crimson-format/LICENSE-save). HMAC-Fehler werden vor
  der Dekompression abgewiesen, Datei- und Nutzdatenlängen sind auf 128 MiB
  begrenzt; überzählige Dateibytes werden abgewiesen. Der aufrufende Core muss
  unbekannte Bereiche und fehlende exakte Roundtrips vor Schreiboperationen
  ablehnen. Die permanente Stallregistrierung ist eine getrennte, vom Nutzer
  beauftragte Erweiterung; der geplante allgemeine Save-Tracker bleibt lesend.

`archive.rs`, `items.rs`, `localization.rs` sind eigene begrenzte Fassaden und
Containerleser auf Basis der in [ARCHIVES.md](research/ARCHIVES.md) belegten
Formate. PAMT/PAPGT werden strukturell gelesen und mit erhaltenen Namens-/Trie-
Puffern neu serialisiert; `serialize_registry` und `serialize_group` sind keine
Rückgabe des eingelesenen Bytevektors. Die SHA-256-Build-/Localehashes kommen aus
der Workbench-/Verifier-Schicht, nicht aus den 32-Bit-Spielchecksummen.

## Öffentliche Schnittstellen

Alle fehlschlagenden Methoden liefern `std::io::Result`.

```rust
Archive::open(root: impl AsRef<Path>) -> Result<Archive>
Archive::groups() -> &[ArchiveGroup]
Archive::list_group(group: &str) -> Result<Vec<ArchiveEntry>>
Archive::extract(entry: &ArchiveEntry) -> Result<Vec<u8>>
Archive::serialize_registry() -> Result<Vec<u8>>
Archive::serialize_group(group: &str) -> Result<Vec<u8>>

ItemTable::parse(body: &[u8], header: &[u8]) -> Result<ItemTable<'_>>
ItemTable::entries() -> &[ItemRecord]
ItemTable::fields(key: u32) -> Result<Vec<RawField>>
ItemTable::serialize_body() -> Result<Vec<u8>>
ItemTable::serialize_header() -> Result<Vec<u8>>

Paloc::parse(bytes: &[u8]) -> Result<Paloc>
Paloc::entries() -> &[PalocEntry]
Paloc::decoded_payload() -> &[u8]
Paloc::original_bytes() -> &[u8]
Paloc::serialize_payload() -> Result<Vec<u8>>
Paloc::serialize() -> Result<Vec<u8>>
```

`ItemRecord` enthält Schlüssel, internen Namen, Typ/Tier/Kategorie, Stackzahl,
Name/Beschreibungsreferenzen, erste Iconreferenz, zehn Inventory-Keys und
Bodyoffset/-länge. `LocalizableText.index` ist der direkte numerische PALOC-
String-Key. Lokal stimmen beispielsweise Item2200s Namensindex
`9448928051312 = (2200 << 32) | 0x70` und Beschreibungsindex
`9448928051313 = (2200 << 32) | 0x71`. Beschreibungen dürfen nicht mit dem
Namensnamespace aufgefüllt werden.

Rohfelder werden je angefordertem Item decodiert, damit der normale Index nicht
über eine Million Feldobjekte im Speicher hält. Jedes `RawField` trägt Pfad,
absolute Bodyoffsets, Typbezeichnung, tatsächliche Hexbytes und dekodierten Wert.
Große u64/i64-Werte werden im Rohfeldwert als Dezimalstring ausgegeben, um
JavaScript-Präzisionsverlust zu verhindern. Nichtendliche f32 werden als Text
angezeigt; die exakten Bits bleiben in `raw_hex`. `interpretation="unknown"`
kennzeichnet unbekannte Felder und die bekannte Upstream-Vermutung zum Quickslot-
Flag. `upstream_named` bedeutet ausdrücklich keine bestätigte Gameplaysemantik.

Die optionale `RawTable::parse(body, header, key_width, count_width)`-Fassade prüft
nur explizit vorgegebene Indexbreiten, Schlüssel/Offsets und erhaltene opaque
Records. Ihr Byteerhalt ist **kein** Beleg für ein verstandenes Body-Schema und
keine Editorfreigabe. cd-core besitzt zusätzlich seine eigene buildversionierte
Indexbeschreibung für die anderen erfassten Tabellen.

## Härtungen gegenüber der Vorlage

- Geprüfte Addition vor Slicezugriffen; Count-, Offset-, Header- und tatsächlich
  vorhandene PAZ-Dateilängen werden validiert.
- Ein File/entpackter Payload ist auf 128 MiB begrenzt, PAMT/PAPGT auf 16 MiB,
  ein Itemrecord auf 4 MiB; generische Arrayreservierungen auf 16 MiB pro Array.
  Aufgelöste Archivpfade haben zusammen maximal 16 MiB, ein Pfad maximal
  4096 Bytes/256 Trie-Schritte. PALOC nimmt maximal 250.000 Einträge an.
- Tries werden iterativ mit Zyklenerkennung gelesen. Verzeichnisdurchquerung,
  absolute virtuelle Pfade, NUL/Steuerzeichen und Windows-Laufwerkssyntax werden
  abgelehnt; kanonisierte reale Lesepfade müssen unter der Game-Root liegen.
- `extract` vertraut nicht auf vom Aufrufer manipulierte `ArchiveEntry`-Offsets:
  Metadaten werden erneut aus dem gegen PAPGT geprüften PAMT gelesen/verglichen.
- Codecs 0/1/2/3 werden explizit behandelt; QuickLZ und unbekannte Codecs sowie
  ICE/AES werden bei Extraktion abgewiesen. Enumeration darf sie diagnostisch
  anzeigen. Zlib-Ausgabe ist auf erwartete Länge plus ein Prüfbyte begrenzt.
- ChaCha20 verwendet fallible Seek-/Keystream-Funktionen. DDS-Dimensionen,
  Mipanzahl und Größenmultiplikationen sind begrenzt/geprüft.
- Upstreams allokierte/geleakte Ersatzstrings wurden durch `Cow<str>` ersetzt.
  Nicht-UTF-8-Bytes bleiben für den Writer erhalten. Optional-Tags werden original
  aufbewahrt, statt jeden von Null verschiedenen Tag zu 1 zu normalisieren.
- Jeder Itemrecord muss den im Header verzeichneten Key und Bereich exakt
  erfüllen und bereits beim Laden einen echten typisierten Writer-Roundtrip
  bestehen. Keine Ankersuche, keine verworfenen Records, keine Restbytes.
- Der Tracker der Itemfelder muss jedes Byte genau einmal abdecken. Ein
  Lücken-/Überlappungsfehler wird statt eines unvollständigen Exports gemeldet.
- PALOC prüft Magic, Headerpadding, Größen, LZ4 und den gesamten Listenbereich.
  Sein Payload wird tatsächlich neu serialisiert. Für unveränderte Container
  wird der ursprüngliche LZ4-Block aufbewahrt; identische Neukompression wird
  nicht behauptet. In Phase 1 existiert keine PALOC-Mutationsschnittstelle.

Auf Dateiebene wird nur `File::open`, `metadata`, `seek`, `read` und
`canonicalize` verwendet. Produktionscode enthält weder `File::create` noch
Dateischreiben. Synthetische Tests erzeugen ausschließlich temporäre Testarchive.
Kein Save-Reader/-Writer wurde in diese Crate importiert.

## Verifikation

`cargo clippy -p crimson-format --all-targets -- -D warnings` ist ohne Warnungen bestanden. 15 eigene synthetische Tests bestanden: verschlüsseltes LZ4-Archiv durch
PAPGT/PAMT/PAZ bis zum Klartext, echter Metadaten-Roundtrip, manipulierte Entry-
Offsets, abgeschnittene echte Chunklänge, falsche Checksummen, Trie-Zyklen,
Pfad-/Codecfehler, Headergrenzen, unlesbare Items, PALOC-LZ4 und Raw-UTF-8-Erhalt,
Optionaltags, riesige Counts und DDS-Dimensionen. Keine übernommenen Live-Tests,
die bei fehlenden Fixtures still als Erfolg zählen.

Explizite native Realdatei-Verifikation am 19.09.2026, ausschließlich lesend:

- 6.816 Items; 6.465.724 Bodybytes und 54.530 Headerbytes byteidentisch durch
  Parser/Writer.
- Alle **1.763.837 Feldranges** sämtlicher Items vollständig und ohne Überlappung.
- `meta/0.papgt` und `0008/0.pamt` durch strukturierte Writer byteidentisch.
- Deutsches Item-PALOC: 13.575 Einträge; Payload und erhaltener Originalcontainer
  byteidentisch.
- Alle 15 registrierten Item-Sprachnamespaces erfolgreich mit der nativen
  Archiv-/PALOC-Fassade gelesen, geprüft und als reine Hash-Metadaten in
  `crates/cd-core/schemas/steam-25381195.locales.json` erfasst.

`cargo run -p crimson-format --example verify_formats` verlangt `CD_GAME_DIR`
explizit und prüft die aktuellen Dateien; fehlt die Variable, schlägt der
explizite Verifier fehl. `fingerprint_locales` gibt nur Metadaten auf stdout aus.
Die eigentlichen optionalen Realdatei-Tests und `.env`-Ladung werden von cd-core
koordiniert. Kein Beispiel schreibt in die Installation oder öffnet Saves.

Nicht nachgewiesen: weitere Itemschemata/Builds, alle Partialtexturvarianten,
QuickLZ/AES/ICE, komplette PAZ-Chunkchecksummen über alle großen Archive, Apply
oder Savefortschritt. Linux-Quellcode ist plattformneutral; ein tatsächlicher
Linux-Lauf ist durch Windows-Builds nicht bewiesen.

Phase 5 ergänzt `item_mods.rs` als eigene begrenzte Fassade über den vorhandenen
Item-Reader/Writer (Stats/Buffs, seit v0.5.1 neue Enchant-Zeilen aus Originalstufen,
Stacks und Reparaturlisten). Kopien erhalten vollständige Zeileninhalte und
erneuern die nullwertigen Trenner. Die Fassade hat keine Dateisystem-Schreib-API. `cd-core`
prüft Status-/Buffreferenzen, Build, experimentelle Stackfreigabe und B0.
Weitere Reader sind eigene Implementierungen; [Nachweise](research/ADVANCED_TABLES.md).
