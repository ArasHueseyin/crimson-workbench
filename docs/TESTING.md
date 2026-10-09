# Prüfplan und Nachweise

Stand: Phase 5, v0.5.9, 23.09.2026. Aktueller Stand in
[PROGRESS.md](PROGRESS.md), Implementierungsgrenzen in [MODS.md](MODS.md).
Live-B0 ist automatisiert an künstlichen Installationen geprüft. Basiswechsel, Inventurintegration und bestätigte Fremdzusätze sind implementiert.
Die manuelle Spielabnahme bleibt **offen**.
Auf Wunsch des Nutzers vom 20.09.2026 werden die manuellen Spieltests erst nach
Abschluss der Entwicklung aller Phasen durchgeführt. Sie blockieren die weitere
Entwicklung nicht und werden bis dahin ausdrücklich als ungetestet geführt.

## Inventar-Paketserializer, Modul 0.23.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 36 zusätzliche Fälle:
26 direkte Proben des Originalsenders/der Streamauswahl und zehn Verbindungen
mit Reparaturtransaktion und Client-Ack. Exakte Feldbreiten und Bytefolge,
Null-/Maximalwerte, falscher Actor, UID-Wechsel, fehlende Empfänger/Puffer,
verworfene Bytes und fehlende/doppelte Zustellung geprüft. Die private
Bestätigung verwirft alle unklaren Ergebnisse und verhindert Replay.

Gesamt: 454 native Szenarien, 4.812 gezählte Aufrufe, 206.786 Bedingungen
einschließlich wiederholter Byteprüfungen. 102 Codebereiche, 127 pdata-Fragmente,
60 private TLS-Stellen plus unveränderte Stackprobe am eigenen Windows-Thread.
CTest-Bedingungen: 11/1.529/519/25/55/612/29/202/273. Produktionssuiten unverändert.
Stream-Puffer, Transport und Server-/Speicherabschluss bleiben künstlich;
der erfolgreiche Poolzweig ist nicht ausgeführt. [Details](../runtime/repair/INVENTORY_PACKET_2949.md).

## Separater Offline-Inventarauftrag

Slot 2 bei geschlossenem Spiel gezielt geändert: 200 leichte und 200 schwere
Kupferbeutel, zusätzlich 30.000 Silber. Originale gesichert; vollständiger
semantischer Vergleich, gültige neue IDs/Slots/Zeiger, bytegenaue Rücknahme auf
Kopien und Integrität der tatsächlich ersetzten Dateien nach erneutem Öffnen
geprüft. Nur `save.save`/`lobby.save` dieses Slots geändert, acht weitere Dateien
unverändert. Anzeige, Verwendung und erneutes Speichern im Spiel bleiben offen.
[Bericht](../.local/inventory-currency/RESULTAT.md). Dieser Einzelauftrag ist kein
allgemeiner Save-Writer und kein Nachweis des Reparatur-Speicherabschlusses.

## Native Pufferaufbereitung und Kompression, Modul 0.22.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 66 neue Fälle auf eigenen
Daten: 40 Größen-/Kompressionsfälle, vier Ausgabe-Wiederverwendungen, sechs Fehler,
14 Verbindungen zum Original-Dateihelfer und zwei Wiederholungen nach Öffnungsfehler.
Ein unabhängiger LZ4-Testdecoder rekonstruiert die ursprünglichen Bytes; die
Ausgabe enthält den exakten Header und beide Größenfelder. Alle Puffer freigegeben.

Gesamt: 418 native Szenarien, 4.289 Aufrufe, 204.378 Bedingungen, darunter viele
wiederholte Byteprüfungen. 100 Codebereiche, 125 pdata-Fragmente und 58 private
TLS-Ersetzungen. Die originale Stackprobe liest das Stacklimit des eigenen Threads.
CTest-Bedingungen: 11/1.529/519/25/55/600/29/202/273. Produktionssuiten unverändert.

Wiederholung eines Eintrags nach Öffnungsfehler komprimiert bereits komprimierte
Daten erneut. Nachgelagerte Verarbeitung und alle Dateioperationen bleiben
Callbacks; kein Integritäts-/Verschlüsselungs- oder Datenträgernachweis.
Allokationsfehler, vollständige Objektserialisierung und Spielanbindung fehlen.
[Nachweis und Grenzen](../runtime/repair/SAVE_ENCODING_2949.md),
`.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.

## Dateischreibhelfer mit privaten Dateiempfängern, Modul 0.21.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 26 Direktfälle und acht
Dispatcher-Integrationen prüfen Pufferbesitz und Fehler bis einschließlich Flush/
Close. Alle Dateioperationen sind private Callbacks mit einem synthetischen Handle.
Kodierung, langer Pfadname und nichtleere Stringdestruktion sind nicht ausgeführt.

Gesamt: 352 native Szenarien, 4.095 Aufrufe, 142.649 Bedingungen. 91 Codebereiche,
118 pdata-Fragmente und 55 private TLS-Stellen. Die CTest-Suiten melden
11/1.529/519/25/55/546/29/202/273 Bedingungen. Produktionssuiten unverändert.
Der erfolgreiche native Ablauf kann trotz fehlgeschlagenem Flush den vorgemerkten
Payload freigeben und Dirty löschen. Das ist ausdrücklich kein Speicherbeleg.

Echter Encoder/WriteFile-Wrapper nur statisch gelesen. Die Vorbereitung kann den
ursprünglichen Payload ändern; der Erhalt bei Fehlern gilt bisher nur mit dem
nichtverändernden Testencoder. Kein Zugriff auf Spielprozess oder Saves; keine
Änderung an der Installation. Die installierte EXE wurde ausschließlich gelesen.
[Nachweis und Grenzen](../runtime/repair/SAVE_FILE_2949.md),
`.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.

## Speicherdispatcher und Warteschlangen, Modul 0.20.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 39 neue native Fälle:
16 Gate-/Statuskombinationen, acht Fehlerfälle, acht Steam-Quota-Fälle,
ein fehlendes Backend sowie sechs Queue-Fälle mit Wachstum und Duplikaten.
Datei-, Plattform- und Diagnoseempfänger bleiben private Testcallbacks.

Gesamt: 318 native Szenarien, 4.039 Aufrufe, 141.411 Bedingungen. 88 Codebereiche,
115 pdata-Fragmente und 51 private TLS-Stellen. Die neun CTest-Suiten melden
11/1.529/519/25/55/528/29/202/273 Bedingungen. Writer und Transaktion unverändert.

