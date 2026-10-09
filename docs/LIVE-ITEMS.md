# Item-Auswahl und Live-Spawner

Stand: 03.10.2026. **Auswahl und automatische Live-Ausgabe sind implementiert, außerhalb des Spiels getestet und bei geschlossenem Spiel installiert. Zusätzliches Aufheben ist nicht mehr vorgesehen. Die Abnahme im echten Spiel steht noch aus.**

Öffnen: Desktop-Verknüpfung **Crimson Workbench**, Navigation **Item-Auswahl**.

## Erweiterung vom 03.10.2026 – installiert am 04.10.2026

**Weiteres Update 04.10.2026, installiert und technisch geprüft; Spielabnahme offen:** Teilwortsuche für alle
Items, optionaler Regex-Modus in Itemdatenbank und Item-Auswahl, größere Icons
und anklickbare Bildvorschau. Suchmuster sind auf 512 Zeichen begrenzt. Ungültige
oder nicht unterstützte Muster zeigen eine Fehlermeldung; ohne Regex werden
Eingaben als normale Teilwörter gesucht. Kategorien, Sortierung und Seiten
bleiben kombinierbar. Reine Zahlen suchen weiterhin eine exakte Item-ID.

Blitzpfeil **1001315**, Feuerpfeil **1001316** und Kältepfeil **1001314** sind in
den geprüften Itemdaten ausdrücklich als Monstermunition gekennzeichnet. Sie
können im Inventar vorhanden sein, ohne als reguläre Spielermunition zu funktionieren.
Die Workbench zeigt deshalb einen Hinweis und sperrt ihre Ausgabe vor einem
nativen Aufruf. Nutzbare Alternativen sind **Pfeil 50001**, **Giftpfeil 50003**
und **Explosionspfeil 1001321**; bei letzterem ist Spielernutzung ausdrücklich
in den Itemdaten vermerkt. Bereits vorhandene NPC-Pfeile werden nicht entfernt.

Neue ASI und Desktop-Version bei geschlossenem Spiel/Workbench ersetzt und
unabhängig zurückgelesen: 36 bestehende Dateien einschließlich 13 aktueller
Savedateien unverändert, 23 Backups geprüft. Die bestehende Desktop-Verknüpfung
öffnet die neue Version. Das Installieren vergibt keine Items oder Tiere.
Nachweis: `.local/live-items-bag-mounts-20261003/items-final-verification.json`.
Spielabnahme der neuen Funktionen bleibt offen.

Der allgemeine Fehler bei Extragroßer Tasche **6003** und Blitzpfeil **1001315**
kommt von einem optionalen Definitionsfeld mit Wert `65535`, das fälschlich als
ungültiges Item behandelt wurde. Der bestätigte native Standardpfad wird nun
zugelassen. Unbekannte IDs, Definitionsabweichungen und zu viele voreingestellte
Sockel erhalten getrennte Meldungen. Der tatsächliche native Konstruktor und
Konverter wurden mit beobachteten Standardwerten im eigenen Prozess für beide
Items geprüft; die Inventar-/Speicherabnahme im Spiel steht aus.

Das Mengenfeld erlaubt Leeren, Tippen und Einfügen mit Prüfung auf ganze Zahlen
von 1 bis 10.000. Eine Schätzung nutzt das Basiskatalog-Stapellimit: 100 Stück
bei Limit 10 ergeben höchstens zehn neue Stapel, vorhandene Stapel kann die
native Vergabe auffüllen. Mods können das Limit ändern. Keine zusätzlichen
Vergabeanfragen oder automatischen Wiederholungen zur Stapelaufteilung; ein
unklarer oder teilweiser Auftrag bleibt zur Prüfung gesperrt.

Die Wissensanzeige liest den **ausgewählten gespeicherten Spielstand**. Sie nutzt
typisierte Wissensbelohnungen in Itemdaten und den gespeicherten Wissenslevel,
keine bloßen Lesebedingungen einer Buchseite. Zustände: bereits erlangt, noch
nicht erlangt, teilweise, unbekannt. Dokumente ohne eindeutige Zuordnung zeigen
unbekannt. Nach Lesen im Spiel speichern und **Wissen neu einlesen**. Bei
beschädigten oder unvollständig lesbaren Dateien wird kein Status erfunden.
Der Abgleich mit einer gelesenen/ungelesenen Schrift im Spiel steht aus.

Neue Auswahl **Reittiere** mit Suche, Familien, Beschreibung und Besitzstatus:
[Registrierung und Grenzen](REITTIERE.md). Registrierung erfolgt bei geschlossenem
Spiel mit frischer Sicherung und exaktem Erhalt vorhandener Savebereiche.
Die Installation dieses Features vergibt keine Tiere oder Items.

