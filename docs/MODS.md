# Modwerkstatt – B0 und B1–B3

Seit v0.4.4 ergänzt **Dateiliste prüfen** die Modwerkstatt: Steam-Depotabgleich
für Namen/Größen, EXE-Funde und Archivgruppen. Der Bericht liest keine PAZ-Inhalte
und bestätigt noch kein Vanilla. [Bedienung und Grenzen](INSTALLATION_CHECK.md).

Stand: 21.09.2026, Desktop v0.5.8. Phase 4 ist im dokumentierten Umfang entwickelt;
die Spielabnahme bleibt offen. Neue B4–B11-Funktionen und deren noch offene
Punkte stehen in [ADVANCED_MODS.md](ADVANCED_MODS.md).
Die Workbench berechnet eigene Tabellenänderungen und PAZ/PAMT-Overlays, zeigt
ihre vollständige Dateiliste und kann sie im Projekt exportieren und proben.
Die separate [Live-Anwendung mit Restore](LIVE_APPLY.md) besitzt einen kontrollierten
Schreibpfad. Einrichtung und Schreibvorgänge benötigen ein beendetes Spiel.
Vorschau und Projektproben dürfen während des Spielens laufen; Spielstände bleiben ungeöffnet.

Nach dem Spielen kann **Dateiinhalte prüfen** die komplette Installation mit den
lokalen Depothashes vergleichen. Fortschritt, Abbruch und Export sind vorhanden;
bei laufendem Spiel gesperrt. [Umfang und Grenzen](CONTENT_AUDIT.md).

## Bedienung

**Ausgangsbasis & Registry-Sicherung** übernimmt einen exportierten Inhaltsbericht
nach aktuellem Metadaten-/Registryvergleich. Bericht und geprüfte Registry-Kopie
bleiben im Projekt erhalten und können später erneut geprüft werden.
[Bedienung und Vertrauensgrenzen](BASELINE.md). Die manuellen Spieltests erfolgen
auf Nutzerwunsch erst nach Ende der Entwicklung aller Phasen; sie gelten bis
dahin als offen und blockieren die weitere Entwicklung nicht.

Unter **Sicherung & Wiederherstellung** lassen sich bestehende geschützte
Projektproben laden, deren Backups prüfen und unterbrochene Proben nach einer
konkreten Rücknahmevorschau wiederherstellen. Bereits zurückgesetzte Proben
werden entsprechend angezeigt; fehlende Sicherungen und zwischenzeitliche
Änderungen sperren den Vorgang. [Bedienung und Grenzen](BACKUP_RECOVERY.md).

1. In der Desktop-App **Modwerkstatt** öffnen.
2. Shopbestand (Vorgabe 999), täglichen Refresh, Zusatzartikel, Dropmengen-/
   Chancenmultiplikator oder Trustmultiplikator einstellen.
3. Optional Händler/Dropsets über Name oder ID eingrenzen. Keine Auswahl bedeutet
   alle freigegebenen Datensätze. Einzelausnahmen haben ausdrücklich Vorrang vor
   globalem Wert und Auswahl. Ein Faktor 1 bewahrt den Ausgangswert.
   Zusatzartikel und garantierte Dropsets haben dagegen ohne Auswahl keine Wirkung.
   Eine manuelle Garantie hat Vorrang vor Chancenmultiplikatoren; sie setzt die
   Basisrate beim Auslösen des gewählten Sets auf 100 %. Unter **Händlerdetails**
   lassen sich eigene Artikelsets sowie täglicher Refresh oder Originalintervall
   je Händler festlegen, auch außerhalb der globalen Händlerauswahl.
4. **Vorschau berechnen**. Feld, Datensatz, Vorher-/Nachherwert und Datei-Hashes
   prüfen. Nach einer Einstellungsänderung muss die Vorschau neu berechnet werden.
