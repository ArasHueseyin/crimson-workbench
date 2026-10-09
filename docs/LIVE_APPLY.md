# Live-B0 – Einrichtung, Apply und Restore (v0.4.8)

Der Desktop und die CLI besitzen jetzt einen Live-Schreibpfad. Er ist bei der
Installation des Nutzers **nicht eingerichtet und wurde dort nicht ausgeführt**.
Die Entwicklung und Abnahme dieser Version verändern oder sperren das laufende
Spiel nicht. Manuelle Spieltests bleiben auf Nutzerwunsch bis zum Abschluss aller
Entwicklungsphasen zurückgestellt.

## Vertrauensmodell

Ein gespeicherter Cachebericht wird nicht automatisch als Vanilla zugelassen.
Für die Einrichtung muss der Nutzer ausdrücklich bestätigen, dass Steams
Dateiprüfung **vor dem ausgewählten Inhaltsbericht** vollständig beendet wurde.
Fehlt diese tatsächliche Voraussetzung, später Steam prüfen lassen und danach
einen neuen vollständigen Bericht sowie Ausgangsstand erstellen. Die Workbench
führt Steam Verify nicht selbst aus und bestätigt diesen Schritt niemals selbst.

Das ist eine Nutzerbestätigung der Herkunft, keine unabhängige kryptografische
Zertifizierung der Steam-Depotlisten. Die vorhandenen Berichtsfelder
`certified_vanilla` und `can_apply` bleiben unverändert `false`. Live-B0 prüft
zusätzlich Bericht, Registry-Kopie, bekannten Lesebuild, Installationspfad und
Dateibestand. Unmittelbar vor Einrichtung, Apply oder Restore werden **alle
erfassten Originaldateien außer der separat behandelten Registry** vollständig
SHA-256-geprüft, während ihre Dateihandles gegen Schreiben und Ersetzen schützen.
Die Registry wird gegen die geprüfte Sicherung und eigene Historie geprüft.

Frühere Dokumente verlangten einen noch nicht verfügbaren unabhängigen
Vanilla-Nachweis. Der implementierte Zulassungsweg ist konkret die oben erklärte
Steam-Nutzerbestätigung plus vollständige erneute Inhaltsprüfung. Ein bloßer
Checkbox-Klick ohne tatsächlich vorausgegangene Steam-Prüfung erfüllt die
Voraussetzung nicht.

## Desktop-Ablauf für den späteren Test

1. Spiel beenden. **Modwerkstatt → Live-Anwendung & Restore → Live-Status laden**.
2. Einen passenden gespeicherten Ausgangsstand auswählen und **Live-Einrichtung
   prüfen**. Pfad, Berichtdatum und Dateianzahl prüfen. Steam-Prüfung nur bestätigen,
   wenn sie vor genau diesem Bericht abgeschlossen wurde.
3. **Live-Basis prüfen und einrichten** erstellt nach der Inhaltsprüfung die
   Registry-Sicherung im Transaktionsordner. Es wird noch kein Mod angewendet.
4. B1–B3 einstellen und **Vorschau berechnen**. Anschließend oben **Live-Dateivorschau
   berechnen**. Die Vorschau zeigt tatsächliche Gruppen-IDs, Registry-Ersetzung und
   gegebenenfalls Entfernung bisheriger eigener Gruppen mit Vorher-/Nachherhashes.
5. **Geprüften Mod auf Spiel anwenden** prüft Quellen, Prozessstatus, Historie und
   Vorschau erneut. Erst danach werden die vorbereiteten Änderungen geschrieben.
6. **Live-Restore prüfen → Geprüften Live-Restore ausführen** nimmt eigene Änderungen
   zurück. Es ist kein Modplan erforderlich. Bei einer unterbrochenen Transaktion
   wird zuerst deren tatsächlicher Commitzustand aufgelöst und dann zurückgesetzt.

