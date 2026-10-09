# Phase 5 – Welt, Ausrüstung und Skills

Stand: 21.09.2026, v0.5.8 Vorschau. **Phase 5 ist teilweise umgesetzt.**
Die unten ausdrücklich offenen Spezifikationspunkte sind keine bestandenen
Features. Die manuelle Spielabnahme erfolgt auf Nutzerwunsch erst nach allen
Entwicklungsphasen. An der laufenden Installation wurde nichts angewendet.

## Bedienung

In der **Modwerkstatt** stehen unter den bisherigen Modulen acht Bereiche
B4–B11. Alle Einstellungen sind zunächst Entwürfe. **Vorschau berechnen**
zeigt die einzelnen Werte und die geplanten Dateien. **Vorschau exportieren**
und **Probe an Projektkopie** benutzen exakt diesen Plan. Nach Änderungen
muss er neu berechnet werden. Die separate Live-Anwendung behält sämtliche
B0-Prüfungen, insbesondere das beendete Spiel und die geprüfte Ausgangsbasis.
Auch eine sichtbar ungültige Zahl sperrt Export/Probe und verwirft eine bereits
vorbereitete Live-Dateifreigabe, selbst wenn intern noch der letzte gültige
Zahlenwert gespeichert ist. Nach Korrektur braucht Live-Apply eine neue Dateiprüfung.

| Modul | Implementiert | Grenze / offener Teil der SPEC |
|---|---|---|
| B4 Welt | Charakterzahlen und gesetzte Limits in Terrain-/Pool-Spawn-Gruppen; Prozentfaktor und Einzelfelder; Reset-Intervalle zweier Spawn-Patrouillen und Wartezeiten von 108 Gebietswiederbesetzungsregeln; MainField-Reittierfreigabe, Stadt-Laufbeschränkung und Stadtflug-Regel; Dauer und Cooldown der `Riding_*`-Charaktere | Keine allgemeinen NPC-Respawn-Zeitgeber, keine vollständige Skript-/Questspawn-Abdeckung. Spielwirkung ungeprüft. |
| B5 Drache | Blackstar-Cooldown 0; Dauer 10/30/60/120 Minuten oder freie Sekunden; Entfernung der Mercenary-Sperre 79 aus Regionslisten und gezielte Deaktivierung der Stadtflug-Abstiegsbedingung | Stadtflug-Regel gilt auch für andere Flugreittiere. Quest-/Zwischensequenz-Abstiege und maximale Flughöhe bleiben unverändert. Spielabnahme offen; 0 Sekunden wird nicht als unbegrenzt erfunden. |
| B6 Inventar | Character, CampWareHouse, WareHouse, Kuku sowie Housing_Dresser, Housing_Refrigerator, Housing_Symbol, Housing_Collecting und Housing_GatheredMaterials; Startplätze und Maximum als freie Ganzzahlen | Workbench erlaubt 1..1.460, abgeleitet vom Standarddeckel im geprüften Inventar-Codepfad; kein Nachweis aller Engine-/Save-Grenzen. Bestehende Save-Slotzahlen können vom Datenstandard abweichen. |
| B7 Stacks | Globale Größe für bereits stapelbare Items; zusätzliche Kategorie-IDs; einzelne Items; ausdrücklicher experimenteller Schalter | 1.000.000 ist eine Workbench-Prüfgrenze. Universelles Engine-Cap unbekannt. `apply_max_stack_cap` kann zusätzlich die Gesamtmenge im Inventar begrenzen und bleibt erhalten. |
| B8 Haltbarkeit | EquipType-Verschleißfaktor 0 plus Engine-Sentinel 65.535 für alle 122 Items mit endlicher Haltbarkeit | Aktuelle 6.816 Items enthalten **keine** `repair_data_list`-Einträge. Kostenfreie Reparatur ist gesperrt, auch bei vorhandenen Regeln. Der gefundene Kostenpfad erlaubt kein sicheres Nullsetzen; ein geeigneter Tabellenpfad fehlt. Geskriptete Haltbarkeitsverluste ungeprüft. |
| B9 Kosten | Skill-/Fahrerlisten, 31 Eigenverbrauchswerte in zwei Skills und 36 Zusatzkosten in 33 Ausrüstungs-Buffs; sieben Kategorien, Einzelausnahmen und Nullkosten-Schalter | Keine universelle Unlimited-Funktion. Skill-Kategorien aus Namen abgeleitet; Ausrüstungs-Buffs separat. Positive Regeneration, gegnerische Drain-Effekte und andere Statusarten unverändert. |
| B10 Skills | Alle 2.069 Skills suchbar; bekannte Zahlen bearbeitbar; globale/individuelle Cooldowns; vollständige Buffmatrix-Inspektion, 87.697 benannte Basis-/Ressourcenfelder und individuelle Bearbeitung bestätigter i64-Werte einschließlich der drei Graphwerte | Unbekannte Bedeutungen/Einheiten bleiben als roh markiert. Referenzen, Typen, Counts, Graphkurven sowie die neu aufgeteilten Summon-/AddSubLevel-Payloads bleiben schreibgeschützt. |
| B11 Items | Stats und kompatible Buff-Paare bearbeiten; neue Enchant-Zeilen aus einer Originalstufe kopieren und bearbeiten; einzelne Stacks; wiederverwendbare Vorlagen | Keine beliebigen unbestätigten Buff-IDs/-Levels. Eine neue Tabellenstufe schaltet keinen Upgradeweg frei und erhöht keine vorhandene Iteminstanz. |

