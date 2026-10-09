# Phase 0 – Machbarkeit

**Ergänzung Phase 2:** Desktopoberfläche, Suche/Filter, Rohfelder, direkte Itemlinks
und echte Icons stehen. A1 ist wegen der weiterhin ungeklärten Händler-, Drop-
und Rezeptquellen noch nicht vollständig. [Aktueller Stand](PROGRESS.md),
[Bedienung und Grenzen](DESKTOP.md). Die folgenden Abschnitte sind historische
Recherchebefunde aus Phase 0 bzw. der unten bezeichneten Phase-1-Ergänzung.

**Ergänzung Phase 1:** Der lesende Rust-Core mit CLI, Item-Lokalisierung und
SQLite-FTS5 steht; Tests und Grenzen siehe [PROGRESS.md](PROGRESS.md).
Der folgende Bericht dokumentiert die Phase-0-Untersuchung. Seine offenen
Gameplay-, Editor-, Save- und Kartenfragen bleiben bestehen; die GUI folgt erst
in Phase 2.

Stand: 19.09.2026, aktualisierte Spec mit eigener Apply-Engine B0. Die Untersuchung
hat die sechs Referenzprojekte und die lokale Steam-Installation einbezogen.
Es wurde kein App-Code geschrieben und keine Spiel-/Save-Datei verändert.

**Ergebnis:** Der lesende Core und die Itemdatenbank haben eine nachgewiesene
technische Basis. Eine eigene Overlay-Engine ist durch Referenzcode gut begründet,
aber hier noch nicht im Spiel getestet. Die vollständige Wunschliste ist nicht
allein durch bekannte Scalar-Patches abgedeckt: Rezepte, Kartenkalibrierung,
Engine-Maxima, vollständige Kosten-/Verschleißfreiheit und einige Trust-/Dragon-
Eigenschaften benötigen weitere Verifikation oder Reverse Engineering.

## Ampel und Evidenz

- **🟢 Grün:** Datenstruktur/Feld oder technischer Weg konkret belegt. Das ist keine
  Releasefreigabe für eine noch nicht gebaute Funktion.
- **🟡 Gelb:** Teile bekannt; mindestens eine benötigte Beziehung, Semantik oder
  aktuelle Buildprüfung fehlt.
- **🔴 Rot:** für den genannten Teil kein belastbarer Implementierungsnachweis.

Quellcodebefunde und lokale Prüfungen sind getrennt. Ein Feldname oder historischer
Offset allein ist kein Nachweis seiner Wirkung. Der Build ist **beobachtet**, noch
nicht als Vanilla oder vollständig unterstützter Workbench-Build zertifiziert.
Details und Quellen: [FORMATS](FORMATS.md), [Tabellenrecherche](research/TABLES.md),
[Apply-Recherche](research/APPLY.md), [Archivrecherche](research/ARCHIVES.md).

## Technische Basis

| Teil | Ampel | Nachweis / Entscheidung |
|---|---|---|
| PAZ/PAMT lesen | 🟢 | Referenzbibliothek gebaut; echte Tabellen aus Gruppe 0008 entschlüsselt/dekomprimiert. PAMT und PAPGT im Speicher bytegleich serialisiert. |
| Iteminfo-Grundstruktur | 🟢 | 6.816 lokale Items, 6.465.724 Bytes: parse → serialize byteidentisch. Das beweist die Strukturerhaltung dieses konkreten Files, nicht die Semantik jedes unbekannten Felds. |
| Deutsche Lokalisierung | 🟢 | Gruppe 0027, `gamedata/stringtable/binary__/ger`, 39 Namespaces gefunden. `item.paloc` tatsächlich LZ4-verpackt; 13.575 Einträge. Payload-Roundtrip nach Dekompression bestanden. |
| Direkte Rust-Git-Abhängigkeit crimson-rs | 🟡 | MIT und lokal buildbar; derzeit nur cdylib/staticlib und private Rust-Module. Ein unverändertes `git`-Dependency liefert nicht die erforderliche Rust-Core-API. Empfehlung: kleiner dokumentierter Fork mit rlib/public API oder begrenzte MIT-Portierung, jeweils gepinnt; keine ungeprüfte Komplettintegration. |
| Alle Tabellen lossless bearbeiten | 🟡 | Iteminfo nachgewiesen; etliche andere Loader sind bewusst lossy. Jeder zusätzliche Editor bleibt bis zu seinem eigenen aktuellen Roundtrip gesperrt. |
| Vanilla-Nachweis / Buildfreigabe | 🟡 | EXE-Version, Steam-Build-ID, Metadaten und 28 Tabellen-Dateihashes erfasst. Herkunft eines unveränderten Baselinesatzes und versionierte Schemas müssen vor Schreibfreigabe feststehen. |
| SQLite FTS5, Tauri/React, Profile | 🟢 | Kein Format-Reverse-Engineering erforderlich. Noch nicht implementiert; Windows-/Linux-Build und reale Such-/UI-Tests folgen in den vorgesehenen Phasen. |

