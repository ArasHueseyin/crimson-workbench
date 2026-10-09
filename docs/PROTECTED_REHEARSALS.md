# Geschützte Projektproben – v0.4.3

Diese Schutzschicht wird von der Desktop-Probe und `mod-recover` für neue
v3-Proben verwendet. Diese Proben erreichen ausschließlich markierte
Projektordner unter `.local/rehearsals/<id>`. Seit v0.4.8 verwendet der getrennte
[Live-Adapter](LIVE_APPLY.md) dieselbe Schutzschicht mit expliziter Zulassung.

Seit v0.4.6 ergänzt eine [Sicherungs-/Wiederherstellungsansicht](BACKUP_RECOVERY.md)
den Ablauf: existierende Sicherung und Historie prüfen, Rücknahme anzeigen und
unter erneut gehaltenen Sperren exakt diese Vorschau ausführen. Fehlende Backups
werden in diesem neuen Ablauf niemals neu angelegt.

## Reihenfolge und gehaltene Sperren

1. Die Probe kopiert unsere eigene ausführbare Datei nach
   `probe/workbench-guard-probe.exe` und erzeugt `probe/source.bin` als ausdrücklich
   synthetische Update-Testdatei. Keine Spiel-EXE und keine Original-PAZ wird kopiert.
2. `protection.json` enthält Größen und vollständige SHA-256-Hashes der erfassten
   Quellen sowie den Hash der ursprünglichen Registry. Der Marker `rehearsal.json`
   bindet dieses Manifest durch seinen Hash. Der einzige akzeptierte Herkunftstyp
   ist `project-copy-only`; daraus kann keine Vanilla-Freigabe entstehen.
3. Vor Anlegen des Engine-Locks oder Backups prüft die Sitzung die bekannten
   ausführbaren Pfade gegen die Windows-Prozessliste. Kandidaten werden über ihren
   tatsächlichen, kanonisierten Imagepfad abgeglichen; Junction-/Pfadaliase werden
   aufgelöst. Ein nicht zuverlässig abfragbarer Kandidat sperrt die Sitzung.
4. Die Sitzung hält EXE-Handles ohne Sharing. Für unveränderte Quelldateien ist
   nur Read-Sharing erlaubt. Root, `meta` und alle Quell-Elternordner werden mit
   Lesezugriff ohne Delete-Sharing gehalten. Reparse-Points, Hardlinks und falsche
   Dateitypen werden vor und nach Öffnen über den tatsächlichen Handle geprüft.
5. Vollständige Quellhashes werden aus den gehaltenen Handles geprüft. Nach dem
   Erwerb aller Sperren erfolgt erneut eine Prozessprüfung. Erst dann öffnet der
   private Transaktionskern seine Historie und erstellt/verifiziert das Backup.
6. Auch `.workbench` und `baseline.papgt` werden gehalten; der Backuphash wird
   nochmals geprüft. Planung, Apply, Reapply, Restore und Recovery behalten die
   Sperren bis zum Ende der Sitzung. Vor den Schreibgrenzen werden Prozessstatus
   und Handle-Eigenschaften erneut kontrolliert. Ein Fehler gibt die Sperren
   nicht vorzeitig frei; die gesamte Sitzung muss beendet werden.

Die Prozessabfragen lesen keine Prozessspeicher und beenden kein Programm.
In der tatsächlichen Desktop-Probe wird die kopierte EXE nicht gestartet.
Nur automatisierte Tests starten unsichtbar eigene Test-EXEs und schließen
anschließend exakt ihre eigenen Kindprozesse.

## Nachgewiesene Fehlerfälle

Die neue Testsuite prüft tatsächliches Windows-Verhalten mit eigenen Dateien:

- Paralleler Start während Apply/Reapply/Restore gesperrt, nach Freigabe möglich.
- Bereits laufende Kopie vor State-/Backuperstellung erkannt.
- Quell-/Backupwrites und Umbenennung geschützter Elternordner gesperrt.
- Fehler in Prozessprüfungen vor/nach Handle-Erwerb: keine Backupanlage,
  alle erworbenen Handles wieder freigegeben.
- Schutz bleibt bei injiziertem Transaktionsabbruch und abgelehnter Recovery aktiv.
- Update vor Recovery und nach abgeschlossenem Apply: neue Quelldateien und
  Registry sowie altes Backup bleiben erhalten. Kein Restore über einen neuen Build.
- Manipuliertes Schutzmanifest, Downgrade eines v3-Markers auf v2, beschädigtes
  Backup, Hardlinks, Pfadausbruch und behauptete Vanilla-Herkunft werden abgewiesen.

Zwei ursprüngliche Annahmen wurden durch diese Tests widerlegt und korrigiert:
Ein exklusiver Read-Handle allein erkennt bereits laufende Images nicht, und
ein reiner Attributzugriff hält keine wirksame Umbenennungssperre. Zusätzlich
mussten beide Seiten des Prozesspfadvergleichs kanonisiert werden.
Die Implementierung verwendet deshalb Prozessabgleich **und** gehaltene
Lesehandles. Primärdokumentation:
[CreateFileW und Sharing](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew),
[QueryFullProcessImageNameW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew).

## Updateprobe und Wiederherstellung

Nach den zwei Apply/Reapply/Restore-Zyklen und dem Recovery-Test wird ausschließlich
die synthetische Quelldatei geändert. Recovery muss diese Abweichung ablehnen,
ohne die Registry zu überschreiben. Anschließend erhält die Probe ihre ursprünglichen
synthetischen Bytes zurück und Recovery wird erneut geprüft. Bei einem Abbruch
während dieses absichtlichen Updateversuchs kann die Probe als geändert zurückbleiben;
sie bleibt zur Diagnose erhalten und kann durch eine neue Probe ersetzt werden.
Es wird kein unbekannter Zustand automatisch als neue Basis angenommen.

Bestehende v2-Proben bleiben über den bisherigen Kernel wiederherstellbar und
werden nicht stillschweigend aufgewertet. Neue v3-Proben müssen immer ihr passendes
Schutzmanifest und ihre Quellen besitzen. Ein nachträglich umbenannter oder
geänderter Quellpfad wird nicht durch einen anderen Dateihash ersetzt.

## Grenzen vor einer Live-Freigabe

Die Windows-Schutzschicht ist jetzt in die **Projekttransaktion** integriert.
Noch offen sind unabhängig bestätigte vollständige Spiel-Basishashes, ein daraus
abgeleitetes vollständiges Verzeichnis aller relevanten Originalarchive und
Startpfade, der Live-Backup-/Updateworkflow und die In-game-Abnahme.
Nicht erfasste ausführbare Kopien außerhalb der bekannten Startpfade und ein
feindlich veränderter Rechner sind kein bewiesener Schutzumfang.

Neue geschützte Proben verlangen Windows und haben keinen ungesicherten Ersatzpfad
auf anderen Plattformen. Die bisherigen v2-Proben bleiben separat les-/wiederherstellbar.
Getestete Abbruchpunkte sind weiterhin kein vollständiger Stromausfallnachweis.
Spiel und Saves wurden durch diese Arbeit weder gesperrt noch verändert.
