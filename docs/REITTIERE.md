# Reittiere auswählen und im Stall registrieren

Stand 04.10.2026: Erweiterung für zusätzliche Tierarten fertig gebaut und an
privaten Savekopien geprüft und bei geschlossenem Spiel/Workbench installiert.
Desktop-EXE, 37 unveränderte Dateien einschließlich aller 13 Saves, 23 Sicherungen
und die bestehende Desktop-Verknüpfung sind unabhängig zurückgeprüft. Die neue
Route erzeugt einen **Basiseintrag** aus dem Schema des ausgewählten Saves.
Sie ist als **Spieltest offen** gekennzeichnet: Die Strukturprüfung bestätigt
keine erfolgreiche Initialisierung, Sichtbarkeit, Beschwörung oder Reitfunktion
im Spiel. Die ältere Erweiterung für zugewiesene Vorlagentiere und Porträts ist
bereits installiert.

**Neue Basiseinträge:** Auch ohne ein gespeichertes Tier derselben Familie
werden Rotfederraptor, Elefant, beide Riesenleguane, Wölfe, erwachsene Kamele
und weitere normale Reittierfamilien auswählbar. Die geprüfte private Kopie
von Konto <Konto-ID>, Slot 0 zeigt 288 statt 219 hinzufügbare Einträge. 24
separate verschlüsselte Kandidaten, darunter alle 13 erwachsenen
Kamelvarianten, bestehen vollständigen Rückvergleich, neue Nummern und
Doppelsperre. Kamelkälber und Nutzvieh erhalten keinen Basiseintrag.

Der Basiseintrag enthält eine frische Tiernummer, den ausgewählten
Charakterschlüssel, leere Level-/Tageszählerstrukturen, Kliffs Besitzkennung
und ein explizit falsches Initialisierungsflag. Die Initialisierung durch das
Spiel ist beabsichtigt, aber noch nicht beobachtet. HP, Ausrüstung, Spawnposition,
Namen, Arbeitszustände und aktive Beschwörungsflags eines anderen Tiers werden
nicht übernommen. Es werden weder alte 170-Byte-Vorlagen noch fremde Klassen-
oder Feldindizes eingespielt. Die 44 aktuellen Mercenary-Felder und die beiden
verschachtelten Klassen werden nach Namen und Typ geprüft; abweichende Layouts
sperren die Route. Eine bereits passende gesunde Familienvorlage hat Vorrang
und behält den bisherigen Klonweg.

**Noch zu prüfen:** Zunächst nur eine gewünschte exotische Variante registrieren,
denselben Save laden und Stall/Sonderreittiermenü, Herbeirufen und Reiten prüfen.
Erst nach erfolgreichem Speichern und Neuladen weitere Varianten ergänzen.
Eine Sicherung von Save und Lobby wird vor jeder Registrierung angelegt.
Quests, Tierwissen und Reitfähigkeiten werden nicht zusätzlich freigeschaltet;
solche Voraussetzungen oder nicht reitbare Tiermodelle können eine Nutzung
weiterhin verhindern. Die Desktop-Installation selbst vergibt keine Tiere.
Nachweise: `.local/mount-family-investigation-20261004/`.

In der Workbench unter **Reittiere** den gespeicherten Spielstand auswählen,
nach Name oder ID suchen und eine Tierart wählen. Die Auswahl zeigt Beschreibung,
Besitzstatus und ob eine Registrierung für diesen Spielstand möglich ist.
**Im Stall registrieren** fügt genau ein Tier hinzu. Das Spiel muss dazu
vollständig geschlossen sein; für eine Live-Stallregistrierung gibt es noch
keinen geprüften nativen Aufruf.

**Vorherige Erweiterung 04.10.2026, installiert und unabhängig geprüft; Spielabnahme steht aus:**
Die Anzeige nennt die hinzufügbaren Tiere, bereits registrierte Tierarten und
fehlende Vorlagen. Familienfilter zeigen die verfügbare Anzahl. Mit **Nur
hinzufügbare Reittiere** werden ungeeignete Einträge ausgeblendet. Bei einem
älteren Konto ohne Vorlage hilft **Neuesten Spielstand auswählen**.
Eine gesunde Vorlage darf einen Namen tragen und einem Charakter zugewiesen
sein. Nur bei der neuen Kopie werden Name, Charakterzuweisung, letzter
Beschwörungsstatus und Hauptreittierstatus weggelassen. Die bestätigten Typen
und Präsenzbits werden geprüft. Das Original samt Name und Zuweisung bleibt
unverändert. Fremde Aufgabenfelder, Fütterungstimer, fehlende Lebenspunkte und
unbekannte Feldformate sperren eine Vorlage weiterhin.
In der neu geprüften Kopie von Konto <Konto-ID>, Slot 1 sind 219 Tiere hinzufügbar
(210 Pferde, sieben Bären, zwei Löwen). Für andere Familien fehlt dort weiterhin
eine geeignete Vorlage.
Der dort zugewiesene Zirkuslöwe ist nun eine geeignete Vorlage für zwei weitere
Löwen. Die vorherige Version zeigt in diesem Stand nur 217 verfügbare Arten.
Die Verfügbarkeit wird für den jeweils gewählten Save neu ermittelt. Für eine
fehlende Familie zuerst ein gesundes Tier im Spiel registrieren und speichern;
auch dann muss dessen Datensatz die geprüften Vorlagenregeln erfüllen.
Nach der Installation wurde der neue zuletzt gespeicherte Stand rein lesend
geprüft: Konto <Konto-ID>, Slot 0, mit denselben 219 verfügbaren Arten
(210 Pferde, sieben Bären, zwei Löwen). Es wurden dabei keine Tiere vergeben.

