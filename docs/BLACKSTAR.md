# Schwarzstern – Umsetzung vom 01.10.2026

**Aktuelles Diagnose-/Beschwörungsupdate installiert, technische Prüfungen bestanden; uneingeschränkte Außenbeschwörung weiterhin unbestätigt.** Der Nutzer meldet nach dem Stadtflug-Update weiterhin ungefähr „Reittiere können hier nicht gerufen werden“. Die neue Version erkennt Schwarzstern zusätzlich während der früheren Beschwörungsaktion und erfasst den Fehlercode der Spielmeldung. Die gewünschte Beschwörungsdauer beträgt 1.000 Minuten. Installation und frühere Saveänderungen erfolgten bei geschlossenem Spiel.

## Verhalten

- **Praktisch unbegrenzte Ausdauer:** Nur Schwarzsterns Ausdauerregeneration wurde stark erhöht. Dies verhindert nicht zwingend jede sichtbare kurze Abnahme des Balkens; anhaltender Flug, Beschleunigung und Feueratem müssen im Spiel geprüft werden.
- **Zusätzliche Beschwörungsorte draußen:** Schwarzstern wird anhand seiner Instanz und seines Charakterdatensatzes erkannt. Regions-, Navigations-, Voxel- und Plattform-Sperren werden im zugehörigen Aufruf umgangen. Die Erkennung umfasst nun auch die normale und die reservierte frühe Client-Beschwörungsaktion, über exakt belegte Charakter-/Instanzaufrufe. Dadurch gilt die Navigations-/Voxel-Freigabe auch bei späteren Aktionsprüfungen im selben Aufruf. Die abschließende Innenraum- und Platzierungsprüfung bleibt aktiv. Andere Reittiere behalten ihre Regeln. Ob diese Erweiterung die gemeldete Sperre behebt, muss im Spiel geprüft werden.
- **Stadtflug und gesperrte Gebiete:** Die separate Regel `IsInTown() && !IsAboveRoad(Bird,20)` sowie die Prüfung `IsVehicleAllowedInEnteredRegion` werden für einen auf Schwarzstern reitenden Charakter oder Schwarzstern selbst freigegeben. Damit sind die vom Nutzer beschriebenen roten Gebiete als Testfälle für Beschwörung und Grenzüberflug abgedeckt; Erfolg in jedem dieser Gebiete ist noch im Spiel zu prüfen.
- **Höhenbegrenzung:** Die gesonderte Prüfung `CheckVehicleAllowableHeight` wird für Schwarzstern freigegeben.
- **Cooldown eine Sekunde:** Schwarzsterns eigener Grundwert wurde von 3600 auf 1 gesetzt. Zusätzlich wurde die im neuesten Save bereits gespeicherte Restzeit von 3.596.726 auf 1.000 Millisekunden gekürzt.
- **Beschwörungsdauer 1.000 Minuten (16 Stunden 40 Minuten):** Der eigene Grundwert wurde von 600 auf 60.000 Sekunden erhöht. Die bereits aktive Beschwörung in Slot 1 hat ebenfalls 1.000 Minuten gespeicherte Restzeit erhalten.

Das automatische Wegfliegen nach dem Absteigen/Landen bleibt ein eigener Mechanismus. Ungültige Spawnpositionen, geschlossene Räume, noch nicht geladene Weltbereiche und gesonderte Ablauf-/Questzustände können weiterhin Grenzen setzen. Die Verlängerung der Beschwörungsdauer verhindert andere Gründe für ein vorzeitiges Ende nicht. „Jeder beliebige Außenpunkt“ ist bisher nicht im Spiel nachgewiesen.

## Installation und Rückbau

Zusätzliche Datei:

`C:\Program Files (x86)\Steam\steamapps\common\Crimson Desert\bin64\CrimsonBlackstar.asi`

SHA-256: `8e1b254874c27022f6746b01f0520b31ed3ee44447d2919dce15f090ed8a886f`, Größe 212.480 Bytes.

