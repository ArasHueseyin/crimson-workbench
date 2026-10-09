# Crimson Workbench – Was ist fertig, was fehlt?

**Sockelstein-Anzeige, Phase 5, 05.10.2026:** Zusatzsockel zeigt in der
Auswahlliste die Grundboni bzw. Spezialeffekte. Beim ausgewählten Stein stehen
alle Werte, Effektstufen und die Beschreibung; bereits bestückte Zusatzsockel
zeigen ebenfalls ihre Boni. Angriff/Verteidigung werden aus Tausendsteln
umgerechnet, Geschwindigkeit und kritische Trefferchance als Stufen angezeigt.
190 Steine aus dem geprüften lokalen ItemInfo gelesen: 30 mit direkten Werten,
116 mit benannten Buff-Effekten, 44 mit Fähigkeitsbeschreibungen. Unbekannte
Felder erhalten eine explizite Rohwert-/Buff-ID-Anzeige; keine erfundenen
Prozentwerte. Die Anzeige nutzt Grunddaten, berechnet keine Endwerte mit allen
Charakter-/Mod-Einflüssen. Lesefehler mit Wiederholen; keine Spielstandzugriffe.
17 Frontend-Tests, 10 Zusatzsockel-UI-Tests und lokale Katalogprüfung bestanden.
Über die bestehende Desktop-Verknüpfung installiert; nur Workbench-EXE ersetzt.
Schreibvorgänge an Spielständen/Spieldateien fanden dafür nicht statt.
Die tatsächliche Wirkung der zusätzlichen Sockel im Spiel bleibt separat offen.

**Laufende Korrekturprüfung, 05.10.2026:** Spiel gestartet und Save geladen.
Rein lesend im laufenden Prozess bestätigt: Client **und** Server haben beim
Panzer 10, Helm 11 und Mecha-Schwert 15 physische/offene Plätze, jeweils Menge 1.
Basissockel auf beiden Seiten identisch, Helm/Schwert weiterhin Verfeinerung 10.
Der beschädigte Client-Prefix 255 ist in diesem Lauf behoben. Keine Spiel- oder
Save-Datei dafür geändert. Visuelle Bestätigung, Bestückung/Wirkung und
Speichern/Neuladen bleiben offen. Sockeländerungen sind bei laufendem Spiel
absichtlich gesperrt; Workbench muss dafür nicht geschlossen werden.

**Zusatzsockel und Korrektur, 04.10.2026:** Workbench → **Zusatzsockel** kann
weitere eigene Waffen, Rüstungen und Accessoires aus einem ausgewählten Save
anzeigen und geeignete Instanzen jeweils einmal um **zehn leere Sockel**
erweitern. Suche, Icons, Instanz-ID und bestehende Sockelanzahl; konkrete
Sperrgründe. Im neuesten Save 329 Instanzen gelesen, 326 neu erweiterbar.
Bis zu 256 konfigurierte Instanzen; keine Bindung neuer Einträge an Toms IDs.

Die erste Installation startete und lud, verursachte aber beschädigte
Client-Anzeigen (Menge 4294903295, geöffnete Sockel 255). Ursache: der separate
Runtime→InitData-Konverter hat nur fünf inline Sockel und wurde zunächst nicht
abgefangen. Die Korrektur normalisiert **sowohl Save als auch Netzwerk-InitData**
auf das originale Format; der Client stellt danach die Zusatzplätze wieder
her. Kompatibilität mit der Startup-Prüfung von LiveItems berücksichtigt.
Korrigierte Mod und Desktop-EXE bei geschlossenem Spiel/Workbench installiert.
Alle 13 Saves und 22 geprüften Bestandsdateien unverändert; alte Zusatzdatei
erhalten. Panzer weiterhin **10**, Schattenhelm **11**, Mecha-Schwert **15**.

86 isolierte native Fälle (inklusive des vollständigen originalen Netzwerk-
Konverters mit Basissockeln 0–5 und beschädigtem Prefix), 22 Policy-/Dateifälle,
163 Core-Unit-Tests, elf Frontend-Unit- und sieben Browsertests bestanden.
**Die Korrektur, Wirkung und Save-/Reload-Zyklus benötigen noch den Spieltest.**
Die Spieloberfläche zeigt weiterhin höchstens fünf Sockel; die weiteren werden
in Workbench bestückt. Weitergabe braucht eigene Installation, Mods und
Gegenstandszuordnung; ein portabler Installer/zweiter Rechner ist noch offen.
[Voraussetzungen für Weitergabe](docs/WEITERGEBEN.md).
Allgemeine Phasenentwicklung bleibt pausiert.
[Umsetzung und Spieltests](.local/extra-sockets-plus10-20261004/RESULTAT.md).

**Separater Mecha-Sockelauftrag, 04.10.2026:** Beim ausgerüsteten
Elektro-Mecha-Langschwert +10 im neuesten `slot0` den fünften, bisher gesperrten
Sockel geöffnet. Genau ein Datenbyte geändert; vier bestehende Steine und alle
anderen Felder erhalten. Nach frischer Prüfung bei geschlossenem Spiel und
Workbench mit Originalbackup installiert und unabhängig zurückgelesen;
zwölf andere Save-/Lobby-Dateien unverändert. **Belegen bei der Hexe, Wirkung
und Speichern/Neuladen noch im Spiel zu prüfen.** Damaliger Forschungsstand
vor der obigen Zusatzsockel-Installation: 32 isolierte
native Fälle für Verarbeitung, Kopieren und eine angepasste Speicherschleife
sowie eine vollständige private Zehn-Eintrag-Formatprobe bestanden. Ein
konkreter nativer Ansatz für zehn Sockel wurde gefunden; damals blieben Ladepfad,
Speicherzyklus und Oberfläche offen. Die spätere Umsetzung steht oben.
Allgemeine Phasenentwicklung bleibt pausiert.
[Ergebnis und Spieltests](.local/mecha-sockets-20261004/RESULTAT.md).
[Machbarkeitsnachweise für zehn Sockel](.local/mecha-extra-sockets-20261004/RESULTAT.md).

**Separater Angel-/Fallschutzauftrag, 03.10.2026:** Eine **Krabbenangelrute**
mit dem vorhandenen Effekt **automatisches Einholen** im neuesten `slot2`
ergänzt; 999 Plätze und Fortschritt erhalten. Neue **CrimsonNoFall.asi**
bei geschlossenem Spiel mit Startschutz installiert: gezielter Fall-HP-Pfad
für Kliff, Damiane, Oongka und 339 Reittier-Definitionen geschützt. Vollständige
native Routine, 684 Schutzfälle, Weiterleitung für übrige Figuren und gemeinsames
Laden mit älteren Mods geprüft. 31 bestehende Dateien unverändert; 17 frische
Sicherungen vor der ASI-Installation. **Echter Spieltest offen. Zehn Köder offen:**
kein entsprechendes Item in der Tabelle gefunden; tatsächlicher Itemname erfragt.
Allgemeine Phasenentwicklung bleibt pausiert.
[Ergebnis, Sicherungen und Tests](.local/fishing-no-fall-20261003/RESULTAT.md).

