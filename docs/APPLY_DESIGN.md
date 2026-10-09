# B0 – Entwurf der eigenen Apply-Engine

Stand: 20.09.2026. Der Zielentwurf stammt aus Phase 0. Seit v0.4.8 existieren
Transaktionskern, geschützte Projektproben und ein eigener Live-Adapter für
Einrichtung, Apply/Reapply, Restore sowie Recovery. [Aktuelle Implementierung,
Vertrauensmodell, Basiswechsel und Grenzen](LIVE_APPLY.md).
Der echte Nutzerbericht und das Registry-Backup sind gespeichert. Eine Live-
Zulassung des Nutzerrechners wurde nicht vorgenommen. Manuelle Spieltests bleiben
bis nach Abschluss aller Entwicklungsphasen zurückgestellt und blockieren die
weitere Entwicklung nicht.
Verbindliche Grundlage: aktualisierte `SPEC.md` und die Bitte des Nutzers, während
des Spielens keine Spieldateien zu verändern. CDUMM ist ausschließlich eine
Recherchequelle; es wird weder installiert noch aufgerufen oder vorausgesetzt.

## Schreibgrenze

Nur B0 darf Spieldateien schreiben. Parser, Datenbank, Modmodule,
Profilwechsel und CLI-Analyse arbeiten mit gelesenen Bytes und Änderungen im
Arbeitsspeicher. Ein Profilwechsel plant dieselbe B0-Transaktion wie Apply.
Spielstände sind in allen Phasen ausschließlich lesbar.

Während Crimson Desert läuft, sind Apply, Restore, Reparatur und Wiederanwendung
gesperrt. Ein gefundenes Spiel wird weder beendet noch neu gestartet. Ein
Lesefehler bei der Prozessprüfung gilt nicht als Nachweis, dass das Spiel beendet
ist. Die Prüfung erfolgt vor der Planung und erneut unmittelbar vor dem Commit.
Ein einfacher Prozesscheck allein beseitigt das Rennen mit einem gleichzeitigen
Spielstart nicht: Für Phase 4 ist ein getesteter Windows-Schreibschutz-/Lock- und
Abbruchmechanismus nötig. Dieser ist inzwischen an eigenen Kopien integriert und
geprüft und an den vollständigen Inhaltsbericht mit allen darin erfassten EXEs angebunden.

## Vertrauenswürdiger Ausgangszustand

Ein frisch berechneter Hash ist eine Beobachtung, kein Beweis für Vanilla.
Dateiversion, Steam-Build-ID oder ein fehlender Modordner reichen ebenfalls nicht.
Eine bekannte Builddefinition muss die erwarteten Ausgangshashes der betroffenen
Metadaten und Originalarchive enthalten oder aus einer unabhängig bestätigten
Vanilla-Installation stammen. Backups werden nach dem Kopieren erneut gehasht.
Die Phase-0-Beobachtung wird ausdrücklich nicht automatisch in eine erlaubte
Builddefinition umgewandelt.

Der Vanilla-Buildfingerprint wird aus den ursprünglichen, zugeordneten Archiven
bestimmt. Ein eigenes Overlay erhält ein separates Inhalts-/Besitzmanifest;
dessen erzeugte Bytes dürfen weder die Vanilla-Basis ersetzen noch bei erneutem
Apply als unbekannter neuer Spielbuild fehlklassifiziert werden. Effektiv geladene
Tabellen und originale Basis bleiben bei Anzeige und Diagnose unterscheidbar.

Unbekannte Builds, veränderte Ausgangsarchive und fremde Gruppen verhindern Apply.
Die Spec erlaubt eine Rückfrage zu Fremdmods; eine Bestätigung hebt die Forderung
nach nachgewiesenem Vanilla und getesteten Schemas nicht auf. Ein späterer
Koexistenzmodus braucht eine gesondert überprüfte Ausgangsbasis und Konfliktplan.
Steam-Dateiprüfung kann als manuelle Abhilfe genannt werden, wird aber nicht
während des Spielens automatisch gestartet.

Der Cache wurde für v0.4.2 genauer geprüft: Beide zum installierten Build
gehörenden Depotmanifeste haben leere Signaturabschnitte. Sie bieten Dateihashes,
aber bisher keinen unabhängig authentifizierten Ursprung. Details und IDs:
[B1/B2-Feldnachweise und Steam-Cachebefund](research/PHASE4_FIELDS.md).

## Geplante Transaktion

1. Schreibsperre pro Installation erwerben; Spielprozess und Rechte prüfen.
2. Build, Vanilla-Nachweis, Backupzustand und fremde Änderungen prüfen.
3. Tabellen aus Vanilla lesen, alle Module deterministisch zusammenführen und
   überlappende widersprüchliche Änderungen mit Feld/Offset melden.
4. Dry Run erzeugen: fachliche Änderungen und vollständige Liste der erzeugten,
   ersetzten oder entfernten Dateien samt Ausgangs-/Zielhash und benötigtem Platz.
