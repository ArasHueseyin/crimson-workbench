# Phase 5 – Prüfung aller Restpunkte

Stand: 23.09.2026, v0.5.9. Grundlage ist die überarbeitete SPEC, B4–B11.
Steam 25455892 / EXE 1.0.0.2949 ist jetzt für Datenlesen und Tabellenplanung
unterstützt. 57 von 61 verwendeten Dateien sind bytegleich; nur Stage-/Questdaten
samt Headern ändern sich. Parser und Erhalt ungewählter Datensätze sind geprüft.
Native Reparaturmethoden müssen getrennt neu belegt werden.
[Buildvergleich](BUILD_SUPPORT.md), [Testcheckliste](../TESTCHECKLISTE.md).

**Phase 5 ist nicht vollständig abgeschlossen.** Die folgende Prüfung trennt
implementierte Funktionen, technische Lücken und die vom Nutzer verschobene
Spielabnahme. Reparatur-Laufzeitentwicklung und eine eigene Reparaturaktion sind
freigegeben. Aktion, Leser, Schreiber und gemeinsamer Transaktionsadapter sind
entwickelt; vollständige Inventarereignisse, echte Engine-Anbindung und Installation
fehlen. Phase 6 und der optionale
Farmmodus-Hotkey wurden nicht begonnen.

| Punkt | Ergebnis der Restprüfung | Verbleibender Nachweis |
|---|---|---|
| B4 Spawnzahlen und Einzelausnahmen | Terrain-/Pool-Gruppen und Limits sind implementiert, inklusive Multiplikatoren und Einzelfeldern. | Sichtbare Wirkung, aktive Populationen und Questabhängigkeiten im Spiel. |
| B4 Respawn-Zeiten | Zwei Stage-Patrouillen und 108 Fraktions-Wiederbesetzungsregeln bearbeitbar. Ein allgemeiner NPC-Timer fehlt weiterhin. | Tabellenquelle und Einheit eines allgemeinen Timers. `_spawnPercent` ist Wahrscheinlichkeit; `_respawnTimeSecond` gehört zu gespeichertem NPC-Zustand; `_spawnIntervalTime` gehört zu Timeline-Beschwörungen. Diese Felder sind kein Ersatz. |
| B4 Städte, Reitdauer und Cooldown | MainField-Freigabe, Stadt-Laufregel, Stadtflug-Bedingung sowie Riding-Charakterwerte implementiert. | Spielabnahme der einzelnen Reittiere. |
| B5 Blackstar | Cooldown, freie Dauer und Presets sowie Regions-/Stadtflug-Regeln implementiert. | Regionswechsel im Spiel; bereits aktive Cooldowns im Save werden nicht migriert. Quest-/Zwischensequenzabstiege sind eigene Regeln. |
| B6 Inventar und Lager | Jetzt neun Bereiche: Character, CampWareHouse, WareHouse, Kuku und fünf Housing-Lager. Start-/Maximalplätze frei eingegeben, mit Verhältnisprüfung. | 1.460 ist der belegte Standarddeckel eines Inventarpfads. Andere Engine-/Save-Grenzen und Konfigurationsabweichungen sind nicht vollständig bewiesen. Spezialcontainer bleiben geschützt. |
| B7 Stapel | Globale Größe, Kategorieauswahl, Einzelwerte und Experimentalschalter für zustandsabhängige Items implementiert. | Universelles Engine-Maximum unbekannt. 1.000.000 bleibt ausdrücklich eine Workbench-Grenze. `applyMaxStackCap` begrenzt in einem belegten Pfad die Gesamtmenge; es bleibt unverändert. Spieltests für gewöhnliche und experimentelle Items getrennt durchführen. |
| B8 Haltbarkeitsverlust | Ergänzt: Alle 122 Items mit endlicher Haltbarkeit erhalten den im Engine-Updater bestätigten Sentinel 65.535. EquipType-Verschleißfaktoren werden zusätzlich auf 0 gesetzt. | Spielabnahme, insbesondere Spezialausrüstung und gesockelte Items. Bereits verbrauchte oder zerstörte Exemplare werden nicht wiederhergestellt. |
| B8 Reparaturkosten 0 | App weiterhin gesperrt. 0.23.0 verbindet Batchprüfung, native Nachherkopien, Feldschreiber und Meldungsadapter. Ausrüstungsweg sowie Inventar-Paketserializer, Streamauswahl und Client-Ack isoliert geprüft. Native Item-/Speicherobjekt-Konverter erhalten reparierte Haupt-/Sockelwerte und Färbedaten; No-Wear-Normalisierung geprüft. Speicherdispatcher, Steam-Zulassung, Queue-Marker und Dateischreibhelfer sowie Pufferaufbereitung/LZ4 auf privaten Daten geprüft; nachgelagerte Verarbeitung und Dateiempfänger bleiben künstlich. | Vollständige serverseitige Inventarroute, erfolgreicher Pool-Transport, reguläre Speicherung/Abschlussbestätigung und übrige Zusatzdaten, echte Effektverarbeitung/Transport/UI und Auftragsrückmeldungen. Gültiger Spieler-/Manager-/Engine-Thread-Host, Eingabe, Loader und B0-Installation fehlen; kein installierbarer Mod. |
| B9 Ausdauer/Spirit | Sieben Kategorien, einschließlich 0: Skill-/Fahrerressourcen, 31 Eigenverbrauchswerte zweier Skills und 36 Zusatzkosten in 33 Ausrüstungs-Buffs. Positive Regeneration bleibt erhalten; Einzelausnahmen sind möglich. | Kategorien aus internen Namen abgeleitet. Kosten außerhalb der gelesenen Ressourcenlisten sind nicht als abgedeckt nachgewiesen; keine universelle Unlimited-Zusage. |
| B10 Skills | Alle 2.069 Skills, vollständige Feld-/Rohdateninspektion, globale/individuelle Cooldowns und bestätigte numerische Werte bearbeitbar. | Unbekannte Bedeutungen/Einheiten bleiben gemäß SPEC roh. Referenzen und Strukturzähler sind keine frei editierbaren Zahlen. |
| B11 God-Items | Stats, kompatible Buffs, Kopieren/Bearbeiten zusätzlicher Enchant-Zeilen und wiederverwendbare Vorlagen implementiert. | Spielabnahme. Tabellenstufen sind kein freigeschalteter Upgradeweg und keine Änderung bestehender Save-Instanzen. |

