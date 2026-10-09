# Reparatur-Laufzeitprototyp

Stand 23.09.2026, Entwicklungsmodul 0.23.0. Die separate Laufzeitentwicklung und
eine eigene Reparaturaktion für vorhandene beschädigte Items sind ausdrücklich
freigegeben. **Noch kein installierbarer Reparaturmod.** Die Workbench bleibt
v0.5.9; ihr Reparaturschalter bleibt gesperrt.

**Neuer Build erkannt:** Steam 25455892 / EXE 1.0.0.2949 ist installiert und
von der App für Tabellen unterstützt. Eine neue, getrennt gepinnte native
Registry-/Referenzprobe besteht auf dieser EXE. Der historische Reparaturhost
bleibt ausschließlich an 25381195 / 1.0.0.2944 gebunden und verweigert die neue EXE.
[Getrennte Buildnachweise](../../docs/BUILD_SUPPORT.md).
[Aktuelle rein lesende native Zuordnung](BUILD_2949.md).

Neu in 0.23.0: Originaler Inventar-Paketserializer und Streamauswahl angebunden.
36 neue Fälle, davon zehn Verbindungen zwischen Transaktion, Sender und Client-Ack.
Insgesamt 454 native Szenarien und neun CTest-Suiten bestanden. Das Paket enthält
keine UID-/Sockel-/Auftragsdaten; frische Prüfung und Zustellbeleg bleiben nötig.
Transport und Server-/Speicherabschluss sind weiterhin eigene Testempfänger.
[Aktueller Nachweis und Grenzen](INVENTORY_PACKET_2949.md).

Historischer Stand 0.22.0: Originale Pufferaufbereitung und LZ4-Kompression mit privatem
Heap angebunden. 66 weitere Fälle, insgesamt 418 native Szenarien und neun
CTest-Suiten bestanden. Wiederholte Einträge nach Öffnungsfehlern können doppelt
komprimiert werden. Nachgelagerte Verarbeitung und Dateiempfänger bleiben
Testcallbacks; tatsächliche Speicherung und Spielhost sind weiterhin offen.
[Aktueller Nachweis und Grenzen](SAVE_ENCODING_2949.md).

Historischer Stand 0.21.0: Originaler Dateischreibhelfer und Pufferbereinigung auf privaten
Objekten ausgeführt. 34 neue Fälle, insgesamt 352 native Szenarien und neun
CTest-Suiten bestanden. Selbst ein fehlgeschlagener Flush kann zu Erfolg und
geleertem Payload führen. Datei-/Kodierungsschnittstellen bleiben Testcallbacks;
tatsächliche Speicherung und Spielanbindung sind offen.
[Aktueller Nachweis und Grenzen](SAVE_FILE_2949.md).

Historischer Stand 0.20.0: Originaler Speicherdispatcher, Steam-Zulassungsprüfung und
Eintragsvormerkung auf privaten Daten geprüft. 39 zusätzliche Fälle, insgesamt
318 native Szenarien und neun CTest-Suiten bestanden. Übersprungene Aufträge
können Erfolg melden und alte Statuswerte behalten. Der echte Dateiweg und die
Timer sind statisch zugeordnet; tatsächliche Speicherung bleibt offen.
[Aktueller Nachweis](SAVE_BACKEND_2949.md).

Historischer Stand 0.19.0: Originale Item-/Speicherobjekt-Konverter samt Konstruktion,
Vektorwachstum und Freigabe auf privaten Objekten ausgeführt. 38 neue Fälle,
darunter 14 mit produktiver Reparaturplanung/native Nachherkopie, prüfen Haupt-
und Sockelwerte, No-Wear-Normalisierung, freie Plätze, Färbedaten sowie Reset und
Wiederverwendung. Neun CTest-Suiten und insgesamt 279 native Szenarien bestanden.
Der reguläre Speichervorgang und die Spielanbindung bleiben offen.
[Aktueller Nachweis und Grenzen](ITEM_SAVE_2949.md).

Historischer Stand 0.18.0: Vollständige SQL-Ausführungsfunktion und Request-Erwerb isoliert
geprüft. Die Funktion führt keinen Datenbankauftrag aus; ihr Umgehungszweig
meldet Erfolg, ohne den Request zu lesen. 19 neue native Fälle einschließlich
Slot-Verbraucher belegen auch erhaltene alte Fehler. Neun CTest-Suiten und
241 native Szenarien bestanden. Tatsächliche Speicherung ist weiterhin offen.
[Nachweis und Grenzen](SQL_DISPATCH_2949.md).

