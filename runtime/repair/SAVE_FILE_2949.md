# Dateischreibhelfer auf privaten Puffern – Modul 0.21.0

Stand: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**34 zusätzliche native Szenarien bestanden.** Alle Dateioperationen einschließlich
Flush und Close sind eigene Callbacks; der Handle ist nur eine private Zahl.
Keine Save-Dateien oder laufenden Spielobjekte wurden verwendet.

Historischer Nachweis 0.21.0. Die native Pufferaufbereitung und Kompression sind
inzwischen separat in [SAVE_ENCODING_2949.md](SAVE_ENCODING_2949.md) geprüft.
Nachgelagerte Verarbeitung und tatsächliche Dateioperationen bleiben offen.

## Ausgeführter Originalcode

| Funktion | RVA / vollständige Länge | Ausführung |
|---|---|---|
| Einen vorgemerkten Eintrag schreiben | `0x2358700 / 0x2c9` | Vorbereitung, Öffnen, Schreiben, Flush, Bereinigung und Fehlerzweige |
| Bytepuffer leeren/freigeben | `0x1373a40 / 0x8e` | Ursprünglichen Payload nach Erfolg sowie verbliebene Testpuffer freigeben |
| Dateiobjekt-String freigeben | `0x394db0 / 0x8e` | Nur der tatsächlich vom Test verwendete leere Sentinelzweig |

Drei zusätzliche Funktionshashes und pdata-Einträge, vier private TLS-Ersetzungen.
Gesamtmanifest: **91 Codebereiche, 118 pdata-Fragmente, 55 TLS-Stellen**. Die
TLS-Stelle des Stringdestruktors und der Heap-Pfad für lange formatierte Namen
sind gebunden, aber in diesen Fällen nicht durchlaufen. Sprachspezifische
Exception-Cleanup-Handler des Spiels werden weiterhin nicht registriert.

Der Schreibhelfer erhält `(Handler, Verzeichnis, Eintrag)`. Der Handler verweist
bei `+0x10` auf seinen Besitzer; dessen String bei `+0x78` wird an den Encoder
weitergegeben. Eintragsname und dieser String können inline oder extern liegen.
Der vorgemerkte Bytepuffer liegt am Eintrag bei `+0x120`.

## Geprüfte Fälle

**26 Direktfälle:** 13 Varianten in beiden privaten TLS-Allokationsmodi:

- regulärer Ablauf mit inline Strings sowie mit beiden externen Strings;
- Vorbereitungsfehler ohne und mit bereits angelegtem temporärem Puffer;
- Öffnungsfehler, Schreibfehler und Öffnungsfehler mit simuliertem Resthandle;
- fehlgeschlagener Flush, fehlgeschlagenes Close und beide zusammen;
- leere Ausgabe ohne Allokation sowie leere Ausgabe mit reserviertem Puffer;
- 4.096 Byte Nutzdaten, bytegenau an den Schreibempfänger übergeben.

**Acht Integrationsfälle:** Erfolg, Öffnungsfehler, Schreibfehler und Flushfehler
in beiden Allokationsmodi mit dem originalen
[Speicherdispatcher](SAVE_BACKEND_2949.md). Eine eigene Brücke übergibt genau
einen Eintrag. Sie ersetzt noch den originalen Queue-/Backup-/Umbenennungsweg.

Geprüft werden Argumente, Aufrufreihenfolge, Ergebnisse, unveränderte Besitzer,
Pufferinhalt und Schutzbytes sowie vollständige Freigabe. Der originale Clear
setzt Zeiger, Kapazität und Länge zurück, lässt das Allokatortag `0xff` erhalten.
Die vorbereitende Testfunktion verändert den Eingabepuffer nicht. Der Nachweis,
dass er bei Öffnungs-/Schreibfehlern erhalten bleibt, gilt für **diese Bindung**;
der echte Encoder kann den Eingabepuffer vorher ändern, siehe unten.

## Warum Erfolg noch keine Speicherung bestätigt