## Ergänzung v0.5.8

Alle 292 BuffInfo-Datensätze vollständig strukturell gelesen und bytegleich
rekonstruiert. 36 Zusatzkosten aus 33 Ausrüstungs-Buffs erhalten getrennte
Regler und Einzelausnahmen. 31 weitere Eigenverbrauchswerte aus zwei Skills
werden über „Weitere Skills: Geist“ erfasst. Vollständige Tabellenfingerprints
und B0-Apply/Restore gelten auch für die hinzugefügte Tabelle.

Der Reparatur-Mengensonderwert -1 bietet keinen Tabellen-Ausweg: Die normalen
Aufrufer prüfen vorher Menge > 0. Der untersuchte NPC-SpawnableTime-Check
prüft Tageszeitfenster, keinen Respawn-Abstand. Die separate Laufzeitentwicklung
für Reparatur wurde inzwischen freigegeben. [Prototyp 0.1.0](../runtime/repair/README.md)
ist entwickelt: 79 native Aufrufe und drei Scanner-Regressionen bestanden.
Die Kostenberechnung allein löst die vorgelagerte Reparatursperre nicht.
Der Nutzer hat inzwischen ausdrücklich die eigene Reparaturaktion freigegeben.
[Entwicklungsmodul 0.2.0](../runtime/repair/ACTION_INTEGRATION.md) ergänzt deren
Planung, sicheren Batch für eigene Abbilder und Befehlswarteschlange. 1.377
Aktionsprüfungen und 83 native Aufrufe bestanden; die Anbindung ans Spiel fehlt.

