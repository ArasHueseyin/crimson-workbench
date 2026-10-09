# Eigene Reparaturaktion – Integrationsvertrag

Stand 22.09.2026, Entwicklungsmodul 0.10.0. Der Nutzer hat ausdrücklich eine
eigene Reparaturaktion für noch vorhandene, beschädigte Items freigegeben.
Die Aktion benötigt keine Item-Reparaturregel und kein Reparaturmaterial.
Verbrauchte/verschwundene Items werden nicht erzeugt. Direkte Save-Schreibzugriffe
bleiben ausgeschlossen; spätere Persistenz muss über das Spiel erfolgen.

## Entwickelter Umfang

`repair_action.h` / `src/action.cpp` enthalten die eigene Reparaturplanung,
ein vollständiges Backend für **eigene Speicherabbilder** und die
Befehlswarteschlange. Die statische Bibliothek `crimson_repair_action.lib`
wird im isolierten Testhost verwendet. Sie ist noch nicht mit dem Spiel verbunden.

- Einzelitem, mitgeführte Items, ausgerüstete Items oder beide Bereiche.
- Hauptitem und jeder vorhandene Sockeleinsatz werden unabhängig auf das
  jeweilige belegte Maximum repariert, auch bei aktueller Haltbarkeit 0.
- Identität, Inventarposition, positive Menge und beide Zustandskopien müssen
  passen. Leere Slots, verbrauchte Items und nicht passende Kopien werden abgewiesen.
- Werte ohne Haltbarkeit, echte unbegrenzte Definitionen und Instanz-Sentinels
  bleiben erhalten. Finite Maxima über 32.767 werden wegen der signed-16-Bit-
  Engine-Arithmetik nicht akzeptiert. Dies ist kein nachgewiesenes Gesamtlimit.
- „Kein Haltbarkeitsverlust“ ist kombinierbar: Ein verifiziertes ursprüngliches
  Maximum und der bekannte aktive 65.535-Override erlauben die Reparatur auf
  das ursprüngliche Maximum. Ein unerklärter Unterschied wird abgewiesen.
- Nur die Haltbarkeitswörter `Item+0x40` und `Sockel+2` können sich ändern.
  Pointer, Menge, ID, Verzauberung, Sockelbelegung/-zahl und Flags bleiben erhalten.
- Der gesamte Batch wird vor dem ersten Schreibzugriff erneut verglichen.
  Änderungen an einem späteren Item lehnen auch vorherige Änderungen ab.
- Doppelte Eingaben werden zusammengefasst; jeder Auftrag läuft höchstens
  einmal. Er verfällt nach fünf Sekunden und darf keinen Welt-/Charakterwechsel
  überleben. Ausführung ausschließlich auf dem festgelegten Dispatch-Thread.
- Ein unbestätigter Commit sperrt die Warteschlange dauerhaft; kein automatischer
  Wiederholungsversuch. Der Host muss nach Aufbau einer neuen, geprüften Sitzung
  eine neue Warteschlange anlegen.

Die Limits 2.048 Items und 64 Sockelplätze sind explizite Verarbeitungsbudgets,
keine behaupteten Engine-/Save-Kapazitäten. Ein unbekanntes Layout wird abgewiesen.

Seit 0.3.0 liefert `crimson_repair_reader.lib` diese Abbilder aus der aktuellen
Client-/Server-Struktur: gezielte Charakterkennungssuche, Besitzerprüfung,
Character-Inventar und Equipment, UID-/Positionsabgleich und vollständiges
Kontrolllesen. Native Inventargetter stimmen auf gemeinsamen Fixtures überein;
die Server-Registry ist mit künstlichen Lock-/Zugriffsmethoden geprüft.
Dieser lesende Teil ist **noch keine Engine-Lease oder Schreibtransaktion**.
[Implementierte Pfade, Belege und Grenzen](READER_LAYOUT.md).

