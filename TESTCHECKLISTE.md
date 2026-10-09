# Crimson Workbench – Was muss getestet werden?

## Sockelstein-Boni in Workbench, 05.10.2026

- [x] 190 eindeutige Steine aus dem lokalen, geprüften ItemInfo gelesen;
  deutsche Namen/Beschreibungen und vollständige Stat-/Buff-Listen vorhanden.
- [x] Zerstörung III: Angriff +3 statt Rohwert 3000, leere Nullwerte ausgeblendet.
- [x] Sturmwind III: Angriffsgeschwindigkeit +3 Stufen; keine geratenen Prozentwerte.
- [x] Fähigkeitsbeschreibung, Effektstufe und explizite Anzeige unbekannter Felder.
- [x] Auswahl während des Ladens, Lesefehler/Wiederholen, Suche sowie vorhandene
  Sperren gegen laufendes Spiel und veraltete Konfiguration geprüft.
- [x] 17 Frontend-Tests und 10 Zusatzsockel-UI-Tests; Vorschau visuell geprüft.
- [ ] Workbench über die Desktop-Verknüpfung öffnen → Zusatzsockel:
  Zerstörung III, Sturmwind III und Flamme des Urteils auswählen und die
  Vorschau rechts prüfen. Dafür darf das Spiel weiterlaufen.
- [ ] Einen bereits bestückten Zusatzsockel wählen: Grundboni/Effekte prüfen.
  Das Einsetzen/Leeren selbst erfordert weiterhin ein geschlossenes Spiel.


**Update 05.10.2026:** Echter Spielstart/Laden und die korrigierten Daten auf
Client/Server rein lesend bestätigt: 10 / 11 / 15 Plätze, Menge jeweils 1,
identische Basissockel, Helm/Schwert Verfeinerung 10. Nachweis:
`.local/extra-sockets-plus10-20261004/runtime-readonly-31728.json`.
Visuelle Anzeige/Benutzung, Steinwirkung und Speicherzyklus noch bestätigen.

## Zehn zusätzliche Sockel: Panzer, Helm und Mecha-Schwert, 04.10.2026

**Korrektur und freie Ausrüstungswahl:** Der erste Spielstart/Ladeversuch
funktionierte, zeigte aber beschädigte Client-Werte. Deshalb gilt XS02 noch
nicht als bestanden. Korrigierter Runtime→InitData-Übergang ist installiert;
keine Spielstände wurden geändert oder zurückgesetzt.

- [x] 36 zusätzliche Fälle mit dem vollständigen originalen Netzwerk-Konverter:
  Basissockel 0–5, leer/bestückt, Prefix normal/0/255; Menge, Flags, Verfeinerung,
  Haltbarkeit und Speichergrenzen erhalten; Client-Erweiterung wiederhergestellt.
- [x] Allgemeine Konfiguration v2 und Migration v1: neue UID, doppelte UID,
  unzulässiger Key, maximale Anzahl und atomare Speicherung geprüft.
- [x] 329 eigene Ausrüstungsinstanzen im neuesten Save gelesen; 326 geeignet;
  keine Save-Schreibzugriffe. Sieben Browsertests einschließlich +10-Auswahl,
  Doppelclick, gesperrtem Spiel, unbekanntem Format und veraltetem Save.
- [ ] **XS05:** In Workbench eine weitere geeignete Instanz auswählen, +10
  hinzufügen; zehn leere Zusatzplätze, vorhandene Steine und Verfeinerung
  unverändert. Im Spiel korrekt ausrüstbar, anschließend speichern/neu laden.
- [ ] **XS06:** Installation auf zweitem Rechner / portables Paket. Die EXE
  allein richtet Laufzeitmods und persönliche Instanz-IDs nicht ein.

Die ältere Liste unten dokumentiert die erste Installation. Aktuell insgesamt
86 native Fälle, 22 Policy-/Dateifälle, 163 Core-Unit-Tests (vier ignoriert),
elf Frontend-Unit-Tests und sieben Browsertests bestanden. Der globale
`tests/live.rs`-Roundtriptest scheitert am Hash der bereits modifizierten
`meta/0.papgt`; Browser/Katalog- und Sockel-Lesetests funktionieren. Die Registry
wurde bei dieser Änderung nicht angefasst.

- [x] Genau drei bestehende Instanzen identifiziert; zehn neue Plätze jeweils
  zusätzlich zu 0 / 1 / 5 Basissockeln: insgesamt 10 / 11 / 15.
- [x] 50 isolierte native Originalroutine-/Integrationsfälle: alle Sockelwerte
  verarbeitet, dynamisch tief kopiert, fünf Basiseinträge geschrieben,
  zusätzliche Live-Einträge beim Speichern unverändert.
- [x] 18 Policy-/Dateifälle: Instanz-/Key-/Padding-Prüfung, Prüfsumme,
  atomare Zusatzdatei, veraltete Schreibversuche ohne Änderung abgewiesen.
- [x] Core-Suite 162 bestanden / 4 absichtlich ignoriert; Frontend elf Unit-
  und vier neue Browsertests: zehnter Zusatzsockel, Doppelclick, Spiel läuft,
  veraltete Revision, überholte Suchantwort. Produktionsbuild bestanden.
- [x] Winmm-Weiterleitung und gemeinsames Laden aller sieben ASIs in privatem
  Prozess; Zusatzsockel-Mod deaktiviert sich bei falscher EXE vor den Hooks.
- [x] Bei geschlossenem Spiel/Workbench mit gehaltenem Startschutz installiert;
  Save-Backups und Desktop-EXE-Sicherung erstellt; 13 Saves und insgesamt
  22 geprüfte Bestandsdateien unverändert; Desktop-Verknüpfung verwendet Update.
- [ ] **XS01:** Workbench über Desktop öffnen → Zusatzsockel: drei passende
  Gegenstände und insgesamt 10 / 11 / 15 Plätze. Jeweils zehn neue Plätze leer.
- [ ] **XS02:** Spiel über Steam starten, Slot 1 laden. Kein Absturz, alle
  bisherigen Gegenstände, Steine, +10-Verfeinerung und Inventar unverändert.
  `CrimsonExtraSockets.log` muss READY und EXTENDED für 10 / 11 / 15 melden.
- [ ] **XS03:** Bei geschlossenem Spiel z. B. Zusatzsockel 10 des Mecha-Schwerts
  in Workbench mit Zerstörung III bestücken, neu starten/laden; Wirkung mit
  vorherigem Stand vergleichen. Spieloberfläche zeigt höchstens fünf Zeilen.
- [ ] **XS04:** Im Spiel speichern, vollständig schließen und erneut laden.
  Zusatzstein bleibt in Workbench belegt; Wirkung bleibt erhalten. Normale
  Saves haben weiter fünf Paddingeinträge; vorhandene Sockel unverändert.

Die getrennte Zusatzdatei zusammen mit den Saves sichern. Workbench-Leeren
entfernt den dortigen Stein direkt; vorherige Belegung wird im Projekt gesichert.
[Nachweise und Installationsbackup](.local/extra-sockets-plus10-20261004/RESULTAT.md).

## Elektro-Mecha-Langschwert: fünfter Sockel, 04.10.2026

- [x] Im neuesten Spielstand genau eine Instanz identifiziert: 1003664,
  vier belegte Sockel und leerer fünfter Platz, Verfeinerung +10.
- [x] Genau ein entschlüsseltes Datenbyte 4 → 5 geändert; sämtliche anderen
  Felder und vier Steine erhalten. Unabhängige HMAC-/ChaCha-/LZ4-Rücklesung
  und bytegenaue Rücknahme bestanden; sechs native Funktionsbereiche erneut gepinnt.
- [x] Nach Schließen beider Apps den frischesten Stand erneut geprüft, Original
  gesichert, nur `slot0/save.save` ersetzt und unabhängig zurückgelesen.
  Zwölf andere Save-/Lobby-Dateien unverändert; kein neues ASI oder EXE-Update.
- [ ] **MS01:** Slot 1 laden; Mecha-Schwert zeigt fünf offene Sockel und +10,
  die vier bisherigen Steine bleiben erhalten.
- [ ] **MS02:** Fünften Sockel bei der Hexe mit einem vorhandenen Stein belegen;
  speichern und neu laden, anschließend Belegung und Wirkung prüfen.

Die spätere Zusatzsockel-Erweiterung ist oben beschrieben; deren Spieltests
sind noch offen.

## Mecha-Schwert: mehr als fünf Sockel, private Machbarkeit, 04.10.2026

- [x] Unveränderte vollständige native Sockel-Werteroutine: zehn Fälle,
  einschließlich sechs/zehn Einträgen, Lücken, Haltbarkeit und begrenzter Freigabe.
- [x] Unveränderte vollständige native Sockelpuffer-Kopie: zehn Fälle;
  sechs/zehn Datensätze vollständig erhalten, unabhängige Zielpuffer.
- [x] Native Sockel-Speicherschleife: zwölf Fälle. Original schreibt fünf;
  Prototyp mit dynamischem Vergleich schreibt fünf/sechs/zehn korrekt.
- [x] Private vollständige Zehn-Eintrag-Spielstandprobe unabhängig zurückgelesen;
  alle anderen Daten erhalten, inverse Rücknahme bytegenau.
- [ ] Vollständigen nativen Ladevorgang mit Spieleallocator anbinden und prüfen.
- [ ] Gesamten nativen Speicherzyklus, Freigaben und Kopien unter realem
  Objektlebenszyklus prüfen; Fixture-Callbacks ersetzen bisher externe Helfer.
- [ ] Zusatzplätze darstellen und verwalten; Hexenbedienung/Standarditems prüfen.
- [ ] Erst nach vollständigem Prototyp gezielte Installation und Spielabnahme:
  tatsächliche Zusatzboni, Aus-/Anlegen, Hexe, Speichern/Neuladen.

## Zusätzliche Reittierfamilien ohne eigene Vorlage, 04.10.2026

Basiseintrag-Route technisch geprüft, gebaut und bei geschlossenem Spiel/Workbench installiert.
Die Spielinitialisierung und Reitfunktion sind weiterhin unbestätigt.