5. **Probe an Projektkopie** führt zweimal Apply → Archiv-Rücklesen → Reapply →
   Archiv-Rücklesen → Restore und anschließend einen absichtlichen Abbruch mit Recovery
   in einem neuen `.local/rehearsals/…`-Ordner aus. Keine Original-PAZ wird kopiert
   oder verändert; die Registry liegt dort ausschließlich als Testkopie. Das Ergebnis
   zeigt die sechs Datei-Übergänge einschließlich Hashes und entfernter Altgruppen.
   Neue v3-Proben halten Start-/Quell-/Backupsperren an eigenen Testdateien und
   prüfen die Ablehnung einer simulierten Quelländerung. Details:
   [Geschützte Projektproben](PROTECTED_REHEARSALS.md).
6. **Vorschau exportieren** erzeugt `exports/mod-preview-…/preview.json` und
   `unapplied/…` mit den erzeugten Kandidatdateien. Das ist ein nicht angewendetes,
   noch nicht im Spiel abgenommenes Overlay. Das Manifest enthält alle Änderungen,
   offenen Gates und Quellenzuschreibungen.

Die Tabelle zeigt höchstens 150 gefilterte Änderungen gleichzeitig; der Export
enthält alle. Große Spielwerte werden als Dezimalstrings übertragen. Faktoren
sind derzeit ganze Zahlen 0–1000, Shopbestände 0–1.000.000. Diese Grenzen begrenzen
die Workbench-Eingaben; sie sind **keine bestätigten Engine-Maxima**.

## Abdeckung des tatsächlichen Datenbuilds

Die bekannten EXE-/Metadaten- und Tabellenhashes werden vor der Interpretation
geprüft. Ein passendes Leseschema ist weiterhin kein Vanilla-Nachweis.

| Bereich | Implementiert | Noch offen |
|---|---|---|
| B0 | Deterministischer Byte-Builder, verschlüsseltes LZ4-Overlay, PAPGT-Registrierung, Dateivorschau; privater Kern mit Backup, unveränderlichen Transaktionsnachweisen, Reapply, bis zu acht Gruppen, Restore und Recovery; Windows-Start-/Quell-/Backupsperren, Updateablehnung, Live-Einrichtung per Steam-Nutzerbestätigung und frischen Quellhashes, Live-Apply/Restore, Originaldatenansicht nach Apply, Basiswechsel nach Updates, bereinigte Depot-Inventur und bestätigte fremde Zusätze | Tatsächliche Stromausfall-/In-game-Abnahme; unbekannte fremde Registry-/Originaländerungen bleiben gesperrt |
| B1 | Bestand von 6.376 Positionen in 397 Händlern; Auswahl und Bestandsausnahmen; Zusatzartikel/Artikelsets und individuelle Sortimente bei 208 Händlern; Refresh-Ausnahmen pro Händler; Tagesrefresh bei 369 interpretierten Intervallen (246 Änderungen) | 39 opake Händler, Ergänzungen bei Sonderhändlern, kompletter Katalog bei allen Händlern gleichzeitig, In-game-Abnahme |
| B2 | Mengenfaktoren in 12.736 Sets; Chancenfaktoren in 12.306 unabhängigen Sets, Auswahl/Ausnahmen und manuelle 100-%-Basisraten | Automatische Bosszuordnung, begrenzte/gewichtete Chancenvarianten und Sentinel-Mengen; In-game-Wirkung |
| B3 | Positive Mengen der drei `DropSet_Friendly_*`-Records; negative Werte bleiben erhalten | Wirkung/Clamping im Spiel |

Von 14.747 Dropsetrecords werden 13.035 Item-Varianten byteidentisch typisiert
rekonstruiert. Davon haben 299 leere Listen, Mengen-Sentinels oder nicht freigegebene
Min-/Max-Verhältnisse. Sie bleiben bei globalen Mengenänderungen unverändert und
werden bei expliziter Auswahl abgewiesen. Drei Friendly-Records werden separat
interpretiert, 1.709 weitere Varianten bleiben opak. Auch Rezepte und
Questbelohnungen verwenden Dropsets; eine globale Änderung betrifft diese mit.
Es gibt keine automatische Bossklassifikation.

## Feld- und Formatnachweise

