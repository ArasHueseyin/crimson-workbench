# Item-Speicherabbild und Rückumwandlung – native private Probe

Nachweis für 0.19.0. Die nachfolgende native Dispatcher-/Queue-Prüfung und
statische Backendzuordnung stehen in [SAVE_BACKEND_2949](SAVE_BACKEND_2949.md).

Stand: 23.09.2026, Entwicklungsmodul 0.19.0. Steam 25455892 / EXE 1.0.0.2949,
SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**38 native Szenarien bestanden:** Originale Funktionskopien wandeln ausschließlich
eigene private Items und Speicherobjekte um. 14 Fälle verbinden den tatsächlichen
Reparaturplan und native Nachherkopien mit beiden Konvertern. Kein Zugriff auf
Spielprozess oder Save-Dateien; kein Nachweis des Schreibens auf Datenträger.

## Alternative SQL-Ergebnisverarbeitung

Die in [SQL_DISPATCH_2949](SQL_DISPATCH_2949.md) beschriebene Methode `+0x30`
gehört zur Transaktion, nicht zum FetchDropResult:

1. Reparatur `0x2be41b0` erwirbt über `0x2bba9d0 → 0x10515670` und
   `0x2bbdde0` einen `ItemPopInventory` aus dem Kontextpool.
2. Die Konstruktion setzt VTable `0x5b05818`; ihr RTTI bestätigt den Typ.
3. `0x27b0950 → 0xff3c350` übergibt diese Transaktion als achtes Argument
   an `0x278a4a0 → 0xfeb27d0`. Das fünfte Argument ist der separate
   `FetchDropResult` an Request `+0x6e8`.
4. Bei Schalterwert 1 ruft der Ergebnisweg die Transaktionsmethode `+0x30`
   auf: `0x35ea90 → 0x873e140`. Dieser vollständige 10-Byte-Körper schreibt
   ausschließlich 0 in den Ergebniszeiger und gibt diesen zurück.

Damit ist auch dieser konkrete Zweig kein Speicherbackend. Der tatsächliche
Schalterwert und mögliche andere Transaktionstypen sind damit nicht bestimmt.

## Item → ItemSaveData

`0x188ccc0`, Länge `0x709`, SHA-256
`584f5202a37f1bc2d08596bddb9c757bb21ff61c56a3f5909b6354daaf016da6`.

Der Empfänger ist ein `ItemSaveData` (RTTI-VTable `0x58b8308`, Größe im
aufrufenden Vektor `0x118`). Quelle ist das bekannte `0xc8`-Byte-Item.
ABI: `(save_object, item, slot_u16)`. Ungültige/leere Items gehen in den
Resetpfad. Für gültige Items sind folgende Übernahmen belegt:

| Wert | Itemquelle | Speicherabbild |
|---|---|---|
| UID | `+0x0`, u64 | `+0x30`, u64 |
| Itemkennung | interner Key `+0x8` über `0x38ab60` | externe Kennung `+0x38`, u32 |
| Slot | drittes Argument | `+0x3c`, u16 |
| Menge | `+0x10`, i64 | `+0x40`, i64 |
| Haupthaltbarkeit | `+0x40`, u16 | `+0x60`, u16 |
| Gespeicherte Sockelanzahl | niederwertiges Byte von `+0x68` | `+0x78`, u8 |
| Logische Sockelgrenze | `+0x70`, u8 | `+0x79`, u8 |

Der Sockelvektor des Speicherabbilds liegt an `+0x80`. Seine Elemente sind
`ItemSocketSaveData` (RTTI-VTable `0x58b7b28`), Stride `0x30`:

- Aktuelle Haltbarkeit `+0x28` erhält das Wort `+0x2` des 6-Byte-ItemSocket.
- Externe Itemkennung `+0x2c` entsteht aus dessen internem Key über `0x38ab60`.
- Die Umwandlung erzeugt **fünf** Sockeldatensätze. Für Positionen außerhalb
  der logischen Grenze bzw. gespeicherten Anzahl verwendet sie einen globalen
  Ersatzdatensatz. Die native Probe bindet diesen als Key `0xffff`, Haltbarkeit 0.

Die Inventarfunktion `0x212cd60` ruft diesen Konverter unter anderem an
`0x212d778` und `0x212dc7b` auf und übernimmt `0x118`-Byte-Speicherobjekte.
Die beiden Reflexions-Setter für `_endurance` (`0x1886c10`, Feld `+0x60`) und
`_currentEndurance` (`0x187ef50`, Feld `+0x28`) passen zu dieser Zuordnung.
Diese Felder waren zuvor ohne gesicherte Objektzuordnung gefunden worden.