- [x] 24 private Save-/Lobby-Kandidaten: Raptor, Elefant, beide Leguane, Wölfe, alle 13 erwachsenen Kamelvarianten und weitere Familien. Entfernen jedes neuen Tiers ergibt exakt den ursprünglichen Savekörper.
- [x] Keine Übernahme fremder HP, Ausrüstung, Level, Namen, aktiver Zuweisung oder Spawnposition. Neue Nummer, HMAC und Doppelsperre geprüft; unbekannte Schemas werden abgelehnt.
- [x] Acht Reittier-UI-Tests: Basiseintrag ist auswählbar, Spielteststatus und Sicherung werden angezeigt. Suche, Bilder, Spielprozesssperre und Wiederaufnahme derselben Anfrage bleiben geprüft.
- [x] Desktop-Update installiert und unabhängig zurückgelesen: 37 Dateien einschließlich aller 13 Saves unverändert, 23 Sicherungen geprüft; keine Tiervergabe.
- [ ] **MT09:** Unter Reittiere neuesten Save auswählen und Neu einlesen. Im unveränderten geprüften Slot 0 stehen 288 Einträge zur Verfügung; Raptor/Elefant/Leguane/Kamele/Wölfe zeigen „Basiseintrag · Spieltest offen“.
- [ ] **MT10:** Zunächst genau eine gewünschte exotische Variante bei geschlossenem Spiel registrieren. Diesen Save laden; Stall/Sonderreittiermenü, Sichtbarkeit, HP und Herbeirufen prüfen.
- [ ] **MT11:** Aufsteigen, laufen, sprinten und absteigen. Anschließend speichern und neu laden; Tier muss erhalten und erneut verwendbar sein. Bestehende Pferde/Bären/Löwen und aktives Reittier prüfen.
- [ ] **MT12:** Erst nach erfolgreicher Einzelprüfung weitere Tierarten ergänzen. Fehlermeldung und Variante melden, falls Initialisierung oder Nutzung scheitert; keine pauschalen Quest-/Wissensfreischaltungen vornehmen.

Die ältere Checkliste darunter dokumentiert den vorherigen Stand mit 219
hinzufügbaren Einträgen und ausschließlich Vorlagen derselben Tiergruppe.

## Erweiterte Reittiervorlagen und Porträts, 04.10.2026

Die Erweiterung ist installiert und unabhängig zurückgelesen; Spielabnahme steht aus.

- [x] Private Savekopie: zugewiesener, benannter Zirkuslöwe dient als Vorlage; neue Kopie hat weder Name noch aktive Zuweisung. Originale bleiben nach vollständigem Rückvergleich unverändert.
- [x] Fehlende HP, beschädigte Namen und unbestätigte Zuweisungstypen werden abgelehnt. Frische Tier-/Itemnummern, HMAC-Rücklesung und Doppelsperre geprüft.
- [x] 299 Bilder dekodiert: 89 exakt zugeordnet und 210 ausdrücklich als Beispielvariante beschriftet. 23 Einträge behalten das allgemeine Symbol.
- [x] Sieben Reittier-UI-Tests einschließlich Beispielbeschriftung in Detailansicht und Zoom; keine Registrierung durch Bildklick.
- [x] Desktop-Update bei geschlossenem Spiel/Workbench installiert: 37 Dateien einschließlich aller 13 Saves und bestehender Mods unverändert; 23 Sicherungen und Desktop-Verknüpfung geprüft.
- [x] Nach Installation neuesten Slot 0 rein lesend geprüft: 219 verfügbare Arten (210 Pferde, sieben Bären, zwei Löwen); keine Saveänderung oder Tiervergabe.
- [ ] **MT06:** Neuesten Spielstand neu einlesen. Bei unverändertem geprüften Slot 1 sollten 219 Tiere verfügbar sein, darunter zwei weitere Löwen. Für andere Tiergruppen gilt inzwischen die oben dokumentierte Basiseintrag-Route.
- [ ] **MT07:** Pferdevariante ohne eigenes Porträt auswählen: „Beispielbild der Tierart“ und Hinweis auf abweichende Farbe/Ausstattung prüfen; beim exakten Porträt erscheint dieser Hinweis nicht.
- [ ] **MT08:** Nur falls ein neues Tier gewünscht ist: bei geschlossenem Spiel registrieren, richtigen Save laden, neues und ursprüngliches Tier samt Name, Zuweisung, Ausrüstung, Reiten und Speichern/Neuladen prüfen.

## Bildvorschau, Suche, Reittiere und NPC-Pfeile, 04.10.2026

Implementiert, technisch geprüft und bei geschlossenem Spiel installiert; Spielabnahme offen.

- [x] Echter Katalog: Teilwortsuche, Bindestriche, Groß-/Kleinschreibung, mehrere Suchwörter, exakte IDs und optionaler Regex-Modus.
- [x] Ungültige Regexmuster werden erklärt; nach Korrektur erscheint wieder die Ergebnisliste. Kategorien bleiben kombinierbar.
- [x] 57 eindeutig zugeordnete echte Tierporträts aus Spielarchiven dekodiert; fehlende Bilder bleiben als allgemeines Symbol erkennbar.
- [x] Private Savekopie: benanntes unzugewiesenes Stalltier als Vorlage; sein Name bleibt erhalten, neue Tiere haben frische Nummern und vollständigen Rückvergleich.
- [ ] **UI01:** Itembild und Tierporträt rechts anklicken, große Vorschau ansehen und mit Esc oder × schließen. Auswahl bleibt erhalten.
- [ ] **UI02:** Im gesamten Itemkatalog beliebige Teilwörter suchen, z. B. `holz`, `pfeil` oder `ring`. Regex optional testen, z. B. `^(Gift|Explosions).*pfeil`.
- [ ] **MT04:** Unter Reittiere neuesten Slot auswählen und Neu einlesen. Verfügbare Anzahl prüfen; **Nur hinzufügbare Reittiere** blendet Einträge ohne Vorlage aus.
- [ ] **MT05:** Falls ein neues Tier gewünscht ist: Spiel speichern und schließen, Tier registrieren, denselben Save laden, Stall, Reiten und erneutes Speichern/Neuladen prüfen.
- [ ] **AM01:** Blitzpfeil 1001315, Feuerpfeil 1001316 und Kältepfeil 1001314 zeigen Monstermunitionshinweis; keine Ausgabe möglich. Pfeil 50001, Giftpfeil 50003 und Explosionspfeil 1001321 bleiben auswählbar.
- [ ] **AM02:** Passende Pfeilvariante im Spiel ausrüsten und mit dem Aeserion-Bogen normal schießen. Nicht mit bereits vergebenen NPC-Blitzpfeilen testen; diese wurden unverändert gelassen.

Die Itemausgabe bestätigt die Inventarmenge; sie bestätigt nicht automatisch,
dass jedes interne Spielitem für den Spieler verwendbar ist.

## Angelrute und Fallschutz, 03.10.2026

- [x] Originale Krabbenangelrute im neuesten `slot2` ergänzt; Save vollständig verglichen.
- [x] 999 Plätze, bestehende Items/Sockel und Questfortschritt erhalten.
- [x] Vollständige native Fallroutine und 684 Schutzfälle für 342 Keys geprüft.
- [x] Übrige Figuren erhalten ursprünglichen Fall-HP-Verlust; Weiterleitung unverändert.
- [x] Neue ASI und drei bestehende ASIs gemeinsam isoliert geladen; falsche EXE abgelehnt.
- [x] Bei geschlossenem Spiel mit Startschutz installiert, 31 bestehende Dateien unverändert.
- [ ] Über Steam starten und neuesten **Slot 2** laden; Krabbenangelrute im Inventar prüfen.
- [ ] Angelrute am Angelplatz ausprobieren: automatisches Einholen prüfen.
- [ ] `bin64/CrimsonNoFall.log` auf neue READY-Zeile prüfen.
- [ ] Sicheren mäßigen Sturz mit Kliff, Damiane und Oongka testen: kein HP-Verlust.
- [ ] Dasselbe mit Löwe/Rokade prüfen; Schwarzstern bei Landung und Absteigen testen.
- [ ] Normale Kampftreffer sollen weiterhin HP abziehen; danach speichern/neu laden.
- Zehn Köder: noch nicht hinzugefügt; tatsächlicher Itemname fehlt.

[Ergebnis, genaue Reichweite und Sicherungen](.local/fishing-no-fall-20261003/RESULTAT.md).

## Löwe und Rokade: Tempo, Ausdauer und Löwen-HP, 03.10.2026

- [x] Beide Tiere eindeutig zugeordnet, alle sechs Stufen strukturell geprüft.
- [x] Zwölf Original-/Änderungsfälle mit den nativen Datenlesern des Spiels geprüft.
- [x] Zwölf abgeschnittene Eingaben abgelehnt; nur 20 gewünschte Werte verändert.
- [x] Archivgruppe `0049` bei geschlossenem Spiel mit Startschutz installiert und extrahiert.
- [x] Ältere Mods sowie alle zwölf Save-/Lobby-Dateien unverändert geprüft.
- [ ] Über Steam starten und Löwen sowie Rokade jeweils neu herbeirufen.
- [ ] Länger sprinten und mehrfach springen: Ausdauer soll nicht ausgehen.
- [ ] Tempo auf ebener Strecke vergleichen; konfigurierter Multiplikator 1,30.
- [ ] Beim Löwen neue maximale HP 1.350 prüfen; vorhandene aktuelle HP ggf. heilen.
- [ ] Speichern/neu laden und beide Tiere nochmals prüfen.

[Ergebnis und Einschränkungen der Offlineprüfung](.local/lion-rokade-upgrades-20261003/RESULTAT.md).

## Anfrage von Ben: Schlachten, 28.09.2026