Große Installationen brauchen mehrere Minuten für den Inhaltsvergleich. Die App
bleibt bedienbar; der Live-Auftrag kann abgebrochen werden. Ein Abbruch während des
Schreibens kann eine wiederherstellbare Transaktion hinterlassen. Deshalb danach
Status und Restore prüfen; es gibt keinen automatischen erneuten Schreibversuch.
Ein bereits erfolgreich abgeschlossener Vorgang wird durch eine verspätete
Abbruchanfrage nicht nachträglich als erfolglos dargestellt. Nach Ansichtswechsel
zeigt **Live-Status laden** den laufenden Auftrag derselben App-Sitzung wieder an.

## Schutz und Ablage

- `.local/live/<Installationspfad-Hash>/admission.json`: ausdrücklich bestätigte
  Herkunft, Ausgangsstand-ID, Installationspfad sowie Bericht-/Registryhash.
  Dieser Datensatz wird vor der ersten Live-Sicherung im Projekt veröffentlicht,
  damit eine unterbrochene Einrichtung mit derselben Basis fortgesetzt werden kann.
  Er bedeutet allein noch keinen erfolgreichen Inhaltsvergleich oder Backupabschluss.
- `<Spiel>/.workbench/baseline.papgt`: geprüfte Original-Registry. Original-PAZ/PAMT
  werden erhalten und nicht gepatcht. Staging und Journal liegen ebenfalls hier,
  damit atomare Umbenennungen auf demselben Dateisystem bleiben.
- `<Spiel>/.workbench/transactions/<id>/intent.json` und `complete.json`: unveränderliche
  Vorher-/Nachherzustände mit Hashes und Commitentscheidung. Neue Intents enthalten
  `started_at` als UTC-Epochsekunden; alte Journale ohne diesen Wert bleiben lesbar.

Vor dem Lesen großer Archive werden alle bekannten EXE-Pfade geöffnet und gegen
einen neuen Start gehalten. Bereits laufende Prozesse werden über Namen und
tatsächlichen Pfad erkannt. Fehler bei der Prozessprüfung verweigern den Vorgang.
Quell-, Elternverzeichnis- und Backupsperren bleiben über die gesamte Transaktion
erhalten. Abbruch/Prozessstatus werden beim Hashen spätestens etwa alle 250 ms
zwischen Leseoperationen geprüft. Es gibt keinen ungeschützten Nicht-Windows-Fallback.
Die Aussage umfasst die erfassten EXEs innerhalb der Installation; beliebige externe,
umbenannte Kopien oder unbekannte Startmechanismen werden nicht als getestet behauptet.

Eine separate, rein lesende Ansicht akzeptiert eine eigene modifizierte Registry
nur, wenn Originalbackup, vollständige Historie und tatsächliche Overlaydateien
sie erklären. Datenbank, Herstellung und Modbuilder lesen dann die Originalgruppen;
ein Reapply multipliziert keine bereits geänderten Werte erneut. Diese Ansicht
nimmt keine Start-/Quellsperren und kann keine Transaktionen ausführen.

## Basiswechsel nach Steam-Update oder Dateiprüfung

Seit v0.4.9 zeigt **Basis nach Spielupdate wechseln** einen separaten Vorgang:

1. Nach dem Spielen Steam-Dateiprüfung abschließen. Die Registry muss bereits der
   Original-Registry der neuen Basis entsprechen. Ein alter Restore ist kein
   Reparaturweg für einen neuen Build.
2. Dateiliste und vollständigen Inhaltsbericht erstellen, Bericht exportieren
   und neuen Ausgangsstand speichern. Eigene Dateien werden anhand der intakten
   Zulassung und Transaktionshistorie separat ausgewiesen; fremde Dateien bleiben
   sichtbar. Eine aktive eigene Mod-Registry verhindert einen Vanilla-Bericht.
3. Neue Basis auswählen und **Basiswechsel prüfen**. Die Vorschau nennt alte/neue
   Basis, alle zu archivierenden Dateien und den Zielordner. Herkunftsbestätigung
   erneut ausdrücklich setzen. Unbekannte Leseschemata werden verweigert.
4. Bei beendetem Spiel ausführen. Alle neuen Originalquellen werden unter
   Start-/Quellschutz frisch gehasht. Die neue Registry bleibt währenddessen
   gegen Änderungen gehalten und wird vom Basiswechsel niemals geschrieben.