[Entwicklungsmodul 0.3.0](../runtime/repair/READER_LAYOUT.md) ergänzt den lesenden
Client-/Server-Adapter mit UID-/Positionsprüfung und Kontrolllesen. 124
Leserprüfungen, drei CTest-Suiten und insgesamt 116 native Aufrufe bestanden.
Die originalen Inventargetter und der neue Leser stimmen auf gemeinsamen
Testdaten überein. Registry-Aufrufe sind mit künstlichen Engine-Locks geprüft;
die echten Zugriffsfreigaben und der vollständige Schreib-/Ereignispfad fehlen.

[Entwicklungsmodul 0.4.0](../runtime/repair/ACK_INTEGRATION.md) ergänzt den
asynchronen Auftragsabschluss mit Sitzungs-/Nachherwertprüfung, Zeitlimit und
dauerhafter Sperre bei Teilfehlern. Der originale Clientpfad kann vor einem
Listenfehler bereits Sockel entfernen; sechs neue native Fälle sichern diese
Semantik ab. 1.481 Aktionsbedingungen und insgesamt 122 native Aufrufe bestanden.
Echte Spielereignisse sind weiterhin nicht angebunden.

[Entwicklungsmodul 0.5.0](../runtime/repair/LOCK_INTEGRATION.md) verbindet den
Leser mit einer nicht wartenden Besitzer-Sperrgruppe. Originale Lock-Methoden
sind mit echten Windows-SRW-Sperren im privaten Testhost geprüft. Vier CTest-
Suiten, 163 Leserbedingungen, 25 Gruppenbedingungen und insgesamt 140 native
Aufrufe bestanden. Die getrennte Lebensdauerverwaltung der Spielobjekte sowie
der vollständige Änderungs-/Ereignispfad bleiben offen.

[Entwicklungsmodul 0.6.0](../runtime/repair/REFERENCE_INTEGRATION.md) besitzt die
nativen Referenzbelege während der Erfassung und gibt sie nach den Sperren
zurück. Abgemeldete und in Zerstörung befindliche Besitzer werden abgewiesen.
249 Leser-/Referenzbedingungen, vier CTest-Suiten und 203 native Aufrufe bestanden.
Geschützte Live-Erwerbsquellen, konkrete Engine-Thread-/TLS-Anbindung und der
vollständige Änderungs-/Ereignispfad fehlen weiterhin.

[Entwicklungsmodul 0.7.0](../runtime/repair/REGISTRY_INTEGRATION.md) ergänzt den
Manager-Quellenadapter und kontextgebundene Freigabe. 55 neue synthetische
Bedingungen und fünf CTest-Suiten bestanden. Die neue EXE wird vom bisherigen
nativen Host abgelehnt; kein erneuter nativer Erfolg behauptet. Buildfreigabe,
konkrete Manager-/Spielerbindung und vollständiger Reparaturweg fehlen weiterhin.

[Entwicklungsmodul 0.8.0](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md) prüft
die geschützte Client-/Server-Suche mit konkreten Referenzmethoden, tatsächlicher
Actor-/PaPtr-Freigabe und echten Windows-Registry-Sperren gemeinsam auf 1.0.0.2949.
30 Szenarien, 263 Aufrufe, 3.490 Bedingungen und sechs CTest-Suiten bestanden.
Managerlebensdauer, aktueller Spieler/Engine-Thread und Reparaturtransaktion
bleiben offen; die bereitgestellten privaten Manager sind noch kein Live-Host.

[Entwicklungsmodul 0.9.0](../runtime/repair/PINNED_CAPTURE_INTEGRATION.md) beseitigt
die ungeschützte zweite Registry-Suche im geschützten Leser. Gültige Kennung,
lebendige Besitzer und aktuelle Clientauswahl werden geprüft. Sechs CTest-Suiten,
362 Leserbedingungen sowie 48 native Szenarien mit 503 Aufrufen und 3.867
Bedingungen bestanden. Neue native Erwerbs-/Sperrmethoden sind mit privaten
Legacy-Inventarfixtures verbunden; das neue Inventarlayout ist damit noch nicht
zugelassen. Die vollständige Reparaturanbindung bleibt offen.