- [x] Aktiven Schlacht-Schritt im neuesten `slot1` identifiziert und Save gesichert.
- [x] Zwölf Save-/Lobby-Dateien abschließend unverändert geprüft.
- Kein modifizierter Queststand zu testen: gemäß „nur wenn sicher“ kein Eingriff.
- [ ] Als regulären Weg bei Ben die Viehhofverwaltung öffnen, ein registriertes
  Tier auswählen und dort schlachten; danach den Berichtsschritt prüfen.

[Befund und Sicherung](.local/greymane-slaughter-20260928/RESULTAT.md).

## Aktueller Auftrag: 30 Stunden Schlaf und zusätzliche Sockel, 28.09.2026

- [x] 288 native Ereignis-/Kalenderfälle mit Hook und Tageswechsel geprüft.
- [x] Fremde Zeitänderungen und kürzere Schlafdauern bleiben unverändert.
- [x] Beide Schlaf-Plugins gemeinsam im isolierten Loader-Test geladen.
- [x] Bei geschlossenem Spiel mit Startschutz installiert; 17 bestehende Dateien identisch.
- [x] Neun Save-Verzeichnisdateien gesichert; Save und vorhandene Sockel unverändert.
- [ ] Über Steam starten, `CrimsonSleepDuration.log` auf READY prüfen.
- [ ] Die weiterhin „12 Stunden“ genannte Option wählen: ein Tag plus sechs Stunden erwarten.
- [ ] APPLIED-Log und tatsächlichen Zeitablauf, Animation und Spielwelt prüfen.
- [ ] Kürzere Schlafoptionen und Ein-Sekunden-Cooldown testen, speichern/neu laden.
- Fünf zusätzliche Sockel und Befüllung: **nicht implementiert; gemäß Nutzerentscheidung vorerst belassen**, kein Ingame-Test dafür vorgesehen.

[Ergebnis, technische Grenzen und Sicherung](.local/sockets-plus5-sleep30-20260928/RESULTAT.md).

## Aktueller Auftrag: Schlaf-Cooldown 1 Sekunde, 27.09.2026

- [x] Eigene 1.000-ms-Regel samt Grenzfällen und bestehenden Sperren geprüft.
- [x] Beide Originalfunktionen der aktuellen EXE isoliert mit Testdaten und Hooks geprüft.
- [x] Automatisches Laden per ASI Loader im privaten Testprozess geprüft.
- [x] Bei geschlossenem Spiel installiert, Dateien zurückgelesen, neun Save-Verzeichnisdateien gesichert.
- [x] EXE, bestehende Overlays/Registrierung und Save-Verzeichnisdateien unverändert.
- [x] READY-Eintrag aus einem echten Spielstart in `bin64/CrimsonSleepCooldown.log` gefunden (28.09.); Praxistest separat offen.
- [ ] Schlafen, mindestens eine Sekunde warten und erneut schlafen.
- [ ] Bei bestehender Sperre nach einer Sekunde die Interaktion erneut öffnen.
- [ ] Optional Warten am Lagerfeuer prüfen; anschließend speichern und neu laden.

[Ergebnis und genaue Testanleitung](.local/sleep-cooldown-1s-20260927/RESULTAT.md).

## Aktueller Auftrag: Graumähnen und kleine Knochen, 27.09.2026

- [x] Zwölf Save-/Lobby-Dateien gesichert; +250 kleine Knochen eingetragen.
- [x] Vollständiger Vergleich, inverse Prüfung, Startschutz und Rücklesen bestanden.
- [x] Graumähnen-Queststände nur gelesen; Fortschritt unverändert.
- [ ] Neuesten `slot1` laden und 253 kleine Knochen im Rucksack kontrollieren.
- [ ] Regulär speichern/neu laden und Menge kontrollieren.
- [ ] Questfreischaltung: offen, technisch noch nicht verifiziert/umgesetzt.

[Questbefund und Sicherung](.local/greymane-unlock-bones-20260927/RESULTAT.md).

## Aktueller Auftrag: Spendenreserve und Ringe, 27.09.2026

- [x] Aktuelle Missionszustände gelesen: Holz und Waffen offen; Geld, Nahrung, Stein abgeschlossen.
- [x] 1.616 hervorragendes Holz, 821 Elfenbein und zwei Ringe ergänzt; je 300 Materialreserve vorgesehen.
- [x] Standardstapel, Ring-Fundstufen, vollständiger Vergleich, inverse Prüfung und geschützter Commit bestanden.
- [ ] Neuesten `slot2` laden: 2.016 hervorragendes Holz, 992 Elfenbein,
  Mal der Finsternis +3 und Demeniss-Siegel +0 prüfen.
- [ ] Bei Carl Spendenwerte kontrollieren: Holz 19, Elfenbein 24 Punkte je Stück.
- [ ] Bis zu 1.316 Holz und 521 Elfenbein für die Endziele spenden; Reserve behalten.
- [ ] Beide Questabschlüsse, Ringverfeinerung sowie Speichern/Neuladen prüfen.

[Ergebnis und Sicherung](.local/camp-donations-reserve-20260927/RESULTAT.md).

## Aktueller Auftrag: Camp-Vorräte und Holz, 27.09.2026

- [x] Aktive Item-IDs und Nahrungsspendenwert geprüft; alle zwölf Save-Dateien gesichert.
- [x] Je 400 der drei Holzmaterialien und 100 Große Mahlzeit eingetragen.
- [x] Vollständiger Vergleich, inverse Byteprüfung, Startschutz und Rücklesen bestanden.
- [ ] Neuesten `slot1` laden: je 400 Holz und 100 Große Mahlzeit kontrollieren.
- [ ] Bei Carl den Nahrungsspendenwert der Mahlzeiten prüfen (rechnerisch 648 je Stück).
- [ ] Nahrung spenden und Fortschritt bei „Ein Tisch voller Überfluss“ prüfen.
- [ ] Regulär speichern/neu laden und verbleibende Mengen sowie Fortschritt prüfen.

[Ergebnis und Sicherung](.local/camp-provisions-wood-20260927/RESULTAT.md).

## Aktueller Auftrag: alle III-Sockelitems und Tempo, 27.09.2026

- [x] Alle 41 aktiven III-Abyss-IDs geprüft und jeweils einmal im Rucksack ergänzt.
- [x] Eile III im ersten angelegten Stiefelsockel eingesetzt; Hingabe I zurückgegeben.
- [x] Vollständiger Save-Vergleich, inverse Byteprüfung, Startschutz und Rücklesen bestanden.
- [ ] Neuesten `slot2` laden; 41 Zugaben, Eile III im Stiefel und Hingabe I im Rucksack prüfen.
- [ ] Mit aktueller Ausrüstung Statuswert **Bewegungsgeschwindigkeit 15** und Laufwirkung prüfen.
  **30 wurde nicht eingerichtet**; 15 ist das erlaubte reguläre Ausweichziel.
- [ ] Einen III-Stein bei der Hexe verwenden, speichern/neu laden und Fortschritt kontrollieren.

[Ergebnis, alle Mengen und Sicherung](.local/abyss-iii-speed-20260927/RESULTAT.md).

## Aktueller Inventarauftrag: Vorräte und Abyss-Ausrüstung, 26.09.2026

- [x] Acht Item-IDs anhand der aktiven Tabelle geprüft; zwölf Save-/Lobby-Dateien gesichert.
- [x] Je 500 der fünf Vorräte und je 10 der drei Abyss-Ausrüstungen ergänzt;
  Standardstapel, vollständiger Vergleich, inverse Prüfung und geschützter Commit bestanden.
- [ ] Neuesten `slot0` laden: 715 Kleine Knochen, 500 Platin, 500 reichliches
  gegrilltes Fleisch, 505 verbesserte Kraftfaust-Kräuterpastillen, 512 Diamanten
  sowie je 10 Zerstörung III, Verstärkung III und Sturmwind III prüfen.
- [ ] Fleisch verwenden; Pastillen bei der nächsten benötigten Wiederbelebung prüfen.
- [ ] Abyss-Ausrüstung bei einer Hexe einsetzen und die jeweiligen Boni prüfen.
- [ ] Regulär speichern, neu laden und Restmengen sowie Fortschritt kontrollieren.

[Ergebnis und Sicherung](.local/supplies-500-20260926/RESULTAT.md).

## Aktueller Inventarauftrag: sieben Materialien, 26.09.2026

- [x] Je 300 Eisenerz, Kupfererz, Azurit, Silbererz, Granat, Golderz und Epidot
  auf den frisch gesicherten neuesten `slot0` ergänzt. Vollständiger Vergleich,
  inverse Byteprüfung, Startschutz und Rücklesen bestanden.
- [ ] Neuesten Stand laden: 320 Eisenerz, 433 Kupfererz, 315 Azurit, 300 Silbererz,
  336 Granat, 300 Golderz und 311 Epidot im Rucksack prüfen.
- [ ] Beim Schmied verwenden, speichern/neu laden und Restmengen sowie Fortschritt prüfen.

[Ergebnis und Sicherung](.local/ores-300-20260926/RESULTAT.md).

## Aktueller Inventarauftrag: Verfeinerungsmaterialien, 25.09.2026

- [x] Aktuelle Rezept-/Itemtabellen geprüft, alle zwölf Save-/Lobby-Dateien gesichert.
- [x] Je 200 Stück von 27 Materialarten ergänzt, insgesamt 5.400; native Stapelgrenzen,
  eindeutige IDs, vollständiger Vergleich und inverse Byteprüfung bestanden.
- [x] Neuester `slot1` von 21:28 Uhr bei geschlossenem Spiel ersetzt und zurückgelesen;
  Installation, übrige Spielstände, bestehende Fähigkeiten und 999 Plätze erhalten.
- [ ] Diesen neuesten Stand laden: z. B. 200 Eisenerz, 239 Kupfererz, 205 Blutgestein,
  251 Skoleziterz, 204 Abyss-Artefakte und 200 Aeserions Schuppen im Rucksack prüfen.
- [ ] Waffe und Rüstung beim Schmied verfeinern, Materialabzug prüfen;
  Aeserions Schuppe bei Stufe 10 auf Verwendbarkeit prüfen.
- [ ] Speichern und neu laden: Mengen, Ausrüstung, Fähigkeiten und Inventar erhalten.

