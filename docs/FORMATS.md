# Phase 0 – Formate und lokaler Build

**Ergänzung Phase 2:** Desktop-Itembrowser und reale Itemicons sind implementiert.
Zusätzliches `stringinfo` (31.812 Records) ist SHA-geprüft und durch typisierte
Body-/Header-Roundtrips abgesichert; Details in [DESKTOP.md](DESKTOP.md).
Händler-/Drop-/Rezeptbeziehungen bleiben ausdrücklich ungeklärt.

**Ergänzung Phase 1:** Der native Core ist jetzt implementiert und geprüft;
siehe [CORE.md](CORE.md), [FORMAT_PORT.md](FORMAT_PORT.md) und
[PROGRESS.md](PROGRESS.md). Die folgenden Abschnitte bleiben der Recherchebericht
aus Phase 0. Neu verifiziert: alle 14 Tabellenindizes mit Body-/Header-Roundtrip,
typisiertes Iteminfo, 15 Itemsprachen und PAPGT/PAMT im eigenen Rust-Core.
Quest-/Stage-Header zählen mit `u32`, die übrigen erfassten Header mit `u16`.
13 Tabellenbodys bleiben uninterpretierte Bytes, ohne semantische Editorfreigabe.

Stand: 19.09.2026. Grundlage ist die aktualisierte `SPEC.md` mit eigener
Apply-Engine. **Beobachtungen sind keine Vanilla-Zertifizierung.** Die reale
Installation wurde ausschließlich gelesen; Referenztools wurden nur in isolierten
Projektverzeichnissen gebaut. Saveinhalte wurden nicht untersucht.

Die vollständigen geprüften Revisionen und Lizenzentscheidungen stehen in
[CREDITS.md](../CREDITS.md). Detailbelege:
[Archive](research/ARCHIVES.md), [Tabellen](research/TABLES.md),
[Apply/Overlay](research/APPLY.md). Jede Aussage zu älteren Schemata bleibt an
ihren Quellstand gebunden.

## Installationsbeobachtung

| Merkmal | Tatsächlich festgestellt |
|---|---|
| Plattform | Steam, App-ID `3321460`, Anzeigename „Crimson Desert Enhanced“ |
| Installationsordner | `C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert` |
| Steam-Build-ID | `25381195` |
| Hauptprogramm | `bin64/CrimsonDesert.exe`, 397.242.776 Bytes |
| EXE-Datei-/Produktversion | `1.0.0.2944` |
| EXE SHA-256 | `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7` |
| Meta-Registry | `meta/0.papgt`, 679 Bytes; 39 registrierte Gruppen |
| Registry SHA-256 | `c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea` |
| Registry-interner Jenkins-Hash | gespeichert und berechnet jeweils `0x953ce06a` |
| Vorhandene numerische Gruppen | 35; alle in PAPGT registriert |
| Registriert, ohne Ordner | `0036`, `0038`, `0039`, `0040` – optionale Gruppen, keine frei belegbaren IDs |
| Statische Tabellen | Gruppe `0008`, `gamedata/binarystaticinfo__/bin/` |
| Deutsche Lokalisierung | Gruppe `0027`, `gamedata/stringtable/binary__/ger/`, 39 Namespaces |
| Tatsächlicher Save-Stamm | `%LOCALAPPDATA%/Pearl Abyss/CD/save`, 11 `.save`-Dateien gefunden |

Der in der Spec beispielhaft genannte Save-Pfad unter `CrimsonDesert/Saved/SaveGames`
existiert hier nicht. Steam wurde direkt über Bibliotheks-/Appmetadaten gefunden.
Epic-/Game-Pass-Erkennung ist noch keine implementierte oder verifizierte Funktion.

Der [maschinenlesbare Beobachtungsbericht](builds/steam-25381195.observed.json)
enthält 38 Originaldatei-Hashes (EXE, PAPGT, PAVER und 35 PAMTs), 28 extrahierte
Tabellen-Dateihashes, Lokalisierungsdaten zur Integritätsprüfung und die realen
Roundtrip-Ergebnisse. Er enthält **keine Tabelleninhalte, Assets oder Savebytes**.
Die 28 Tabellenfiles sind Bodies und Header von `characterinfo`, `crafttoolinfo`,
`crafttoolgroupinfo`, `dropsetinfo`, `fieldinfo`, `inventory`, `iteminfo`,
`multichangeinfo`, `questinfo`, `regioninfo`, `skill`, `stageinfo`, `storeinfo` und
`vehicleinfo`. Weitere künftig verwendete Tabellen müssen ergänzend erfasst werden.