Zusätzlicher statischer Nachweis: acht Datei-/Konstruktor-/Timerfunktionen,
fünf VTables, 13 Instruktionsanker und die beiden Dateiimporte. Der originale
Schreibhelfer ignoriert das Flush-Ergebnis; er wurde noch nicht ausgeführt.
Keine Save-Dateien, Steam-API oder laufenden Spielobjekte verwendet. Kein
Datenträgerabschluss und kein installierbarer Reparaturmod nachgewiesen.
[Nachweis und Grenzen](../runtime/repair/SAVE_BACKEND_2949.md),
`.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.

## Item-/Speicherobjekt-Umwandlung, Modul 0.19.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 38 neue native Fälle
führen Originalkonverter und deren Lebensdauerfunktionen auf privaten Objekten
aus; 14 Fälle verwenden produktive Reparaturpläne und native Nachherkopien.
Hauptwerte einschließlich 0/No-Wear/Sentinel, Sockelgeometrien, Färbedaten,
Reset/Wiederverwendung und vollständige Freigabe in beiden Allokationsmodi geprüft.

Gesamt: 279 native Szenarien, 3.950 Aufrufe, 139.784 Bedingungen. 82 Codebereiche,
108 pdata-Fragmente, 50 private TLS-Stellen. Die neun CTest-Suiten melden
11/1.529/519/25/55/492/29/202/273 Bedingungen; die 492 Manifestbedingungen gehören
nicht zu einer zusätzlichen Spielprüfung. Writer und Transaktion unverändert.

Keine reguläre Serialisierung auf Datenträger, kein tatsächlicher Save-Abschluss
und kein vollständiger Engine-Commit. Katalog, Uhr, Reflexionsbenachrichtigungen,
Sockelrichtlinien und Heap sind kontrollierte Testabhängigkeiten. Übrige optionale
Itemdaten bleiben offen. Unveränderte Rust-/UI-Teile nicht erneut getestet.
[Nachweis und genaue Grenzen](../runtime/repair/ITEM_SAVE_2949.md),
`.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`.

## Vollständiger SQL-Auftragsweg, Modul 0.18.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 19 neue native Fälle:
acht Ausführungszweige/Fehlerfälle, drei Request-Erwerbsfälle und acht kombinierte
Slot-Verbraucherfälle in beiden privaten TLS-Allokationsmodi. Status-/Fristfelder,
temporärer Diagnosezeiger, erhaltene alte Fehler, Nullzeiger und Unwind geprüft.

Gesamt: 241 native Szenarien, 3.674 Aufrufe und 128.415 Bedingungen. 58 Codebereiche,
76 pdata-Fragmente und 35 private TLS-Stellen. Produktionssuiten unverändert:
43 Transaktionsszenarien/273 Bedingungen und 35 Schreibszenarien/202 Bedingungen.

Die Ausführung kann Erfolg ohne Request-Zugriff liefern und enthält keinen
Datenbankaufruf. Das native Testergebnis weist ausdrücklich
`sql_execute_is_persistence_proof=false` aus. Noch keine reale Speicherung,
kein vollständiger Engine-Commit und kein installierbarer Reparaturmod.
Uhr, Diagnose, Kapazitätsverwaltung und Teile der Ereignisempfänger bleiben
Testabhängigkeiten. Unveränderte Rust-/UI-Teile nicht erneut getestet.
[Nachweis](../runtime/repair/SQL_DISPATCH_2949.md),
`.local/repair-runtime-v18-{build-test.log,native-result.json,validation.json}`.

Zusätzlicher statischer Nachweis: sieben Funktionshashes, vier RTTI-Typen,
Sprung-/VTable-Zuordnung und 15 Instruktionsanker für alternative Transaktion
und Item-/Speicherobjekt-Umwandlungen. Kein weiterer nativer Erfolg gezählt.
Die Umwandlungen enthalten Haupt- und Sockelhaltbarkeit, sind aber noch nicht
mit eigenen Daten ausgeführt; der tatsächliche Save-Vorgang ist nicht geprüft.
[Nachweis](../runtime/repair/ITEM_SAVE_2949.md),
`.local/repair-runtime-v18-save-route.json`.

## Inventar-Client-Rückmeldung, Modul 0.17.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 36 zusätzliche native
Fälle führen den originalen Inventar-Ack aus: 14 Direktfälle in beiden TLS-Modi
und acht Fälle im gemeinsamen Adapter mit nativen Kopien/Feldschreiber.
Absolute Hauptwerte, Menge/Key/Slot-/Containerfehler, unveränderte Sockel und
korrekte Sperr-/Referenzfolge geprüft. UID-Wechsel, fehlende/doppelte Zustellung
und Fehler beim Abschluss ergeben keinen Reparaturerfolg oder Replay.

Gesamt: 222 native Szenarien, 3.526 Aufrufe, 125.524 Bedingungen. 56 Codebereiche,
70 pdata-Fragmente und 35 private TLS-Stellen. Bestehende Produktionssuiten:
43 Transaktionsszenarien/273 Bedingungen, 35 Schreibszenarien/202 Bedingungen.

Containerübersetzung, leere Namensformatierung und UI sind Testempfänger; die
nichtleeren Stringpfade sind nicht ausgeführt. Inventar-Server und Persistenz
ebenfalls künstlich. Die Integrationsprobe meldet nach dem vorherigen Feldschritt
Delta 0; die tatsächliche UI-Reaktion ist noch zu prüfen. Keine Spielreparatur
behauptet. Die nur statisch geprüfte Socket-SQL-Zuordnung belegt keinen
Haltbarkeits-Speicherweg. Unveränderte Rust-/UI-Teile nicht erneut ausgeführt.
[Nachweis](../runtime/repair/INVENTORY_EVENTS_2949.md),
`.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`.

## Slot-Verarbeitung und Batchabschluss, Modul 0.16.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Ablaufsuite:
43 Szenarien, 273 Bedingungen; Schreiber unverändert 35 Szenarien/202 Bedingungen.
Fehlender Abschlussadapter verhindert Kopieren und Schreiben. Abschlussfehler
oder Sitzungswechsel nach Meldungen bestätigen keinen Erfolg. Aufrufreihenfolge,
freigegebene Sperren und bis zum Ende gehaltene Referenzen/Kopien sind geprüft.

24 neue native Verbraucherfälle umfassen beide Allokationswege, leere Listen,
Duplikate, fehlende/ungültige Items, Hauptwerte 0 und 65.535 sowie 64 Slots.
UID-/Haltbarkeitspaare, native Tabellenbereinigung, erneute Nutzung und Freigabe
sind geprüft. Kapazitätsfehler, positive Ausführungsfehler und gespeicherte negative
Fehler treten nach dem Leeren auf; erneut ausführen stellt die Markierungen nicht
wieder her. Die Persistenzschnittstelle ist ein eigener Testempfänger.

18 Ausrüstungsfälle verwenden den gemeinsamen Adapter samt Slot-Verbraucher.
Drei zusätzliche Fehlerfälle sperren den Auftrag als unklar, obwohl Client-Ack
und Feldänderung erfolgreich waren. Gesamt: 186 native Szenarien, 2.826 gezählte
Aufrufe, 124.527 Bedingungen; viele Bedingungen prüfen wiederholt die Tabellen.
55 Codebereiche, 69 pdata-Fragmente und 32 private TLS-Stellen sind gepinnt.

Tatsächliche Speicherung ist nicht belegt; insbesondere fehlt der ergänzende
Weg für Sockeldatensätze. Inventarereignisse und Spielhost bleiben offen. Kein
Zugriff auf den Spielprozess oder Saves, keine Installationsänderung. App und
ursprünglicher Prüfbericht sowie 38 EXE-/Metadatendateien bleiben hashgleich.
Unveränderte Rust-/UI-Teile wurden nicht erneut getestet. Die spätere Spielabnahme
steht mit konkreten Erwartungen in der [Testcheckliste](../TESTCHECKLISTE.md).
[Nachweis](../runtime/repair/PERSISTENCE_2949.md),
`.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`.

## Gemeinsamer Ablauf und vorbereitete Nachherkopien, Modul 0.15.0

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Neue Ablaufsuite:
40 Szenarien, 247 Bedingungen. Feldschreiber: 35 Szenarien, 202 Bedingungen.
Aktion/Leser/Sperren/Registry/Itembesitzer: 1.529/519/25/55/29 Bedingungen.
Alle Kopien vor dem ersten Store, gesamte Meldungszulassung, falscher Thread,
unzugängliche Zusatzdaten, Allokationsbudget, Kopier-/Schreib-/Meldungsabbrüche,
Sitzungswechsel und unabhängige Bestätigung geprüft. Fehler ab dem ersten
Storeversuch gelten niemals als unveränderte Ablehnung oder Erfolg.

Die 15 nativen Ausrüstungsfälle verwenden den Produktionsadapter statt einer
manuellen Feldschleife. Originale Itemkopien, Feldschreiber, Slot-Markierung,
Entsperren und Server-/Client-Meldungsweg verbunden. Neue Erfassung nach dem
Vorgang verweigert absichtlich abweichende Slots und Zustandskopien. 32 native
Itemfälle prüfen auch leere/teilweise gespeicherte Sockel und Kopiervorbereitung.
Gesamt: 159 native Szenarien, 2.023 gezählte Aufrufe, 119.953 Bedingungen.
51 Codebereiche/57 pdata-Fragmente/26 TLS-Stellen unverändert gepinnt.

Korrekturen: Nur gespeicherte Sockel lesen; logische Plätze dürfen über der
Allokationskapazität liegen. Die Zusatzvektoren bei `+0x78`/`+0xA8` besitzen
16-/6-Byte-Datensätze. Ein Test prüft die zuvor übersehene hintere Überlappung.
Frühere Zähler enthalten Vergleiche ungespeicherter Sockel und den inzwischen
ersetzten manuellen Ereignisaufbau; sie sind nicht direkt als Fallzahlen vergleichbar.

Keine vollständige Live-Reparatur: konkrete Inventarereignisse, Slot-Verbraucher,
echte Empfänger und Host bleiben offen. Unveränderte Rust-/UI-Teile nicht neu getestet.
[Details](../runtime/repair/TRANSACTION_2949.md),
`.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`.

## Vollständiger Haltbarkeits-Feldschritt, Modul 0.14.0

Acht CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Neue Schreibsuite:
34 Szenarien, 197 Bedingungen. Aktions-/Leser-/Sperr-/Registry-/Itembedingungen
bleiben 1.537/519/25/55/30. Alle Bereiche und beide Zustandskopien, No-Wear,
Sentinel, leere Aufträge und ausschließlich geänderte Wörter sind geprüft.

Gegenprüfungen: Kapazitätsfehler, gemeinsame/überlappende Item-/Sockel-/Zusatzpuffer,
geschützte Strukturen, verweigerte Schreibbarkeit eines späten Feldes, veralteter
Batch und Sitzungswechsel. Während der Vorprüfung geänderte Werte, bytegleiche
verlegte Arrays und leere Slots verhindern sämtliche Stores. Teilfehler, falsche
Schreibrückmeldungen und Änderungen nach begonnenem Schreiben ergeben niemals
einen Erfolg oder eine automatische Wiederholung. Eigene Windows-Seiten prüfen
Alignment, Überlauf und Seitenschutz. 2.048 Items/4.096 Stores innerhalb des
Implementierungsbudgets bestehen einschließlich vollständiger Nachkontrolle.

Acht zusätzliche native Fälle verbinden den neuen Schreiber mit originalen
Referenz-/Sperrfunktionen und tatsächlichen privaten Speicheradressen, mit direkter
und registrierter Referenzverwaltung. Alle Quellbytes stimmen mit einem unabhängigen
Nachhervergleich überein. Gesamt: 141 native Szenarien, 1.408 Aufrufe und 120.069
Bedingungen. Das Originalfunktionsmanifest bleibt bei 51 Codebereichen, 57 pdata-
Fragmenten, 26 privaten TLS-Lesestellen und 306 Manifestbedingungen.

`fields_written` ist ausdrücklich kein bestätigter Engine-Commit. Die vollständige
Verbindung mit vorab angelegten nativen Kopien und den Ereignissen fehlt noch;
die bisherigen 15 Ereignisfälle behalten ihren separaten privaten Feldaufbau.
Spielhost, Slot-Consumer, Inventarereignisse und echte Effekt-/Transportverarbeitung
bleiben offen. Unveränderte App-/Rust-/UI-Teile nicht erneut getestet.
[Details](../runtime/repair/FIELD_WRITER_2949.md),
`.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`.

## Gehaltene Erfassung und native Slot-Markierung, Modul 0.13.0

Sieben CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Leser jetzt 519
Bedingungen; Aktion/Sperren/Registry/Itembesitzer unverändert 1.537/25/55/30.
`HeldCapture` hält beide Besitzer und Sperren für Planung und erneute Vorprüfung;
getrenntes Entsperren lässt Referenzen für die Benachrichtigung bestehen.
Wiedererwerb, falscher Thread, Änderungen an Sitzung/Quelle, veraltete Batches
und Freigabe bei Fehlern/Exceptions geprüft. Ein konkurrierender Thread kann
nach `open` keine der beiden tatsächlichen Windows-Sperren erwerben.

16 neue native Fälle prüfen Slot-Markierung mit Duplikaten, Kollisionen,
verschobenen Bucket-Verkettungen und Wachstum auf 64 Einträge, in beiden
TLS-Allokationsmodi. Die 15 Ereignisfälle nutzen die originale Markierung unter
den Sperren vor dem Server-Notifier. Insgesamt 133 native Szenarien, 1.296 Aufrufe
und 119.997 Bedingungen; viele davon prüfen dieselbe Tabelle nach jeder Einfügung.
Manifest: 51 Codebereiche, 57 pdata-Fragmente, 26 private TLS-Lesestellen;
306 Manifestbedingungen ohne EXE. Alle privaten Allokationen mit Schutzmarkierungen
geprüft und ausgeglichen; der zusätzliche Insert-Unwind-Alias ist bytegleich.

Der Slot-Consumer und die echte Listenbereinigung sind offen. Testheap,
Effekt-/Transport-/UI-Empfänger bleiben eigene Abhängigkeiten. Keine Aussage über
vollständigen nativen Schreiber, Spielhost oder erfolgreiche Reparatur im Spiel.
Kein geänderter App-/UI-Code; daher keine erneute Rust-/UI-Prüfung behauptet.
[Details](../runtime/repair/DIRTY_SLOTS_2949.md),
`.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`.

## Native Itemkopie und Ereignisintegration, Modul 0.12.0

Sieben CTest-Suiten, MSVC Release `/W4 /WX`, 30 neue Bedingungen für den nativen
Itembesitzer. Feste Konstruktor-/Zuweisungs-/Destruktorbindung, Threadgrenze,
Headerprüfung vor Zieländerung, Überlappung, doppelte Freigabe und unbrauchbare
native Rückgaben sind geprüft. Aktions-/Leser-/Sperr-/Quellensuiten bleiben bei
1.537/385/25/55 Bedingungen.

117 native Szenarien mit 851 gezählten Aufrufen und 13.639 Bedingungen bestanden.
14 neue Fälle führen die originalen Itemmethoden samt verschachtelten Kopierhelfern
aus. Sockel, beide zusätzlichen Listen und optionale 24-/16-Byte-Blöcke werden
unabhängig kopiert; Wachstum, Ersetzen, Selbstzuweisung und ungültige Quellkapazität
geprüft. Beide TLS-Allokationswege nutzen einen eigenen Windows-Heap mit
Schutzmarkierungen und vollständig ausgeglichenen Freigaben. Quelle bleibt erhalten.

Die 15 Ereignisfälle verwenden jetzt die originalen nativen Nachherkopien unter
den bestehenden Actor-Referenzen: Kopieren unter den Besitzersperren, Benachrichtigung
danach, Destruktion nach Rückkehr. Das Manifest umfasst 45 Codebereiche, 49 pdata-
Fragmente und 24 private TLS-Umleitungen. Ein byteidentischer, gepinnter Unwind-
Alias wird für die Kette normalisiert. Ohne EXE nur 270 Manifestbedingungen.

Kein Nachweis der echten Engine-Allokation, vollständigen Slot-/Inventaränderung,
Effektverarbeitung, Nachrichtenübertragung oder Spielhostbindung. Exceptions und
Allokationsfehler fremder Funktionen bleiben außerhalb der Probe. App-/UI-Code
unverändert, keine neue Rust-/UI-Testausführung behauptet.
[Details](../runtime/repair/ITEM_LIFECYCLE_2949.md),
`.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`.

## Ereignisvorbereitung und native Equipment-Ereignisse, Modul 0.11.0

Sechs CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 1.537 Aktionsbedingungen,
385 Leserbedingungen, 25 Sperrgruppenbedingungen und 55 Quellenadapterbedingungen.
Die eigene Ereignisvorbereitung berücksichtigt aktive Maxima, No-Wear, signierte
Haltbarkeit, reine Sockelreparatur und weiterhin kaputte Nachherzustände.

103 native Szenarien mit 758 gezählten Aufrufen und 9.311 Bedingungen bestanden.
15 neue Fälle verbinden originale Server-Benachrichtigung, Ereignisinitialisierung,
Client-Ack und Sockel-Collector mit privaten Effekt-/Transport-/UI-Empfängern.
Geprüft sind Repaired-Zustandsart 3 und ihre Argumente, optionale Kind-Ereignisse,
No-Wear, Sockelerhalt sowie Ablehnung falscher/fehlender/doppelter Rückmeldung,
fehlender Slots, Teiländerungen, fehlender Belege und geänderter Sitzung.

Die private Reparatur hält native Actor-Referenzen über die gesamte Ereignisfolge;
Besitzersperren werden zuvor freigegeben. Erfolg verlangt tatsächliche Fixture-
Callbacks und frische passende Nachherwerte. Ein Rückgabewert 0 allein genügt nicht.
28 Codebereiche, 25 pdata-Fragmente und acht private TLS-Leseumleitungen; ohne EXE
führt die Manifestprüfung nur 84 Bedingungen und keine Originalfunktionen aus.

Der reale Effektprozessor, Nachrichtentransport, UI und Persistenz sind dadurch
nicht geprüft. Engine-Schreibweg/Itemkopie, Slot-Invalidierung, Inventarereignisse,
Manager-/Thread-/Weltbindung und Installation bleiben offen. App-/Frontendcode
unverändert; keine erneuten Rust-/UI-Tests behauptet. Spielprozess und Saves nicht
benutzt. [Details und Grenzen](../runtime/repair/EQUIPMENT_EVENTS_2949.md).
Nachweise: `.local/repair-runtime-v11-{build-test.log,native-result.json,validation.json}`.

## Native Inventar-/Ausrüstungszugriffe auf 2949, Modul 0.10.0

Sechs CTest-Suiten, MSVC Release `/W4 /WX`, 385 Leserbedingungen und 88 native
Szenarien mit 578 Aufrufen sowie 6.219 Bedingungen bestanden. Die Erweiterung
belegt 21 Inventar-/Besitzerfälle, acht Fälle der aktuellen Clientauswahl,
acht Equipment-Slotfälle und drei Haltbarkeits-Updaterfälle. Der Leser verlangt
einen explizit bekannten Build und weist unbestätigte Selektoren vorher ab.

Native Inventargetter und eigener Leser liefern für dieselben Fixtures passende
Ergebnisse einschließlich Presence-Normalisierung, Ausschlusslisten und
ungültigen Slots. Ein unzugänglicher eigener Slot prüft alle vier Unwindfragmente.
Clientauswahl mit gültigen/ungültigen nativen Referenzbelegen sowie Normal-/User-
und direktem/registriertem Modus geprüft. Positive Sockel-Reparatur über den
Originalupdater bleibt nachweislich ungeeignet: Hauptitem 35 → 100, Sockel 5 → 0.

Equipment-Wrapper mit Delta 0: Native Lock-Referenz und echte exklusive Sperren,
zweiter Slottag mit Abstand `0xd0`, fehlende/leere/verbrauchte Items, Sentinel
und Sockel. Die protokollierten Definitionsabfragen müssen tatsächlich auf den
gewählten Slot zeigen; Rückgabewert 0 allein ist kein Selektionsnachweis.
Quellbytes, Sperrzählung und Freigabe der leeren temporären Itemkopie sind geprüft.

24 Codebereiche und 20 pdata-Fragmente gepinnt; sechs TLS-Lesestellen in eigenen
Kopien auf private Daten umgeleitet. Drei vollständig gehashte Aufrufer belegen
Client-/Server-Kontext- und Komponentenpfade zusätzlich statisch. 72 Manifest-
bedingungen laufen ohne EXE und zählen nicht als native Ausführung.

Die 2949-Lesefelder besitzen damit eigene Nachweise. Die Engine-Thread-/Manager-
bindung, Übergänge und der eigentliche Reparaturcommit sind weiterhin offen.
Der Equipmenttest führt keine Nichtzero-Delta-Ereignisse aus; Tabellenresolver,
temporäre leere Items und Teile der Lebensdauerverwaltung bleiben Fixtures.
App-/Rust-/UI-Code unverändert; keine neue Prüfung dieser unveränderten Bereiche
behauptet. Spiel und Saves nicht benutzt. Nachweise:
`.local/repair-runtime-v10-{build-test.log,native-result.json,reader-result.json,validation.json}`.
[Genaue Testgrenzen](../runtime/repair/INVENTORY_2949.md).

## Erfassung aus gehaltenen Referenzen 0.9.0

Sechs CTest-Suiten, MSVC Release `/W4 /WX`, 362 Leser-/Referenzbedingungen und
die gemeinsame native Probe mit 48 Szenarien, 503 Aufrufen und 3.867 Bedingungen
bestanden. Geprüft sind die ausschließlich gehaltenen Besitzer, fehlende/andere
Clientauswahl, Änderungen beim Kontrolllesen und die Freigabe nach Fehlern.
Explizit unzugängliche Registrybereiche werden im geschützten Leser nicht gelesen.

Der native 2949-Host verbindet jetzt echte Referenzen und nicht wartende
Windows-Besitzersperren mit dem Leser. Konkurrierende Sperren führen zum Abbruch
vor Itemzugriff; alle erworbenen Sperren werden vor Referenzen zurückgegeben.
Inventar, Equipment und Sockel inklusive Kontrolllesen sind unter beiden
Besitzersperren geprüft. Ein währenddessen ersetzter Registryknoten bewirkt
keinen Zugriff auf einen unbesessenen Actor. Itemquellen bleiben unverändert.

16 Codebereiche, zehn pdata-Grenzen und fünf gezielt umgeleitete private
TLS-Lesestellen. Der Test ohne EXE bestätigt nur 48 Manifestbedingungen.
Die neue Lock-Try-Funktion verwendet echte Windows-Imports auf privaten Daten.
Das Inventar-/Clientankerlayout dieser Probe bleibt ein Legacy-Fixture;
sein Zusammenspiel mit den nativen Methoden ist keine 2949-Layoutzulassung.
Manager-/Threadbindung, Weltwechsel und Reparaturtransaktion bleiben offen.

Die Aktions-/Sperrgruppen-/Quellenadapter-Suiten bestehen weiterhin mit
1.481/25/55 Bedingungen. App-/Frontendcode wurde nicht verändert; keine neue
Rust-/UI-Testausführung behauptet. Laufendes Spiel und Saves bleiben unberührt.
Nachweise: `.local/repair-runtime-v9-{build-test.log,native-result.json,reader-result.json,validation.json}`.
[Details und Grenzen](../runtime/repair/PINNED_CAPTURE_INTEGRATION.md).

## Gemeinsamer nativer Registry-/Referenzerwerb 0.8.0

Sechs CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Der separate native
Host für EXE 1.0.0.2949 besteht 30 Szenarien mit 263 Aufrufen und 3.490 Bedingungen:
geschützte Client-/Server-Lookups, Normal-/User-Referenzmethoden, PaPtr-Freigabe,
Generation/Masken, ungültige Belege trotz Pointer, bestehender Besitz, Rollback,
verzögerte Bereinigung und genau einmalige Zerstörung. Vier Fälle verwenden
konkurrierende echte Windows-SRW-Schreibsperren und entfernen Registryeinträge
vor der Freigabe. Die anschließende Suche beachtet den aktuellen Registryzustand.

15 vollständige Codebereiche, neun pdata-Grenzen, konkrete Actor-VTables und
der vollständige EXE-Hash sind gepinnt. Nur vier neun Byte lange TLS-Lesezugriffe
werden in den privaten Kopien auf private Daten umgeleitet. Threadmap, Parent-
Referenzen und endgültige Bereinigung/Zerstörung sind Fixture-Abhängigkeiten;
kein vollständiger Live-Host oder Engine-Commit behauptet. Der direkte native
Erwerb akzeptiert Alive=0; die zusätzliche Ablehnung im Leser bleibt erforderlich.

Alter Host verweigert die neue EXE; neuer Registry-Host verweigert eine unbekannte
PE-Testdatei vor nativer Ausführung. Die übrigen Suiten bestätigen unverändert
1.481 Aktions-, 249 Leser-/Referenz-, 25 Sperrgruppen- und 55 Adapterbedingungen.
Die sechste CTest-Suite prüft ohne Spiel-EXE nur die 45 Manifestbedingungen;
der native Erfolg stammt aus dem gesonderten lokalen Aufruf.

App v0.5.9, Desktop-Verknüpfung, ursprünglicher Audit und die 38 erfassten
Installationshashes unverändert. Keine neue Rust-/Frontendprüfung erforderlich
oder behauptet; kein Spielprozess-/Save-Zugriff. Nachweise:
`.local/repair-runtime-v8-{build-test.log,native-result.json,build-refusals.json,validation.json}`.
[Genaue Pfade und Grenzen](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

## Tabellenunterstützung für Steam 25455892 / App v0.5.9

- 181 Rust-Tests im Workspace, ein gesondert ausgeführter Test der bisherigen
  Advanced-Tabellen, sieben Frontend-Unit-Tests und 61 UI-Flows bestanden.
  Clippy für Workspace/alle Targets mit `-D warnings` und Formatprüfung bestanden.
- Der vollständige Dateivergleich umfasst 61 tatsächlich verwendete Tabellen-,
  Icon- und Sprachdateien: 57 bytegleich, nur Quest-/Stage-Body und Header geändert.
  Alle 14 indizierten Paare haben bytegleiche Roundtrips. Neue Anzahl: 52.082
  Stages, 1.098 Quests. Vollständiger Stage-Modvergleich: ausschließlich die
  beiden gewählten Patrouillen verändert, alle übrigen Stages einschließlich
  beider neuer IDs erhalten; keine Questdatei im Overlay.
- Buildauswahl und Sicherungsbelege prüfen beide bekannten Builds getrennt.
  Falsche Build-ID, gemischte Registry-/EXE-Metadaten, unvollständige Dateien und
  falsche Größen werden abgewiesen. Index/Export benutzen die passende Schema-ID.
- Release-CLI: `metadata_matches=true`, `read_schema_supported=true`, Schema
  `steam-25455892-gamedata-2.3-v2`, `certified_vanilla=false`; Tabellenaufruf erfolgreich.
  Ein eigener PE-Testbestand mit unbekannter Version `0.5.9.0` wird mit Exit 2
  und Versionsfehler abgewiesen, bevor eine Mod-Sicherung geprüft wird. Diese
  als Testdatei kopierte Workbench-EXE wurde nicht ausgeführt.
- Echte Release-App v0.5.9 im eigens gestarteten versteckten Testfenster:
  3.359 Änderungen über 13 Tabellen, 2.069 Skills, neun Inventarbereiche,
  122 No-Wear-Items, 36 Buff-Zusatzkosten und 31 Skill-Eigenkosten geprüft.
  Reparatur bleibt sowohl in der Oberfläche als auch bei direkten Requests gesperrt.
  Zwei Apply-/Restore-Zyklen an einer Projektkopie einschließlich Wiederanwendung,
  Recovery, Startschutz und simulierter Update-Abweisung bestanden. Keine Seitenfehler;
  1.024-Pixel-Ansichten geprüft, Abschlussansicht zusätzlich visuell kontrolliert.
- Das Spiel lief während der App-Probe. 38 EXE-/Metadatenhashes und der ursprüngliche
  Audit blieben unverändert; kein Live-Apply und keine B0-Einrichtung erfolgt.

Die normale Tauri-Buildausgabe konnte eine inzwischen geöffnete ältere Workbench
nicht ersetzen (`os error 5`). Die bereits fertig kompilierte Release-EXE aus
`target/release/deps` wurde nach Versions-/Hashkontrolle als
`target/release/crimson-workbench-0.5.9.exe` bereitgestellt und genau diese Datei
im obigen Desktop-Test geprüft. SHA-256:
`30eba52e4d07237aabe35ae1ca9a6daa3de07607be1f1e8ee2db62a72cba7b18`,
16.183.808 Bytes. Desktop-Verknüpfung und Startskript verwenden diese Datei;
das bestehende Fenster wurde nicht geschlossen.

Die native Reparatur-Runtime ist damit nicht für die neue EXE zugelassen.
Eine gesonderte rein lesende RTTI-/pdata-Probe erfasst acht relevante VTables
neu. Fehlende Funktionsgrenzen bleiben ausdrücklich unbestimmt; keine native
Ausführung und keine Übernahme alter Adressen.

Nachweise: `.local/phase5-build-update-{workspace-tests,pinned-tests,frontend-tests,ui-tests,clippy}.log`,
`phase5-build-update-{fingerprint,tables,unknown-version,delivery,shortcut}.json`,
`.local/phase5-build25455892-native/result.json` und
`.local/build-25455892-table-comparison.json`.
[Buildumfang](BUILD_SUPPORT.md), [spätere Nutzerabnahme](../TESTCHECKLISTE.md).

## Registry-Quellenadapter 0.7.0 und Buildwechsel (vorheriger App-Stand)

Nutzerlesbare Fälle mit Erwartungswerten: [TESTCHECKLISTE.md](../TESTCHECKLISTE.md).
Fünf CTest-Suiten und MSVC Release `/W4 /WX` bestanden: 55 neue Registry-Bedingungen,
249 Leser-/Referenzbedingungen, 25 Sperrgruppenbedingungen, 1.481 Aktionsbedingungen.
Die neuen Adaptertests verwenden künstliche Manager-Lookups und zwei verschiedene
Freigabemethoden. Fehler in beiden Quellen, gleiche Actoradresse, Nullbindung,
volle Kennung, korrekte Kontextbindung und genau einmalige Rückgabe sind geprüft.

Die installierte EXE ist jetzt 1.0.0.2949 / Steam 25455892. 24 der bisher
38 erfassten Dateien unterscheiden sich. Der gepinnte native Host lehnt die neue
EXE ab; das wurde als erwarteter negativer Test geprüft. CLI-Fingerprint meldet
`metadata_matches=false` und `read_schema_supported=false`. Der zusätzliche
CLI-Tabellenversuch stoppt schon an der fehlenden Projektprobensicherung; er
belegt keine konkrete Build-Fehlermeldung. Keine native Erfolgsprobe für 0.7.0
und keine neue UI-/Rust-/Tabellen-Roundtripprüfung behauptet.

App und Audit unverändert. 38 neue beobachtete Hashes nach der Arbeit nochmals
bestätigt. Keine Spiel-/Save-Schreibzugriffe und keine Prozessinteraktion.
Nachweise: `.local/repair-runtime-v7-{build-test.log,validation.json,build-refusals.json}`,
`repair-runtime-v7-{action,reader,lease,registry}-result.json` und
`repair-runtime-v7-current-fingerprint.json`.
[Buildbeobachtung](builds/steam-25455892.observed.json),
[Vertrag und Grenzen](../runtime/repair/REGISTRY_INTEGRATION.md).

## Reparatur-Referenzverwaltung 0.6.0 (vorheriger Stand)

Vier CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 249 Leser-/Referenzbedingungen,
25 Gruppenbedingungen und 1.481 Aktionsbedingungen. Neuer Nachweis: Erwerbsfehler
beider Quellen, balancierte Freigabe auch ungültiger Belege, Sperren vor Referenzen,
volle Kennung, Alive-/Zerstörungszustand vor/nach Erfassung, doppelte/aliasierte
Besitzer, falscher Thread, nicht kopierbarer Besitz und Exception-Unwind.

203 native Aufrufe und 3.859 Bedingungen. 13 Referenzszenarien mit Original-
Erwerb, bestehender Referenzprüfung, PaPtr-Freigabe und konkretem Actor-Release.
Der tatsächliche Release-Override wurde an fünf Actor-VTables kontrolliert.
Geprüft sind beide direkten TLS-Modi und der registrierte Pfad, verzögerte
Zerstörung, ungültige Belege mit Pointer, Sonderbelege und ausstehende Bereinigung.
Drei TLS-Lesezugriffe sind auf private Daten umgeleitet; Threadmap, eingebettete
Sperren und Bereinigungs-/Zerstörungsabhängigkeiten bleiben Fixture-Callbacks.
Keine Behauptung eines vollständigen Live-Hosts oder tatsächlicher Spielereignisse.

38 Installationshashes, App und ursprünglicher Audit unverändert. Kein Spiel-/
Save-Zugriff; keine neue Rust-/Frontendprüfung. Nachweise:
`.local/repair-runtime-v6-build-test.log`, `.local/repair-runtime-v6-validation.json`,
`repair-runtime-v6-{action,reader,lease}-result.json`.
[Details und Grenzen](../runtime/repair/REFERENCE_INTEGRATION.md).

## Reparatur-Besitzersperren 0.5.0 (vorheriger Stand)

Vier CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 1.481 Aktionsbedingungen,
163 Leserbedingungen und 25 Bedingungen für die Sperrgruppe. Die Leserproben
verweigern Itemzugriff ohne beide Sperren; Fehlerfälle prüfen Rückgabe beider
Sperren, falsche Bindungen, geänderte Anker und identische Sperrobjekte.

140 native Aufrufe mit 2.408 Bedingungen. Originale Try-/Release-Kopien verwenden
auf eigenen Testobjekten echte Windows-SRW-Imports. Geprüft sind Exklusivität,
Rekursion, konkurrierende Threads, Rücknahme bei zweiter belegter Sperre und
Ablehnung eines Schreibversuchs bei gehaltenem Leselock. Genau ein neun Byte
langer TLS-Lesezugriff der Try-Kopie ist auf private Daten je Thread umgeleitet;
Restbytes sind unverändert. Dies ergänzt die älteren Proben mit deren Lock-Stubs.

EXE vor/nach Probe verifiziert; 38 Installationshashes, App und ursprünglicher
Audit unverändert. Keine neue App-Testausführung, keine Spiel-/Save-Zugriffe.
Lebensdauerreferenzen der echten Spielobjekte und der Engine-Commit fehlen.
Nachweise: `.local/repair-runtime-v5-build-test.log`,
`.local/repair-runtime-v5-validation.json` sowie
`repair-runtime-v5-{action,reader,lease}-result.json`.
[Details und Grenzen](../runtime/repair/LOCK_INTEGRATION.md).

## Reparatur-Abschlusssteuerung 0.4.0 (vorheriger Stand)

Drei CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 1.481 Bedingungen für
die Aktion, 124 für den unveränderten Leser. Neue Prüfungen: asynchron angenommene
Aufträge zählen noch nicht als Erfolg; Bestätigung braucht alle Nachherwerte und
Ereignisse. Fremde IDs, alte Sitzungen bei gleicher ID, falscher Thread, doppelte
Bestätigung, Teiländerungen, fehlende Items, Katalog-/Weltwechsel und Timeout.
Nach Annahme kein Abbruch/Retry; unklarer Abschluss sperrt die Queue dauerhaft.

122 native Aufrufe mit 2.221 Bedingungen. Sechs zusätzliche Fälle im Client-Ack:
Teiländerung und Sockelentfernung vor Listenfehler, korrekte Abbilder mit Delta 0,
falsche Serverliste trotz korrekter Abbilder, normaler Verschleiß als scheinbarer
Erfolg, erfolgreiche Sockelentfernung und fehlender Slot mit Fehlercode 0.
Die Nachherwertprüfung unterscheidet diese Fälle vom tatsächlichen Reparaturziel.

Originaler Updater, Lock-Konstruktor und Sockelentferner; beim Client-Ack wird
genau ein neun Byte großer TLS-Zugriff auf private Testdaten umgeleitet.
Alle übrigen Ack-Bytes unverändert geprüft, drei Sockel-Unwind-Fragmente samt
Prologkette registriert. UI, Listenallokation und virtuelle Lock-Methoden sind
Stubs. Kein realer Spielereignisversand und kein vollständiger Engine-Commit.
EXE vor/nach Probe geprüft. 38 EXE-/Metadatenhashes, Audit und App unverändert;
kein Spielprozess-/Save-Zugriff. Keine neue Rust-/Frontendprüfung behauptet.

Nachweise: `.local/repair-runtime-v4-{build-test.log,contract.log,validation.json}`,
`repair-runtime-v4-{action,reader}-result.json`.
[Auftragsabschluss, native Befunde und Testgrenzen](../runtime/repair/ACK_INTEGRATION.md).

## Lesender Reparaturadapter 0.3.0 (vorheriger Stand)

Drei CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 124 Bedingungen prüfen
den Leser: Client-/Server-Zuordnung über Charakterkennung, Possessor-/Owner-
Prüfungen, nur vorhandene Items im Character-Inventar und Equipment, UID-/Slot-
Übereinstimmung, Socketdaten, Ausschlüsse, Layout-/Lesefehler und Änderungen
während der Erfassung. 2.048 Items erfolgreich erfasst; Überschreitung des
Gesamtbudgets verwirft die gesamte Ausgabe. Reparatur der resultierenden
privaten Abbilder lässt sämtliche Quelldaten unverändert.

Der native Host besteht 116 Aufrufe mit 1.387 Bedingungen. Neu: Vergleich des
eigenen Inventarlesers mit der Originalfunktion auf denselben künstlichen Daten,
Holder-/Possessorpfade und zwölf Registryfälle. Echte Registry-/Typfunktionskopien,
aber ausdrücklich künstliche nichtwerfende Lock-/Actor-Zugriffsmethoden. Ein
ungültiges Ergebnis kann weiter einen Actorpointer tragen; Validitätsbyte geprüft.
Der eigene Reader besitzt noch keine solche Engine-Zugriffsfreigabe.

Alle vier `pdata`-Fragmente des Inventargetters einschließlich ihrer Unwind-
Ketten werden registriert. Ein absichtlich unzugänglicher privater Slot kehrt
als erwartete Exception zum Testhost zurück. Der Leser lehnt denselben Bereich
ohne direkte Dereferenzierung ab. Die bestehende Aktionssuite bleibt bei 1.377
erfolgreichen Bedingungen. Vollständiger EXE-Hash vor/nach nativer Probe geprüft.

38 EXE-/Metadatenhashes, App v0.5.8 und ursprünglicher Audit unverändert.
Kein Zugriff auf den Spielprozess/Saves, keine Installation. Echte Engine-Leases,
Änderungstransaktion/Benachrichtigungen, Bedieneingabe, Loader und B0 bleiben offen.
Keine neue Spielabnahme, Persistenzzusage oder Rust-/Frontendprüfung behauptet.
Der vorhandene Windows-CI-Befehl erfasst alle drei Suiten; lokal bestanden,
kein GitHub-CI-Lauf gestartet.

Nachweise: `.local/repair-runtime-v3-{build-test.log,contract.log,validation.json}`,
`repair-runtime-v3-{action,reader}-result.json`.
[Leser, native Belege und verbleibende Grenzen](../runtime/repair/READER_LAYOUT.md).

## Eigene Reparaturaktion 0.2.0 (vorheriger Stand)

Beide CTest-Suiten und MSVC Release mit `/W4 /WX` bestanden. Die neue Aktionssuite
prüft 1.377 Bedingungen: Einzel-/Bereichsauswahl, Haupt-/Sockelhaltbarkeit,
No-Wear-Kombination, unveränderte übrige Itembytes, vollständige Batch-Ablehnung
bei Änderungen, Identitäts-/Mengen-/Layoutfehler, Sentinel- und Grenzwerte,
Weltwechsel, falsche Threads, Ablauf/Abbruch sowie gleichzeitige Eingaben.
Ein unklarer Commit sperrt Folgeaufträge; es gibt keine automatische Wiederholung.

Der erweiterte native Host führt insgesamt 83 Aufrufe und 1.252 Prüfbedingungen
aus. Vier zusätzliche Aufrufe der originalen Updater-Kopie bestätigen die
Sockelabweichung bei positivem Delta und die korrekte Weiterverarbeitung der
eigenen Reparaturwerte mit Delta 0, normalem Verschleiß und No-Wear-Override.
Der Getter und sämtliche Item-/Socketinstanzen sind künstliche Testdaten;
vollständiger EXE-Hash wird vor und nach der Probe geprüft.

Die neue statische Bibliothek und Testprogramme bleiben im privaten Buildordner.
Der tatsächliche Spieladapter, Engine-Benachrichtigungen, Bedieneingabe, Loader
und B0-Installation fehlen weiterhin. Keine Spielabnahme oder Persistenzzusage.
App v0.5.8 bleibt unverändert; keine neuen Rust-/Frontend-Ergebnisse behauptet.
38 EXE-/Metadatenhashes, vorhandene App und ursprünglicher Audit unverändert.
Kein Spielprozess- oder Savezugriff, keine Installation.

Der Windows-CI-Schritt ist ergänzt; sein Befehl wurde lokal ohne Spieldateien
ausgeführt und bestand beide Suiten. Kein neuer GitHub-CI-Lauf wurde gestartet.

Nachweise: `.local/repair-runtime-v2-{build-test.log,action-result.json,validation.json}`.
[Implementierung und Reproduktion](../runtime/repair/README.md),
[Spieladapter-Vertrag](../runtime/repair/ACTION_INTEGRATION.md).

## Reparatur-Laufzeitprototyp 0.1.0 (vorheriger Stand)

Separater C++-Release unter `.local/repair-runtime-build/Release/` gebaut;
Desktop-App bleibt v0.5.8. CTest-Vertragstest, 79 native Funktionsaufrufe mit
1.236 Prüfbedingungen, drei Python-Regressionen und zwei bestehende Rust-
Reparatursperren bestanden. Compilerwarnungen sind Fehler (`/W4 /WX`).
Kein neuer Frontend-/Tauri-Code; die vorherigen App-Prüfungen wurden dafür
nicht als neue Tests ausgegeben oder erneut vollständig ausgeführt.

Native Probe: Original, koordinierter Änderungskandidat und Restore; passende,
leere und unpassende Reparaturregeln; Mengen-/Kostenfälle; Fehlerpfade;
unveränderte Eingaben/Materialaufträge und leere Prepare-/Commit-Transaktionen.
Die zwei absichtlich ausgelösten Divisionen durch null bleiben auf den eigenen
Testhost beschränkt. Quell-EXE vor/nach Ausführung vollständig geprüft.
Der Audit bestätigt 16 direkte Referenzen, einschließlich überlappender
Opcode-Kandidaten; indirekte Aufrufe werden nicht als geprüft ausgewiesen.

38 EXE-/Metadatenhashes, ursprünglicher Audit und App-EXE unverändert.
Kein Spieleprozess-/Savezugriff, kein Loader installiert. Die Reparaturfunktion
bleibt mangels vollständigem Aufruf-/UI-/Installationspfad gesperrt.
Diese Probe ist keine Spielabnahme.

Nachweise: `.local/repair-runtime-v1-validation.json`,
`repair-runtime-v1-build-test.log`, `repair-runtime-v1-audit.json`,
`repair-runtime-v1-{audit-tests,core-guard,writer-guard}.log`.
[Reproduktion und Grenzen](../runtime/repair/README.md).

## Zusätzliche Eigenkosten v0.5.8

**Geprüft:** 179 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
sieben Frontend-Unit-Tests und 61 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
und Release-Build grün. Alle 292 BuffInfo-Records und Tabellenfiles bytegleich
rekonstruiert; 36 Zusatzkosten bei 0/50/200 Prozent auf präzise Diffs geprüft.
31 weitere Skill-Eigenkosten einschließlich Einzelwert-Vorrang geprüft.

Fertige EXE v0.5.8: 3.359 Änderungen über 13 Tabellen,
zwei Apply-/Restore-Zyklen an der Projektkopie. Reapply, Recovery, Startschutz
und Updateablehnung bestanden. Neue Kostenansicht bei 1.024 Pixeln und fertige
Vorschau visuell geprüft. 38 EXE-/Metadatenhashes und der ursprüngliche Prüfbericht
unverändert; keine Live-Anwendung, kein Savezugriff, eigene Testinstanz beendet.
Die vorhandene Desktop-Verknüpfung öffnet v0.5.8.

Nachweise: `.local/phase5-v9-{rust,pinned,clippy,unit,ui,build}.log`,
`phase5-v9-native/result.json`, `phase5-v9-validation.json`.
Phase 5 bleibt wegen der dokumentierten technischen Lücken offen.

Manuelle Ergänzung zur späteren Spielabnahme (jetzt nicht durchführen):
- Ausrüstungs-Buff mit Zusatzkosten, etwa Myurdin, bei 100/50/0 Prozent prüfen.
  Ausdauer und Geist getrennt testen; Verhalten nach Ablegen der Ausrüstung.
- Referenzierte aktive Skills und laufenden Verbrauch getrennt prüfen; ein
  positiver Regenerationseffekt muss weiter funktionieren.
- „Weitere Skills: Geist“ für Ressourcenverbrauch/Elementarverstärkung prüfen;
  individuelle Buffausnahme muss dem Faktor vorgehen. Zusätzlicher Verbrauch
  außerhalb dieser Tabellenfelder bleibt gesondert zu protokollieren.
- Gegnerische Drain-Effekte und Dot-Debuffs dürfen unverändert bleiben.
- Export und Restore müssen die zusätzlichen `buffinfo`-Dateien einschließen.

## Phase-5-Restprüfung v0.5.7 (vorheriger Stand)

**Geprüft:** 176 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
sieben Frontend-Unit-Tests und 60 UI-Flows; Fmt, Clippy `-D warnings`, TypeScript
und Release-Build grün. Alle 6.816 Items auf präzisen Haltbarkeits-Diff und
Idempotenz geprüft, alle 2.069 vollständigen Skill-Records bytegleich rekonstruiert.

Fertige EXE v0.5.7: 122 Haltbarkeitsänderungen und alle sechs neuen Lager in
der tatsächlichen UI, Vorschau und im Export geprüft. Insgesamt 3.323
Änderungen über zwölf Tabellen, zwei Apply-/Restore-Zyklen an der Projektkopie;
Reapply, Recovery, Startschutz und Updateablehnung bestanden. Neue Lager- und
Haltbarkeitsansicht bei 1.024 Pixeln sowie Vorschau visuell geprüft. Keine
Browserfehler. 38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht unverändert.
Keine Mods live angewendet oder Saveinhalte gelesen. Eigene Testinstanz beendet;
die bestehende Desktop-Verknüpfung öffnet v0.5.7.

Nachweise: `.local/phase5-v8-{rust,pinned,clippy,unit,ui,build}.log`,
`phase5-v8-native/result.json`, `phase5-v8-validation.json`.
Spielabnahmen bleiben verschoben. Phase 5 ist wegen der dokumentierten
technischen Lücken weiterhin nicht vollständig abgeschlossen.

Zusätzliche Regressionen: alle 6.816 Items auf exakten Haltbarkeits-Diff,
unveränderte 0-/65535-Werte, Kombination mit Itemänderungen und Idempotenz;
vollständige Inventar-Move-Strukturen, abgebrochene Eingaben und geschützte
Spezialcontainer; alle neun Slotbereiche und deren Grenzen; sechs zusätzliche
Lager zusammen mit Haltbarkeit in UI, Export und Projekt-Apply/Restore.

[Restpunktmatrix und technische Lücken](PHASE5_REMAINING.md).
Manuelle Spieltests bleiben zurückgestellt.

## Phase-5-Erweiterung v0.5.6 (vorheriger Stand)

Der Reader wird mit einer Gruppenreferenz ungleich Record-ID und eingebetteten
Kennungsbytes in einem UTF-8-Text geprüft. Beide gültigen Fälle bleiben lesbar;
ein ungültiger Bufftyp darf nicht durch einen späteren Suffix gerettet werden.
Zusätzliche Tests trennen u16/u32-Referenzen, negative Ressourcenwerte,
Regenerationsflag und Itemkostenbytes und prüfen Counts sowie jeden abgeschnittenen
Präfix eines nichtleeren Beispiels. Sämtliche neuen Inspektionsfelder bleiben
schreibgeschützt. Bestehende Buffänderungen bewahren den neuen Suffix bytegleich.

Die reale, hashgeprüfte Probe rekonstruiert alle **2.069 vollständigen Records**
aus 87.697 Basisfeldern, Matrixcounts und 4.607 Buffeinträgen. Keine Lücke in der
Byteabdeckung außerhalb der Buffmatrix mehr. Die UI-Probe prüft Namens-/Wertesuche,
leere Treffer, Schutz vor Bearbeitung, unveränderte bestehende Vorschau und
Darstellung bei 1.024 Pixeln.

**Geprüft:** 175 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
sieben Frontend-Unit-Tests und 59 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
und Release-Build grün. Alle 2.069 vollständigen Skills bytegenau rekonstruiert.

Fertige EXE v0.5.6: Basisfelder echter Skills per IPC und UI geprüft,
Referenzen schreibgeschützt; Namens-/Wertesuche sowie Darstellung bei 1.024 Pixeln.
3.189 reguläre Änderungen über zwölf Tabellen, Export und zwei Apply-/Restore-
Zyklen an einer Projektkopie erfolgreich. Reapply, Recovery, Startschutz und
Updateablehnung bestanden. Keine Browserfehler. Die neue Basisdatenansicht und
die fertige Vorschau wurden visuell geprüft.

38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht unverändert. Keine Mods
live angewendet, keine Saveinhalte gelesen. Eigene unsichtbare Testinstanz beendet.
Die bestehende Desktop-Verknüpfung öffnet v0.5.6. Manuelle Spieltests bleiben
wie vereinbart zurückgestellt.

Nachweise: `.local/phase5-v7-{rust,pinned,clippy,unit,ui,build}.log`,
`phase5-v7-native/result.json`, `phase5-v7-validation.json`.

Manuelle Workbench-Prüfung nach Abschluss aller Phasen: Skill auswählen,
**Buffmatrix & vollständiger Datensatz → Basisdaten, Voraussetzungen & Ressourcen**
öffnen; nach `varyStatAmount`, `parentSkill` und einer Kennung suchen. Originalwerte
bleiben von vorgemerkten Änderungen getrennt. Nur bereits freigegebene Skillfelder
oberhalb bearbeiten; Referenzen haben keinen Schreibschalter. Keine neue
Gameplaywirkung durch die reine Inspektionsansicht behauptet.

## Phase-5-Erweiterung v0.5.5 (vorheriger Stand)

Die Reparatursperre wird mit synthetisch vorhandenen Regeln geprüft: globale
und individuelle Flags (auch zu unbekannten Itemschlüsseln) müssen vor einer
anderen gültigen Skilländerung scheitern. Weder Änderungen noch Ersetzungen
dürfen zurückbleiben. Ein separater Format-Test prüft, dass alte Requests mit
`free_repair: true` den Writer nicht erreichen; das Flag bleibt für die gezielte
Korrektur alter Vorlagen lesbar und andere Werte bleiben erhalten.

Der neue UI-Flow gibt absichtlich nichtleere Reparaturmetadaten zurück. Trotzdem
bleiben beide Schalter zunächst gesperrt. Eine alte Vorlage mit aktivem Flag
liefert keine exportierbare Vorschau. Das Häkchen lässt sich entfernen; danach
ist die Vorschau mit den unveränderten übrigen Itemwerten wieder möglich.

Der tatsächliche Nullfehler wurde ausschließlich statisch nachgewiesen, nicht
im laufenden Spiel ausgelöst.

**Abschluss:** 173 Rust-Tests, eine separat tatsächlich ausgeführte hashgeprüfte
Tabellenprobe, sieben Frontend-Unit-Tests und 58 UI-Flows erfolgreich. Fmt,
Clippy `-D warnings`, TypeScript und Release-Build grün.

Nativer unsichtbarer Release-Test v0.5.5: globale und individuelle
`free_repair: true`-Requests werden per echtem IPC abgelehnt. Normale Vorschau
mit 3.189 Änderungen über zwölf Tabellen, Export und zwei Apply-/Restore-Zyklen
an einer Projektkopie erfolgreich. Reapply, Recovery, Startschutz und
Updateablehnung bestanden. Reparaturansicht bei 1.024 Pixeln und fertige
Vorschau visuell geprüft; keine Browserfehler. 38 EXE-/Metadatenhashes und
der ursprüngliche Prüfbericht unverändert. Keine Mods live angewendet und keine
Saveinhalte gelesen. Eigene Testinstanz beendet. Spielabnahme weiter auf Nutzerwunsch vertagt.

Belege: `.local/phase5-v6-rust.log`, `phase5-v6-pinned.log`,
`phase5-v6-clippy.log`, `phase5-v6-unit.log`, `phase5-v6-ui.log`,
`phase5-v6-build.log`, `phase5-v6-native/result.json`,
`phase5-v6-validation.json`. Kostenfreie Reparatur gilt weiterhin als offen;
Phase 5 ist noch nicht vollständig entwickelt.

## Phase-5-Erweiterung v0.5.4 (vorheriger Stand)

Neue Prüfungen decken die vollständige Feldabgrenzung von neun Summon-Payloads
und einem AddSubLevel-Payload ab. Alle 4.607 Matrixeinträge der 2.069 Skills
werden aus den angezeigten Werten bytegleich rekonstruiert. Die beiden neuen
Varianten enthalten keine editierbaren Payloadfelder. Tests lehnen Änderungen
an Referenzen, Counts und neu gelesenen Zeit-/Spawnwerten ab. Bestehende
Common-Buffänderungen müssen den Summon-Payload unverändert erhalten.

Synthetische Fälle: nichtleere Select-Liste, mehrbyteiger UTF-8-Text und dadurch
verschobene Folgefelder, jeder abgeschnittene Summon-Präfix, übergroße Counts
und nicht unterstützte eingebettete Bedingungen. AddSubLevel-Referenz und
unbekanntes Vier-Byte-Feld werden einzeln geprüft; keine i64-Umdeutung.

UI: Feldnamensuche einschließlich Groß-/Kleinschreibung, keine Treffer,
Rücksetzen des Suchfilters und keine Schreibfelder für neue Payloadwerte.
Ein anfänglicher neuer Test erwartete bei unverändertem Formular einen
Advanced-Request; korrekt ist dessen vollständiges Weglassen. Die Assertion
wurde entsprechend korrigiert, ohne den Produktcode dafür zu ändern.
**Abschluss:** 171 Rust-Tests, eine separat tatsächlich ausgeführte hashgeprüfte
Tabellenprobe, sieben Frontend-Unit-Tests und 57 UI-Flows erfolgreich. Fmt,
Clippy `-D warnings`, TypeScript und Release-Build grün. Nach einer abschließenden
Textpräzisierung wurde der Release erneut gebaut; der native Test verwendet
nur diese fertige v0.5.4 und prüft zusätzlich deren Versionsanzeige.

Nativer unsichtbarer Release-Test: alle zehn neuen Payloads über echtes IPC,
Referenzschutz und Feldnamensuche; 3.189 Änderungen über zwölf Tabellen,
Export und zwei Apply-/Restore-Zyklen an einer Projektkopie. Reapply, Recovery,
Startschutz und Updateablehnung bestanden. Summon-Ansicht bei 1.024 Pixeln und
fertige Vorschau visuell geprüft; auch der innere Detailbereich hat keinen
horizontalen Überlauf. Keine Browserfehler. 38 EXE-/Metadatenhashes und der
ursprüngliche Prüfbericht unverändert; keine Live-Anwendung oder Saveinhalte
gelesen. Eigene Testinstanz anschließend beendet.

Belege: `.local/phase5-v5-rust.log`, `phase5-v5-pinned-final.log`,
`phase5-v5-clippy.log`, `phase5-v5-unit.log`, `phase5-v5-ui.log`,
`phase5-v5-build-final.log`, `phase5-v5-native/result.json`,
`phase5-v5-validation.json`. Manuelle Spieltests bleiben zurückgestellt;
Phase 5 gilt weiterhin nicht als vollständig entwickelt.

## Phase-5-Erweiterung v0.5.3 (vorheriger Stand)

**169 Rust-Tests**, **1 separat ausgeführte lokale Tabellenprobe**,
**7 Frontend-Unit-Tests** und **56 UI-Flows bestanden**. Fmt, Clippy mit
`-D warnings` und TypeScript erfolgreich.

- Wiederbesetzung: vollständige Struktur, leere Listen, übergroße Counts,
  abgeschnittene Records, Suffixbytes und Timer-Sentinels; Faktor, Einzelausnahme
  und Grenzen. Alle 108 echten Regeln werden geparst und halbiert, ausschließlich
  die vier Bytes von `delayTime` dürfen sich unterscheiden. Questverknüpfungen,
  Bedingungen, Raten, Close-Timer, Knoten und Schutzwerte bleiben bytegleich.
  Header/Body werden erneut aufgebaut und eingelesen. Der Integrationstest
  exportiert und entpackt nun zwölf geänderte Tabellen.
- Inventar: beide Felder aller drei echten Inventare erlauben 1460;
  1461 und 65535 werden backendseitig abgelehnt. Die UI übernimmt dieselbe
  Grenze aus dem Katalog und blockiert ungültige Vorschauen.
- Die optional neue Request-Eigenschaft bleibt bei alten Requests vollständig
  abwesend. Vorschau, Einzelausnahmen, Rücksetzen und getrennte Patrouillen-
  /Spawn-Einstellungen werden im UI geprüft.
- Ein neuer UI-Test deckte einen bestehenden Freigabe-Randfall auf: Eine
  ungültige sichtbare Zahl änderte den letzten gültigen Request nicht. Die
  DOM-Gültigkeit wird jetzt nach den Eingabe-Handlern berücksichtigt. Export,
  Probe und Live-Dateivorschau werden gesperrt; eine vorbereitete Live-Freigabe
  verfällt und kehrt nach Korrektur nicht automatisch zurück. Frühere
  fehlschlagende Testläufe bleiben als Diagnose erhalten; der abschließende
  vollständige Lauf ist grün.

Belege: `.local/phase5-v4-rust.log`, `phase5-v4-pinned-final.log`,
`phase5-v4-clippy.log`, `phase5-v4-unit.log`, `phase5-v4-ui-pass.log`.
Die Tabellenprobe bleibt im gewöhnlichen Workspace-Test explizit `ignored`
und wurde zusätzlich separat tatsächlich ausgeführt.

Nativer unsichtbarer Release-Test v0.5.3: **3.189 Änderungen über zwölf Tabellen**,
108 Wiederbesetzungsregeln (davon eine Einzelausnahme), 2.922 Advanced-Records,
Export und zwei Apply-/Restore-Zyklen ausschließlich an der Projektkopie.
Reapply, Recovery, Startschutz und Updateablehnung bestanden. Alle 38 geprüften
EXE-/Metadatendateien und der ursprüngliche Prüfbericht blieben identisch;
Spiel lief, keine Live-Anwendung oder initialisierte Workbench im Spielordner,
keine Saveinhalte gelesen. Weltansicht bei 1.024 Pixeln und fertige Vorschau
visuell geprüft; kein horizontaler Überlauf, keine Browserfehler. Eigene
Testinstanz anschließend beendet. Belege: `.local/phase5-v4-native/result.json`,
`phase5-v4-build.log`, `phase5-v4-validation.json`.

## Phase-5-Erweiterung v0.5.2 (vorheriger Stand)

**167 Rust-Tests**, zusätzlich **1 explizite lokale Tabellenprobe**,
**7 Frontend-Unit-Tests** und **53 UI-Flows bestanden**. Fmt und Clippy mit
`-D warnings` erfolgreich. Zwei anfängliche UI-Testassertionen unterschieden
zwischen einem fehlenden optionalen Property und `undefined`; die produktive
JSON-/Plan-Kanonisierung ließ beides korrekt aus. Die korrigierten Tests prüfen
weiterhin, dass das Entfernen von Overrides keinen veralteten Plan hinterlässt.

Neue Prüfungen: genaue i64-Änderung und Identität aller übrigen Buffbytes;
Grenzen, Bruchzahlen, Exponenten, unbekannte Pfade, strukturelle Felder,
Referenzen und opake Werte werden abgelehnt. Graphkurven bleiben geschützt.
Cooldown- und Buffänderung am selben realen Skill kommen gemeinsam im Overlay an.
Alte Requests ohne neue optionale Felder behalten ihre serialisierte Form.

Patrouillen: beide Originaltimer, Faktor und Einzelüberschreibung, positive
Grenzen, Sentinelablehnung und abgeschnittene Präfixe. Alle übrigen Stagebytes
und Records bleiben unverändert. Stadtflug: exakte Regelidentität, Ausdruck,
Baum und Flags; Fremdwerte und bereits geänderte Regeln werden abgelehnt.
Beide Bedienoptionen ändern nur denselben Conditionrecord; alle anderen
10.797 Bedingungen bleiben byteidentisch. Die Archivintegration baut und
entpackt elf Tabellen ausschließlich im Projekt-/Temp-Bereich.

Finale Protokolle: `.local/phase5-v3-rust-final.log`,
`.local/phase5-v3-pinned-final.log`, `.local/phase5-v3-clippy-final.log`,
`.local/phase5-v3-unit.log`, `.local/phase5-v3-ui-final.log`.
Der optionale Tabellenprobentest bleibt im normalen Workspace-Lauf `ignored`
und wurde separat tatsächlich ausgeführt. Manuelle Spielabnahme weiterhin offen.

TypeScript und Release-Build erfolgreich. Nativer unsichtbarer Test der EXE
v0.5.2: **3.081 Änderungen über elf Tabellen**, i64-Buffänderung plus Cooldown,
beide Patrouillen-Timer und Stadtflug-Regel im selben Plan; Export und zwei
Apply-/Restore-Zyklen nur an der Projektkopie. Reapply, Recovery, Start-/Backup-
Schutz und Updateablehnung erfolgreich. Alle 38 geprüften EXE-/Metadatendateien
und der ursprüngliche Prüfbericht unverändert; Spiel lief, keine Live-Anwendung
oder initialisierte `.workbench` im Spiel. Eigene Testinstanz danach beendet.
Screenshots bei 1.024/1.440 Pixeln visuell geprüft. Der erste native Start in
der Sandbox hatte keinen erreichbaren WebView-Testzugang; der abschließende
Lauf mit lokalem Testprofil und nötigen Ausführungsrechten bestand vollständig.
Belege: `.local/phase5-v3-native/result.json`, `.local/phase5-v3-build.log`,
`.local/phase5-v3-validation.json`.

## Phase-5-Erweiterung v0.5.1 (vorheriger Stand)

**162 Rust-Tests**, zusätzlich **1 explizite lokale Tabellenprobe**,
**7 Frontend-Unit-Tests** und **51 UI-Flows bestanden**. Fmt, Clippy mit
`-D warnings`, TypeScript und Release-Build erfolgreich.

Neue Prüfungen: Enchant-Kopie mit Preisen/Buffs/Stats, sortierte Einfügung
mit korrekten Trennern, Ablehnung doppelter Ziele, verketteter Kopierquellen,
unbekannter Quellen und nicht bestätigter Originalstrukturen. Statänderung
auf neuer Stufe, neu aufgebaute Header und unveränderte andere Itemrecords.
Stacks werden auch nach neu hinzugefügten Buffs/Enchant-Zeilen geprüft.
Alte Vorlagen ohne Kopierregeln bleiben kompatibel; leere optionale Listen
machen eine Vorschau nicht fälschlich veraltet.

Alle 2.069 Skillmatrizen und 4.607 Einträge einschließlich null sind vollständig
und lückenlos inspizierbar. Jede ausgegebene Feldrepräsentation wird im Test
zurück in Bytes gewandelt und mit den Originalbytes verglichen. Tests für
abgeschnittene Daten, ungültige Flags/Counts, zusätzliche Bytes und signed
Werte; neue IPC-Abfrage lehnt veraltete Sitzungen ab. Buffwerte bleiben
schreibgeschützt, Rohdaten/Typgrenzen sind keine bestätigte Gameplaysemantik.

Nativer unsichtbarer Lauf: `.local/phase5-v2-native/result.json`, **3.077
Änderungen über neun Tabellen**, neue Enchant-Stufe und Skillmatrixansicht,
Export und zwei Apply-/Restore-Zyklen nur an Projektkopien. Reapply, Recovery,
Start-/Backupschutz und Updateablehnung erfolgreich. Alle 38 geprüften
EXE-/Metadatenhashes und der ursprüngliche Prüfbericht unverändert. Spiel
lief; keine initialisierte `.workbench` in der Installation, keine Live-Anwendung.
Screenshots bei 1.024 und 1.440 Pixeln visuell geprüft.

Protokolle: `.local/phase5-v2-{rust,pinned,unit,ui,clippy}-final.log`,
`.local/phase5-v2-build.log`, `.local/phase5-v2-validation.json`.

## Phase-5-Prüfungen v0.5.0 (vorheriger Stand)

Rust-Workspace plus lokale zusätzliche Echtdateiprobe, Frontend-Unit-Tests und
Headless-UI-Flows prüfen den neuen Stand. Finale Protokolle und Zählungen liegen
unter `.local/phase5-*-final.log` bzw. `.local/phase5-validation.json`.

Ergebnis: **158 Rust-Tests**, zusätzlich **1 lokale Tabellenprobe**,
**6 Frontend-Unit-Tests** und **49 UI-Flows bestanden**. Der optionale
Tabellenprobentest wird im allgemeinen Workspace-Lauf übersprungen und wurde
separat explizit ausgeführt. Fmt, Clippy `-D warnings`, TypeScript und
Release-Build erfolgreich.

Neue Kernprüfungen: negative Kosten ohne Veränderung positiver Regeneration;
Originalaufbau statt wiederholter Multiplikation; Präzedenz global → Skill →
Einzelfeld; unveränderte opake Bytes; Sentinel-Erhalt, Überlaufablehnung,
unbekannte IDs/Kategorien/Felder; gesperrte nicht vorhandene Reparaturregeln.
Die lokale hashgeprüfte Probe liest alle unterstützten Records, verändert
mehrere Tabellen, fügt einen Stat ein und prüft den neuen Header sowie alle
anderen Itemrecords auf Byteidentität. Die reguläre Echtdateiintegration
baut ein Overlay mit neun Tabellen und entpackt es ausschließlich in Temp.

Frontend: exakte Requests, kanonischer Vergleich nach Rust-Sortierung,
ungültige Eingaben, gesperrte Reparaturoption, vorzeichenbehaftete Skillwerte,
lokale Vorlagen über Neustart und auf ein anderes Item, späte IPC-Antworten
und 1024px ohne horizontalen Überlauf. Ungültiger Vorlagenspeicher wird verworfen.

Nativer Lauf `.local/phase5-native/result.json`: v0.5.0, 2.811 Records,
2.069 Skills, 6.816 Items, 2.833 Items ohne erkannte Zustandsmerkmale,
0 Items mit Reparaturregeln, 3.076 geplante Änderungen über neun Tabellen.
Projektprobe mit zwei Apply-/Restore-Zyklen, Reapply, Recovery, gehaltenem
Schutz und Updateablehnung bestanden. 38 Original-EXE-/Metadatendateien
und der ursprüngliche Audit unverändert. Kein Live-Apply, kein Savezugriff,
keine Steuerung oder Sperre des laufenden Spiels. Screenshots wurden geprüft.

### Manuelle Abnahme B4–B11 – erst nach allen Entwicklungsphasen

Alle folgenden Punkte sind **offen**, keiner wurde automatisiert als Gameplay-
Erfolg gewertet. Voraussetzungen aus dem B0-Prüfplan gelten weiterhin.
Jeweils ein kleines, eindeutig beobachtbares Beispiel einzeln prüfen und
danach Restore durchführen. Bei jeder Zeile Original-/Mod-/Restore-Beobachtung
und Buildnummer dokumentieren.

| Modul | Spätere Prüfung |
|---|---|
| B4 | Identifizierte Terrain-/Pool-Gruppe mit ×2 sowie Einzelausnahme vergleichen; keine unbeteiligten Spawns verändert. Reittier im MainField/in einer Stadt rufen und bewegen; Dauer und Cooldown messen. Hidden-Estate-/Watergate-Reset mit Faktor und Einzelwert beobachten; reale Uhr und Spielzeit getrennt erfassen. Allgemeine NPC-Timer bleiben offen. |
| B4 Wiederbesetzung | Original, 50 % und eine Einzelausnahme an bekannten Fraktionsgebieten vergleichen. Vorhandene Bedingungen/Questzustände, Spielzeit gegenüber Echtzeit und nach Restore die Originalwartezeit prüfen. Keine allgemeine NPC-Abdeckung unterstellen. |
| B5 | Blackstar-Cooldown, 10-/30-Minuten-Dauer und eigenen Wert messen. Abyss-Regionssperre und Stadtflug abseits von Straßen getrennt prüfen, auch mit anderem Flugreittier; Kopfgeld-/Questbedingungen unverändert. Andere Grenzen prüfen; Ausnahmen/geskriptete Abstiege aufzeichnen. Bossdrache unverändert. |
| B6 | Start-/Maximalplätze für alle neun Bereiche einschließlich Kuku und fünf Housing-Lagern einzeln prüfen, einschließlich bereits vorhandener Spielstände. Erst moderate Erhöhung; große Werte und Verkleinerung nicht als bestanden übernehmen. |
| B7 | Gewöhnliche stapelbare Items, ausgewählte Kategorie und Einzelitem vergleichen; Aufteilen, Kaufen, Looten, Verschieben und Neuladen. Experimentelle Ausrüstung separat mit verschiedenen Haltbarkeits-/Enchant-/Sockelzuständen prüfen; Zusammenführung/Verlust wäre ein Fehler. |
| B8 | Bekannte Verschleißhandlung mit „Kein Haltbarkeitsverlust“ und nach Restore vergleichen: gewöhnliche Ausrüstung, Spezialausrüstung mit endlicher Haltbarkeit und gesockelte Items getrennt. Bereits verbrauchte Exemplare sind kein Wiederherstellungstest. Reparaturkostenprüfung ist zurückgestellt, weil dieser Build noch keinen freigegebenen Kostenpfad hat. |
| B9 | Pro Kategorie Original, 50 %, 0 % vergleichen; Bewegung/Kampf und Fahrerfälle abdecken. Positive Regeneration muss unverändert bleiben. Nicht erfasste Kosten ausdrücklich dokumentieren. |
| B10 | Einen tatsächlichen nichtleeren Cooldown global und individuell ändern, Priorität prüfen; bekannte Zahlen auf einem einzelnen Skill prüfen. Einen bestätigten i64-Buffwert gezielt ändern; andere Matrixbytes bleiben unverändert. Unbekannte Bedeutungen nicht als geprüfte Effekte deklarieren. |
| B11 | Ein Item/Enchant-Level mit einem Stat und einem kompatiblen Buff testen. Zusätzlich eine neue Stufe aus einer Originalstufe kopieren; Tabellenexistenz allein beweist keinen erreichbaren Upgradeweg. Equip/Unequip, tatsächlich erreichbare Level, neues/bestehendes Item und Neuladen vergleichen. Vorlage auf geeignetes zweites Item übernehmen; unpassende Quellen/Level/IDs müssen abgelehnt werden. |

Danach gemeinsames Overlay mehrerer Module, Restore und erneutes Apply prüfen.
Damit sind weder die noch offenen Formatfunktionen noch die Spielabnahme
vorweggenommen.

## Phase-4-Prüfungen v0.4.10

**154 Rust-Tests bestanden**, einschließlich vier Echtdateiintegrationen.
Fmt und Clippy mit `-D warnings` erfolgreich.

Neun neue Core-Szenarien: exakte Zusatzbestätigung für Inventur/Inhaltsbericht und
Apply/Reapply/Restore, unveränderte fremde PAZ/PAMT/DLL-Dateien, kollisionsfreie
eigene Gruppen, leere Dateien und zusätzliche `.EXE`, gleich große nachträgliche
Änderungen, neue Kinder und veraltete Vorschauen; Widerruf vor/während geschützter
Arbeit mit Freigabe der Handles; gesperrte Originaländerungen, fremde Registry,
Saves einschließlich konfigurierter geschützter Unterordner, private Pfade
und Hardlinks; beschädigte Bestätigungen und Eingabegrenzen;
später von Steam übernommene Pfade; Basiswechsel unter Erhalt fremder Dateien.
Der letzte der 128 Protokolleinträge ist für einen Widerruf reserviert. Alle acht
Fremddateitests bestanden nach Ergänzung dieses Grenzfalls erneut.

44 UI-Flows und 4 Frontend-Unit-Tests bestanden, TypeScript und Frontend-Build
passen. Fünf neue UI-Flows prüfen freiwilligen Aufruf, explizite genaue Bestätigung
ohne Live-Aktion, Widerruf auch bei gesperrter neuer Vorschau, veraltete Antworten,
neue Checkbox nach Fehler, leeren Bestand und 1024px-Layout. Der neue Ablauf wurde
nicht auf eine echte Fremdmod-Installation angewendet.

Release v0.4.10 und native Windows-Prüfung bestanden. Zusatzdateivorschau auf der
echten Installation rein lesend geprüft: leerer Bestand, keine Bestätigung und
keine Live-Einrichtung. Drei neue Sitzungssperren, bestehende sieben Live-Sperren,
Projektproben und Restore erfolgreich. Keine JavaScript-Fehler; 1024px-Ansicht
visuell geprüft. Alle 38 EXE-/Metadatenhashes und der ursprüngliche Nutzerbericht
unverändert; kein kompletter PAZ-Hashlauf und keine Spielsperre.
Nachweise: `.local/phase4-v11-validation.json`, `.local/phase4-v11-native/result.json`,
`.local/phase4-v11-rust-tests.log`, `.local/phase4-v11-foreign-final.log`.

Spätere manuelle Abnahme: siehe [Fremddatei-Checkliste](FOREIGN_FILES.md).
Die Bestätigung von Zusatzdateien ist kein Test ihrer Spielwirkung. Registrierte
Fremdmods werden nicht mit Workbench-Mods verschmolzen; Originaländerungen brauchen
zuerst die Wiederherstellung einer belegten Basis.

## Phase-4-Prüfungen v0.4.9

**145 Rust-Tests bestanden**, einschließlich vier Echtdateiintegrationen.
Fmt und Clippy mit `-D warnings` erfolgreich. Acht neue Core-Tests prüfen bytegleichen Archiverhalt und Apply/Restore auf der
neuen Basis, sechs Unterbrechungsgrenzen, teilweise neue Sicherungen, Namenskollisionen,
gleich große Änderungen eigener Dateien, fremde Kinder, Quelländerungen, Abbruch,
Doppelbelegung von Archiv/Quelle, Journalmanipulation, Hardlinks, partielle
Projektveröffentlichungen und die integrierte Inventur/Inhaltsprüfung. Synthetische
unbekannte Builds werden vom öffentlichen Zulassungspfad weiterhin abgewiesen.

Fünf neue UI-Flows prüfen Vorschau und Herkunftsbestätigung beim Basiswechsel,
Schreibsperre während des Spielens, gezielte Wiederaufnahme, ungültig gewordene
Auswahl und getrennte Darstellung eigener/fremder Dateien. 39 UI-Flows und
vier Frontend-Unit-Tests bestanden; TypeScript und Frontend-Build erfolgreich.
Ein Windows-Testbefund zu offenen Kinddateien beim Ordnerumbenennen wurde behoben:
Archivdateien werden nach dem Verschieben erneut verifiziert und bis zum Abschluss
gehalten. Ein Regressionstest zur reinen Vorschau bei gesperrtem Ausgabecache
prüft, dass fehlende Live-Zulassungen keine unnötige Schreibberechtigung verlangen.

Release-Build und native Windows-Prüfung bestanden: echte Inventur/gespeicherte
Basis/Live-Vorschau nur gelesen, sieben Live-Sitzungssperren geprüft und bestehende
Mod-/Recovery-Projektproben erneut erfolgreich. Kein Live-Setup, Apply oder
Basiswechsel am echten Spiel, keine Spielsperre. Alle 38 beobachteten Metadaten-
und EXE-Hashes sowie der Nutzerbericht unverändert. Keine JavaScript-Fehler;
1024px-Screenshot geprüft. Nachweise: `.local/phase4-v10-validation.json`,
`.local/phase4-v10-native/result.json`, `.local/phase4-v10-rust-tests.log`.

Spätere manuelle Abnahme nach Abschluss aller Entwicklungsphasen:

- Eigenen Mod anwenden, Inventur öffnen: eigene Gruppen separat, Registry als
  eigener Mod gekennzeichnet; neuer Vanilla-Inhaltsbericht gesperrt.
- Nach abgeschlossenem Update/Steam-Verify neue Basis erfassen. Bei unbekanntem
  Schema keine Anwendung; bei bekanntem Schema Basiswechsel vorschauen und starten.
- Alte Historie und Gruppen im Archiv, neue Original-Registry unverändert;
  Mod neu berechnen, anwenden und anschließend auf neue Basis zurücksetzen.
- Unterbrochenen Basiswechsel ausdrücklich fortsetzen; keinerlei automatische
  Wiederholung beim Öffnen der App oder bei Navigation.

## Phase-4-Prüfungen v0.4.8

**137 Rust-Tests, 4 Frontend-Unit-Tests und 34 Headless-UI-Flows bestanden.**
Vollständiger Rust-Lauf einschließlich vier Echtdateitests; Fmt, Clippy mit
`-D warnings`, Frontend- und Windows-Release-Build erfolgreich. Die 69 bisherigen
Schreibabbruchpunkte bleiben enthalten. Nach Ergänzung korrekter Body-/Headerpaare
in den neuen künstlichen Testarchiven bestanden alle fünf neuen Adaptertests.

Geprüft wurden Apply/Reapply/Restore auf synthetischen Installationen, bytegleiche
Originalquellen und Originaldatenansicht nach Apply, veraltete Vorschauen,
fremde Dateien, gleich große Quelländerungen, fehlende/defekte Backups,
Abbruch mit Handlefreigabe, Recovery vor und nach Registry-Commit und die
Schreibsperre des reinen Inspektionskerns. Ein zusätzlicher Workertest prüft
Einzelauftrag, Sitzungsisolation, Abbruch und korrekt gemeldeten späten Erfolg.

Die fünf neuen UI-Flows prüfen explizite Herkunftsbestätigung, Write-Sperre bei
laufendem Spiel, Anbindung des aktuellen Feldplans, veraltete Freigaben ohne
automatischen Retry, unabhängig verfügbare Rücknahme, Workerabbruch und verspätete
Antworten nach geänderten Modwerten. Alle 34 UI-Flows bestanden im ersten Gesamtlauf.

Native Release v0.4.8: Die echte Live-Einrichtungsvorschau wurde ausschließlich
lesend mit 285 Originaldateien geprüft. Keine Herkunftsbestätigung gesetzt,
keine Einrichtung oder Anwendung ausgelöst. Alle sechs IPCs verweigern ungültige
Sitzungen. Bestehende Projektproben und Restore bestanden; kein JavaScript-Fehler,
1024px-Screenshot geprüft. Spiel lief weiter, kein Live-Transaktionsordner erstellt,
38 beobachtete EXE-/Metadatenhashes sowie ursprünglicher Nutzerbericht unverändert.

Nachweise: `.local/phase4-v9-rust-tests.log`, `.local/phase4-v9-native/result.json`,
`.local/phase4-v9-validation.json`. [Ablauf und verbleibende Grenzen](LIVE_APPLY.md).
Tatsächliche Spielwirkung, beliebige Stromausfälle und beliebige externe Startpfade
sind durch diese Tests nicht bewiesen. Die Live-Einrichtung des Nutzerrechners
ist ausdrücklich nicht erfolgt.

## Phase-4-Prüfungen v0.4.7

**131 Rust-Tests, 4 Frontend-Unit-Tests, 29 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` bestanden. Release-Build und native Windows-
Abnahme erfolgreich; vier Echtdateiintegrationen und bisherige 69
Transaktions-Abbruchpunkte weiterhin enthalten.