Aeserions Schuppen liegen gemäß ihrer nativen Stapelgrenze als 200 Einzelstücke vor.
[Vollständige Materialliste und Sicherung](.local/refinement-materials-200-20260925/RESULTAT.md).

Stand: 23.09.2026. Übersicht der Funktionen: [PHASENSTATUS](PHASENSTATUS.md).

**Die allgemeinen Phasentests bleiben zurückgestellt.** Für den separat
beauftragten Inventarfix läuft ein gezielter Spieltest. Die automatisierten Prüfungen
führe ich während der Entwicklung aus. Unten steht die spätere Abnahme mit
konkreten Erwartungen; noch fehlende Funktionen sind ausdrücklich markiert.

## Separater Inventarauftrag vom 23.09.2026

**Zusatzauftrag vom 25.09.2026, aktuell zurückgenommen:** Die Zugabe von 70
Abyss-Gegenständen entsprach laut Nutzer nicht der gewünschten Fähigkeitenressource.

- [x] Gegenstand aus aktiven Spieldaten identifiziert; zwölf Saves/Lobbys gesichert.
- [x] Sieben neue 10er-Stapel mit eindeutigen IDs; vollständiger semantischer
  Vergleich, inverse Byteprüfung und unabhängiger Codec geprüft.
- [x] Live-Save und Lobby zurückgelesen; übrige zehn Dateien sowie Modinstallation
  unverändert, 999 Inventarplätze und bestehende Gegenstände erhalten.
- [x] Um 15:31 Uhr vollständig zurückgenommen: beide Dateien und ursprüngliche
  Änderungszeiten bytegenau wiederhergestellt, ursprünglich fünf Exemplare erhalten.
- [x] Nutzer benennt Abyss-Verknüpfungen und bestätigt die Zugabe für alle drei
  Charaktere; die vorige Inventargegenstandsvergabe bleibt zurückgenommen.
- [x] Um 15:36 Uhr je 70 zusätzliche Einheiten in alle drei Fähigkeitenkonten
  eingetragen: 0/16/42 → 70/86/112. Vollständiger Save-Vergleich, inverse Prüfung
  und Rücklesen bestanden; gespeicherte Fähigkeiten, Inventar, Lobby und
  Installation im Dateivergleich unverändert.
- **Fehlgeschlagene Spielabnahme laut Nutzer:** Der Fähigkeitenbaum wurde
  nach der +70-Zugabe zurückgesetzt. Ursache nicht eindeutig geklärt.
- [x] Auf Nutzerwunsch um 15:52 Uhr je **150 zusätzliche** Einheiten ergänzt:
  70/86/112 → **220/236/262**. Sechs feste Felder ohne Datenverschiebung geändert;
  sämtliche übrigen entpackten Save-Bytes identisch. Rückerstattungszähler anhand
  der aktuellen EXE untersucht; Sicherung und Rücklesen abgeschlossen.
- [ ] Slot 0 vom 25.09., 01:23 Uhr laden und Guthaben im Fähigkeitenmenü bei
  Kliff, Damiane und Oongka prüfen. Zusätzlich prüfen, ob der Fähigkeitenbaum
  erhalten bleibt; der vorherige Reset ist nicht als behoben nachgewiesen.
- [ ] Verknüpfung zum Erlernen einer Fähigkeit einsetzen, speichern/neu laden
  und Restguthaben sowie erlernte Fähigkeit prüfen.

[Rücknahmebericht und Sicherungen](.local/abyss-artifacts-rollback-20260925/RESULTAT.md).
[Aktuelle Zugabe der Abyss-Verknüpfungen](.local/abyss-links-all-150-20260925/RESULTAT.md).

**Neuester Zusatzauftrag, 23:42 Uhr:** Im neuesten Stand `slot100` von 23:35 Uhr
sind 500 Schlüssel und 100 zusätzliche Hernand-Münzen eingetragen.

- [x] Frische Sicherung, vollständiger semantischer Vergleich, inverse Byteprüfung,
  unabhängiges Entpacken, eindeutige IDs und 113.590 Objektzeiger geprüft.
- [x] Stapeltabelle über das vorhandene Journal aktualisiert; vorherige Moddateien
  gesichert, Update auf Projektkopie getestet, installierte Dateien zurückgelesen.
- [x] Goldbarren und 999 Plätze erhalten; zehn andere Save-/Lobby-Dateien bytegleich.
- [ ] Neuesten Stand laden: 500 Schlüssel im Rucksack und 103 Hernand-Münzen bei
  den besonderen Gegenständen, jeweils ein Stapel.
- [ ] Einen Schlüssel verwenden und eine Münze bei Turnali einsetzen;
  anschließend speichern/neu laden und verbleibende Mengen sowie 999 Plätze prüfen.

[Bericht und Sicherungen für Schlüssel/Münzen](.local/keys-hernand-2976/RESULTAT.md).

**Aktueller Stand, 20:51 Uhr:** Build 25477059 / EXE 1.0.0.2976; neuester Slot 1.
Die früheren Mengen/Slots weiter unten dokumentieren die ersten Aufträge.

- [x] Aktuellen Fortschritt gesichert, 843 verbleibende Goldbarren von fünf
  Einträgen zu einem Stapel zusammengeführt; 38.401,01 Silber und je 200 Beutel erhalten.
- [x] Goldbarren-Stapelgrenze 1.000 und Inventarmaximum 999 im neuen Build
  installiert, Save-Erweiterung 949. Archive und Save unabhängig zurückgelesen.
- [x] Vollständiger Save-Vergleich, bytegenaue inverse Prüfung, Apply/Restore
  auf Projektkopie und 22 Bibliothekstests bestanden.
- [x] Spiel startet; neuesten Slot 1 geladen: ein Stapel mit 843 Goldbarren
  und 999 Gesamtplätze. Nutzerbestätigung: „Ja, beides stimmt“.
- [ ] Regulär speichern und neu laden: Menge, Stapel und Kapazität bleiben erhalten.
- [ ] Goldbarren verwenden/verkaufen und neue Gegenstände oberhalb der früheren
  240-Plätze-Grenze aufnehmen; danach speichern/neu laden.

[Aktueller Bericht](.local/inventory-stack-2976/RESULTAT.md).

- [x] Bei geschlossenem Spiel Slot 2 gesichert und um vier 100er-Beutelstapel
  sowie 30.000 Silber ergänzt. Neuer Geldbestand: 30.231,01 Silber.
- [x] Freie Plätze, eindeutige IDs, Lobby-Zähler, vollständiger Vergleich der
  übrigen Daten und Integrität nach erneutem Öffnen geprüft. Acht andere Dateien unverändert.
- [ ] Beim nächsten gewünschten Spieltest den geänderten Slot laden: insgesamt
  200 leichte/200 schwere Kupferbeutel und den Geldbestand kontrollieren.
- [ ] Je einen Beutel verwenden; anschließend regulär speichern und erneut
  laden. Verbleibende Mengen, Guthaben und übriges Inventar müssen erhalten sein.

Das Spiel wurde dafür nicht gestartet. [Bericht und Originalsicherung](.local/inventory-currency/RESULTAT.md).

**Anschließender Auftrag:** Der neueste Slot 0 enthält die vorherigen Beutel und
das Guthaben nach einem weiteren regulären Speichervorgang. Dort wurden zusätzlich
1.000 Goldbarren eingetragen; neue Sicherung und erneute Integritätsprüfung bestanden.

- [ ] Beim späteren Spieltest Slot 0 laden: 1.000 Goldbarren sowie weiterhin
  200 leichte/200 schwere Kupferbeutel und 30.231,01 Silber prüfen.
- [ ] Goldbarren verwenden/verkaufen, regulär speichern und erneut laden;
  verbleibende Mengen und korrekt verändertes Guthaben prüfen.
- [x] 999 Rucksackplätze eingerichtet: Tabellenmaximum 999 installiert,
  Save-Erweiterung auf 949 gesetzt (50 + 949). Sicherungen, vollständiger
  Bytevergleich, unabhängiges Entpacken und Integrität nach erneutem Öffnen geprüft.
  Gegenstände und Geld sowie die neun anderen Save-/Lobby-Dateien unverändert.
- [ ] Slot 0 laden: Anzeige von 999 Gesamtplätzen prüfen. Gegenstände oberhalb
  der bisherigen 109 Plätze aufnehmen und sortieren, später auch oberhalb
  der ursprünglichen Tabellenobergrenze 240. Regulär speichern und neu laden;
  Kapazität, Gegenstände und Mengen müssen erhalten bleiben.

Die Tabellenänderung besteht zwei Apply-/Restore-Zyklen auf einer Projektkopie
und ist inzwischen installiert; dies ist kein Spieltest.
[Abschlussbericht und Sicherung](.local/inventory-999-save/RESULTAT.md).

## Aktuelle Voraussetzung: Das Spiel wurde aktualisiert

Installiert ist inzwischen **Steam-Build 25477059 / EXE 1.0.0.2976**. Die vier
Tabellen für den aktuellen Inventarfix sind geprüft; die vollständige allgemeine
App-Unterstützung muss separat aktualisiert werden. Folgende Nachweise gelten
für den vorherigen Build **25455892 / EXE 1.0.0.2949**:
**Workbench v0.5.9 unterstützt dessen Tabellen und weiterhin den bisherigen
Build 25381195 / EXE 1.0.0.2944.** Von 38 erfassten EXE-/Metadatendateien sind 24
verändert. Das ist keine vollständige Inhaltsprüfung der Installation.

61 verwendete Tabellen-, Icon- und Sprachdateien sind verglichen; alle 14
indizierten Tabellenpaare bestehen den bytegleichen Roundtrip. Beim Modvergleich
bleiben alle ungewählten Stages einschließlich der zwei neuen Einträge erhalten.
Die vollständige native Reparatur ist weiterhin nicht freigegeben. Der historische
Testhost verweigert alte Funktionskopien aus dieser EXE; die neue Probe besitzt
eigene, getrennte Funktions- und Layoutnachweise.
Ein alter Prüfbericht oder ein altes Backup ist keine Grundlage für Apply/Restore
über den neuen Zustand der Installation. [Buildnachweis](docs/BUILD_SUPPORT.md).

