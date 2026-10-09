# Speicherdispatcher und vorgemerkte Datensätze – Modul 0.20.0

Stand: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**39 neue native Szenarien auf privaten Objekten bestanden.** Keine Save-Dateien,
Steam-Schnittstellen oder laufenden Spielobjekte verwendet. Kein installierbarer Mod.
Die [Item-/Speicherobjekt-Konverter](ITEM_SAVE_2949.md) bleiben separat geprüft.

Historischer Nachweis 0.20.0. Die nachfolgende native Prüfung des Dateischreibhelfers
ist in [SAVE_FILE_2949.md](SAVE_FILE_2949.md) dokumentiert; echte Dateioperationen
und vollständiger Speicherabschluss bleiben offen.

## Native Funktionen und Bindungen

| Funktion | Originalweg | Prüfung |
|---|---|---|
| Speicherdispatcher | `0x23556b0 → 0xee09e40`, vollständiger Körper `0x2e3` | Aufrufreihenfolge, Zustandsbytes, Überspringen, Fehler und Wiederholung |
| Steam-Zulassungsprüfung | `0x235adb0 → 0xee1cd90`, `0x55` | Originalfunktion im Dispatcher, private Plattformabfragen |
| Vormerken eines Eintrags | `0x235b5d0 → 0xee2a210`, vollständiger Körper `0x13f` | Drei Warteschlangen, Wachstum, Duplikate, beide Allokationsmodi |

Sechs zusätzliche Codebereiche einschließlich der drei Sprünge. Der Marker besitzt
fünf pdata-Fragmente; die erste Grenze allein umfasst nur `0x2f` Bytes und ist
**keine vollständige Funktion**. Vier Unwindketten sowie der bytegleiche Alias
`0x62bd504 → 0x171512d8` werden geprüft. Eine neue TLS-Stelle wird privat ersetzt.
Gesamtmanifest: 88 Codebereiche, 115 pdata-Fragmente und 51 private TLS-Stellen.

## Tatsächlich geprüfte Bedingungen

Der Dispatcher erhält Kontext und Speicherzustand. Sein erster Backendaufruf
erhält denselben Speicherzustand in RDX; das ist für den Load-Zweig erforderlich.
Die weitere Reihenfolge ist `+0x48` mit Kontext `+0x18`, dann `+0x28` und `+0x30`
mit dem Speicherzustand. Die Empfänger für Laden, Schreiben, Löschen und Diagnose
sind eigene Testcallbacks; sie greifen auf keine Dateien zu.

- **16 Gate-/Statusfälle:** alle Kombinationen von `+0x698`, `+0x699`, `+0x69c`
  mit vorherigem Erfolgsbyte 0/1. `+0x69c != 0`, `+0x699 != 0` oder `+0x698 == 0`
  liefert true ohne Backendzugriff. Ein alter Erfolgswert bleibt dabei unverändert.
  Übersprungene Arbeit verträgt im Test sogar einen fehlenden Kontext.
- **8 Fehlerfälle:** Abbruch in jedem der vier Schritte, mit inline oder extern
  hinterlegtem Diagnosenamen. Nur der tatsächlich erreichte Präfix wird ausgeführt.
  Der Dispatcher setzt `+0x69b = 0`, erhält Dirty `+0x698` und meldet genau einmal.
- **8 native Steam-Prüfungen:** erfolgreiche Quota-Abfrage, verfügbare Bytes > 0
  und Dateianzahl < 10.000 erforderlich. 9.999/10.000, fehlende Kapazität,
  Abfragefehler sowie große Grenzwerte geprüft. Die Gesamt-Quota wird vom nativen
  Helper nicht zusätzlich gegen die verfügbaren Bytes verglichen. Plattformdaten
  und API-Objekt sind privat; keine Steam-Cloud-Abfrage erfolgt.
- **1 Fehler bei fehlendem Backend:** aktive Arbeit verursacht den erwarteten
  privaten Zugriffsfehler; Original-Stack-Unwind kehrt ohne Erfolgsmeldung zurück.
- **6 Vormerkfälle:** drei Queue-Arten in beiden TLS-Modi. Je neun Einträge
  erzwingen Wachstum; die letzte Adresse ist absichtlich ein Duplikat. Der Marker
  dedupliziert nicht und verändert nur Queue-Header und Dirty-Byte. Eintragsobjekte
  und andere Felder bleiben unverändert. Die Fixture gibt ihren Queue-Puffer frei.

