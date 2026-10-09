# Fremde Mods und Zusatzdateien – v0.4.10

Workbench trennt Originaldateien, nachgewiesene eigene Dateien und fremde
Zusätze. Unbekannte Dateien sind nicht automatisch schädlich oder Mods: Auch
Logs, Konfigurationen und Reste anderer Manager können zusätzliche Dateien sein.
Ohne ausdrückliche Bestätigung blockieren sie Live-B0.

## Unterstützter Umgang

**Modwerkstatt → Fremde Mods & Zusatzdateien → Zusatzdateien prüfen** erstellt
auf Anforderung eine genaue Liste mit Pfaden, Größen und SHA-256-Werten. Hierbei
werden ausschließlich Zusätze gehasht; Originalarchive werden nicht zusätzlich
vollständig gelesen. Die Prüfung und Bestätigung verändern keine Spieldatei und
nehmen keine Start-/Quellsperren. Sie sind auch während des Spielens möglich.
Die getrennten Live-Schreibabläufe bleiben währenddessen gesperrt.

Nach dem Lesen der Liste die Checkbox zum Beibehalten setzen und **Geprüfte
Zusatzdateien bestätigen** wählen. Vor dem Speichern wird die Liste erneut
aufgebaut und gehasht; veraltete Vorschauen werden ohne automatischen Retry
abgewiesen. Bestätigt wird genau dieser Dateisatz, keine beliebige künftige
Version und kein ganzer Ordner mit ungeprüften späteren Inhalten.

Die Bestätigung wird ausschließlich im Projekt gespeichert. Sie ist kein
Vanilla-Nachweis und keine Zusage zur technischen Kompatibilität fremder Mods.
Sie startet weder eine Inhaltsprüfung noch Apply. Anschließend Dateiliste und
Live-Vorschau neu laden. **Bestätigung zurücknehmen** widerruft sie, ohne die
Zusatzdateien zu löschen oder zu ändern. Live-Vorgänge erfordern danach wieder
einen zulässigen Bestand; bei weiterhin vorhandenen Zusätzen gegebenenfalls
neu bestätigen. Ein leerer Bestand braucht keine Bestätigung.

## Grenzen der Bestätigung

- Bestätigbar sind zusätzliche gewöhnliche Dateien und Ordner außerhalb der
  Depot-Originale, der Registry-/Metadatenbereiche und privater Workbench-Pfade.
  Dazu können unregistrierte PAZ-Gruppen, Loaderdateien und Konfigurationen gehören.
- Eine fremd modifizierte Registry oder registrierte Fremdgruppen werden nicht
  übernommen. Zuerst den fremden Mod mit seinem Manager zurücknehmen und nach
  dem Spielen Steam-Dateiprüfung abschließen. Verbleibende unregistrierte Zusätze
  können danach separat bestätigt werden. Workbench verschmilzt keine fremden
  Tabellenänderungen mit den eigenen Modulen.
- Fehlende, abweichende oder veränderte Originale bleiben gesperrt. Eine
  Zusatzbestätigung kann Originalhashes, Vanilla-Herkunftsbestätigung, bekannte
  Leseschemata, Backup, Prozessschutz oder aktuelle Dateivorschauen nicht umgehen.
  Gleiche Dateigröße allein beweist keinen unveränderten Originalinhalt.
- Symlinks/Junctions, Hardlinks, Savepfade und zusätzlich konfigurierte geschützte
  Unterordner werden nicht zum Beibehalten
  zugelassen. Saveinhalte werden dabei nicht gelesen. Grenzen: 512 Dateien,
  1.024 zusätzliche Ordner, zusammen 512 MiB und 128 Protokolleinträge.
  Der letzte Eintrag bleibt einem Widerruf vorbehalten.

## Einbindung in B0 und Berichte

Inventur und Inhaltsprüfung weisen bestätigte Fremddateien separat aus.
Originale Depotdateien können niemals durch diese Zuordnung aus ihrem Vergleich
verschwinden, auch wenn Steam später einen früher zusätzlichen Pfad übernimmt.
Die Inventur prüft bei der Zuordnung nur Metadaten; ein gleich großer nachträglicher
Inhaltswechsel wird spätestens durch die geschützte Live-Prüfung abgewiesen.
Ein Bericht mit `foreign_approval` prüft seine aufgelisteten Depot-Originale und
verweist auf die separate Bestätigung; er zertifiziert die Fremddateien nicht.
Alte Berichte ohne das optionale Feld bleiben lesbar.

Einrichtung, Apply/Reapply, Restore und Basiswechsel nehmen die bestätigten Dateien
in ihren frischen SHA-256-Vergleich und die gehaltenen Quellsperren auf. Zusätzliche
EXEs gehören zum Startschutz, auch mit großgeschriebener Dateiendung. Leere Dateien
werden ebenfalls mit ihrem tatsächlichen Hash geprüft. Fremdgruppen werden bei der
Vergabe eigener Gruppen-IDs ausgespart und bleiben unregistriert, sofern sie zuvor
unregistriert waren. Restore entfernt ausschließlich eigene Änderungen.

Live-Vorschauen binden außerdem die aktuelle Bestätigung. Änderung oder Widerruf
macht sie ungültig; ein parallel über die CLI ausgelöster Widerruf wird an den
Schutzgrenzen des laufenden Vorgangs erkannt. Die Desktop-App verweigert Änderungen
der Bestätigung während ihres eigenen Live-Auftrags. Es gibt keinen automatischen
Schreibversuch nach Fehlern. Bei einem unterbrochenen Basiswechsel müssen die
bereits bestätigten Fremddateien weiterhin exakt passen; ihre erneute Bestätigung
während einer unvollständigen eigenen Historie wird nicht angeboten.

Unveränderliche Datensätze liegen unter
`.local/foreign/<Installationskennung>/<laufende-Nummer>.json`. Der jüngste
Datensatz bestimmt die aktuelle Bestätigung; ein leerer Satz ist ein Widerruf.
Beschädigte oder lückenhafte Protokolle erlauben keine Übernahme.

## CLI und spätere manuelle Abnahme

```powershell
cargo run --release -p cd-cli --locked -- foreign-preview
cargo run --release -p cd-cli --locked -- foreign-confirm --review-id <review.review_id> --preserve
cargo run --release -p cd-cli --locked -- foreign-revoke --approval-id <approval_id>
```

Nach Abschluss aller Entwicklungsphasen manuell prüfen: bekannte Zusatzdateien
auflisten und bestätigen, eigenen Mod anwenden, Fremddateien unverändert vorfinden,
eigene Änderungen zurücknehmen und die Bestätigung widerrufen. Fremde Registry,
geänderte Originale und geänderte Zusatzinhalte müssen weiterhin die entsprechenden
Vorgänge sperren. Die Spielwirkung fremder Mods ist durch die automatisierten
Tests nicht bewiesen. [Gesamter Live-Ablauf](LIVE_APPLY.md).