Bei erfolgreichem Öffnen und Schreiben beziehungsweise leerer Ausgabe:

1. Flush wird aufgerufen; sein Ergebnis beeinflusst den Ablauf nicht.
2. Der originale Clear gibt den vorgemerkten Payload frei.
3. Close wird aufgerufen; sein Ergebnis beeinflusst den Erfolg ebenfalls nicht.
4. Der Helfer gibt true zurück. Der verbundene Dispatcher setzt seinen
   Erfolgsstatus und löscht Dirty auch beim simulierten Flushfehler.

Bei Öffnungs-/Schreibfehlern liefert der Helfer false und ruft keinen Flush auf.
Der Dispatcher erhält dann Dirty und löscht einen vorherigen Erfolgsstatus.
Temporäre Ausgabeallokationen werden auch nach Vorbereitungsfehlern freigegeben.

Das sind Aussagen über Originalcode mit privaten Empfängern. Es wurde weder
ein echter Schreibvorgang noch ein Datenträgerabschluss bestätigt. Der produktive
Reparaturadapter erhält dadurch keine Freigabe, diese Bool-Rückgabe als
korrelierte Erfolgsmeldung zu verwenden.

## Nächste Bindungen, bisher nur statisch gelesen

`0x235c750 / 0x31a` bereitet den Dateipuffer vor. Ein Flag bei Eintrag `+0xa4`
entscheidet über einen zusätzlichen Verarbeitungsschritt. Bei gesetzter Maske `0x02`
ruft der Code `0x12c61e0` auf und ersetzt anschließend den ursprünglichen Payload;
`0x235c040` ist ein weiterer Verarbeitungsschritt. Erst danach werden Längen
bei `+0xaa`/`+0xae` gesetzt und ein 128-Byte-Header plus Nutzdaten ausgegeben.
Die genaue Kodierung und deren Fehler-/Besitzsemantik müssen noch nativ geprüft
werden. Insbesondere ist ein Fehler vor dem Dateiöffnen kein allgemeiner Beleg
für unveränderte vorgemerkte Nutzdaten.

`0x12b7dd0 / 0xe3` ist der nachgelagerte Schreibwrapper. Nach erfolgreichem
`WriteFile` vergleicht er die tatsächlich geschriebene mit der angeforderten
Bytezahl; eine Teilmenge wird als Fehler behandelt. Auch dieser Wrapper und
seine Benachrichtigung sind bisher nur statisch gelesen.

Weitere offene Punkte: vollständige Serialisierung aus ItemSaveData, ursprüngliche
Queue-/Backup-/Umbenennungslogik, gültiger Zeitpunkt und Thread, belastbarer
Abschlussbeleg zum Reparaturauftrag sowie Spielhost und Installation.

## Ergebnis und spätere Spieltests

MSVC Release `/W4 /WX` und **neun CTest-Suiten** bestanden. Insgesamt **352 native
Szenarien, 4.095 Aufrufe und 142.649 Bedingungen**. Produktionssuiten unverändert:
35 Schreibszenarien/202 Bedingungen, 43 Transaktionsszenarien/273 Bedingungen.
Das Testergebnis meldet `save_file_io=private_callbacks`, `save_disk_commit=false`,
`engine_commit=false`, `live_installable=false`.

Später gelten [R06 und R14](../../TESTCHECKLISTE.md): reparierte Haupt-/Sockelwerte
und Färbungen vor/nach Reparatur sowie nach regulärem Speichern und normalem
Spielneustart vergleichen. Technische Fehlerfälle werden hier im Testprogramm
simuliert; der Nutzer soll keine Stromausfälle oder Save-Beschädigungen erzeugen.
Die Reparatur bleibt in App v0.5.9 gesperrt.

Quellen: [native Probe](tests/save_file_native.inl),
[Funktionsmanifest](tests/registry_native.cpp), [Unwindprüfung](src/pe.cpp).
Nachweise: `.local/verify_save_file_v21.py`, `.local/repair-runtime-v21-save-file.json`,
`.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.