## Bereits automatisch geprüft

- [x] App-Prüfung für v0.5.9: 181 Rust-Tests, sieben Frontend-Unit-Tests und
  61 UI-Flows; zusätzlich der gesonderte Vergleich der bisherigen Advanced-Tabellen.
  Die Integrationstests lesen den neuen Build 1.0.0.2949.
- [x] Gebaute Desktop-App v0.5.9: Vorschau mit 3.359 Änderungen über 13 Tabellen,
  zwei Apply-/Restore-Zyklen an einer Projektkopie, keine Seitenfehler.
  Spiel-Metadaten und ursprünglicher Prüfbericht bleiben unverändert.
- [x] Beide Build-Identitäten bleiben getrennt. Sicherungsberichte mit gemischten
  Metadaten, falscher Build-ID, fehlenden Dateien oder falschen Größen werden abgewiesen.
- [x] 52.082 Stages und 1.098 Quests bleiben vollständig lesbar. Modänderungen
  betreffen nur die beiden gewählten Patrouillen; Questdateien bleiben außerhalb des Overlays.
- [x] Aktuelles Reparatur-Entwicklungsmodul 0.23.0: neun CTest-Suiten,
  MSVC Release mit Warnungen als Fehler.
- [x] 1.529 Aktionsbedingungen, 519 Leser-/Referenzbedingungen,
  25 Sperrgruppenbedingungen und 55 neue Registry-Adapterbedingungen.
- [x] Fehlerhafter Erwerb, alte/vollständige Kennung, ungültige Bindung,
  teilweise erworbene Referenzen und Freigabe in umgekehrter Reihenfolge.
- [x] Jede Quelle verwendet ihre eigene Freigabemethode; keine globale
  Methodenbindung, die durch einen anderen Kontext überschrieben werden kann.
- [x] Native Erwerbs-/Sperr-/Inventar-/Ereignis-/Speicherobjektprobe auf 1.0.0.2949: 454 Szenarien,
  4.812 native Aufrufe und 206.786 Bedingungen, einschließlich wiederholter Byte-/Tabellenprüfungen. Echte Windows-Sperren halten die
  Referenzsuche während eines Schreibzugriffs an; danach entfernte Einträge werden abgewiesen.
- [x] Fehler beim zweiten Besitzer geben die erste Referenz zurück. Ungültige
  Belege gelten trotz enthaltenem Pointer nicht als Besitz. Normal-/User-Varianten,
  alte Generationskennung, fehlende Einträge und verzögerte Freigabe sind geprüft.
- [x] Der Leser nutzt nur gehaltene Besitzer und liest keine ungeschützte Registry
  oder unbesessene Ersatzauswahl. Bei blockierter Besitzersperre werden keine Items
  gelesen; Fehler geben alle Sperren vor den Referenzen zurück.
- [x] Gemeinsame Erfassung von Inventar, Ausrüstung und Sockeln unter echten
  Besitzersperren; Wechsel von Clientauswahl, Kennung, Daten und Sperrbindung
  werden geprüft. Die verwendeten Lesefelder besitzen inzwischen eigene 2949-Belege.
- [x] Originale Inventar-/Besitzergetter des neuen Builds stimmen auf identischen
  privaten Quellen mit dem Leser überein. Vier native Unwindfragmente geprüft.
- [x] Aktuelle Clientauswahl und Ausrüstungs-Slotwahl nativ geprüft; der
  Ausrüstungstest verwendet Delta 0 und beweist noch keine Reparaturereignisse.
- [x] Unbekannte/nicht angegebene Leser-Builds werden vor Erwerb und Lesen abgewiesen.
- [x] Original-Updater reproduziert auch im neuen Build den schädlichen positiven
  Sockel-Reparaturfall; er wird nicht als eigene Reparaturfunktion verwendet.
- [x] Ereignisvorbereitung behält den vorherigen Broken-Zustand und berücksichtigt
  No-Wear; mitgeführte Items dürfen den Ausrüstungsweg nicht verwenden.
- [x] Originaler Server-Notifier und Client-Ack auf privaten reparierten Items:
  richtige Effekt-/UI-Aufrufe, No-Wear, Sockelerhalt und vollständige Nachherwerte.
  Transport, Effektverarbeitung und UI-Empfänger sind künstliche Testabhängigkeiten.
- [x] Fehlende/falsche/doppelte Rückmeldung, falscher Clientslot, Teiländerung,
  fehlender Ereignisbeleg und Sitzungswechsel erzeugen keinen Reparaturerfolg.
- [x] Native Itemkopien besitzen eigene Sockel-/Zusatzdaten. Wiederholte Zuweisung,
  Selbstzuweisung, leere Daten und beide Allokationswege erhalten die Quelle und
  geben alle Kopierpuffer frei. Zusätzliche 29 Bedingungen prüfen den Besitzervertrag.
- [x] Ausrüstungsereignisse verwenden jetzt diese echten nativen Nachherkopien:
  Kopieren unter Besitzersperren, Benachrichtigung nach Entsperren, Freigabe danach.
- [x] Originale Markierung geänderter Ausrüstungsslots: Duplikate, Kollisionen,
  Wachstum und beide Allokationswege. Die Ereignisprobe markiert vor dem Entsperren;
  Verbraucher und native Tabellenbereinigung sind inzwischen ebenfalls integriert.
- [x] Gehaltene Erfassung: Besitzer und Sperren bleiben für Planung/Vorprüfung
  erhalten. Refresh, fremder Thread, veralteter Batch und geänderte Sitzung geprüft;
  Fehler geben alle Sperren vor den Referenzen frei.
- [x] Feldschreiber: 35 Szenarien und 202 Bedingungen. Gesamter Batch vorab geprüft;
  anschließend nur geänderte Haltbarkeitswörter in beiden Zustandskopien geschrieben.
  Veraltete/überlappende/unbeschreibbare Quellen, Teilfehler, leere Slots und
  Metadaten geprüft; acht native Integrationsfälle verwenden echte eigene Speicherblöcke.
- [x] Gemeinsamer Ablauf: 43 Szenarien und 273 Bedingungen. Beide Nachherkopien
  jedes Items entstehen vor dem ersten Store; alle Meldungswege müssen vorhanden
  sein. Kopier-, Schreib-, Markierungs- und Meldefehler erzeugen keinen Erfolg.
- [x] Die 18 nativen Ausrüstungsfälle verwenden diesen Adapter einschließlich
  Originalkopien, Schreiber, Slot-Markierung und Server-/Client-Ereignisweg.
  Bestätigung liest anschließend unter neu erworbenen Besitzern; falsche Slots
  oder widersprüchliche Zustandskopien werden abgewiesen.
- [x] Batchabschluss nach allen Meldungen: fehlende Bindung verhindert das
  Schreiben; spätere Abschlussfehler oder Sitzungswechsel ergeben keinen Erfolg.
- [x] 24 native Fälle für die Verarbeitung markierter Ausrüstungsslots. Exakte
  UID-/Haupt-Haltbarkeitspaare, 64 Slots, Clear/Wiederverwendung/Freigabe und
  Fehler nach bereits geleerter Liste geprüft. Die Persistenzschnittstelle bleibt
  ein Testempfänger; tatsächliche Speicherbestätigung ist damit nicht nachgewiesen.
- [x] Leere und teilweise gespeicherte Sockelplätze geprüft: Es werden nur
  vorhandene Datensätze gelesen, auch wenn mehr Sockelplätze möglich sind.
  Falsche Zusatzdaten und übergroße Allokationen werden vor dem Kopieren abgewiesen.
- [x] 36 native Inventarereignisfälle: originaler Client-Ack mit absoluten
  Hauptwerten, ungültigen/leeren Slots und exaktem UI-Aufruf. Acht Fälle verbinden
  den Ack mit Kopien und Feldschreiber; UID-Wechsel, fehlende/doppelte Zustellung
  und Abschlussfehler verhindern Erfolg. Server, UI und Persistenz bleiben Testempfänger.
- [x] 36 zusätzliche Fälle für originalen Inventar-Paketserializer und Streamauswahl:
  exakte zehn Nutzbytes, keine UID-/Sockel-/Auftragsdaten, fehlende Empfänger/Puffer,
  unpassende Kanäle und fehlgeschlagener Poolbezug. Zehn Transaktionen verbinden
  den Sender mit dem Client-Ack; Fehler, ausbleibende/doppelte Zustellung, UID- oder
  Actor-Abweichung zählen nicht als Erfolg. Reale Transport-/Speicherbindung offen.
- [x] Neue EXE wird vom alten nativen Testhost vor Funktionsausführung abgelehnt.
- [x] 19 native Fälle mit vollständiger SQL-Ausführungsfunktion und originalem
  Request-Erwerb: Umgehungszweig, alte Fehler, Diagnosefristen, Nullzeiger und
  Integration mit dem Slot-Verbraucher. Erfolg ist ohne Lesen oder Speichern des
  Requests möglich; diese Rückgabe zählt ausdrücklich nicht als Speicherbestätigung.
- [x] 38 native Fälle für Item → Speicherobjekt → Item: Hauptwerte 0/beschädigt/voll/
  Sentinel/über Maximum, No-Wear, freie und teilweise belegte Sockel. 14 Fälle
  verwenden tatsächliche Reparaturpläne und native Nachherkopien. Färbedaten,
  Zurücksetzen/Wiederverwenden, Quellenerhalt und vollständige Freigabe geprüft.
  Beide privaten Allokationsmodi bestehen. Kein Nachweis eines Schreibens auf Datenträger.
- [x] 39 native Speicherablauffälle: alle Skip-/Dirty-/Statuskombinationen, vier
  Fehlerstufen, Wiederholung, fehlendes Backend, Steam-Zulassungsprüfung und
  Wachstum dreier Warteschlangen. Originaldispatcher und Marker laufen auf
  privaten Daten; Datei-/Plattformempfänger bleiben künstlich. [Grenzen](runtime/repair/SAVE_BACKEND_2949.md).