[Entwicklungsmodul 0.10.0](../runtime/repair/INVENTORY_2949.md) ergänzt die eigenen
2949-Nachweise für Inventarslots, Holder/Possessor, aktuelle Auswahl, Ausrüstung
und Sockelfelder. Leser verlangt expliziten bekannten Build. Sechs CTest-Suiten,
385 Leserbedingungen und 88 native Szenarien mit 578 Aufrufen sowie 6.219
Bedingungen bestanden. Der eigene Leser stimmt mit aktuellen nativen Gettern
überein. Delta-0-Ausrüstungsprüfung beweist keine Änderungsereignisse; vollständiger
Commit und tatsächliche Spielhostbindung bleiben offen.

[Entwicklungsmodul 0.11.0](../runtime/repair/EQUIPMENT_EVENTS_2949.md) ergänzt
Ereignismetadaten mit aktivem No-Wear und vorherigem Broken-Zustand. Originaler
Server-Notifier und Client-Ack sind auf privaten reparierten Items gemeinsam
geprüft, einschließlich Scheinerfolgen und Teiländerungen. Sechs CTest-Suiten,
1.537 Aktionsbedingungen sowie 103 native Szenarien mit 758 Aufrufen und 9.311
Bedingungen bestanden. Tatsächliche Effektverarbeitung, Transport und UI bleiben
Testabhängigkeiten; der vollständige Engine-Commit und Spielhost fehlen weiterhin.

[Entwicklungsmodul 0.12.0](../runtime/repair/ITEM_LIFECYCLE_2949.md) ergänzt den
Besitzer nativer Itemkopien. Originale Konstruktion, tiefe Zuweisung und Destruktion
mit allen verwendeten Listenhelfern auf eigenen Quellen/Allokationen geprüft;
die Ereignisprobe verwendet jetzt diese Kopien. Sieben CTest-Suiten, 30 neue
Besitzerbedingungen sowie 117 native Szenarien mit 851 Aufrufen und 13.639
Bedingungen bestanden. Vollständiger Schreibweg, Slot-Aktualisierung und Host
bleiben offen; der private Heap ersetzt weiterhin den Engine-Allocator im Test.

[Entwicklungsmodul 0.13.0](../runtime/repair/DIRTY_SLOTS_2949.md) hält Besitzer
und Sperren für Erfassung/Refresh/Vorprüfung. Originale Slot-Markierung einschließlich
Kollisionen und Wachstum mit der Ereignisprobe verbunden. Sieben CTest-Suiten,
519 Leserbedingungen und 133 native Szenarien mit 1.296 Aufrufen bestanden.
Der spätere Slot-Verbraucher, vollständige Schreiber, Inventarereignisse und
Spielhost sind weiterhin offen. Kein Erfolg im laufenden Spiel behauptet.

[Entwicklungsmodul 0.14.0](../runtime/repair/FIELD_WRITER_2949.md) ergänzt den
Feldschreiber für beide Bereiche/Zustandskopien. Acht CTest-Suiten, 34 neue
Schreibszenarien und insgesamt 141 native Szenarien bestehen. Vorher-/Nachherprüfung
erfasst auch leere Slots und Metadaten; Teilfehler bleiben unklar und werden nicht
automatisch wiederholt. Die Verbindung mit vorbereiteten nativen Kopien und allen
Ereignissen zum Engine-Commit sowie der Spielhost sind weiterhin offen.

