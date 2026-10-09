# Reparatur 0.14.0 – vollständiger Schreibschritt für Haltbarkeitsfelder

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE-Hash:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**Entwicklungsmodul, noch keine installierbare Reparaturaktion.** App v0.5.9
bleibt unverändert und die Reparaturoption gesperrt.

## Implementierter Schreibschritt

`crimson_repair_writer.lib` enthält `writer::Batch` und den lokalen
Speicherzugriff `writer::LocalAccess`. Ein Batch nimmt einen unveränderlichen
Reparaturplan und eine noch gehaltene `reader::HeldCapture` entgegen. Er kann
mitgeführte und ausgerüstete Items einzeln, getrennt oder gemeinsam bearbeiten,
jeweils Hauptitem und Sockel in beiden Zustandskopien.

Adressen werden ausschließlich aus einer frischen Erfassung derselben gehaltenen
Besitzer abgeleitet. Ein abgelöstes `Capture` oder ein rein lesender `Memory`-Adapter
erteilt keine Schreibberechtigung. Der Schreibadapter muss dasselbe Objekt sein,
das die Quellen für die gehaltene Erfassung liest.

Die Vorprüfung erfasst **den gesamten Batch vor dem ersten Schreiben**:

1. Aktuellen Frame, Auswahl, vollständige Kennungen und Besitzer erneut prüfen.
2. Sämtliche Vorherabbilder, Definitionen, Identitäten und Positionen mit dem Plan
   vergleichen; Nachherabbild zunächst ausschließlich privat berechnen.
3. Tatsächliche Speicherbereiche, Ausrichtung, Sockelanzahl, logische Grenze und
   Allokationskapazität prüfen. Zusatzvektoren und optionale Daten bleiben geschützt.
4. Überlappungen mit anderen Items, Sockeln, leeren/gefilterten Slots, gelesenen
   Strukturen, Besitzer-/Sperrdaten und zusätzlichen Itemdaten abweisen.
5. Schreibbarkeit aller geänderten Wörter prüfen und anschließend den gesamten
   Lesebestand samt Adressen und Quellbytes erneut vergleichen.

Erst danach schreibt der Adapter ausschließlich tatsächlich geänderte 16-Bit-Werte:
Hauptitem `+0x40`, Sockeldatensatz `+2`. Unveränderte und ungenutzte Werte werden
nicht geschrieben. Die eigentliche Schleife legt keine neuen C++-Puffer an.
Jeder Store vergleicht außerdem seinen bisherigen Wert. Ein vollständiges
Kontrolllesen prüft anschließend das erwartete Nachherabbild und den gesamten
ursprünglichen Lesebestand mit genau diesen erlaubten Änderungen, einschließlich
leerer Slots und struktureller Metadaten.

`LocalAccess` arbeitet nur im aufrufenden Prozess. Es gibt keine PID-Suche,
Remote-Zugriffe oder Änderung von Seitenschutzrechten. Schreibgeschützte,
ausführbare, Guard- und unzugängliche Seiten werden abgewiesen. OS-Speicherfehler
werden nur im begrenzten eigenen Lese-/Schreibrahmen behandelt.

## Ergebnis- und Lebensdauervertrag

- `rejected` aus der Vorprüfung bedeutet: Dieser Aufruf hat nichts geschrieben.
- `fields_written` bestätigt ausschließlich die geprüften Feldwerte. Die
  Besitzersperren und Referenzen bleiben für die folgenden Phasen gehalten.
- Sobald ein Store versucht wurde, ergibt dessen Fehler `outcome_unknown`, auch
  beim ersten Wort. Es gibt keine blinde Rücknahme oder automatische Wiederholung.
  Ein alter Vorherstand wird danach nicht mehr über `view()` als gültig ausgegeben.
- Jede gehaltene Erfassung erlaubt nur einen Schreibversuch. Späteres Kontrolllesen
  setzt diese Grenze nicht zurück. Falscher Thread und freigegebene Besitzer erlauben
  keine Speicheroperation. `already_attempted` ist kein Nachweis, dass ein früherer
  Aufruf nichts verändert hat.

Diese Ergebnisse dürfen nicht pauschal auf `action::Commit::applied` abgebildet
werden. Der vollständige Engine-Adapter muss native Nachherkopien und Ereigniswege
vor der Änderung vorbereiten, anschließend Slot-Markierungen sowie sämtliche
Inventar-/Ausrüstungsereignisse ausführen und eindeutige Rückmeldungen prüfen.
Ein Fehler nach Feldänderungen muss zum unklaren Gesamtauftrag führen.