Seit 0.4.0 kann die Transaktion `pending` melden. Die Queue hält den Plan bis
zur sitzungsgebundenen Bestätigung aller Nachherwerte und Engine-Ereignisse.
Fremde/alte Rückmeldungen werden abgewiesen. Teilfehler, Weltwechsel und Timeout
sperren Folgeaufträge ohne Wiederholung oder Rücksetzung. Ein nativer Client-
Fehlerpfad kann Sockel bereits entfernt haben; deshalb ist Fehler != unverändert.
[Implementierter Abschlussweg und native Befunde](ACK_INTEGRATION.md).

Seit 0.5.0 erfasst `capture_with_locks` die Daten unter beiden vom Host
gebundenen Besitzer-Sperren. Diese werden bei jedem Rücksprung freigegeben.
Originale Lock-Methoden sind mit echten Windows-SRW-Sperren im privaten Testhost
geprüft. Der Host muss weiterhin die Lebensdauer der Besitzer unabhängig sichern
und die Methodenbindungen validieren. [Sperrvertrag](LOCK_INTEGRATION.md).

Seit 0.6.0 hält `capture_with_references` zwei native Referenzbelege bis nach
Freigabe der Inventarsperren. Kennung und Alive-/Zerstörungszustand werden unter
den Sperren vor und nach Erfassung geprüft. Der Host muss noch den Erwerb aus
geschützten Quellen und unveränderten Engine-TLS-Modus gewährleisten.
[Implementierte Referenzverwaltung und native Belege](REFERENCE_INTEGRATION.md).

Seit 0.7.0 bindet `crimson_repair_registry.lib` Manager-Lookup und Belegfreigabe
als Quelle an. Acquire/Release verwenden denselben Quellenkontext.
[Vertrag und damalige Updategrenze](REGISTRY_INTEGRATION.md).

Seit 0.8.0 besteht die gemeinsame native Client-/Server-Suche samt konkreten
Referenzmethoden und Freigabe auf 1.0.0.2949. Echte Windows-Registry-Sperren
schützen dabei private Testobjekte. Aktuelle Manager-/Spieler-/Threadbindung
und weitere Reparaturmethoden sind weiterhin offen; der alte native Host
akzeptiert die neue EXE nicht. [Gemeinsamer Nachweis und Grenzen](REGISTRY_NATIVE_INTEGRATION.md).

## Anforderungen an den noch fehlenden Spieladapter

Seit 0.9.0 durchsucht die geschützte Erfassung keine Registry erneut, sondern
liest direkt aus dem gehaltenen Besitzerpaar. Gültige vollständige Kennung und
Alive-/Zerstörungsprüfungen sind auch für `capture_with_locks` zwingend. Native
Erwerbs-/Sperrmethoden von 2949 sind gemeinsam mit dem Leser auf privaten
Legacy-Layoutfixtures geprüft. Das ist kein Nachweis des neuen Inventarlayouts.
[Gemeinsame Erfassung und offene Hostpflichten](PINNED_CAPTURE_INTEGRATION.md).

Der abstrakte `Transaction`-Vertrag ist **kein implementierter Spieladapter**.
Seit 0.10.0 sind die Lesefelder und nativen Inventar-/Ausrüstungszugriffe für
2949 separat belegt; Leseraufrufe benötigen einen expliziten bekannten Build.
Die echte Manager-/Threadbindung und Änderungstransaktion bleiben offen.
[Nachweis und Reichweite](INVENTORY_2949.md).

Sein Commit muss alle folgenden Aufgaben selbst erfüllen, bevor die Funktion
installierbar wird:

1. Den entwickelten lesenden Resolver mit echten Engine-Zugriffsfreigaben verbinden
   und seine Client-/Server-Zuordnung für tatsächliche Spielzustände bestätigen.
   Keine NPC-, Händler-, Lager- oder temporären Inventarplanerkopien.
   Welt-/Charakterwechsel invalidieren die Sitzungskennung und ausstehende Aufträge;
   die Epochennummer muss der Host bei jedem Übergang erneuern.
2. Vollständigen EXE-Build, die aufgerufenen Funktionen und aktuelle Tabellen
   prüfen. Die ursprünglichen Itemdefinitionen müssen aus dem über B0 zugelassenen
   Vanilla-Bestand stammen. Der aktuelle lokale Bestand ist dafür noch nicht
   zugelassen. Keine Adressübernahme aus einer älteren Community-Version.