Die Folgeerweiterung für zugewiesene, gesunde Vorlagentiere und zusätzliche
Tierporträts ist installiert und unabhängig geprüft; Spielabnahme steht aus.
299 Einträge haben ein Bild (89 exakte Porträts, 210 beschriftete Beispielbilder).
Die neue Kopie übernimmt weder Name noch aktive Charakterzuweisung; das
Original bleibt erhalten. Eine weitere, technisch geprüfte Erweiterung bietet
Basiseinträge ohne Familienvorlage; das Desktop-Update ist installiert. Die Oberfläche
kennzeichnet ihre noch unbestätigte Spielinitialisierung und Reitfunktion.
Details und Grenzen stehen in [Reittiere](REITTIERE.md).

Nachweise: `.local/live-items-bag-mounts-20261003/`. 155 Core-Tests, 14 Codec-Tests,
11 Frontend-Unit-Tests und zehn gezielte Browser-Tests bestanden. Native
Bag-/Arrow-Standardpfade, Queue/Kontext, automatische Anbindung, echte Rust/C++-
Pipe und gemeinsamer Loader in eigenen Prozessen geprüft. Echte private
Savekopien bestehen Wissenslesen, je eine Bären-/Pferderegistrierung, frische
Tier-/Itemnummern, verschlüsselte Rücklesung und vollständigen Rückvergleich.
Diese Prüfungen ersetzen keine Spielabnahme.

## Implementiert

- 6.816 Gegenstände aus der geprüften lokalen Installation, ohne externe Item-API.
- Icons aus den Spielarchiven; ein Platzhalter bei fehlender Bildreferenz.
- Beschreibung in Liste und Detailansicht, numerische Tabellenwerte und Auswahl der Verfeinerungsstufe.
- Fünf Hauptkategorien und benannte Untergruppen aus `ItemGroupInfo`. Die Gruppenzuordnung verwendet dessen Itemlisten, nicht geratene Item-Kategorien.
- Suche nach Namen, Beschreibung, interner ID und exakter numerischer Item-ID; Kombination mit Kategorien.
- Virtuelle Liste und Seiten mit je 200 Ergebnissen. Veraltete Such- und Detailantworten können neuere Ergebnisse nicht überschreiben.
- Mengeneingabe und echter Befehl **„Ins Inventar geben“**. Der Button wird nur bei verbundenem Live-Modul aktiv; ohne passende Runtime bleibt er gesperrt.
- Lesender Datenabgleich für Build 2976: 38 Metadateien, 28 Kern-Tabellendateien sowie alle verwendeten zusätzlichen Tabellen und Lokalisierungen geprüft. Kein Laufzeit-Schreibnachweis und keine Vanilla-Zertifizierung.

Die Auswahl liest die **Grunddaten**. Installierte Tabellenänderungen, individuelle Sockel, Charakterboni und aktuelle Inventarinstanzen sind darin noch nicht eingerechnet. Bei einer außerhalb des Workbench-Journals veränderten Registry ist ausschließlich der vollständig hashgeprüfte originale Basis-Katalog zugänglich. Die Datei-Schreibpfade behalten ihre bisherigen Besitzprüfungen.

## Diagnose-Mod

`CrimsonLiveItemsProbe.asi` ist bei geschlossenem Spiel installiert. Er überprüft die vollständige EXE und die vollständige beobachtete Inventarroutine des Builds 2976, bevor er einen Hook installiert. Der Hook beobachtet nur den Aufrufkontext und leitet jeden ursprünglichen Aufruf genau einmal weiter. Er hält keine Spielobjekt-Zeiger über den Aufruf hinaus und hat keinen Item-Geben-Befehl.

Beim nächsten normalen Spielstart mit geladenem Spielstand kann der Mod Kontextbeobachtungen nach `bin64/CrimsonLiveItemsProbe.log` schreiben. Ein `READY` bedeutet nur, dass der lesende Hook eingerichtet wurde. Auch eine erfolgreiche Beobachtung gibt noch keine Schreibfreigabe.

Während der Installation wurden 32 bestehende Spiel-/Save-Dateien unverändert geprüft und 18 Dateien frisch gesichert. Registry, Spiel-EXE und Spielstände wurden nicht bearbeitet. Installation und Tests liegen unter `.local/live-items-20261003/`.

## Installiertes Live-Modul

`CrimsonLiveItems.asi` verwendet die nativen Erzeugungs- und Inventartransaktionsroutinen des Builds 2976. Die vollständige EXE und die verwendeten zentralen Funktionen werden vor dem Hook geprüft. Neue Itemdaten entstehen mit dem nativen Konstruktor und dessen vollständiger InitData-Konvertierung; Haltbarkeit und voreingestellte Eigenschaften kommen aus den aktuell geladenen Definitionen.