Sieben neue Basistests: unveränderter Bericht und passende Registry-Kopie;
fehlende/widersprüchliche Berichtsfelder, doppelte Felder/Pfade, unzulässige
Herkunftsflags, Pfadausbrüche und Größenüberläufe; veraltete Vorschau/Buildwechsel;
Abbrüche nach Berichtskopie, nach Registry-Kopie und vor Manifestveröffentlichung;
Manipulation/Hardlinks; Erhalt alter Sicherung bei Änderungen; geschützte Ziele
und Eingabelimits. Gleiche Dateigröße allein gilt ausdrücklich nicht als erneuter
Inhaltsnachweis; ein entsprechender Negativtest besteht ohne Vertrauensfreigabe.

Vier neue UI-Flows: explizite Übernahme und Wiederöffnen nach Navigation,
Ablehnung geänderter Vorschauen ohne Wiederholungsversuch, Update-/Korruptionsstatus,
verworfene Auswahl und verspätete Antworten. Gesamter UI-Lauf beim ersten
Durchlauf grün. 1024px ohne horizontales Überlaufen.

Native Abnahme übernimmt den echten Nutzerbericht vom 20.09.2026 byteidentisch,
erstellt die passende 679-Byte-Registry-Kopie im Projekt und prüft deren Hash nach
erneutem Öffnen. Vier neue IPCs verweigern veraltete Sitzungen. CLI bestätigt den
persistierten Stand nach Beendigung der eigenen Testinstanz. Nutzerbericht und
38 beobachtete Spiel-EXE-/Metadatenhashes unverändert; das Spiel lief weiter.
Bestehende Mod-/Recovery-Proben bestanden ebenfalls. Keine Spiel- oder Savewrites,
keine Spielsperren, kein erneuter vollständiger Hashlauf der Originalarchive.