**Store:** Key u16, Name mit u32-Länge; gemessene Listenpositionen relativ zum
Namensende +47 (310 Records), +95 (85), +99 (1), +115 (1). Die Workbench akzeptiert
nur genau einen passenden Kandidaten mit Besitzerkey, Itemduplikat und vollständigem
Listenroundtrip. Sie wählt weder ein Legacyformat noch einen bestpassenden Parser.
Der Bestand liegt im aktuellen Stockrecord bei +18 als u32. Der Diskriminator
bei +42 muss 1 sein, Itemkeys bei +43 und +102 müssen übereinstimmen. Nach 114
Headbytes folgen ein optionaler Block, **acht rohe September-Bytes** und eine
Liste von 12-Byte-Einträgen. Alle unbekannten Bytes, Preise, Bedingungen und
äußeren Recordteile bleiben erhalten. Die acht Bytes werden nicht als Refresh
interpretiert. Die bestätigten Refresh-/Indexfelder, Angebotsvorlagen und ihre Grenzen stehen
in [B1/B2-Feldnachweise](research/PHASE4_FIELDS.md).

**Drops:** Der bestätigte Itemzweig hat 64 Bytes pro Drop. Min/Max sind 64 Bit;
die Vorschau verweigert Sentinel-/Überlaufwerte. Die separate Chancenoption
ändert ausschließlich `rate` und die Gesamtsumme bei unterstütztem Rolltyp 0,
mit Skala 1.000.000 und Deckelung auf 100 %. Rolltyp, Rollzahl und unbekannte
Felder bleiben erhalten. Der Friendly-Zweig
hat zusätzlich 28 rohe Bytes; diese werden erhalten. Seine Min/Max-Werte werden
vorzeichenbehaftet interpretiert. Aktuelle Belege: Donate +50, Talk +5, Threat −200.
Bei ×3 ergibt die isolierte Ausgabe +150, +15, unverändert −200. Das ist ein
Byte-/Schemasnachweis, noch kein gemessener NPC-Vertrauenszuwachs im Spiel.

**Tabellenpaar:** Leere Ersetzung rekonstruiert echte Bodies und Header bytegleich.
Die allgemeine Rebuildfunktion bewahrt physische Recordreihenfolge, Präfix und
ursprüngliche Header-Keyreihenfolge und berechnet Offsets bei Größenänderungen
neu. Neben dem künstlichen Wachstumstest prüfen echte Shopergänzungen sowohl
kleine Artikelsets als auch alle 6.816 Items bei einem einzelnen Händler samt
unveränderten übrigen Records und lesbarem Overlay.

**Overlay:** Eigene Implementierung über bestehende MIT-Krypto-/Kompressionsbasis,
keine CDUMM-Laufzeit. LZ4-Blöcke, ChaCha pro korrektem Dateinamen, Alignment 16,
Jenkins-Checksummen für PAZ/PAMT/PAPGT. SHA-256 dient getrennt als Identitätsnachweis.
Die Verschlüsselungsheaderbytes und Sprachmaske stammen aus der geprüften
Quellgruppe. Die neue Gruppe steht vorne in PAPGT, Originaleinträge und Headerbits
bleiben erhalten. Die ID kollidiert weder mit registrierten/reservierten Gruppen
noch mit existierenden Ordnern. Der Reader bestätigt die erzeugten Dateien in
einer isolierten Kopie. Der Vorrang im laufenden Spiel ist damit nicht bewiesen.

## Sortiment und Auswahlgrenzen

Zusatzartikel werden an vorhandene normale Verkaufsangebote angehängt. Bereits
kaufbare Items werden nicht dupliziert. Die Vorlage stammt aus demselben Händler
und bewahrt dessen Preisfaktoren. Bestehende Waren/Save-Indizes bleiben erhalten;
neue Indizes folgen auf das bisherige Maximum. Eine reine Ankaufsposition
verhindert ein neues Verkaufsangebot nicht. Der Bestand neuer Angebote folgt
der Händlerausnahme, dem globalen Wert oder standardmäßig 999.

