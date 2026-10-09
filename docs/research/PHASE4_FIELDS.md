# B1/B2: zusätzliche Feldnachweise für Build 25381195

Stand 19.09.2026, Workbench v0.4.2. Eigene statische Auswertung der lokalen
EXE und Vergleich mit den bereits hashgebundenen Tabellen. Kein Debugger am
Spielprozess, kein Hook, kein Start/Stop, keine Saveauswertung. Spielbytes und
Disassembly bleiben ausschließlich in ausgeschlossenen lokalen Arbeitsdateien.

Untersuchte EXE: `bin64/CrimsonDesert.exe`, Version `1.0.0.2944`, SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Adressen unten sind virtuelle Adressen der Datei bei ImageBase `0x140000000`;
sie werden nicht zur Laufzeit benutzt. Das Ergebnis ist ein Feldnachweis,
keine Vanilla-Zertifizierung oder In-game-Abnahme.

## Händler und täglicher Refresh

Readerbereiche `0x14151b9a0–0x14151bec7` (Stock) und
`0x14151c09d–0x14151c3cf` (Store-Ausschnitt): Feldnamen in Fehlerpfaden,
Lesegrößen und Reihenfolge lassen sich den vorhandenen Recordbytes zuordnen.
`C` bezeichnet die gemessene Position des Stocklisten-Counts im Store.

| Feld | Diskposition | Breite | Behandlung |
|---|---|---|---|
| resetHour | C−17 | u32 | Erhalten, einschließlich Sentinel |
| resetDay | C−13 | u32 | Nur bestätigte 3/7 auf 1; bestehende 1 unverändert |
| buyableStockCount | C−9 | u32 | Bei neuen Verkaufsangeboten erhöhen |
| sellableStockCount | C−5 | u32 | Erhalten |
| Listenanzahl | C | u32 | Mit tatsächlicher Positionszahl neu schreiben |
| maxRefillCount | Stock+18 | u32 | Bestehende Bestandsoption |
| stockIndex | Stock+26 | u32 | Für neue Positionen fortlaufend; Bestand erhalten |
| importantSaveIndex | Stock+30 | u32 | Oberhalb bisherigem Maximum; Lücken erhalten |
| refillByResetStore | Stock+38 | u8 | Aus vorhandener normaler Vorlage erhalten |
| isStockSellable / isStockBuyable | Stock+39/+40 | je u8 | Perspektive des Spielers: verkaufen / kaufen |
| isRestoreItem | Stock+41 | u8 | Aus Vorlage erhalten |
| Itemkey und Duplikat | Stock+43/+102 | je u32 | Beide gemeinsam ersetzen |

Alle 397 interpretierten Stores haben passende Buyable-/Sellable-Counts;
alle 6.376 bestehenden Stock-Indizes entsprechen ihrer Listenposition.
369 Stores besitzen Resetintervall 1, 3 oder 7. Die globale Tagesoption ändert
genau **246** Records von 3/7 auf 1. 123 sind bereits täglich. 28 interpretierte
Stores mit permanentem/unbekanntem Intervall und 39 opake Stores bleiben erhalten.
Die acht September-Zusatzbytes sind **nicht** das Refreshfeld und bleiben roh.