**Separater Reittierauftrag, 03.10.2026, 01:50 Uhr:** Löwe und Rokade erhalten
Bewegungsmultiplikator **1,30**, Ausdauerkostenreduktion **100 %** und starke
Ausdauerregeneration. Beim Löwen maximale/initiale HP von **450 auf 1.350**;
Rokades Boni in allen fünf Stufen. Bei geschlossenem Spiel mit Startschutz
als Archivgruppe `0049` installiert. Nur zwei von 7.250 Charakterdatensätzen,
20 Zahlenfelder geändert; zwölf native Leseprüfungen und zwölf Ablehnungsfälle
bestanden. Alle zwölf Save-/Lobby-Dateien und ältere Mods unverändert.
**Tatsächliche Tempo-/Ausdauerwirkung und HP im Spiel noch zu prüfen.**
[Ergebnis und weitere Ideen](.local/lion-rokade-upgrades-20261003/RESULTAT.md).

**Separater Questauftrag, 28.09.2026:** „Anfrage von Ben“, Schritt „Schlachte ein
Tier vom Viehhof“, im neuesten `slot1` eindeutig identifiziert. Zwölf Save-/Lobby-
Dateien gesichert und unverändert geprüft. **Kein künstlicher Abschluss**, gemäß
Nutzerbedingung „nur wenn sicher“: vollständiger Abschlussablauf und Folgen nicht
verifiziert. Reguläre Alternative über Bens Viehhofverwaltung dokumentiert.
[Prüfung und Sicherung](.local/greymane-slaughter-20260928/RESULTAT.md).

**Separater Sockel-/Schlafauftrag, 28.09.2026, 00:29 Uhr:** Zusätzliche ASI-Mod
für **30 Spielstunden über die bisherige 12-Stunden-Auswahl** installiert;
Beschriftung bleibt 12 Stunden. 288 native Ereignis-/Kalenderfälle und gemeinsames
Laden mit der Ein-Sekunden-Mod bestanden. 17 bestehende Dateien unverändert,
neun Save-Verzeichnisdateien gesichert. **Spieltest noch offen.**
**Fünf zusätzliche Sockel pro angelegtem Item und ihre Befüllung nicht umgesetzt:**
aktive EXE lädt/speichert fünf Einträge und erstellt fünf UI-Zeilen.
18 angelegte Items geprüft, passende III-Steine als Kandidaten dokumentiert;
kein Save und keine vorhandenen Sockel geändert. Nach erneuter Prüfung gemäß
Nutzerentscheidung „wenn möglich, ansonsten lassen“ wird die ungesicherte
Zusatzsockelerweiterung samt Befüllung vorerst belassen. Die allgemeine Phasenentwicklung
bleibt pausiert. [Ergebnis und offene Arbeiten](.local/sockets-plus5-sleep30-20260928/RESULTAT.md).

**Separater Schlafauftrag, 27.09.2026, 20:29 Uhr:** Eigene ASI-Mod für
**1 Sekunde reale Schlaf-/Wartesperre** bei geschlossenem Spiel installiert.
Zeitgrenzen, bestehende Sperren, beide nativen Funktionen mit Testdaten und
automatisches Laden isoliert geprüft. Zwei neue Dateien; EXE, vorhandene Mods
und Spielstände unverändert. Acht Save-/Lobby-Dateien plus Steam-Metadatei gesichert.
**Echter Spielstart und Schlaf-/Wartetest noch offen.** Die Mod prüft die exakte
EXE-Version und deaktiviert ihre Hooks bei Abweichung. Keine Workbench-UI-Integration;
allgemeine Phasenentwicklung weiterhin pausiert.
[Ergebnis, Sicherung und Testanleitung](.local/sleep-cooldown-1s-20260927/RESULTAT.md).

**Separater Inventar-/Questauftrag, 27.09.2026, 15:09 Uhr:** +250 kleine Knochen
in `slot1`, vorher 3, jetzt 253; fünf Stapel zu 50 ergänzt. Vollständiger
Vergleich, inverse Byteprüfung, Startschutz und Rücklesen bestanden.
999 Plätze und Fortschritt erhalten. Gewünschte Freischaltung der Graumähnen-Reihen
**nicht umgesetzt**: Startbedingungen der aktuellen Version nicht vollständig
dekodierbar; spätere Questgruppen fehlen teilweise komplett im Save.
„Camperweiterung“ ist bereits offen, alle fünf Spendenquests sind abgeschlossen.
[Ergebnis und Sicherung](.local/greymane-unlock-bones-20260927/RESULTAT.md).

**Separater Inventarauftrag, 27.09.2026, 14:39 Uhr:** Im neuesten `slot2`
sind **1.616 hervorragendes Holz**, **821 Elfenbein**, **Mal der Finsternis +3**
und **Demeniss-Siegel +0** zusätzlich im Rucksack. Holz/Elfenbein decken die
Endziele der noch offenen Holz-/Waffenspenden ab und enthalten je 300 Reserve.
Geld, Nahrung und Stein sind laut Missionszuständen bereits abgeschlossen.
Vollständiger Vergleich, inverse Byteprüfung, Startschutz und Rücklesen bestanden;
Spenden-/Ringabnahme im Spiel offen. 999 Plätze und Fortschritt erhalten.
[Ergebnis, Spendenmengen und Sicherung](.local/camp-donations-reserve-20260927/RESULTAT.md).

**Separater Inventarauftrag, 27.09.2026, 14:27 Uhr:** Im neuesten `slot1`
sind je **400 Bauholz, hochwertiges Holz und hervorragendes Holz** sowie
**100 Große Mahlzeit** eingetragen. Die Mahlzeiten entsprechen laut aktiver
Tabelle 64.800 Nahrungspunkten für „Ein Tisch voller Überfluss“ und müssen
bei Carl gespendet werden. 29 neue Stapel; 999 Plätze und Fortschritt erhalten.
Vollständiger Vergleich, inverse Prüfung, Startschutz und Rücklesen bestanden;
Spenden-/Questabnahme im Spiel offen. Schmuckauswahl erstellt, Schmuck nicht ergänzt.
[Ergebnis, Auswahl und Sicherung](.local/camp-provisions-wood-20260927/RESULTAT.md).