[Entwicklungsmodul 0.15.0](../runtime/repair/TRANSACTION_2949.md) verbindet
vorbereitete Nachherkopien, Feldschritt, Markierung und Meldungsadapter. Neun
CTest-Suiten, 40 Ablaufszenarien und 159 native Szenarien bestehen. Die 15 nativen
Ausrüstungsfälle verwenden diesen gemeinsamen Adapter und neue Erfassungen zur
Bestätigung. Sockelanzahl/logische Grenze und Zusatzvektor-Strides korrigiert.
Konkrete Inventarereignisse, vollständige Empfänger, Slot-Verbraucher und Host
bleiben offen; eine Meldungsübermittlung allein gilt weiterhin nicht als Erfolg.

[Entwicklungsmodul 0.16.0](../runtime/repair/PERSISTENCE_2949.md) ergänzt den
einmaligen Batchabschluss nach allen Meldungen. Originaler Slot-Verbraucher,
wiederverwendbares Clear und endgültige Tabellenfreigabe sind auf privaten
Komponenten integriert. Neun CTest-Suiten, 43 Ablaufszenarien und 186 native
Szenarien bestehen, darunter 24 Verbraucherfälle und 18 Ausrüstungsfälle.
Die Liste wird bereits vor der Persistenzantwort geleert; Fehler dürfen deshalb
weder Erfolg noch eine automatische Wiederholung auslösen. Der Empfänger bleibt
künstlich. Der belegte Verbraucher überträgt keine Sockeldatensätze; ihr ergänzender
Speicherweg, Inventarereignisse, tatsächliche Rückmeldungen und Spielhost fehlen.

[Entwicklungsmodul 0.17.0](../runtime/repair/INVENTORY_EVENTS_2949.md) verbindet
den originalen Inventar-Client-Ack mit privaten Quellen und dem gemeinsamen Ablauf.
36 neue native Fälle; neun CTest-Suiten und insgesamt 222 native Szenarien bestehen.
Die Bestätigung verweigert UID-Wechsel, fehlende/doppelte Meldungen und
Abschlussfehler. Der Ack prüft selbst keine UID und verändert keine Sockel.
Inventar-Server, tatsächliche Persistenz-/Ladesemantik und Spielhost bleiben offen.
Der gefundene Socket-SQL-Helper überträgt die Itemkennung und ist deshalb noch
kein belegter Haltbarkeits-Speicherweg.

[Entwicklungsmodul 0.18.0](../runtime/repair/SQL_DISPATCH_2949.md) prüft den
vollständigen SQL-Ausführungs-Shim und Request-Erwerb. 19 neue native Fälle,
insgesamt 241 native Szenarien und neun CTest-Suiten bestanden. Der Shim führt
selbst keinen Datenbankauftrag aus und kann Erfolg ohne Request-Zugriff liefern.
Die nachgelagerte Verarbeitung und tatsächliche Persistenz sind weiter offen;
der laufende Spielmodus wurde dafür nicht abgefragt.
Der getrennte ItemSaveData-Konverter samt Gegenweg übernimmt Hauptwert und
Sockelhaltbarkeit. Der damalige statische Nachweis ist in 0.19.0 nativ erweitert.

[Entwicklungsmodul 0.19.0](../runtime/repair/ITEM_SAVE_2949.md) führt die Original-
Umwandlungen auf privaten Objekten aus. 38 neue Szenarien, davon 14 mit tatsächlicher
Reparaturplanung und nativer Nachherkopie. Haupt-/Sockelwerte, No-Wear-Normalisierung,
Färbedaten, Reset/Wiederverwendung und ausgeglichene Allokationen geprüft. Insgesamt
279 native Szenarien und neun CTest-Suiten bestanden. Noch offen sind der reguläre
Speichervorgang/Abschluss, weitere optionale Daten und tatsächliche Sockelrichtlinien.
Die fünf vom Lader angelegten Datensätze erteilen keine allgemeine Zulassung für
Reparaturgeometrien außerhalb der bisherigen Vorprüfung.

## Konkrete Änderungen der vorherigen Restprüfung