Nachweise: `.local/phase4-v8-rust-tests.log`, `.local/phase4-v8-native/result.json`,
`.local/phase4-v8-baseline-status.json`, `.local/phase4-v8-validation.json`.
[Bedienung und Grenzen](BASELINE.md).

## Phase-4-Prüfungen v0.4.6

**124 Rust-Tests und 4 Frontend-Unit-Tests bestanden; 25 verschiedene UI-Flows
erfolgreich geprüft.** Fmt/Clippy mit `-D warnings` und Release-Build bestanden.
Vier echte Spieldaten-Integrationen sind enthalten. Die 69 bestehenden
Transaktions-Abbruchpunkte prüfen jetzt zusätzlich die konkrete Rücknahmevorschau
gegen die Vorher-/Nachher-Hashes und tatsächlich entfernten Dateien.

Sechs neue Core-Tests: fehlendes Backup wird nicht neu erzeugt, Vorschau verändert
keine Inhaltsdateien oder Verzeichnisse, Restore entspricht dem Plan, offene
Transaktionen vor/nach Commit werden richtig eingeordnet, Pläne veralten bei
neuen Transaktionen und sind nicht zwischen Projektkopien übertragbar. Quellupdates,
fremde Dateien, manipulierte Registry/Overlays, beschädigte Sicherung, Hardlinks
und geschützte Pfade werden ohne Bereinigung zurückgewiesen; Listenumfang begrenzt.

