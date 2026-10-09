# Besitzer-Sperren und geschützte Erfassung 0.5.0

Historischer Stand. Die direkte Erfassung gehaltener Besitzer und die gemeinsame
Probe mit den Sperrfunktionen des neuen Builds stehen in
[Modul 0.9.0](PINNED_CAPTURE_INTEGRATION.md).

Stand 22.09.2026. Separates Entwicklungsmodul; kein installierbarer Mod.
Die Workbench bleibt v0.5.8. Alle Ausführungsproben verwenden eigene Testobjekte.

## Implementiert

`crimson_repair_lease.lib` fasst bis zu zwei vom Host gebundene Sperren zusammen.
Es prüft alle Bindungen vor dem ersten Aufruf, sortiert nach Objektadresse und
versucht exklusiven Zugriff ohne Warten. Ist die zweite Sperre belegt, wird die
erste sofort freigegeben. Identische Bindungen werden einmal gehalten;
widersprüchliche Bindungen desselben Objekts werden abgewiesen. Freigabe erfolgt
in umgekehrter Reihenfolge und erhält eine schon vorhandene äußere Rekursion.

Die Gruppe ist weder kopierbar noch verschiebbar und gehört zu ihrem
Erstellungsthread. Öffentliche Aufrufe auf einem anderen Thread werden abgewiesen.
Eine noch gehaltene Gruppe auf einem fremden Thread zu zerstören ist ein
Programmierfehler und beendet den Host mit `std::terminate`; fremde Windows-
Sperren dürfen dort nicht freigegeben werden.

`capture_with_locks` verbindet dies mit dem bestehenden Inventarleser. Der Host
liefert bereits gegen Freigabe geschützte Client-/Server-Besitzer und geprüfte
Methodenbindungen. Der Leser vergleicht deren Sperrzeiger an `Actor+8` vor und
unter den Sperren, erfasst beide Inventare samt Kontrolllesen und prüft die
aktuell aufgelösten Besitzer erneut. Fehler verwerfen das private Ergebnis.
Beim Rücksprung sind beide Sperren frei: Das Ergebnis ist eine Datenkopie,
keine fortbestehende Schreibberechtigung.

## Nachweis am gepinnten Build

Steam 25381195, EXE 1.0.0.2944, SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Adressen sind RVAs. `WindowsRWLockBase`-VTable `0x5719fe0` und
`WindowsRWLock<2,0>`-VTable `0x55856e8`: Freigabe bei `+0x20`, Try bei `+0x28`.
`false` wählt exklusiven Zugriff, `true` gemeinsamen Lesezugriff. Die native
Struktur enthält SRWLOCK bei `+0x10`, Besitzer-Thread bei `+0x28`, Rekursion bei
`+0x2c`. Eine `NullRWLock`-Bindung wäre kein Nachweis gegenseitigen Ausschlusses.

| Funktion | Bytes | SHA-256 |
|---|---:|---|
| Try `0x1371c20` | 174 | `45f3b9499ce28442b126fd0941aa5711d1cc1a58e19a7d3893330d83ac430c90` |
| Release `0x1371bf0` | 40 | `b43c0aebed083eebd4ce7e824ed7acb44bec3f81d92a8ab52cd1be3981a5740a` |

Im privaten Try-Funktionsabbild wird ausschließlich der neun Byte große
TLS-Lesezugriff bei `0x1371c4f` auf private Daten je Testthread umgeleitet.
Alle übrigen Try-Bytes und der vollständige Release-Code bleiben bytegleich.
Der Testhost verändert weder TEB noch echte Engine-TLS-Daten. Die ursprünglichen
Importaufrufe verwenden echte Windows-SRW-Funktionen und `GetCurrentThreadId`
auf eigenen Lock-Objekten. Die Unwind-Registrierung entfernt den nicht nutzbaren
Spiel-Cleanup-Handler der privaten Kopie. Dies ist keine Ausnahmeprüfung eines
vollständig geladenen Spielmoduls.

Geprüft sind exklusiver Zugriff mit tatsächlichem Windows-Gegentest, Rekursion,
ein konkurrierender Thread mit Rücknahme der ersten Sperre sowie ein vorhandener
Leser, der den Schreibversuch ablehnt. Ältere Registry-/Ack-Proben verwenden
weiterhin ihre dokumentierten Lock-Stubs.

## Ergänzung seit 0.6.0

Die Referenzverwaltung und deren Einbindung in den Leser sind inzwischen
entwickelt. Geschützte Live-Quellen und konkrete Hostbindungen fehlen weiterhin.
[Aktueller Referenzvertrag](REFERENCE_INTEGRATION.md).

## Grenze des ursprünglichen Stands 0.5.0

Eine gehaltene Inventarsperre ersetzt keine gültige Actor-Referenz. Der getrennte
Actor-Zugriffspfad `0x1436bf0` prüft die vollständige Kennung und verwendet je nach
Engine-TLS direkte Referenzzähler oder eine Registrierung im Thread. Im normalen
Pfad kommen die eingebettete Sperre `Actor+0x18`, Zustandsflags, Alive-Feld und
Referenzregistrierung hinzu. Diese Lebensdauerverwaltung ist noch nicht in den
Host integriert. Ein Pointervergleich oder selbst erfundener Zähler genügt nicht.

Der künftige Host muss Lebensdauer, aktuelle Methodentabellen, initialisierten
Engine-Thread, Sitzung und Besitzereignung selbst belegen. Es fehlen außerdem
der vollständige Änderungsweg einschließlich Spielereignissen, deren Zuordnung
zum Auftrag, Bedieneingabe, Loader und B0-Installation. Die Bibliotheken liefern
keinen Live-Resolver für Lock-Bindungen und verändern keine Itemdaten im Spiel.

## Prüfung

Vier CTest-Suiten, MSVC Release `/W4 /WX`: bestanden. 1.481 Aktionsbedingungen,
163 Leserbedingungen, 25 Gruppenbedingungen; nativer Host insgesamt 140 Aufrufe
und 2.408 Bedingungen. Leserproben verhindern Itemzugriff ohne beide Sperren und
prüfen auch geänderte Besitzeranker, falsche Bindungen und Fehler beim Erfassen.
Nachweise: `.local/repair-runtime-v5-build-test.log` und
`.local/repair-runtime-v5-validation.json`. Kein Spielprozess-/Save-Zugriff.