Die Daten passen zum von crimson-rs beschriebenen 2.03-Layout. Die EXE-Version
ist jedoch nicht identisch mit der öffentlich bezeichneten Patchversion; sie wird
nicht entsprechend umbenannt. Es gibt noch keine freigegebene Workbench-Schema-
Registry. Unbekannte Hashkombinationen müssen in Phase 1 abgewiesen werden.

## PAZ, PAMT und Gruppenregistry

Die numerischen Gruppenordner enthalten `0.pamt` und nummerierte `.paz`-Chunks.
PAMT führt Pfade/Namen, Chunkzuordnung, Offsets, komprimierte/unkomprimierte Größen
und Flags; PAZ enthält die Nutzdaten. Metadaten und Payload werden getrennt gelesen.
Boundschecks müssen vor Allokation, Seek, Entschlüsselung und Dekompression gelten.

Die [Unpacker-Spezifikation](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/PAZ_DECRYPTION.md)
und der aktuelle Rust-Reader beschreiben ChaCha20. Das ältere Rezept benutzt einen
kleingeschriebenen Basisdateinamen und feste Ableitungskonstanten. Der tatsächlich
hier geprüfte [Rust-Code](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/crypto/chacha20.rs)
benutzt die Basename-Checksum **plus drei PAMT-`encrypt_info`-Bytes** und führt
selbst keine Lowercase-Normalisierung aus. Den Archivnamen daher unverändert
übernehmen; Verzeichnisanteil entfernen. Counter-/Nonce-Semantik muss mit der
benutzten Rust-ChaCha-Variante übereinstimmen. Das historische feste Rezept ist
keine universelle aktuelle Schlüsselableitung.

Der aktuelle Rust-Code ordnet das untere Flags-Nibble als 0=None, 1=Partial,
2=LZ4, 3=Zlib, 4=QuickLZ zu; die März-Dokumentation weicht bei 3/4 davon ab.
None/LZ4/Zlib und einige Partialvarianten sind implementiert, QuickLZ nicht.
Verschlüsselung steht im oberen Nibble: None/ChaCha20 werden unterstützt,
ICE/AES nicht. Unbekannte Flags muss Workbench ausdrücklich ablehnen, statt den
teilweisen None-Fallback des Referenzparsers zu übernehmen. Kein LZ4-Frame unterstellen und
nicht allein aus gleicher/ungleicher Länge den Codec erraten. Die bestehende
Rust-Implementierung unterstützt mehrere Header-/DDS-Mip-Teilkompressionsfälle,
aber daraus folgt keine universelle Texturunterstützung. Lokale Tabellenextraktion
gelang; ein vollständiger Weltkarten-Texturtest wurde nicht ausgeführt.

`meta/0.papgt` registriert Gruppen samt optionalem Flag, Sprachmaske und PAMT-Hash.
Der referenzierte Aufbau hat einen 12-Byte-Header, 12-Byte-Gruppenrecords und eine
größenpräfigierte Namenstabelle. Im lokalen Build beträgt die allgemeine Sprachmaske
`0x7fff`; ältere Quellen und einige Builder benutzen `0x3fff`. Weder diese Konstante
noch Gruppenzählungsheuristiken werden blind übernommen.

Für Overlays beschreibt CDUMM `hashlittle` über PAZ-Daten und über PAMT/PAPGT ab
Byte 12, Seed `0xC5EDE`; die Gruppenregistry referenziert den richtigen PAMT-Hash.
Diese 32-Bit-Spielchecksummen sind **nicht** die SHA-256-Build-/Backupsignaturen.
Genaue Layouts und Quellzeilen stehen in [APPLY.md](research/APPLY.md).

## Tabellen und die Septemberänderungen