Eine getrennte Plausibilisierung liefert der Modautor von
[Daily Store Refresh](https://www.nexusmods.com/crimsondesert/mods/3515):
Seine Beschreibung nennt ebenfalls 246 normale 3-/7-Tage-Intervalle.
Sie betrifft einen anderen Build und ersetzt unseren eigenen Feldnachweis nicht.

## Neue Artikel und Sortimente

Nur eine bereits vorhandene, bedingungsfreie, auffüllbare Verkaufsposition
desselben Händlers dient als Vorlage. Die Vorlage ist 127 Bytes lang, hat
Flags `[1,0,1,0]` bei +38, Itemwertart 0 und keine optionale Bedingung,
Zusatzbytes oder Effekte ab +110. Ihre Preisfaktoren und unbekannten Innenbytes
bleiben erhalten. Es werden keine Spielbytes fest in den Programmcode eingebettet.

210 Händler besitzen eine solche Vorlage. Bei zwei Händlern sind bestehende
importantSaveIndices nicht eindeutig; sie werden abgewiesen. Damit sind
**208 Händler** für Ergänzungen freigegeben. Nicht fortlaufende Stock-Indizes
oder widersprüchliche Counts sperren das Einfügen ebenfalls. Bestehende Records,
Bedingungen, Preise und Save-Indizes werden nicht umnummeriert oder ersetzt.
Eine vorhandene kaufbare Itemposition wird nicht dupliziert; eine bloße
Ankaufsposition verhindert ein zusätzliches Verkaufsangebot nicht.

Auswahl aus allen 6.816 lokalisierten Items, auch komplette Auswahl, ist möglich.
Neue Positionen erhalten Händlerausnahme, globalen Bestand oder standardmäßig
999. Seit v0.4.3 kann `vendor_options` pro Händler die gemeinsame Artikelauswahl
ersetzen oder mit einer leeren Liste unterdrücken. Separate Refresh-Ausnahmen
erzwingen tägliches Auffüllen oder erhalten das Originalintervall. Explizite
Ausnahmen gelten auch außerhalb der globalen Händlerauswahl; `null` erbt wieder
die globale Einstellung einschließlich ihrer Auswahl. Bestandsausnahmen bleiben separat.
Der Tabellenindex wird bei Recordwachstum neu aufgebaut. Echte Tests prüfen
ein kleines Sortiment und alle Items bei Händler 3101 samt unveränderten
anderen Records und Rücklesen des verschlüsselten Overlays.

Die Workbench begrenzt einen Plan auf 100.000 Kombinationen aus unterstütztem
Händler und gewähltem Item sowie 9.999 Positionen je Händler. Dies sind
Vorschau-/Parsergrenzen, keine empirisch belegten Engine-Maxima. Ein kompletter
Katalog bei allen Händlern gleichzeitig ist damit noch nicht unterstützt.
Sonderhändler, beliebige Itemarten, Preise, UI-Verhalten und persistenter
Warenbestand benötigen weiterhin In-game-Prüfung.

## Dropchancen und manuelle Garantien

Der Dropsetreader liegt bei `0x1414f9be0–0x1414f9e08`. Die Auswahlfunktion
`0x1422cfd70–0x1422cffda` verwendet bei Rolltyp 0 pro Eintrag einen neuen
Zufallswert im Bereich 0 bis 999.999 und prüft, ob dessen Rate größer ist.
Typ 1 begrenzt erfolgreiche Einträge auf die Rollzahl. Typ 2 verwendet eine
gewichtete kumulative Auswahl. Typen 1/2 werden deshalb **nicht** wie unabhängige
Prozentchancen behandelt oder auf Typ 0 umgeschrieben.

Der neue Modifier akzeptiert ausschließlich unbedingte Itemeinträge mit
Rolltyp 0, passendem Itemduplikat, gültigen Mengen, Rate ≤ 1.000.000,
unveränderten Null-/Sonderfeldern und Gesamtgewicht gleich Summe der Raten.
Im gebundenen Build erfüllen **12.306 Sets** diese Kriterien. Alle haben auch
positive Mindestmengen und sind für die manuelle Garantieauswahl geeignet.
Die 32-Bit-Felder nach der 64-Bit-Rate werden nicht als zweite Chance verändert.

Berechnung: `min(original_rate × multiplier, 1_000_000)` mit breitem Integer;
anschließend Gesamtgewicht neu summieren. Rolltyp, Rollzahl und alle anderen
Felder bleiben erhalten. Menge und Chance werden gemeinsam aus dem Original
neu berechnet. Einzelausnahmen haben Vorrang vor globalem Faktor/Auswahl;
explizite Garantien setzen die Basisrate auf 1.000.000 und haben wiederum Vorrang
vor Chancenfaktoren. Eine Garantie bei effektivem Mengenfaktor 0 wird abgewiesen.

„Garantiert“ bezeichnet die Einzelchance **beim Auslösen dieses Sets**.
Übergeordnete Ereignisse, Laufzeitmodifikatoren, weitere Spielbedingungen und
Bosszuordnungen werden nicht geändert oder automatisch nachgewiesen. Bosssets
müssen manuell ausgewählt werden. Zufallsauswahl und Ergebnisweitergabe sind
statisch untersucht; eine gemessene Gameplaywirkung steht aus. Mengen und
globale Chancen können auch Rezept-/Questbelohnungen betreffen.

## B0: Befund zum lokalen Steam-Cache

Für den installierten Build wurden ausschließlich die beiden zugeordneten
Depotcache-Manifeste gelesen:

- Depot 3321461, Manifest 1190168329432671189: 282 Einträge.
- Depot 3321466, Manifest 4235648687492059552: 4 Einträge.

Die Namen sind unverschlüsselt, Metadaten vorhanden; beide Signaturabschnitte
haben jedoch Länge **0**. Ein Dateiabgleich mit diesen lokalen Hashes allein
bestätigt nicht unabhängig deren Herkunft. Es wurde weder Steam-Verify noch
ein Download oder ein vollständiger 152-GB-Archivhashlauf gestartet.
Die Strukturerkennung wurde anhand des primären
[SteamKit-DepotManifest-Readers](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs)
überprüft; kein Code daraus wurde übernommen und keine Abhängigkeit hinzugefügt.
Live-Apply benötigt weiterhin eine vertrauenswürdig bestätigte Ausgangsbasis,
verifizierte Live-Sicherung und integrierten Startschutz.

Lokale Belege: `.local/phase4-field-xrefs.json`,
`.local/phase4-reader-evidence.txt`, `.local/phase4-drop-runtime.txt`,
`.local/phase4-steam-manifest-inspection.json`,
`.local/phase4-v3-rust-tests.log`, `.local/phase4-v3-native/result.json`.