**Separater Inventar-/Tempoauftrag, 27.09.2026, 02:51 Uhr:** Im neuesten `slot2`
sind alle **41 Abyss-Sockelitems III jeweils einmal zusätzlich** eingetragen.
Zusätzlich steckt **Eile III** in Kliffs angelegten Frostfluch-Plattenschuhen;
**Hingabe I** wurde in den Rucksack zurückgegeben. Ausrüstungsbeitrag nun rechnerisch
15 Bewegungsstufen; als erlaubtes Ausweichziel wurde das reguläre Maximum verwendet.
**30 ist nicht eingerichtet.** Vollständiger Vergleich, inverse Byteprüfung,
Startschutz und Rücklesen bestanden; Ingame-Anzeige und Wirkung bleiben zu prüfen.
Fortschritt und 999 Plätze erhalten. [Ergebnis](.local/abyss-iii-speed-20260927/RESULTAT.md).

**Separater Inventarauftrag, 26.09.2026, 14:06 Uhr:** Im frisch gesicherten
neuesten `slot0` sind je **500 zusätzliche Kleine Knochen, Platin, reichliches
gegrilltes Fleisch, verbesserte Kraftfaust-Kräuterpastillen und Diamanten** sowie
je **10 Zerstörung III, Verstärkung III und Sturmwind III** eingetragen.
2.530 Stück in 108 neuen Stapeln; 582 Rucksackeinträge bei 999 Plätzen.
Vollständiger Vergleich, inverse Byteprüfung, Startschutz und Rücklesen bestanden.
Vorhandener Fortschritt und Installation erhalten; Spielabnahme offen.
[Mengen, Ergebnis und Sicherung](.local/supplies-500-20260926/RESULTAT.md).

**Separater Inventarauftrag, 26.09.2026, 13:52 Uhr:** Im frisch gesicherten
neuesten `slot0` sind je **300 zusätzliche Eisenerz, Kupfererz, Azurit, Silbererz,
Granat, Golderz und Epidot** eingetragen (2.100 Stück, 42 neue 50er-Stapel).
Bestehende Gegenstände, Fähigkeiten, Geld, 842 Goldbarren und 999 Plätze sind
im vollständigen Save-Vergleich erhalten. Inverse Byteprüfung, Startschutz und
Live-Rücklesen bestanden; Spielabnahme offen. Phasenentwicklung weiterhin pausiert.
[Mengen, Ergebnis und Sicherungen](.local/ores-300-20260926/RESULTAT.md).

**Separater Inventarauftrag, 25.09.2026, 21:40 Uhr:** Im neuesten `slot1`
(Spielstand von 21:28 Uhr) sind je **200 zusätzliche Stück von 27 Materialarten**
für die Verfeinerung von Waffen, Schilden und Rüstung eingetragen, insgesamt 5.400.
Reguläre Stapelgrenzen: 308 neue Einträge, darunter 200 einzelne Aeserions Schuppen.
Vorhandene Gegenstände, Fähigkeiten, Geld, 843 Goldbarren und 999 Plätze sind im
vollständigen Save-Vergleich erhalten. Sicherung, inverse Byteprüfung, Startschutz
und Rücklesen bestanden. Verwendung beim Schmied sowie Speichern/Neuladen bleiben
im Spiel zu prüfen. Die allgemeine Phasenentwicklung bleibt pausiert.
[Materialliste, Ergebnis und Sicherungen](.local/refinement-materials-200-20260925/RESULTAT.md).

Stand: 23.09.2026. Desktop-App **v0.5.9**. Die kostenlose Reparatur wird separat
entwickelt (Entwicklungsmodul **0.23.0**) und ist noch nicht in der App nutzbar. Diese Übersicht beschreibt den
aktuellen Gesamtstand; ältere Einträge in `docs/PROGRESS.md` sind Entwicklungsverlauf.

**Neuer Build erkannt:** Installiert ist inzwischen Steam **25477059 / EXE 1.0.0.2976**.
Für den aktuellen Inventarauftrag wurden die vier betroffenen Tabellen und 38
EXE-/Metadatendateien neu geprüft. Die allgemeine App unterstützt diesen neuesten
Build noch nicht vollständig. Die Tabellenunterstützung für den vorherigen
Build 25455892 / EXE 1.0.0.2949 ist in v0.5.9 ergänzt; Build 25381195 bleibt
ebenfalls unterstützt. 61 verwendete Dateien wurden verglichen, 14 indizierte
Tabellenpaare bytegleich rekonstruiert. Die neuen Stage-/Questeinträge bleiben
erhalten. Die native Reparaturanbindung für die neue EXE ist weiterhin offen;
eine B0-Vanilla-Basis wurde nicht eingerichtet.
[Buildnachweis](docs/BUILD_SUPPORT.md), [Was getestet werden muss](TESTCHECKLISTE.md).

**Deine Vorgabe:** Spieltests erst nach der Entwicklung aller Phasen. Diese
Abnahmen bleiben offen und sind nicht als bestanden gezählt. Während du spielst,
werden weder Spielprozess noch Installation verändert. Die Reparaturentwicklung
verändert keine Saves. Auf deinen separaten Auftrag wurde bei geschlossenem Spiel
Slot 2 um 200 leichte/200 schwere Kupferbeutel und 30.000 Silber ergänzt;
[Ergebnis und Sicherung](.local/inventory-currency/RESULTAT.md).

**Pause auf deinen Wunsch:** Die Phase-5-Arbeit ist bis zur Fortsetzung pausiert.
Der anschließende Inventarauftrag ergänzt im inzwischen neuesten Slot 0
1.000 Goldbarren; Beutel und Geld bleiben erhalten. **999 Rucksackplätze sind
auf deinen erneuten direkten Auftrag eingerichtet:** Die Inventartabelle erlaubt
999 Plätze, der neueste Slot 0 enthält 949 zusätzliche Plätze zu den 50 Startplätzen.
Installation und Save wurden gesichert und unabhängig zurückgelesen; die Anzeige,
Nutzung höherer Plätze und reguläres Speichern/Neuladen im Spiel bleiben ungeprüft.
Die gezielte Änderung richtet keine allgemeine B0-Vanilla-Basis ein.
[Abschluss und Sicherungen](.local/inventory-999-save/RESULTAT.md).

**Aktueller Inventarauftrag, 20:51 Uhr:** Nach dem Steam-Update startet das Spiel
wieder, die alte Modregistrierung war jedoch ersetzt. Im inzwischen neuesten
Slot 1 wurden die verbleibenden **843 Goldbarren zu einem Stapel** zusammengeführt.
Neue Archive setzen die Stapelgrenze auf **1.000** und das Inventarmaximum auf
**999**; der Save enthält wieder die passenden 949 Erweiterungsplätze.
Geld, Gegenstände und neuer Spielfortschritt bleiben erhalten. Installation und
Save sind offline geprüft. **Der Nutzer bestätigt den Spielstart, einen Stapel
mit 843 Goldbarren und 999 Inventarplätze.** Reguläres Speichern/Neuladen nach
dieser Korrektur bleibt als Folgetest offen.
[Aktueller Bericht und Sicherungen](.local/inventory-stack-2976/RESULTAT.md).