[Vollständige Restpunktprüfung](PHASE5_REMAINING.md).

Diese offenen Punkte benötigen zusätzliche Format-/Verhaltensnachweise. Es gibt
keine versteckten Platzhalter-Patches, Runtime-Hooks oder Änderungen an Saves.
Phase 6 und die optionale Runtime-Phase wurden nicht begonnen.

## Regeln und Reihenfolge

- Prozentwerte: 100 = unverändert, 50 = halb, 300 = dreifach. Ganzzahldivision
  rundet Richtung 0. Faktoren gelten jedes Mal auf das Original, niemals auf
  eine bereits modifizierte Ausgabe. Überläufe werden abgelehnt.
- Spawn-Nullen und Maximal-Sentinelwerte 255/65.535 werden global bewahrt.
  Einzelfelder erlauben eine bewusste Ausnahme. Andere Grenzüberschreitungen
  führen zu einem Fehler statt zu stiller Sättigung.
- **Patrouillen-Reset in Prozent** erfasst ausschließlich Stage 1017811
  (Hidden Estate) und 1002224 (Watergate), beide mit direkter Spawnreferenz
  und ursprünglich `resetSecond = 259200`. 50 % ergibt 129600 Sekunden
  in den Tabelleneinheiten. Individuelle Sekundenwerte stehen im Welt-Browser;
  Workbenchgrenze 1..31.536.000. Keine Hochrechnung auf sämtliche NPCs oder
  Zusicherung derselben Dauer auf der realen Uhr.
- **Wiederbesetzung: Wartezeit in Prozent** skaliert ausschließlich `delayTime`
  der 108 `factionreblockadinginfo`-Regeln. Originalwerte: 24 × 86.400,
  84 × 432.000. 50 % halbiert diese Tabellenwerte. Individuelle Werte im
  Welt-Browser (Suche nach Gebietsname/ID), Grenze 1..31.536.000.
  Faktor 1..10.000 %, standardmäßig aus. Questbedingungen, Wahrscheinlichkeiten,
  andere Questzeitgeber und Gebietsverknüpfungen bleiben byteidentisch.
  Das ist keine allgemeine NPC-Respawn-Regel; Spielzeiteinheit, Fortschritts-
  abhängigkeiten und Wirkung werden erst bei der späteren Spielabnahme geprüft.
- Stadtfreigabe und Drachen-Regionsoption deaktivieren dieselbe Bedingung
  1011130 (`IsInTown() && !IsAboveRoad(Bird,20)`) genau einmal. Alle anderen
  10.797 Bedingungen einschließlich Stadt-, Kopfgeld- und Questprüfungen bleiben
  byteidentisch. Die gemeinsame Regel betrifft sämtliche Flugreittiere.
- Individueller Skillfaktor ersetzt den globalen Faktor. Blackstar-Optionen
  ersetzen globale Reittierwerte. **Explizite Einzelfelder haben zuletzt Vorrang**.
- Inventory-Startplätze müssen kleiner/gleich dem Maximum sein. Das Ändern
  von Datenstandards ersetzt keine Save-Migration. Große Zahlen und
  Verkleinerungen sind vor einer Freigabe im Spiel zu prüfen.
- Nicht bestätigte Felder, Kategorien, Skills, Items, Status-IDs, Buff-Paare
  und weder vorhandene noch explizit neu vorgemerkte Enchant-Zeilen führen zu einer Fehlermeldung.