Der erste Lauf der neuen Tests scheiterte an einem unvollständigen synthetischen
Tabellenpaar in der Fixture; der nötige Header wurde ergänzt. Im ersten UI-Lauf
bestanden die 21 bisherigen und zwei neue Flows. Zwei neue Testselektoren wurden
korrigiert: `option.disabled` direkt prüfen und den Itemdatenbank-Knopf inklusive
Zähler adressieren. Danach bestanden alle vier neuen Flows. Kein Produktionscode
wurde zur Umgehung dieser Fixture-/Selektorfehler angepasst.

Native Release-Abnahme: Backupstatus und deaktivierter Restore bei bereits
zurückgesetzter Probe; gezielt fehlender Abschlussmarker in einer neuen Kopie
unserer Probe; offene Transaktion korrekt erkannt, über UI wiederhergestellt,
Abschlussmarker byteinhaltlich identisch und anschließende Zustandsprüfung grün.
Alle neuen IPCs verweigern veraltete Sitzungen. CLI-Liste/Vorschau/Restore an
derselben Kopie ebenfalls erfolgreich. 1024px ohne Überlaufen und visuell geprüft;
keine JavaScript-Fehler. Bestehende Modflows und Prozess-/Update-Schutzproben grün.
Spiel lief weiter, 38 beobachtete Spiel-EXE-/Metadatenhashes unverändert.