## Funktionen

Die Dateinamen in der Tabelle meinen jeweils `.staticinfobody` plus den
tabellenspezifischen `.staticinfoheader` unter `gamedata/binarystaticinfo__/bin`.
Feldnamen sind Quellenbezeichnungen, keine bereits freigegebene Workbench-API.

| Funktion | Ampel | Konkrete Datenbasis und verbleibende Arbeit |
|---|---|---|
| **A1 Itemdatenbank** | 🟡 | `iteminfo`: Schlüssel, `string_key`, Kategorie/Typ, Iconreferenz, `max_stack_count`, Stats/Buffs. Item-Roundtrip und deutsches PALOC lokal bestanden. FTS5/Filter/virtuelle Tabelle sind Implementierungsarbeit. Item→Lokalisierung, Icons, Store→Region und Drop→Quelle sowie Rezeptlinks noch vollständig prüfen. Unbekannte Bytes als Offset/Typ/Wert zeigen. |
| **A2 Crafting** | 🟡 | `crafttoolinfo`, `crafttoolgroupinfo`, `multichangeinfo`, `itemgroupinfo`: `_ingredientsItemGroupInfo`, `_recipeItemGroupInfoList`, `_fixedMaterialDataList`, `_resultDropInfoList`, `_craftToolInfo`. Erste drei Tabellen lokal vorhanden/gehasht. 🔴 Vollständiger Zutaten-/Ausgabegraph inklusive Mengen, Alternativen und Erfolgsregeln noch unbewiesen. Erst bekannte Rezepte gegen Spielanzeige abgleichen; danach Rekursion, Zyklen und gemeinsam verbrauchbaren Eigenbestand berechnen. |
| **A3 Karte** | 🟡 | Assetpfad `ui/texture/image/worldmap/`; `UIMapTextureInfo`, `BitmapPositionInfo`, `FactionNodeInfo`, `GamePlayTriggerInfo`, `GimmickGateInfo`, `KnowledgeInfo` enthalten Positions-/Textur-/Grenzkandidaten. 🔴 Vollständiger Atlas, Koordinatentransform und vollständige Collectible-Instanzen fehlen. UIMapTexture kann Icons statt Hintergrund bezeichnen. Keine Karten-UI vor diesem Nachweis. |
| **A4 100%-Tracker** | 🟡 | `questinfo`, `stageinfo`, Mission-/Gimmick-Tabellen plus `QuestSaveData`/`FieldGimmickSaveData`. Lokaler Save-Pfad gefunden; Saveinhalt nicht analysiert. Zustandsmapping und belastbarer Nenner über optionale/verzweigte Inhalte fehlen. Collectibles nicht pauschal als aus Save ableitbar darstellen. Shared read, stabile Wiederholungslesung und Dateiwatcher später implementieren. |
| **B0 Apply-Engine** | 🟡 | PAZ/PAMT und PAPGT-Registry strukturell bekannt; aktuelle Registry lokal geprüft. CDUMM erzeugt eigene Gruppen, berechnet Jenkins-Checksummen und registriert vorne. Kein CDUMM-Aufruf/-Dependency nötig. 🔴 Lokaler Engine-Vorrang, Apply/Restore-Zyklen, Vanilla-Herkunft, Crash-Recovery und Spielstart-Rennen noch ungetestet. [Entwurf](APPLY_DESIGN.md). |
| **B1 Shops** | 🟡 | `storeinfo`, Itemreferenz, Stock-/Preis-/Refreshfelder. Body/Header lokal gehasht. Aktuelle Quelle beschreibt +8 rohe Bytes je Stockstruktur und ein zusätzliches Store-Byte. Historische Parser nennen 105→113, daraus folgt keine allgemeine feste Recordgröße bei optionalen Blöcken/Listen. 999 Bestand und täglicher Refresh erst nach eigenem Roundtrip, aktuellen Feldtests und Prüfung großer Händlerlisten; pauschal alle Items kann Sonder-/Questitems einschließen. |
| **B2 Drops** | 🟡 | `dropsetinfo`: Itemreferenzen, Gruppen-/Einzelchance und Mengen. Aktuelle Referenzen unterscheiden Set- und Item-Chance; Semantik nicht auf einen einzigen Prozentwert reduzieren. 🔴 Automatische Bossklassifikation fehlt; zunächst manuelle Auswahl. Global-/Override-Priorität und Chance-/Mengenbegrenzungen buildbezogen definieren. |
| **B3 Trust** | 🟡 | GildyBoyes `DropSet_Friendly_*`-Heuristik markiert vier Bytepositionen bei Offsets 59/63/67/71 relativ nach Name+NUL. Daraus lässt sich noch kein Feldtyp ableiten. Das ist ein lokalisierbarer historischer Hinweis, **kein bestätigtes aktuelles Trust-Feld**. 🔴 Typ, Einheit, Ereigniszuordnung und Multiplikatorwirkung benötigen kontrollierte Prüfung. |
| **B4 World** | 🟡 | `characterinfo`: `_terrainRegionSpawnPerCount`, `_callMercenaryCoolTime`, `_callMercenarySpawnDuration`; `regioninfo`: `_limitVehicleRun`, `_isTown`, `_vehicleMercenaryAllowType`; `fieldinfo`: `_canCallVehicle`; Faction-/Collection-/Gimmickdaten für weitere Spawns/Timer. Einige Felder sind Heuristik/dev-only. Respawnquellen und Spawn-Instanzen nicht gleichsetzen. |
| **B5 Dragon** | 🟡 | Mountfelder aus `characterinfo`, `vehicleinfo` und Region-/Field-Flags. Historische Dragon-ID ist nur Kandidat. 🔴 Blackstar-Zuordnung, verlässlicher Grenzübertritt ohne Dismount und Bedeutung von Duration/Cooldown 0 nicht bestätigt. |
| **B6 Inventory/Lager** | 🟡 | `inventory`: `default_slot_count`, `max_slot_count` (u16) im Referenzparser; Tabelle lokal gehasht. Referenz sucht plausible Wertpaare heuristisch. 🔴 Lager-Eintrag und hartes Engine-Maximum unbekannt. u16-Maximum 65.535 ist kein zulässiger Produktgrenzwert ohne Nachweis; Referenzmods fehlen noch. |
| **B7 Stacks** | 🟡 | Aktuelles typisiertes `iteminfo.max_stack_count` und `_applyMaxStackCap`; alte Quellen widersprechen sich bei Feldbreiten. Preset 999999 ist kein Maximalwertbeleg. 🔴 Engine-Maximum und verlustfreie Stacks mit Instanzzustand unbekannt. Equipment/Enchant/Sockets/Durability ausschließlich experimentell nach Tests. |
| **B8 Durability** | 🟡 | `iteminfo._maxEndurance`, Repair-/Equip-Daten und `_decreaseEndurancePercent` als Kandidaten. 🔴 Maximalhaltbarkeit 65535 bedeutet nicht „kein Verschleiß“; Reparaturkosten-Nullfeld und echte Verlustabschaltung sind nicht bestätigt. |
| **B9 Stamina & Spirit** | 🟡 | `skill._useResourceStatList/_useDriverResourceStatList` und entsprechende Stat-Hashes. 🔴 Alle Aktionskategorien, Einheiten und universell kostenloser Verbrauch fehlen. Ausrüstungsreduktion/Regeneration ersetzt keinen bestätigten globalen Multiplikator. |
| **B10 Skills** | 🟡 | `skill`, `skilltreeinfo`, `cooltime`, Ressourcen-/Bufflisten. Tabelle lokal gehasht; zusätzliche Roundtrip- und Wirkungstests nötig. Ein Referenzbutton „Zero Cooldowns“ setzt 100; Nullsemantik deshalb nicht aus UI-Text ableiten. |
| **B11 God-item** | 🟡 | `iteminfo` Stat-/Buff-/Passive-/Enchantlisten und `drop_default_data`; Struktur roundtrip-fähig. Namensräume und Statwirkungen separat prüfen. Längenänderung braucht neuen Headerindex; RegisterConditionSkillBuffData auf ungeeigneten Items ist ein dokumentierter Ladefehlerkandidat. Templates erst für bestätigte Kombinationen. |
| **C Profile / Farm** | 🟡 | Profil-JSON und deterministische Einstellungen benötigen keine Spieldaten. Farm = Drops ×10, Spawns ×3; Wirkung hängt von B0/B2/B4 ab. Wechsel bei laufendem Spiel plant höchstens Änderungen und fordert späteres Schließen; er führt kein Apply/Restore aus. |
| **D Runtime-ASI** | 🔴 | Keine Hooks untersucht, keine Umsetzung begonnen. Eigene spätere Phase ausschließlich nach explizitem Go. |

