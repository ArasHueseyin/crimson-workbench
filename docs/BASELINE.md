# Gespeicherte Ausgangsbasis und Registry-Sicherung – v0.4.7

Die Modwerkstatt kann einen vollständigen exportierten Inhaltsprüfbericht mit
der aktuellen Dateiliste, den Steam-Depotlisten und der Registry vergleichen.
Danach speichert sie den unveränderten Bericht und eine erneut geprüfte Kopie
von `meta/0.papgt` im Projekt. Damit ist der beobachtete Ausgangsstand nach einem
App-Neustart verfügbar. Die Originalarchive werden nicht kopiert oder erneut
gehasht; diese Funktion ist keine vollständige Spielsicherung.

Seit v0.4.8 kann der separate [Live-Adapter](LIVE_APPLY.md) einen solchen Stand
mit zusätzlicher Herkunftsbestätigung und frischen geschützten Quellhashes nutzen.
Die hier beschriebene Speicherung allein richtet Live-B0 nicht ein.

## Bedienung

1. **Modwerkstatt → Ausgangsbasis & Registry-Sicherung → Prüfstände laden**.
2. Einen zuvor exportierten Bericht auswählen und **Bericht für Ausgangsbasis
   prüfen** wählen. Dateien, Größen, Depot- und Buildidentität sowie der aktuelle
   Registry-Inhalt müssen zum Bericht passen.
3. **Ausgangsbasis speichern** kopiert Bericht und Registry in einen neuen
   Projektordner. Die Vorschau wird vor dem Speichern erneut geprüft. Eine
   geänderte Installation oder Berichtsdatei macht die Vorschau ungültig.
4. Später den gespeicherten Ausgangsstand auswählen und **Gespeicherten Stand
   prüfen** wählen. Die App prüft die gespeicherten Dateien und vergleicht die
   aktuellen Installationsmetadaten. Alte Sicherungen werden nicht überschrieben.

Die ursprüngliche vollständige Inhaltsprüfung bleibt beim laufenden Spiel
gesperrt. Deren bereits exportierter Bericht lässt sich hingegen während des
Spielens übernehmen: Dieser Ablauf liest nur Metadaten und die kleine Registry,
verwendet keine Start-/Schreibsperren und schreibt ausschließlich ins Projekt.
Registry-Handles erlauben Read/Write/Delete-Sharing. Es wird kein Prozess gesteuert.

## Inhalt und Veröffentlichung

Jeder Stand hat einen eigenen Ordner `.local/baselines/<id>` mit:

| Datei | Inhalt |
|---|---|
| `audit.json` | Byteidentische Kopie des ausgewählten vollständigen Prüfberichts einschließlich aller beobachteten SHA-256-Werte |
| `registry.papgt` | Kopie der Registry, deren Größe und SHA-256 zum Prüfbericht passen |
| `snapshot.json` | Versioniertes Abschlussmanifest mit Berichts-/Registryhash, Metadatenidentität und Erstellzeit |

Bericht und Registry werden nach dem Schreiben erneut gelesen und verglichen.
Auch die aktuellen Metadaten werden unmittelbar vor dem Abschluss nochmals
verglichen. Erst danach wird das Manifest atomar als neue Datei veröffentlicht.
Eine unterbrochene Kopie ohne gültiges Manifest kann nicht als gespeicherter
Stand geöffnet werden. Sie bleibt zur Diagnose erhalten; Wiederholen erstellt
einen neuen Ordner. Kein existierender Stand wird ergänzt oder ersetzt.

Eingaben sind auf lokale Berichtdateien direkt unter `exports/` mit Namen
`installation-audit-*.json` beschränkt. Berichtgröße maximal 8 MiB, Registry
maximal 1 MiB, Manifest maximal 16 KiB. Unbekannte/mehrfach angegebene JSON-Felder,
falscher Berichtstyp, Teilberichte, widersprüchliche Erfolgsflags, behauptete
Vanilla-/Schreibfreigabe, doppelte oder unsichere Dateipfade, ungültige Hashes,
unzulässige Größen und widersprüchliche Depotlisten werden abgewiesen. Projekt-
und Spiel-/Save-Pfadgrenzen sowie Hardlinkprüfungen bleiben aktiv.

