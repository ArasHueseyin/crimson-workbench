# Sicherung und Wiederherstellung – v0.4.6

Die Modwerkstatt kann bestehende **geschützte Projektproben** wieder öffnen,
deren Sicherung prüfen und die Rücknahme vorab anzeigen. Diese Funktion arbeitet
ausschließlich unter `.local/rehearsals/<id>` im Projekt. Sie kann keine
Spielinstallation wiederherstellen und erteilt keine Live-Schreibfreigabe.

## Bedienung

1. **Modwerkstatt → Sicherung & Wiederherstellung → Projektproben laden**.
2. Eine Projektprobe auswählen und **Backup und Rücknahme prüfen** wählen.
   Direkt nach einer neuen Probe führt **Letzte Projektprobe prüfen** zum selben
   Prüfablauf. Das Laden der Liste allein startet keine Wiederherstellung.
3. Die Vorschau nennt Sicherungsgröße und SHA-256, geprüfte Quellen, offene
   Transaktionen sowie die zu ersetzenden oder zu entfernenden Dateien mit ihren
   Vorher-/Nachher-Hashes. Leere eigene Gruppenordner werden zusätzlich genannt.
4. **Projektkopie wiederherstellen** schließt eine offene Transaktion ab und
   stellt anschließend die ursprüngliche Registry wieder her. Die App liest
   danach den tatsächlichen Zustand erneut ein. Bei einer bereits vollständig
   zurückgesetzten Probe ist der Knopf gesperrt.

Normale erfolgreiche Proben sind bereits zurückgesetzt. Die Funktion hilft bei
abgebrochenen Proben oder einem späteren erneuten Öffnen; der Nutzer muss keinen
Abbruch erzeugen. Alte v2-Proben und beschädigte Marker werden in der Liste mit
Begründung angezeigt und sind für diesen Ablauf nicht auswählbar. Die bestehende
CLI `mod-recover` bleibt für alte Proben erhalten.

## Sicherungs- und Vorschaugrenzen

- Die Liste liest ausschließlich Marker. Sie ist auf 2.000 Verzeichniseinträge
  begrenzt und zeigt höchstens 200 Einträge absteigend nach Ordnername.
- Die ausgewählte v3-Probe muss ihr hashgebundenes Schutzmanifest, ihre Quellen,
  das bestehende Engine-Lock, die Registry-Sicherung und die Historie besitzen.
  **Die neue Vorschau und die darauf folgende Wiederherstellung legen fehlende
  Backups oder Historien niemals neu an.** Eine unvollständige Initialisierung
  bleibt zur Diagnose erhalten; eine neue Probe bekommt einen eigenen Ordner.
- Die Prüfung hält die vorhandenen Start-/Quell-/Backup-/Ordnersperren an der
  Projektkopie und das exklusive Transaktionslock. Es werden keine Inhaltsdateien,
  Marker, Backups oder Journale angelegt oder verändert. Windows kann durch
  Lesezugriffe Dateisystem-Zugriffsmetadaten ändern.
- Die Rücknahmevorschau ist an den physischen Projektpfad, Marker, Backuphash,
  nächste Transaktionsnummer, offene Transaktion, Registryzustand und die konkret
  vorhandenen zu entfernenden Dateien gebunden. Unmittelbar vor Restore wird sie
  unter erneut gehaltenen Sperren rekonstruiert und verglichen. Eine andere
  Projektkopie oder ein veralteter Plan wird abgewiesen.
- Der Planbezeichner ist ein Änderungsvergleich, kein Herkunftszertifikat und
  kein Sicherheitsgeheimnis. Der bestehende Pfadschutz, das Schutzmanifest und
  die erneuten Quell-/Backup-/Historienprüfungen bleiben erforderlich.
- Fehlende aktive Dateien, fremde Inhalte, beschädigte Backups, Hardlinks,
  veränderte Quellen oder Registry und ein bereits laufendes geschütztes
  Testprogramm verhindern die Wiederherstellung. Es gibt keine automatische
  Reparatur oder Wiederholung nach einem Fehler.

Bei einer offenen Transaktion entscheidet die bereits sichtbare Registry wie im
bisherigen Kern: Vorher-Zustand bedeutet Rollback, Nachher-Zustand bedeutet
Abschluss des Commits. Nur nachgewiesene eigene obsolete Dateien werden entfernt.
Danach wird gegebenenfalls eine eigene Restore-Transaktion angelegt, die zuerst
die Registry auf die Sicherung zurücksetzt und erst dann die aktiven eigenen
Overlaydateien entfernt. Ursprüngliche Archive bleiben unberührt. Sicherung,
frühere Journale und private Stagingdaten bleiben erhalten.

Ein Ergebnisbericht `recovery-<id>.json` wird innerhalb derselben Probe erzeugt.
Schlägt nur dieser Berichtsexport fehl, kann der Restore bereits abgeschlossen
sein; **erneut prüfen** zeigt dann den tatsächlichen Zustand. Es wird niemals
automatisch nochmals geschrieben.

## CLI und API

```powershell
cargo run -p cd-cli --locked -- mod-backups
cargo run -p cd-cli --locked -- mod-restore-preview .local/rehearsals/<id>
cargo run -p cd-cli --locked -- mod-restore .local/rehearsals/<id> --review-id <review_id>
```

Die `review_id` stammt aus der unmittelbar vorher geprüften Vorschau.
`apply::recovery::{list, inspect, restore}` verwendet dieselbe Projektpfadgrenze
wie die bisherigen Proben. Desktopbefehle `rehearsal_list`, `rehearsal_review`
und `rehearsal_restore` verlangen eine aktuelle Katalogsitzung und sind nur für
das lokale Hauptfenster freigegeben. Ein Sitzungswechsel oder Ansichtswechsel
verhindert die Anzeige verspäteter UI-Antworten. Eine bereits gestartete
Transaktion läuft unter ihren Sperren zu Ende; Navigation ist kein Abbruch.

## Nachweise und offene Arbeit

Sechs neue Core-Tests prüfen lesende Vorschau, fehlende Sicherung, identische
Vorher-/Nachher-Hashes, offene Transaktionen vor/nach Commit, veraltete oder
fremde Vorschauen, beschädigte Quellen/Backups/Registry/Overlays, Hardlinks,
geschützte Pfade und begrenzte Listen. An allen 69 bestehenden
Apply-/Reapply-/Restore-Abbruchpunkten werden zusätzlich die Vorschau-Hashes
gegen die tatsächlich wiederhergestellten oder entfernten Dateien geprüft.
Vier UI-Flows prüfen den expliziten Ablauf, fehlerhafte Sicherungen, veraltete
Pläne, leere/alte Einträge, Navigation während einer Prüfung und 1024px Breite.

Der erfolgreiche vollständige Inhaltsvergleich der Nutzerinstallation bleibt
ein [separater Cachevergleich](CONTENT_AUDIT.md). Unabhängig bestätigte
Vanilla-Herkunft, Live-Backup-/Updateintegration, vollständige Absicherung der
Live-Start-/Ladepfade und In-game-Abnahme bleiben offen. Phase 4 ist noch nicht
abgeschlossen; Phase 5 wurde nicht begonnen.
