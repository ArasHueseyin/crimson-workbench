# Herstellungsrechner – Phase 3

Stand: 19.09.2026, Steam-Build 25381195, EXE 1.0.0.2944. Die App berechnet
**1.108 Rezepte mit eindeutigem Item-Ergebnis** aus der lokalen Installation.
Händler, Preise, Bestände, Regionen und die Zuordnung von Dropsets zu konkreten
Gegnern/Weltobjekten sind weiterhin offen. Damit ist der Rechner nutzbar, der
gesamte Quellenumfang von A2 aber noch nicht vollständig erfüllt.

## Bedienung

1. **Herstellungsplan** öffnen, Item suchen und gewünschte Gesamtmenge eingeben.
2. Bei mehreren Rezepten die gewünschte Variante im Materialbaum wählen.
   Austauschbare Zutatengruppen bieten ein Auswahlfeld mit echten Itemnamen.
3. Unter **Schon vorhanden** Vorräte eintragen, einschließlich bereits
   hergestellter Zwischenprodukte oder des Zielitems. Vorräte werden nur einmal
   im gesamten Baum verbraucht. **Leeren** setzt die manuellen Mengen zurück.
4. **Noch beschaffen** zeigt die zusammengefassten fehlenden Materialien.
   „Stattdessen beschaffen“ beendet die Herstellung an einem Zwischenprodukt.
5. Materialzeilen aufklappen: Itemdetails, gelesene Dropset-Referenzen und
   ausdrücklich als ungeprüft markierte Händler-/Weltquellen.

In den Itemdetails unter **Verknüpfungen** führen „Herstellen“ und „Als Zutat
verwendet“ direkt zum passenden Rezept. Die Auswahl und manuellen Mengen bleiben
beim Seitenwechsel erhalten. Neue Installation, Neueinlesen, Sprachwechsel oder
Neustart setzen sie zurück. Es gibt keine Save-Auswertung und keinen Apply-Aufruf.

Pro Zutatengruppe wird eine Materialart gewählt. Standard ist das erste nach
lokalisiertem Namen sortierte Material; dies ist **keine Preisoptimierung**.
Gemischte Ersatzmaterialien innerhalb eines Gruppenslots werden nicht optimiert.
Wissen und Bedingungen sind Voraussetzungen, keine aus einem Save bestätigten
Freischaltungen. Die Anzeige „kein berechenbares Rezept“ bedeutet nicht
„im Spiel unmöglich herzustellen“.

## Wo die Rezepte liegen

Archivgruppe `0008`, Verzeichnis `gamedata/binarystaticinfo__/bin`:

| Tabelle | Zeilen | Verwendung |
|---|---:|---|
| `multichangeinfo` | 18.576 | Werkzeug, Voraussetzungen, feste Materialien, Zutatengruppen, Ergebnis-Dropsets |
| `itemgroupinfo` | 1.602 | Gruppenmitglieder und untergeordnete Gruppen |
| `dropsetinfo` | 14.747 | Ergebnis-Items und Mengen; auch allgemeine Item→Dropset-Referenzen |
| `crafttoolinfo` | 19 | Werkzeugschlüssel; weiterhin der bereits geprüfte Tabellenbestand |

Alle Indizes haben einen u16-Zeilenanzähler. Multichange und Dropset verwenden
u32-Schlüssel, ItemGroup u16-Schlüssel; Offset jeweils u32. Die Rezepte stammen
**nicht** aus einem Scan beliebiger Item-IDs oder einer eingebauten Rezeptliste.

Die bisherigen 28 Tabellenfingerprints gelten weiter. Die zwei neuen
ItemGroup-Fingerprints stehen in
`crates/cd-core/schemas/steam-25381195.crafting.json`. Sie werden vor dem
Interpretieren verglichen. Das Crafting-Schema ist eigenständig für diesen
Build implementiert; der bestehende Item-Suchindex `…gamedata-2.3-v2` ändert sich
nicht, weil er keine Rezeptdaten speichert.

## Gemessene Datensatzstruktur

Alle Zahlen little-endian. `bytes` = u32-Länge + Bytes; `text` = u8-Kategorie +
u64-Lokalisierungsindex + `bytes`; `array(T)` = u32-Anzahl + T-Einträge.
Unbekannte Felder werden erhalten, aber nicht mit erfundener Bedeutung benannt.

**MultiChange:** u32 key, bytes stringKey, u8 blocked, u16 tool, u8 consume,
array(u32 Referenz + text) conditions, u32 knowledge, bytes tag, 5 rohe Flags,
array(fixed), array(group), u32 elemental, array(u32) states, text description,
text name, 3 u32-Stringreferenzen, text complete, array(u32) results,
array(u32) additional.

- `fixed`: u32 item/character/gimmick, u64 count/coupon, u16 enchant – **30 Bytes**.
- `group`: u16 group, u64 count, u16 ungeklärtes Zusatzfeld – **12 Bytes**.
- 17.069 Zeilen tragen Verbrauchstyp 3 und gehören überwiegend zu
  Verstärkungsvarianten; der Rechner interpretiert sie nicht als normale Rezepte.
- Alle 1.507 Zeilen mit Verbrauchstyp 1 haben genau einen Ergebnis-Dropset-Verweis.
  399 davon werden wegen Sonder-/Zufalls-/Verstärkungsergebnissen ausgeschlossen.
- Alle 18.576 Records werden vollständig gelesen und aus ihren Feldern wieder
  serialisiert; die Bytes stimmen jeweils exakt mit dem Original überein.