Nachweise: `.local/phase4-v7-rust-tests.log`, `.local/phase4-v7-native/result.json`,
`.local/phase4-v7-restore-preview.json`, `.local/phase4-v7-cli-recovery.json`,
`.local/phase4-v7-validation.json`. [Bedienung und Grenzen](BACKUP_RECOVERY.md).

## Phase-4-Prüfungen v0.4.5

**Nutzerabnahme der vollständigen Inhaltsprüfung, 20.09.2026:** Exportierter
Desktop-Bericht `exports/installation-audit-1789903405-11284-1-0.json` geprüft.
285 Dateien / 154.097.618.899 Bytes / 195 PAZ-Archive in 140 Sekunden; alle
SHA-1-Vergleiche erfolgreich, Metadaten laut Bericht stabil. Berichtskonsistenz
und Übereinstimmung der 38 zuvor beobachteten SHA-256-Werte bestätigt, ohne
erneuten Zugriff auf Spielinhalte. Nachweis: `.local/user-content-audit-review.json`.
Dies ergänzt die unten beschriebene automatisierte Abnahme; Vanilla-Zertifizierung,
Live-Apply und manuelle In-game-Modultests bleiben offen.

**118 Rust-Tests, 4 Frontend-Unit-Tests, 21 Headless-UI-Flows bestanden.**
Fmt/Clippy mit `-D warnings` fehlerfrei; vier Echtdateitests und bisherige
69 Transaktions-Abbruchpunkte eingeschlossen.

Neue [Inhaltsprüfung](CONTENT_AUDIT.md): bekannte SHA-1-/SHA-256-Vektoren,
Mehrblock-/Leerdateien, gleich große Inhaltsänderungen, Abbruch, Spielstatusfehler
und neuer Spielstart, Änderungen bereits gehashter Quellen, Depot-/Buildwechsel,
Wachstum, zusätzliche/fehlende/hart verlinkte Quellen. Der echte öffentliche
Prozesscheck wird vor Inhaltszugriffen geprüft; eigene Lesehandles erlauben
konkurrierende Writes und erkennen die nachfolgende Metadatenänderung.
Worker-Tests prüfen einen aktiven Auftrag, Sitzungsgrenzen, Abbruch und Panik.
UI-Flows prüfen die Aktivierung nach Metadatenprüfung bei beendetem Spiel,
Fortschritt, Ansichtswechsel, Abbruch, Fehlermeldung, Abweichungen und Berichtsexport.