Historischer Stand 0.17.0: Originaler Inventar-Client-Ack in den privaten gemeinsamen Ablauf
eingebunden. 36 neue native Fälle prüfen absolute Hauptwerte, Slotfehler, UID-Wechsel
und fehlende/doppelte Rückmeldungen. Der Aufruf besitzt keine eigene UID- oder
Zielwertprüfung und ersetzt keine Server-/Speicherbestätigung. Neun CTest-Suiten
und 222 native Szenarien bestanden. Der untersuchte Socket-SQL-Helper überträgt
Itemkennungen; eine Haltbarkeitsspeicherung ist damit nicht belegt.
[Aktueller Nachweis und Grenzen](INVENTORY_EVENTS_2949.md).

Historischer Stand 0.16.0: Ein gemeinsamer Abschluss folgt nach allen Itemmeldungen. Die
Ausrüstungsprobe verarbeitet dabei die markierten Slots mit dem originalen
Verbraucher und leert bzw. zerstört ihre privaten Tabellen mit Originalfunktionen.
Fehler bei der Weitergabe an die Persistenzschnittstelle bestätigen keinen Erfolg,
auch wenn die UI-Meldung bereits angekommen ist. Neun CTest-Suiten, 43 Ablauf-
szenarien und 186 native Szenarien bestanden, darunter 24 neue Verbraucherfälle.
Tatsächliche Persistenz und ihr ergänzender Sockelweg, konkrete Inventarereignisse,
echte Empfänger/Rückmeldungen sowie der Spielhost bleiben offen.
[Aktueller Nachweis und Grenzen](PERSISTENCE_2949.md).

Historischer Stand 0.15.0: Gemeinsamer Ablauf mit vollständiger Vorprüfung aller Meldungswege
und nativen Nachherkopien vor dem ersten Quellschreiben. Feldschreiber, Slot-Markierung
und Ausrüstungsereignisse sind in den 15 nativen Ereignisfällen verbunden.
Leere/teilweise gespeicherte Sockelplätze und Zusatzvektor-Strides korrigiert.
Neun CTest-Suiten, 40 Ablaufszenarien und 159 native Szenarien bestanden.
Konkrete Inventarereignisse, Slot-Verbraucher, echte Empfänger und Spielhost fehlen.
[Aktueller Nachweis und Grenzen](TRANSACTION_2949.md).

Historischer Stand 0.14.0: Vollständiger Feldschreiber für mitgeführte und ausgerüstete Items
unter gehaltenen Besitzersperren. Gesamte Vorprüfung vor dem ersten Store,
Überlappungs-/Kapazitätsprüfung und Kontrolle des ganzen Lesebestands danach.
Acht CTest-Suiten, 34 Schreibszenarien und 141 native Szenarien bestanden.
Die Verbindung mit nativen Nachherkopien/Ereignissen zur vollständigen
Engine-Transaktion und der Spielhost fehlen weiterhin.
[Aktueller Nachweis und Grenzen](FIELD_WRITER_2949.md).

Seit 0.13.0: Gehaltene Erfassung für Planung und erneute Vorprüfung unter
denselben Besitzerreferenzen und Sperren. Originale Markierung geänderter
Ausrüstungsslots samt Kollisionen und Wachstum in den Ereignistest integriert.
Sieben CTest-Suiten, 519 Leserbedingungen und 133 native Szenarien bestanden.
Der vollständige Schreiber, Verbraucher der Slot-Meldungen und Spielhost fehlen.
[Aktueller Nachweis und Grenzen](DIRTY_SLOTS_2949.md).

Seit 0.12.0: Native Itemwerte mit fest gebundener Konstruktion, tiefer Zuweisung
und Freigabe. Sockel-/Zusatzdaten besitzen unabhängige Allokationen; fehlerhafte
Header werden vor der Zieländerung abgewiesen. Die Ereignisprobe nutzt jetzt
originale Itemkopien. Sieben CTest-Suiten, 30 Besitzerbedingungen und 117 native
Szenarien mit 851 Aufrufen und 13.639 Bedingungen bestanden. Vollständiger
Schreibweg, Slot-Invalidierung, Ereignisverarbeitung und Spielhost bleiben offen.
[Aktueller Nachweis und Grenzen](ITEM_LIFECYCLE_2949.md).

Seit 0.11.0: Ereignismetadaten aus dem Reparaturplan, mit aktivem No-Wear und
vorherigem Broken-Zustand. Originaler Server-Notifier und Client-Ack gemeinsam
auf privaten reparierten Items geprüft, einschließlich fehlerhafter Rückmeldungen.
Sechs CTest-Suiten, 1.537 Aktionsbedingungen und 103 native Szenarien mit 758
Aufrufen und 9.311 Bedingungen bestanden. Effekt-/Transport-/UI-Empfänger bleiben
Testabhängigkeiten; vollständige Spieltransaktion und Host fehlen weiterhin.
[Aktueller Nachweis und Grenzen](EQUIPMENT_EVENTS_2949.md).