Das erste Live-Modul wurde bei vollständig geschlossenem Spiel installiert und zurückgelesen. Die automatische Version ersetzt jetzt dieses Modul und `target/release/crimson-workbench-live.exe`, ebenfalls bei geschlossenem Spiel und geschlossener Workbench. Die bestehende Desktop-Verknüpfung öffnet damit die neue Version. 36 bestehende Dateien einschließlich aller 13 aktuellen Savedateien und der Verknüpfung blieben unverändert; 23 Dateien wurden gesichert. Beide ersetzten Dateien sind als `auto-install-originals/replaced-0.bin` (ASI) und `replaced-1.bin` (Workbench) erhalten. Aktuelle Installationsnachweise und Backups: `.local/live-items-autopump-20261003/`; erste Version: `.local/live-items-20261003/`.

Anfragen werden automatisch nach einem passenden Server-Charakterauftrag verarbeitet. Die beiden geprüften Aufrufwege halten eine native Referenz auf dessen Besitzer bis nach der Ausgabe. Der Originalauftrag läuft genau einmal; seine Argumente, sein Fehlercode und sein Ergebnis bleiben erhalten. Geprüft werden Server-Realm, Kontextpool, native Objektklasse, aktiver Spieler, vollständige Charakterkennung und Rückverweise zwischen Besitzer, Steuerung und Inventar. Keine Objektzeiger werden für spätere Aufrufe gespeichert und keine TLS-Werte im Spiel geändert. Ein Rekursionsschutz verhindert eine weitere Ausgabe innerhalb einer verschachtelten Spielaufgabe.

Die Statusabfrage prüft die ausgewählte Figur ausschließlich lesend. So bleibt die Auswahl auch bei Alt-Tab/Pause nutzbar, wenn dieselbe zuvor bestätigte Figur weiterhin aktiv ist. Ein Auftrag wird erst auf dem Spielthread mit gültiger nativer Referenz ausgeführt. Bei geänderter oder fehlender Charakterkennung werden wartende Aufträge abgewiesen. Ein längerer Kontextabbruch ohne lesende Bestätigung macht die Verbindung erneut unbereit. Diese Version unterstützt den geprüften SelfPlayer-Kontext; andere Spielertypen werden nicht ungeprüft zugelassen.

Die Workbench spricht eine lokale Named Pipe des bestätigten Spielprozesses an. Nur derselbe Windows-Benutzer darf sich verbinden; Remote-Verbindungen sind abgewiesen. Eine Anfrage enthält Item-ID, Menge, UUID und Spielinstanz. Die Runtime verhindert erneutes Geben derselben UUID, manipulierte Anfragen, Aufträge für eine andere Spielinstanz und parallele Schreibaufträge. Die Workbench speichert offene IDs vor dem Senden und fragt nach einem Verbindungsfehler dieselbe ID ab, statt erneut zu geben.

### Bedienung

1. Spiel normal starten, Spielstand laden und kurz ins normale Spiel zurückkehren. Das Modul bestätigt den Charakter automatisch; du musst nichts aufheben.
2. In **Item-Auswahl** Item und Menge wählen, **Ins Inventar geben** anklicken.
3. Falls das Spiel pausiert ist oder im Hintergrund keine Charakteraufträge verarbeitet: kurz ins Spiel zurückkehren. Die Anfrage wird beim nächsten passenden Charakterauftrag automatisch verarbeitet, ohne zusätzliches Aufheben oder einen Inventarvorgang.
4. Die Runtime bestätigt die zusätzliche Menge anhand des Inventars. Danach normal im Spiel speichern. Eine bestätigte Menge ist noch kein Nachweis, dass ein späteres Speichern und Neuladen erfolgreich war.

Wartende Anfragen lassen sich abbrechen und verfallen ohne Ausführung nach 30 Sekunden. Bei einem nativen Fehler oder einer abweichenden Menge wird das Ergebnis als unklar markiert und weiteres Geben in dieser Spielinstanz gesperrt. Keine automatische Wiederholung.

Grenzen: 1–10.000 Stück pro Anfrage; bei Itemarten ohne das bestätigte native Mengenflag bleibt eine Grenze von 100 bestehen. Dieses Flag ist kein sicherer Nachweis für fehlende Stapelbarkeit. Fünf voreingestellte Sockel beim Erzeugen; Items mit darüber hinaus veränderten Sockeldefinitionen werden vor dem Erzeugen abgewiesen. Die Auswahl zeigt weiterhin Basistabellenwerte; die Runtime verwendet tatsächlich geladene Definitionen. Das Protokoll behält maximal 512 Anfrage-IDs bis zum Spielneustart, ohne alte IDs zu vergessen.