## ItemSaveData → Item

`0x188c440`, Länge `0x879`, SHA-256
`b97e21b13592fe436d6c1165f5165faea44f5a6b15fe0141d8d53a4d05c096af`.
ABI: `(save_object, destination_item, slot_out_u16)`.

Der Gegenweg übersetzt externe Kennungen über `0x38aac0`, übernimmt den
gespeicherten Hauptwert und bis zu fünf Sockelwerte aus den `0x30`-Byte-Records.
Er baut zunächst Initialisierungsdaten auf, ruft `0x240a040` auf und weist die
entstandene native Itemkopie über `0x240b020` zu.

Der Hauptwert ist **kein ungeprüfter bytegleicher Rückweg**: Gespeicherte 0
kann bei endlichem Maximum zunächst auf 1 gesetzt werden; es folgt ein
vorzeichenbehafteter Vergleich mit dem ItemInfo-Maximum. Nach Konstruktion wird
bei Maximum `0xffff` der No-Wear-Sentinel gesetzt; ein negativer i16-Hauptwert
bei endlichem Maximum wird durch dieses Maximum ersetzt. Die native Probe bestätigt
0 → 1, über Maximum → Maximum, gespeichertes `0xffff` bei endlichem Maximum → Maximum
und aktives No-Wear-Maximum → `0xffff`. Eine vorbereitete Reparatur verwendet weiterhin
das verifizierte endliche Vanilla-Maximum; erst der Lader wendet die No-Wear-Regel an.

Der originale Initialisierungskonstruktor materialisiert fünf Sockeldatensätze
und erhält die gespeicherte logische Grenze. Key und Haltbarkeit stimmen für alle
fünf Datensätze, auch für freie Plätze. Die privaten Richtlinienschalter an
`0x6cf7248/+4` stehen in dieser Probe auf 0. Ihr Wert im Spiel wird nicht gelesen.
Insbesondere ist damit keine allgemeine Zulassung einer initialisierten Anzahl
größer als die logische Grenze für den Reparaturadapter bewiesen.

## Native Prüfung 0.19.0

Je 19 Szenarien in beiden TLS-Allokationsmodi:

| Fälle je Modus | Prüfung |
|---|---|
| 12 direkte Umwandlungen | Hauptwert 0/35/100/150/65535, aktives No-Wear, 0/1/2/5 initialisierte Sockel, logische Grenze 0/5, freier Sockel und Sockelwert 0 |
| 6 vorbereitete Reparaturen | Hauptitem und Sockel beschädigt, nur Sockel beschädigt, No-Wear, Haupt-/Sockelwert 0, keine initialisierten Sockel, ein Sockel bei logischer Grenze 1 |
| 1 Reparatur mit Zusatzdaten und Wiederverwendung | Drei Färbedatensätze, ungültiges Item setzt gefülltes Speicherobjekt zurück; neues Item ersetzt danach Speicherobjekt und bereits gefüllte Lade-Zielinstanz |

Geprüft werden UID, Itemkennung, Menge, Enhancement, Slot, Hauptwert und alle
Sockelwerte. Für die drei Färbedatensätze bleiben sämtliche 13 bedeutungstragenden
Bytes erhalten; ihre Item-/Speicherstrides unterscheiden sich (`0x10`/`0x38`).
Quellen und Schutzmarkierungen bleiben unverändert. Der Lader verändert weder
das Speicherobjekt noch dessen Sockel-/Färbevektoren. Reset entfernt die gültige
Identität und Vektorlängen; reservierte Kapazität bleibt für Wiederverwendung
erhalten. Keine alten Sockel oder Färbungen gelangen in das Ersatzitem.
Originale Destruktoren geben alle privaten Allokationen ausgeglichen frei.

24 zusätzliche vollständige Funktionsbereiche sind per SHA-256 gepinnt,
einschließlich separater Unwindfragmente und 15 zusätzlicher TLS-Lesestellen.
Die Stelle `0x188c50a` lädt ursprünglich RCX statt RAX; nur genau diese geprüfte
Instruktion erhält die entsprechende private TLS-Umleitung. Das gesamte Manifest
enthält 82 Bereiche, 108 pdata-Fragmente und 50 TLS-Stellen. Code liegt ausführbar
und schreibgeschützt; die Original-EXE wird nur gelesen.