- [x] Datenlesen wählt das neue Leseschema; der Suchindex führt die passende
  Schema-ID. Das erteilt keine Vanilla-Zulassung oder Live-Apply-Freigabe.

Der historische vollständige native Host am **alten** Build hatte 203 Aufrufe
und 3.859 Bedingungen (Modul 0.6.0). Die aktuelle Probe ergänzt eigene Nachweise
für Inventar-/Ausrüstungszugriffe und Teile der Ereignisfolge; sie ist weiterhin
kein Nachweis einer vollständigen Reparatur im Spiel.
[Aktuelle Testgrenzen](runtime/repair/INVENTORY_EVENTS_2949.md).
Die Engine-Threadmap und endgültige Objektbereinigung bleiben Testabhängigkeiten.
Details: [TESTING](docs/TESTING.md).

## Noch von mir technisch zu prüfen

- [x] Neue Tabellen-/Sprachdateien, Parser-Roundtrips, geänderte Datensätze und
  Mod-Diffs zuordnen. [Vergleich](docs/BUILD_SUPPORT.md).
- [x] Versionsauswahl vor der Prüfung einer Mod-Sicherung: Der unterstützte neue
  Build lädt seine Originaltabellen; ein eigener Testbestand mit unbekannter
  PE-Version wird vorher mit dem Versionsfehler abgewiesen.
- [x] Geschützte Client-/Server-Suche mit tatsächlichen Referenzmethoden und
  Freigabe auf dem neuen Build gemeinsam isoliert ausführen.
- [x] Gehaltene Referenzen direkt für die Inventarerfassung nutzen; keine zweite
  ungeschützte Suche. Erwerb, Besitzersperren und Leser gemeinsam auf privaten Daten prüfen.
- [x] Native Funktionsgrenzen und Leselayouts für Inventar, Besitzer, aktuelle
  Auswahl, Ausrüstung und Sockel auf dem neuen Build belegen.
- [x] Ausrüstungsereignisse korrekt vorbereiten und die Server-/Client-Aufrufwege
  mit privaten Transport-, Effekt- und UI-Empfängern prüfen.
- [x] Originale Itemkonstruktion, tiefe Kopie und Destruktion mit eigenen
  Allokationen prüfen und in die Ereignisprobe integrieren.
- [x] Originale Slot-Markierung in die Ausrüstungsprobe integrieren und eine
  durchgehend geschützte Erfassung für die Reparaturvorprüfung implementieren.
- [x] Vollständigen Haltbarkeits-Feldschritt für mitgeführte und ausgerüstete Items
  mit Vor-/Nachprüfung und Behandlung unklarer Teilfehler implementieren.
- [x] Feldschreiber mit vorab vollständig vorbereiteten nativen Nachherkopien
  und bereichsabhängigen Meldungsadaptern verbinden; Ausrüstungsweg nativ isoliert prüfen.
- [x] Originalen Inventar-Client-Ack mit dem privaten gemeinsamen Ablauf verbinden;
  UID-Wechsel und unvollständige Rückmeldungen separat abweisen.
- [ ] Serverseitige Inventarmeldungen, gemischte Aufträge und vollständige echte
  Effekt-/UI-/Rückmeldewege prüfen. Der Client-Ack allein bestätigt keine Speicherung.
- [x] Verbraucher der Slot-Meldungen und native Bereinigung im privaten
  Ausrüstungsablauf anbinden; leere Liste allein gilt nicht als Erfolgsbestätigung.
- [ ] Tatsächliche Persistenz und ergänzenden Sockelweg anbinden. Der belegte
  Ausrüstungsverbraucher übergibt nur UID und Haupt-Haltbarkeit. Der untersuchte
  Socket-SQL-Helper überträgt die Itemkennung. Der getrennte ItemSaveData-Weg
  übernimmt Haupt- und Sockelwerte auch in der nativen Probe; der tatsächliche Save-Vorgang fehlt.
- [x] Originale Item-/Speicherobjekt-Umwandlungen auf privaten Quellen ausführen:
  Hauptwerte, Sentinel, mehrere Sockel, freie Plätze, Identität und Färbedaten;
  reparierte Kopien und Wiederverwendung einbeziehen.
- [ ] Weitere optionale Itemdaten und die tatsächlichen Engine-Einstellungen
  zur Sockelanzahl prüfen. Die Probe bindet diese Einstellungen selbst; der Lader
  erzeugt fünf Datensätze, auch wenn die logische Grenze kleiner ist.
- [ ] Reguläre Erzeugung/Serialisierung und Abschluss der Speicherobjekte anbinden;
  die Umwandlung im Arbeitsspeicher allein bestätigt noch keinen gespeicherten Auftrag.
- [x] Dateihelfer mit privaten Puffer-/Handle-Empfängern prüfen: 34 native Fälle
  für Vorbereitungs-/Öffnungs-/Schreib-/Flush-/Closefehler, leere Ausgabe und
  Lebensdauer in beiden Allokationsmodi. [Nachweis](runtime/repair/SAVE_FILE_2949.md).
- [x] Native Pufferaufbereitung, Allokation und LZ4-Kompression mit Größen bis
  131.072 Byte prüfen; Fehler, Ausgabe-Wiederverwendung und erneut aufgerufene
  Einträge einbeziehen. 66 Fälle bestanden. [Nachweis](runtime/repair/SAVE_ENCODING_2949.md).
- [ ] Nachgelagerte Verarbeitung einschließlich Integrität/Verschlüsselung, den
  Queue-/Backup-/Umbenennungsweg und belastbaren Abschlussbeleg anbinden.
  Nach fehlgeschlagenem Öffnen müssen neue Versuche frische Rohdaten verwenden. Flush-/Closefehler können im Originalcode trotzdem
  Erfolg und geleerte Puffer erzeugen. Eine interne Erfolgsmeldung genügt nicht.
- [ ] Reparaturänderungen und vollständige Ereignis-/Effektpfade auf dem neuen Build belegen.
- [ ] Aktuellen Spieler und Manager auf einem belegten Engine-Thread auflösen;
  Welt-/Charakterwechsel, Wiederverwendung von Kennungen und TLS-Moduswechsel
  dürfen keine fremden oder veralteten Besitzer akzeptieren.
- [ ] Vollständige Reparaturtransaktion samt Inventar-/Ausrüstungsereignissen,
  abgeleiteten Effekten und eindeutiger Rückmeldung entwickeln und prüfen.
- [ ] Neue Module gegen Fehler, Teilabbrüche, Wiederanwendung, Rücknahme und
  Updates prüfen. Die Testdaten liegen außerhalb der Spielinstallation.
- [ ] Phasen 6–8 nach ihrer Implementierung automatisiert prüfen.

## Spätere Testreihenfolge für dich

Erst nach Unterstützung des dann installierten Builds und Abschluss der
Entwicklung: zunächst App bedienen, dann B0 mit einer kleinen Änderung prüfen,
danach jedes Modul einzeln und zuletzt Kombinationen. Für Installationsänderungen
Spiel schließen; Wirkung nach dem nächsten Start prüfen. Rücknahme ebenfalls
bei geschlossenem Spiel. Die Workbench schreibt keine Saves.

Für einen Vergleich dieselbe Ausgangssituation verwenden und Vorher-/Nachherwerte
notieren. Ein einzelner Zufallsdrop belegt keinen Multiplikator. Bei Problemen
Fall-ID, App-/Spielversion, Einstellungen, Ist-Ergebnis und gegebenenfalls einen
Screenshot festhalten.

### Phasen 0–3: Daten und Bedienung

| ID | Test | Erwartetes Ergebnis |
|---|---|---|
| A01 | Unterstützte Installation auswählen; anschließend einen unbekannten Build prüfen | Passende Daten laden; unbekannter Build wird klar abgelehnt, keine scheinbar gültigen Daten oder Änderungen |
| A02 | Deutsches Item suchen, Filter kombinieren, blättern, Details und Icon öffnen | Richtiger Datensatz, keine vertauschten/veralteten Details, Bedienung bleibt flüssig |
| A03 | Sprache wechseln und Suche wiederholen | Namen/Details passen zur Sprache; fehlende Übersetzungen und rohe Felder bleiben erkennbar |
| A04 | Verfügbare Item-/Rezeptlinks und JSON-Export benutzen | Richtige Ziele und Werte; Export landet im Projekt, keine Änderung am Spiel |
| A05 | Bekanntes Rezept mit einer und mehreren Zielmengen berechnen | Materialbaum und Gesamtbedarf stimmen; Chargenrundung ist nachvollziehbar |
| A06 | Vorräte, Rezeptalternativen und gemeinsam benötigte Materialien ändern | Vorräte werden insgesamt nur einmal abgezogen; keine negativen Mengen oder doppelt gezählten Restbedarfe |
| A07 | Nach einem unterstützten Spielupdate die Daten erneut öffnen | Daten und Suchindex gehören zum neuen Build; alte Sicherungsberichte erhalten keine automatische Schreibfreigabe für die neue Installation |

Unbekannte Händler-/Fundortzuordnungen müssen als unbekannt erscheinen und sind
kein bestandener Quellenvergleich. Der vollständige Ausbau dieser Verknüpfungen fehlt noch.

### Phase 4: Apply, Shops, Drops und Vertrauen

