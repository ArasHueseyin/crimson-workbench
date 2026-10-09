# Reparatur: Bestätigung und Benachrichtigung 0.4.0

Stand 22.09.2026. Die Aktionssteuerung unterstützt jetzt eine asynchrone
Bestätigung. Sie stellt **keine installierte Spielanbindung** dar. Der native
Client-Rückmeldepfad ist ausschließlich im eigenen Testhost untersucht.

## Entwickelter Abschlussweg

`Transaction::commit` kann zusätzlich `pending` zurückgeben. `Queue::execute`
meldet dann `awaiting_confirmation`, ohne einen reparierten Gegenstand zu zählen.
Der Plan bleibt bis zur Bestätigung erhalten. Erneute Eingaben, Abbruch und eine
zweite Ausführung sind währenddessen gesperrt; es gibt keine Wiederholung.
Der Engine-Host darf eine Sperre nicht über diese Wartezeit hinweg halten.

`Queue::confirm` verlangt Auftragsnummer, ursprüngliche Sitzung der Rückmeldung,
eine frisch erfasste Sitzung samt Itemabbildern und die ausdrückliche Bestätigung
aller erforderlichen Engine-Ereignisse. Die Sitzung umfasst Weltgeneration,
Charakter und Katalogrevision. Eine fremde/alte Rückmeldung kann auch mit derselben
numerischen Auftragsnummer keinen neuen Auftrag abschließen.

`verify_applied` prüft sämtliche geplanten Nachherwerte beider Repräsentationen
einschließlich IDs, Positionen, Mengen, Sockeln und übrigen Itembytes. Die Reihenfolge
der Items im Erfassungsarray ist unerheblich. Erst passende Daten **und** bestätigte
Benachrichtigungen führen zu `applied` und einer Erfolgsmengenzählung. Die Prüfung
liest ausschließlich eigene Abbilder und schreibt keine Reparaturwerte.

Fehler, Teiländerung, fehlendes Item, geänderte Sitzung/Definition oder eine
Bestätigungsfrist von fünf Sekunden führen nach Annahme zu `outcome_unknown`.
Die Warteschlange sperrt sich dauerhaft. Keine spekulative Rücksetzung und kein
automatischer Wiederholungsversuch. Ein späteres Erfolgssignal hebt die Sperre
nicht auf. Der Host muss den tatsächlichen Zustand neu erfassen und eine neue
geprüfte Sitzung beginnen, bevor er eine neue Warteschlange anlegt.

`Queue::poll` muss auf jedem Dispatch-Tick aufgerufen werden, auch beim Laden
oder in Menüs. Vor Beginn einer Transaktion kann ein alter/abgelaufener Auftrag
noch ohne Nebenwirkungen verworfen werden. Nach Annahme wird ein Weltwechsel
oder Timeout als unklarer Ausgang behandelt. Bestätigung und Polling dürfen nur
auf dem festgelegten Thread laufen. Während `commit` eintreffende Rückmeldungen
muss der Host bis nach der Rückkehr aus `execute` zurückstellen.

Synchrones `Commit::applied` behält seinen bisherigen Vertrag: Der Adapter muss
bereits sämtliche Änderungen und Ereignisse bestätigt haben. Das neue `pending`
ist kein Erfolg und keine Freigabe, einen Paketversand als abgeschlossene Reparatur
auszugeben. Die Zuordnung realer Spielereignisse zu diesen Aufträgen fehlt weiterhin.

## Befunde aus der gepinnten EXE

Steam 25381195 / EXE 1.0.0.2944, SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Alle Adressen sind RVAs; nur Dateien auf dem Datenträger wurden gelesen.

| Pfad | Befund |
|---|---|
| Gemeinsamer Equipment-Wrapper `0x20c90e0` | Sperrbereich, Updater, Itemkopie und virtuelle Slotaktualisierung; danach Freigabe und Ereignismethode mit Item, Slottag, Delta, vorherigem Broken-Zustand und Liste entfernter Sockel. |
| Server-VTable `0x5b28f80`, Slot `+0x190` → `0x2adfd90` | Behandelt Broken-/Repaired-Übergänge und weitere Effekte, verarbeitet entfernte Sockel und ruft den Nachrichtenweg `0x29ead60` auf. Nur statisch untersucht, nicht vollständig emuliert oder angebunden. |
| Server-VTable Slot `+0x198` → `0x2ae0640` | Gibt den Slottag an den Pfad bei `0x20cd610` weiter. Dies allein ist keine vollständige Reparaturbenachrichtigung. |
| Client-Ack `0x98fb70` | Wendet zuerst das Delta an, entfernt danach verbrauchte Sockel und vergleicht erst dann die Liste mit der Servermeldung. UI-Aufrufe folgen erfolgreichem Vergleich. |
| Sockelfunktion `0x240e3e0` | **Kein reiner Leser:** Sammelt vorhandene endliche Sockel mit Haltbarkeit <= 0, ersetzt sie im Item durch leere Einträge und erhält deren Slotindex. |
| Lock-Konstruktor `0x393650` | Referenzzählung, optionale Flags und virtuelle Lock-Aufrufe. Im Test originale Konstruktorlogik, aber künstliche Lock-Implementierung. Kein Nachweis einer exklusiven Engine-Transaktion. |