Seit 0.10.0: Eigene Nachweise der Inventar-/Besitzergetter, Clientauswahl,
Ausrüstungs-Slotwahl und Sockelfelder auf 2949. Der Leser verlangt einen bekannten
Build. Sechs CTest-Suiten, 385 Leserbedingungen und 88 native Szenarien mit 578
Aufrufen und 6.219 Bedingungen bestanden. Der Ausrüstungstest nutzt Delta 0;
vollständige Änderungen/Ereignisse und der Spielhost bleiben offen.
[Aktueller Nachweis und Grenzen](INVENTORY_2949.md).

Seit 0.9.0: Inventarerfassung direkt aus den gehaltenen Referenzen, ohne zweite
ungeschützte Registry-Suche. Gültige Kennung und lebendige Besitzer sind zwingend.
362 Leserbedingungen und sechs CTest-Suiten bestanden. Die gemeinsame native
Probe umfasst 48 Szenarien, 503 Aufrufe und 3.867 Bedingungen mit echten Registry-
und Besitzersperren. Ihr Inventarlayout ist ein privates Fixture des bisherigen
Lesers, noch keine Layoutfreigabe für den neuen Build.
[Aktueller Nachweis und Grenzen](PINNED_CAPTURE_INTEGRATION.md).

Seit 0.8.0: Geschützte Client-/Server-Suche, konkrete Normal-/User-Referenzmethoden
und Freigabe gemeinsam nativ ausgeführt, mit echten Windows-Registry-Sperren auf
privaten Testobjekten. 30 Szenarien, 263 Aufrufe, 3.490 Bedingungen und sechs
CTest-Suiten bestanden. Aktueller Spieler/Manager und Engine-Thread-Bindung bleiben
offen. [Nachweis und Grenzen](REGISTRY_NATIVE_INTEGRATION.md).

Seit 0.7.0: Registry-Quellenadapter mit eigener Acquire-/Release-Bindung je
Manager; keine globale Freigabemethode. Fünf CTest-Suiten und 55 neue Bedingungen
mit künstlichen Quellen bestanden. [Registry-Vertrag und damaliger Buildwechsel](REGISTRY_INTEGRATION.md).
[Konkrete spätere Testcheckliste](../../TESTCHECKLISTE.md).

## Bisheriger lesender Spieladapter und eigene Reparaturaktion

Seit 0.6.0: Verwaltung nativer Actor-Referenzen und Erfassung unter Referenzen
und Inventarsperren. Fehler geben zuerst Sperren, dann Referenzen zurück.
Abgemeldete oder bereits in Zerstörung befindliche Besitzer werden abgewiesen.
Der konkrete Actor-Freigabepfad ist isoliert geprüft, einschließlich ausstehender
Zustandsbereinigung. Der Live-Erwerb aus geschützten Quellen bleibt noch offen.
[Referenzvertrag und Testgrenzen](REFERENCE_INTEGRATION.md).

Seit 0.5.0: Gemeinsame, nicht wartende Besitzer-Sperren und eine damit
verbundene Inventarerfassung. Originale Lock-Methoden sind im privaten Testhost
mit echten Windows-SRW-Sperren, Rekursion und konkurrierenden Threads geprüft.
Die Sperren allein sichern nicht die Lebensdauer der Spielobjekte.
[Vertrag, native Belege und Grenzen](LOCK_INTEGRATION.md).

Seit 0.4.0: Angenommene Reparaturaufträge können auf Bestätigung warten.
Auftragsnummer/Sitzung, frisch erfasste Nachherwerte und bestätigte Engine-Ereignisse
müssen passen. Bis dahin kein Erfolg und kein zweiter Auftrag. Bei Teilfehler,
Weltwechsel oder Timeout sperrt sich die Warteschlange ohne Wiederholung.
Der native Clientpfad kann Sockel bereits vor einer Fehlermeldung entfernen;
dieser Fall ist nun isoliert nachgewiesen. [Bestätigung und Native-Probe](ACK_INTEGRATION.md).

`crimson_repair_reader.lib` liest das dokumentierte Client-/Server-Layout und
kopiert vorhandene Inventar-/Ausrüstungsitems in private Abbilder. Seit 0.10.0
liegen eigene Nachweise für beide Builds 2944 und 2949 vor. Kennung,
Besitzer, UID, Position und beide Zustandskopien müssen passen; Veränderungen
beim Kontrolllesen verwerfen die Erfassung. Kein Heapscan und kein Spielzugriff
aus der Workbench. Die native Inventarfunktion und der eigene Leser stimmen
auf denselben künstlichen Containern überein. [Layout und Grenzen](READER_LAYOUT.md).