| ID | Test | Erwartetes Ergebnis |
|---|---|---|
| B01 | Kleine Änderung in der Vorschau prüfen | Nur gewähltes Feld/Modul und zugehörige Dateien ändern sich; Vorschau schreibt nichts ins Spiel |
| B02 | Apply/Restore bei laufendem Spiel versuchen | Vor jeder Änderung gesperrt; Spiel läuft unverändert weiter |
| B03 | Bei geschlossenem Spiel kleine Änderung anwenden, starten, Wirkung prüfen, schließen und zurücknehmen | Gewählte Wirkung vorhanden; Rücknahme bestätigt die zum aktuellen Build gehörende Basis |
| B04 | Gleiche Einstellung erneut anwenden, danach einen anderen Faktor und schließlich das Modul deaktivieren | Keine aufeinander gestapelten Multiplikatoren; deaktivierte Wirkung verschwindet |
| B05 | Ein unterstützter Händler: kleines Sortiment, Bestand, Kauf, Tageswechsel, Ausnahmen | Richtige kaufbare Artikel und Auffüllung; einmalige Angebote bleiben einmalig, keine Duplikate |
| B06 | Dropchance und Menge erst getrennt, dann gemeinsam prüfen | Mengen passen; Chancen über ausreichend viele gleiche Versuche vergleichen. Manuelle Garantie gilt nur für das gewählte unterstützte Set |
| B07 | Dieselbe Vertrauensaktion beim selben NPC vergleichen | Positiver Zuwachs passt zum Faktor; negative Strafen bleiben unverändert |

Update-, Stromausfall- und Crashfälle werden zuerst in isolierten Installationen
automatisiert geprüft. Du sollst dafür keinen Absturz oder Stromausfall provozieren.
Alle Artikel bei allen Händlern und eine automatische Bosszuordnung sind nicht freigegeben.

### Phase 5: Erweiterte Module

| ID | Test | Erwartetes Ergebnis |
|---|---|---|
| C01 | Eine ausgewählte Spawn-Gruppe und eine Ausnahme vergleichen | Belegte Spawnzahlen/-limits passen; ausgeschlossene Gruppen bleiben unverändert |
| C02 | Unterstützte Patrouillen-/Wiederbesetzungsregel auslösen | Gewählte Regel verändert sich nachvollziehbar; dies ist kein Test eines allgemeinen NPC-Respawn-Timers, der noch fehlt |
| C03 | Reittier in Stadt, mehrere Regionen, Auf-/Absteigen, Interaktion und Questübergang | Belegte Freigaben/Dauern wirken; kein festhängender Zustand oder unerwarteter Verlust von Interaktionen |
| C04 | Blackstar rufen, erneut rufen, Ridedauer und Regionsgrenzen prüfen | Cooldown/Dauer wie eingestellt; allgemeine Grenzen und Sonderzonen getrennt dokumentieren |
| C05 | Inventar und jeden verwendeten Lagerbereich mit einer moderaten Platzänderung prüfen | Kaufen, Aufheben, Verschieben, Sortieren und Neuladen funktionieren; keine verschwundenen Gegenstände |
| C06 | Materialien/Konsumgüter stapeln, teilen, kaufen, lagern und neuladen | Mengen bleiben exakt erhalten und Stapelgrenzen stimmen |
| C07 | Experimentelle Instanzitems erst gesondert prüfen | Haltbarkeit, Verzauberungen, Sockel und Identität bleiben pro Exemplar erhalten; keine Vermischung |
| C08 | Waffen/Werkzeuge/Sockel belasten und tatsächliche Haltbarkeit vergleichen | Bei No-Wear kein tatsächlicher Wertverlust; eine größere Maximalanzeige allein reicht nicht |
| C09 | Sprinten, Klettern, Fliegen und verschiedene Skills mit Kostenfaktor 1, 0,5 und 0 vergleichen | Verbrauch der unterstützten Kategorie passt; Regeneration bleibt erhalten; unbekannte Kosten separat festhalten |
| C10 | Einen Skill und globalen Cooldown ändern, dann eine Ausnahme setzen | Gewählte Priorität wirkt; andere Skills bleiben entsprechend ihrer Einstellung unverändert |
| C11 | Ein Item mit bestätigtem Stat/Buff/Enchant ändern, ausrüsten und neuladen | Anzeige und tatsächlicher Effekt passen; keine Endlosschleife, verlorenen Eigenschaften oder unbeabsichtigten Änderungen |

Größenlimits, Spezialcontainer und Instanzitems sind noch nicht universell
abgenommen. Keine sofortigen Maximalwerte als ersten Test verwenden.

### Kostenlose Reparatur: erst nach fertiger Spielanbindung

Die Reparatur ist aktuell gesperrt und noch nicht manuell testbar.

- [ ] **R01:** Vorhandenes beschädigtes Einzelitem reparieren: korrekter Zielwert,
  kein Geld-/Materialverbrauch; Menge, UID, Verzauberung und Sockelbelegung bleiben erhalten.
- [ ] **R02:** Mitgeführte und ausgerüstete Items getrennt sowie zusammen reparieren;
  Hauptitem und beschädigte Sockeleinsätze jeweils unabhängig prüfen.
- [ ] **R03:** Unbeschädigte Items, Items ohne Haltbarkeit und leere Slots ergeben
  keine unnötigen Änderungen; verbrauchte/zerstörte Exemplare werden nicht neu erzeugt.
- [ ] **R04:** Mit aktiviertem No-Wear kombinieren; Werte, Darstellung und spätere
  Benutzung müssen dem definierten Reparaturziel entsprechen.
- [ ] **R05:** Schnell wiederholt auslösen sowie Ausrüstungs-/Charakterwechsel und
  Laden/Regionswechsel prüfen: keine Doppelreparatur, falsche Besitzer oder hängenbleibenden Aufträge.
- [ ] **R06:** Reparaturerfolg muss in Inventar, Ausrüstung, abgeleiteten Effekten
  und nach regulärem Speichern/Neuladen durch das Spiel übereinstimmen. Zusätzlich
  das Spiel normal beenden, neu starten und Hauptitem sowie jeden Sockel vergleichen.
  Eine Erfolgsmeldung oder sofort korrekte Anzeige allein genügt nicht. Vorher-/
  Nachherwerte und Werte nach dem Neustart für dasselbe Exemplar festhalten.
- [ ] **R07:** Bei abgewiesenem Auftrag keine Änderung; bei unklarem Ergebnis
  klare Meldung und keine automatische Wiederholung. Technische Fehler werden
  zuerst von mir im Testhost simuliert.
- [ ] **R08:** Jeden steuerbaren Protagonisten einzeln prüfen, danach mit
  anwesenden Begleitern und beim Reiten: Reparatur betrifft ausschließlich den
  ausgewählten Spielerbereich, keine Begleiter-, NPC-, Händler- oder Lageritems.
- [ ] **R09:** Nach regulärem Neuladen neu auslösen: Ein alter Auftrag darf nicht
  in die neue Sitzung gelangen, auch wenn Charakter und Gegenstände gleich aussehen.
- [ ] **R10:** Ein noch vorhandenes Item mit Haltbarkeit 0 reparieren und mit einem
  nur teilweise beschädigten Item vergleichen: Ausrüstungswerte und Buffs müssen
  direkt passen, ohne Aus-/Anziehen oder erneutes Öffnen des Inventars. Dasselbe
  mit reiner Sockelreparatur und aktiviertem No-Wear prüfen; keine doppelten Effekte.
- [ ] **R11:** Zwei Exemplare desselben Items mit unterschiedlichen Sockeln und
  Eigenschaften besitzen: zuerst eines gezielt, später beide reparieren. Jede
  Identität und ihre Eigenschaften müssen erhalten bleiben; keine Übertragung
  zwischen Exemplaren. Anschließend regulär neuladen und erneut vergleichen.
- [ ] **R12:** Mehrere beschädigte Ausrüstungsslots in einem Auftrag reparieren,
  danach unmittelbar Werte/Effekte prüfen und erneut auslösen. Jeder gewählte Slot
  aktualisiert sich genau einmal; bereits volle Items bleiben unverändert. Auch
  nach Aus-/Anziehen, Regionswechsel und regulärem Neuladen stimmen die Werte.
- [ ] **R13:** Ein Item mit verfügbaren, aber unbelegten Sockelplätzen und eines
  mit nur teilweise belegten Plätzen reparieren. Hauptwert und vorhandene Einsätze
  stimmen; freie Plätze bleiben frei. Keine neuen Sockel oder Gegenstände entstehen.
  Danach normal speichern/neuladen und Belegung sowie Eigenschaften vergleichen.
- [ ] **R14:** Ein gefärbtes Item mit mehreren unterschiedlichen Färbungen reparieren,
  regulär speichern und neu laden. Alle Farben/Eigenschaften bleiben beim richtigen
  Exemplar; ein anschließend repariertes anderes Item übernimmt keine alten Daten.

Für R02/R06/R10 die Ausgangslagen einzeln verwenden:

| Ausgangslage | Nach Reparatur prüfen |
|---|---|
| Hauptitem beschädigt, Sockel intakt | Hauptwert voll; Sockel, Menge und Eigenschaften unverändert |
| Hauptitem noch vorhanden, Haltbarkeit 0 | Hauptwert voll; zuvor deaktivierte Ausrüstungseffekte wieder wirksam |
| Hauptitem intakt, Sockel beschädigt | Sockel voll; kein Sockel verschwunden, keine zusätzlichen Hauptitem-Effekte |
| Hauptitem und Sockel beschädigt | Beide unabhängig voll; Anzeige und tatsächliche Wirkung stimmen |
| No-Wear aktiv | Definiertes Reparaturziel erreicht; keine falsche oder doppelte Effektaktivierung |

### Phasen 6–8: nach deren Implementierung

| ID | Test | Erwartetes Ergebnis |
|---|---|---|
| D01 | Profile anlegen, exportieren/importieren und Normal ↔ Farm wechseln | Alle Einstellungen vollständig; Drops ×10/Spawns ×3 anpassbar; kein Wechsel während laufendem Spiel |
| D02 | Ein fehlerhaftes Profil importieren | Verständliche Ablehnung; bestehende Profile bleiben erhalten |
| E01 | Save auswählen, bekannten Questfortschritt vergleichen und im Spiel weiterspielen/speichern | Richtige Zustände und Aktualisierung; keine Schreibzugriffe oder Blockierung des Spiels durch den Leser |
| E02 | Nicht unterstützten/beschädigten/gerade unvollständig geschriebenen Save lesen | Keine erfundenen 0 %; Fehler erkennbar, letzter gültiger Stand bleibt erhalten |
| F01 | Kartenpunkte an mehreren bekannten Orten vergleichen | Positionen auch außerhalb der Kalibrierpunkte korrekt; Suche, Ebenen und Details passen |
| F02 | Sammelzustand vergleichen, sofern vom Saveformat unterstützt | Erledigte Marker stimmen; fehlende Daten werden als unbekannt geführt |

