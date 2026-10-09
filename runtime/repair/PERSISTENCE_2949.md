# Reparatur 0.16.0 – Slot-Verarbeitung und Abschluss des Batches

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**Weiterhin ein Entwicklungsmodul, keine installierte Reparaturfunktion.**
Die App bleibt v0.5.9. Spielprozess, Spielinstallation und Saves werden nicht verändert.

## Implementierter Abschluss

**Präzisierung aus 0.18.0:** Die hier als Persistenzschnittstelle bezeichnete
SQL-Ausführungsfunktion kann Erfolg liefern, ohne den Request zu lesen, und
führt selbst keinen Datenbankauftrag aus. Die nachgelagerte Verarbeitung und
Speicher-/Ladesemantik sind weiterhin offen. [Vollständiger Nachweis](SQL_DISPATCH_2949.md).

Der gemeinsame Transaktionsadapter verlangt jetzt `Environment::finish_unlocked`.
Fehlt diese Bindung, wird der ganze Auftrag vor Kopieren und Schreiben abgewiesen.
Der Aufruf erfolgt genau einmal nach allen Itemmeldungen, mit freigegebenen
Besitzersperren und noch gültigen Referenzen/Kopien. Der Host kann darin die
gesammelten Änderungen weiterverarbeiten. Erst danach wird `pending` zurückgegeben.

Ein Fehler oder Sitzungswechsel in diesem Schritt bedeutet `unknown`, weil
Quellwerte und Meldungen bereits geändert bzw. versendet sein können. Es gibt
keine automatische Wiederholung. `Report::finalized` bestätigt ausschließlich
den erfolgreichen Abschluss dieses gebundenen Aufrufs, keine dauerhafte Speicherung.
Die Warteschlange verlangt weiterhin frisch gelesene Werte und zugeordnete Belege.

In der nativen Ausrüstungsprobe ist dieser Schritt mit dem originalen Verbraucher
der Slot-Tabelle verbunden. Die vorher künstliche Tabellenfreigabe wurde durch
die originalen Clear-/Destruktorfunktionen ersetzt. Das wird ausschließlich an
privat angelegten Testkomponenten ausgeführt; die Workbench besitzt keine
Berechtigung, eine echte Spielkomponente eigenständig zu zerstören.

## Belegtes Verhalten des aktuellen Builds

| Funktion | RVA | Vollständige Länge |
|---|---|---|
| Server-Ausrüstung, vtable `+0x130`: Verbraucher | `0x2ad1d70` | `0x6db` |
| Hash-Iterator für Slot-Einträge | `0x411cd0` | `0xb6` |
| Wiederverwendbares Leeren der Tabelle | `0x40b7b0` | `0xdf` |
| Endgültige Freigabe der Tabelle | `0x410590` | `0xd0` |

Vollständige Hashes: [Manifest](tests/dirty_functions.inl). Die Clear-/Destruktor-
funktionen besitzen je fünf separat geprüfte pdata-Fragmente. Keine gekürzten
Funktionskopien. Gesamt: 55 Codebereiche, 69 pdata-Fragmente, 32 private TLS-Stellen.
Die Erweiterung der privaten TLS-Daten erlaubt auch den Kontextzugriff bei `+0x250`.

Der Verbraucher erwirbt die Besitzersperre, läuft über markierte Slots und sucht
deren aktuelle Ausrüstungsitems. Fehlende Slots, ungültige Itemkeys und Menge 0
werden ausgelassen. Er sammelt UID und 16-Bit-Haupt-Haltbarkeit, leert die
Slot-Tabelle und löst die Sperre. Danach übergibt er den gesammelten Auftrag an
die Persistenzschnittstelle. Bei Haupt-Haltbarkeit 0 enthält der Auftrag ein
entsprechendes Kennzeichen; nach erfolgreicher Verarbeitung folgt ein zusätzlicher
Zähler-/Meldungspfad. Diese Entscheidung ist vom vorherigen Broken-Zustand des
separaten Reparaturereignisses zu unterscheiden.