3. Zugriff und Änderungen in einer belegten Engine-Transaktion serialisieren.
   Ein eigener C++-Mutex allein sperrt keine Engine-Threads. Der Input-/Render-
   Thread darf nur einen Auftrag einstellen, keine Inventaränderungen ausführen.
4. Gegen frisch aufgelöste Item-IDs und Vorherwerte prüfen, beide Repräsentationen
   korrekt aktualisieren und die Engine-Benachrichtigungen/abgeleiteten Effekte
   auslösen. Ein bloßer Speicherwrite oder ein späterer UI-Refresh genügt nicht.
5. `applied` erst nach bestätigtem Abschluss melden. `rejected` verspricht keinerlei
   Änderung. Bei unklarem/teilweisem Ergebnis `unknown`; keine spekulative
   Rücksetzung nach einem möglicherweise schon ausgelösten Engine-Ereignis.
   Alternativ `pending` und anschließend `Queue::confirm`: echte Meldungen mit
   Auftragsnummer/Sitzung zuordnen, frische Daten unter neuer Engine-Lease erfassen
   und alle notwendigen Ereignisse bestätigen. Diese Spielereigniszuordnung fehlt.
6. Eigene Bedieneingabe anbinden, Loader nur beim nächsten Spielstart aktivieren
   und Installation/Rücknahme über B0 inklusive Konflikt-/Updateprüfung abwickeln.

`apply_to_owned_snapshot()` darf nicht durch Umdeuten fremder Speicheradressen
als vermeintlicher Spieladapter eingesetzt werden. Es mutiert ausschließlich
seine explizit besessenen Vektoren/Arrays und löst keine Engine-Ereignisse aus.
Die Binärdateien im privaten Buildordner gehören nicht in den Spieleordner.

## Beleg im aktuellen Build

EXE 1.0.0.2944, SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Alle Adressen als RVA:

| Funktion / Feld | Nachweis |
|---|---|
| Item-Updater `0x240d650`, 405 Bytes | SHA-256 `cdbf73177286a1465e808e462c5465c06756ab9910183eb09c2321ee5a195a3f`; liest Hauptwert `+0x40`, Definition `+0x400`, Sockeldaten `+0x60`, Count `+0x68`, Kapazität `+0x70`, Sockelstride 6 |
| Equipment-Wrapper `0x20c90e0` | Prüft vorhandenes Item; aktualisiert anschließend weitere Komponenten über virtuelle Methoden. Tabellenzeiger `+0x90`, Array `+8`, Count `+0x10`, Eintragsstride `0xd0`, Slottag `+0xc8` |
| Client-Aufrufer `0x98fb70` | Verarbeitet Haltbarkeitsdelta und Sockelliste; daran hängt weitere UI-/Zustandsaktualisierung |
| Alter Inventarreparaturpfad `0x2be41a0` | Materialauswahl, Transaktion, `repairItemEndurance`-Datenbankoperation, danach Instanzwrite bei `0x2be47f5` |

Der unveränderte Updater ist **kein allgemeiner RepairAll-Helfer**: Im nativen
Test repariert Delta +65 das Hauptitem von 35 auf 100, lässt einen 0/100-Sockel
bei 0 und setzt einen 10/100-Sockel auf 0. Die eigene Aktion setzt jeden Wert
unabhängig korrekt auf 100. Danach bestätigt der Originalcode mit Delta 0
unveränderte Werte und mit normalem Verschleiß -1 den Wert 99 bei allen drei
Feldern. Mit dem bekannten No-Wear-Sentinel bleiben alle drei bei 100.
Alle Aufrufe erfolgen nur im eigenen Speicher des Testhosts, mit künstlichen
Definitionen. Damit ist die Feldnutzung geprüft, nicht die komplette Spieltransaktion.

Die MIT-Referenz [Trinity, Commit 70c9a00](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b)
liefert Hinweise auf getrennte Server-/Client-Kopien. Ihre älteren Layoutwerte
`0x88/0xc8/0xc0` passen **nicht** zum hier nachgewiesenen Equipment-Layout
`0x90/0xd0/0xc8`. Kein Code, Hook oder Loader daraus wurde übernommen.

Tests und Reproduktion: [README](README.md).