## Karte: vorgeschalteter Machbarkeitsbericht

Vorhanden sind Assetpfad- und Positionskandidaten sowie ein Reader für mehrere
PAZ-Kompressionsvarianten. Nicht nachgewiesen sind ein verwendbarer kompletter
Weltkartenhintergrund, dessen Tileanordnung/Projektion, eindeutige Instanzpositionen
aller Sammelobjekte oder eine vollständige Save-Zuordnung. Teilkomprimierte
Texturen benötigen einen Variantencheck; Kommentare in älteren Readern beschreiben
teils noch nicht unterstützte Fälle.

Nächste Schritte in Phase 8: konkrete Assets inventarisieren und dekodieren,
mehrere bekannte Welt-/Bildpunktpaare sammeln, Achsen und Maßstab bestimmen,
Transformation an unabhängigen Punkten überprüfen, dann Layerdaten mit stabilen
IDs verbinden. Bis dahin ist Leaflet `CRS.Simple` eine geplante Darstellung und
keine validierte Koordinatenlösung.

## Prioritäten für Phase 1 nach Durchsicht

1. Read-only-Erkennung, Buildbeobachtung und klare Trennung von bekannten Schemas
   und unbekannten Builds; keine Schreibbefugnis für das Core-Lesemodul.
2. Gepinnte Rust-Integration entscheiden, Archiv-/Item-/PALOC-Nachweise in echte
   Workbench-Tests mit `.env`-Pfaden überführen; unbekannte Felder bewahren.
3. CLI und SQLite-Index implementieren; weitere Tabellen erst mit passendem
   Roundtrip aufnehmen. GUI/Editoren und insbesondere Apply bleiben spätere Phasen.

Es bestehen keine Fragen, die die Lieferung dieses Rechercheberichts verhindern.
Die fehlenden Referenzmods und Wirkungsnachweise sind dokumentierte Abhängigkeiten
späterer Funktionen. Phase 1 startet nicht automatisch.