**Die Liste ist bereits leer, bevor die Persistenzschnittstelle antwortet.**
Abgelehnte Kapazität, positiver Ausführungsfehler und gespeicherter negativer
Fehlercode sind geprüft. Ein erneuter Verbraucheraufruf findet danach keine
Einträge mehr. Eine leere Liste oder erfolgreiche UI-Aktualisierung darf daher
keinen Reparaturerfolg bestätigen.

Clear gibt alle Slot-Payloads frei, behält aber das Node-Array zur Wiederverwendung.
Erneutes Clear auf leerer Liste ist wirkungslos. Neue Markierungen funktionieren
mit zurückgesetzten Seriennummern. Der Destruktor gibt schließlich auch das Array
frei und setzt die Tabelle auf den leeren Ausgangszustand.

## Prüfung

Neun CTest-Suiten, MSVC Release `/W4 /WX`, bestanden:

- Transaktionsadapter: **43 Szenarien, 273 Bedingungen**, einschließlich fehlendem
  Abschlussadapter, Abschlussfehler und Sitzungswechsel nach Itemmeldungen.
- Native Slot-Tabellenfälle: Clear, mehrfaches Clear, Wiederverwendung und originale
  Endfreigabe zu den bisherigen Kollisionen, Duplikaten und Wachstumsfällen ergänzt.
- **24 neue native Verbraucherfälle**, beide Allokationswege: leere Liste,
  einzelne/doppelte Markierung, fehlender Slot, ungültiges/verbrauchtes Item,
  Hauptwert 0 bzw. 65.535, 64 Slots, Kapazitäts- und Ausführungsfehler. Exakte
  UID-/Haltbarkeitspaare, unveränderte Items und vollständige Freigabe geprüft.
- **18 integrierte Ausrüstungsfälle**: Feldschritt, native Nachherkopien, Markierung,
  Server-/Client-Ereignisse und Verbraucher im gemeinsamen Adapter. Drei neue Fälle
  bestätigen, dass Persistenzfehler auch nach erfolgreichem Client-Ack den Auftrag
  dauerhaft als unklar sperren. Kein Replay und kein Erfolg allein aus reparierten Feldern.
- Gesamt **186 native Szenarien, 2.826 gezählte Aufrufe, 124.527 Bedingungen**.
  Viele Bedingungen sind wiederholte Tabellenprüfungen, keine unabhängigen Szenarien.

## Verbleibende Arbeit

Die Persistenzschnittstelle ist im Test ein eigener begrenzter Empfänger. Es
werden keine Datenbank und keine Saves geöffnet. Tatsächliche Speicherung, ihre
Lebensdauer, Fehlerbehandlung und eindeutige Auftragsrückmeldung sind noch offen.
Der hier belegte Verbraucher übergibt **keine Sockeldatensätze**. Für die vollständige
Sockelreparatur fehlt daher weiterhin der Nachweis des ergänzenden Persistenzwegs.

Konkrete Inventarmeldungen für mitgeführte Items, echte Effekt-/Transport-/UI-
Verarbeitung, verifizierter Spielhost/Engine-Thread/TLS/Weltwechsel, Eingabe,
Loader und B0-Einbau bleiben offen. Die vorhandenen Ergebnisse sind kein
bestätigter Live-Engine-Commit. Manuelle Tests bleiben wie vereinbart auf später
verschoben; insbesondere Reparatur, normales Spielende und Neuladen aus R06.

[Quellcode des Adapters](src/transaction.cpp),
[Verbraucherproben](tests/dirty_consumer_native.inl),
[integrierter Ausrüstungsablauf](tests/equipment_events_native.inl),
[Testcheckliste](../../TESTCHECKLISTE.md).
Nachweise: `.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`.
Recherche: `.local/repair-runtime-v16-{components,dirty-candidates,callers}.json`.
Vorgänger: [vorbereitete Kopien 0.15.0](TRANSACTION_2949.md).
