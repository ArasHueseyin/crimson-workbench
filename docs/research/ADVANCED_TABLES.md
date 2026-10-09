# Bestätigte Felder für Phase 5

Neu: [Reparatur-Laufzeitprototyp](REPAIR_RUNTIME.md) mit isoliertem nativen
Ausführungsnachweis. Die folgenden statischen Tabellenbefunde bleiben gültig.

21.09.2026. Ausschließlich Steam 25381195, EXE 1.0.0.2944,
SHA-256 `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Die Analyse liest die EXE als Datei und kleine ausgewählte Archivtabellen;
kein Debugger, Prozesszugriff, Runtime-Hook oder Save-Reader.

Die folgenden virtuellen Adressen beziehen sich ausschließlich auf diese EXE.
Die Feldnamen stammen aus ihren Serialisierungsfehlermeldungen; Breiten und
Reihenfolge aus den zugehörigen Lesern. Adressen sind keine Laufzeit-Patchziele.

| Tabelle | Datensätze | Verifiziertes Layout |
|---|---:|---|
| characterinfo | 7.250 | u32-Key, längenpräfixierter UTF-8-Name, Blockbyte; Präfix bis vehicle u16, callMercenaryCoolTime u64, callMercenarySpawnDuration u64; Reader um 0x1414f03f6 |
| regioninfo | 1.007 | Reader 0x141516f00; limitVehicleRun, isTown und anschließend forbiddenMercenaryKeyList: u32-Count + **u8**-Keys; vollständiges Ende geprüft |
| inventory | 21 | IDs 2/8/9/13/15–19 editierbar. Slots u16; Reader 0x1415087c0. Ab v0.5.7 zwanzig Präfixe strukturell gelesen, Character-Slotstart 9272 weiterhin hashgebunden; vollständiger Suffix geprüft. Details unten |
| skill | 2.069 | Cooldown u32 direkt nach Blockbyte, Reader 0x141518830; ab v0.5.6 vollständig struktureller Matrix-/Suffix-Reader, keine Gleichheit von skillGroupKey und Record-Key vorausgesetzt |
| equiptypeinfo | 117 | Reader 0x1414fc990; decreaseEndurancePercent u64 nach 12+8+2 Präfixbytes hinter dem Recordheader; vollständiges Ende geprüft |
| spawningpoolautospawninfo | 140 | **u32**-Headercount, u32-Key; Reader 0x14151a910; vollständiger Record geprüft |
| terrainregionautospawninfo | 134 | **u32**-Headercount, u32-Key; Reader 0x14151a450; Präfix und alle Spawn-Gruppen gelesen, unveränderter Bitmap-/Scheduling-Suffix opak |
| fieldinfo | 8 | Reader 0x141505b30: aktuelle letzten 15 skalaren Bytes; alwaysCallVehicle_dev bei Ende−10. Nur Key 1 / MainField freigegeben |
| statusinfo | 84 | u32-Key/Name aus eigener kleiner, hashgeprüfter Tabelle; keine erfundenen Stat-IDs |
| iteminfo | 6.816 | Vollständiger typisierter 2.03-Reader/Writer des vorhandenen MIT-Ports; vor und nach Bearbeitung vollständiger Roundtrip |
| stageinfo | 52.080 | Nur zwei explizite Spawn-Patrouillen bearbeitbar; u32-Headercount, aktueller Sequencer-Präfix und `resetSecond` u32 bei Reader 0x14151b1ae |
| conditioninfo | 10.798 | Nur Regel 1011130; vollständige Prüfung von Header, booleschem Baum, drei Flags, Originalausdruck und Parsertyp; Reader 0x1414ef3b0 |
| factionreblockadinginfo | 108 | u16-Headercount und u16-Key; vollständiger Reader 0x1414ffd10, nur delayTime u32 editierbar |

## Vollständige Skill-Feldabdeckung v0.5.6

Eigene Nachprüfung des aktuellen `SkillInfo`-Readers `0x141518830` und seiner
Ressourcenhelfer. Der u32 unmittelbar hinter der Buffmatrix wird bei
`0x1415188ee` gelesen und bei `0x141518904` nach mem +0x28 geschrieben:
`skillGroupKey`, **nicht** nochmals der Datensatzschlüssel. Danach folgt die
`parentSkill`-Referenz (`0x141518907`). Die bisherige Suchabgrenzung über gleiche
Kennungsbytes wird vollständig entfernt. Bereits vorhandene Bufflayouts
bestimmen nun direkt die Suffixposition; unbekannte Typen und ungültige
Counts/Nullflags werden abgelehnt. Kein Rückfall auf eine spätere Kennung.

Die Buffmatrix war bereits intern strukturell gelesen; bis v0.5.5 hing ihre
vorgegebene Endposition jedoch noch von dieser Suffixsuche ab. Erst v0.5.6
entfernt die Suchabhängigkeit im gesamten Produktpfad, auch beim Catalog-Read.

Zusätzliche schreibgeschützte Felder: Header, Gruppen-/Elternreferenzen,
Lernvoraussetzungen, Upgrade-Graphen mit Kurventag, Charakter-/Bedingungslisten,
Wissens-/Fraktionsreferenzen, Ressourcenlisten, Batterie, Flags, Slots,
Skillgruppen, Entwicklertexte und Videoreferenz. Feldnamen stammen aus den
Reader-Fehlermeldungen; keine Gameplay-Einheiten hinzuerfunden.

`UseResourceStat` (`0x141518720`) serialisiert u8 `statType`, u32
`statusInfo`, u8 `isRegen`, i64 `varyStatAmount`, u32 `increaseStatusInfo`
und u32 `decreaseStatusInfo`. Die beiden letzten Wörter wurden zuvor nur
übersprungen; sie sind **keine** zusätzliche 64-Bit-Zahl. Der Listenreader
`0x141527950` ruft diesen Helfer auf. `UseResourceItem` (`0x14152cfe0`)
liest pro Eintrag eine u32-Itemreferenz (`0x14152d062`, Auflösung mit
`0x14038aac0`) und acht Bytes `useItemCount` (`0x14152d09f`). Deren
signedness und erlaubter Änderungsbereich sind durch diese Leseoperation
allein nicht bewiesen; deshalb benannter Hexwert und keine neue Editierfreigabe.
Die bestehenden skalaren Änderungen und Buff-Pfade bleiben kompatibel.

**Datenprobe:** 87.697 Header-/Suffixfelder über 2.069 Skills. Alle Recordbytes
werden aus diesen Feldern, Matrixcounts und Bufffeldern rekonstruiert;
lückenlose Abdeckung und exakte Gleichheit aller Records geprüft. Weiterhin
4.607 Buffeinträge und neun Summon-/ein AddSubLevel-Payload. Synthetische Proben
verwenden abweichende Gruppenreferenzen, Kennungsbytes in UTF-8-Texten,
nichtleere Referenz-/Ressourcenlisten, abgeschnittene Records und übergroße Counts.

Private Nachweise: `.local/phase5-v7-skill-reader.txt`,
`phase5-v7-resource-readers.txt`, `phase5-v7-resource-stat.txt`,
`phase5-v7-pinned.log`. Keine EXE-, Runtime-, Spiel- oder Save-Änderung.
Dies schließt die bisherige Darstellungslücke außerhalb der Buffmatrix;
unbekannte Buffbedeutungen/Einheiten und andere Phase-5-Lücken bleiben offen.

### Ergänzungen v0.5.5: Reparaturkostenpfad und verworfener Nullansatz

Die ItemInfo-Zugriffe auf `mem+0x408` (Reparaturliste) und `mem+0x410`
(Count) führen zu einem gemeinsamen Kostenpfad `0x14240dfb0` sowie dem
Serverpfad `0x142be3ee0`. Beide verwenden den ItemInfo-Getter `0x14038ab60`.
Eine leere Reparaturliste liefert `eErrNoCantRepairItem` (globaler Fehlercode
bei `0x146cf7b5c`, Namenregistrierung `0x1422072a7`). Eine fehlende passende
Material-/Stilregel liefert `eErrNoCantRepairResourceItem` (`0x146cf7b70`).
Die aktuell leeren 6.816 Listen sind damit keine Regeln mit verstecktem Preis 0.
Es folgt daraus keine Behauptung, alle denkbaren Skriptkosten seien bekannt.

Die Kostenhilfe `0x142411f20` verwendet `RepairData.resource_item_count`
bei `mem+8`. Ein globaler Prozentwert bei `0x146cfd9b8` wird mit 100 initialisiert
(`0x1401f42fb`, Konfigurationshash `0x64cbd1c9`). Bei diesem Standardwert liefert
die Hilfe die Materialanzahl direkt zurück, ohne Nullwertklemme. Der Aufrufer
dividiert das verfügbare Material durch genau diesen Rückgabewert:
`idiv rbx` bei `0x14240e0e5` und `idiv r14` bei `0x142be406d`.
Der Sonderwert −1 umgeht zwar eine Materialbegrenzung; für gewöhnliche
Materialmengen gibt es vor der Division keine Nullprüfung. **Ein Nullsetzen der
Materialanzahl kann deshalb eine Division durch null auslösen.** Kein Laufzeit-
Crash wurde provoziert; dies ist ein statischer Nachweis an der gepinnten EXE.

Der v0.5.0 eingeführte allgemeine Item-Writer enthielt dafür bereits einen
Nullsetzpfad. Im unveränderten unterstützten Build war er durch die leeren
Listen und die Core-Prüfung nicht erreichbar. v0.5.5 entfernt den Schreibpfad
vollständig. Core und Format-API verweigern `free_repair: true` unabhängig von
der Zahl vorhandener Regeln; Core prüft globale und individuelle Flags vor
allen Advanced-Änderungen. `false` bleibt mit alten Requests kompatibel.
Die Oberfläche verwendet die reine Regelanzahl nicht mehr als Freigabe und
ermöglicht das Entfernen einer alten aktiven Option aus Item-Vorlagen.

Ein weiterer Spawn-Kandidat ist jetzt zugeordnet: `_spawnIntervalTime`
(`0x1423d2ba0` → `0x14f6e42c0`) registriert sich über `0x140c34f80` bei
`GameData_TimelineEvent_SummonCharacter`. RTTI-Metadaten-VTable `0x145622f88`,
Objekt-VTable `0x14562f498`. Der Editor-Text benennt Sekunden, aber es handelt
sich um ein Beschwörungsereignis innerhalb einer Timeline. Dies belegt keinen
allgemeinen NPC-Respawn-Regler. Keine Timeline-/EXE-/Save-Änderung vorgenommen.

Belege: `.local/phase5-v6-repair-{cost,resource,globals}.txt`,
`phase5-v6-repair-global-xrefs.json`, `phase5-v6-interval-{config,type,rtti}.txt`.
Kostenfreie Reparatur bleibt offen: Für einen separaten, geeigneten Datenpfad
liegt noch kein Nachweis vor. Runtime-Phase D wurde nicht begonnen.

### Ergänzungen v0.5.4: benannte Beschwörungsfelder

Typ 10: `SummonBuffData` ruft bei `0x141f2d790` für `mem+0x90`
den Reader `0x1414ed330` (`SummonCharacterData`) auf; anschließend folgen
u8/u32/u32 bei `mem+0x180/184/188`. Der eingebettete Reader bestätigt die
Feldnamen durch eigene Fehlertexte. Die Select-Liste hat einen u32-Count
und je zwei u16-Referenzen (`0x1415439b0`). `0x14151f0a0` liest jeweils
vier feste u32-Hashes, keinen dynamischen Count. Beide Texthilfen
`0x14141f540`/`0x14141f710` haben einen u32-Bytecount. Terrain-Daten folgen
`0x1414ecd40`: u32, Text, u8, u16, acht Bytes. `0x142157fe0` liest eine
Vier-Byte-Referenz. Optionale eingebettete Bedingungen fehlen in allen neun
beobachteten Payloads und werden bei Auftreten ausdrücklich abgelehnt.

Damit werden alle neun Payloads mit je 181 Bytes vollständig aufgeteilt,
auch die drei hintereinander in Skill 41358. Select-Listen und Texte sind
längenbasiert; keine pauschale Annahme einer festen Summon-Gesamtlänge.
Es gibt 58 einzelne Felder im beobachteten Summon-Payload. Referenzen,
Counts und alle neuen Werte bleiben schreibgeschützt. Ganzzahlansichten
zeigen Bitwerte ohne zugesicherte Einheit; Position, Yaw und Höhenbereich
bleiben Hexwerte. Aus `deadLimitTime`, `spawnPercent` oder
`spawnableCheckInterval` wird kein allgemeiner NPC-Respawn-Regler abgeleitet.

Typ 74: Der unverschleierte Schreiber `0x141f315b0` schreibt zuerst den
aufgelösten Vier-Byte-Schlüssel aus `mem+0x90` (Getter `0x14042fe50`),
danach vier Bytes aus `mem+0x94`. Letzteres bleibt `payload.mem_148` in
Hexdarstellung; keine Annahme eines Level-Integerformats. Der einzelne
beobachtete Payload liegt in Skill 65145, Zeile 0/Spalte 1. Der frühere
Acht-Byte-Rohblock wird keinesfalls als bearbeitbarer i64-Wert umgedeutet.

Die Tabellenprobe rekonstruiert alle Matrixbytes einschließlich dieser zehn
Payloads aus den angezeigten Feldern. Synthetische Tests prüfen UTF-8,
nichtleere Select-Listen, verschobene Folgefelder, jeden abgeschnittenen
Summon-Präfix und abgelehnte Strukturänderungen. Die Suche in der Oberfläche
berücksichtigt zusätzlich Feldpfade; bestehende editierbare Pfade bleiben stabil.
Belege: `.local/phase5-v5-{types,summon-helpers,sublevel}.txt`,
`phase5-v5-structured-buffs.json`, `phase5-v5-pinned-final.log`.

**Weitere Recherchegrenzen:** `_respawnTimeSecond` gehört laut RTTI zu
`NPCScheduleMissenscenePatternSaveData` (Metadaten-VTable `0x1458088f0`,
Initialisierung `0x14195ad90`), nicht zu einer belegten editierbaren Spawn-Tabelle.
Die selektiv gelesene `gamedata/npcschedulespawnoption.xml` enthält Skill-/Spawn-
Zuordnungen, keinen Respawn-Timer (11.166 Bytes, SHA-256
`f2dabc48db34533385d44619e062228820232c61a214ae342f3ed244c4957f9a`).
Saveinhalte wurden nicht gelesen. Leere Reparaturlisten und gefundene
Repair-Komponenten/GUI-Texte liefern weiterhin keine zusätzliche bestätigte
Kostenquelle. Diese Lücken bleiben offen.

### Ergänzungen v0.5.3: Wiederbesetzung und Inventardeckel

`FactionReblockadingInfo` wird bei 0x1414ffd10 vollständig gelesen: u16-Key,
UTF-8-Name, Blockbyte, Questliste, Fraktionsknotenliste, `delayTime:u32`,
`protectCombatPower:f32`. Die Questliste (0x14153b680) enthält pro Eintrag
Questreferenz u32, Bedingungsreferenz u32, Rate u64 und `closeTime:u32`.
Auch die im Speicher nur zwei Bytes belegende Bedingungsreferenz wird auf
Dateiebene mit vier Bytes gelesen (0x14151efe0). Die Knotenreferenzen sind u32.
Alle 108 Records konsumieren genau ihr Ende, enthalten Quests und Knoten;
24 Wartezeiten sind 86400, 84 sind 432000. Nur die vier Bytes von `delayTime`
werden verändert. Keine Bearbeitung der Quest-Resets, Close-Timer, Bedingungen,
Wahrscheinlichkeiten oder Knoten. Die Zeiteinheit und das Zusammenspiel mit
Gebiets-/Questzuständen sind damit noch nicht im Spiel abgenommen.

Zusätzlich hashgeprüfte Dateien: Body 10.295 Bytes,
SHA-256 `8d17f717a2ac59b468419128cfe92746eb1c5f8aa39a26933a9166bc30b807f2`;
Header 650 Bytes,
SHA-256 `4ddc468506db2b615a4760328a1fe34fbee425d86d27589ad10ca1ece200aae9`.
Die Hashes gehören zum Advanced-Manifest; fehlende/veränderte Quellen werden
vor einem Plan abgelehnt. Lokale Belege: `.local/phase5-v4-reoccupation.json`,
`phase5-v4-reblock-reader.txt`, `phase5-v4-helpers.txt`, `phase5-v4-ref-widths.txt`.

Der Inventarpfad 0x142407770 vergleicht bei 0x142407b1f einen Slotindex mit dem
globalen i32 bei 0x146cfc078. Bei Index >= Wert wird der Pfad abgebrochen.
Die statische Initialisierung 0x1401ef7bb setzt diesen Wert auf **0x5b4 = 1460**
und registriert die Konfiguration mit Hash 0xf1197dab. Ein weiterer Zugriff bei
0x140ee5273 verwendet ihn zur Vorbelegung einer Inventarliste. Deshalb senkt
v0.5.3 die Workbench-Eingabegrenze für beide Slotfelder der Inventare 2/8/9 auf
1460. Die Formatbreite bleibt u16; eine Grenzverletzung wird abgelehnt, nicht
gekürzt. **Dies ist ein belegter Standarddeckel eines Codepfads und eine
konservative Workbench-Grenze, kein Beweis des aktuellen Laufzeitwerts, aller
Lagerpfade oder der Save-Kompatibilität.** Konfigurationsüberschreibungen wurden
nicht ausgeschlossen. Belege: `.local/phase5-v4-globals.txt`,
`phase5-v4-global-xrefs.json`, `phase5-v4-stack-cap.txt`.

Im selben Pfad schaltet `ItemInfo._applyMaxStackCap` (mem +0x111) die Prüfung
gegen den itembezogenen `maxStackCount` (mem +0x18). Ein separater Standardwert
200 wird nur in einem Sonderfall für ursprünglich einzelne Items herangezogen.
Er ist **kein globales Stack-Maximum**. Deshalb bleibt die bisherige explizite
Workbench-Stackgrenze 1.000.000 bestehen; ein universelles Engine-Maximum ist
weiter offen. Es wurde keine Konfiguration oder EXE geändert.

`LocalizableString` beginnt mit einer Kategorie, **nicht** mit einem Boolean.
UTF-8-Längen enthalten keinen angenommenen Nullterminator: Das nächste Byte
im Recordheader ist das echte Blockbyte. Alte Referenzreader verwechseln
hier teilweise Blockbyte und Cooldown. Alte FieldInfo-/RegionInfo-Layouts
sind ebenfalls nicht auf diesen Build übertragbar.

## Spawn-Gruppen

AutoSpawnTargetData (0x14151a120) enthält Gruppenlisten und Filter sowie
spawnLimitCount u16. AutoSpawnGroupData (0x141519c40) enthält eine Charakterliste;
AutoSpawnCharacterData (0x141519ab0) serialisiert:

`character u32, group u16, reason u32, subcharacter u32, subgroup u16,
subreason u32, characterCount u8, subCharacterCount u8`.

Farbwerte der Gruppe sind vier f32 (16 Bytes). Referenzen, Bedingungen, Prozent-
und Distanzwerte werden nicht für einen Mengenfaktor umgedeutet. Das Produkt
ändert nur Counts/Limits. Ein unerkannter Record sperrt den Katalog.

Die EXE enthält `_respawnTimeSeconds` in ItemInfo/GimmickInfo und reflektierte
`_respawnTimeSecond`/`_spawnIntervalTime`-Namen. Daraus folgt **kein belegtes
allgemeines NPC-Respawn-Feld** in diesen Spawn-Tabellen. Deshalb keine erfundene
Timer-Position und keine Umdeutung von Spawn-Abständen zu Sekunden.

**Gegenprobe v0.5.1:** Die gepinnte NattKh-Oberfläche bezeichnet
`spline.raw_qword` als Respawn-Timer. Im aktuellen Schema liegt dieser u64 am
Gruppenende nach Farbwerten und drei Flags. Der aktuelle EXE-Reader bezeichnet
ihn bei `0x14151a0a4` ausdrücklich als **`_spawnPercent`**. Alle 1.246 Gruppen
wurden nur lesend geprüft; Werte 0..1.000.000 passen zu einer skalierten
Wahrscheinlichkeit und beweisen keine Millisekunden. Dieser Kandidat ist damit
als allgemeiner Respawn-Timer verworfen und bleibt bei Mengenänderungen erhalten.
Belege: `.local/phase5-v2-spawn-group-reader.txt`,
`.local/phase5-v2-spawn-timer-candidate.log`.

## Reittiere und Regionen

Riding_Dragon_1 ist Key 1000799, vehicle 16984, Cooldown 3600, Dauer 600.
Boss_Dragon_60003 ist ein separater Datensatz und wird nicht bearbeitet.
Die aktuelle Mercenary-Tabelle identifiziert Vehicle_Dragon als Key 79.
Region_Abyss (Key 8) enthält die Verbotsliste `[65,78,79,80,82,81]`; andere
Regionen enthalten keine Einträge. Die Option entfernt nur 79. Alle anderen
Einträge bleiben erhalten; Count und Tabellenoffsets werden aktualisiert.

Das globale Städtefeature bearbeitet limitVehicleRun nur bei isTown == 1
und alwaysCallVehicle_dev nur im MainField. Es verändert keine Lobby-,
Rematch-, UI- oder anderen Instanzfelder. Die Entwicklung behauptet keine
nachgewiesene Abschaltung aller möglichen Dismount-/Summon-Prüfpfade.

## Skills und Items

Der komplette Skill-Post-Buff-Suffix wird strukturell gelesen. Die Buffmatrix
bleibt bei Änderungen byteidentisch; seit v0.5.1 ist ihre Struktur zusätzlich
schreibgeschützt inspizierbar (Details unten). Stamina = Status 1000026, Mp = 1000027;
nur Stat-Typ 3 und negative Kosten werden skaliert. Beispiele: Climb −10000,
ClimbStruggle −40000, Swimming +20000 (Regeneration), Swimming_Run −10000.
Dieser Unterschied ist auch im Mutationstest abgesichert.

Seit v0.5.1 kann eine ausgewählte Original-Enchant-Zeile in eine neue Zielstufe
kopiert werden. Die Originalreihenfolge muss streng aufsteigend sein, alle
Trenner müssen null sein. Ganze Zeilen werden über den typisierten Reader/Writer
kopiert und anschließend sortiert; 64 neue/256 gesamte Zeilen sind Workbenchgrenzen.
Stats/Buffs werden danach geändert. Ein erreichbarer Upgradeweg ist damit nicht
nachgewiesen. Ganzzahlbreiten folgen dem Format; Einfügen verändert
die tatsächliche Arraylänge und führt zum Neuaufbau des Tabellenheaders.
Die Liste bekannter Buff/Level-Paare stammt aus anderen Originalitems desselben
Equip-Typs. Vollständige Semantik/Balance/Kompatibilität ist nicht bewiesen.

Alle 6.816 aktuellen Item-Reparaturlisten sind leer. `max_endurance == 65535`
ist bei normalen Items ein Sentinel und wird nicht als Beweis aktiver
Haltbarkeit benutzt. `item_charge_type == 2` und ein einzelner Verbrauch sind
auch auf gewöhnlicher Munition vorhanden. Der konservative Stackfilter
berücksichtigt daher zusätzlich tatsächliche Equip-/Enchant-/Socket-/Ladungs-,
Subitem- und Siegelmerkmale; er ist keine Zusicherung verlustfreien Stackens.

## Skill-Buffmatrix v0.5.1

Eigene statische Leseanalyse der EXE-Datei (kein Prozesszugriff): Factory
`0x141f24930`, Typ-Switch `0x141f289b8`, gemeinsamer Reader
`0x141f28cc0..0x141f29284`, virtueller Payload-Reader an VTable + `0x50`.
RTTI liefert die Typnamen. Die Formatbeschreibung
`crates/cd-core/schemas/steam-25381195.skill-buffs.json` enthält ausschließlich
Typnamen und Strukturbreiten, keine Spielwerte oder Codeausschnitte.

Alle 2.069 Skills werden deterministisch bis zum separat validierten Suffix
gelesen: 4.607 Einträge einschließlich null. Kein Suchlauf mit erster
passender Payloadgrenze und kein längenratender Laufzeitcache. Variabel sind
u32-Strings, String-/Referenzarrays und das typabhängige Immune-Array.
Typ 35 enthält ein u32-Array, 44 einen String plus 30 Bytes, 45 einen
u32-Wert, Stringarray und Flag, 87 ein Referenzarray und zwei Flags.
Typ 61 liest einen String und u32 (Weiterleitung `0x14e047180`),
Typ 92 zwei u16 (`0x14e04fce0`).

**Historische Grenze v0.5.1–v0.5.3:** Payload 10 (Summon, 181 Bytes) und 74
(AddSubLevel, 8 Bytes) waren nur als buildgebundene Rohblöcke abgegrenzt.
Die Aufteilung ab v0.5.4 ist unten dokumentiert; keine vollständige semantische
Interpretation oder Editierfreigabe dieser Felder. Seit v0.5.2 sind Graphen
als drei i64-Werte und u32-Kurventag sichtbar. Die drei
Werte sind editierbar, der Kurventag bleibt schreibgeschützt.
Zahlen sind Rohinterpretationen ihrer Bitbreite, keine behauptete
Einheit. `common.mem_*` folgt Strukturpositionen, nicht erratenen Gameplaynamen.
Die vollständige Rohansicht enthält auch sämtliche Referenzen und unbekannten
Werte außerhalb der Matrix. Abweichende, abgeschnittene oder mehrdeutige
Strukturen werden abgelehnt, nicht stillschweigend übersprungen.

Nachweise: `.local/phase5-v2-buff-*.txt/json`,
`.local/phase5-v2-deterministic.log`, `.local/phase5-v2-pinned.log`.
Die ursprünglich mehrdeutige Forschungsprobe wurde nicht als Produktparser
übernommen. Der Test prüft vollständige lückenlose Matrixabdeckung pro Record.

## Herkunft und Reproduzierbarkeit

### Ergänzungen v0.5.2

**Skill-Buffs:** `common.mem_24/32/40`, feste i64-Payloadwerte und die drei
i64-Komponenten des Graphreaders `0x1424047b0` lassen sich überschreiben.
Referenzen, Counts, Flags, Kurventags und opake Payloads werden abgelehnt.
Pro Skill höchstens 1.000, insgesamt höchstens 10.000 Änderungen; Zahlen bleiben
dezimal als Strings erhalten, ohne JavaScript-Rundung. Jede Änderung schreibt
genau acht Bytes. Der Reader validiert vor und nach der Bearbeitung dieselbe
Matrixgrenze. Das ist eine Editierfreigabe der belegten Zahlenstruktur, keine
Behauptung ihrer Gameplaybedeutung.

**Stage-Reset:** Eigener aktueller Präfixreader des SequencerDesc
(`0x142348900`) und StageInfo (`0x14151abc0`). Forschungsprobe: 51.150 Präfixe
abgegrenzt, 930 mit komplexen Bindings ausdrücklich ausgeschlossen; keine
fehlgeschlagenen Reads innerhalb der unterstützten Variante. Von 618 direkten
Spawnreferenzen haben genau zwei positive endliche `resetSecond`-Werte:
1017811 / `Faction_Byron_HiddenEstate_Block_FactionPatrol` und 1002224 /
`Watergate_Block_Patrol`, jeweils 259200. Das Produkt erlaubt ausschließlich
diese zwei Schlüssel und Namen, mit positivem Timer und direkter Spawnreferenz.
Die anderen 52.078 Stages einschließlich Sentinelwerten und Questtimern bleiben
unverändert. `stageinfo` ist bereits im ursprünglichen Hashmanifest enthalten.
Nachweise: `.local/phase5-v3-stage-*.txt`, `phase5-v3-stages.json`.

**Stadtflug:** Der Hinweis auf Condition 1011130 stammt aus
[Flight Freedom, signatures.h](https://github.com/shin2344234/flight-freedom/blob/d61a9c37fc2fc0970d155329ddbf143038f0910d/mod/src/game/signatures.h)
(MIT; Recherchehinweis, kein übernommener Plugin-Code). Eigene statische
Prüfung im installierten Build bestätigt den Ausdruck
`IsInTown() && !IsAboveRoad(Bird,20)`. Die Tabelle wird zusätzlich gehasht;
ihre Body-/Headerhashes stehen in `steam-25381195.advanced.json`.

ConditionInfo liest den Baum über `0x1422b4300`, danach drei u8-Flags,
Originalausdruck und Parsertyp. Factory `0x142440540` verwendet Opcode 1
für `AndContentsLogicFunction`, 2 für `NotContentsLogicFunction`, 3 für
eine ConditionData mit u16-Typ. Typ 2 hat RTTI `ConditionData_CheckNone`
(VTable `0x145950b00`), keinen Payload und beantwortet die Prüfung mit Yes=0.
`NotContentsLogicFunction` (`0x1424414f0`) wandelt 0 in No=1, 1 in 0 und
erhält Unknown=2. Damit ist `!CheckNone()` eine belegte falsche Bedingung,
kein geratenes Flag. Originalkey, Stringkey, Blockbyte, drei Flags und
Parsertyp bleiben erhalten; Ausdruck und Baum werden konsistent ersetzt.
Der Tabellenheader wird anschließend neu aufgebaut. Andere 10.797 Bedingungen
bleiben byteidentisch. Das betrifft die gemeinsame Stadtflug-Regel, auch bei
anderen Flugreittieren. Keine Aussage über Quest-/Zwischensequenz-Abstiege
oder Höhengrenzen. Keine Änderung an EXE, Runtime oder Saves.
Nachweise: `.local/phase5-v3-condition-*.txt/json`.

Der zusätzlich geprüfte Kandidat `MercenaryInfo._isMustHideAndRestore`
steht bei Blackstar bereits auf 0. Er wird deshalb nicht als scheinbare
Grenzfreigabe angeboten. Alle Item-Reparaturlisten bleiben leer; aus den
vorhandenen Repair-UI-/Komponentennamen wurde kein Kostenfeld abgeleitet.
Allgemeine NPC-Timer und Engine-Caps bleiben offen.

### Bisherige Quellen

- [crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0), lokal gepinnter
  Commit `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0`, MIT; vorhandener Item-Port.
- Lokal gepinnte NattKh-ModdingTools und NattKh-Editor dienten als
  Formatreferenzen. Keine Übernahme ihres Parsercodes in die neuen Core-Reader;
  deren alte Layoutannahmen wurden am aktuellen EXE-Reader geprüft.
- Eigene begrenzte In-Memory-Reader unter `mods/advanced/reader.rs`;
  neue Item-Fassade `vendor/crimson-format/src/item_mods.rs`.
- Private Auszüge und Disassemblierungen liegen ignoriert unter `.local/`,
  keine Spieltabellen/EXE-Ausschnitte werden mit dem Quellprojekt ausgeliefert.
- `pinned_advanced_records_and_edits` prüft die zuvor extrahierten Quellen
  explizit gegen die veröffentlichten Hashes, dann Identität, Mutationen,
  Offsetneubau, positive Ressourcenzuwächse und Stat-Einfügen. Der Test ist
  außerhalb dieser lokalen Rechercheumgebung bewusst `ignored`.


## Restprüfung v0.5.7: Item-Haltbarkeit und weitere Lager

Grundlage bleibt die hashgeprüfte EXE 1.0.0.2944 des Steam-Builds 25381195.
Nur Datei-Reads, kein Zugriff auf Prozessspeicher oder Saveinhalte.

`ItemInfo::_maxEndurance`: Reader `0x14150fd15`/`0x14150fd25` liest zwei Bytes
in Objektposition `+0x400`; ItemInfo-Auflösung über `0x14038ab60`. Der gemeinsame
Haltbarkeits-Updater `0x14240d650` vergleicht bei `0x14240d687` mit `0xffff`
und überspringt dann den Write der Instanz-Haltbarkeit (`+0x40`). Im selben
Updater gilt die Sentinel-Prüfung auch für angehängte Subitems (`0x14240d75c`).
Weitere Broken-/Nutzbarkeitsprüfungen vergleichen ebenfalls mit 0xffff,
z. B. Funktionen `0x14e6f9230`, `0x149d52c90`, `0x14ed246d0`. Damit ist 65.535 ein
belegter Sonderwert, nicht eine erfundene große Haltbarkeitszahl.

122 der 6.816 Items haben endliche, positive Haltbarkeit. Der neue Writer
ersetzt ausschließlich dieses u16 und lässt 0 sowie 65.535 unverändert.
`is_destroy_when_broken`, Reparaturlisten und andere Flags bleiben bytegleich.
Zusammen mit EquipType-Faktor 0 erreicht ihn der bestehende `no_wear`-Schalter;
keine Änderung des gespeicherten Request-/Vorlagenformats erforderlich.
Alle 6.816 Records wurden auf präzisen Feld-Diff und Idempotenz geprüft.
Eine nicht mehr existierende Instanz wird dadurch nicht wiederhergestellt.

Inventar: Reader `0x1415087c0`; Move-Array `0x141535d90`, Move-Record
`0x141508490`, verschachteltes ItemMoveData `0x141508360`. Selektorlisten
enthalten je u16+u8. Move: u8, zwei u16-Referenzen, u32-Itemreferenz, drei
Lokalisierungstexte, Count + 17-Byte-ItemMove-Records, optionale Condition,
Lokalisierungstext. Die zwanzig Nicht-Character-Records haben keine eingebettete
Move-Condition. Unbekannte Condition-Varianten werden abgelehnt. Character
behält seinen hashgeprüften Slotstart bei 9.272, da sein Baum noch opaque ist.
Der komplette Suffix wird jeweils bis EOF geprüft.

Neu freigegebene IDs und Slot-Offsets: Kuku 13/117, Housing_Dresser 15/148,
Housing_Refrigerator 16/147, Housing_Symbol 17/130, Housing_Collecting 18/141,
Housing_GatheredMaterials 19/164. Diese Positionen ergeben sich aus dem Reader;
eine Suche nach plausiblen Slotzahlen wäre beim Dresser mehrdeutig (0/0 bei 144).
Character und beide bisherigen Lager bleiben unterstützt. Währungs-, Quest-,
Recovery- und Fahrzeugcontainer bleiben ohne Editor. 1.460 ist weiterhin eine
vorsichtige Grenze aus einem bekannten Inventarpfad, kein universeller Savebeweis.

Stack-Codepfad: `0x1424078e2` summiert die Instanzmengen über das Inventar.
Bei gesetztem `applyMaxStackCap` (`ItemInfo +0x111`, Prüfung `0x142407a32`)
wird bei `0x142407ad6` die verbleibende Menge aus maxStackCount minus Summe
abgeleitet. Das Flag beeinflusst damit auch einen Gesamtmengen-Deckel; es wird
nicht blind deaktiviert. Ein universelles Maximum folgt daraus nicht.

Weitere Spawn-Namen `_spawnTime`, `_spawnableTime`, `_spawnDelayMin` und
`_spawnTermMin` sind in Reflection-Code auffindbar, liefern aber keinen
nachgewiesenen NPC-Tabellen-Timer. Das Umfeld enthält unter anderem SpawnVolume-
und Spline-Parameter. Ohne Klassen-/Dateizuordnung folgt daraus kein Editor.
Die zuvor belegten Gegenbeispiele (Wahrscheinlichkeit, Savedata, Timeline)
bleiben maßgeblich. Kein neuer allgemeiner Respawn-Patch.

Lokale Belege: `.local/phase5-v8-{focused,inventory-arrays,inventory-move,
inventory-nested,inventory-conditions,gap-readers,endurance-consumers}.txt`,
`phase5-v8-inventories.json`, `phase5-v8-pinned.log`. Disassembly und Original-
Inhalte sind ausschließlich ignorierte lokale Forschungsdaten.


## v0.5.8: vollständiges BuffInfo und zusätzliche Eigenkosten

Eigene statische Analyse desselben gepinnten EXE-Builds, kein Prozesszugriff.
`BuffInfo`-Reader `0x1414f98d0`: Kopf (u32-Key, UTF-8, Blockflag), u32-Anzahl
von (u32-Level, nullable polymorphem Buff), min/maxLevel (u32), Sequencertext,
Berechnungstyp (u8), drei 4-Byte-Referenzen und zwei boolesche Abschlussflags.
Nullable-Dispatch `0x141f2a190`, bestehender gemeinsamer Buffreader/-factory.
Alle 292 Datensätze und beide vollständigen Tabellenfiles round-trip-geprüft.

Typ 114 `AdditionalUseResourceStatBuffData`, Reader `0x141f351f0`: erst eine
u32-gezählte Skillreferenzliste (je vier Bytes), dann eine u32-gezählte Liste
`UseResourceStat`. Gemeinsamer Reader `0x141518720`: StatType u8, StatusRef u32,
isRegen u8, varyStatAmount i64, IncreaseStatusRef u32, DecreaseStatusRef u32.
36 negative Kosten in 33 BuffInfo-Records. Nur StatType 3 und Status 1000026
(Stamina) / 1000027 (Mp) freigegeben. 0/50/200-Prozent-Patches ersetzen
ausschließlich diese acht Bytes je Eintrag; alle anderen Bytes geprüft erhalten.
Der zuständige `CommonAdditionalUseResourceStatBuffProcessor` (`0x1420a9eb0`)
ordnet die Ressourcenliste jeder referenzierten Skill-ID zu; Registrieren und
Entfernen erfolgen über `0x1417addd0` / `0x1417adf40`. Der Zusatz gilt somit
für die im Buff benannten Skills und ist keine Änderung des allgemeinen
Ressourcenmaximums oder der übrigen Buffeffekte.
Neue reine Leselayouts sind als opaque markiert. Typ 69 besteht nach Sprung
von `0x141f310d0` zu `0x14e04cb70` aus zwei 4-Byte-Feldern.

Zwei benannte Skillidentitäten bilden Eigenverbrauch über Typ 12 ab:
40013 `Active_UseResource_Mp` (30 Werte) und 10300
`Skill_ElementalReinforce_UseMp` (ein Wert). Reader `0x141f2b970`: StatusRef
vier Bytes, dann drei i64; nur erster i64 nach Status 1000027 wird skaliert.
Die zwei übrigen Werte (Grenzen), Common-Teil und andere Payloads bleiben
erhalten. Keine pauschale Skalierung aller negativen Buffs: gegnerische Drain-
und Dot-Effekte sowie negative Regenerationsmodifikatoren sind keine Eigenkosten.
Explizite `skill_buffs`-Werte überschreiben berechnete Faktoren vor dem Patch.

Neue Quellenfingerprints im Advanced-Manifest: BuffInfo-Body 486.717 Bytes,
SHA-256 `67bb4107c1161a2f9684903d1053c061948a2f4381fe87f9e3c82b874d983ec7`;
Header 2.338 Bytes, SHA-256
`a5ec6b9db438898c93baaec0d28e60056319a649ffad5833de0fdb9c69a7a85d`.

### Weitere ausgeschlossene Wege

Der Reparaturhelper kennt Menge -1 als Sonderpfad, aber der normale Wrapper
liest die reale Ressourcenmenge und lehnt <= 0 ab (`0x140a11577..140a1157e`);
ebenso der serverseitige Aufrufer (`0x142be4311..142be4316`). Das macht einen
Nullkostenpatch nicht sicher. RepairAll hat einen anderen Pfad, beseitigt aber
die weiterhin erreichbare Division im Einzelreparaturpfad nicht.

`AICondition_IsOverMiseensceneSpawnableTime`: Vtable `0x1458c1bf8`, Check
`0x1422e3b10` ruft `0x1417160a0`. Der Check vergleicht Felder aus NPC-Plan-
Einträgen mit aktueller Tageszeit; kein bestätigter allgemeiner Respawn-Abstand.
Keine Save-, EXE- oder Laufzeitänderungen vorgenommen.
