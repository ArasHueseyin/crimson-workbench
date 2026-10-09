# SQL-Auftragsweg: keine Speicherbestätigung (0.18.0)

Stand: 23.09.2026, Steam 25455892 / EXE 1.0.0.2949. Alle Ausführungen erfolgen
auf privaten Daten im eigenen Testprogramm. Die installierte EXE wird nur
gelesen. Kein Zugriff auf Spielprozess oder Saves, keine Installationsänderung.
Die Desktop-App bleibt v0.5.9; kostenlose Reparatur bleibt gesperrt.

## Ergebnis

**Die bisher als SQL-Ausführung bezeichnete Funktion führt selbst keinen
Datenbankauftrag aus.** Ihr vollständiger Funktionskörper ist jetzt gebunden
und isoliert geprüft. Bei gesetztem Schalter liefert sie 0, ohne den Auftrag
überhaupt zu lesen. Auch ein Nullzeiger wird in diesem Zweig akzeptiert.

Der Ausrüstungs-Slot-Verbraucher kann damit seine Markierungen leeren und einen
erfolgreichen Status erhalten, obwohl keine Speicherung stattgefunden hat.
Ein leerer Änderungsbestand, ein erfolgreicher Client-Ack und die Rückgabe 0
sind jeweils kein Persistenznachweis. Das präzisiert die bisherige Bezeichnung
„Persistenzschnittstelle“ in [0.16.0](PERSISTENCE_2949.md).

Die Produktionsfreigabe bleibt unverändert: `engine_commit=false` und
`live_installable=false`. In diesem Schritt wurde keine Spielanbindung ergänzt.

## Gepinnte Originalfunktionen

EXE-SHA-256:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.

| Funktion | RVA | Länge | SHA-256 |
|---|---|---|---|
| Gesamter Ausführungs-Shim | `0x2776d00` | `0xc8` | `9cf1709ef75ad14106cdcd8e9f66fb55260612696421e28117e00f770b1ad5e3` |
| Erwerb des gemeinsamen Request-Zeigers | `0x2c1da50` | `0x3f` | `6e744fe8978f3b0ae40d289c92e21a39779228870ac5ce901802e20ed8ba422d` |

Der Shim umfasst fünf pdata-Fragmente: `0x2776d00/0x37`, `0x2776d37/0x27`,
`0x2776d5e/0x41`, `0x2776d9f/0x1e`, `0x2776dbd/0xb`. Die verketteten
Unwinddaten werden geprüft. Nur das erste Fragment zu untersuchen wäre unvollständig.

### Ausführungs-Shim

- Byte an RVA `0x6ceee08` ungleich 0: sofortige Rückgabe 0; keine Prüfung,
  keine Änderung des Requests und insbesondere kein Löschen eines alten Fehlers.
- Byte gleich 0: Request `+0x50` wird auf 0 und `+0x48` auf 4 gesetzt.
  Der Backend-Zeiger kommt aus `+0x10`. Intervall 0 an Backend `+0x34`
  führt unmittelbar zu Rückgabe 1.
- Bei Intervall > 0 wird die Uhr aufgerufen und Backend `+0x38` auf
  aktuelle Zeit plus Intervall gesetzt. Ist die vorherige Frist abgelaufen
  oder genau erreicht und der Schalter weiterhin 0, folgt ein Diagnoseaufruf.
  Dafür wird Request `+0x28` vorübergehend auf Request `+0xb0` umgestellt,
  danach zurückgesetzt und der Status erneut geleert. Rückgabe bleibt 1.
- Es gibt im vollständigen Körper keinen weiteren Aufruf, der einen
  Datenbank- oder Save-Schreibweg implementiert. Uhr und Diagnoseempfänger
  sind im Test eigene kontrollierte Funktionen.

Der tatsächliche Schalterwert im laufenden Spiel wurde nicht gelesen. Aus der
Dateiuntersuchung allein wird kein aktiver Laufzeitmodus abgeleitet.

### Request-Erwerb