5. Bisherige eigene Gruppen und der komplette private Transaktionsordner werden
   unverändert nach `<Spiel>/.workbench-history/<Nummer>/` umbenannt. Die Vorschau
   bindet Hashes sämtlicher archivierter Dateien. Keine alten Originalarchive
   werden kopiert und keine Archivstände gelöscht. Danach entsteht eine neue
   Registry-Sicherung mit leerer Transaktionshistorie; einen Mod anschließend
   mit aktuellen Einstellungen neu berechnen und ausdrücklich anwenden.

Das unveränderliche Projektprotokoll liegt unter
`.local/live/<Installationskennung>/updates/<Nummer>/{intent.json,complete.json}`.
Der erste Zulassungsdatensatz wird nicht überschrieben. Ein offener Vorgang
blockiert Apply/Restore und Inhaltsbericht und bietet das Fortsetzen mit exakt
seiner vorgesehenen Basis an. Deterministische `.part`-Dateien ermöglichen auch
Fortsetzung einer unterbrochenen Protokollveröffentlichung. Eine teilweise neu
angelegte Registry-Sicherung wird nur bei passendem Inhalt vervollständigt.

Archivgruppen werden vor ihrer Historie verschoben. Windows erlaubt hier keine
Umbenennung mit offenen Kinddateien: Jeder Baum wird vorher und nachher gegen
seine Hashliste geprüft; anschließend bleiben alle Archivdateien bis zum
Abschluss gegen Schreiben/Löschen gehalten. Gleichzeitige Änderungen anderer
Programme sind kein unterstützter Arbeitsablauf und führen bei Abweichungen
zum Abbruch. Vollständige Originalquellen und neue Registry bleiben durchgehend
geschützt. Kollidierende Gruppen, Fremdinhalte, Links, beschädigte Sicherungen
und geänderte eigene Moddateien werden verweigert.

Die allgemeine Inventur liest weiterhin keine PAZ-Inhalte. Die Zuordnung
**Eigene Workbench-Dateien** ist ein Metadatenbefund aus eigener Historie und
keine erneute Inhaltsprüfung. Originale Depotdateien können niemals durch diese
Zuordnung aus dem Originalvergleich verschwinden. Bekannte Archive werden in
späteren Inventuren über Namen, Größen und Struktur geprüft.

Seit v0.4.10 können genau geprüfte zusätzliche Dateien ausdrücklich zum
unveränderten Beibehalten bestätigt werden. Sie bleiben fremde Dateien und
werden vor Live-Schreibvorgängen frisch geprüft und geschützt. Modifizierte
Originaldateien und fremde Registry-Registrierungen bleiben gesperrt.
[Unterstützter Umgang, Widerruf und Grenzen](FOREIGN_FILES.md). Echte Stromausfälle und Spielwirkung bleiben
ungeprüft; manuelle Spieltests sind bis nach allen Entwicklungsphasen verschoben.

## CLI

```powershell
cargo run --release -p cd-cli --locked -- live-status
cargo run --release -p cd-cli --locked -- live-setup-preview <Ausgangsstand-ID>
cargo run --release -p cd-cli --locked -- live-setup <Ausgangsstand-ID> --review-id <Vorschau-ID> --steam-verified-before-audit
cargo run --release -p cd-cli --locked -- live-update-preview <Neue-Ausgangsstand-ID>
cargo run --release -p cd-cli --locked -- live-update <Neue-Ausgangsstand-ID> --review-id <Vorschau-ID> --steam-verified-before-audit
cargo run --release -p cd-cli --locked -- live-preview .local/live-request.json
cargo run --release -p cd-cli --locked -- live-execute .local/live-request.json --review-id <Vorschau-ID>
```

Request-JSON: `{"action":"restore"}` oder
`{"action":"apply","settings":{"trust_multiplier":2}}`. Nicht angegebene
Modparameter verwenden dieselben unveränderten Standardwerte wie die Modvorschau.
CLI-Schreibbefehle haben dieselben Herkunfts-, Vorschau-, Quell- und Prozessprüfungen.
Die Desktop-Version bietet zusätzlich den kooperativen Abbruchknopf.