**Zusätzlicher Inventarauftrag, 23:42 Uhr:** Im frisch gesicherten neuesten
Stand `slot100` von 23:35 Uhr sind **500 Schlüssel in einem Stapel** ergänzt.
Die vorhandenen 3 Hernand-Münzen für Verfeinerung wurden um 100 auf **103 in
einem Stapel** erhöht. Stapelgrenzen: Schlüssel 500, Hernand-Münzen 200;
Gold-Stapelgrenze 1.000 und 999 Plätze erhalten. Vollständiger Save-Vergleich,
inverse Prüfung, Archivwechsel-Probe und Live-Rücklesen bestanden. Die neue
Spielprüfung bleibt offen. [Bericht und Sicherungen](.local/keys-hernand-2976/RESULTAT.md).
Der frische Originalsave hatte nach weiterem Spielen/Speichern bereits weiterhin
843 Goldbarren in einem Stapel und 949 Erweiterungsplätze.

**Inventarauftrag vom 25.09.2026, 15:24 Uhr:** Im neuesten Slot 0 (gespeichert
um 01:23 Uhr) wurden **70 fertige Abyss-Artefakte** ergänzt; mit den vorhandenen
5 sind es **75**. Die Zugabe liegt in sieben regulären 10er-Stapeln. Spielstände
gesichert, vollständiger semantischer Vergleich, inverse Byteprüfung und
Zurücklesen nach dem Commit bestanden. Spielinstallation, vorhandene Gegenstände
und 999 Plätze erhalten. Die Verwendung im Fähigkeitenmenü bleibt zu prüfen.
[Bericht und Sicherungen](.local/abyss-artifacts-70-20260925/RESULTAT.md).

**Korrektur, 25.09.2026, 15:31 Uhr:** Die 70 Abyss-Gegenstände wurden auf
Nutzerwunsch **vollständig zurückgenommen**, weil sie nicht der gemeinten
Fähigkeitenressource entsprechen. Beide betroffenen Dateien und Änderungszeiten
sind bytegenau auf den Stand unmittelbar vor der Zugabe zurückgeführt; die
unveränderten Nachher-Hashes wurden vor der Rücknahme geprüft. Ursprüngliche
5 Exemplare, Fortschritt und bisherige Mods bleiben erhalten. Die korrekte
Zielressource ist noch zu klären; kein Ersatz wurde vergeben.
[Rücknahme und Sicherung](.local/abyss-artifacts-rollback-20260925/RESULTAT.md).

**Anschließende Zuordnung, 25.09.2026, 15:36 Uhr:** Gewünscht sind
**Abyss-Verknüpfungen für alle drei Charaktere**. Je 70 zusätzliche Einheiten
sind in die drei Fähigkeitenkonten eingetragen (0/16/42 → 70/86/112).
Die entsprechende Gesamtgutschrift wurde je Konto angepasst; gespeicherte
Fähigkeiten und Inventar blieben im Dateivergleich unverändert. Neue Sicherung,
vollständiger Vergleich, inverse Byteprüfung und Zurücklesen bestanden.
**Danach meldete der Nutzer einen zurückgesetzten Fähigkeitenbaum im Spiel.**
Die Spielabnahme ist nicht bestanden; die Ursache ist noch nicht geklärt.
[Bericht zur früheren Zugabe](.local/abyss-links-all-70-20260925/RESULTAT.md).

**Ausgleich auf Nutzerwunsch, 25.09.2026, 15:52 Uhr:** Je **150 weitere
Abyss-Verknüpfungen** für alle drei Charaktere eingetragen. Konten im Save:
70/86/112 → **220/236/262**. Rückerstattungs-Gesamtwert je Konto 262;
seine Verwendung wurde zusätzlich anhand der aktuellen EXE geprüft.
Die Änderung betrifft sechs vorhandene 16-Bit-Felder ohne Datenverschiebung;
alle übrigen entpackten Save-Bytes bleiben identisch. Neue Sicherung,
vollständiger Vergleich, unabhängiger Codec und Live-Rücklesen bestanden.
Die Ursache des vorherigen Resets ist damit **nicht als behoben nachgewiesen**.
Laden, Fähigkeitenzustand, Verwendung und Speichern/Neuladen bleiben zu testen.
[Aktueller Bericht und Sicherungen](.local/abyss-links-all-150-20260925/RESULTAT.md).

| Phase | Thema | Stand |
|---|---|---|
| 0 | Recherche und Machbarkeit | Abgeschlossen; unbekannte Felder dokumentiert |
| 1 | Dateiformate, Core und CLI | Implementiert und automatisiert geprüft |
| 2 | Desktop-App und Itemdatenbank | Implementiert; Teile der Quellenverknüpfungen fehlen |
| 3 | Herstellungsrechner | Berechnung implementiert; Beschaffungsquellen unvollständig |
| 4 | Apply-Engine, Shops, Drops, Vertrauen | Im dokumentierten Umfang implementiert; Grenzen und Spielabnahmen offen |
| 5 | Welt, Drache, Inventar, Haltbarkeit, Skills und Items | In Arbeit; wesentliche Teile vorhanden, Restpunkte unten |
| 6 | Profile und Farm-Preset | Noch nicht begonnen |
| 7 | Lesender Save-Reader und Fortschrittstracker | Noch nicht begonnen |
| 8 | Interaktive Karte | Noch nicht begonnen; zuerst Machbarkeit konkretisieren |
| D, optional | Farm-Hotkey im laufenden Spiel | Nicht begonnen; separate spätere Freigabe erforderlich |

## Phase 0 – Recherche und Machbarkeit

**Implementiert / geliefert:** Build erfasst, Dateiformate und Feature-Zuordnung
untersucht, Quellen und Lizenzen dokumentiert. Berichte:
[FORMATS](docs/FORMATS.md), [FEASIBILITY](docs/FEASIBILITY.md), [CREDITS](CREDITS.md).

**Noch offen:** Die dort als unbekannt erkannten Funktionen werden in den
jeweiligen späteren Phasen weiter untersucht. Recherche ist kein Wirkungsnachweis im Spiel.

## Phase 1 – Core und CLI

