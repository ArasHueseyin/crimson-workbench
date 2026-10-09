# Vollständige Inhaltsprüfung – v0.4.5

Die Modwerkstatt kann nach dem Beenden des Spiels alle installierten Depotdateien
blockweise lesen, mit den SHA-1-Werten im lokalen Steam-Cache vergleichen und
zusätzlich SHA-256 aufzeichnen. Es werden keine Spieldateien geändert, repariert,
zurückgesetzt oder gegen einen Spielstart gesperrt. Das Ergebnis ist ein eigener
Berichtstyp, keine B0-Schreibfreigabe.

## Bedienung

1. Spiel beenden und in der Modwerkstatt **Dateiliste prüfen** wählen.
2. Bei vollständiger, abweichungsfreier Dateiliste und erkannt beendetem Spiel
   wird **Dateiinhalte prüfen** verfügbar. Der Umfang wird vorab angezeigt;
   derzeit sind es etwa 143,51 GiB bzw. 154.097.618.899 Bytes.
3. Fortschritt zeigt aktuelle Datei, gelesene Bytes und fertige Dateien. Die
   Prüfung läuft bei einem Ansichtswechsel weiter. **Inhaltsprüfung abbrechen**
   fordert den Abbruch an. Ein Wechsel der Installation oder Itemsprache bricht
   den Auftrag ebenfalls ab. Je Desktopinstanz läuft höchstens ein Auftrag.
4. Ein abgeschlossener Bericht zeigt passende oder abweichende Inhalte. Mit
   **Prüfbericht exportieren** wird eine neue JSON unter `exports/` geschrieben.
   Zeitstempel und Grenzen bleiben im Bericht sichtbar.

CLI, ebenfalls nur bei beendetem Spiel:

```powershell
cargo run --release -p cd-cli --locked -- installation-verify --output exports/installation-content.json
```

Fortschritt erscheint auf stderr, ohne `--output` erscheint das Ergebnis auf
stdout. Inhaltsabweichungen ergeben einen vollständigen Diagnosebericht und
Exitcode 2. Ein abgebrochener oder unvollständiger Lauf erzeugt keinen vollständigen
Bericht. Ctrl+C beendet die CLI; die Desktop-App hat einen eigenen Abbruchknopf.
Berichte werden nur auf ausdrücklichen Export geschrieben; es gibt kein automatisches
Speichern von Teilständen und keine Wiederaufnahme eines abgebrochenen Hashlaufs.

## Umfang und Herkunft

Der Prüfer nimmt seine Dateiliste selbst aus der aktuellen Installation und den
zugehörigen Depotlisten. Eine vom Frontend übermittelte Liste, fremde JSON oder ein
altes Metadatenresultat können keinen Hashlauf vorgeben. Fehlende, zusätzliche,
anders typisierte Dateien, unbekannte Depotlisten und Registry-Probleme müssen
zuerst geklärt werden. Es werden nur erwartete Depotdateien geöffnet, keine
zusätzlichen Laufzeitdateien oder Savepfade. Kein Steam-Login, Download oder
automatisches „Verify integrity“ wird gestartet.

SHA-1 dient ausschließlich der Kompatibilität mit dem bestehenden Steam-Format.
SHA-256 entsteht im selben Lesedurchlauf. Die Cache-Manifeste werden über SHA-256
an den Bericht gebunden. Ein passender Cachevergleich bestätigt dessen Herkunft
nicht: `all_files_match_cache` darf true werden, aber `certified_vanilla` und
`can_apply` bleiben immer false. Dieser Bericht lässt sich nicht als Vanilla-
Snapshot importieren. Der historische Metadatenbericht behält weiterhin
`content_verified: false`; die beiden Berichtstypen sind getrennt.

Seit v0.4.7 kann ein exportierter Inhaltsbericht mit einer dazu passenden
Registry-Kopie als [beobachteter Ausgangsstand](BASELINE.md) im Projekt gespeichert
werden. Das ist ausdrücklich kein Import als zertifizierte Vanilla-Basis und
erteilt keine Schreibfreigabe. Der Originalbericht bleibt unverändert.

## Spielstart, Änderungen und Pfade

Die echte Windows-Prozessprüfung erfolgt vor der Vorbereitung, vor jeder Datei,
zwischen Leseblöcken nach jeweils mindestens 250 ms und vor dem Abschluss.
Unklarer Prozessstatus verhindert einen erfolgreichen Lauf. Ein erkanntes Spiel
wird niemals beendet; die Inhaltsprüfung bricht stattdessen ab. Abbruchwünsche
werden zwischen 1-MiB-Blöcken geprüft. Eine bereits laufende Dateisystemoperation
muss zunächst zurückkehren; es gibt kein garantiertes Abbruch-Zeitlimit bei einem
blockierenden Datenträger.