- Als zustandsabhängig erkannte Items werden bei globalen Stacks ohne
  Experimentalschalter ausgelassen. Einzeländerungen auf eine Stapelgröße über 1 werden abgelehnt.
  Erfasst werden Equip-Typen, endliche Haltbarkeit, mehrere Enchant-Zeilen,
  Passivskills/Buffs, Sockeldaten, Ladungs-/Schärfemerkmale, Subitems und
  Siegel-/Behälterdaten. Dies beweist keine allgemeine Stapelverträglichkeit.
- Item-Stats sind vorzeichenbehaftete Rohzahlen. Grenze ±1.000.000.000;
  `per_level` muss ins tatsächliche i8-Format −128..127 passen. Keine erfundenen
  Prozent-/Schadenseinheiten. Buffs stammen aus beobachteten ID/Level-Paaren
  desselben Equip-Typs; auch dies ersetzt keinen Spieltest.

## Neue Enchant-Stufen und Skill-Details

Unter B11 ein Item auswählen, **Enchant-Stufe hinzufügen** öffnen und eine
Originalstufe sowie eine neue Zielnummer wählen. **Enchant-Stufe vormerken**
kopiert deren gesamte Zeile einschließlich Stats, Buffs und Preisen. Danach
können Stats/Buffs für die neue Stufe geändert werden. Nur Originalstufen
dürfen Kopierquelle sein; vorhandene/doppelte Ziele, fremde Trenndaten oder
unsortierte Originalstrukturen werden abgelehnt. Grenze: 64 neue und 256
gesamte Zeilen pro Item, Stufennummer 0..65.535 (Format-/Workbenchgrenzen).
Beim Entfernen einer vorgemerkten Stufe werden ihre vorgemerkten Stats/Buffs
ebenfalls entfernt. Vorlagen speichern diese Kopierregeln mit.

Neue Buffs oder zusätzliche Enchant-Stufen können ein vorher gewöhnliches
Item zustandsabhängig machen. Hat es danach Stapelgröße über 1, verlangt die
Vorschau den Experimentalschalter oder ausdrücklich Stapelgröße 1.

Unter B10 nach der Skill-Auswahl **Buffmatrix & vollständiger Datensatz**
öffnen. Alle 4.607 Einträge (einschließlich null) sind über Matrixzeile,
Spalte, Typnamen und Feldnamen auffindbar. Seit v0.5.4 sind die neun
Summon-Payloads in benannte Felder und der AddSubLevel-Payload in Referenz
und unbekanntes Vier-Byte-Feld aufgeteilt. Auch benannte Spawn-/Zeitfelder
bleiben hier schreibgeschützt; Namen bestätigen weder Einheiten noch
Editierregeln. Koordinaten/unklare Zahlenformate bleiben als Hexwerte sichtbar. Die Strukturpositionen `mem_*` sind keine
bestätigten Feldnamen; Bytepositionen beziehen sich auf den Dateidatensatz.
Die Ansicht zeigt die Originaldaten, nicht einen angewendeten Mod. Die
gesamten Originalbytes einschließlich noch unbekannter Suffixwerte stehen
als Hexansicht bereit. Bei bestätigten i64-Feldern **Rohwert überschreiben**
aktivieren und eine Ganzzahl zwischen −1.000.000.000 und 1.000.000.000 eingeben.
Jede Änderung ersetzt genau acht Bytes; alle anderen Matrixbytes bleiben
erhalten. Einheiten werden nicht aus den Zahlen geraten. Ausgewählte Rohwerte
und Cooldown-/Kostenänderungen werden im selben Plan zusammengeführt.
Ohne explizite Buffänderung bleibt die Matrix vollständig byteidentisch.

## Basisdaten, Voraussetzungen und Ressourcen eines Skills

Unter **Buffmatrix & vollständiger Datensatz** den Abschnitt **Basisdaten,
Voraussetzungen & Ressourcen** öffnen. Er zeigt alle Felder vor und nach der
Buffmatrix: unter anderem Gruppen-/Elternreferenzen, Upgrade-Graphen,
Charakter-/Bedingungslisten, Statusressourcen samt Regenerationsflag,
Itemressourcen, Batterie, UI-Flags und Entwicklertexte. Die Suche findet
Feldnamen, Kennungen und Originalwerte. Referenzen sind als solche markiert;
unbestätigte Zahlenformate wie `useItemCount` bleiben als Hexwerte sichtbar.