## Ausgeführte Tests

**Acht CTest-Suiten**, MSVC Release `/W4 /WX`, bestanden. Die neue Schreibsuite
enthält **34 Szenarien und 197 Bedingungen**:

- Alle Bereiche/Einzelitem, aktive No-Wear-Regel, Sentinel, reine Sockeländerung,
  unveränderte Felder und leere Aufträge.
- Ungültige Sockel-/Zusatzkapazität, gemeinsame Item-/Sockelpuffer, Sockel innerhalb
  eines Items oder Actor-Headers und Überschneidung mit Zusatzdaten.
- Ein später veraltetes Item, unbeschreibbares letztes Feld, geänderte Sitzung,
  andere Spielerauswahl, ungültiger Plan und ein rein lesender Adapter.
- Werteänderung, Umzug einer bytegleichen Itemliste und Änderung eines leeren Slots
  während der Schreibbarkeitsprüfung; alles vor dem ersten Store abgewiesen.
- Fehler vor/nach einem tatsächlichen Store, Teilabbruch, vorgetäuschte erfolgreiche
  Stores, unerwartete Änderung eines leeren Slots und Auswahlwechsel beim Schreiben.
- Fremder Thread, Wiederholung und geschlossene Besitzphase; reale eigene Windows-
  Seiten mit Read-only-/Executable-Schutz, ungerade Adressen und Grenzüberlauf.
- Maximaler Implementierungsumfang: 2.048 vorhandene Items und 4.096 geänderte
  Wörter in beiden Zustandskopien. Dies ist keine zugesagte Inventar-/Enginegrenze.

**Acht zusätzliche native Integrationsfälle** verbinden den Produktionsschreiber
mit den originalen Referenz-/Sperrfunktionen des aktuellen Builds und tatsächlichen
privaten Speicheradressen. Normaler und registrierter Referenzmodus, beide
Zustandskopien, alle Bereiche und ein unabhängig erstellter Bytevergleich bestehen.
Die nativen Besitzersperren bleiben ohne Unterbrechung bis nach dem Kontrolllesen
gehalten. Native und synthetische Leseproben verwenden dieselben Quellnachweise.

Aktueller Gesamtstand: **141 native Szenarien, 1.408 gezählte Aufrufe und 120.069
Bedingungen**. Weiterhin 51 gepinnte Codebereiche, 57 pdata-Fragmente und 26 private
TLS-Lesestellen. Die große Bedingungszahl stammt überwiegend aus wiederholter
Prüfung der Slot-Tabelle aus 0.13.0, nicht aus ebenso vielen unabhängigen Fällen.

## Verbleibende Integration

Der neue Feldschreiber ist noch nicht mit der vollständigen Ereignisfolge zu einer
Engine-Transaktion verbunden. Die bisherigen 15 Ausrüstungsereignisfälle behalten
ihren getrennten privaten Feldaufbau. Nachherkopien müssen vor dem ersten
Quellschreiben vollständig vorbereitet werden. Für mitgeführte Items fehlen noch
die Ereigniswege, für Slot-Markierungen deren Verbraucher und native Bereinigung.
Echte Effekt-/Transport-/UI-Verarbeitung und Auftragszuordnung bleiben offen.

Ebenso fehlen bestätigter Spieler-/Manager-/Engine-Thread-/Weltzugriff, Eingabe,
Loader und B0-Installation. Der Host muss weiterhin Kontext-/Kataloglebensdauer,
Weltübergänge und TLS sichern; der Schreibadapter erzeugt diese Voraussetzungen
nicht selbst. Keine Schreibausführung im Spielprozess, keine Savezugriffe oder
Änderungen an der Spielinstallation. App-/Rust-/UI-Code ist unverändert und wurde
für diese C++-Änderung nicht erneut getestet.

Nachweise: `.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`.
[Quellcode](src/writer.cpp), [Windows-Zugriff](src/writer_win.cpp),
[Testfälle](tests/writer_tests.cpp), [native Integration](tests/writer_native.inl).
Vorgänger: [Slot-Markierung 0.13.0](DIRTY_SLOTS_2949.md).
Manuelle Spielabnahme bleibt wie vereinbart auf später verschoben:
[TESTCHECKLISTE](../../TESTCHECKLISTE.md), insbesondere R01–R12.