`crimson_repair_action.lib` implementiert Einzelreparatur und RepairAll für
mitgeführte/ausgerüstete Items einschließlich beschädigter Sockeleinsätze.
Leere Reparaturlisten und fehlendes Material sind für diesen eigenen Weg keine
Voraussetzung. Die Aktion erzeugt keine verschwundenen Items und ändert weder
Mengen noch andere Eigenschaften. Der vorhandene No-Wear-Override ist kombinierbar.

Planung, Prüfung beider Zustandskopien, vollständige Vorprüfung eines Batches,
Anwendung auf eigene Speicherabbilder und eine gegen doppelte/verspätete Eingaben
geschützte Warteschlange sind entwickelt. Der gemeinsame Transaktionsadapter ist
seit 0.15.0 vorhanden. **Die vollständige Spielanbindung fehlt:** Das ausführbare
Backend benutzt ausschließlich private Testdaten; reale Empfänger und Host sind
nicht angebunden. Die Bibliothek wird nicht von der App geladen.

Historischer nativer Stand 0.6.0 / EXE 1.0.0.2944: 249 Prüfbedingungen für Leser und Referenzverwaltung, 25 für die Sperrgruppe und 1.481 für die Aktion bestanden. Zusätzlich liest eine isolierte
Kopie des echten Engine-Updaters die durch die eigene Aktion reparierten Felder
korrekt weiter. Insgesamt 203 native Aufrufe und 3.859 Prüfbedingungen im nativen
Host, vier CTest-Suiten, MSVC Release `/W4 /WX` bestanden. Der Originalupdater
ist mit positivem Delta für Sockel ungeeignet; diese Regression ist reproduziert.

Genauer Umfang, Layoutnachweise und noch benötigte Engine-Integration:
[ACTION_INTEGRATION.md](ACTION_INTEGRATION.md).

## Weiter vorhandener Kosten-Prototyp 0.1.0

`CrimsonRepairPrototype.dll` erzeugt einen koordinierten Änderungskandidaten
für zwei vollständig hashgeprüfte Funktionskopien des Builds Steam 25381195,
EXE 1.0.0.2944. Der gemeinsame und der serverseitige Helfer behalten ihre
Reparaturmengenberechnung, Gültigkeitsprüfungen und Haltbarkeitsbegrenzung.
Materialkosten werden auf 0 gesetzt, ihre Division und Materialbegrenzung
umgangen. Negative Kosten und Mengen unter -1 werden abgelehnt. Der originale
Mengensonderwert -1 bleibt erhalten. Der Server legt keinen Materialauftrag an.

Die Bibliothek verarbeitet ausschließlich getrennte Datenpuffer. Beide
vollständigen Funktionshashes müssen passen, bevor ein Ausgabepuffer geändert
wird. Unbekannte Bytes, eine bereits veränderte Funktion, falsche ABI, falsche
Längen, Nullzeiger und überlappende Puffer werden abgewiesen. Der Aufrufer muss
gültige, während des Aufrufs unveränderte Puffer bereitstellen.

Der native Testhost reserviert eigenen Speicher und führt nur Kopien der
ausgewählten Funktionen aus. Itemdefinition, Instanz, Reparaturregel, Material-
bestand, Kostenresolver und Autorisierungsdienst sind künstliche Testdaten bzw.
Stubs. Auch die leeren Material-Transaktionen werden durch die originalen,
unveränderten Prepare-/Commit-Funktionskopien ausgeführt. Eingaben bleiben
unverändert; Rücksetzen der Funktionskopien stellt die normale Berechnung wieder her.

**Bisheriger Nachweis 0.1.0:** 79 native Aufrufe, 1.236 Prüfbedingungen,
CTest-Vertragstest und drei Python-Regressionen bestanden. Die zwei absichtlich
ausgeführten Nullkosten-Divisionen des Originalcodes werden ausschließlich im
eigenen Testhost als erwartete Exceptions abgefangen. Kein Spieleprozess wird
geöffnet, gestartet, angehalten oder verändert.

## Noch offen / Abgrenzung der beiden Wege

- Die 6.816 Item-Reparaturlisten des aktuellen Builds sind leer. Der Prototyp
  erhält die originale Ablehnung solcher Listen; seine positiven Tests benutzen
  ausdrücklich künstliche, nichtleere Regeln mit positiver Reparaturmenge. Die
  neue eigene Aktion benötigt diese Listen nicht.