Dieser Bereich zeigt Originalwerte und ist vollständig schreibgeschützt.
Bereits freigegebene Zahlen weiterhin in den Skill-Feldern oberhalb bearbeiten;
dortige Änderungen bleiben beim Filtern erhalten. Ein Originalwert in der
Detailansicht ist deshalb nicht zwangsläufig der vorgemerkte Wert. Es werden
keine zusätzlichen Schreibrechte aus Feldnamen oder Referenzen abgeleitet.

Seit v0.5.6 bestimmt der Reader das Ende der Buffmatrix direkt aus ihrem
Format. `skillGroupKey` ist eine Gruppenreferenz und muss nicht mit dem
Datensatzschlüssel übereinstimmen. Die Prüfung rekonstruiert alle 2.069
vollständigen Skill-Datensätze einschließlich Header und Suffix byteidentisch.

## Vorlagen

Ein Item wählen, Werte vormerken, einen Namen vergeben und **Item-Vorlage
speichern** benutzen. Die Vorlage enthält nur die Item-Einstellungen, keine
Spieldateien. Sie bleibt im lokalen WebView-Speicher derselben App erhalten.
Mit **Vorlage auf Item übernehmen** wird sie auf das ausgewählte Item angewendet;
die Vorschau prüft dessen tatsächliche Eignung erneut. Speichern unter demselben
Namen ersetzt die Vorlage. Maximal 100 Vorlagen, Namen bis 80 Zeichen.
Die Vorlagen sind noch keine Phase-6-Profile. Das Löschen des App-Speichers
entfernt sie. Beschädigte oder formal ungültige Einträge werden nicht geladen.

## Technischer Nachweis

Parser, Aufbau und Grenzen: [Recherche](research/ADVANCED_TABLES.md).
Alle geänderten Tabellen gelangen in dasselbe B0-Overlay. Bei längeren
Item-/kürzeren Regionsdatensätzen werden Header-Offsets neu aufgebaut.
Die Primärprüfung erfolgt vor jeder semantischen Interpretation anhand der
bekannten Build-/Tabellenhashes. Zusätzliche kleine Tabellen sind separat in
`crates/cd-core/schemas/steam-25381195.advanced.json` gepinnt.

Automatische und manuelle Prüfungen: [TESTING.md](TESTING.md).
Roundtrip- und Projektproben belegen Dateikonsistenz und Rücknahme, nicht die
Gameplaywirkung.


## Nicht unterstützte Reparaturoptionen in alten Vorlagen

Seit v0.5.5 wird `free_repair: true` sowohl global als auch pro Item immer
abgelehnt. Die Materialkosten können im Spiel als Divisor verwendet werden;
Nullsetzen ist daher kein freigegebener Weg zu kostenloser Reparatur. Der
Schalter für Verschleiß bleibt davon unabhängig. Alte Vorlagen bleiben lesbar:
Eine darin aktivierte Reparaturoption lässt sich abwählen. Stats, Buffs,
Stapelgröße und Enchant-Kopien bleiben dabei erhalten. Anschließend die
Vorschau erneut berechnen und bei Bedarf die korrigierte Vorlage speichern.

## Zusätzliche Ausrüstungs- und Skillkosten

Unter B9 regelt **Ausrüstungs-Buffs** die zusätzlichen Ressourcenlisten aus
`buffinfo`: 100 Prozent erhält das Original, 50 halbiert, 0 entfernt die
bestätigten Eigenkosten. Der Browser **Buff-Eigenkosten** erlaubt die Suche
nach Buffname/ID und einzelne Ausnahmen. Die Kategorie wirkt auf die in jedem
Buff hinterlegten Skills; deren Referenzen werden nicht geändert.

**Weitere Skills: Geist** erfasst zusätzlich die Buffmatrix-Verbrauchswerte
der Skills 40013 und 10300. Einzelwerte in B10 → Buffmatrix haben Vorrang.
Die gemeinsamen Nullkosten-Schalter setzen alle sieben Kategorien der gewählten
Ressource; vorhandene Einzelausnahmen bleiben ausdrücklich erhalten.
Positive Ressourcenwerte und Regenerationsflags werden nicht genullt, ebenso
wenig fremde Angriffs-/Dot-Effekte oder Bufflaufzeiten. Die Funktion ist keine
Zusage unbegrenzter Ressourcen in jedem Engine- oder Skriptpfad.