Die frühere grobe Beschreibung einer „Sockelliste“ bedeutete nicht, dass diese
Funktion read-only ist. Ein unbedachter Aufruf des Verschleiß-/Ack-Pfads kann noch
vor einem Rückmeldefehler die Sockelbelegung verändern. Daher kann ein Fehlercode
nach dem Aufruf nicht als Zusage „keine Änderung“ behandelt werden. Ein vorhandener
beschädigter Einsatz muss vor solchen Pfaden über einen bestätigten Reparaturweg
behandelt werden; ein bereits entfernter Einsatz wird nicht wiederhergestellt.

## Native Probe und ihre Grenzen

| Funktion | Größe | SHA-256 des Originals |
|---|---:|---|
| Client-Ack `0x98fb70` | 773 | `d8221b3537464395a14ebbaf63b2bf124d15bbe0167c05e9a585f7338eb87aa1` |
| Lock-Konstruktor `0x393650` | 104 | `9361a1820340878d8bc677ec8cd21654fbab72c61102b5529939f23533e18e2f` |
| Sockelentfernung `0x240e3e0` | 465, drei Unwind-Fragmente | `88087dcf346127a076e9c715e91d78e3ce5b7ef5e8a10e3ca6a71a9afc8f759b` |

Die Client-Kopie enthält genau eine Testanpassung: Der neun Byte große TLS-Lesezugriff
bei `0x98fe2a` wird auf künstliche TLS-Daten im eigenen Arena-Speicher umgeleitet.
Das vorhandene TLS/TEB des Testprozesses wird weder umgebaut noch als Spiel-TLS
interpretiert. Alle anderen Ack-Bytes sind unverändert und einzeln verglichen.
Die Original-Sockelfunktion und der Original-Lock-Konstruktor bleiben unverändert.
Die drei Sockel-Unwind-Fragmente samt geprüfter Prologkette werden registriert.

Itemdefinitionen, UI-Empfänger, Listenanhängen, Freigabe und virtuelle Lock-Methoden
sind Test-Stubs. Die Fehlernummer 902 ist eine künstliche Konstante am originalen
Fehlerpfad. Native C++-Cleanup-Handler für diese fremden Funktionen werden im
Testhost nicht registriert; die verwendeten Stubs werfen keine Exceptions.
Keine Engine-Nachricht wurde tatsächlich verschickt, keine Persistenz ausgelöst.

Sechs zusätzliche native Fälle:

1. Positives Delta repariert das Hauptitem, verbraucht die Sockel und führt
   anschließend wegen abweichender Entfernungsliste zum Fehler. Die Teiländerung
   bleibt sichtbar und wird von der Nachherwertprüfung abgewiesen.
2. Bereits korrekt reparierte private Abbilder plus Delta 0 bleiben erhalten;
   leere Entfernungsliste passt, UI-Callback erfolgt, Nachherwerte passen.
3. Dieselben korrekten Abbilder mit falscher Entfernungsliste führen zum Fehler;
   passende Felder allein reichen folglich nicht zur Erfolgsbestätigung.
4. Normaler Verschleiß -1 liefert Erfolg und UI-Callback, erfüllt aber den
   Reparaturplan nicht. Auch Fehlercode 0 allein reicht nicht als Nachweis.
5. Vorhandene Sockel mit Haltbarkeit 0 werden entfernt; eine passende
   Serverliste führt zu zwei Sockel- und einem Panel-Callback. Kein Reparaturerfolg.
6. Ein fehlender Ausrüstungsslot kann im geprüften Debug-Flag-Pfad Fehlercode 0
   ohne Reparatur liefern. Die Nachherwertprüfung verhindert einen Scheinerfolg.

Insgesamt 122 native Aufrufe mit 2.221 Prüfbedingungen, 1.481 Aktionsbedingungen
und 124 Leserbedingungen bestanden; drei CTest-Suiten und MSVC `/W4 /WX` grün.
Reproduktion: [README](README.md). Die Spielanbindung bleibt technisch offen:
echte Engine-Zugriffsfreigaben, vollständiger Änderungs-/Ereignispfad für Inventar
und Ausrüstung, Bedieneingabe, Loader und B0. `installable` bleibt `false`.