Der optionale Farm-Hotkey ist ein eigenes späteres Thema und gehört noch nicht
zu dieser Abnahme.

## Einfaches Ergebnisprotokoll

| Datum | Fall-ID | App-/Spielversion | Einstellung / Vorherwert | Erwartet | Beobachtet | Bestanden / Fehler / offen |
|---|---|---|---|---|---|---|
| | | | | | | |

Ausführliche Entwicklernachweise: [docs/TESTING.md](docs/TESTING.md).

## Zusatzauftrag Schwarzstern – installiert am 01.10.2026

- [ ] **BS01:** Spiel über Steam starten, neuesten Save laden; normaler Start ohne Absturz. Aktuelle READY-Zeile mit `early summon action support` und `14 hooks` prüfen; ältere Zeilen nicht als Nachweis verwenden.
- [ ] **BS02:** Schwarzstern entlassen und erneut rufen; etwa eine Sekunde Cooldown, keine doppelte Instanz. Nach Speichern/Neuladen wiederholen.
- [ ] **BS03:** Mehrere Minuten Flug, Beschleunigung und Feueratem; Ausdauer geht praktisch nicht aus, Lebenspunkte bleiben normal.
- [ ] **BS04:** In zuvor gesperrtem freiem Außenbereich beschwören; an Haus, Höhle und Innenraumgrenze Ablehnung prüfen.
- [ ] **BS05:** Andere Reittiere und vorhandene Schlaf-/Inventarmods funktionieren unverändert.
- [ ] **BS06:** Bei anhaltendem Flug bleibt Schwarzstern länger als zehn Minuten beschworen. Die neue Grunddauer und der vorhandene Resttimer in Slot 1 wurden auf 1.000 Minuten gesetzt; Speichern/Neuladen und erneutes Beschwören ebenfalls prüfen. Vollständiger 1.000-Minuten-Spieltest bisher nicht durchgeführt.
- [ ] **BS07:** In eines der roten Gebiete aus dem Screenshot hineinfliegen und in einer Stadt abseits von Straßen tief fliegen; kein Abwerfen durch die bisherigen Gebiets-/Stadtprüfungen.
- [ ] **BS08:** Schwarzstern an genau der zuletzt gesperrten Außenstelle im roten Gebiet beschwören. Der Nutzer meldete mit der vorherigen Version weiter ungefähr „Reittiere können hier nicht gerufen werden“; diese Abnahme ist bisher fehlgeschlagen. Bei erneuter Ablehnung Standort und Meldung festhalten, danach speichern und Spiel vollständig schließen. Neue `GAME_ERROR`-/`ATTEMPT`-/`REFUSED`-Zeilen aus `bin64/CrimsonBlackstar.log` auswerten. Die aktuelle Version erfasst frühere Aktionen; ein universeller Erfolg ist weiterhin unbestätigt.
- [ ] **BS09:** Über die bisherige Höhenbegrenzung fliegen; kein Abwerfen durch die Höhenprüfung. Andere Reittiere behalten ihre bisherigen Regeln.
- [ ] **BS10:** Außenfläche auf einem bisher gesperrten Gimmick/Podest testen; Innenräume, fehlender Platz und ungültige Spawnpositionen weiterhin abweisen. Keine doppelte Schwarzstern-Instanz.

Die Beschwörungsdauer beträgt 1.000 Minuten. Stadt-, Regions- und Höhenprüfungen sind für Schwarzstern freigegeben; andere Gründe für Wegfliegen oder eine Ablehnung durch Ablauf-/Platzierungsprüfungen sind weiterhin möglich. Die neue Version ist technisch geprüft, die Spielabnahme bleibt offen.
Details und technische Nachweise: [Schwarzstern](docs/BLACKSTAR.md).

Für den nächsten gezielten Diagnoseschritt reichen **BS01 und ein BS08-Versuch** an derselben Außenstelle. Die weiteren Fälle bleiben Teil der späteren vollständigen Abnahme.
## Item-Auswahl / Live-Spawner, 03.10.2026

- [x] Echter Katalog mit 6.816 Items, Icon, Gruppen, kombinierter Suche und exakter numerischer ID geprüft.
- [x] Grundwerte und Verfeinerungsstufen abrufbar; Benutzeroberfläche und Desktop-Build geprüft.
- [x] Acht Browser-Tests für Katalog/Auswahl bestanden; Core-Tests bestanden.
- [x] Lesende Probe und gemeinsamer Loader mit den vier bestehenden Mods geprüft; fremde EXE abgewiesen.
- [x] Probe hat einen Spieler-Inventarkontext auf Realm 1 beobachtet. Kein Schreibnachweis.
- [x] Neue Runtime: 13 Argumente/Originalergebnis, konkurrierende Warteschlange, UUID-Schutz, falsche Spielinstanz, Abbruch, Verfall und Fehler-Sperre geprüft.
- [x] Zwölf Tests mit tatsächlichem Build-2976-Konstruktor/Konverter/Destruktor im eigenen Prozess; Menge 1/1.000 und 0–5 Sockel. Definitionen/Allocator sind Testdaten.
- [x] Echte Windows-Pipe Rust/C++ im eigenen Testprozess geprüft; keine Spielverbindung.
- [x] Elf Browser-Tests, darunter Mengenprüfung, einmaliges Senden, Abbruch und wiederhergestellte Anfrage-ID bei verlorener Antwort.
- [x] Neue ASI und Desktop-Version bei geschlossenem Spiel installiert und zurückgelesen; Desktop-Verknüpfung aktualisiert. 33 bestehende Dateien unverändert, 19 Dateien frisch gesichert.
- [x] Automatische Ausgabe: tatsächlicher nativer Taskdispatcher/Wrapper mit MinHook im eigenen Prozess, Referenzlebensdauer, Originalaufruf, verschachtelte Aufgaben und Doppelvergabe-Schutz geprüft. Native Registry-/Inventarabhängigkeiten sind Testdaten.
- [x] Protokoll 2, fehlender/falscher Kontext, Ladeabbruch, Charakterkennung und Pause/Alt-Tab geprüft; Produktionsbuild und sechs ASIs im gemeinsamen Loader bestanden.
- [x] Automatische ASI und Desktop-Version bei geschlossenem Spiel/Workbench ersetzt und zurückgelesen; 36 bestehende Dateien einschließlich 13 Saves unverändert, 23 Backups erstellt.
- [ ] Spielstand laden und kurz normal spielen: Verbindung wird automatisch bereit. Kein Aufheben erforderlich. Neue `ARMED`- und `CONTEXT`-Zeilen in `CrimsonLiveItems.log` bestätigen den Einstieg.
- [ ] Erst **ein Eisenerz** per Button geben, **ohne etwas aufzuheben**. Falls es wartet, ins normale Spiel zurückkehren. Menge sehen, normal speichern, Spiel beenden und denselben Spielstand neu laden. Menge muss erhalten bleiben.
- [ ] Danach Stapel/volles Inventar/Pause/Alt-Tab/Charakterwechsel prüfen, wartende Anfrage abbrechen und bei pausiertem Spiel 30 Sekunden verfallen lassen.
- [ ] Während einer wartenden Anfrage den Charakter wechseln oder einen anderen Save laden: Auftrag wird abgewiesen, keine Vergabe an eine andere Figur. Nach fehlendem Kontext muss die Verbindung erneut bereit werden.
- [ ] Höhere Mengen und weitere Itemarten erst nach diesem Nachweis prüfen.

Die neue Anbindung ist installiert und wird beim nächsten normalen Spielstart geladen; der echte Inventar-/Speichern-/Neuladen-Test steht noch aus. Details: [LIVE-ITEMS.md](docs/LIVE-ITEMS.md).

## Erweiterung Tasche/Pfeile/Reittiere/Wissen/Menge, 03.10.2026

Technische Prüfungen bestanden; neue Version am 04.10.2026 bei geschlossenem
Spiel/Workbench installiert und zurückgelesen. Spielabnahmen bleiben offen:

- [ ] **LI01:** Extragroße Tasche 6003, Menge 1 geben. Kein alter allgemeiner Fehler; Tasche sichtbar, speichern und denselben Slot neu laden.
- [ ] **LI02:** Blitzpfeil 1001315, Menge 100 geben. Mengenänderung bestätigen, speichern/neuladen. Bei unklarem oder teilweisem Ergebnis keine neue Anfrage auf Verdacht.
- [ ] **LI03:** Mengenfeld leeren, 100 tippen, markieren/ersetzen und einfügen. Bei tatsächlichem Stapellimit 10 und genügend Platz: insgesamt +100 Stück in maximal zehn neuen Stapeln; vorhandene können aufgefüllt werden. Moddefinition kann vom Katalog abweichen.
- [ ] **LI04:** Gelesene und ungelesene Schrift mit Wissensbelohnung vergleichen. Richtigen Slot wählen; nach Lesen/Speichern „Wissen neu einlesen“. Status wechselt; Dokument ohne eindeutige Zuordnung zeigt unbekannt.
- [ ] **MT01:** Bei geschlossenem Spiel einen geeigneten gewöhnlichen Reittiertyp registrieren. Richtigen Slot laden, Stall und neues Tier prüfen; vorhandene Tiere, Ausrüstung, Fähigkeiten und Quests unverändert.
- [ ] **MT02:** Neues Tier reiten, speichern und neu laden. Gleiche Tierart danach als vorhanden markiert und nicht doppelt vergeben.
- [ ] **MT03:** Laufendes Spiel oder fehlende Vorlage sperren Registrierung. Bei verlorener Antwort dieselbe Anfrage prüfen; keine Doppelvergabe durch Wiederholung.

Sicherung und Grenzen: [Reittiere](docs/REITTIERE.md). Die Installation allein
fügt keine Tiere oder Items hinzu.