„Alle Artikel auswählen“ bezieht sich auf 6.816 Items. Ein Plan erlaubt maximal
100.000 Kombinationen aus unterstütztem Händler und Item, ein Händler maximal
9.999 Positionen. Für den vollständigen Katalog deshalb einzelne Händler wählen.
Diese Grenzen schützen die Vorschau und sind keine nachgewiesenen Engine-Maxima.
Explizit ausgewählte ungeeignete Händler/Dropsets führen zu einer Fehlermeldung.
Globale Optionen erhalten nicht unterstützte Varianten; die Abdeckung bleibt sichtbar.

**Händlerdetails:** Eine eigene Artikelauswahl ersetzt für diesen Händler die
gemeinsame Zusatzartikelauswahl. Eine leere eigene Liste fügt dort nichts hinzu.
„Globale Einstellung“ bzw. deaktivierte eigene Auswahl erbt wieder Wert und
Geltungsbereich der globalen Auswahl. Explizite Ausnahmen gelten auch außerhalb
der ausgewählten Händler. „Originalintervall behalten“ unterdrückt den globalen
Tagesrefresh, ohne ein neues Intervall zu erfinden. Bestandsausnahmen bleiben
separat einstellbar. Die Planbegrenzung zählt die tatsächlich gewählten Itemsets.

## Transaktionsprobe und Freigabegrenze

Der Kern ist über geschützte Projektproben und seit v0.4.8 über den separaten
[Live-Adapter](LIVE_APPLY.md) erreichbar. Die Herkunftsbestätigung ist explizit;
beobachtete Hashes allein genügen nicht. Fehler bei der Windows-Prozessprüfung
verweigern den Vorgang. Die nachfolgenden Absätze beschreiben auch die älteren
Projektproben und gelten nicht als Nachweis eines tatsächlichen Spieltests.

Neue v0.4.3-Proben verwenden zusätzlich ein hashgebundenes Schutzmanifest sowie
gehaltene Windows-Datei-/Ordnerhandles und Prozesspfadprüfungen. Die Sperren
betreffen ausschließlich eine Kopie unserer eigenen EXE und eine synthetische
Quelle; kein Handle sperrt während der Probe das laufende Spiel. Nach Erstellung
und Hashprüfung wird auch das Registry-Backup gegen Änderungen gehalten.

Seit v0.4.1 hält die Probe einen exklusiven Kopie-Lock und ein verifiziertes
Registry-Backup. Jede Transaktion schreibt zuerst einen unveränderlichen
`intent.json` mit Vorher-/Nachherzustand. Beide Overlaydateien entstehen in einem
privaten Stagingordner; nach Prüfsummen-, Dekompressions- und Längenprüfung wird
die vollständige Gruppe veröffentlicht. Unter Windows erfolgen Veröffentlichung
und Registry-Ersetzung mit `MoveFileExW` und Write-through.

Reapply reserviert neue Gruppen-IDs und baut immer von der ursprünglichen Basis.
Erst nach Bereitstellung aller neuen Gruppen wird die Registry atomar ersetzt;
erst danach werden nachweislich eigene Altdateien entfernt. Der private Kern
unterstützt bis zu acht Gruppen und verweigert doppelte virtuelle Tabellenpfade.
Die aktuellen B1–B3-Module benötigen weiterhin nur eine Gruppe.

Recovery entscheidet anhand der tatsächlichen Registry zwischen Vorher- und
Nachherzustand. Ein unveränderlicher `complete.json` bindet den Abschluss an den
Hash des Intents. Unvollständige private Stagingdateien bleiben zur Diagnose
erhalten; bekannte Teilstücke von Backup und Abschlussnachweis können weiter-
geschrieben werden. Fremde Änderungen, fremde Gruppeninhalte, Kollisionen,
Reparse-Points, Hardlinks oder abweichende Registry/Backups verhindern weitere
Änderungen. Nur eigene Dateien werden entfernt, ohne rekursives Löschen.