## Noch offen

1. Automatischen Charakterkontext im echten Spiel bestätigen: In `CrimsonLiveItems.log` müssen die neue `ARMED`-Zeile und eine `CONTEXT`-Zeile erscheinen. Die alte Diagnose allein belegt keinen funktionierenden Grant-Hook.
2. Abnahme mit **einem Eisenerz**: ohne zusätzliches Aufheben geben, zusätzliche Menge im Inventar sehen, normal speichern, Spiel vollständig schließen und denselben Spielstand neu laden.
3. Danach Stapeln, volles Inventar, Pause/Alt-Tab, Laden/Charakterwechsel, Abbruch, 30-Sekunden-Verfall und weitere Itemarten prüfen. Erst nach dem Speichern-/Laden-Nachweis höhere Mengen verwenden.

## Bereits geprüft

- 150 Core-Tests bestanden, ein bestehender Test ignoriert.
- Echter Katalogtest: alle 6.816 Items, DDS-Icon, Kategorien, Kategorie-/Text-/ID-Suche, zusätzliche Ausrüstungswerte und Export.
- Acht Browser-Tests: bestehender Katalog und neue Auswahl, Icons/Beschreibung, Verfeinerungswerte, Pagination, überholte Suchantworten und Layout bei 1024 Pixeln.
- Produktions-Frontend und Desktop-EXE gebaut; Desktop-Verknüpfung aktualisiert.
- Probe: lesende Objektprüfung einschließlich ungültiger Zeiger, exakt einmalige Weiterleitung und unveränderte beobachtete Bytes. Bestehender ASI-Loader lädt alle fünf Mods; eine fremde EXE wird vor dem Hook abgewiesen.
- Neue Runtime: exakt einmalige Weiterleitung aller 13 Transaktionsargumente, konkurrierendes Dequeue, UUID-Wiederholung, abweichender Inhalt, falsche Spielinstanz, Abbruch, Verfall und Sperre nach unklarem Ergebnis geprüft.
- Zwölf Fälle mit tatsächlichem, hashgeprüftem Build-2976-Code in einem eigenen Testprozess: Konstruktor, vollständige InitData-Konvertierung und Destruktor; 0–5 Sockel, Menge 1/1.000, native Standardhaltbarkeit, neue Item-ID und Speichergrenzen. Definitionen/Allocator sind Testdaten; dies ist kein Test der realen Inventar-/Speicherroutine.
- Echte Windows-Pipe zwischen Rust und C++ im eigenen Testprozess: Status, Geben/Wiederholung, Ergebnisabfrage, Abbruch und Ablehnung einer anderen Spielinstanz. Keine Verbindung zum laufenden Spiel.
- Elf Browser-Tests, sieben Frontend-Unit-Tests und fünf Desktop-Unit-Tests bestanden. Gemeinsamer Loader mit allen sechs ASIs und Ablehnung einer fremden EXE geprüft.
- Installation der neuen Runtime und Desktop-Version zurückgelesen; bestehende Spielstände, Registry, Mods und EXE unverändert geprüft. Desktop-Verknüpfung zeigt auf die neue Version.
- Automatische Ausgabe: tatsächlicher Build-2976-Taskdispatcher, dessen Wrapper und der MinHook-Trampoline in einem eigenen Testprozess ausgeführt. Native Referenz bleibt über den Callback erhalten; Argumente/Fehler/Originalaufruf, verschachtelter Auftrag, UUID-Wiederholung, falsche Figur/Realm, fehlender Pool, Kontextabbruch, Pause und Fehler-Sperre geprüft. Registry/Referenzfreigabe und Inventar sind Testdaten; dies ist noch kein echter Spiel-Inventar-/Speichertest.
- Alle 13 aktuellen Code-/Aufrufweg-Pins geprüft; Windows-Pipe mit Protokoll 2 getestet. Fünf Desktop- und sieben Frontend-Unit-Tests bestanden; Produktionsbuild und gemeinsamer Loader mit allen sechs ASIs geprüft.
- Update mit geschlossenem Spiel/Workbench, gesperrtem Spielstart, aktuellen Backups und Zurücklesen durchgeführt. Der erste Austausch wurde von Windows wegen offener Zielhandles verweigert; die alten Dateien blieben unverändert. Der korrigierte Austausch hält die Spiel-EXE bis zum Abschluss exklusiv gesperrt.

## Rückbau der Diagnose

Bei vollständig geschlossenem Spiel kann ausschließlich `bin64/CrimsonLiveItemsProbe.asi` entfernt werden. Die vier bestehenden Mods und die Registry bleiben dabei bestehen. Das Diagnose-Log kann für die weitere Analyse aufbewahrt werden.