**ItemGroup:** u16 key, bytes stringKey, u8 blocked, text groupName,
array(u16) groups, array(u32) items, array(u8) ungeklärte Typen, 11 rohe Schlussbytes.
Die Schlussbytes enthalten bekannte Feldkandidaten wie Sortierung/Icon, werden
hier aber nicht semantisch verwendet. Alle 1.602 Zeilen roundtrippen exakt.
Untergruppen werden rekursiv vereinigt und dedupliziert; Zyklen, gesperrte Gruppen,
unbekannte Gruppentypen und fehlende Mitglieder werden nicht still ignoriert.

**Dropset, begrenzter Item-Reader:** u32 key, bytes stringKey, u8 blocked,
u8 rollType, u32 rolls, bytes condition, u32 tag, array(itemDrop), u16 slots,
u64 weight/totalRate/unknownTail, bytes originalString, u8 unknownEnd.

`itemDrop`: u8 flag, u32 item/unknown/kind, 5 rohe Bytes, u32 condition/post,
u64 rate, u32 rate2/unknown2, u64 max/min, u16 enchant, u32 duplicateItem.
Der bestätigte einfache Item-Eintrag ist **64 Bytes**, nicht die ungeprüften
68 Bytes des gelesenen Referenzparsers. Variante flag=1/kind=0 wird gelesen;
andere Varianten bleiben vollständig opak. 13.035 Dropset-Records bestehen
ausschließlich aus dieser gelesenen Variante und roundtrippen exakt. Die anderen
1.712 werden nicht durch Suchmarker „repariert“ oder als erfolgreich interpretiert.

Für berechenbare Ergebnisse gelten zusätzliche konservative Bedingungen:
ein Item, gleiche positive Min-/Maxmenge, keine Zusatzbedingungen oder
Sonderfelder, passende doppelte Item-ID, Basis-/Standard-Verstärkungsstufe,
Rate und Gesamtwert 1.000.000, bestätigte einfache Rollkonfiguration. Allgemeine
Tabellenraten anderer Dropsets werden roh gezeigt; daraus wird keine unbestätigte
Prozentchance oder Weltquelle errechnet.

Die Index-/Recordgrenzen aller drei Tabellen werden ebenfalls byteidentisch
rekonstruiert. Ein Roundtrip beweist die Byteabdeckung, nicht automatisch die
Gameplaybedeutung jedes Feldes. Nicht geklärte Verbraucherregeln bleiben vom
berechenbaren Teil ausgeschlossen.

## Konkrete lokale Gegenproben

- MultiChange 1 → Dropset 160100 → Item 50001: 5 Bauholz (710001) und
  1 Eisenerz (720001) ergeben 30 Pfeile. Ziel 31, Vorrat 3 Bauholz:
  2 Vorgänge, 60 erzeugte Pfeile, 7 Bauholz und 2 Eisenerz fehlen, 29 Pfeile übrig.
- MultiChange 343 → Dropset 1015536 → Item 1002066, „Gemischter Eintopf“:
  5 Wasser und vier Zutatengruppen mit Mengen 2/3/2/2. Die Meeresfrüchtegruppe
  17494 löst sich zu 48 Items auf. Die Gruppe wird nicht als Bedarf aller
  48 Alternativen missverstanden.
- Alle 1.108 freigegebenen Rezepte erzeugen im Echtdateitest einen begrenzten
  Plan. Weitere Spielregeln, Freischaltungen und Wirkungen wurden nicht im
  laufenden Spiel getestet.

## Rechenregeln und Grenzen

`Vorgänge = ceil(fehlende Menge / Ergebnis pro Vorgang)`. Exakte u64-Arithmetik
mit Überlaufprüfung; über IPC/JSON sind Mengen Dezimalstrings. Vorräte und
produzierte Überschüsse gehören zu einem gemeinsamen Ledger für den ganzen
Baum. Der Nutzer kann Zwischenprodukte extern beschaffen. Rezeptzyklen erzeugen
eine sichtbare Warnung und einen Beschaffungsposten, keine Endlosschleife.
Maximal 1.000 Baumknoten, 32 Ebenen, Zielmenge 1 bis 10^12. Keine probabilistische
Erwartungswertrechnung, kein globaler Kosten-/Rezeptoptimierer.

CLI:

```powershell
.\target\release\cd-cli.exe recipes --item 50001
.\target\release\cd-cli.exe craft .local\plan.json --output exports\materialplan.json
```

`plan.json`, selbst angelegt im Projekt:

```json
{"target":50001,"quantity":"31","owned":{"710001":"3"},"recipes":{"50001":1},"choices":{},"acquire":[]}
```

JSON-Ausgaben verwenden dieselbe geschützte Ausgabe-Policy wie der bisherige
Core. Der Rechner besitzt keine Schreibmethode für Spielarchive oder Saves.

## Quellen und eigenständige Umsetzung

Die gepinnten Quellen und Lizenzentscheidungen stehen in [CREDITS](../CREDITS.md)
und [der Tabellenrecherche](research/TABLES.md). Die NattKh-Schemaunterlagen
lieferten Feldkandidaten; ihre Registrierungsreihenfolge ist nicht die
Serialisierungsreihenfolge. Die MIT-crimson-rs-Loader belegten Keys/Namen und
Indexbreiten, lieferten aber keinen Rezeptrechner. NattKhs MPL-Dropset-Editor
wurde als Formatvergleich gelesen; sein Byteformat wurde nicht ungeprüft
übernommen. Die neuen Rust-Reader/Writer und der Planer sind eigene
Implementierungen anhand der lokal vermessenen Daten. Kein fremder
Python-/Editorcode und keine Spieldatei wurde in das Projekt kopiert.
