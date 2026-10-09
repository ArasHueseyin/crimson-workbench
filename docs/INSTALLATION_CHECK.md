# Installationsprüfung – Metadateninventur

Stand: 20.09.2026. Die Modwerkstatt bietet **Dateiliste prüfen**. Die Prüfung
liest Verzeichnis-Metadaten, die lokale Steam-App-/Depotliste und `meta/0.papgt`.
Sie öffnet keine PAZ-, EXE-, DLL- oder Saveinhalte, setzt keine Startsperren,
startet keine Steam-Verifikation und schreibt nichts ins Spielverzeichnis.
Die CLI funktioniert dafür auch bei einem unbekannten Datenbuild:

```powershell
cargo run -p cd-cli --locked -- installation-check --output .local/installation.json
```

Ohne `--output` erscheint JSON auf stdout. Ausgabedateien unterliegen der
bestehenden Projekt-Pfadkontrolle. Es gibt weiterhin keinen Live-Schreibbefehl.
In der Desktop-App ist der Bericht an die geöffnete Katalogsitzung gebunden;
für nicht unterstützte Katalogbuilds kann die CLI separat diagnostizieren.

## Aussage des Berichts

- `directory_scan_complete`: Verzeichnisbaum ohne Lese-/Budgetfehler aufgelistet.
  Symbolische Verknüpfungen/Reparse-Punkte werden als ungeprüft gemeldet und nicht verfolgt.
- `depot_comparison_available`: alle installierten Depotlisten gefunden und
  strukturell geprüft; IDs und Gesamtgrößen passen zum Appmanifest. Ein fehlendes
  Depot führt nicht zu einem vermeintlich vollständigen Teilvergleich.
- `size_matches`: Name, Typ und Größe stimmen mit der lokalen Depotliste überein.
  **Kein Inhaltsvergleich.** Gleich große Änderungen bleiben unerkannt.
- `missing`, `size_mismatch`, `type_mismatch`, `additional`, `unsupported_entry`
  unterscheiden Abweichungen. Zusätzliche Dateien können legitime Laufzeitdateien
  sein; allein daraus wird kein Fremdmod-Befund abgeleitet.
- Registry-Gruppen werden mit dem Verzeichnis-/Depotbestand abgeglichen;
  nicht installierte optionale Sprachgruppen sind kein Fehler. Unregistrierte
  Archivgruppen und fehlende erforderliche Gruppen werden gemeldet.
- EXE-Liste vereinigt erwartete und gefundene EXE-Pfade innerhalb der Installation,
  auch fehlende oder zusätzliche EXEs. Eine EXE-Endung belegt weder deren Rolle
  noch sämtliche möglichen externen Start- und Ladepfade.

`content_verified`, `certified_vanilla`, `can_apply` und die Depot-Eigenschaft
`authenticated` bleiben **immer false**. Der Bericht ist nicht als B0-Snapshot
importierbar und kann keine Schreibberechtigung erteilen. Der Registryvergleich
mit dem beobachteten Build ist zusätzlich diagnostisch, kein Vanilla-Zertifikat.

## Befund an dieser Installation

Steam-Build **25381195**, Prüfung während des laufenden Spiels:

| Befund | Ergebnis |
|---|---|
| Erwartete / gefundene Dateien | 285 / 285, alle Namen und Größen passend |
| Gesamte erwartete Dateigröße | 154.097.618.899 Bytes (nur Metadaten summiert) |
| Depot 3321461 | Manifest 1190168329432671189, 281 Dateien und ein Verzeichniseintrag |
| Depot 3321466 | Manifest 4235648687492059552, 4 Dateien |
| EXE-Funde | `bin64/CrimsonDesert.exe`, `bin64/crashpad_handler.exe`, `bin64/pers.exe` |
| Registry | SHA-256 entspricht dem bisher beobachteten Build |
| Cache-Signaturen | beide leer; kein authentifizierter Herkunftsnachweis |
| Abweichungen | keine bei dieser Metadatenprüfung |

Nachweis: `.local/phase4-v5-installation.json`. Dieser lokale Bericht enthält
Dateinamen, Größen und erwartete SHA-1-Werte aus dem Cache, keine Spielinhalte.
Er wird nicht ins Repository aufgenommen.