5. Eigene Overlaygruppe in einem Stagingbereich erstellen; PAMT, Offsets,
   Kompression, Prüfsummen und Rücklesen prüfen. Gruppen-ID nicht fest einbauen,
   sondern Kollisionen und belegte IDs berücksichtigen. Auch in PAPGT registrierte
   IDs ohne vorhandenen Ordner sind belegt (etwa optionale Sprachpakete).
6. Journal und überprüfte Sicherung vor dem ersten Spielverzeichnis-Schreibzugriff
   dauerhaft speichern. Erwartete Vorher-Hashes direkt vor dem Commit erneut prüfen.
7. Nach erneuter Spielprüfung Overlaydateien auf demselben Volume vorbereiten und
   vollständig bereitstellen; Gruppenregistrierung zuletzt ersetzen.
8. Alle Zielhashes verifizieren, Journal abschließen und nächsten Spielstart als
   Zeitpunkt der Wirkung anzeigen.

Tempdatei plus Rename ist nur für **eine Datei** atomar. Eine Gruppe aus PAZ,
PAMT und PAPGT ist dadurch noch keine atomare Transaktion. Crash-Recovery benötigt
ein versioniertes Journal mit Zuständen, dauerhafte Flushes, eine klar bestimmte
Commit-Grenze und Wiederanlauf-/Rollbacktests an jeder Schreibgrenze. Recovery
prüft ebenfalls zuerst, ob das Spiel läuft; sie schreibt dann gegebenenfalls erst
nach dessen Beendigung. Der genaue Windows-Mechanismus wird in Phase 4 umgesetzt.

## Wiederanwendung und Restore

Jedes Apply baut von Vanilla plus aktuellen Einstellungen neu. Eigene bereits
modifizierte Tabellen sind niemals die Basis eines weiteren Apply.

Restore prüft, dass registrierte Gruppe und Registry noch unserem letzten Commit
entsprechen, setzt die passende Registry-Sicherung zurück, entfernt ausschließlich
unsere nachweislich eigenen Dateien und prüft alle betroffenen Ausgangshashes.
Fremde Änderungen seit Apply führen zum Abbruch; sie werden nicht durch eine alte
Sicherung überschrieben. Unterbrechungen müssen wiederanlaufbar sein.

Bei einem Spielupdate werden alter Snapshot und Journal aufbewahrt. Es wird nie
blind eine alte `0.papgt` über den neuen Build kopiert. Ein unverändert gebliebenes
Overlay ist ebenfalls nicht automatisch kompatibel. Neue Schemas und ein neuer
Vanilla-Nachweis sind Voraussetzung für ein angebotenes erneutes Apply. Falls
ein unbekannter Zustand nicht sicher automatisch zu bereinigen ist, bleibt B0
gesperrt und benennt die nötige manuelle Wiederherstellung.

## Ausstehender Nachweis

Die Referenzimplementierung und die lokale Archivstruktur können die technische
Machbarkeit eines Overlays begründen. Dass jede unserer Tabellen über eine eigene
Gruppe tatsächlich Vorrang erhält und das Spiel damit startet, muss zusätzlich
mit einem kontrollierten Test nachgewiesen werden. In Phase 0 werden dafür keine
Spieldateien geändert. In-place-Patching bleibt bis zu einem belegten Scheitern
des Overlays und vollständig getesteten Sicherungs-/Restorepfaden deaktiviert.

CDUMM stellt seine Gruppenregistrierungen vor vorhandene Einträge und beschreibt
die Auflösung als „first match wins“. Eine höhere numerische Gruppen-ID allein
ist daher kein belastbarer Vorrangmechanismus. Tabellen-Overlays benötigen auch
die zugehörigen `.staticinfoheader`; bei geänderter Datensatzgröße müssen deren
Offsets neu berechnet werden. Quellen und technische Details:
[`research/APPLY.md`](research/APPLY.md).

NattKhs `MODDING_GUIDE.md` dokumentiert zusätzlich einen älteren In-game-Fehler,
wenn `iteminfo` und `equipslotinfo` in derselben Gruppe liegen. Der Gruppenplan
muss deshalb mehrere eigene Gruppen unterstützen, sollte `equipslotinfo` später
benötigt werden. Die damalige Trennung ist ein Testfall für den aktuellen Build,
keine bereits auf dieser Installation bestätigte Eigenschaft. Ebenso sind die
dort genannten freien IDs ab 0036 und die Sprachmaske `0x3fff` veraltet: Die
lokale Registry enthält reservierte IDs bis 0040 und die Maske `0x7fff`.

Spielinterne Integritätswerte sind Jenkins-`hashlittle`-Checksummen und werden
separat von den SHA-256-Hashes für Buildvergleich, Besitznachweis und Backupprüfung
behandelt. Der geplante Tabellen-Overlaypfad benötigt keine Änderung an
`meta/0.pathc`; Textur-Overlays sind kein Bestandteil von B0 in dieser Spec.