- Haltbarkeit ergänzt, mit bytegenauer Prüfung sämtlicher 6.816 Items:
  ausschließlich `max_endurance` wird geändert; 0 und vorhandene Sentinels
  bleiben unverändert. Kombination mit anderen Itemänderungen und erneute
  Anwendung sind geprüft. Die Einstellungs-/Vorlagenformate bleiben kompatibel.
- Kuku und fünf Hauslager ergänzt. Zwanzig Inventarpräfixe werden strukturell
  gelesen, einschließlich verschachtelter Item-Move-Listen. Der variable
  Character-Bedingungsbaum bleibt hashgebunden und unverändert erhalten.
  Alle 21 Datensätze bestehen die Prüfung; nur neun erhalten editierbare Slots.
- Vorschau, Rücksetzen, Export und Apply/Restore an der Projektkopie erfassen
  die neuen Werte gemeinsam mit den übrigen Modulen.

## Warum noch keine vollständige Freigabe möglich ist

Allgemeine NPC-Respawns, kostenlose Reparatur und universelle Größenlimits
brauchen zusätzliche Verhaltens-/Formatnachweise. Die vorhandenen Belege
erlauben dafür keine korrekten weiteren Tabellenpatches. Die bloße Bezeichnung
einer Option in einem älteren Community-Mod belegt das aktuelle Dateiformat
nicht. Im Ordner `reference/mods/` liegt weiterhin kein Vergleichsmod.

Die Nutzerentscheidung verschiebt manuelle Spieltests bis nach der Entwicklung
aller Phasen. Diese Tests sind offen, nicht fehlgeschlagen oder bestanden.
Save-Änderungen bleiben ausgeschlossen. Die Runtimeentwicklung für Reparatur ist
freigegeben; auch die eigene Aktion hat noch keinen Einbau- oder Aktivierungspfad.
Ein pauschales „Phase 5 fertig“ würde daher Funktionen behaupten, die fehlen.

Automatisierte Ergebnisse: [Prüfbericht](TESTING.md).
Technische Adressen und Typen: [Feldnachweise](research/ADVANCED_TABLES.md).
Bedienung und Grenzen: [B4–B11](ADVANCED_MODS.md).


[Entwicklungsmodul 0.20.0](../runtime/repair/SAVE_BACKEND_2949.md) ergänzt
39 native Fälle für Speicherdispatcher, Steam-Zulassung und vorgemerkte Einträge.
Insgesamt 318 native Szenarien und neun CTest-Suiten bestanden. Datei-/Plattform-
Empfänger bleiben eigene Testcallbacks. Die konkreten Dateimethoden und Timer
sind statisch zugeordnet; der originale Schreibhelfer ignoriert das Flush-Ergebnis.
Seine native Pufferverarbeitung, echte Abschlussbestätigung und vollständige
Spielanbindung fehlen weiterhin. Keine Änderung an App, Installation oder Saves.


[Entwicklungsmodul 0.21.0](../runtime/repair/SAVE_FILE_2949.md) ergänzt
34 native Fälle für den Dateischreibhelfer und Pufferbereinigung. Insgesamt 352
native Szenarien und neun CTest-Suiten bestanden. Flush-/Closefehler verhindern
im Originalcode weder Erfolg noch Payload-Freigabe; diese Rückgabe bestätigt
keine dauerhafte Speicherung. Echter Encoder, vollständiger Queue-/Backupweg,
Abschlussbeleg und Spielhost bleiben offen. Die Desktop-App bleibt unverändert.


[Entwicklungsmodul 0.22.0](../runtime/repair/SAVE_ENCODING_2949.md) ergänzt
66 native Fälle für Aufbereitung, Allokation und Kompression. Insgesamt 418 native
Szenarien und neun CTest-Suiten bestanden. Ein erneuter Aufruf desselben Eintrags
nach Öffnungsfehler kann doppelt komprimieren; neue Versuche benötigen frische
Rohdaten. Integrität/Verschlüsselung, vollständige Speicher-/Ereigniskette und
Spielhost bleiben offen. App v0.5.9, Installation und Saves unverändert.
