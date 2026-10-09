# Native Speicherpuffer und Kompression – Modul 0.22.0

Stand: 23.09.2026. Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**66 zusätzliche native Szenarien bestanden.** Originale Pufferaufbereitung,
Allokation und Kompression laufen mit privaten Einträgen. Der nachgelagerte
Verarbeitungsschritt ist weiterhin ein eigener Callback. Dateioperationen
bleiben simuliert; keine Save-Dateien oder laufenden Spielobjekte verwendet.

## Neue Originalfunktionen

| Funktion | RVA / vollständige Länge |
|---|---|
| Speicherpuffer vorbereiten | `0x235c750 / 0x31a` |
| Bytepuffer dimensionieren | `0x1326490 / 0x142` |
| Allokationswrapper | `0x13737c0 / 0x69` |
| Kompressionswrapper | `0x12c61e0 / 0x35` |
| Kompressionskörper | `0x12c4fe0 / 0x11fd` |
| Kompressionszustand initialisieren | `0x12c6220 / 0x31` |
| Windows-Stackprobe | `0x4896270 / 0x4e` |
| 16-Bit-Offset schreiben | `0x12c4fc0 / 0x4` |
| Übereinstimmende Bytes zählen | `0x12c4fd0 / 0x9` |

Neun weitere Funktionshashes, sieben pdata-Einträge und drei private TLS-
Ersetzungen. Gesamt: **100 Codebereiche, 125 pdata-Fragmente und 58 ersetzte
TLS-Stellen**. Die Stackprobe liest unverändert `GS:[0x10]`, also das Stacklimit
des eigenen Windows-Testthreads. Sie liest keine Spiel-TLS.

Die benachbarten sechs Byte langen CRT-Sprungstellen für memcpy/memset bekommen
je eine sechs Byte lange private Umleitung mit separatem Funktionszeiger.
Ein längerer Trampolin würde die nächste Sprungstelle überschreiben. Alle
Codeseiten bleiben RX, alle verwendeten Zeigerseiten R; keine RWX-Seiten.

## Geprüfte Inhalte und Grenzen

- **40 Größen-/Kompressionsfälle:** 0, 1, 12, 13, 255, 256, 4.096, 65.546,
  65.547 und 131.072 Byte, jeweils mit/ohne Kompression und in beiden TLS-Modi.
  Wiederholungen und deterministische Zufallsbytes; beide nativen Zweige um
  die Grenze `0x1000b` werden erreicht.
- **Vier Wiederverwendungsfälle:** ausreichende Ausgabeallokation behalten und
  zu kleine Ausgabeallokation ersetzen; Inhalt, Länge, Kapazität und Freigabe geprüft.
- **Sechs Fehlerfälle:** Kompressionsfehler mit noch unveränderten Rohdaten;
  nachgelagerter Fehler mit/ohne vorherige Kompression. Der Ausgabedeskriptor bleibt
  unverändert. Bereits erfolgreich komprimierter Eingabepuffer bleibt komprimiert.
- **14 Integrationen mit dem Original-Dateihelfer:** normal mit/ohne Kompression,
  Öffnungs-, Schreib-, Flush-, Kompressions- und nachgelagerter Verarbeitungsfehler
  in beiden TLS-Modi. Tatsächliche Dateioperationen sind eigene Callbacks.
- **Zwei Wiederholungsfälle:** denselben privaten Eintrag nach Öffnungsfehler erneut
  aufrufen. Der zweite Durchlauf komprimiert den bereits komprimierten Inhalt
  erneut und trägt dessen Länge als neue ursprüngliche Länge ein.

Komprimierte Bytes werden unabhängig mit einem kleinen, begrenzten Testdecoder
zurück in die ursprüngliche Bytefolge übersetzt. Der Decoder verwendet die
[offizielle LZ4-Blockbeschreibung](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md);
er ist keine Kopie des nativen Kompressors und kein produktiver Save-Reader.
Die Fixture begrenzt ihre Allokationen auf 262.144 Byte. Daraus folgt keine
allgemeine Spiel- oder Savegrößengrenze.