Bei vier erfolgreich durchlaufenen Schritten setzt der Dispatcher `+0x69b = 1`
und löscht `+0x698`. Ein weiterer Aufruf überspringt den nun sauberen Zustand;
die beiden Erfolgsfälle prüfen das ausdrücklich. Diese Statusänderung bestätigt
nur den hier ausgeführten Ablauf mit Testempfängern, keinen Datenträgerabschluss.

Die drei Queue-Header liegen bei `+0x6a0`, `+0x6b0`, `+0x6c0`. Die folgende
Dateianalyse ordnet sie Laden, Schreiben und Löschen zu. Die Art wählt der
native Marker ungeprüft; die Probe verwendet ausschließlich 0/1/2.

## Separater statischer Dateinachweis

Diese Funktionen wurden **nur gelesen**, nicht nativ ausgeführt:

- Konstruktor `0x2354f50` setzt einen `GamePlaySaveDataFileHandler_Steam`
  (RTTI-VTable `0x5984070`) in Kontext `+0x10`. Dessen Plattformpointer `+0x148`
  ist zunächst null; seine spätere Initialisierung und Lebensdauer bleiben offen.
- VTable `+0x20` ist `0x2356ef0 → 0xee13560` (Load), `+0x28` ist `0x2357180`
  (Save), `+0x30` ist `0x2357c10` (Delete); `+0x48` ist der nativ geprüfte
  Steam-Helper. Die Basis-VTable `0x5983f90` verwendet dieselben Dateimethoden.
- Der Save-Weg ruft für Einträge der Schreibqueue `0x2358700` auf. Dieser bereitet
  einen Ausgabepuffer vor, prüft Öffnen (`0x12b7a90`) und Schreiben (`0x12b7dd0`),
  ruft dann den echten Import `FlushFileBuffers` auf (`IAT 0x51eb420`). Sein
  Rückgabewert wird vor Payload-Bereinigung und Erfolg **nicht geprüft**.
  `CloseHandle` folgt über `IAT 0x51eb480`; auch dieser Rückgabewert bestimmt den
  Erfolg hier nicht. Keiner dieser Datei-/Handle-Aufrufe wurde ausgeführt.
- Timer-Typen sind zugeordnet: `TrocTrFlushPendingSaveDataOnceTimer`
  (`0x5b34358`, Handler `0x2c35750 → 0x10665180`), `TrocTrSaveGameDataAutoTimer`
  (`0x5b34658`, `0x2c35e40`) und `TrocTrGamePlaySaveDataRepeatTimer`
  (`0x5b34fd0`, `0x2c3a4e0 → 0x10690e60`). Daraus folgt noch keine zulässige
  Aufrufzeit oder Threadbindung für einen Reparaturauftrag.

## Ergebnis und verbleibende Arbeit

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Gesamt **318 native
Szenarien, 4.039 Aufrufe und 141.411 Bedingungen**. Unveränderte Produktionssuiten:
35 Schreibszenarien/202 Bedingungen, 43 Transaktionsszenarien/273 Bedingungen.
Der native Test meldet `save_backend=fixture_callbacks`, `save_disk_commit=false`,
`engine_commit=false`, `live_installable=false`.

Als Nächstes den originalen Dateihelfer mit privaten Puffer-/Handle-Empfängern
prüfen, den Serialisierer und die zeitliche Verarbeitung anbinden sowie echte
Abschlussbelege zum konkreten Reparaturauftrag zuordnen. Serverseitige
Inventarmeldungen, vollständige Effekte/UI, Spielhost und Installation bleiben offen.
Die spätere Spielabnahme R06 bleibt erforderlich; keine simulierten Stromausfälle
oder absichtlichen beschädigten Saves vom Nutzer verlangt.

Quellen: [native Probe](tests/save_dispatch_native.inl),
[Manifest/Unwindregistrierung](tests/registry_native.cpp),
[Unwindprüfung](src/pe.cpp). Reproduzierbare Artefakte:
`.local/verify_save_backend_v20.py`, `.local/repair-runtime-v20-save-backend.json`,
`.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.