89 Tierporträts sind über den vollständigen internen Charakternamen eindeutig
zugeordnet; zusätzliche Archiv-Namensschemata werden erkannt. 210 weitere
Varianten zeigen ausdrücklich beschriftete Beispielbilder derselben Tierart.
Fell, Farbe und Ausstattung können abweichen. Die Auswahl verwendet dafür
bestätigte Arten-/Rassenpräfixe, keine pauschale Fahrzeugfamilie: Ein Hirsch in
der Pferde-Fahrzeugfamilie bekommt beispielsweise kein Pferdebild.
299 der 322 Einträge haben damit ein aus dem Spielarchiv dekodiertes Bild.
Sichtbare Listeneinträge laden ihre Bilder bei Bedarf; gemeinsame Texturen
werden einmal dekodiert und ohne Variantenbeschriftung zwischengespeichert.
23 Einträge haben weiterhin keine passende Zuordnung. Bei mehrdeutigen
gleichrangigen Zuordnungen bleibt
ein ausdrücklich als allgemein beschriftetes Tiersymbol. Das große Bild rechts
lässt sich anklicken und mit **Esc** oder **×** wieder schließen. Namen, Tierart
und ID lassen sich weiterhin suchen; **Regex-Suche** ergänzt optionale Suchmuster.

Der Katalog enthält 322 normale Reittiere des geprüften Builds 2976. Nur Tiere
mit einer bestätigten Fahrzeugfamilie und entweder einer passenden gesunden
Vorlage oder dem geprüften aktuellen Basiseintrag-Layout sind hinzufügbar. Kanonen, Wagen,
Sequenzobjekte und Sonderbeschwörungen werden ausgeschlossen. Bereits vorhandene
Tierarten können nicht doppelt hinzugefügt werden. Die Verfügbarkeit hängt vom
gewählten Spielstand ab. Die Vorlage liefert zunächst Level, Lebenspunkte und
Ausrüstung; individuelle Eigenschaften werden nicht erfunden.

Vor Änderungen werden `save.save` und `lobby.save` exklusiv gelesen, ihr HMAC
geprüft und vollständig bytegenau neu serialisiert. Ein neues Stalltier erhält
frische Tier- und Itemnummern. Danach muss das Entfernen dieses einen Tiers aus
dem Kandidaten wieder exakt den ursprünglichen Savekörper ergeben. Der Lobby-
Nummernzähler wird passend erhöht. Keine vorhandenen Quests, Fähigkeiten oder
Inventare werden bearbeitet. Unbekannte Objektbereiche, beschädigte Dateien,
fremde Änderungen, Verknüpfungen und nicht unterstützte Builds führen zur
Ablehnung. Die Spiel-EXE bleibt während der Transaktion gegen einen Start gesperrt.

Originale, Kandidaten, Absicht und Ergebnis liegen unter
`.local/mount-grants/<Anfrage-ID>/`. Die Workbench zeigt den Backupordner im
Ergebnis. Bei verlorener Antwort dieselbe gespeicherte Anfrage prüfen; keine
neue Anfrage auf Verdacht senden. Eine fertige Anfrage-ID kann nicht erneut
ein Tier erzeugen. Bei Fehler zwischen beiden Dateiwechseln werden nur die
geprüften Dateigenerationen zurückgesetzt; fremde Änderungen bleiben erhalten.

Nachweise der Erweiterung liegen in `.local/mount-catalog-expansion-20261004/`.
Dabei wurden keine echten Saves bearbeitet und keine Tiere vergeben.
Geprüft wurden synthetische Transaktionen einschließlich Zwischenfehler,
fremder Änderung und Pfadablehnung sowie echte private Savekopien mit je einem
neuen Bären, Pferd und Löwen, vollständiger Rückvergleich und erneutes Ablehnen derselben
Tierart. Die Stallanzeige, Reiten, Speichern und Neuladen im Spiel sind noch offen.