**Implementiert:** Installationserkennung für Steam/Epic/Game Pass, Archivlesen,
Entschlüsselung und Dekompression, Build-/Hashprüfung, versionierte Parser,
byteidentische Roundtrips, Lokalisierung in 15 Sprachen, SQLite-Volltextindex
und CLI für Inspektion, Suche, Roundtrip und Buildvergleich.

**Noch offen / Grenzen:** Neue Spielbuilds benötigen eigene Prüfung. Unbekannte
Felder bleiben roh. Ein lokaler Linux-Testnachweis fehlt; Windows wurde geprüft.
Die Erkennung eines Savepfads ist noch kein Save-Reader.

## Phase 2 – Desktop-App und Itemdatenbank (A1)

**Implementiert:** Tauri-/React-App mit dunklem Layout, 6.816 Items, Volltextsuche,
Filter, Sortierung, virtuelle Tabelle, echte extrahierbare Icons, Sprachwechsel,
Detailansicht, Rohfelder, Itemreferenzen und geschützter JSON-Export.
Die Desktop-Verknüpfung öffnet die gebaute App.

**Noch offen:** Vollständige Verknüpfung Item → Händler → Stadt/Region/Preis/Bestand
sowie Dropset → konkreter Gegner/Fundort. Rezeptlinks kamen in Phase 3 hinzu.
Unbekannte Kategorien/Stats werden nicht mit erfundenen Bedeutungen beschriftet.

## Phase 3 – Herstellungsrechner (A2)

**Implementiert:** 1.108 berechenbare Rezepte, Zielmenge, rekursiver Materialbaum,
aggregierter Restbedarf, vorhandene Vorräte, Chargenüberschüsse, Rezept- und
Materialalternativen, manuelle Zwischenprodukte, Zykluswarnungen, Item-Rezeptlinks
sowie Planexport und CLI.

**Noch offen:** Händler, Regionen, Preise und konkrete Dropquellen für Materialien.
Sonder-/Verstärkungsrezepte sind nicht pauschal als berechenbar freigegeben.
Vorräte aus einem Save zu übernehmen hängt von Phase 7 ab.

## Phase 4 – Apply-Engine, Shops, Drops und Vertrauen (B0–B3)

**Implementiert:** Eigener Overlay-Builder ohne externen Modmanager, Vorschau,
Apply/Reapply/Restore, Backups und Hashprüfung, Journal und Recovery,
Buildwechsel-/Konfliktprüfung, Spielstartschutz, Vanilla-Herkunftsprüfung,
Inventur sowie bestätigter Erhalt fremder Zusatzdateien. Wiederanwendung baut
aus dem Ausgangsbestand und den aktuellen Einstellungen neu auf.

- **Shops:** Bestände, ausgewählte zusätzliche Artikel/Artikelsets, individuelle
  Sortimente und Tagesrefresh für unterstützte Händler.
- **Drops:** Mengen- und Chancenfaktoren, Einzelausnahmen sowie manuell gewählte
  100-%-Basisraten für unterstützte Dropsets.
- **Vertrauen:** Multiplikation belegter positiver Zuwächse; negative Strafen bleiben erhalten.

**Noch offen / Grenzen:** Keine universelle Abdeckung aller Sonderhändler und
Chancenvarianten; keine automatische Bossklassifikation. Der komplette Katalog
bei allen Händlern gleichzeitig ist nicht freigegeben. Fremde Änderungen an
Originalquellen sind keine bestätigten kompatiblen Mods. Die aktuelle Installation
ist noch nicht als B0-Vanilla-Basis zugelassen; der gespeicherte Prüfbericht allein
ist keine Zulassung. Echte Spielwirkung und manuelle Abnahme stehen noch aus.

Die Entwicklung wurde für diesen dokumentierten Umfang abgeschlossen; das bedeutet
nicht, dass jeder ursprüngliche Wunsch universell abgedeckt ist. [Details](docs/MODS.md).

## Phase 5 – Erweiterte Modmodule (B4–B11)

| Modul | Implementiert | Noch offen |
|---|---|---|
| B4 Welt | Spawnzahlen/-gruppen, Auswahl und Ausnahmen; zwei Stage-Patrouillen und 108 Wiederbesetzungsregeln; belegte Stadt-, Reitdauer- und Cooldownwerte | Allgemeiner NPC-Respawn-Timer; Spielwirkung und Questabhängigkeiten |
| B5 Drache | Blackstar-Cooldown, Dauer/Preset und belegte Regions-/Stadtflugregeln | Spielabnahme von Regionswechseln; Sonderzonen und Questabstiege bleiben eigene Regeln |
| B6 Inventar/Lager | Freie Start-/Maximalwerte für neun Bereiche, Verhältnisprüfung | Universelle Engine-/Save-Grenzen nicht bewiesen; Spezialcontainer bleiben geschützt |
| B7 Stapel | Globale Größe, Kategorien, Ausnahmen, experimentelle Instanzitems | Universelle sichere Maxima und Spieltests für Instanzitems |
| B8 Haltbarkeit | Belegter No-Wear-Sentinel für 122 Items und Verschleißfaktoren | Spielabnahme, insbesondere Spezialausrüstung/Sockel; keine Wiederherstellung zerstörter Items |
| B8 Kostenlose Reparatur | Gemeinsamer Ablauf mit Batchprüfung, nativen Kopien, Feldschreiber und Meldungsadaptern. Ausrüstungsweg und Inventar-Client-Ack isoliert angebunden. Native Item-/Speicherobjekt-Umwandlung erhält reparierte Haupt-/Sockelwerte und geprüfte Färbedaten | Serverseitige Inventarmeldungen, regulärer Speichervorgang und dessen Bestätigung, übrige Zusatzdaten, echte Effekte/UI/Rückmeldungen, Manager-/Spieler-/Threadbindung, Eingabe, Loader und B0-Einbau fehlen. In der App noch gesperrt |
| B9 Ausdauer/Spirit | Sieben Kostenkategorien einschließlich Nullkosten; zusätzliche Eigenverbrauchswerte aus Skills/Buffs | Kosten außerhalb der belegten Listen; kein universell nachgewiesenes Unlimited |
| B10 Skills | Alle 2.069 Skills, Feld-/Rohdatenansicht, globale/individuelle Cooldowns und belegte numerische Werte | Bedeutung/Einheiten unbekannter Felder und Spielabnahme |
| B11 God-Items | Stats, kompatible Buffs, zusätzliche Enchant-Zeilen und wiederverwendbare Itemvorlagen | Spielabnahme; Vorlagen ändern keine bestehenden Save-Instanzen |

