# Reparatur 0.11.0 – Vorbereitung und Prüfung von Ausrüstungsereignissen

Historischer Ereignisnachweis. Die Probe verwendet inzwischen native Itemkopien:
[Entwicklungsmodul 0.12.0](ITEM_LIFECYCLE_2949.md).

Stand 22.09.2026. Entwicklungsmodul für Steam 25455892 / EXE 1.0.0.2949,
SHA-256 `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
Die App bleibt v0.5.9; kostenlose Reparatur ist weiterhin nicht freigeschaltet.

## Implementierung

`action::equipment_notice` gewinnt die Ereignisdaten aus dem unveränderlichen
Reparaturplan: Sitzung, genaue Item-/Slotidentität sowie Broken-Zustand vor und
nach der Änderung. Die Berechnung entspricht der originalen Ausrüstungslogik:
aktive Maximalhaltbarkeit ungleich `0xffff` und **vorzeichenbehaftete** aktuelle
Haltbarkeit <= 0. Der aktive No-Wear-Override wird dadurch berücksichtigt.
Ein beliebiger Nachherwert reicht nicht, um den vorherigen Zustand zu rekonstruieren.

Fehlende oder verschobene Items, nicht vorbereitete Pläne und mitgeführte Items
werden abgelehnt; alte Ausgabedaten werden gelöscht. Die Funktion liest keine
Spielpointer und erzeugt weder einen Ereignisbeleg noch eine Zugriffsfreigabe.
Auch `is_broken` wird ausgegeben: Eine reine Sockelreparatur an einem im Engine-Sinn
weiterhin kaputten Hauptitem darf nicht als Broken→Repaired-Wechsel behandelt werden.

## Native Verbindung im privaten Testhost

Der Test verbindet die eigene Planung und Warteschlange mit dem originalen
Server-Notifier und dem originalen Client-Ack. Vorhandene native Actor-Referenzen
bleiben während der gesamten Probe gehalten. Die Reparaturwerte werden ausschließlich
in eigene Testobjekte unter beiden nativen Besitzersperren kopiert. Vor den
Benachrichtigungen sind beide Sperren freigegeben; die Callbacks prüfen das erneut.

Beide Testrepräsentationen erhalten unabhängig ihre korrekten Item-/Sockelwerte.
Danach laufen die Benachrichtigungen mit **Delta 0 und leerer Entfernungsliste**.
Der positive Standard-Updater bleibt für Reparatur ungeeignet, weil er Sockel
verbrauchen kann. Delta 0 repariert selbst ebenfalls nichts.

| Originalfunktion | RVA | Bytes | Ausführung |
|---|---|---:|---|
| Server-Equipment-Notifier | `0x2adfda0` | 1.429 | Repaired-/unveränderte Zustände, Effektaufruf, optionaler Kind-Ereignisaufruf und Paketgrenze |
| Ereignisinitialisierung | `0x20efb50` | 177 | Originale Feldinitialisierung; Laufzeitkonstanten als private Fixtures |
| Client-Ack | `0x98fb70` | 773 | Delta, Sockellistenprüfung, Fehler und UI-Aufruf |
| Destruktiver Sockel-Collector | `0x240e3f0` | 465 | Drei geprüfte Unwind-Fragmente; entfernt vorhandene endliche Sockel bei Haltbarkeit <= 0 |

Die aktuelle Server-VTable `0x5b28f80`, Slot `+0x190`, verweist auf `0x2adfda0`.
Die vollständigen Funktionshashes stehen in `tests/registry_native.cpp`.
Das gesamte Manifest umfasst 28 Codebereiche, 25 pdata-Fragmente und acht
gezielte private TLS-Umleitungen. Originale C++-Cleanup-Handler werden nicht
registriert; Fixture-Abhängigkeiten werfen keine Exceptions. Die nativen
Repaired-Zweige benötigen aktiviertes AVX, das der Test vor Ausführung prüft.

## Ergebnisse und Fehlerfälle

Sechs CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Insgesamt **103 native
Szenarien, 758 gezählte Aufrufe und 9.311 Prüfbedingungen**; dazu 1.537 Aktions-,
385 Leser-, 25 Sperrgruppen- und 55 Quellenadapterbedingungen. Ohne EXE werden
nur 84 Manifestbedingungen geprüft, keine Originalfunktionen ausgeführt.

15 neue Integrationsfälle:

- Kaputtes Hauptitem, normal beschädigtes Item und reine Sockelreparatur.
- No-Wear und unbegrenztes Hauptitem; keine erfundenen Broken→Repaired-Ereignisse.
- Originaler Repaired-Zweig übergibt Zustandsart 3, richtigen Slot und Item an
  den Effektprozessor. Optionale Kind-Ereignisse enthalten passende Kennung und
  Fixture-Zeit; fehlendes Effektmerkmal überspringt den Prozessor.
- Ausbleibende Paketlieferung, falsche Entfernungsliste und fehlender Clientslot.
  Auch Fehlercode 0 bei fehlendem Slot erzeugt keinen Reparaturerfolg.
- Unreparierter Client trotz UI-Aufruf; ein noch auf 0 stehender Sockel wird
  bereits vor dem Listenfehler entfernt. Rückgabefehler bedeutet keine Rücknahme.
- Fehlender Ereignisbeleg, geänderte Sitzung und doppelte Lieferung. Nur passende
  Nachherbilder **und** vollständige Belege dürfen den Auftrag abschließen.

Nach Teilfehler oder Timeout bleibt die Warteschlange gesperrt, ohne Wiederholung
oder spekulative Rücksetzung. Mengen, Identitäten, Sockelbelegung und übrige
Itembytes werden für die erfolgreichen Fälle vollständig nachgeprüft.

## Präzise Nachweisgrenze

**Dies ist weiterhin keine vollständige Spieltransaktion.** Transport bei
`0x29ead70`, tatsächliche Effektverarbeitung bei `0x17b1d30`, Kind-Referenzliste,
Uhr, Tabellenresolver, UI-Empfänger und Speicherfreigabe sind Testabhängigkeiten.
Die Probe bestätigt die Aufrufentscheidung und Argumente dieser Schnittstellen,
nicht ihre Wirkung im Spiel, einen echten Paketversand oder Persistenz.
Auch die Callback-Zuordnung zu Auftragsnummern ist nur im privaten Test gegeben.

Offen bleiben insbesondere:

- Engine-Thread, Managerleben, aktuelle Spielerbindung und Weltwechsel.
- Echter Item-Schreibweg samt Engine-eigener Itemkopie/Lebensdauer; zwei rohe
  Speicheränderungen und die hier gezeigte Ereignisfolge reichen nicht als Freigabe.
- Slot-Invalidierung (`+0x198`), vollständige Effekt-/Kind-/Transportverarbeitung,
  noch kaputte Nachherzustände und Inventarereignisse für mitgeführte Items.
- Reale, eindeutig zugeordnete Bestätigungen; Eingabe, Loader und B0-Installation.

Es wurden keine Spielprozesse oder Saves geöffnet und keine Installationsdateien
geschrieben. Die Desktop-App wurde für diese Runtimeänderung nicht neu gebaut.

Reproduktion: `./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"`.
Lokale Nachweise: `.local/repair-runtime-v11-build-test.log`,
`.local/repair-runtime-v11-native-result.json`, `.local/repair-runtime-v11-validation.json`.
Spätere manuelle Fälle: [TESTCHECKLISTE](../../TESTCHECKLISTE.md), insbesondere R01–R10.