Der Encoder setzt ursprüngliche und verarbeitete Länge bei Eintrag `+0xaa` und
`+0xae`. Die Ausgabe besteht aus den 128 Bytes ab `+0x98`, gefolgt von Nutzdaten;
der Puffer besitzt zwei zusätzliche Nullbytes außerhalb der Nutzlänge.
Identität, übrige Headerbytes und Schutzbereiche bleiben erhalten. Beide
Allokationsmodi geben alle temporären und verbliebenen Puffer vollständig frei.

## Bedeutung für Reparaturen

Die ursprünglichen Rohdaten können schon vor dem Öffnen einer Datei durch ihre
komprimierte Darstellung ersetzt werden. Ein Öffnungsfehler lässt diesen Zustand
stehen. Wiederholung desselben Eintrags ist daher kein neutraler Wiederholversuch:
der Test bestätigt doppelte Kompression und eine geänderte Größenangabe.
Ein neuer Reparatur-/Speicherversuch muss seine Rohdaten neu erzeugen oder den
vollständig belegten Wiederholweg der Engine benutzen. Es wird keine automatische
Wiederholung in den produktiven Reparaturadapter eingeführt.

Der bereits geprüfte [Dateihelfer](SAVE_FILE_2949.md) kann weiterhin nach einem
fehlgeschlagenen Flush Erfolg melden und den Payload freigeben. Eine boolesche
Rückgabe oder ein geleerter Puffer allein bestätigt deshalb weder den konkreten
Reparaturauftrag noch einen Datenträgerabschluss.

## Noch offen

`0x235c040 → 0xee36650` wird durch einen kontrollierten, normalerweise unverändernden
Callback ersetzt. Sein Originalcode führt über verteilte und indirekte Sprünge.
Die 44 bisher betrachteten Zielanfänge sind **unvollständige Blockfenster**, keine
zusätzlichen geprüften oder ausführbaren Funktionskörper. Verschlüsselung,
Authentifizierung und ihre Schlüssel-/Nonce-Verwaltung sind nicht nachgewiesen.

Weitere Grenzen: Allokationsfehler, Allokatortags außer `0xff`, vollständige
Objektserialisierung, originaler Queue-/Backup-/Umbenennungsweg, Spielhost,
Zeitpunkt/Thread, echte Ereignisempfänger und eindeutige Abschlussrückmeldung.
Die Kompressions-Fehlerprobe erzwingt den Fehler durch einen Callback; sie behauptet
keinen solchen Fehler bei korrekt dimensionierter nativer Kompression.

## Ergebnis

MSVC Release `/W4 /WX`, neun CTest-Suiten und insgesamt **418 native Szenarien,
4.289 Aufrufe und 204.378 Bedingungen** bestanden. Viele Bedingungen sind
wiederholte Byte-/Manifestprüfungen. Die unveränderten Produktionssuiten melden
35 Schreibszenarien/202 Bedingungen und 43 Transaktionsszenarien/273 Bedingungen.
`save_postprocess=fixture_callback`, `save_compression=native_lz4`,
`save_disk_commit=false`, `engine_commit=false`, `live_installable=false`.

Die spätere Spielabnahme bleibt [R06/R09](../../TESTCHECKLISTE.md); technische
Fehler und Wiederholungen werden vorab auf eigenen Daten simuliert. Phase 5
bleibt offen, App v0.5.9 und Spielinstallation bleiben unverändert.

Quellen: [native Probe](tests/save_encode_native.inl),
[Funktionsmanifest](tests/encode_functions.inl), [Testarena](tests/registry_native.cpp).
Nachweise: `.local/inspect_encoder_v22.py`,
`.local/repair-runtime-v22-encoder-dependencies.json`,
`.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.