Tests unterbrechen an **69 Apply-/Reapply-/Restore-Schreibgrenzen**, einschließlich
halb geschriebener Dateien, und öffnen den Kern zur Recovery neu. Auch Abbrüche
während Recovery werden wiederaufgenommen. Das beweist diese Ablaufgrenzen,
**keine vollständige Robustheit gegen beliebige I/O-Ausfälle oder Stromausfall**.
Die Historie bleibt erhalten; es gibt noch keine automatische Archivbereinigung.
Der frühere v1-Kern bleibt unverändert als Testreferenz erhalten. Alte v1-Proben
werden nicht migriert; für v2 wird eine neue Projektprobe erstellt.
Auch der Test, dass ein exklusiver Windows-Dateihandle einen parallelen Start
verhindert, läuft ausschließlich mit einer Kopie unserer eigenen Test-EXE.
Seit v0.4.3 ist dieser Mechanismus zusammen mit einer vollständigen Prozesspfad-
prüfung integriert. Live-B0 erfasst alle EXEs und Originaldateien des vollständigen
Prüfberichts; tatsächlich beobachtet wurden drei EXEs. Beliebige externe oder
umbenannte Startkopien sind damit nicht als geprüft behauptet.

Eine fremde Änderung/Spielaktualisierung wird nicht mit einem alten Backup
überschrieben. Die Live-Anbindung wird an künstlichen Installationen getestet; der echte Spieltest bleibt offen.

## CLI

```powershell
cargo run -p cd-cli -- mod-info
cargo run -p cd-cli -- mod-preview .local/mod-request.json --output exports/mod-plan.json
cargo run -p cd-cli -- mod-rehearse .local/mod-request.json
cargo run -p cd-cli -- mod-export .local/mod-request.json
cargo run -p cd-cli -- mod-recover .local/rehearsals/ORDNER-ID
```

`mod-recover` akzeptiert ausschließlich markierte v2-/v3-Proben unter
`.local/rehearsals/<id>` desselben Projekts. Es schließt eine offene Transaktion
ab, stellt anschließend die ursprüngliche **Kopie** wieder her und speichert
einen neuen Recovery-Bericht. Es benötigt keinen aktuellen Tabellenparser;
geschützte Spiel-/Savepfade und unmarkierte Ordner bleiben ausgeschlossen.
V3 verlangt zusätzlich den passenden Schutzmanifesthash und unveränderte
Quelldateien. V2 wird nicht automatisch aufgewertet; ein v3-Downgrade wird abgelehnt.

Beispiel für eine Konfiguration (alle anderen Werte sind unverändert):

```json
{
  "shop_stock": 999,
  "vendors": [3101],
  "vendor_overrides": { "3201": 50 },
  "vendor_options": {
    "3101": { "items": [2200], "daily_refresh": false }
  },
  "daily_refresh": true,
  "shop_items": [2200, 50001],
  "quantity_multiplier": 3,
  "chance_multiplier": 2,
  "dropsets": [175521],
  "chance_overrides": {},
  "guaranteed_dropsets": [],
  "trust_multiplier": 3
}
```

Unbekannte Felder, nicht freigegebene explizite IDs und Werte außerhalb der Grenzen
werden abgewiesen. Die Requestdatei ist auf 1 MiB begrenzt. Ausgaben landen
ausschließlich in `.local/` oder `exports/`; das Originalspiel bleibt geschützt.

## Nächste notwendige Arbeiten

Unabhängig bestätigte vollständige Vanilla-Hashes und Backup-/Updatepfade sind
Voraussetzung für einen Live-Schreibpfad. Ein lokal gecachter Steam-Manifestname
allein zertifiziert diese Basis nicht. Danach sind umfassendere Transaktions-/
Startschutztests und die manuelle In-game-Abnahme aus [TESTING.md](TESTING.md)
nötig. Beide untersuchten lokalen Steam-Depotmanifeste enthalten keine
Signaturdaten; ihr Hashbestand allein ist kein unabhängig bestätigter Nachweis.
B1/B2 sind im beschriebenen Umfang implementiert, die universelle Händlerabdeckung
und Gameplayabnahme bleiben offen. Eine Steam-Prüfung oder ein Spielstart wird
während des Spielens nicht ausgelöst.