## Parser und Grenzen

Eigener Rust-Reader für das öffentlich dokumentierte binäre Depotformat:
Payload, Metadaten, Signaturabschnitt und Endmarker; begrenztes Protobuf-Lesen
ohne Chunkdaten zu entpacken oder Dateiinhalte herunterzuladen. Geprüft werden
Payload-CRC32 inklusive Längenpräfix, Identität, Dateihashlängen, Gesamtgröße,
Pfad-/Typkonflikte und doppelte Felder. Ein vorhandener Signaturblob würde
ebenfalls **nicht** automatisch als authentifiziert gelten.

Grenzen: 16 MiB pro Depotmanifest, höchstens 32 Depots, 16.384 kombinierte
Verzeichniseinträge, Tiefe 32; Appmanifest und Registry jeweils 1 MiB.
Verschlüsselte Dateinamen und Depotlinks werden nicht unterstützt. Widersprüchliche
Cachekopien, doppelte VDF-Schlüssel, Pending-TargetBuildID oder Änderungen am
Appmanifest während des Lesens verhindern den Vergleich. Nicht-Steam-Installationen
erhalten weiterhin eine Verzeichnisinventur mit ausdrücklicher Vergleichslücke.

Ohne Startsperren ist das kein atomarer Snapshot. Gleichzeitige Änderungen oder
Pfadwechsel sind möglich. Auch deshalb wird der Bericht niemals als Quelle für
Live-Apply, Restore oder Recovery verwendet. Die spätere Live-Sitzung muss sämtliche
relevanten Quellen aus gehaltenen Handles prüfen; siehe
[geschützte Projektproben](PROTECTED_REHEARSALS.md).

Seit v0.4.5 ergänzt eine getrennte [Inhaltsprüfung](CONTENT_AUDIT.md) diesen
Metadatenbericht. Sie ist bei laufendem oder unbekanntem Spielstatus gesperrt
und bestätigt auch bei passenden Inhalten kein Vanilla.

## Tests und nächster B0-Schritt

Synthetische Tests prüfen jeden abgeschnittenen Präfix einer gültigen Depotliste,
beschädigte CRCs, Traversal-/Gerätepfade, Duplikate, überlaufende Varints, falsche
IDs/Größen, fehlende und widersprüchliche Caches, Update-Metadaten, beschädigte
Registry, Typwechsel, zusätzliche EXEs und Archivgruppen, Größenänderungen,
gleich große Inhaltsänderungen sowie Verzeichnistiefe und Windows-Junctions.
Auch bei vollständig passenden Metadaten bleibt Live-Apply gesperrt.

Seit v0.4.9 ordnet die Inventur Dateien aus einer intakten eigenen Live-Zulassung,
Transaktionshistorie und abgeschlossenen Basiswechseln separat zu. Sie listet
sie als Workbench-Dateien statt unbekannter Zusätze; originale Depotdateien
bleiben immer im Originalvergleich. Eine eigene aktive Registry wird ausdrücklich
angezeigt und verhindert einen neuen Vanilla-Inhaltsbericht. Fehlende oder
beschädigte Eigentumsnachweise, offene Wechsel und fremde Kinder bleiben Fehler.
Die Zuordnung hasht keine Archivinhalte und zertifiziert kein Vanilla.

Der Nutzer hat den vollständigen Originallauf durchgeführt; Live-Zulassung,
Quellschutz, Backup und Basiswechsel sind separat implementiert. Auf der echten
Installation wurden diese Schreibabläufe nicht ausgeführt. Die manuellen
Spieltests bleiben auf Nutzerwunsch verschoben. [Live-Ablauf](LIVE_APPLY.md).

Formatreferenzen, keine Codeübernahme:
[SteamKit DepotManifest](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs),
[Steam-Protobuf-Definition](https://github.com/SteamDatabase/Protobufs/blob/master/steam/content_manifest.proto).

Seit v0.4.10 weist die Inventur außerdem ausdrücklich bestätigte fremde Zusätze
separat aus. Diese bleiben fremd; sie sind keine Workbench-Dateien und keine
Vanilla-Originale. Unbestätigte, neue oder in der Größe abweichende Einträge bleiben
im Abweichungsbericht. [Bestätigung und Grenzen](FOREIGN_FILES.md).