**Phase 5 ist noch nicht fertig.** Die Reparaturentwicklung wird nur mit privaten
Testdaten und Funktionskopien im eigenen Testprozess geprüft. Sie ist kein bereits
installierter Mod. Details: [Restpunkte](docs/PHASE5_REMAINING.md),
[Reparaturentwicklung](runtime/repair/README.md),
[Objekt-Referenzen und Testgrenzen](runtime/repair/REFERENCE_INTEGRATION.md).
[Gemeinsamer nativer Erwerbsweg auf dem neuen Build](runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).
[Aktuelle Erfassung und ihre Testgrenzen (0.9.0)](runtime/repair/PINNED_CAPTURE_INTEGRATION.md).
[Inventar-/Ausrüstungsnachweis für den neuen Build (0.10.0)](runtime/repair/INVENTORY_2949.md).
[Ereignisvorbereitung und isolierte Server-/Client-Probe (0.11.0)](runtime/repair/EQUIPMENT_EVENTS_2949.md).
[Native Itemkopie und Lebensdauer (0.12.0)](runtime/repair/ITEM_LIFECYCLE_2949.md).
[Slot-Markierung und gehaltene Erfassung (0.13.0)](runtime/repair/DIRTY_SLOTS_2949.md).
[Feldschreiber für Inventar und Ausrüstung (0.14.0)](runtime/repair/FIELD_WRITER_2949.md).
[Vorbereitete Kopien und gemeinsamer Ablauf (0.15.0)](runtime/repair/TRANSACTION_2949.md).
[Slot-Verarbeitung und Batchabschluss (0.16.0)](runtime/repair/PERSISTENCE_2949.md).
[Inventar-Client-Rückmeldung (0.17.0)](runtime/repair/INVENTORY_EVENTS_2949.md).
[SQL-Auftragsweg und fehlende Speicherbestätigung (0.18.0)](runtime/repair/SQL_DISPATCH_2949.md).

**Aktuell geprüft:** 454 native Szenarien und neun CTest-Suiten. 38 Fälle
führen den [Item-/Speicherobjekt-Weg](runtime/repair/ITEM_SAVE_2949.md) aus,
darunter 14 mit dem tatsächlichen Reparaturplan und nativen Nachherkopien.
Beschädigte/volle Hauptwerte, No-Wear, freie/teilbelegte Sockel, Färbedaten und
Wiederverwendung der Objekte sind geprüft. Der native Lader normalisiert einige
Werte; diese Sonderregeln sind dokumentiert. Das beweist noch keinen Abschluss
des regulären Speichervorgangs. Die SQL-Erfolgsrückgabe allein bestätigt ebenfalls
keine Speicherung; der Reparaturschalter bleibt gesperrt.

39 weitere Fälle prüfen den [Speicherdispatcher und seine Warteschlangen](runtime/repair/SAVE_BACKEND_2949.md),
einschließlich Fehlerabbrüchen, wiederholten Aufrufen und Steam-Zulassungsprüfung
mit privaten Plattformdaten. Weitere 34 Fälle führen den
[Dateischreibhelfer](runtime/repair/SAVE_FILE_2949.md) mit privaten Dateiempfängern
aus. Flush-/Closefehler können trotzdem Erfolg und geleerten Payload ergeben.
Weitere 66 Fälle prüfen die [native Pufferaufbereitung und Kompression](runtime/repair/SAVE_ENCODING_2949.md).
Nach einem Öffnungsfehler erneut verwendete Einträge können doppelt komprimiert
werden; neue Versuche brauchen frische Rohdaten. Nachgelagerte Verarbeitung,
regulärer Schreibabschluss und Spielhostbindung bleiben offen.

36 neue Fälle prüfen den [originalen Inventar-Paketserializer](runtime/repair/INVENTORY_PACKET_2949.md)
und die Streamauswahl; zehn verbinden sie mit der Reparaturtransaktion und dem
Client-Ack. Im Paket fehlen Item-UID, Sockel- und Auftragsdaten. Fehlende/doppelte
Zustellung oder ein falscher Actor werden im privaten Ablauf abgewiesen. Reale
Transportbindung, erfolgreicher Poolzweig und Server-/Speicherabschluss fehlen.

## Phase 6 – Profile und Farm-Preset (C)

**Noch zu implementieren:** Benannte vollständige Modulprofile, JSON-Import/-Export,
Farm-Preset mit Drops ×10 und Spawns ×3, anpassbare Werte und Profilwechsel über B0.
Vorhandene Itemvorlagen sind noch kein vollständiges Profilsystem.
Profilwechsel sollen bei laufendem Spiel gesperrt bleiben und beim nächsten Start wirken.

## Phase 7 – Save-Reader und 100-%-Tracker (A4)

**Noch zu implementieren:** Save-Auswahl und lesendes Parsing, Quest-/Stage-
Fortschritt, Gruppierung nach Region/Kategorie, Fortschrittsanzeigen, Aktualisierung
bei Änderungen sowie Sammelobjekte und Vorratsimport, soweit das Format sie hergibt.
Keine Save-Schreibfunktion vorgesehen.

## Phase 8 – Interaktive Karte (A3)

**Noch zu implementieren:** Zuerst konkreter Machbarkeitsbericht, danach verfügbare
Kartentexturen/Kacheln, bestätigte Positionsquellen, Koordinatenkalibrierung,
Ebenen, Suche und Markerdetails. Gesammelte Objekte nur bei belegter Unterstützung
durch den Save-Reader. Eine vollständige Karte ist noch nicht zugesagt.

## Optional D – Farm-Hotkey im Spiel

**Nicht begonnen.** Separates späteres ASI-/Runtime-Modul für den Farm-Hotkey,
erst nach den übrigen Funktionen und eigener Freigabe. Die bereits freigegebene
Reparaturentwicklung ist nicht automatisch eine Freigabe dieses Farm-Hotkeys.

## Nächste Schritte und Nachweise

1. Technische Restpunkte aus Phase 5 weiter bearbeiten, aktuell native Funktionen des neuen Builds und die Reparaturanbindung.
2. Danach Phase 6, anschließend 7 und 8 gemäß Phasenplan.
3. Manuelle Spieltests nach Abschluss der Entwicklung aller Phasen, wie vereinbart.

Aktuelle Prüfung und Änderungen: [PROGRESS](docs/PROGRESS.md).
Automatisierte Nachweise und spätere Spielabnahmen: [TESTING](docs/TESTING.md).
Konkrete Fälle mit erwartetem Ergebnis: [TESTCHECKLISTE](TESTCHECKLISTE.md).

## Zusatzauftrag – Schwarzstern (01.10.2026)