Der alte interne Pfad `gamedata/binary__/client/bin/` wurde durch
`gamedata/binarystaticinfo__/bin/` ersetzt. Body/Header heißen nun
`.staticinfobody` und `.staticinfoheader` statt `.pabgb/.pabgh`. Das ist lokal
bestätigt. [CDUMMs table_ext.py](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/archive/table_ext.py)
datiert die Änderung auf den 04.09.2026; NattKhs `table_layout.py` nennt 2.01.00.

Bodies enthalten unterschiedlich aufgebaute Records mit Längen, Strings, Arrays
und optionalen/getaggten Unterstrukturen. Header sind Schlüssel→Bodyoffset-Indizes;
die Schlüsselbreite ist tabellenspezifisch. Bei Iteminfo ist das bekannte Muster
`u16 count + N × (u32 key, u32 offset)`; andere Tabellen benutzen u16-Keys.
Speicherlayout-Offsets aus IDA sind keine Datei-Offsets. Unbekannte Felder,
Reihenfolge und Padding bleiben unverändert roh erhalten.

Bei Größenänderungen muss der passende Header aus den **neu serialisierten**
Records aufgebaut werden. Ein Vanillaheader neben gewachsenem Body kann still
falsche Items adressieren, auch wenn das Spiel startet. Daher werden Body und
Header als zusammengehöriger Prüf- und Ausgabegegenstand behandelt.

### Shops

Die [aktuelle CDUMM-Storequelle](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/src/cdumm/engine/storeinfo_native_parser.py)
beschreibt für ihren gepinnten September-Build acht neue rohe Bytes zwischen
optionalem `sub_data` und `effect_list`-Count sowie ein zusätzliches Byte im
Storeteil. Die acht Bytes besitzen keine bestätigte fachliche Bedeutung.
Ältere NattKh-Parser nehmen 105 bzw. 113 Bytes an und widersprechen sich teilweise
bei Lese-/Schreiboffsets. Das ist **kein universeller Fixed-size-Recordvertrag**.
Die lokale Storetabelle ist vorhanden/gehasht; ein vollständiger Store-Roundtrip
und die aktuellen Stock-/Refreshsemantiken sind noch nicht nachgewiesen.

### Iteminfo

Der lokale Body hat 6.465.724 Bytes und 6.816 Items. Seine SHA-256 ist
`87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce`.
`parse_iteminfo_from_bytes` → `serialize_iteminfo` des gepinnten crimson-rs ergab
**byteidentische Ausgabe**. Jedes gelesene `inventory_info_list` hat zehn Einträge,
passend zur 2.03-Erweiterung gegenüber neun. Der Tabellenheader ist gehasht; sein
eigenständiger Parser-/Serializer-Nachweis ist noch ein Workbench-Testauftrag.

Eine erfolgreiche Serialisierung beweist nicht die Wirkung sämtlicher Statfelder.
Insbesondere benötigt B11 validierte Statnamensräume und kompatible Buff-/Item-
Kombinationen. „Lossy“-Parser, die unlesbare Records überspringen, erfüllen die
Spec nicht und dürfen keine Editoren oder Buildfreigaben begründen.

## Lokalisierung

Seit dem aktuellen Layout liegen Sprachdateien in Namespace-Dateien je Sprache.
Lokal ist Deutsch `ger` in Gruppe `0027`. Die geprüfte `item.paloc` hat 649.912 Bytes,
einen `paloc`-Header von `0x200` Bytes und einen LZ4-Block von 649.400 Bytes, der
zu 1.807.930 Bytes dekomprimiert. Darin wurden 13.575 Einträge gelesen.

Der **dekomprimierte PALOC-Payload** lässt sich bytegleich parsen/serialisieren.
Das ist kein Nachweis identischer LZ4-Neukompression oder eines Roundtrips der
gesamten verpackten Datei. Für die geplante lesende Lokalisierung genügt zunächst
die geprüfte Dekodierung; der Originalcontainer bleibt unverändert. Weitere
Namespaces/Sprachen und alle Item-Key-Verknüpfungen brauchen Integrationstests.

## Saves und Fortschritt