`0x2c1da50` baut einen 16-Byte-Wrapper um den Zeiger aus Kontext `+0x88`.
Es entstehen weder ein neuer unabhängiger Request noch ein zurückgesetzter
Fehlerzustand. Wiederholter Erwerb liefert denselben Zeiger; auch Null bleibt Null.
Der Getter lässt R8 unverändert. Das erklärt die Registerverwendung des zuvor
untersuchten Socket-Helfers, ersetzt aber keinen Haltbarkeits-Speichernachweis.

## Ausgeführte Prüfungen

19 neue native Szenarien in [sql_native.inl](tests/sql_native.inl):

- Acht Shim-Fälle: Schalter 1, Schalter 255 mit Nullrequest, Intervall 0,
  zukünftige/exakte/vergangene Frist, während des Uhraufrufs geänderter Schalter
  sowie Nullbackend. Ganze Request-/Backend-Abbilder, Diagnosefolge und
  Unwind bei Zugriffsausnahme werden verglichen.
- Drei Erwerbsfälle: gültiger, fehlender und wiederholt erworbener Request.
  Alter Fehler -77 und die Schutzbytes hinter dem Wrapper bleiben erhalten.
- Acht integrierte Verbraucherfälle: vier Situationen in beiden privaten
  TLS-Allokationsmodi. Originaler Erwerb und vollständiger Shim ersetzen die
  entsprechenden Callbacks. Schalterzweig mit Status 0 wird akzeptiert;
  Rückgabe 1 sowie alte Fehler -7 und +7 ergeben die erwarteten Fehlercodes.
  UID 22 und Haupthaltbarkeit 100 werden korrekt vorbereitet; die Liste wird
  original bereinigt. Referenzen, Sperren und Allokationen sind ausgeglichen.

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Insgesamt **241 native
Szenarien, 3.674 Aufrufe und 128.415 Bedingungen**; viele Bedingungen vergleichen
wiederholt Tabellen. Manifest: 58 Codebereiche, 76 pdata-Fragmente, 35 private
TLS-Stellen. Produktionssuiten unverändert: 43 Transaktionsszenarien/273
Bedingungen und 35 Schreibszenarien/202 Bedingungen.

Kapazitätsverwaltung, Diagnose-/Fehlerempfänger und bisherige UI-/Transport-
Abhängigkeiten bleiben teilweise Testfunktionen. Die beiden neuen Originalkörper
selbst erhalten keine TLS-Ersetzung. Kein Testergebnis bestätigt eine reale Speicherung.

## Nächster erforderlicher Nachweis

Der nachgelagerte Ergebnisweg `0x278a4a0` springt nach `0xfeb27d0` und ruft
bei Schalterwert 1 die virtuelle Methode `+0x30` der Transaktion auf.
Das ist sein achtes Argument; der ebenfalls übergebene FetchDropResult an
Request `+0x6e8` ist das fünfte Argument und nicht der virtuelle Empfänger
dieses Zweigs. Die inzwischen zugeordnete Methode des `ItemPopInventory`
schreibt ebenfalls nur einen Erfolgsstatus. Ein separater Umwandlungsweg zwischen
Item und `ItemSaveData` übernimmt beide Haltbarkeitsarten; er ist bisher nur
statisch belegt. [Zuordnung und nächste native Prüfung](ITEM_SAVE_2949.md).
Der reguläre Speicher-/Ladeweg muss weiter verfolgt werden. Der Socket-Helfer allein
überträgt weiterhin nur eine ItemInfo-Kennung, keinen belegten Haltbarkeitswert.

Serverseitige Inventarmeldungen, vollständige Ereignisse/UI, Rückmeldezuordnung,
Host mit gültiger Spieler-/Manager-/Threadbindung, Eingabe und Loader/B0 fehlen
weiterhin. Die spätere Spielabnahme R06 verlangt einen Vergleich von Hauptitem
und jedem Sockel nach Speichern, Neuladen und vollständigem Neustart.
[Testcheckliste](../../TESTCHECKLISTE.md), [Phasenstatus](../../PHASENSTATUS.md).