Der bereits vorhandene `winmm.dll`-Loader lädt sie beim nächsten normalen Spielstart. Die Erweiterung akzeptiert ausschließlich die geprüfte EXE `1.0.0.2976` mit SHA-256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`. Bei anderer Version oder abweichenden Zielbytes bleibt sie deaktiviert. Die EXE auf der Festplatte und alle bestehenden Modarchive wurden nicht verändert.

Das Laufzeitprotokoll entsteht neben der Erweiterung als `CrimsonBlackstar.log`:

- `READY: Blackstar early summon action support ...`: vierzehn Hooks und ein Bedingungsslot installiert, Charakterdaten noch abzuwarten. Ältere READY-Zeilen bleiben im bestehenden Log erhalten.
- `APPLIED: Blackstar stamina regeneration ...`: Schwarzsterns vollständig geladene Daten erkannt und geändert.
- `APPLIED: Blackstar summon region restriction bypass ...`: die Beschwörungsfreigabe wurde erstmals verwendet.
- `APPLIED: Blackstar town/regional flight/flight height restriction bypass`: die neue Flugfreigabe wurde erstmals für Schwarzstern verwendet.
- `APPLIED: Blackstar summon navigation/voxel/platform ...`: eine zusätzliche Beschwörungsfreigabe wurde verwendet.
- `REFUSED: Blackstar initial/spawn validator result ...`: eine verbliebene Zustands-/Platzierungsprüfung hat abgelehnt. Ort und sichtbare Spielmeldung zusammen mit diesem Ergebnis untersuchen.
- `REFUSED: Blackstar common/server validator ...`: die zusätzliche gemeinsame/serverseitige Prüfung hat abgelehnt; ihre Antwort wird nur protokolliert.
- `ATTEMPT: Blackstar ... frame event returned ...`: Ergebnis der früheren Beschwörungsaktion, höchstens 24 Zeilen pro Sitzung.
- `GAME_ERROR: code=... caller=... BlackstarScope=...; stack: ...`: Fehlercode und Aufrufweg der Spielmeldung, höchstens 64 unterschiedliche Kombinationen pro Sitzung. Auch andere Spielmeldungen können enthalten sein. Alle Spielmeldungen werden unverändert weitergereicht; eine Meldung wird niemals in Erfolg umgewandelt.
- `REFUSED` / `DISABLED`: unerwartete Daten oder Spielversion; die entsprechende Änderung wird nicht angewendet.

Zum Rückbau bei **geschlossenem Spiel** nur `CrimsonBlackstar.asi` aus `bin64` entfernen oder außerhalb des Spielordners ablegen. Bestehende Schlafmods und `winmm.dll` behalten. Bereits gespeicherte Timer bleiben dabei erhalten; neue Beschwörungen verwenden wieder die ursprünglichen Grundwerte. Ein komplettes altes Savebackup sollte nicht über später erspielten Fortschritt kopiert werden. Für die Rücknahme ausschließlich der Verlängerung liegt die vorherige ASI zusätzlich im unten genannten neuen Backup; ein noch aktiver langer Timer wäre getrennt und gezielt anzupassen.

## Save und Sicherung

Die beiden früheren Saveänderungen betrafen den damals neuesten Stand:

`%LOCALAPPDATA%\Pearl Abyss\CD\save\<Konto-ID>\slot1\save.save`

Erste Änderung: `MercenaryClanSaveData._callMercenaryCoolTimeSaveList`, Eintrag für Schwarzstern (`_mercenaryNo = 1003255`), `_coolTimeLeft`.

Neun Savedateien beziehungsweise Begleitdateien wurden vorab gesichert unter:

`<Projektordner>\.local\blackstar-mod-20261001\backups\20260930T231256Z`

HMAC und unabhängiger Decoder nach dem Schreiben erfolgreich. Der vollständige semantische Savevergleich zeigt genau die Änderung dieses Skalars. Nur die Bodybytes `1927537`, `1927538`, `1927539` unterscheiden sich. Alle anderen Bodybytes, Feldmasken, Reservierungsdaten und die Nonce stimmen. Fähigkeiten, Inventar, Questfortschritt, aktive Beschwörungsdauer, andere Slots und Lobbydateien wurden bei dieser ersten Änderung nicht verändert. Der Rücktausch des Skalars ergibt den Originalbody exakt.

**Anschließende Verlängerung auf 1.000 Minuten:** Das Feld `MercenaryClanSaveData._callMercenarySpawnDurationSaveList._spawnEndDurationLeft` desselben Schwarzstern-Eintrags (`_vehicleKey = 16984`) wurde von 596.726 auf 60.000.000 Millisekunden gesetzt. Der vollständige semantische Vergleich zeigt ausschließlich diese Änderung; im Body unterscheiden sich nur die vier Bytes `1927589` bis `1927592`. HMAC, unabhängiges Wiederöffnen, inverse Wiederherstellung des Originalbodys und Erhalt aller übrigen Felder wurden geprüft. Der Ein-Sekunden-Cooldown bleibt erhalten.

Frische Sicherung vor der Verlängerung, einschließlich der vorherigen ASI und aller neun Save-/Begleitdateien:

`<Projektordner>\.local\blackstar-mod-20261001\duration-1000m\backups\20260930T232129Z`

Prüf- und Installationsberichte der Verlängerung: `.local/blackstar-mod-20261001/duration-1000m/`. Der damals installierte Slot 1 hatte SHA-256 `068f7fe02c759261ddd9d2108ea375333c0443909c83952862d996c46a95bd93`. 17 geschützte Dateien wurden nach dieser Installation unverändert verifiziert.

**Stadt-/Gebietsflug-Update:** Hier wurden keine Savedateien geändert. Alle neun aktuellen Save-/Begleitdateien, die vorherige ASI und das bisherige Laufzeitprotokoll wurden frisch gesichert unter:

`<Projektordner>\.local\blackstar-mod-20261001\city-flight\backups\20260930T234228Z`

19 geschützte Dateien wurden nach Austausch ausschließlich der ASI unverändert verifiziert. Der neueste Save bei dieser Installation war **Slot 2**. Prüf- und Installationsberichte: `.local/blackstar-mod-20261001/city-flight/`.

**Frühe Beschwörungsaktion und Diagnose, 01.10.2026:** Das bisherige Laufzeitlog belegt das Laden des Stadtflug-Updates und die ausgeführten Stadt-/Regions-/Höhenfreigaben, enthält aber keine REFUSED-Zeile zur weiterhin gemeldeten Ablehnung. Daraus lässt sich die blockierende Prüfung noch nicht eindeutig bestimmen. Die früheren Client-Aktionen und die zusätzliche gemeinsame Prüfung sind nun erfasst. Besondere Gebiets-/Questzustände wurden nicht pauschal freigegeben.

Frische Sicherung der vorherigen ASI, aller neun Save-/Begleitdateien und des bisherigen Logs:

`<Projektordner>\.local\blackstar-mod-20261001\summon-precheck\backups\20261001T184027Z`

Nur die ASI wurde ersetzt. Alle 19 geschützten Dateien, einschließlich sämtlicher Saves, sind unverändert verifiziert. Nachweise und Installationsbericht: `.local/blackstar-mod-20261001/summon-precheck/`. Die genaue Restursache und der Erfolg an den gemeldeten Außenstellen bleiben offen.

## Technischer Nachweis

Der native `CharacterLevelData`-Reader und der `DataDefinedDefaultStatData`-Reader wurden als private Codekopien mit den tatsächlichen Schwarzstern-Daten ausgeführt. Der Reader konsumiert exakt den belegten Leveldatensatz. Über `statusgroupinfo` (`1000001`) und `statusinfo` (`1000026`, `Stamina`) ist Regenerationseintrag 12 eindeutig zugeordnet.

Im Charakterdatensatz ändert die Erweiterung nach erfolgreichem Laden nur:

- Charakterdatensatz `_callMercenaryCoolTime`, Laufzeitoffset `0x70`: 3600 → 1.
- Charakterdatensatz `_callMercenarySpawnDuration`, Laufzeitoffset `0x78`: 600 → 60.000 Sekunden. Der native Reader liest an diesem Offset acht Bytes; 60.000.000 Millisekunden passen auch in einen vorzeichenbehafteten 32-Bit-Zeitwert.
- Leveldaten → Regenerationseintrag 12 → `_regenStat`: 20 → 30.000.000. Maximum 300.000, Anfangswert 100.000 und sämtliche anderen Eigenschaften bleiben identisch.

Ein zusätzlicher Resolverhook deckt bereits geladene Datensätze ab. Alle Layout-, Anzahl- und Ausgangswertprüfungen müssen stimmen. Die zusätzlichen Beschwörungsfreigaben gelten nur innerhalb eines threadlokalen Schwarzstern-Beschwörungsaufrufs. Geschachtelte Aufrufe für andere Reittiere und andere Threads erhalten keine Freigabe.

Die neue Flugfreigabe liest den Akteur aus dem nativen Bedingungskontext (`+0x828`), löst den Charakterindex über die belegte Komponentenroute auf und prüft Charakterkey `1000799`. Bei einem Reiter wird der native Reittier-Lookup verwendet und seine temporäre Referenz auch bei einem Lesefehler wieder freigegeben. Die erste Beschwörungsprüfung verwendet den originalen Instanz-Lookup mit der Instanznummer als 64-Bit-Wert. Der Plattformtest erhält nur an seiner belegten Aufrufstelle eine private Datenkopie mit freigegebenem Flag; gemeinsame Gimmickdaten werden nicht umgeschrieben.

Die Höhenprüfung verwendet einen atomar ersetzten Vtable-Slot (`0x5949e48`) mit der originalen Funktion als Rückfall. Ihr AVX-Funktionsanfang ließ sich im Test nicht durch MinHook verlagern. Die Slotlösung ist separat getestet; abweichende Pointer werden nicht überschrieben. Alle vierzehn Codehooks, Helfer, Selektoraufrufe und der Originalslot werden vor der Aktivierung geprüft. Bei einer fehlgeschlagenen Installation werden die Codehooks zurückgenommen.

Bestanden:

- Originale Level-/Statreader auf echten Daten; exakte Grenzen und Ausdauerzuordnung.
- Bytevergleich: nur die vorgesehenen Charakter-/Ausdauerfelder wechseln; Gesundheit, Maxima und andere Reittiere bleiben unverändert.
- Zehn Fälle mit abweichenden Daten werden ohne Schreibzugriff zurückgewiesen, einschließlich falscher Dauer und unvollständig geänderter Datensätze.
- Wiederholte Anwendung, geschachtelte Aufrufe, falsche Aufrufstellen und Threadtrennung.
- MinHook kann die vierzehn verwendeten originalen Funktionsanfänge korrekt verlagern; Aktivieren und Entfernen der Hooks in privaten Kopien erfolgreich.
- Beide frühen Auswahlwege, geschachtelte Aktionen, fehlende/fremde Auswahl, andere Threads und Wiederherstellung nach Ausnahme getestet. Die fünf originalen Aktionsargumente, Rückgabepointer und Fehlerausgabe bleiben erhalten. Für andere Reittiere bleiben die ursprünglichen „nein“-/„unbekannt“-Antworten erhalten.
- Die Fehlerdiagnose bleibt auf 64 unterschiedliche Kombinationen begrenzt; im Test werden trotzdem alle 100 Spielmeldungen mit ihren Originalargumenten und Rückgaben weitergegeben.
- Die originalen Stadt-, Regions- und Höhenprüfungen wurden in privaten Codekopien ausgeführt: erlaubte, verbotene und unbekannte Antworten korrekt zugeordnet.
- Der originale Instanz-Lookup wurde mit echten Speicherstrukturen nachgestellt und ausgeführt: Schwarzstern, anderes Reittier und fehlende Instanz korrekt erkannt.
- Abgrenzung gegen Wyvern, unberittene Spieler, andere Akteurtypen, andere Straßenparameter und fremde Aufrufstellen. Reittierreferenzen bleiben auch bei einem ausgelösten Lesefehler ausgeglichen.
- Atomarer Bedingungsslot: virtuelle Ausführung, Zurückweisen fremder Pointer, Wiederherstellung des Originals und des schreibgeschützten Speichers geprüft.
- Sieben verkürzte Eingaben werden vom originalen Statreader zurückgewiesen.
- Vorhandener Loader lädt die neue Erweiterung zusammen mit beiden Schlafplugins; eine nicht unterstützte EXE wird vor Hookinstallation abgewiesen.
- Nach Installation stimmen die Prüfsummen der bestehenden Inventararchive, der EXE, des Loaders und beider Schlafmods weiterhin.

Diese Tests ersetzen keinen Lauf im tatsächlichen Spielprozess. Insbesondere ist „jeder beliebige Außenpunkt“ damit nicht nachgewiesen.

## Jetzt im Spiel testen

1. Normal über Steam starten und den neuesten Spielstand laden (**Slot 2** zum Zeitpunkt dieses Updates). Im Log die neue `READY`-Zeile mit `town/region/height` prüfen.
2. Schwarzstern regulär verlassen beziehungsweise entlassen und erneut beschwören. Der Cooldown soll nach etwa einer Sekunde abgelaufen sein; bereits vorhandene Instanzen und laufende Animationen bleiben geschützt.
3. Mehrere Minuten fliegen, beschleunigen und wiederholt Feueratem verwenden. Ausdauer soll praktisch nicht ausgehen; Lebenspunkte verhalten sich normal.
4. In einem zuvor gesperrten, freien Außenbereich beschwören. Danach an Haus, Höhle und Innen-/Außenschwelle kontrollieren, dass geschlossene Räume weiterhin abgewiesen werden.
5. Ein rotes Gebiet aus dem Screenshot aufsuchen: von außen über die Grenze fliegen, tief über einen Stadtbereich abseits einer Straße fliegen und anschließend auf freiem Boden im Gebiet neu beschwören. Bei einer Ablehnung den Ort und die genaue Meldung festhalten; die neue Erweiterung protokolliert zusätzlich den ablehnenden Validator.
6. Bei anhaltendem Flug kontrollieren, dass die Beschwörung nach zehn Minuten weiterläuft. Speichern und Neuladen soll die verlängerte Restzeit erhalten; auch eine neue Beschwörung soll länger als zehn Minuten möglich sein. Ein tatsächlicher Lauf über volle 1.000 Minuten wurde nicht durchgeführt.
7. Bei freiem Außenbereich die bisherige Höhenbegrenzung überschreiten und kontrollieren, dass sie kein Abwerfen mehr auslöst. Wyvern und Bodenreittiere behalten ihre bisherigen Regeln.

Quellen der vorausgehenden Analyse: [Unlimited Dragon Flying](https://www.nexusmods.com/crimsondesert/mods/356?tab=description), [Flight Freedom](https://github.com/shin2344234/flight-freedom/tree/e8e53f8081a113fa1708632b883783cebe76a8a5). Umsetzung aus den lokalen Spieldaten; bestehende Fremdmodarchive wurden nicht installiert.

Quellcode, private Fixtures und Prüfprotokolle: `.local/blackstar-mod-20261001/`. Die Befunde der vorherigen Analyse bleiben in `.local/blackstar-analysis-20261001/` erhalten.