**Installiert, Spieltest offen:** Eigene ASI-Erweiterung für praktisch unbegrenzte
Ausdauer, Beschwörungs-Cooldown von einer Sekunde und die gezielte Aufhebung der
regionalen Beschwörungsablehnung für Schwarzstern. Innenraum- und
Hindernisprüfungen bleiben aktiv. Der alte gespeicherte Timer im neuesten Slot 1
wurde ebenfalls auf eine Sekunde gekürzt, mit exakt geprüftem Savevergleich.
Anschließend Beschwörungsdauer auf **1.000 Minuten (16 Stunden 40 Minuten)**
erhöht, einschließlich des gespeicherten Resttimers in Slot 1. Frisches Backup
der vorherigen Erweiterung und aller Savedateien erstellt; Savevergleich zeigt
ausschließlich die neue Dauer. Native Reader-/Hooktests und Loaderprüfung
bestanden.

Nach dem Spielbericht über gesperrte Städte und rote Kartenflächen ebenfalls
installiert: gezielte Freigaben für Schwarzsterns Stadt-/Gebietsflug und Höhe,
sowie weitere Navigations-/Voxel-/Plattformprüfungen während seiner Beschwörung.
Originale Innenraum- und abschließende Platzierungsprüfungen bleiben aktiv.
Aktuelle Savedateien unverändert; vorherige ASI und alle Saves frisch gesichert.
Native Bedingungs-/Instanz-Lookups, Reittierreferenzen, zehn Codehooks,
Bedingungsslot und Loader getestet. Abnahme der roten Gebiete im Spiel offen.

Details, Sicherung, Rückbau und manuelle Abnahme: [Schwarzstern](docs/BLACKSTAR.md).
Dieser Zusatzauftrag ändert den Entwicklungsstand der übrigen Phasen nicht.

**Zusatzauftrag Item-Auswahl / Live-Spawner, 03.10.2026:** Auswahl mit Icons,
Beschreibungen, Tabellenwerten je Verfeinerungsstufe, Haupt-/Unterkategorien,
Text-/ID-Suche und virtueller Liste umgesetzt. Build 2976 für lesenden
Basis-Katalog geprüft; Desktop-Verknüpfung aktualisiert. Lesender Diagnose-Mod
installiert, 32 bestehende Spiel-/Save-Dateien unverändert geprüft. **Live-Item-Geben
ist jetzt implementiert und außerhalb des Spiels getestet:** native Item-Erzeugung,
Inventartransaktion, lokale Verbindung, Warteschlange, UUID-Schutz, Abbruch und
Ergebnisabfrage. Neue ASI und Desktop-Version sind bei geschlossenem Spiel
installiert; die Desktop-Verknüpfung wurde aktualisiert. 33 bestehende Dateien
unverändert geprüft, 19 Dateien frisch gesichert. Ein echter Inventar-/Speichern-/
Neuladen-Test steht noch aus. Die lesende Probe bleibt zusätzlich vorhanden.
Details: [Item-Auswahl und Live-Spawner](docs/LIVE-ITEMS.md).

**Automatische Item-Ausgabe, 03.10.2026:** Das zusätzliche Aufheben ist durch einen
Charakterauftrags-Hook ersetzt. Er verarbeitet die Anfrage mit der vom Spiel
gehaltenen nativen Referenz und prüft erneut die aktive Figur. Status bleibt bei
bestätigter Figur auch während Alt-Tab nutzbar; Ausführung erfolgt im Spielablauf.
UUID-Schutz, Abbruch und Ergebnisabfrage bleiben erhalten. Tatsächlicher nativer
Dispatcher/Wrapper und MinHook im eigenen Testprozess, Protokoll 2, Queue-/
Kontextwechsel-/Pausenfälle, Produktionsbuild und Loader bestanden. ASI und
Workbench bei geschlossenem Spiel/Workbench ersetzt und zurückgelesen; 36
bestehende Dateien unverändert, 23 Backups. **Ein Eisenerz ohne Aufheben geben,
speichern und neu laden ist noch im echten Spiel zu prüfen.**

**Weiterhin gemeldete Beschwörungssperre, 01.10.2026:** Der Nutzer meldet nach dem
Stadtflug-Update weiter ungefähr „Reittiere können hier nicht gerufen werden“.
Das alte Update wurde laut Laufzeitlog geladen; die bisher protokollierten
Spawnprüfungen zeigen die konkrete Ablehnung nicht. Ein weiteres Update erkennt
Schwarzstern während der früheren Client-Beschwörungsaktion und protokolliert
gemeinsame/serverseitige Ablehnungen sowie Fehlercode und Aufrufweg der
Spielmeldung. Drei native Testsuiten und Loaderprüfung bestanden; nur die ASI
bei geschlossenem Spiel ersetzt. Alle neun Saves und 19 geschützten Dateien
unverändert geprüft, frisches Backup erstellt. **Die Restursache und der Erfolg
an der gemeldeten Stelle sind noch offen; weiterer Spielversuch erforderlich.**

## Zusatzauftrag Item-Auswahl – Tasche, Pfeile, Reittiere und Wissen (03.10.2026)

**Implementiert, außerhalb des Spiels geprüft und am 04.10.2026 bei geschlossenem
Spiel/Workbench installiert:** Fehlklassifikation eines optionalen Definitionsfelds korrigiert,
insbesondere Extragroße Tasche 6003 und Blitzpfeil 1001315. Native Standardpfade
geprüft; Sockel-/Mengenprüfungen bleiben aktiv, Fehlermeldungen unterscheiden die
Ursachen. Das Mengenfeld erlaubt Leeren, Tippen und Einfügen mit Stapelschätzung.

Neue Reittierauswahl mit Suche, Familien, Beschreibung, Spielstandauswahl und
Besitzstatus. Bei geschlossenem Spiel kann genau ein passendes Tier im Stall
registriert werden, sofern eine bestätigte unzugewiesene Vorlage derselben
Familie im Save vorhanden ist. Frische Nummern, Sicherung, Rücklesung, Erhalt
vorhandener Savebereiche und Schutz vor Doppelvergabe implementiert. Live-
Registrierung, Sonderbeschwörungen und individuelle Tiericons bleiben offen;
Stall/Reiten/Speichern/Neuladen im Spiel noch zu prüfen.

Dokumente und Items mit bestätigter Wissensbelohnung zeigen ihren Status aus dem
ausgewählten **gespeicherten** Spielstand; fehlende Zuordnung heißt unbekannt.
Nach Lesen/Speichern neu einlesen. Abgleich mit dem Spiel offen. Dies ist ein
begrenzter Teil des Save-Lesers, kein fertiger Quest-/100-%-Tracker der Phase 7.

Details: [Live-Items](docs/LIVE-ITEMS.md), [Reittiere](docs/REITTIERE.md).

## Zusatzauftrag zusätzliche Reittierfamilien (04.10.2026)