Die Listen sind auf je 2.000 Verzeichniseinträge begrenzt und zeigen höchstens
200 passende Namen. Verknüpfte Einträge werden nicht angeboten. Der Nutzer
wählt den Bericht bzw. Ausgangsstand ausdrücklich; eine neue Prüfung oder ein
Speichervorgang wird nicht automatisch gestartet.

## Bedeutung des Status

- **Metadaten passen:** Gespeicherter Bericht und Registry-Kopie sind intakt;
  aktuelle Build-/Depot-/Dateimetadaten und Registry passen zum gespeicherten
  Stand. Das ist keine erneute Inhaltsprüfung der großen Archive.
- **Installation weicht ab:** Der gespeicherte Stand bleibt erhalten; die
  aktuelle Installation hat abweichende oder unvollständige Metadaten. Es erfolgt
  keine Reparatur und kein Restore über einen möglicherweise neuen Build.
- **Installation nicht prüfbar:** Der gespeicherte Stand ist intern intakt,
  aber die Installation konnte nicht inventarisiert werden.
- Ein beschädigter gespeicherter Bericht, eine geänderte Registry-Kopie oder
  ein ungültiges Manifest führt zu einem Fehler statt einem Erfolgsstatus.

Der Zeitpunkt der ursprünglichen Inhaltsprüfung bleibt sichtbar. Gleich große
Archivänderungen mit passenden Metadaten können unbemerkt bleiben; ein eigener
Test hält diese Grenze ausdrücklich fest. Vor Live-Apply müssen die relevanten
Quellinhalte unter dem dafür vorgesehenen Schutz erneut geprüft werden.

Die Herkunft bleibt **lokaler, nicht unabhängig authentifizierter Steam-Cache**.
Das Manifest ist eine prüfbare Ablage und kein externes Herkunftszertifikat.
`certified_vanilla` und `can_apply` bleiben false. Die gespeicherte Basis wird
noch nicht als vertrauenswürdige B0-Live-Basis zugelassen; die Registry-Kopie
kann nicht über diese Ansicht in die Installation zurückgeschrieben werden.

## CLI und Desktop

```powershell
cargo run -p cd-cli --locked -- baseline-list
cargo run -p cd-cli --locked -- baseline-preview installation-audit-<name>.json
cargo run -p cd-cli --locked -- baseline-capture installation-audit-<name>.json --review-id <review_id>
cargo run -p cd-cli --locked -- baseline-status <id>
```

Die vier Desktopbefehle `baseline_catalog`, `baseline_preview`, `baseline_capture`
und `baseline_inspect` verlangen eine aktuelle Katalogsitzung und sind nur für
das lokale Hauptfenster zugelassen. Verspätete UI-Antworten nach Navigation oder
Sitzungswechsel können keine alte Speichervorschau wieder aktivieren.

## Testreihenfolge

Der Nutzer hat am 20.09.2026 entschieden, die manuellen Spieltests erst nach
Abschluss der Entwicklung aller Phasen durchzuführen. Sie bleiben dokumentiert
und gelten bis dahin als offen. Automatisierte Tests, Sicherungs-/Pfadprüfungen
und die Voraussetzungen für Live-Schreibzugriffe bleiben davon unabhängig.

Sieben neue Core-Tests decken Speicherinhalt, Eingabegrenzen, unterbrochene
Veröffentlichung, veraltete Vorschauen, Manipulation/Hardlinks, Updatezustand,
Erhalt alter Sicherungen und unveränderte Quelldateien ab. Vier neue UI-Flows
prüfen Übernahme, Wiederöffnen, Fehler-/Updatezustände und verworfene Vorschauen.