Ein früher Gesamtlauf traf eine Prozessabfrage nach dem Ende einer gleichnamigen
`probe.exe` aus einem anderen Test. Die Schutzschranke blieb geschlossen; die
betroffenen eigenen Prozess-Fixtures werden nun serialisiert. Der isolierte Test
und der erneute Gesamtlauf bestehen. Die UI-Fixture musste die frischen Objekte
realer IPC-Antworten nachbilden, statt React-Stateobjekte in-place zu verändern.

Native Release-Abnahme v0.4.5 bestanden: neuer Inhaltsknopf während des laufenden
Spiels gesperrt; Start/Abbruch/Export verweigern ungültige Sitzungen. Alle 18
Befehle stimmen mit der rein lokalen Main-Window-Capability überein. Der große
Originalhashlauf wurde nicht gestartet; positive Inhaltsläufe verwenden
synthetische Dateien. Bestehende Modflows und geschützte Projektproben inklusive
Recovery und Updateablehnung bestanden; CLI-Recovery derselben Probe ebenfalls.
1024px ohne Überlaufen, Screenshot geprüft, keine JavaScript-Fehler. Alle 38
beobachteten EXE-/Metadatenhashes nach der nativen Probe unverändert.
Nachweise: `.local/phase4-v6-validation.json`, `.local/phase4-v6-native/result.json`,
`.local/phase4-v6-rust-tests-final.log`.

## Phase-4-Prüfungen v0.4.4