- Vorgeschaltete UI-/Serveraufrufer verlangen ein vorhandenes Reparaturmaterial.
  Sie werden hier nicht umgangen. Menge 0 bleibt eine Ablehnung.
- Weitere originale UI-Berechnungen und RepairAll sind noch nicht umgestellt. Der globale
  Kostenhelper bleibt unverändert, da weitere Aufrufer durch dessen Wert teilen.
- Der lesende Resolver und der isoliert geprüfte geschützte Referenzerwerb sind
  entwickelt. Die tatsächliche Manager-/Spieler-/Threadbindung sowie die
  vollständige Transaktion samt Inventar-/UI-Benachrichtigungen fehlen.
  Auch eine eigene Bedieneingabe ist noch nicht angebunden.
- Loader, synchronisierte Aktivierung beim nächsten Spielstart und Installation/
  Wiederherstellung über B0 fehlen. Die DLL hat keine `InitializeASI`-Funktion,
  keinen automatischen Hook und keinen Installationsbefehl.

`crimson_repair_installable()` liefert deshalb immer `false`. Den Prototyp nicht
als aktivierbare Option in die Workbench aufnehmen. Die Freigabe zur Entwicklung
besteht bereits; dies ist eine technische Lücke, keine ausstehende Zustimmung.
Farmmodus-Hotkey und Phase 6 gehören nicht zu diesem Reparaturauftrag.

## Reproduzieren

Windows x64, Visual Studio 2022 C++ Build Tools, Windows SDK und CMake benötigt.
Aus dem Projektordner, zunächst nur die synthetischen Vertragstests:

```powershell
./runtime/repair/test.ps1
```

Der Windows-Schritt in `.github/workflows/core.yml` führt diese sechs Suiten
ebenfalls ohne Spieldateien aus. Der native EXE-Nachweis bleibt eine separate
lokale Probe; kein Spielinhalt wird in CI benötigt oder bereitgestellt.

Optional die aktuelle gepinnte EXE 1.0.0.2949 für die Registry-Probe ausschließlich
als Lesequelle angeben:

```powershell
./runtime/repair/test.ps1 -RegistryGameExe "$env:CD_GAME_DIR/bin64/CrimsonDesert.exe"
```

Der separate Schalter `-GameExe` ist weiterhin ausschließlich für den historischen
Host und EXE 1.0.0.2944 bestimmt. Beide Selektoren verlangen ihren eigenen Hash.

Ausgaben bleiben unter `.local/repair-runtime-build/`. Die vollständige EXE
wird vor und nach der Probe gehasht. Nur ausgewählte Funktionskörper werden in
private Speicherbereiche kopiert; die EXE wird weder geladen noch gestartet.
Code-Seiten sind beim Ausführen RX, beim Befüllen RW. Der Testhost registriert
die passenden Stack-Unwind-Informationen. Im historischen Host wird beim Debit-Commit ausschließlich
der Pfad für eine leere Liste ausgeführt. Dessen C++-Cleanup-Handler und der
Cleanup-Handler der mit nichtwerfenden Stubs geprüften Registry/Client-Ack werden
nicht registriert. Inventar-/Sockel-Unwind-Ketten bleiben vollständig erhalten.
Im Client-Ack wird ausschließlich der geprüfte TLS-Lesezugriff in der privaten
Kopie auf künstliche Allocator-Daten umgeleitet; Details im Ack-Nachweis.
Dies ist kein allgemeiner Engine-Emulator.

Der zusätzliche Aufrufer-Audit benötigt die lokal vorhandenen Pakete `pefile`
und `capstone`:

```powershell
python -m unittest discover -s runtime/repair/tools -p test_audit_build.py
python runtime/repair/tools/audit_build.py "$env:CD_GAME_DIR/bin64/CrimsonDesert.exe" > .local/repair-audit.json
```

Der Audit findet 16 direkte E8/E9-Aufrufe/Sprünge zu den drei untersuchten
Helfern und bestätigt ihre Instruktionsgrenzen. Er erfasst keine indirekten
Aufrufe. Überlappende opcodeähnliche Bytes können echte Referenzen nicht mehr
verdecken. Die Importliste bestätigt XInput-Ordinale 2/3 und zwei DXGI-Factory-
Funktionen; daraus folgt noch keine geprüfte Loaderlösung.

Weitere Belege: [Laufzeit-Feldnachweis](../../docs/research/REPAIR_RUNTIME.md),
[Fortschritt](../../docs/PROGRESS.md), [Restpunkte](../../docs/PHASE5_REMAINING.md).