Quellen: [Manifest](tests/save_functions.inl), [native Probe](tests/save_native.inl).
Katalog-/Keyübersetzung, Reflexionsschlüssel/Benachrichtigungen, Uhr, Richtlinien,
leere Ersatzdaten und Allokator sind eigene Testabhängigkeiten. Nichtleere
Reflexionsmetadaten, optionale Itemgruppen-/Charakterkonvertierungsdaten und
Fehler bei fehlgeschlagener nativer Allokation sind hier nicht ausgeführt.
Sprachspezifische C++-Cleanup-Handler des Spiels werden nicht registriert;
geprüfte Stack-Unwinddaten bleiben erhalten. Die Testcallbacks werfen nicht.

## Nachweise und nächste Prüfung

`.local/verify_save_route_v18.py` prüft sieben vollständige Funktionshashes,
vier RTTI-Typen, den Sprung und die VTable-Methode sowie 15 Instruktionsanker.
Historisches Ergebnis: `.local/repair-runtime-v18-save-route.json`, ausdrücklich
`static_only=true`, `native_execution=false`.

Aktuell: neun CTest-Suiten bestanden; insgesamt **279 native Szenarien,
3.950 Aufrufe und 139.784 Bedingungen**. 38 davon sind diese Umwandlungsfälle,
14 davon verwenden vorbereitete Reparaturen. Ergebnis und Validierung:
`.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`.
`save_disk_commit`, `engine_commit` und `live_installable` bleiben ausdrücklich false.

Als Nächstes reguläre Erzeugung, Serialisierung und Abschluss dieser Objekte
verfolgen, beginnend mit dem Inventaraufrufer `0x212cd60`. Weitere optionale
Zusatzdaten und die tatsächlichen Sockelrichtlinien bleiben gesonderte Prüfungen.
Dazu ist kein Lesen bestehender Saves nötig.

### Statischer Anschluss an den übergeordneten Speicherweg

Nach der nativen Probe wurden sechs weitere vollständige Funktionen, drei VTables,
drei Sprungziele und 14 Instruktionsanker lesend geprüft. Dieser Anschluss wurde
**noch nicht nativ ausgeführt**:

- Client-/Common-/ServerInventoryActorComponent verwenden `0x212cd60` an VTable
  `+0xd0`, den Lader `0x212a880` an `+0x50` und den Namensgetter `0x9c19a0`
  an `+0xd8`. Letzterer liefert exakt `inventory`.
- `0x20695f0` durchläuft eine Komponentenliste (`+0x78`, Anzahl `+0x80`, Stride 16),
  fragt den Namen über `+0xd8` ab und ruft `+0xd0` mit Ergebniszeiger und eigener
  Sammlung für Speicherobjekte auf. Ein Fehler beendet die Weiterverarbeitung.
- `0x2069c73` ruft `0x1296fa0` mit der Objektsammlung und einem Ausgabepuffer auf.
  Anschließend übernimmt `0x235af90 → 0xee28130` dessen Besitz in einen benannten
  Eintrag. Der Serialisierer selbst ist hier noch nicht semantisch geprüft.
- `0x23556b0 → 0xee09e40` ruft am Backend (`Kontext +0x10`) die virtuellen Slots
  `+0x20`, `+0x48`, `+0x28`, `+0x30` auf. Die jeweiligen Fehlerdiagnosen heißen
  Load Fail, Valid Fail, Save Fail und Delete Fail. Der konkrete Backendtyp fehlt.
- Auch dieser Dispatcher hat einen Erfolgszweig ohne Backendaufruf: gesetztes
  Byte `+0x69c`/`+0x699` oder gelöschtes `+0x698` überspringt die Folge. Erst nach
  erfolgreich durchlaufenen Backendaufrufen setzt er `+0x69b = 1` und `+0x698 = 0`.
  Ein bloßes true darf deshalb wiederum keinen Reparaturauftrag bestätigen.

Reproduzierbar mit `.local/verify_save_dispatch_v19.py`; Ergebnis
`.local/repair-runtime-v19-save-dispatch.json`. Komponentenbesitz, konkrete
Backendmethoden, Zeitplanung und Abschlusszuordnung sind die nächsten Schritte.

Der gefundene Umwandlungsweg beweist, dass beide Haltbarkeitsarten im
Speicherobjekt vertreten sind. Er beweist noch nicht, wann dieses Objekt erzeugt,
auf Datenträger geschrieben oder für einen konkreten Reparaturauftrag bestätigt wird.
Die spätere Spielabnahme R06 bleibt offen: [TESTCHECKLISTE](../../TESTCHECKLISTE.md).