**106 Rust-Tests, 4 Frontend-Unit-Tests, 17 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` fehlerfrei. Vier Echtdateitests wurden ausgeführt;
69 bestehende Transaktions-Abbruchpunkte bleiben enthalten.

Zehn neue Rust-Tests für [Installationsprüfung](INSTALLATION_CHECK.md): alle
Abschneidepositionen einer synthetischen Depotliste, CRC/IDs/Größen/Signaturstatus,
Pfadausbrüche/Gerätenamen/Duplikate, Protobuf-Überläufe, zusätzliche/fehlende Dateien,
Typwechsel, Registrydefekte, fehlende/widersprüchliche Caches, Steam-Updatezustände,
Metadatenlimits, Tiefe und eine Windows-Junction. Gleiche Dateigrößen bei geändertem
Inhalt dürfen niemals als Inhalts- oder Vanilla-Nachweis gelten.

Zwei neue UI-Flows: bewusster Start statt automatischem Scan, zusätzliche EXEs,
Vertrauensgrenze und Wiederholung; sichtbare Fehlermeldung mit erneutem Start.
Die echte CLI-Inventur zählt 285 passende Dateien, drei EXEs und zwei Depots.
Die erste native Abnahme fand eine fehlende Tauri-ACL-Zuordnung für den neuen
Lesebefehl. `build.rs` und die lokale Main-Window-Capability wurden ergänzt;
kein allgemeiner oder Remote-Zugriff hinzugefügt. Die finale Release-EXE bestand
die Wiederholung einschließlich echter Inventur, bestehender Modflows, geschützter
Projektproben und Recovery. 1024px ohne Überlaufen, keine JavaScript-Fehler;
CLI-Recovery derselben v3-Probe ebenfalls erfolgreich. Alle 38 beobachteten
EXE-/Metadatenhashes nach der nativen Probe unverändert.

Nachweise: `.local/phase4-v5-validation.json`, `.local/phase4-v5-native/result.json`,
`.local/phase4-v5-rust-tests-final.log`, `.local/phase4-v5-cli-recovery.json`.


## Phase-4-Prüfungen v0.4.3

**96 Rust-Tests, 4 Frontend-Unit-Tests und 15 Headless-UI-Flows bestanden**;
Fmt/Clippy mit `-D warnings` fehlerfrei. Vier Echtdateitests wurden tatsächlich
ausgeführt. Die 69 bisherigen Transaktions-Abbruchpunkte bleiben eingeschlossen.

- Integrierte Sitzungssperren über Apply/Reapply/Restore/Recovery: EXE-Start,
  Quell-/Backupwrites und Umbenennung relevanter Elternordner gesperrt.
- Bereits laufende eigene Testkopie vor State-/Backuperstellung erkannt; reale
  Prozesspfade auf beiden Seiten kanonisiert. Prozessprüffehler sperren auch
  Recovery und Restore; Handles bleiben bis zum Ende der Sitzung gehalten.
- Abbruch nach Intent-Veröffentlichung mit gehaltenen Sperren, danach Recovery.
- Simuliertes Quellupdate vor Recovery und nach Commit: neue Quellen/Registry
  bleiben bytegleich erhalten, altes Backup und eigene Overlaydateien werden
  nicht über einen fremden/neuen Zustand zurückgeschrieben oder entfernt.
- Manipulierte Schutzmanifeste, v3-Downgrade, beschädigte Backups, Hardlinks,
  Pfadausbrüche und eine behauptete Vanilla-Herkunft werden abgewiesen.
- Echte Händlerdaten: eigene leere Artikelauswahl und Originalrefresh unterdrücken
  globale Werte; ein anderer Händler außerhalb der Auswahl erhält seine eigenen
  Artikel und Tagesrefresh. Alle übrigen Händlerrecords bleiben bytegleich.
- UI: individuelle Auswahl/Refresh übermitteln, anschließend wieder erben und
  Ausnahme entfernen; Schutz-/Updateprobe wird mit ihrem Testumfang angezeigt.

Die ersten Schutztests deckten auf, dass exklusives EXE-Öffnen allein bereits
laufende Images nicht erkennt und Attributzugriff keine wirksame Ordnersperre
hält. Ein weiterer Test erforderte den beidseitig kanonisierten Prozesspfadvergleich.
Nach diesen Korrekturen bestanden die Tests ohne Abschwächung ihrer Anforderungen.

Belege: `.local/phase4-v4-rust-tests.log`, `.local/phase4-v4-mods-tests.log`,
`.local/phase4-v4-protected-tests.log`. Ablauf und Grenzen:
[Geschützte Projektproben](PROTECTED_REHEARSALS.md).
Die finale Release-EXE v0.4.3 bestand außerdem den nativen WebView2-Test mit
echten Daten: 34.488 bisherige Änderungen, geschützte Probezyklen mit Recovery,
Updateablehnung, Tagesrefresh, Tabellenwachstum, Dropchancen und manuelle
Garantien. Die individuelle Händlerauswahl ersetzte die globale Artikelauswahl
korrekt; Export und 1024px bestanden ohne JavaScript-Fehler. Ein zunächst
mehrdeutiger Testselektor für verschachtelte Details wurde korrigiert; dieselbe
EXE bestand danach den vollständigen Lauf. Die neue CLI stellte dieselbe
v3-Probe anschließend idempotent wieder her. Belege unter
`.local/phase4-v4-native/`, `.local/phase4-v4-cli-recovery.json` und
`.local/phase4-v4-validation.json`.

Windows wurde tatsächlich geprüft; keine Linux-Ausführung und weiterhin kein
Live-Apply, keine Vanilla-Zertifizierung und keine In-game-Abnahme.

## Phase-4-Prüfungen v0.4.2 (historisch)

**87 Rust-Tests, 4 Frontend-Unit-Tests und 14 Headless-UI-Flows bestanden.**
Fmt/Clippy mit `-D warnings` fehlerfrei. Der vollständige Workspace-Lauf enthält
vier tatsächlich ausgeführte Echtdateitests; anschließend bestand der erweiterte
Shoptest für alle 6.816 Items bei einem Händler ebenfalls.

- Neue Stockpositionen erhalten neue Listen-/Save-Indizes; bestehende Lücken,
  Preise, Bedingungen und ursprüngliche Positionen bleiben bytegleich.
  Doppelte Save-Indizes und veränderte Listenreihenfolge sperren Ergänzungen.
- Eine reine Ankaufsposition verhindert kein neues kaufbares Angebot desselben
  Items. Buyable-/Sellable-Counts bleiben konsistent.
- Tagesrefresh verändert nur das bestätigte Tagesfeld; Sentinel-Intervalle
  bleiben erhalten. Echte globale Vorschau: genau 246 Änderungen.
- Kleine Ergänzung und vollständiges Sortiment bei Händler 3101: Bodywachstum,
  neu berechneter Header, Rücklesen des verschlüsselten Overlays und alle anderen
  Händlerrecords unverändert.
- Mengen ×3 und Chance ×2 bei Set 175521: Rate 35.000 → 70.000 direkt an den
  erzeugten Bytes geprüft. Exakte Skalierung, Nullfaktor, 100-%-Deckelung und
  breite Zwischenwerte; manuelle Garantie hat Vorrang vor Chancenfaktor 0.
- Ablehnung gewichteter/bedingter Chancenvarianten, Garantie mit Mengenfaktor 0,
  unbekannter Items, ungeeigneter Händler und übergroßer Vorschauauswahl.
- UI überträgt tägliches Auffüllen, zusätzliche Artikel und manuelle Garantien
  gemeinsam; Zurücksetzen setzt die Optionen zurück. Bestehende Export-/
  Probe-/Ausnahmetests und gesperrter Live-Apply bleiben grün.

Die finale Release-EXE v0.4.2 bestand den nativen WebView2-Test mit echten Daten:
bestehender 34.488-Änderungen-Plan, zwei Apply/Reapply/Restore-Zyklen plus Recovery,
246 Tagesrefresh-Änderungen, zwei neue Artikel bei Händler 3101, Chance ×2 und
Menge ×3 in einem gemeinsamen Plan sowie explizite 100-%-Basisrate. Auch die
gewachsene Tabelle bestand zwei Probezyklen plus Recovery; Exporte und 1024px
ohne Überlaufen geprüft. Keine JavaScript-Fehler. Das Spiel lief weiter.

Ein erster nativer Prüflauf scheiterte an falsch kodierten Umlauten im Testskript.
Nach Skriptkorrektur bestand dieselbe unveränderte Release-EXE alle Prüfungen.
Der Launcher schloss jeweils ausschließlich seine eigene unsichtbare Testinstanz.

Nachweise: `.local/phase4-v3-rust-tests.log`,
`.local/phase4-v3-all-items-test.log`, `.local/phase4-v3-native/result.json`
und `.local/phase4-v3-validation.json`. Statische Feldbelege:
[PHASE4_FIELDS.md](research/PHASE4_FIELDS.md).
Live-B0, universelle Händlerabdeckung und In-game-Nachweise bleiben offen.

## Phase-4-Prüfungen v0.4.1 (historisch)

Ergebnis: **83 Rust-Tests, 4 Frontend-Unit-Tests und 13 Headless-UI-Flows grün**;
Fmt/Clippy fehlerfrei. Vollständiger Workspace-Lauf mit 82 Tests plus abschließende
Formatsuite mit 18 statt 17 Tests nach Ergänzung der exakten Längenprüfung.
Die finale Release-EXE v0.4.1 bestand den nativen Test mit 34.488 Änderungen,
Reapply/Recovery, sechs Dateiübergängen, Export und 1024px-Darstellung.

Zusätzliche Prüfungen des neuen privaten Transaktionskerns:

- Zwei verschlüsselte Overlaygruppen, erneute Anwendung geänderter Inhalte,
  alte Dateien bis zum Registry-Commit erhalten, anschließend Restore bytegleich.
- 69 injizierte Abbrüche bei Apply, Reapply und Restore, einschließlich Teilwrites,
  Veröffentlichung der Gruppe, Registry-Commit, Bereinigung und Abschlussnachweis.
- Wiederholter Abbruch während Recovery vor und nach Registry-Commit.
- Schutz gegen belegte IDs, veraltete Pläne, fremde Inhalte, beschädigte Backups,
  parallele Locks, Hardlinks und fehlgeschlagene Prozessprüfungen.
- Prüfung des gesamten PAMT/PAZ-Paars samt exakter dekomprimierter Länge sowie
  Ablehnung doppelter virtueller Tabellenpfade über mehrere Gruppen.
- Öffentlicher Recovery-Befehl verweigert geschützte, falsch platzierte,
  unmarkierte und unbekannt versionierte Proben sowie verlinkte Marker.
- Desktop-Probe mit echten Daten: zwei Apply/Reapply/Restore-Zyklen, Recovery,
  sechs Dateiübergänge; Headless-UI prüft die Anzeige einschließlich entfernter Dateien.

Neue native Nachweise liegen unter `.local/phase4-v2-native/`, Rust-Ausgabe unter
`.local/phase4-v2-rust-tests.log` und `.local/phase4-v2-format-tests.log`.
`mod-recover` wurde auch per CLI an der nativen Probe wiederholt (idempotent,
ursprüngliche Registry erhalten). Der Abschlussvergleich bestätigt unveränderte
Hashes aller 38 beobachteten EXE-/Metadaten-Dateien; kein vollständiger PAZ-Abgleich.
Injizierte Abbrüche sind keine tatsächlichen Stromausfälle oder Nachweise für alle
Dateisystem-/Datenträgerfehler. Live-Startschutz und In-game-Abnahme bleiben offen.

## Phase-4-Prüfungen v0.4.0 (historisch)

Ergebnis: **74 Rust-Tests, 4 Frontend-Unit-Tests und 13 Headless-UI-Flows grün**,
ebenso native Prüfung der finalen Release-App v0.4.0. Fmt/Clippy fehlerfrei.

- Neuer `mods_live`-Test mit tatsächlichen Tabellen: exakte Abdeckung, unveränderte
  Tabellenpaare, deterministisches Overlay, gezielte Händler-/Dropset-Auswahl,
  alle anderen Records bytegleich, drei Friendly-Werte und negative Strafe,
  Sentinel-Ablehnung, unbekannte Optionsnamen und Eingabegrenzen.
- Synthetischer Storetest: unbekannte September-Bytes, optionaler Block und
  Effektliste bleiben bei Bestandsänderung exakt erhalten. Index-Wachstum bewahrt
  Bodypräfix und nichtphysische Header-Keyreihenfolge.
- Overlaytest liest verschlüsselten LZ4-Body/Header über den echten Archivreader
  zurück, prüft Determinismus, Kollisionen, Paare und verbotene Dateinamen.
- Private Transaktionsprobe: sechs Apply-/fünf Restore-Unterbrechungsgrenzen,
  Wiederöffnung/Recovery, beschädigtes Backup, konkurrierender Lock, fremde
  Registry-/Gruppenänderung, Hardlinks sowie laufender/unklarer Prozesscheck.
- Windows-Startschutztest ausschließlich mit einer kopierten eigenen Test-EXE:
  Start bei exklusivem Dateihandle verweigert, nach Freigabe erfolgreich.
- Vier zusätzliche Headless-UI-Flows: explizite Vorschau, stets gesperrter
  Live-Apply, geprüfte Plan-ID beim Export/Probelauf, stale Vorschau, Auswahl und
  Ausnahmen, Eingabefehler, 1024px und Backend-Ablehnung.

`app/scripts/native-mods-smoke.mjs` prüfte die echte Oberfläche in einem separat
gestarteten unsichtbaren Workbench-Fenster: alle Shops 999, Mengen ×2, Trust ×3,
Dateiplan, zwei Probeläufe, Export mit Credits, Trust-Strafe unverändert sowie
Sperre einer veralteten Vorschau. Ergebnis/Screenshots unter `.local/phase4-native/`.
Das Script startet und bedient kein Spiel. Der zugehörige Launcher beendet
anschließend ausschließlich seine eigene Workbench-Testinstanz.

Die Unterbrechungstests sind keine Stromausfall-Simulation. Vollständige Recovery
bei beliebigem Abbruch während Datei-I/O, Live-Reapply, mehrere Gruppen und
integrierter Schutz gegen jeden Launcherpfad sind vor Live-Freigabe noch nötig.

## Phase-3-Nachweis

65 Rust-Tests, 4 Frontend-Unit-Tests und 9 Headless-Edge-Flows bestanden.
`crates/cd-core/tests/crafting_live.rs` prüft den vollständigen Rezept-/Gruppen-
Roundtrip, alle freigegebenen Item-Dropsets, die konkreten Pfeil-/Eintopfbeispiele
und die Planung jedes der 1.108 freigegebenen Rezepte. Fehlende lokale Konfiguration
wird explizit übersprungen; konfigurierte unbekannte Builds schlagen fehl.

`app/scripts/native-crafting-smoke.mjs` prüft die echte App mit den lokalen Daten
in einem unsichtbaren eigenen WebView2-Fenster. Screenshots/Ergebnis:
`.local/phase3-native/`. Debug- und finale Release-EXE bestanden; der bisherige
Itembrowser einschließlich Sprachwechsel wurde danach mit der Release-EXE
erneut geprüft (`.local/phase3-item-regression/`). Kein Spielstart, keine
In-game-Aktion, kein Savezugriff.

Reproduzierbare Befehle: dieselben Workspace-/Frontend-Kommandos unten; zusätzlich
`cargo test -p cd-core --test crafting_live --locked -- --nocapture`.

Manuelle Durchsicht:

- Herstellungsziel Pfeil (50001), Menge 31, Bauholz-Vorrat 3: 7 Bauholz und
  2 Eisenerz fehlen; 60 Pfeile werden hergestellt und 29 bleiben übrig.
- Gemischten Eintopf wählen, Rezept 343, Meeresfrüchtealternative wechseln:
  Baum, Restbedarf und Vorratsliste müssen dasselbe gewählte Item verwenden.
- Zwischenprodukt vorhanden / manuell beschaffen, Rezepte wechseln, Vorräte leeren.
- Über Itemdetails → Verknüpfungen → Herstellen zum Rechner wechseln.
- Quellenlücken, unbekannte Chancen und Zykluswarnungen nicht als gesicherte
  Weltfundorte, 0 % oder kostenloses Herstellen missverstehen.

Linux wurde lokal weiterhin nicht ausgeführt. Die Zahlen der früheren Phasen
unten sind historische Nachweise.

## Phase-2-Nachweis

```powershell
npm ci --prefix app
npm run build --prefix app
npm test --prefix app
npm run test:e2e --prefix app
cargo test --workspace --locked -- --test-threads=2 --nocapture
cargo clippy --workspace --all-targets --locked -- -D warnings
```

57 Rust-Tests, 4 Frontend-Unit-Tests und 6 Headless-Edge-Flows bestanden unter
Windows. Der neue Echtdateitest prüft deutsche FTS-Suche, vollständige Stringinfo-
Roundtrips, tatsächliche DDS-Icondekodierung, Stats, Pagination, Rohfelder und
JSON-Export in einem isolierten temporären Projekt. Keine Ausgabe ins Spiel.

Zusätzlich lief die gebaute Tauri-App mit echtem WebView2 in einem **unsichtbaren
eigenen Testfenster**, nicht im Browser-Mock: reales Item 2200, Icon, Rohfelder,
JSON-Ausgabe und Sprachwechsel ger → eng bestanden, keine JavaScript-Fehler.
Prüfer: `app/scripts/native-smoke.mjs`; Ergebnisse/Screenshots unter
`.local/phase2-native/`. Der eigene Testprozess wurde anschließend geschlossen.

Manuelle Durchsicht, ohne In-game-Eingriff:

- App starten; automatische Erkennung oder Pfadauswahl prüfen.
- `Stumpfpfeil` suchen, Icon/Name/Felder ansehen; mehrere Filter kombinieren.
- Liste weit scrollen, Sprachwechsel, unbekannte Felder und Itemlinks ausprobieren.
- JSON speichern und den angezeigten `exports/`-Pfad prüfen.
- Fehlende Händler-/Drop-/Rezeptbeziehungen sind sichtbar als offen markiert und
  werden nicht als „keine Quellen vorhanden“ interpretiert.

Linux-CI ist vorbereitet, hier nicht ausgeführt. Die folgenden Phase-1-Zahlen
bleiben der damalige Nachweis; der aktuelle Gesamtstand steht oben.

## Phase-1-Nachweis und Wiederholung

```powershell
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --locked -- -D warnings
cargo test --workspace --locked -- --test-threads=2 --nocapture
cargo build --release --locked -p cd-cli
.\target\release\cd-cli.exe roundtrip --all-fields
.\target\release\cd-cli.exe search Stumpfpfeil
```

51 Tests bestanden unter Windows, darunter der tatsächlich ausgeführte Test
gegen die lokale Installation. Die CLI bestätigte 34 Bytevergleiche und die
Feldabdeckung aller 6.816 Items. Release-CLI-End-to-End-Prüfungen kontrollierten
auch FTS5, Cache-Wiederverwendung, Sprachwechsel, Builddiff, unbekannte Builds und
Dateiausgabeschutz. Logs bleiben unter `.local/` und werden nicht eingecheckt.

Der Echtdateitest liest `.env` im Projektroot und anschließend die Spielarchive.
Fehlt `CD_GAME_DIR`, meldet er **SKIP**; `--nocapture` macht das im Log sichtbar.
Ein gesetzter, aber ungültiger oder veränderter Build ist ein Fehler, kein Skip.
Es werden keine Originaldateien zurückgeschrieben oder Saveinhalte geöffnet.
Synthetische Fehler-/Pfadtests nutzen temporäre, isolierte Verzeichnisse.

GitHub Actions prüft Windows und Ubuntu; Linux wurde lokal nicht ausgeführt.
Frontendtests beginnen mit dem Frontend in Phase 2. Der 13-fache Rohdatenbody-
Roundtrip ist keine semantische Editorfreigabe; PALOC-Neukompression ist nicht
als byteidentisch nachgewiesen.

## Ausführungsgrenze

Der Nutzer spielt während der Entwicklung. In Phase 0 und 1 bleiben Spielverzeichnis
und Spielstände unverändert; auch Steam-Dateiprüfung, Apply, Restore und
Spielneustart werden nicht automatisch ausgelöst. Manuelle Spieltests
führt der Nutzer gemäß seiner späteren Entscheidung erst nach Abschluss der
Entwicklung aller Phasen durch.

## Phase 1: Core und Parser

- `cargo test --workspace`: synthetische valide/ungültige Daten, gekürzte Header,
  ungültige Längen/Offsets/Referenzen, unbekannte Builds und Obergrenzen testen.
- Reale Eingaben ausschließlich aus `CD_GAME_DIR` und `CD_SAVE_DIR` in `.env`;
  fehlende Installation ausdrücklich als übersprungen ausgeben. Ein früher Return
  eines Referenztests zählt nicht als bestandener Test mit echten Spieldaten.
- Jeden unterstützten Tabellenbody und seinen Header parsen und serialisieren:
  Bytevergleich, nicht nur gleiche Datensatzzahl oder gleiches JSON. Auch unbekannte
  Felder, Padding und Reihenfolge müssen erhalten bleiben.
- Für lossless bezeichnete Formate Hash und vollständige Leseposition prüfen.
  Ein unveränderter Rohbyteblock allein belegt keine korrekte Feldinterpretation.
- Lokalisierung mit deutschen Texten, Umlauten, fehlenden Schlüsseln und der
  aktuellen LZ4-Hülle prüfen. Suchindex einschließlich FTS5 nach Buildwechsel
  invalidieren und deterministisch neu aufbauen.
- Lesecode erhält keine beschreibbaren Spiel-/Save-Handles. Tests nutzen nur
  isolierte Testordner für ihre Ausgaben. Linux-Kompilation in CI separat prüfen.

## B0: vor dem ersten Modul-Apply erforderlich

- Unbekannter Build, fremde Hashes, ungeprüftes Backup, unzureichender Speicher,
  fehlende Rechte und laufender/nicht zuverlässig prüfbarer Spielprozess müssen
  vor der ersten Änderung abbrechen. Dies gilt auch für Restore und Recovery.
- Dry Run darf keine Spieldatei ändern. Er nennt jede Datei, fachliche Änderung,
  Vorher-/Nachher-Hashes, Overlay-ID und Konflikte.
- Overlay in isolierter Kopie erzeugen und mit einem unabhängigen Reader prüfen:
  Registry, PAMT, Checksums, PAZ-Offsets, Kompression, Body-/Headerkonsistenz.
- Vanilla → Apply → Restore: alle betroffenen Originaldateien bytegleich;
  anschließend Apply → Restore → Apply mit deterministischem Ergebnis testen.
- Einstellungen ändern/Module deaktivieren: Neuberechnung aus Vanilla; keine
  Multiplikation bereits modifizierter Werte, keine zurückbleibenden Module.
- An jeder Transaktionsgrenze Crash/Abbruch simulieren: nach Backup, einzelnen
  Stagingdateien, Overlaybereitstellung, Registrywechsel und Verifikation.
- Gleichzeitiges Apply, Spielstart während Commit und fremde Änderungen zwischen
  Vorschau und Commit testen. Prozessprüfung allein schützt diesen Ablauf nicht.
- Update nach Apply simulieren; niemals Registry/Backups eines alten Builds über
  neue Spielarchive schreiben. Sicher bereinigen oder mit genauer Diagnose sperren.
- Endgültiger manueller Nachweis: Spiel mit kleinster reversibler Änderung starten,
  Hauptmenü/Save laden, Wirkung prüfen, Spiel schließen, Restore, erneut starten.
  Keine Freigabe allein aufgrund einer formal gültigen Overlaydatei.

## Manuelle Modulprüfungen (alle noch offen)

Je Modul zuerst aktuelle Vanilla-Werte dokumentieren, nur dieses Modul aktivieren,
Vorschau prüfen, Spiel schließen, Apply durchführen, Spiel starten und beobachten.
Danach Spiel schließen und Restore samt Hashvergleich ausführen. Kombinationen
erst nach erfolgreichen Einzeltests. Spielstände werden durch Workbench nie
geschrieben; für Tests benutzt der Nutzer bei Bedarf eine eigene Sicherung.

| Modul | Im Spiel zu prüfen |
|---|---|
| B1 Shops | Ausgewählte Händler und Artikel, Preis unverändert falls nicht editiert, Bestand 999, Tageswechsel/Refresh, Kauf mehrerer Slots, ausgeschlossene und Questartikel; große Listen auf UI-/Enginegrenzen prüfen. |
| B2 Drops | Ausgewähltes Dropset vor/nach Änderung; Chance/Anzahl und Gruppensemantik getrennt prüfen. 100 % an bestätigter Auswahl, keine unbelegte Bossklassifikation; statistische Tests für Zufallsdrops. |
| B3 Trust | Derselbe NPC und dieselbe Aktion vor/nach Änderung; gemessener Vertrauenszuwachs, Grenzen/negative oder Sonderwerte, keine unbeabsichtigten Itemdrop-Änderungen. |
| B4 World | Spawnanzahl, Ablauf des Respawn-Timers, mehrere Regionen, Mounts in Städten, Ridedauer/Cooldown; Missionen, Duelle, Zwischensequenzen und Interaktionen ausdrücklich prüfen. |
| B5 Dragon | Blackstar rufen, erneut rufen, Ablauf der Ridedauer und Überschreiten mehrerer Regionsgrenzen; Spezialzonen und missionsbedingte Sperren getrennt prüfen. |
| B6 Inventory | Inventar und Lager getrennt: freie Zahl, Kaufen/Beute/Umsortieren, Speichern/Neuladen, Grenzwerte. Kein Maximalwert als sicher freigeben, bevor er empirisch belegt ist. |
| B7 Stacks | Konsumgüter und Materialien stapeln/teilen/kaufen/lagern/neuladen. Equipment mit Haltbarkeit, Verzauberung oder Sockeln ausschließlich im ausdrücklich experimentellen Test; Instanzwerte dürfen nicht verloren gehen. |
| B8 Durability | Tatsächlichen Haltbarkeitswert mehrerer Waffen/Werkzeuge vor und nach Belastung vergleichen: akzeptiert wird kein Verlust, nicht bloß hohe Resthaltbarkeit durch max_endurance=65535. Reparaturpreis/-aktion getrennt; negative/ungültige Zustände ausschließen. |
| B9 Stamina & Spirit | Sprinten, Klettern, Fliegen und Skills getrennt; Faktoren 0/0,5/1 sowie unbegrenzter Modus, UI-Anzeige und tatsächlichen Verbrauch prüfen. |
| B10 Skills | Globaler und einzelner Cooldown, Überschneidungen, mehrere Skilltypen, numerische Sonderwerte; Effekte anderer Werte nur nach bestätigter Semantik testen. |
| B11 God-item | Einzelnen bestätigten Stat/Buff ändern, dann mehrere hinzufügen; Anzeige und tatsächliche Wirkung, Ausrüsten/Neuladen, leere/mehrfach belegte Arrays, Größenänderung und Headeroffsets. Inkompatible Weapon-Condition-Passives auf Nicht-Waffen vor Apply ablehnen und diese Ablehnung automatisiert testen; dokumentierter Kandidat für Endlosladen/starken Speicherverbrauch. |
| C Profile | Normal ↔ Farm mit Drops ×10/Spawns ×3; deaktivierte Module verschwinden, wiederholtes Apply identisch, laufendes Spiel sperrt Änderungen, JSON-Importfehler beschädigen kein Profil. |

Zusätzliche v0.4.2-Abnahme für B1/B2, weiterhin offen:

1. Kleines Artikelsortiment nur bei einem unterstützten Händler: neue Artikel
   kaufbar, alter Bestand unverändert, Preise plausibel, keine doppelten Angebote.
2. Kauf → Tageswechsel → erneuter Besuch: Auffüllen normaler Angebote; einmalige
   Angebote bleiben einmalig. Speichern/Neuladen durch den Nutzer prüft, dass neue
   Save-Indizes keine anderen Warenpositionen verändern.
3. Pro Händler eigene Artikel setzen, mit leerer Liste globale Ergänzungen
   unterdrücken und Originalrefresh erhalten; explizite Ausnahme außerhalb der
   globalen Auswahl und anschließendes Zurücksetzen auf Vererbung prüfen.
   Erst danach größere Artikelsets; Sonderitems und Oberflächengrenzen prüfen.
   Alle Items bei allen Händlern ist in dieser Version nicht freigegeben.
4. Kleines bekanntes Rolltyp-0-Set: Chance ×2 und Menge ×3 getrennt und kombiniert,
   dann manuelle Basisrate 100 %. Übergeordneten Trigger und mögliche
   Laufzeitmodifikatoren getrennt betrachten; kein automatisch erkannter Boss.
5. Chance 0 ohne Garantie: kein entsprechender Eintrag; mit expliziter Garantie
   hat diese Vorrang. Mengenfaktor 0 plus Garantie muss vor Export abgewiesen werden.

## Weitere Funktionen

A1: Suche, Kategorien, Deutsch/Fallback, unbekannte Felder als Offset/Typ/Rohwert,
verifizierte Querverweise. A2: alternative Rezepte, Zyklen, Ausgabemengen, Rundung
und gemeinsam verbrauchter eigener Bestand. A4: Save wird beim Speichern nicht
blockiert; unvollständiger Lesezeitpunkt wird erneut gelesen, letzter gültiger
Stand bleibt sichtbar; Questfortschritt gegen bekannte Spielsituationen prüfen.
A4 zusätzlich: falsches HMAC, unbekannte Saveversion und unvollständige Dateikopie
müssen als ungültiger Lesestand erkannt werden, nicht als 0 % Fortschritt.
A3: Transform aus mehreren bekannten Punkten kalibrieren, an unabhängigen
Punkten prüfen; fehlende Positions-/Sammelzustände sichtbar als unbekannt führen.