Die [Save-Formatquelle](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/src/save/mod.rs)
beschreibt einen 0x80-Byte-Header mit `SAVE`-Magic, Version/Flags, Größenfeldern,
16-Byte-Nonce und HMAC-SHA256. Der Body wird ChaCha20-entschlüsselt und als
LZ4-Block dekodiert; HMAC prüft die entschlüsselten komprimierten Bytes.
Versionabhängige Schlüssel und weitere rohe Headerfelder sind zu beachten.
Die Referenz kann einen Body trotz falschem HMAC mit `hmac_ok=false` zurückgeben.
Workbench darf ihn nicht als verlässlich gelesenen Fortschritt behandeln; die
Integritätsprüfung braucht einen ausdrücklichen Fehlerzustand und Wiederholungslesung.

Quest-/Mission-/Stage-Verknüpfungen sind aus Quellen bekannt. Welche Savezustände
im **hier installierten Build** welcher 100%-Kategorie entsprechen, wurde nicht
geprüft. Auch eine vollständige Collectible-Abdeckung ist nicht belegt. Lokale
Spielstände wurden weder kopiert noch entschlüsselt oder verändert. Für Phase 7:
geteiltes Read-only-Öffnen, konsistente Wiederholungslesung bei parallelem Speichern,
keine Save-Writer-API aus Referenzen übernehmen.

## Eigene Apply-Engine und Referenzfehler

CDUMM dient allein als Quellreferenz. Eigene numerische PAZ/PAMT-Gruppen werden in
PAPGT registriert; laut aktuellem Referenzcode gewinnt der erste passende Eintrag.
Deshalb setzt dessen Builder Overlayeinträge vor Vanilla. Die numerisch höchste
Gruppennummer allein ist kein Vorrangnachweis. Alle aktiven Module müssen vor
Ausgabe pro Tabellenpfad zusammengeführt werden; das Spiel macht keinen Feldmerge.

NattKhs [MODDING_GUIDE](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/blob/96e7f78fcb00d6cb5615171a7f359f0c2de5b114/CrimsonGameMods/MODDING_GUIDE.md)
berichtet ältere Fehler mit nichtnumerischen Gruppen, alten Headeroffsets und
gemeinsam gepacktem `iteminfo`/`equipslotinfo`. Diese Hinweise werden zu Regressionstests;
seine alten Pfade, Sprachmasken und vermeintlich freien Gruppen-IDs sind keine
aktuellen Vorgaben. Andere Manager können unbekannte Gruppen entfernen – dies
muss später als fremde Zustandsänderung erkannt werden.

Die lokale Registry ist konsistent, aber der Overlayvorrang wurde absichtlich
nicht durch Spielverzeichnis-Schreibversuche geprüft. [APPLY_DESIGN](APPLY_DESIGN.md)
beschreibt Backupherkunft, Journal, Crash-Recovery, laufendes-Spiel-Sperre auch
für Restore sowie die Grenzen eines bloßen Tempdatei-Rename. In-place-Patching
ist kein implementierter Fallback. `meta/0.pathc` muss für reine Tabellenoverlays
nach der Referenz nicht geändert werden.

## Integrationsentscheidung und Prüfgrenzen

crimson-rs ist MIT-lizenziert, lokal mit Rust 1.95 gebaut und für den geprüften
Itembody/PALOC-Payload geeignet. Es stellt aber nur `cdylib`/`staticlib` und private
Rust-Module bereit. Ein unverändertes gepinntes Git-Dependency bietet somit nicht
die gewünschte öffentliche Rust-API. Phase 1 soll einen kleinen gepflegten Fork
mit `rlib`/öffentlichen Core-Modulen oder eine begrenzte, lizenzierte Portierung
entscheiden. Python/C-ABI dienten hier nur dem Referenznachweis.

Die Referenztests meldeten 354 bestanden, 46 ignoriert, 0 fehlgeschlagen. Einige
Live-Tests melden ohne Fixtures intern frühzeitig Erfolg; dieser Suite-Lauf ist
deshalb kein umfassender Realdateibeweis. Separat tatsächlich geprüft wurden
Itembody, `0008/0.pamt`, `meta/0.papgt` sowie der deutsche Item-PALOC-Payload.
Keine App-, Frontend-, Apply-, Restore- oder In-game-Tests wurden durchgeführt.
Ein vollständiger PAZ-Backupsnapshot wurde in Phase 0 nicht angelegt und ist vor
jeder späteren Schreibfreigabe weiterhin erforderlich.