**Implementiert, an privaten Saves geprüft, gebaut und installiert;
Spielinitialisierung ausdrücklich unbestätigt.** Neue Basiseinträge ermöglichen
zusätzliche Familien ohne gespeicherte Vorlage derselben Tierart: Raptor,
Elefant, Leguane, Wölfe, erwachsene Kamele und weitere normale Reittiere.
Die private Kopie von Slot 0 hat 288 statt 219 hinzufügbare Einträge. 24 getrennte
Kandidaten, einschließlich aller 13 erwachsenen Kamelvarianten, bestehen
vollständigen Rückvergleich, Verschlüsselung, frische Nummern und Doppelsperre.
Die Route übernimmt keine Tierwerte oder Ausrüstung, sondern erstellt leere
Strukturen mit aktuellen Feldnamen/-typen und einem Initialisierungsflag false.
Das gewünschte Initialisieren durch das Spiel muss erst beobachtet werden.
Nutzvieh, Kamelkälber, Sonderbeschwörungen und unbekannte Schemas bleiben außerhalb
dieser Route. Vorhandene gesunde Familienvorlagen behalten den bisherigen Weg.
Acht Reittier-UI-Tests, elf Frontend-Tests, fünf App-Tests und 159 Core-Unit-Tests
bestehen. Der strenge Vanilla-Roundtrip der aktiven Modinstallation verweigert
weiterhin deren geänderte `meta/0.papgt`; sie wird für diesen Auftrag nicht
zurückgesetzt. Auch die strenge Archiv-Modvorschau verweigert die außerhalb ihres
Journals veränderte Registry. Der echte lesende Browser-/Katalogtest und die Saveprüfungen bestehen.
Das Desktop-Update wurde bei geschlossenem Spiel/Workbench installiert: 37
Dateien einschließlich aller 13 Saves unverändert; 23 Sicherungen und Desktop-
Verknüpfung unabhängig geprüft.
Keine Tiervergabe und keine echten Saveänderungen durch die Entwicklung.
Nachweise: `.local/mount-family-investigation-20261004/`; Spieltests MT09–MT12 in
[TESTCHECKLISTE](TESTCHECKLISTE.md).

## Zusatzauftrag Reittiervorlagen, Bildvorschau und Suche (04.10.2026)

**Folgeerweiterung installiert und unabhängig zurückgelesen; Spielabnahme offen:**
Gesunde, zugewiesene Vorlagentiere werden unterstützt. Nur neue Kopien verlieren
Name und die drei bestätigten Zuweisungsfelder; vorhandene Tiere bleiben durch
vollständigen Rückvergleich geschützt. Die frisch gelesene private Kopie von
Slot 1 zeigt dadurch 219 statt 217 hinzufügbare Tiere, einschließlich zweier
Löwenvarianten. Fehlende Familien, Aufgaben-/Fütterungsfelder, fehlende HP und
unbekannte Zuweisungsformate bleiben gesperrt.
89 exakte Tierporträts und 210 beschriftete Beispielbilder decken 299 von
322 Einträgen ab; 23 behalten ein allgemeines Symbol. Gemeinsame Texturen werden
einmal dekodiert, die Beispielbeschriftung bleibt an der einzelnen Variante.
Private Nachweise: `.local/mount-catalog-expansion-20261004/`.
Das Desktop-Update wurde bei geschlossenem Spiel und geschlossener Workbench
installiert; 37 vorhandene Dateien einschließlich aller 13 Saves und der Mods
sind unverändert. 23 Sicherungen und die bestehende Desktop-Verknüpfung wurden
unabhängig geprüft. Bei der Installation wurden keine Tiere vergeben.
Die vorherige Version ist im folgenden Abschnitt dokumentiert.

**Implementiert, technisch geprüft und am 04.10.2026 bei geschlossenem Spiel installiert.** Eine eigene
Tierbenennung blockiert eine sonst geeignete unzugewiesene Stallvorlage nicht
mehr. In einer privaten Kopie des aktuellen Slot 1 sind 219 Tierarten hinzufügbar;
die Anzeige zeigt Anzahl, Familien und die Ursache bei fehlender Vorlage.
Ein Bär, ein Pferd und ein Löwe wurden ausschließlich in privaten Savekandidaten
erzeugt und durch vollständigen Rückvergleich geprüft.

Itembilder und Reittierbilder sind größer und per Klick vergrößerbar. 57 echte
Tierporträts konnten eindeutig zugeordnet und dekodiert werden; fehlende Bilder
bleiben als allgemeines Tiersymbol erkennbar. Die normale Itemsuche findet jetzt
Teilwörter in Namen, Beschreibungen und internen Namen, unabhängig von Groß-/
Kleinschreibung und Bindestrichen. Mehrere Wörter werden kombiniert. Reine
numerische Item-IDs bleiben exakt. Optionaler Regex-Modus für Itemdatenbank,
Item-Auswahl und Reittiere mit Fehlermeldung bei ungültigen Mustern.

158 Core-Tests, fünf App-Tests, elf Frontend-Tests und 16 gezielte UI-Tests sind bestanden. Der echte
Katalogtest umfasst 6.816 Items, Teilwort-/Regex-Suche und die 57 Tierporträts.
Die bestehende Desktop-Verknüpfung öffnet die neue EXE. 37 vorhandene Dateien, darunter alle 13 Save-Dateien und die nativen Mods, sind unverändert; 23 Sicherungen wurden geprüft. Die Spielabnahme bleibt offen.

**Pfeilfehler:** Trotz vorhandenen Blitzpfeilen meldete der normale Schuss mit
dem Aeserion-Bogen „nicht genug Pfeile“. Rein lesend wurden 200 Stück der ID
1001315 synchron im Client-/Serverinventar bestätigt. Die Itemnotiz bezeichnet
Blitzpfeil 1001315, Feuerpfeil 1001316 und Kältepfeil 1001314 als Monstermunition.
Die Auswahl kennzeichnet diese Varianten und sperrt ihre erneute Ausgabe auch
im Backend. Pfeil 50001, Giftpfeil 50003 und Explosionspfeil 1001321 bleiben
verfügbar. Der Explosionspfeil ist ausdrücklich für Spieler vorgesehen.
Vorhandene NPC-Pfeile wurden weder entfernt noch umgewandelt; die Verwendung
einer passenden Pfeilvariante mit dem Aeserion-Bogen muss noch im Spiel geprüft werden.

Beide installierten Dateien unabhängig zurückgelesen; 36 bestehende Dateien
einschließlich 13 aktueller Saves und Desktop-Verknüpfung unverändert geprüft,
23 Sicherungen erstellt und geprüft. Bei der Installation wurden keine Items
oder Tiere vergeben und keine Saves bearbeitet. Spielabnahmen bleiben offen.
