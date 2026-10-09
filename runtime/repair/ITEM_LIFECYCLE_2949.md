# Reparatur 0.12.0 – native Itemkopien für Ausrüstungsereignisse

Historischer Entwicklungsstand; ergänzt durch
[Slot-Markierung und gehaltene Erfassung 0.13.0](DIRTY_SLOTS_2949.md).

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. Vollständiger EXE-Hash:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
Weiterhin ein Entwicklungsmodul; App v0.5.9 und gesperrte Reparaturoption unverändert.

## Neu implementiert

`crimson_repair_item.lib` besitzt native Itemwerte über `item::Value`. Konstruktion,
Zuweisung und Freigabe sind je Objekt fest gebunden. Kopieren und Verschieben des
Besitzers sind ausgeschlossen. Zugriff und Operationen auf anderen Threads werden
abgewiesen; eine Zerstörung auf dem falschen Thread beendet den Prozess gemäß dem
gleichen Vertrag wie die bestehende Referenzverwaltung.

Vor einer Zuweisung werden Null-/Fehlzeiger, Überlappung, Ausrichtung sowie die drei
Vektorheader geprüft. Ein ungültiger Header darf den vorhandenen Zielwert nicht
erst löschen. Bei einer unerwarteten nativen Rückgabe wird der Wert unbenutzbar;
er kann erst nach Freigabe neu konstruiert werden. Aufräumen bleibt gewährleistet,
soweit die gebundenen nativen Funktionen ihren dokumentierten Vertrag erfüllen.

Die Bindung muss später aus einem verifizierten, während der Nutzung geladenen
Spielmodul kommen. Der Host muss auf dem korrekten Engine-Thread mit gültigem TLS
laufen und den Quellwert samt allen verschachtelten Daten schützen. Die Bibliothek
erteilt diese Freigaben nicht. `action::Image` ist ein abgetrenntes Abbild und darf
mit seinen gespeicherten Pointerbytes nicht als native Quelle übergeben werden.

## Belegtes Kopierlayout

Die tatsächliche Zuweisung ist kein vollständiges `memcpy`: Sie löscht vorherige
optionale Daten und kopiert relevante Felder sowie verschachtelte Speicherbereiche
über eigene Helfer. Paddingbytes werden teilweise nicht kopiert.

| Bereich | Header / Größe | Verhalten |
|---|---|---|
| Sockeldaten | Pointer `+0x60`, Anzahl `+0x68`, Speicher-Kapazität `+0x6c`; 6 Bytes pro Eintrag | Eigene Allokation; Anzahl und Inhalte werden kopiert |
| Logische Sockelgrenze | Byte `+0x70` | Separat von Speicher-Kapazität; wird ebenfalls kopiert |
| Zusätzliche Liste | Pointer `+0x78`, Anzahl `+0x80`, Kapazität `+0x84`; Stride 16 | 13 belegte Inhaltsbytes pro Eintrag, restliche Bytes Padding |
| Weitere Liste | Pointer `+0xa8`, Anzahl `+0xb0`, Kapazität `+0xb4`; Stride 6 | Eigene Allokation und Inhaltskopie |
| Optionaler Block | Pointer `+0xb8`, 24 Bytes | Neu kopiert, vorheriger Block freigegeben |
| Optionaler Block | Pointer `+0xc0`, 16 Bytes | Neu kopiert, vorheriger Block freigegeben |

Die fachliche Bedeutung der beiden zusätzlichen Listen/Blöcke wird aus ihrer
Kopiergröße nicht abgeleitet. Die neue Kapazitätsprüfung gehört zur nativen
Kopierfreigabe; der bestehende Leser wurde in diesem Schritt nicht geändert.

Native Einstiege: Konstruktor `0x2409750`, Zuweisung `0x240b020` → `0xf300c80`,
Destruktor `0x240af10` → `0xf2fe4a0`, Leeren `0x240b430` → `0xf312630`.
Alle 17 neuen Codebereiche einschließlich der vollständigen Listen-/Allokations-
helfer sind mit Größen und Hashes in `tests/item_functions.inl` gepinnt.

## Integration und Prüfung

Die 15 Ausrüstungsereignistests verwenden jetzt diesen Besitzer mit originaler
Konstruktion, tiefer Zuweisung und Destruktion. Die Nachherkopie entsteht noch unter
den Besitzersperren. Nach deren Freigabe erhält der originale Server-Notifier die
unabhängige Kopie. Erst nach allen Callbacks wird sie freigegeben; Actor-Referenzen
bleiben bis dahin gehalten.

14 zusätzliche native Kopierfälle prüfen beide TLS-Allokationswege: einfache Items,
Sockel, alle zusätzlichen Datenbereiche, erneutes Kopieren mit größerer Anzahl,
Ersetzen durch leere Daten, Selbstzuweisung und ungültige Quellkapazität. Quelle
und Quellspeicher bleiben unverändert. Eigener Windows-Heap mit Schutzmarkierungen
prüft Allokationsgrenzen, unabhängige Pointer und vollständig ausgeglichene Freigaben.

**Sieben CTest-Suiten, MSVC Release `/W4 /WX`, 30 neue Besitzertestbedingungen und
117 native Szenarien mit 851 gezählten Aufrufen sowie 13.639 Bedingungen bestanden.**
Die übrigen Suiten: 1.537 Aktions-, 385 Leser-, 25 Sperrgruppen- und 55 Quellenbedingungen.
Der Test ohne EXE prüft nur 270 Manifestbedingungen.

Insgesamt 45 Codebereiche, 49 pdata-Fragmente und 24 private TLS-Umleitungen.
Bei einem verschobenen Listenhelfer zeigt die Unwind-Kette auf ein byteidentisches
älteres Prologrecord. Ausschließlich dieser gepinnte Alias wird auf das registrierte
Record normalisiert; auch verschachtelte Ketten sind explizit geprüft.
Originale C++-Cleanup-Handler werden weiterhin nicht registriert. Allokationsfehler
und Exceptions in fremden Funktionen sind damit nicht als beherrschbar nachgewiesen.

## Noch offen

Die native Itemkopie ist jetzt implementiert und isoliert geprüft. Sie ersetzt
weiterhin keinen vollständigen Reparaturcommit: Spielhost/Manager/Threadbindung,
eigentliche Itemänderung, Slot-Invalidierung, Inventarereignisse, echte Effekt-
und Transportverarbeitung, Auftragszuordnung, Eingabe, Loader und B0 fehlen.
Der private Heap ist kein nachgewiesener Engine-Allocator. Slot-Invalidierung
wurde weiter statisch eingegrenzt, aber noch nicht implementiert oder ausgeführt.

Keine Spielprozess-/Save-Zugriffe oder Änderungen der Installation. Reproduktion:
`./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"`.
Nachweise: `.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`.
[Phasenstatus](../../PHASENSTATUS.md), [spätere Tests R01–R11](../../TESTCHECKLISTE.md).
