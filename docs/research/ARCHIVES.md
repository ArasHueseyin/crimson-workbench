# Archiv-, Parser- und Dependency-Recherche

Stand: 19. September 2026. Phase 0; kein Anwendungscode, kein Apply, keine Änderung an Spiel oder Saves. Die während des Spielens ausgeführten Proben lesen kleine Metadaten und ausgewählte PAZ-Bereiche. Extrahierte Inhalte liegen ausschließlich unter `.local/archive-probe/extracted/`; öffentliche Referenzquellen, Builds und Forschungshilfen unter `.research/`. Beide Verzeichnisse müssen ignoriert bleiben.

## Gepinnte Quellen und Lizenz

| Quelle | Geprüfter Commit | Commit-Zeit | Lizenz |
|---|---|---|---|
| [bbfox0703/crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0) | `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0` | 2026-09-18 16:51:44 +08:00 | [MIT, Copyright 2026 Tommy Tran](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/LICENSE) |
| [lazorr410/crimson-desert-unpacker](https://github.com/lazorr410/crimson-desert-unpacker/tree/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e) | `f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e` | 2026-03-23 15:30:39 -04:00 | [MIT, Copyright 2026 lazorr410](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/LICENSE) |

Beide LICENSE-Dateien wurden im gepinnten Checkout gelesen. Bei Übernahme den vollständigen Copyright- und MIT-Lizenztext erhalten und die Quellen in `CREDITS.md` führen. Die Softwarelizenz ist kein Nachweis für die Weitergabe von Spieldaten; diese werden nicht Teil des Projektrepos.

## Dependency-Entscheidung

**crimson-rs ist als aktuelle Formatbasis brauchbar, aber keine unmittelbar importierbare Rust-Crate für cd-core.** [Cargo.toml](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/Cargo.toml) erzeugt ausschließlich `cdylib` und `staticlib`, kein `rlib`. [src/lib.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/lib.rs) hält `binary`, `item_info` und `save` privat. Ein einfacher gepinnter Cargo-Git-Eintrag liefert daher nicht die benötigte öffentliche Rust-API.

Der Build unterstützt drei Konfigurationen: leere Default-Features; `c_abi`; optional `python` mit PyO3 0.27.2/abi3 für Python ab 3.12. Direkte Dependencies sind `chacha20` 0.10, `lz4_flex` 0.11, `flate2` 1, `hmac` 0.12 und `sha2` 0.10; `tempfile` 3 wird für Tests verwendet. Python ist für eine native C-ABI-DLL nicht erforderlich. Eine vollständige transitive Lizenzinventur wurde noch nicht abgeschlossen: `cargo metadata --offline --locked` wollte einen nicht im Cache vorhandenen plattformspezifischen Lockfile-Eintrag (`bitflags` 2.11.0) laden. Das ist kein Buildfehler der geprüften Windows-Konfiguration.

Empfehlung für Phase 1: minimale, nachvollziehbare Anpassung eines gepinnten Forks mit `rlib` und begrenzter öffentlicher Fassade, sofern eine eigene Git-Dependency bereitgestellt werden soll. Alternativ nur benötigte MIT-Module mit Quellenbelegen portieren. C-ABI ist eine tatsächlich funktionierende Integrationsoption, zieht für einen Rust-Backendkern aber zusätzliche FFI-/Artefaktverwaltung nach sich. Den bestehenden umfangreichen Save-Schreib-ABI nicht ungefiltert als Backend-API exponieren.

## Lokal ausgeführte Prüfungen

Windows; `cargo 1.95.0`, `rustc 1.95.0`; Python 3.12. Referenzcheckout unverändert. Buildziel `.research/target-crimson-rs/`.

- `cargo test --locked --lib --features c_abi --no-run`: erfolgreich.
- `cargo test --locked --lib --features c_abi -- --test-threads=2`: **354 passed, 0 failed, 46 ignored**. `CRIMSON_GAME_ROOT` und `LOCALAPPDATA` waren absichtlich auf nicht vorhandene Forschungsordner gesetzt. Viele Live-Tests überspringen intern bei fehlenden Daten und zählen trotzdem als „passed“; dieses Ergebnis allein bestätigt keine Spielkompatibilität.
- `cargo build --locked --features c_abi`: erfolgreich; 51 Warnungen, keine Fehler.
- `cargo build --locked --features c_abi,python`: erfolgreich. Die Python-Fassade wurde aus der erzeugten DLL im Forschungsordner geladen, ohne Installation ins System-Python.
- Die unten genannten Realdatei-Proben wurden anschließend separat mit der unveränderten Referenz-API ausgeführt. Keine Save-Datei wurde in dieser Recherche gelesen oder geschrieben. Linux wurde nicht gebaut/geprüft.

Logs: `.research/crimson-rs-build.log`, `crimson-rs-tests.log`, `crimson-rs-dll-build.log`, `crimson-rs-python-build.log`. Forschungshelfer: `.research/probe_archives.py`, `extract_selected.py`, `roundtrip_probe.py`, `locale_probe.py`, `registry_diff.py`. Dies sind reproduzierbare Phase-0-Proben, kein Anwendungscode.

## Auf dieser Installation bestätigte Ergebnisse

Installationsbasis: `C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert`. Die durch die Hauptrecherche beobachteten `0.paver`-Bytes `02 00 03 00 00 00 38 51 04 03` bedeuten laut [paver.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paver.rs) `major=2, minor=3, patch=0, build=0x03045138`. Das passt zum Parserziel 2.03. Steam-Build-ID und EXE-Version sind getrennte Fingerprintbestandteile und nicht mit diesem Versionsstempel gleichzusetzen.

| Probe | Echtes lokales Ergebnis |
|---|---|
| `iteminfo.staticinfobody`, aus Gruppe 0008 | 6.816 Items; 6.465.724 Bytes vollständig geparst und serialisiert; **byte-identisch** |
| Item-SHA256, Eingabe und Ausgabe | `87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce` |
| Item-`inventory_info_list` | Länge 10 für alle 6.816 Items |
| `meta/0.papgt` | 679 Bytes; Referenz parse → serialize **byte-identisch** |
| PAPGT-SHA256 | `c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea` |
| `0008/0.pamt` | 1.109.183 Bytes; Referenz parse → serialize **byte-identisch** |
| PAMT-SHA256 | `3723c5d61f6a0e8a9682b549112bbf072c5bb10e1eb8f87155668f65a79cfc90` |
| Deutsches `item.paloc` | Gruppe 0027, `gamedata/stringtable/binary__/ger`; 649.912 Bytes, 13.575 Einträge |
| PALOC-SHA256, extrahierter Container | `d8fd26ad4a845e4c6434f4c393fbe64866dc1f63f2a07df3be7d61fc9d734e5c` |
| PALOC-Kompression | `paloc`-Magic; Header 0x200; LZ4-Block 649.400 Bytes → 1.807.930 Bytes; **Payload parse → serialize byte-identisch** |

28 Dateien wurden selektiv extrahiert und gehasht: Body und Header von `characterinfo`, `crafttoolgroupinfo`, `crafttoolinfo`, `dropsetinfo`, `fieldinfo`, `iteminfo`, `questinfo`, `regioninfo`, `skill`, `stageinfo`, `storeinfo`, `vehicleinfo`, `inventory`, `multichangeinfo` (14 Paare). Die Datei `.local/archive-probe/table-fingerprints.json` enthält jeden Fingerprint. Die Probe extrahiert ausschließlich ausgewählte Dateibereiche, keine kompletten PAZ-Archive. Tabellenhashes bestätigen Identität, nicht die korrekte Interpretation sämtlicher Felder und nicht die Vanilla-Herkunft.

Messresultate: `.local/archive-probe/item-roundtrip.json`, `container-roundtrip.json`, `localization-fingerprint.json`, `table-fingerprints.json`, `metadata.json`, `registry-observation.json`. Keine extrahierten Inhalte in öffentliche Dokumentation kopieren.

## PAZ/PAMT: tatsächlich implementierte Formate

[Archivdokumentation](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/docs/archive-format.md) und [PAMT-Quellcode](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/pamt.rs): `meta/0.papgt` registriert Gruppen, jede Gruppe besitzt `0.pamt` und numerische PAZ-Chunks. PAZ besitzt keinen eigenen Header; PAMT enthält Verzeichnis-/Datei-Tries, Dateigrößen, Offsets, Chunk-IDs, Flags und Checksummen.

PAMT-Header gemäß ausführbarem Quellcode: `checksum:u32`, `count:u16`, `unknown0:u16`, anschließend `unknown0:u8` plus `encrypt_info:[u8;3]`; zusammen 12 Bytes. Der vereinfachte Header-Offsettext in `archive-format.md` ist damit nicht durchgehend konsistent. Roh-Dateieintrag: vier `u32` (Nameoffset, Chunkoffset, komprimierte Größe, Originalgröße), `chunk_id:u16`, `flags:u8`, `unknown:u8` = 20 Bytes. Ein neuer Parser muss Rohwerte erhalten und Grenzen validieren.

Unteres Flags-Nibble: 0 keine Kompression, 1 Partial, 2 LZ4, 3 Zlib, 4 QuickLZ. Oberes Nibble: 0 keine Verschlüsselung, 1 ICE, 2 AES, 3 ChaCha20. [paz.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paz.rs) implementiert None/LZ4/Zlib sowie mehrere Partial-Lesevarianten; QuickLZ ist nicht implementiert. Schreiben unterstützt None/LZ4/Zlib. Tatsächlich implementierte Kryptografie ist None/ChaCha20; AES/ICE liefern bei Extraktion „unsupported“. Unbekannte Nibbles werden im Referenzparser teilweise als None abgebildet: cd-core sollte stattdessen unbekannte Werte ablehnen und Rohflags anzeigen.

[ChaCha20-Code](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/chacha20.rs): Schlüssel/Nonce deterministisch aus Basename-Checksum und drei `encrypt_info`-Bytes der jeweiligen Gruppe. Kein Runtime-Hook/Schlüsselabgriff nötig. Verzeichnisanteil wird entfernt; der Code selbst macht keine Lowercase-Normalisierung. Die gelesenen Archivnamen sind daher unverändert zu verwenden. Entschlüsselung erfolgt vor Dekompression.

[PAMT-Parser](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/pamt.rs) prüft die Checksum über Bytes nach dem 12-Byte-Header und optional die aus PAPGT übergebene Gruppenchecksum. Der verwendete [Jenkins-Hash](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/checksum.rs) initialisiert mit Dateilänge plus `0xDEBA1DCD`. Dass die Probe die Checksummen versteht, ist durch echte PAMT/PAPGT-Parse-Ergebnisse belegt; vollständige PAZ-Chunkhashes wurden während des Spielens nicht berechnet.

## September-Kompatibilität und Tabellenlimits

Der aktuelle [Layoutresolver](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/gamedata_layout.rs) kennt ab 2.01 `gamedata/binarystaticinfo__/bin/<table>.staticinfobody` und `.staticinfoheader`; Fallback ist `gamedata/binary__/client/bin/*.pabgb`/`.pabgh`. Das neue Layout ist auf dieser Installation tatsächlich vorhanden. Der Rust-Resolver ist `cfg(test)`/crate-private und keine direkt nutzbare öffentliche Discovery-API.

Nicht „September“ als einzelne Schema-Version behandeln: 2.01 benennt Container um, 2.03 erweitert unter anderem die Item-Inventarliste von 9 auf 10 Einträge. Der gepinnte [Versionsgate](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paver.rs) erlaubt aktuell nur `(2,3)`. Die genaue historische Umstellung am 4. September wurde hier nicht anhand alter lokaler Binärdateien verglichen. Die aktuelle 2.03-Itemkompatibilität wurde dagegen real verifiziert.

Es gibt kein universelles PABGB-Body-Schema. Indexformen unterscheiden sich: `u16 count + (u32 key,u32 offset)` etwa bei skill; `u16 count + (u16 key,u32 offset)` bei Stores; weitere Tabellen besitzen breitere Schlüssel. Stage/Quest können über PABGH-basierte C-ABI-Loader mit vollständigen Schlüsselmengen geladen werden; Namens-Ankerscans sind potenziell unvollständig. Header mit extrahieren, Raw-Abschnitte behalten.

**crimson-rs allein erfüllt B1 nicht.** [store_info/mod.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/store_info/mod.rs) extrahiert nur Key und internen Namen; Kaufpreis/Verkaufspreis/Limits und variable Stock-Records werden nicht vollständig decodiert. Die Behauptung „Stock-Record +8 Bytes“ aus der Spezifikation wurde mit dieser Referenz nicht unabhängig bewiesen. Kein pauschaler fester Stock-Stride und keine Editor-Freigabe aus einem bloßen Store-Namenslookup ableiten.

## Lokalisierung und Save-Reader

[paloc.rs](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paloc.rs) nimmt nackte Entry-Listen und seit 2.03 LZ4-Container an. Ein Entry enthält `u64 unk_id`, längenpräfixierten String-Key und String-Wert; am Ende steht ein `u32 count`. Der Name `CString` im Referenzcode bedeutet hier nicht zwingend NUL-terminierte C-Saite. Namespaces stehen teils in numerischen String-Keys `(Jenkins(internal_name) << 32) | namespace`, etwa Itemnamen `0x70`, Questüberschriften `0x100`, Mission-/Stage-Titel `0x101`.

Deutsch wurde lokal in Gruppe 0027 mit 39 Namespace-Dateien gefunden. `item.paloc` hat PAZ-Flags `0x30`: ChaCha20, keine PAZ-Kompression; die LZ4-Kompression liegt innerhalb der PALOC-Datei. Container-Roundtrip muss ausdrücklich von Payload-Roundtrip unterschieden werden: erneutes LZ4-Komprimieren muss nicht dieselben komprimierten Bytes ergeben. Für eine harte unverändert-byte-identische Containeranforderung die ursprünglichen komprimierten Bytes behalten, solange keine Änderung vorliegt. Hier ist die dekomprimierte Entry-Liste nach echter Serialisierung identisch; keine Gleichheit neu komprimierter Container behauptet.

[Save-Modul](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/mod.rs): 128-Byte-Header mit `SAVE`, Version/Flags, Größen, 16-Byte-Nonce und HMAC-SHA256. ChaCha20-Entschlüsselung, HMAC über komprimierte Klartextbytes, LZ4-Blockdekompression, Body-Schema/TOC/Objektdecoder. [Save::parse](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/io.rs) liefert den Body selbst bei falschem HMAC und setzt nur `hmac_ok=false`; cd-core muss diesen Zustand ausdrücklich behandeln. Vollständiger Save-Fortschritt/100%-Tracker ist dadurch noch nicht bewiesen. Die vorhandene Save-Mutations-API gehört nicht zum autorisierten Produktumfang: Saves bleiben read-only. Keine aktuellen Nutzer-Saves in dieser Probe geöffnet.

## B0: eigener Overlay-Writer

[PackGroupBuilder](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/paz.rs) implementiert die benötigten Grundoperationen: Klartext komprimieren/verschlüsseln, PAZ-Chunk schreiben, Offsets/Flags/Größen erfassen, Trie-Indizes aufbauen, Chunkchecksummen und PAMT-Checksum erzeugen. [PAPGT add_entry/to_bytes](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/binary/papgt.rs) registriert/upsertet eine Gruppe vorne, verschiebt Namensoffsets und berechnet die PAPGT-Checksum neu. Die behauptete Engine-Suchreihenfolge „erster Treffer gewinnt“ ist upstream dokumentiert, hier aber nicht durch einen lokalen Apply-/Startversuch bestätigt. **Overlay-Grundformat grün; lokale Engineakzeptanz für alle geplanten Tabellen gelb.**

PAPGT: 12-Byte-Header; `entry_count:u8`; je Entry `is_optional:u8`, `language:u16`, `always_zero:u8`, `name_offset:u32`, `pamt_checksum:u32`; danach `i32`-Länge und NUL-terminierter Namenspuffer. Die reale Registry enthält **39 Einträge**, Jenkins-Checksum `0x953ce06a`. Vorhanden sind **35** numerische Gruppenordner, alle registriert. `0036`, `0038`, `0039`, `0040` sind bereits registrierte optionale Gruppen ohne Ordner. Deshalb sind fehlende Ordner keine freien IDs. Die reale allgemeine Sprachmaske ist **0x7fff**, während ältere Dokumentation/Defaultwerte teils `0x3fff` nennen. Gruppenauswahl und Sprachmaske aus Registry und tatsächlich unterstützten Sprachen ableiten.

Die Referenz ist kein fertiger sicherer Apply-Manager: PackGroupBuilder schreibt direkt in sein Ziel, besitzt keine produktweite Transaktion, Prozessprüfung, Vanilla-Beglaubigung, Backup-/Recovery-Zustandsmaschine oder Fremdmod-Eigentumsverwaltung. Diese Aufgaben muss B0 selbst implementieren. Registry-Upsert auf einem bereits modifizierten PAPGT lässt alte Namen im Raw-Namenspuffer stehen; aus dem unveränderten Snapshot neu aufbauen erfüllt „rebuild, don't stack“. Grenzprüfungen für `u8`-Entrycount, `u16`-Chunk-ID und `u32`-Offsets/Größen ergänzen.

## Warum der März-Unpacker nur Ergänzungsreferenz ist

[PAZ_DECRYPTION.md](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/PAZ_DECRYPTION.md) beschreibt deterministic Basename-Keys, ChaCha20 und LZ4-Blockdaten sowie historische größenfixierte In-place-Repackingstrategien und Timestamp-Erhaltung. Diese Constraints wurden hier weder als allgemeine Engine-Regel übernommen noch getestet; sie sind für B0s eigene Overlay-Gruppe kein belegter Zwang.

Konkrete Widersprüche/Begrenzungen: Das Dokument nennt Kompression 3 „custom“ und 4 „zlib“, während der neuere crimson-rs-Quellcode 3 Zlib/4 QuickLZ verwendet. [paz_parse.py](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/python/paz_parse.py) betrachtet nur `.xml` als verschlüsselt; der lokal bestätigte `.paloc`-Eintrag ist jedoch ebenfalls ChaCha20-verschlüsselt. Sein Basename-Key-Rezept besitzt nicht die allgemeine PAMT-`encrypt_info`-Parametrisierung der aktuellen Rust-Referenz. Den Unpacker deshalb nicht als aktuellen universellen Extractor/Writer in cd-core übernehmen. Seine C++/Python-Programme wurden nicht gebaut oder auf aktuelle Spieldateien angewandt; die aktuelle Extraktion erfolgte über die geprüfte Rust-Referenz.