Windows-Quellen werden mit Lesezugriff und Read/Write/Delete-Sharing geöffnet.
Vor dem ersten Inhaltsbyte wird der tatsächliche Pfad des geöffneten Handles
gegen die Installation geprüft. Reparse-Punkte, Verzeichnisse und Hardlinks
werden abgewiesen. Dateigröße, Änderungs-/Erstellzeit und Dateikennung werden
vor und nach dem Lesen sowie am Ende des gesamten Laufs verglichen. Wachstum ist
auf höchstens ein zusätzlich gelesenes Byte begrenzt und führt zum Abbruch.
Auch Registry, Depotlisten und der gesamte Datei-/Größenbestand werden erneut geprüft.

Diese Kontrollen erkennen normale Updates und Wechsel während des Laufs. Ohne
gehaltene Schreib-/Startsperren ist das **kein atomarer Snapshot** und kein Beweis
gegen gezielte Manipulation von Inhalt und Zeitstempeln. Ein späterer Spielstart
oder ein Update kann den Befund sofort veralten lassen. Die Live-Engine muss ihre
Quellen erneut unter eigenen Sperren prüfen; siehe
[geschützte Projektproben](PROTECTED_REHEARSALS.md).

## Nachweise und offene Abnahme

Synthetische Tests: vollständiger SHA-1-/SHA-256-Vergleich einschließlich bekannter
`abc`-Testvektoren, mehrerer Leseblöcke und leerer Dateien; gleich große geänderte
Inhalte; Abbruch vor/während des Auftrags; Spielstatusfehler und Spielstart;
Änderungen bereits gelesener Quellen oder Steam-Metadaten; Wachstum, fehlende,
zusätzliche und hart verlinkte Quellen. Ein gehaltener Audit-Lesehandle erlaubt
Schreibzugriffe, deren Metadatenänderung danach erkannt wird.

Worker-Tests prüfen nur einen aktiven Auftrag, Sitzungsgrenzen, Abbruch und
Panikbehandlung ohne Erfolgsbericht. UI-Tests prüfen die Startsperre bei laufendem
oder unbekanntem Spiel, Fortschritt, Ansichtswechsel, Abbruch, Abweichungen und
Berichtsexport. Der native Desktoptest startet den 154-GB-Lauf nicht automatisch.

Der Nutzer hat den vollständigen Originallauf am 20.09.2026 von 13:23:25 bis
13:25:45 Uhr (Europe/Vienna) über die Desktop-App ausgeführt und den Bericht
`exports/installation-audit-1789903405-11284-1-0.json` bereitgestellt:
285 Dateien, darunter 195 PAZ-Archive, zusammen 154.097.618.899 Bytes; alle
SHA-1-Werte stimmen mit den lokalen Depotlisten überein. Der Bericht meldet
stabile Metadaten. Die nachfolgende reine Berichtsauswertung bestätigt alle
Einzelvergleiche, eindeutige Pfade, Gesamtumfang und die 38 zuvor beobachteten
EXE-/Metadaten-SHA-256-Werte. Nachweis: `.local/user-content-audit-review.json`;
SHA-256 des Originalberichts:
`f32c50a354dbd81e85889cb764b4ecdf6d83958ed09c9b9eef70077cc48a64dc`.

Damit ist der vollständige lesende Originallauf erfolgt. Ein unabhängig
bestätigter Vanilla-Nachweis und In-game-Abnahme bleiben offen. Live-Backup und
Basiswechsel sind inzwischen separat implementiert, aber auf dieser Installation
noch nicht ausgeführt. Die Berichtswerte
`certified_vanilla` und `can_apply` bleiben false. Der Bericht beschreibt den
Zustand zum Prüfzeitpunkt und wurde bei seiner Auswertung nicht durch einen
erneuten Scan der Installation aktualisiert.

Referenzen: [RustCrypto SHA-1 API und Verwendung für Altformate](https://docs.rs/sha1/0.10.6/sha1/),
[Windows CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew),
[GetFinalPathNameByHandleW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew).

Seit v0.4.9 verwendet die Inhaltsprüfung die Workbench-bewusste Inventur. Nur
anhand intakter eigener Historie zugeordnete Dateien werden aus der Liste der
zu prüfenden Originale genommen; fremde Zusätze bleiben blockierend. Eine aktive
Mod-Registry oder ein offener Basiswechsel verhindert den Start. Die originale
Dateiliste bleibt für Berichte und gespeicherte Ausgangsstände rückwärtskompatibel.

Seit v0.4.10 dürfen genau bestätigte Zusatzdateien vorhanden bleiben. Der Bericht
prüft weiterhin alle Depot-Originale und nennt gegebenenfalls `foreign_approval`
als Verweis auf die separate Bestätigung. Er zertifiziert deren Inhalte nicht.
Änderungen des Bestätigungsstands während der Prüfung verwerfen das Ergebnis.
Vor Live-Schreibzugriffen werden die Zusätze frisch gehasht und geschützt.
[Details](FOREIGN_FILES.md).
