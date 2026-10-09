# Crimson Workbench – Fortschritt

Stand: 23.09.2026. **Phase 5: App v0.5.9; Reparaturadapter als Entwicklungsmodul 0.23.0.
Allgemeine NPC-Respawnzeiten und kostenlose Reparatur bleiben technisch offen.**
Phase 4 ist in ihrem dokumentierten Umfang entwickelt. Maßgeblich bleibt die
aktualisierte `SPEC.md`; Spielabnahmen sind auf Nutzerwunsch zurückgestellt.
Die Reparaturentwicklung verändert weder laufendes Spiel noch Saves. Der separate,
ausdrücklich beauftragte Offline-Inventarauftrag wurde bei geschlossenem Spiel
ausgeführt; siehe den eigenen Eintrag unten.

**Nutzerentscheidung vom 20.09.2026:** Manuelle Spieltests erfolgen erst nach
Abschluss der Entwicklung aller Phasen. Offene In-game-Abnahmen werden separat
geführt und blockieren den weiteren Entwicklungsfortschritt nicht. Automatisierte
Tests und Schutzprüfungen bleiben erforderlich; eine verschobene Abnahme gilt
nicht als bestanden. Während des Spielens weiterhin keine Änderung oder Sperre
an der laufenden Installation.

## Separater Inventarauftrag: offline abgeschlossen

Slot 2 bei geschlossenem Spiel gesichert und um 200 leichte/200 schwere
Kupferbeutel sowie 30.000 Silber ergänzt. Vier gültige 100er-Stapel in freien
Plätzen, eindeutige IDs und angepasster Lobby-Zähler. Neuer Geldbestand 30.231,01
Silber. Nur zwei Dateien geändert, acht weitere unverändert. Geänderte Dateien
erneut geöffnet und HMAC-geprüft; Anzeige/Verwendung und erneutes Speichern im
Spiel sind ungeprüft. Kein allgemeiner Save-Writer in der App und kein Abschluss
von Phase 7. [Vollständiger Nachweis](../.local/inventory-currency/RESULTAT.md).

## Reparatur 0.23.0: Inventar-Paketserializer und Streamauswahl

- Zwei vollständige Originalfunktionen integriert; eigener 0x300-Byte-TLS-Kontext.
- 26 Direktfälle für Feldbreiten, Kennungen, Empfänger, Streamauswahl und
  Buffer-/Schreibfehler; zehn Transaktionen mit nativem Client-Ack.
- Kein Reparaturerfolg bei falschem Actor, UID-Wechsel, innerem Ack-Fehler oder
  fehlender/doppelter Zustellung. Das Paket enthält weder Item-UID noch Sockel-
  oder Auftragsdaten. Erfolgreicher Poolzweig und realer Transport bleiben offen.
- MSVC Release `/W4 /WX`, neun CTest-Suiten und 454 native Szenarien bestanden:
  4.812 gezählte Aufrufe, 206.786 Bedingungen. App und Produktionssuiten unverändert.
- Spieler-/Manager-/Threadbindung, tatsächliche Serverreparatur, Speicherung,
  Eingabe, Loader und B0 fehlen weiterhin. [Nachweis](../runtime/repair/INVENTORY_PACKET_2949.md).

## Reparatur 0.22.0: native Aufbereitung und Kompression

- Neun vollständige Originalfunktionen für Pufferaufbereitung, Allokation und
  Kompression hinzugefügt. Benachbarte CRT-Thunks werden privat ohne Überlappung gebunden.
- 66 neue native Fälle: Größen bis 131.072 Byte mit/ohne Kompression, Ausgabe-
  Wiederverwendung, Fehler, 14 Dateihelfer-Integrationen und zwei Wiederholungen.
  Unabhängiger LZ4-Testdecoder rekonstruiert die ursprünglichen Bytes.
- Nach einem Öffnungsfehler erneut aufgerufene Einträge werden nochmals komprimiert;
  ihre Größenangabe bezieht sich dann auf den schon komprimierten Inhalt. Neue
  Versuche müssen frische Rohdaten erzeugen. Keine Wiederholung freigegeben.
- MSVC Release `/W4 /WX` und neun CTest-Suiten bestanden. Insgesamt 418 native
  Szenarien, 4.289 Aufrufe und 204.378 Bedingungen. App und Produktionssuiten unverändert.
- Nachgelagerte Verarbeitung bleibt ein Callback; Integrität/Verschlüsselung,
  vollständige Serialisierung, Queue-/Backupweg, Spielhost und Abschlussbeleg fehlen.
  [Nachweis](../runtime/repair/SAVE_ENCODING_2949.md),
  `.local/repair-runtime-v22-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.21.0: originaler Dateischreibhelfer

- Drei vollständige Originalfunktionen für Schreiben und Bereinigung eingebunden;
  Dateioperationen und Encoder sind private Callbacks. Keine Save-Zugriffe.
- 34 neue native Fälle prüfen Vorbereitungs-/Öffnungs-/Schreibfehler, fehlgeschlagene
  Flush-/Close-Aufrufe, leere/große Puffer, Stringvarianten und beide Allokationsmodi.
  Acht davon verbinden Helfer und Originaldispatcher über eine eigene Eintragsbrücke.
- Fehlgeschlagene Flush-/Close-Aufrufe verhindern weder Erfolg noch Payload-Freigabe;
  der Dispatcher kann ebenfalls Dirty löschen. Deshalb keine Speicherbestätigung.
- MSVC Release `/W4 /WX` und neun CTest-Suiten bestanden. Insgesamt 352 native
  Szenarien, 4.095 Aufrufe und 142.649 Bedingungen. App und Produktionssuiten unverändert.
- Echter Encoder und WriteFile-Wrapper statisch weiter zugeordnet. Der Encoder
  kann den Eingabepuffer bereits vor dem Dateiöffnen ersetzen; sein Verhalten,
  vollständige Serialisierung, Queue-/Backupweg und Abschlussbeleg bleiben offen.
- Phasenübersicht und Testcheckliste aktualisiert. [Nachweis](../runtime/repair/SAVE_FILE_2949.md),
  `.local/repair-runtime-v21-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.20.0: Speicherdispatcher und Warteschlangen

- Originaldispatcher, Steam-Zulassungsprüfung und Queue-Marker mit sechs weiteren
  Codebereichen auf privaten Objekten ausgeführt. 39 neue Fälle prüfen Skip-/Status-
  Kombinationen, vier Fehlerstufen, Wiederholung, Quota-Grenzen und Queue-Wachstum.
- Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Insgesamt 318 native
  Szenarien, 4.039 Aufrufe und 141.411 Bedingungen; Produktionssuiten unverändert.
- Konkreten Datei-Backendtyp, drei Dateimethoden und drei Timer statisch zugeordnet.
  Der originale Dateihelfer prüft Öffnen/Schreiben, ignoriert jedoch das Ergebnis
  von `FlushFileBuffers`. Seine native Prüfung und tatsächliche Persistenz fehlen.
- Datei- und Plattformempfänger sind eigene Testcallbacks. Statuswechsel und
  geleerte Warteschlangen bestätigen keinen Datenträgerabschluss. Serverseitige
  Inventarmeldungen, vollständige Spielanbindung und Installation bleiben offen.
- App v0.5.9, Installation und Saves unverändert. Phasenübersicht und Testcheckliste
  aktualisiert. [Nachweis](../runtime/repair/SAVE_BACKEND_2949.md),
  `.local/repair-runtime-v20-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.19.0: native Item-/Speicherobjekt-Umwandlung

- 24 zusätzliche vollständige Originalfunktionen gepinnt; Konverter, Konstruktion,
  Vektorwachstum, Reset und Destruktion auf eigenen Objekten ausgeführt.
- 38 neue native Fälle in beiden Allokationsmodi. Darunter 14 Reparaturplan-/
  Nachherkopie-Integrationen, Hauptwert 0/beschädigt/voll/Sentinel, No-Wear,
  freie/teilbelegte Sockel und drei Färbedatensätze. Wiederverwendung entfernt
  alte Identitäten, Sockel und Färbedaten; Quellen bleiben unverändert.
- Lader normalisiert Hauptwerte und erzeugt fünf Sockeldatensätze. Seine privaten
  Richtlinienwerte werden nicht mit tatsächlich laufenden Spieleinstellungen gleichgesetzt.
- MSVC Release `/W4 /WX`, neun CTest-Suiten und insgesamt 279 native Szenarien,
  3.950 Aufrufe und 139.784 Bedingungen bestanden. Produktionssuiten unverändert.
- Phasenübersicht und Testcheckliste aktualisiert; späterer Spieltest R14 ergänzt
  Färbedatenerhalt nach Reparatur/Neuladen. Regulärer Save-Abschluss, zusätzliche
  optionale Daten, vollständige Ereignisse und Spielhost bleiben offen.
- App v0.5.9, Installation und Saves unverändert. Keine Reparaturfreigabe.
  [Nachweis](../runtime/repair/ITEM_SAVE_2949.md),
  `.local/repair-runtime-v19-{build-test.log,native-result.json,validation.json}`.
- Übergeordneten Komponentensammler und Backendverteiler anschließend statisch
  zugeordnet: sechs vollständige Funktionshashes, drei VTables, drei Sprünge und
  14 Instruktionsanker. Auch der Backendverteiler kann bei übersprungener Arbeit
  true liefern. Kein weiterer nativer Erfolg oder Speicherabschluss gezählt.

## Reparatur 0.18.0: SQL-Erfolg ist keine Speicherbestätigung

- Vollständigen SQL-Ausführungs-Shim mit allen fünf Unwindfragmenten sowie
  den originalen Request-Erwerb gepinnt und auf privaten Daten ausgeführt.
  Der Umgehungszweig liefert 0 ohne Request-Zugriff; der andere Zweig setzt
  Statusfelder, steuert Diagnoseausgaben und liefert 1. Kein Datenbankaufruf.
- 19 neue native Szenarien prüfen Fristen, alte Fehler, Nullzeiger und die
  Verbindung mit dem originalen Ausrüstungs-Slot-Verbraucher. Ein erfolgreicher
  Status bei bereits geleerter Liste bestätigt weiterhin keine Persistenz.
- Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Gesamt: 241 native
  Szenarien, 3.674 Aufrufe, 128.415 Bedingungen; Produktionssuiten unverändert.
- Nachgelagerte Ergebnisverarbeitung ruft bei Schalter 1 eine virtuelle
  Transaktionsmethode auf; der konkrete `ItemPopInventory` meldet dort ebenfalls
  nur Erfolg. Der tatsächliche Spiel-Schalterwert ist unbekannt.
- Separaten Item-/Speicherobjekt-Konverter samt Gegenweg zugeordnet: Hauptwert
  und Sockelhaltbarkeit werden übertragen. Sieben Funktionshashes, vier RTTI-Typen
  und 15 Instruktionsanker statisch geprüft. Native Umwandlungsproben und tatsächliche
  Speicherung stehen noch aus. [Statischer Nachweis](../runtime/repair/ITEM_SAVE_2949.md).
- Phasenstatus und Testcheckliste aktualisiert. R06 verlangt dokumentierte
  Werte vor/nach Reparatur und nach regulärem Neustart für Hauptitem und Sockel.
  App, Installation und Saves unverändert; Phase 5 bleibt offen.
- [Nachweis](../runtime/repair/SQL_DISPATCH_2949.md),
  `.local/repair-runtime-v18-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.17.0: originaler Inventar-Client-Ack

- Originale Client-Rückmeldung für mitgeführte Items mit privaten Inventarquellen
  und dem gemeinsamen Transaktionsadapter verbunden. Der Ack setzt den Hauptwert
  absolut; Sockel bleiben unverändert. Formatierung unter Sperre und UI-Aufruf
  nach Freigabe mit noch gehaltenen Besitzerreferenzen geprüft.
- **36 neue native Fälle:** 28 Direktfälle und acht Integrationsfälle. Ungültige
  Slots, ausgetauschte UID, fehlende/doppelte Zustellung und Abschlussfehler geprüft.
  Der Ack selbst prüft weder UID noch Zielwertzulassung. Der äußere Paketverteiler
  ignoriert seinen Fehlercode; dieser äußere Status darf keinen Erfolg bestätigen.
- **Geprüft:** neun CTest-Suiten, MSVC Release `/W4 /WX`; insgesamt 222 native
  Szenarien, 3.526 Aufrufe und 125.524 Bedingungen. Bestehende Produktionssuiten
  unverändert. Name, UI, Inventar-Server und Persistenz sind Testabhängigkeiten.
- Socket-SQL-Helper und beide Aufrufer nur lesend zugeordnet: gespeichert wird
  eine ItemInfo-Kennung; die Haltbarkeit steht separat und wird nur im nachfolgenden
  Speicherabbild gesetzt. Kein belegter Persistenzweg für Sockelhaltbarkeit.
- **Offen:** Inventar-Serverweg, tatsächliche Persistenz-/Ladesemantik, echte
  UI/Effekte/Transport/Rückmeldungen, Host, Eingabe und Loader/B0. Phase 5 bleibt offen.
- [Nachweis](../runtime/repair/INVENTORY_EVENTS_2949.md),
  `.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`.
  Phasenübersicht/Testcheckliste aktualisiert; Desktop-App, Spielinstallation und Saves unverändert.

## Reparatur 0.16.0: Slot-Verarbeitung und Batchabschluss

- Der Transaktionsadapter verlangt einen abschließenden Aufruf nach allen
  Itemmeldungen. Fehlende Bindung verhindert jedes Schreiben; spätere Fehler
  ergeben einen unklaren Auftrag ohne Wiederholung. Referenzen und Kopien bleiben
  während des Abschlusses gültig, Besitzersperren sind bereits freigegeben.
- Originalen Ausrüstungs-Slot-Verbraucher eingebunden. Er sammelt UID und
  Haupthaltbarkeit, leert die Markierungen und übergibt danach den Auftrag an
  die Persistenzschnittstelle. Diese Schnittstelle bleibt im Test künstlich.
- Originales Clear mit Wiederverwendung und endgültige Tabellenfreigabe ersetzen
  die manuelle Bereinigung privater Testkomponenten. Beide Allokationswege geprüft.
- **Geprüft:** neun CTest-Suiten, MSVC Release `/W4 /WX`; 43 Ablaufszenarien mit
  273 Bedingungen, 24 neue Verbraucherfälle und 18 integrierte Ausrüstungsfälle.
  Gesamt: 186 native Szenarien, 2.826 Aufrufe und 124.527 Bedingungen; viele davon
  wiederholte Tabellenprüfungen. Fehler nach erfolgreichem Client-Ack erzeugen
  keinen Erfolg und keine Wiederholung trotz bereits leerer Slot-Liste.
- **Offen:** tatsächliche Persistenz und ergänzender Sockelweg – der untersuchte
  Verbraucher überträgt keine Sockeldatensätze. Konkrete Inventarereignisse, echte
  Effekte/Transport/UI, Rückmeldezuordnung, Host, Eingabe und Loader/B0 fehlen.
- Phasenübersicht und Testcheckliste aktualisiert. R06 verlangt auch normales
  Beenden/Neustarten und erneuten Vergleich von Hauptitem und jedem Sockel.
  App v0.5.9 unverändert; kein Spielprozess-/Savezugriff, keine Installationsänderung.
- [Nachweis](../runtime/repair/PERSISTENCE_2949.md),
  `.local/repair-runtime-v16-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.15.0: vorbereitete Kopien und gemeinsamer Ablauf

- Neue Bibliothek `crimson_repair_transaction`: alle Meldungswege vorab prüfen,
  beide nativen Nachherkopien jedes Items vorbereiten, dann Feldschritt, Markierung,
  Entsperren und Meldungsübermittlung ausführen. Referenzen bleiben bis zum Ende
  gültig; der Auftrag wartet danach auf frische Daten und zugeordnete Rückmeldungen.
- Zusatzdaten vor nativen Kopieraufrufen auf Lesbarkeit und Allokationsbudget
  prüfen. Teilfehler führen ohne Wiederholung zu einem unklaren Ergebnis.
- Gespeicherte Sockelanzahl und logische Platzgrenze getrennt; freie Plätze
  benötigen keinen gelesenen Datensatz. Vertauschte Zusatzvektor-Strides in der
  Überschneidungsprüfung korrigiert und mit gezielter Regression geprüft.
- Die 15 nativen Ausrüstungsfälle nutzen den gemeinsamen Adapter. Originale
  Kopien, Schreiber, Slot-Markierung und Server-/Client-Ereignisse verbunden;
  Bestätigung liest unter neu erworbenen Besitzern. Empfänger bleiben Testcallbacks.
- **Geprüft:** neun CTest-Suiten, MSVC Release `/W4 /WX`; 40 Ablaufszenarien mit
  247 Bedingungen, 35 Schreibszenarien mit 202 Bedingungen, 32 native Itemfälle.
  Gesamt: 159 native Szenarien, 2.023 Aufrufe, 119.953 Bedingungen. Zähleränderungen
  durch den ersetzten manuellen Aufbau und ungespeicherte Sockel dokumentiert.
- **Offen:** konkrete Inventarereignisse, Slot-Verbraucher/Bereinigung, echte
  Effekt-/Transport-/UI-Verarbeitung und Rückmeldezuordnung, Host, Eingabe, Loader/B0.
- Phasenübersicht und Testcheckliste ergänzt, insbesondere R13 für freie Sockelplätze.
  App v0.5.9 unverändert; kein Spielprozess-/Savezugriff, keine Installationsänderung.
- [Nachweis](../runtime/repair/TRANSACTION_2949.md),
  `.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.14.0: Feldschreiber für vollständige Reparaturbatches

- Neue Bibliothek `crimson_repair_writer`: frische Erfassung unter gehaltenen
  Besitzersperren, vollständige Vorprüfung und tatsächliches Schreiben der
  geänderten Haltbarkeitswörter in Inventar/Ausrüstung und beiden Zustandskopien.
- Adressen stammen aus der Erfassung; unzulässige Kapazitäten, Speicherüberlappung,
  unbeschreibbare Felder und ein veralteter späterer Eintrag stoppen den ganzen
  Batch vor dem ersten Store. Unveränderte Wörter werden nicht geschrieben.
- Der komplette Lesebestand wird vorher/nachher verglichen, einschließlich leerer
  Slots und Metadaten. Teilfehler ergeben ein unklares Ergebnis ohne automatische
  Wiederholung; veraltete Vorherdaten sind nach begonnenem Schreiben verborgen.
- **Geprüft:** acht CTest-Suiten, MSVC Release `/W4 /WX`; neue Schreibsuite mit
  34 Szenarien/197 Bedingungen, einschließlich 2.048 Items und 4.096 Stores.
  Acht neue native Integrationsfälle mit tatsächlichen privaten Speicheradressen
  und originaler Referenz-/Sperrverwaltung. Gesamt: 141 native Szenarien,
  1.408 Aufrufe und 120.069 Bedingungen.
- **Offen:** Feldschritt mit vorab vorbereiteten nativen Nachherkopien und
  vollständigen Inventar-/Ausrüstungsereignissen verbinden. Slot-Verbraucher,
  tatsächliche Effekte/Transport/UI, Host, Eingabe, Loader und B0 fehlen weiterhin.
- Phasenübersicht und Testcheckliste aktualisiert; App v0.5.9 unverändert.
  Keine Spielprozess-/Savezugriffe oder Schreibzugriffe auf die Installation.
- [Nachweis](../runtime/repair/FIELD_WRITER_2949.md),
  `.local/repair-runtime-v14-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.13.0: Slot-Markierung und durchgehend gehaltene Erfassung

- `HeldCapture` hält beide Referenzen und Besitzersperren nach der Erfassung für
  Planung/Vorprüfung. Refresh prüft die aktuelle Sitzung und alle Lesedaten erneut;
  Fehler verwerfen alte Ergebnisse und geben Sperren vor Referenzen zurück.
- Originale Markierung geänderter Ausrüstungsslots vollständig ausgeführt,
  einschließlich Platzsuche, Kollisionen, Wachstum und Bucket-Verkettung. Beide
  TLS-Allokationswege auf einem privaten Heap mit 32-Byte-Ausrichtung geprüft.
- Die 15 Ausrüstungsereignisfälle markieren nun unter beiden Sperren, bevor die
  Benachrichtigung auf derselben privaten Komponente nach dem Entsperren läuft.
  Die eigene Itemkopie und tatsächliche Actor-Referenzen bleiben dabei erhalten.
- **Geprüft:** sieben CTest-Suiten, MSVC Release `/W4 /WX`, 519 Leserbedingungen;
  133 native Szenarien, 1.296 Aufrufe und 119.997 Bedingungen. Der hohe Zähler
  enthält wiederholte vollständige Tabellenprüfungen nach einzelnen Einfügungen.
- **Offen:** vollständiger nativer Schreiber, Verbraucher/Bereinigung der
  Slot-Meldungen, Inventarereignisse und echte Effekt-/Transportverarbeitung,
  Spieler-/Manager-/Thread-/Weltbindung sowie Eingabe, Loader und B0-Installation.
- Phasenübersicht und Testcheckliste aktualisiert; R12 ergänzt mehrere reparierte
  Ausrüstungsslots. Keine Spielprozess-/Savezugriffe oder Installationsänderung.
- [Nachweis](../runtime/repair/DIRTY_SLOTS_2949.md),
  `.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.12.0: native Itemkopien besitzen und für Ereignisse verwenden

- Neue Bibliothek `crimson_repair_item`: Konstruktion, tiefe Zuweisung und
  Freigabe je Objekt gebunden, ohne kopierbaren/verschiebbaren Besitzer.
  Falscher Thread, ungültige Header und fehlende Bindungen werden abgewiesen;
  fehlgeschlagene native Rückgaben stellen keinen benutzbaren Itemwert bereit.
- Originale Itemmethoden und sämtliche verwendeten Listen-/Allokationshelfer
  auf privaten Quellen ausgeführt. Eigene Sockel-, Listen- und optionale
  Datenkopien bleiben unabhängig von der Quelle; sämtliche Puffer werden freigegeben.
- Alle 15 Ereignistests nutzen die native Nachherkopie: Kopieren unter den
  Besitzersperren, Benachrichtigung nach deren Freigabe, danach Destruktion.
- **Geprüft:** sieben CTest-Suiten, MSVC `/W4 /WX`, 30 neue Besitzerbedingungen;
  117 native Szenarien, 851 Aufrufe und 13.639 Bedingungen. 45 Codebereiche,
  49 pdata-Fragmente und 24 private TLS-Umleitungen. Beide Allokationswege,
  Wachstum, leere Daten, Selbstzuweisung und Ablehnung ungültiger Kapazität geprüft.
- **Offen:** vollständiger Schreibweg, Aktualisierung geänderter Slots,
  Inventarereignisse, wirkliche Effekt-/Transportverarbeitung, Host und Installation.
  Der private Heap ist weiterhin eine Testabhängigkeit. Die Slot-Methode wurde
  statisch weiter eingegrenzt, noch nicht implementiert oder ausgeführt.
- Phasenstatus/Testcheckliste aktualisiert; R11 ergänzt unterschiedliche Exemplare
  desselben Items. App, Spielinstallation und Saves bleiben unverändert.
- [Nachweis](../runtime/repair/ITEM_LIFECYCLE_2949.md),
  `.local/repair-runtime-v12-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.11.0: Ausrüstungsereignisse vorbereiten und gemeinsam prüfen

- Ereignisdaten aus dem unveränderlichen Reparaturplan: genaue Identität/Sitzung,
  vorheriger und nachheriger Broken-Zustand. Aktiver No-Wear-Override wird
  berücksichtigt; mitgeführte Items und falsche Slots werden abgewiesen.
- Originaler Server-Notifier und aktueller Client-Ack mit eigenen Testobjekten
  verbunden. Native Referenzen bleiben gehalten; beide Besitzersperren sind vor
  den Ereignissen freigegeben. Delta 0 erhält korrekt reparierte Sockelwerte.
- 15 neue native Fälle belegen Effekt-/Kind-/UI-Aufrufentscheidungen und
  verweigern Scheinerfolge bei Teiländerung, fehlender/falscher/doppelter
  Rückmeldung, fehlendem Slot, Ereignisbeleg oder Sitzungswechsel.
- **Geprüft:** sechs CTest-Suiten, MSVC Release `/W4 /WX`, 1.537 Aktionsbedingungen;
  insgesamt 103 native Szenarien, 758 Aufrufe und 9.311 Bedingungen. 28 Codebereiche,
  25 pdata-Fragmente und acht private TLS-Umleitungen.
- **Grenze:** echte Effektverarbeitung, Transport und UI bleiben Fixture-Empfänger.
  Engine-Schreibweg/Itemkopie, Slot-Invalidierung, Inventarereignisse, Host,
  Eingabe, Loader und B0 fehlen. Kein installierbarer Reparaturmod behauptet.
- Phasenstatus und Testcheckliste aktualisiert; R10 ergänzt den direkten Vergleich
  kaputter/beschädigter Items, Sockel und No-Wear auf sofort korrekte Effekte.
  App v0.5.9 bleibt unverändert; keine Spielprozess-/Savezugriffe oder Spielschreibzugriffe.
- [Nachweis](../runtime/repair/EQUIPMENT_EVENTS_2949.md),
  `.local/repair-runtime-v11-{build-test.log,native-result.json,validation.json}`.

## Reparatur 0.10.0: Lesefelder und Zugriffe auf dem neuen Build bestätigt

- Eigene 2949-Nachweise für Inventarslots, Holder-/Possessorauflösung, aktuelle
  Clientauswahl, Ausrüstungsslots und Sockelfelder. Die ursprünglichen Getter
  und der Leser stimmen auf identischen privaten Quellen überein.
- Der Leser verlangt ausdrücklich einen bekannten Build. Nicht gesetzte und
  unbekannte Selektoren scheitern vor Referenzerwerb, Sperren und Speicherlesen.
- Originaler Equipmentpfad mit Delta 0 nutzt echte native Lock-Referenzen und
  exklusive Windows-Sperren. Protokollierte Definitionsabfragen belegen die
  tatsächliche Wahl des zweiten passenden Slots. Nichtzero-Delta-Ereignisse
  werden durch diesen Layouttest nicht als bewiesen eingestuft.
- Positives Hauptitem-Reparaturdelta setzt im Originalupdater weiterhin einen
  beschädigten Sockel auf 0. Dieser Pfad bleibt für die eigene Reparatur ungeeignet;
  der Befund ist nun am neuen Build nativ reproduziert.
- **Geprüft:** sechs CTest-Suiten, MSVC Release `/W4 /WX`, 385 Leserbedingungen;
  88 native Szenarien, 578 Aufrufe, 6.219 Bedingungen. Vollständiger EXE-Hash,
  24 Codebereiche, 20 pdata-Fragmente; sechs gezielte private TLS-Umleitungen.
- **Offen:** tatsächliche Manager-/Spieler-/Engine-Threadbindung, Weltwechsel,
  vollständige Reparaturänderungen und Ereignisse/Effekte, Eingabe, Loader, B0.
  Tabellenresolver, leere temporäre Itemkonstruktion/-destruktion und Teile der
  Lebensdauerverwaltung sind ausdrücklich begrenzte Testabhängigkeiten.
- App v0.5.9 unverändert; Spielprozess und Saves nicht benutzt, Installation nicht
  verändert. [Nachweis](../runtime/repair/INVENTORY_2949.md),
  `.local/repair-runtime-v10-{build-test.log,native-result.json,reader-result.json,validation.json}`.
  Phasenstatus und Testcheckliste aktualisiert; keine vorgezogene Spielabnahme.

## Reparatur 0.9.0: gehaltene Besitzer direkt erfassen

- Eine zweite ungeschützte Server-Registry-Suche aus dem geschützten Leser entfernt.
  Nur das bereits gehaltene Paar wird gelesen; andere Clientauswahl wird vor
  deren Dereferenzierung abgewiesen. Vollständige Kennung und lebendige Besitzer
  sind jetzt auch im reinen Sperrpfad zwingend.
- Gemeinsame native Probe um den originalen WindowsRWLock-Try-Pfad von 2949
  erweitert. Tatsächliche Registry-/Actor-Referenzen und Besitzersperren schützen
  Inventar-, Equipment- und Sockelerfassung einschließlich Kontrolllesen.
- **Geprüft:** sechs CTest-Suiten, MSVC Release `/W4 /WX`, 362 Leserbedingungen;
  48 native Szenarien, 503 Aufrufe und 3.867 Bedingungen. Blockierte Besitzer,
  geänderte Auswahl/Kennung, ersetzte Registryknoten, Fehler und Freigabereihenfolge.
- **Nachweisgrenze:** Der kombinierte Test verwendet für Inventar/Clientanker
  private Fixtures des bisherigen Lesers. Neue Erwerbs-/Sperrfunktionen sind
  nativ geprüft; daraus folgt keine Live-Zulassung dieses Inventarlayouts auf 2949.
  Managerlebensdauer, tatsächlicher Engine-Dispatch/Weltwechsel, Transaktion und
  Ereignisse, Eingabe, Loader sowie B0-Einbau bleiben offen.
- Workbench v0.5.9 bleibt unverändert; keine Spielprozess-/Save-Zugriffe oder
  Installationsänderungen. Spieltests weiterhin auf Nutzerwunsch zurückgestellt.
- [Integrationsnachweis](../runtime/repair/PINNED_CAPTURE_INTEGRATION.md),
  `.local/repair-runtime-v9-{build-test.log,native-result.json,reader-result.json,validation.json}`.
  [Phasenstatus](../PHASENSTATUS.md) und [Testcheckliste](../TESTCHECKLISTE.md)
  aktualisiert, einschließlich Protagonisten-/Begleiterabgrenzung und Neuladen.

## Reparatur 0.8.0: geschützter Registry-/Referenzerwerb auf dem neuen Build

- Eigenständige, vollständig gepinnte native Probe für EXE 1.0.0.2949 ergänzt.
  Sie führt Client-/Server-Lookups, konkrete Normal-/User-Referenzmethoden und
  Actor-/PaPtr-Freigabe gemeinsam über den vorhandenen Quellenadapter aus.
- Originale Lock-Methoden verwenden echte Windows-SRW-Sperren auf privaten
  Registryobjekten. Referenzen werden vor dem Entsperren erworben; fehlgeschlagene
  zweite Suchen geben den ersten Besitz zurück. Eine konkurrierende Schreibsperre
  hält den Lookup an; danach entfernte Einträge werden nicht mehr akzeptiert.
- **Geprüft:** sechs CTest-Suiten, MSVC Release `/W4 /WX`; 30 native Szenarien,
  263 native Aufrufe und 3.490 Bedingungen. Direkte/registrierte Referenzmodi,
  Normal-/User-Parameter, veraltete Generationskennung, ungültige Belege mit Pointer,
  verschachtelter Besitz, verzögerte Bereinigung und Zerstörung sind erfasst.
- **Getrennte Freigaben:** alter nativer Host verweigert weiterhin 1.0.0.2949;
  neuer Registry-Host verweigert unbekannte EXEs. Kein Übertragen alter Adressen.
- **Weiter offen:** aktuelle Spieler-/Managerauflösung, Lebensdauer der Manager,
  Engine-Thread-/TLS-Bindung, weitere Inventar-/Änderungs-/Ereignismethoden,
  vollständige Reparaturtransaktion, Eingabe, Loader und B0-Einbau.
  Threadmap, übergeordnete Referenzen und endgültige Bereinigung bleiben
  ausdrückliche Testabhängigkeiten. Die App-Reparatur ist weiter gesperrt.
- App v0.5.9 und Desktop-Verknüpfung unverändert. Keine neue Rust-/Frontendprüfung
  behauptet; diese Nachweise stehen im folgenden App-Schritt. Keine Änderung
  am laufenden Spiel, an Saves oder der Installation.
- [Integrationsnachweis](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md),
  `.local/repair-runtime-v8-{build-test.log,native-result.json,validation.json}`.
  [Phasenstatus](../PHASENSTATUS.md) und [Testcheckliste](../TESTCHECKLISTE.md) aktualisiert.

## Spielupdate: Tabellenunterstützung und Desktop v0.5.9

- Steam 25455892 / EXE 1.0.0.2949 zusätzlich unterstützt; der bisherige Build
  bleibt eigenständig im Code. 61 verwendete Dateien verglichen, davon 57
  unverändert. Stage-/Queständerungen strukturell und datensatzweise nachgewiesen.
- Alle 14 indizierten Tabellenpaare rekonstruieren exakt ihre Originalbytes.
  Die zwei neuen Stages bleiben in Mod-Overlays erhalten; ausschließlich die
  beiden gewählten Patrouillen werden geändert. Neuer Stand: 52.082 Stages,
  1.098 Quests, weiterhin 6.816 Items und 2.069 Skills.
- Schema-ID für Index und Export wird passend gewählt. Sicherungsbelege müssen
  einen vollständigen Build samt Identität treffen; gemischte Belege abgewiesen.
  Unbekannte Versionen führen direkt zum Versionsfehler statt zu einer fehlenden
  Projektprobensicherung. Eine eigene PE-Testinstallation bestätigt diese Ablehnung.
- **Geprüft:** 181 Rust-Tests plus ein gesonderter Advanced-Tabellentest,
  sieben Frontend-Tests, 61 UI-Flows, Clippy und Formatprüfung. Die echte Release-App
  prüft 3.359 Änderungen über 13 Tabellen und zwei Apply-/Restore-Zyklen im Projekt.
- **Bereitgestellt:** `target/release/crimson-workbench-0.5.9.exe`. Eine inzwischen
  geöffnete Workbench verhindert das Ersetzen des bisherigen Dateinamens; das
  fertige Release-Artefakt wurde separat geprüft und bereitgestellt. Desktop-Link
  und Startskript zeigen auf v0.5.9. Das vorhandene Fenster bleibt geöffnet.
- **Unverändert:** laufendes Spiel, ursprünglicher Audit und die 38 erfassten
  EXE-/Metadatendateien. Keine B0-Einrichtung, keine Save-Schreibzugriffe.
- **Weiter offen:** native Reparaturanbindung, allgemeine NPC-Respawnzeiten und
  die übrigen ausgewiesenen Phase-5-Grenzen. Acht native VTables für den neuen Build
  inzwischen erneut rein lesend erfasst; dies ist noch keine native Ausführungsfreigabe.
- [Buildnachweis](BUILD_SUPPORT.md), [Prüfung](TESTING.md),
  [aktualisierte Phasenübersicht](../PHASENSTATUS.md),
  [spätere Testfälle mit Erwartungen](../TESTCHECKLISTE.md).

## Registry-Adapter 0.7.0 und Spielupdate (vorheriger App-Stand)

- Registry-Quellenadapter implementiert: vollständige Kennung an den Lookup,
  native Belege behalten und je Quelle mit der passenden Methode freigeben.
  Source-Release erhält jetzt seinen eigenen Kontext. Keine globale Methodenbindung.
- **Geprüft:** fünf CTest-Suiten, MSVC Release `/W4 /WX`; 55 neue Registry-Bedingungen,
  249 Leser-/Referenzbedingungen, 25 Sperrgruppenbedingungen, 1.481 Aktionsbedingungen.
  Die Registry-Tests sind synthetisch, keine neue native Erfolgsprobe.
- **Update erkannt:** Steam 25455892 / EXE 1.0.0.2949, SHA-256
  `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
  24 von 38 zuvor erfassten EXE-/Metadatendateien verändert. Der neue Bericht
  `docs/builds/steam-25455892.observed.json` ist ausdrücklich keine Schemafreigabe.
- Bisheriger nativer Host verweigert die neue EXE vor Funktionsausführung;
  CLI-Fingerprint meldet den Build als unbekannt. 203 native Aufrufe/3.859
  Bedingungen bleiben historisch auf 1.0.0.2944 begrenzt. Ein gesonderter
  Tabellenaufruf stoppt schon bei fehlender Projektprobensicherung und zählt
  nicht als passende Build-Fehlermeldung.
- **Weiter offen:** neue Build-/Tabellennachweise, aktueller Spieler/Manager und
  Engine-Thread-/TLS-Anbindung, zusammengeführte native Registry-Probe,
  Reparaturtransaktion samt Ereignissen, Eingabe, Loader und B0-Einbau.
- App v0.5.8 und ursprünglicher Audit unverändert; aktuelle 38 Dateien seit
  neuer Beobachtung erneut unverändert. Kein Spielprozess-/Save-Zugriff oder
  Schreiben in die Installation durch diese Arbeit.
- [Registry-Vertrag](../runtime/repair/REGISTRY_INTEGRATION.md),
  `.local/repair-runtime-v7-validation.json`.
- Neu: [TESTCHECKLISTE.md](../TESTCHECKLISTE.md) enthält Fälle/Erwartungen nach
  Phasen, technische Restprüfungen und separates Ergebnisprotokoll. Deine
  Spieltests bleiben bis nach Abschluss aller Entwicklungsphasen zurückgestellt.

## Reparatur: Objekt-Referenzen und Erfassung 0.6.0 (vorheriger Stand)

- Nicht kopierbare native Referenzbelege für beide Besitzer und balancierte
  Freigabe implementiert. Fehlgeschlagener Erwerb verwirft auch eine bereits
  gehaltene erste Referenz. Ein ungültiger Beleg gilt trotz Pointer nicht als Besitz.
- Leser hält Referenzen über die komplette Erfassung und gibt Sperren zuerst
  zurück. Vollständige Kennung, Alive-/Zerstörungszustand werden davor/danach
  unter Sperren geprüft; abgemeldete Besitzer werden abgewiesen.
- Native PaPtr-Freigabe und konkreter Actor-Override geprüft. Der Basispfad ist
  kein Ersatz: Der tatsächliche Override kann zusätzlich eine ausstehende
  Zustandsbereinigung auslösen. Der direkte Erwerb kann trotz Alive=0 erfolgreich
  sein; die Leserablehnung dieses Falls ist getestet.
- **Geprüft:** vier CTest-Suiten, MSVC Release `/W4 /WX`, 249 Leser-/Referenzbedingungen,
  25 Sperrgruppenbedingungen, 1.481 Aktionsbedingungen; 203 native Aufrufe und
  3.859 Bedingungen. Threadmap, eingebettete Sperren und Bereinigungsabhängigkeiten
  sind Fixture-Callbacks; kein vollständiger Engine-Lebensdauernachweis behauptet.
- **Weiter offen:** Erwerb aktueller Spielerreferenzen aus geschützten Live-Quellen,
  konkrete Host-/TLS-Bindung, vollständige Reparaturtransaktion mit Ereigniszuordnung,
  Eingabe, Loader und B0-Installation. Phase 5 bleibt offen.
- App v0.5.8, ursprünglicher Audit und 38 Installationshashes unverändert.
  Kein Spielprozess-/Save-Zugriff. [Referenzvertrag](../runtime/repair/REFERENCE_INTEGRATION.md),
  `.local/repair-runtime-v6-validation.json`.
- Neu: [PHASENSTATUS.md](../PHASENSTATUS.md) fasst Implementierung und Restpunkte
  der Phasen 0–8 sowie des optionalen Farm-Hotkeys zusammen.

## Reparatur: Besitzer-Sperren und Erfassung 0.5.0 (vorheriger Stand)

- Gemeinsame, nicht wartende Sperrgruppe für Client/Server implementiert.
  Bei Konflikt wird eine zuvor erworbene Sperre freigegeben. Bindungen werden
  vollständig vorgeprüft; Rekursion, Reihenfolge und Threadzugehörigkeit geprüft.
- Inventarleser erfasst und kontrolliert die Daten unter beiden Sperren;
  geänderte Besitzer-/Sperrzeiger und ungültige Daten verwerfen das Ergebnis.
  Die zurückgegebene Kopie besitzt keine fortbestehende Schreibfreigabe.
- Originale Try-/Release-Methoden im eigenen Testprozess mit echten Windows-
  SRW-Sperren geprüft, einschließlich konkurrierender Threads. Ein TLS-Lesezugriff
  der privaten Try-Kopie wird auf Daten je Testthread umgeleitet.
- **Geprüft:** vier CTest-Suiten, MSVC Release `/W4 /WX`, 1.481 Aktionsbedingungen,
  163 Leserbedingungen, 25 Gruppenbedingungen; 140 native Aufrufe mit 2.408
  Bedingungen. App v0.5.8, Audit und alle 38 erfassten Installationsdateien unverändert.
- **Weiter offen:** unabhängig gesicherte Lebensdauer der Spielobjekte, vollständige
  Änderungstransaktion samt Ereigniszuordnung, Eingabe, Loader und B0-Einbau.
  Kein Spielprozess-/Save-Zugriff. Phase 5 bleibt offen.
- [Belege und Integrationsvertrag](../runtime/repair/LOCK_INTEGRATION.md),
  `.local/repair-runtime-v5-validation.json`.

## Reparatur: bestätigter Auftragsabschluss 0.4.0 (vorheriger Stand)

- Asynchrone Annahme und bestätigter Abschluss getrennt implementiert. Die
  Auftragssteuerung hält den Plan und meldet bis zur Bestätigung keinen Erfolg.
  Auftragsnummer plus ursprüngliche Sitzung, frisch erfasste Nachherwerte beider
  Repräsentationen und bestätigte Benachrichtigungen sind erforderlich.
- Alte/fremde Rückmeldungen werden abgewiesen; kein zweiter Auftrag, Abbruch
  oder erneuter Commit während der Wartezeit. Teiländerung, Weltwechsel und
  fünf Sekunden ohne Bestätigung sperren die Queue dauerhaft. Keine Rücksetzung
  oder automatische Wiederholung nach möglicher Änderung.
- Originalen Client-Ack, Lock-Konstruktor und Sockelentferner isoliert geprüft.
  Der Ack verändert zuerst Daten und vergleicht danach die Serverliste.
  Er kann Sockel vor einer Fehlermeldung entfernen; auch Fehlercode 0 beweist
  keine erfolgreiche Reparatur. Diese Fälle sind reproduzierbare Regressionen.
- **Geprüft:** 1.481 Aktionsbedingungen, 124 Leserbedingungen, 122 native Aufrufe
  mit 2.221 Bedingungen; drei CTest-Suiten und MSVC Release `/W4 /WX` bestanden.
  Der Client-Ack benutzt einen ausdrücklich umgeleiteten TLS-Lesezugriff und
  künstliche UI-/Allocator-/Lock-Abhängigkeiten; keine echte Engine-Nachricht.
- **Weiter offen:** echte Engine-Leases, vollständiger Änderungsweg und Zuordnung
  tatsächlicher Spielereignisse, Bedieneingabe, Loader und B0-Installation.
  Phase 5 bleibt offen; die manuelle Spielabnahme bleibt zurückgestellt.
- App/Verknüpfung v0.5.8 unverändert. 38 EXE-/Metadatenhashes und ursprünglicher
  Audit unverändert; kein Spielprozess-/Save-Zugriff oder Live-Einbau.
- [Abschlusssteuerung und Belege](../runtime/repair/ACK_INTEGRATION.md),
  `.local/repair-runtime-v4-validation.json`.

## Reparatur: lesender Adapter 0.3.0 (vorheriger Stand)

- Aktuelle Client-/Server-Struktur angebunden: gezielte Suche über die volle
  Charakterkennung, zusätzliche Besitzerprüfungen, nur Character-Inventar und
  Equipment. Item-IDs, Positionen und beide Zustandskopien müssen zusammenpassen.
- Item-/Sockeldaten werden ausschließlich in private Abbilder kopiert.
  Vollständiges Kontrolllesen erkennt geänderte Daten und verwirft die Ausgabe.
  Fehlende/mehrdeutige Kennungen, ungültige Layouts und Budgetüberschreitungen
  werden abgewiesen. Der Leser ersetzt keine Engine-Sperre.
- Aktuelles Inventarlayout mit nativen Kopien geprüft: Slotgrenze `+0x0c`,
  zusammenhängende `0xc8`-Byte-Einträge und Ausschlusslisten. Leser und Original
  liefern auf denselben Fixtures dieselben vorhandenen Slots. Native Registry-
  Suche inklusive Zugriffsablehnung geprüft; echte Lock-/Lease-Methoden sind Stubs.
- **Geprüft:** 124 Leserbedingungen, 1.377 Aktionsbedingungen, 116 native Aufrufe
  mit 1.387 Bedingungen. Drei CTest-Suiten und MSVC Release `/W4 /WX` bestanden;
  auch 2.048 Items und die gesamte Ablehnung eines zu großen Batches geprüft.
- **Weiter technisch offen:** echte Engine-Zugriffsfreigaben, bestätigte
  Änderungs-/Benachrichtigungstransaktion, Bedienung, Loader und B0-Installation.
  Der native Registry-Rückgabepointer allein ist kein gültiger Zugriffsnachweis.
- App/Verknüpfung bleiben v0.5.8; Reparatur noch nicht aktivierbar. 38 EXE-/
  Metadatenhashes, App und ursprünglicher Audit unverändert. Kein Zugriff auf
  Spielprozess oder Saves; verschobene Spieltests und Phase-5-Restpunkte bleiben offen.
- [Leser und Layoutbelege](../runtime/repair/READER_LAYOUT.md),
  [Reproduktion](../runtime/repair/README.md), `.local/repair-runtime-v3-validation.json`.

## Eigene Reparaturaktion: Entwicklungsmodul 0.2.0 (vorheriger Stand)

- Nutzerentscheidung: Eine eigene Reparaturaktion für noch vorhandene,
  beschädigte Items ergänzen. Auch vorher nicht über Spielregeln reparierbare
  Items sind damit im Umfang; keine Wiederherstellung verschwundener Items.
- Eigene Planung und Anwendung auf private Itemabbilder entwickelt: Einzelitem,
  Inventar, Ausrüstung oder beides; Haupt- und Sockelhaltbarkeit unabhängig.
  Benötigt weder die leeren Item-Reparaturlisten noch Reparaturmaterial.
- UID-/Positionsprüfung, Vergleich beider Zustandskopien, vollständige Batch-
  Vorprüfung und Schreibbegrenzung auf Haltbarkeitswörter. Kombination mit dem
  bekannten No-Wear-Override repariert auf das verifizierte Vanilla-Maximum.
- Warteschlange gegen doppelte Eingaben, falsche Threads und verspätete Ausführung;
  Sitzungswechsel verwirft alte Aufträge. Unklarer Commit sperrt weitere Aufträge.
- **Geprüft:** 1.377 Bedingungen für die Aktion; 83 native Aufrufe mit 1.252
  Bedingungen, zwei CTest-Suiten, MSVC Release `/W4 /WX`. Der echte Updater liest
  die reparierten Haupt-/Sockelwerte korrekt weiter. Sein positiver Delta-Pfad
  ist für RepairAll ungeeignet; der Fehlerfall ist isoliert reproduziert.
- **Noch nicht im Spiel nutzbar:** Spielerresolver, vollständige Engine-
  Transaktion/Benachrichtigungen, Bedienung, Loader und B0-Installation fehlen.
  Das vorhandene Backend arbeitet ausschließlich mit eigenen Speicherabbildern.
  Die native Probe beweist Feldnutzung, keine tatsächliche Spielpersistenz.
- Kein Spielprozess-/Save-Zugriff. Desktop-App unverändert v0.5.8; keine
  Installation, keine vollständige Phase-5-Freigabe. Spieltests bleiben verschoben.
- [Aktion und Integrationsstand](../runtime/repair/ACTION_INTEGRATION.md),
  [Tests](../runtime/repair/README.md), `.local/repair-runtime-v2-validation.json`.

## Freigegebene Reparatur-Laufzeitentwicklung: Prototyp 0.1.0 (vorheriger Stand)

- Das aktuelle „go“ gibt die separate Laufzeitentwicklung für kostenlose
  Reparatur frei. Keine erneute Zustimmung hierfür erforderlich. Spieltests
  bleiben verschoben; Farmmodus-Hotkey und Phase 6 wurden nicht begonnen.
- Eigenes C++-Modul und native Testumgebung unter `runtime/repair/` entwickelt.
  Koordinierte, hashgeprüfte Änderungen beider Reparaturhelper: Kosten 0 ohne
  Materialdivision/-begrenzung; kein zusätzlicher Materialauftrag. Fehler,
  gültige Mengenberechnung, Maximalhaltbarkeit und vorhandene Aufträge geprüft.
- 79 native Funktionsaufrufe in privaten Kopien bestanden, einschließlich
  unveränderter Prepare-/Commit-Funktionen mit leerer Materialliste und Restore.
  1.236 Prüfbedingungen, CTest-Vertragstest, drei Python-Regressionen und zwei
  bestehende Rust-Reparatursperren bestanden. MSVC Release `/W4 /WX` grün.
- Audit verbessert: 16 direkte Referenzen einschließlich des zuvor übersehenen
  gemeinsamen Kostenaufrufs. Keine Aussage über indirekte Aufrufe.
- **Noch kein installierbarer Reparaturmod:** Reale Reparaturlisten sind leer;
  vorgeschaltete Materialauswahl, weitere UI-/RepairAll-Pfade, Loader und B0-
  Installation fehlen. Positive Tests verwenden künstliche Reparaturregeln.
  Die tatsächlich gespeicherte Haltbarkeit eines Spielitems wurde nicht verändert.
- 38 EXE-/Metadatenhashes und ursprünglicher Audit unverändert. Kein Zugriff auf
  den Spieleprozess oder Saves, keine Live-Anwendung. Desktop-App bleibt v0.5.8;
  ihr Reparaturschalter bleibt gesperrt. Keine vollständige Phase-5-Freigabe.
- [Prototyp und Reproduktion](../runtime/repair/README.md),
  [technischer Nachweis](research/REPAIR_RUNTIME.md),
  `.local/repair-runtime-v1-validation.json`.

## Phase 5: v0.5.8 – zusätzliche Eigenkosten in Buffs und Skills (App-Stand)

- Eigener vollständiger Reader für alle 292 BuffInfo-Datensätze, einschließlich
  der verschachtelten Skill- und Ressourcenlisten von Bufftyp 114.
- 36 zusätzliche Eigenkosten in 33 Ausrüstungs-Buffs separat für Ausdauer und
  Geist regelbar. Vorschau, Einzelausnahmen, Export und B0 integriert.
- 31 Geistverbrauchswerte in `Active_UseResource_Mp` und
  `Skill_ElementalReinforce_UseMp` über „Weitere Skills“ erfasst. Explizite
  Buffänderungen haben Vorrang; Original → Endwert erscheint einmal im Diff.
- Nullkosten-Schalter setzen alle sieben Kategorien der jeweiligen Ressource.
  Positive Regeneration, gegnerische Drain-Effekte, Bufflaufzeiten, Grenzen,
  Zielskill-Referenzen und Regenerationsflags bleiben dabei erhalten.
- **Geprüft:** 179 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 61 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
  und Release grün. Finale EXE: 3.359 Änderungen über 13 Tabellen;
  zwei Apply-/Restore-Zyklen an der Projektkopie inklusive BuffInfo. Reapply,
  Recovery, Startschutz und Updateablehnung bestanden; neue Ansicht visuell geprüft.
- 38 EXE-/Metadatenhashes und ursprünglicher Bericht unverändert. Desktop-
  Verknüpfung zeigt auf v0.5.8; eigene Testinstanz beendet.
- EXE: 16.156.160 Bytes, SHA-256 `b32583fbf48680d66519fda761185a8a8ca7d18a1235186aa5d245a1a4ae5dbb`.
  Nachweise: `.local/phase5-v9-validation.json`, [Prüfplan](TESTING.md).
- Reparatur-Aufrufer geprüft: Der Mengen-Sonderwert -1 ist im normalen
  Materialpfad nicht erreichbar, da die Aufrufer Mengen <= 0 ablehnen.
  Kein sicherer Tabellenweg für kostenlose Reparatur nachgewiesen. Die
  zunächst angefragte Freigabe für Laufzeitentwicklung wurde inzwischen erteilt;
  aktueller Prototyp siehe oben.
- Weitere Spawn-Recherche: `IsOverMiseensceneSpawnableTime` prüft Zeitfenster
  eines NPC-Plans, keinen allgemeinen Respawn-Abstand. Keine Änderung daran.
- Phase 5 bleibt offen; Phase 6 und Runtime-Phase D nicht begonnen. Spieltests
  bleiben auf Nutzerwunsch verschoben, laufendes Spiel und Saves unberührt.

## Phase 5: v0.5.7 – Haltbarkeit, Lager und Prüfung aller Restpunkte (vorheriger Stand)

- „Kein Haltbarkeitsverlust“ verbindet den EquipType-Faktor mit dem bestätigten
  Engine-Sentinel für alle 122 Items mit endlicher Haltbarkeit. Keine Save-
  Migration oder Wiederherstellung verbrauchter Items; Reparatur bleibt gesperrt.
- Kuku und fünf Housing-Lager ergänzt: jetzt neun editierbare Slotbereiche.
  Zwanzig Inventarpräfixe strukturell gelesen, vollständige Suffixprüfung aller
  21 Datensätze; der Character-Bedingungsbaum bleibt hashgebunden erhalten.
- Alle B4–B11-Anforderungen neu abgeglichen. [Restpunktmatrix](PHASE5_REMAINING.md)
  unterscheidet implementierte Funktionen, technische Lücken und Spielabnahmen.
  Allgemeine NPC-Respawns, kostenlose Reparatur und universelle Engine-/Save-
  Maxima sind weiterhin unbelegt; kein vollständiger Phase-5-Abschluss behauptet.
- **Geprüft:** 176 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 60 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
  und Release-Build grün. Finale EXE: 3.323 Änderungen über zwölf Tabellen;
  zwei Apply-/Restore-Zyklen an der Projektkopie einschließlich aller neuen
  Lager und 122 Item-Haltbarkeitswerte. Reapply/Recovery/Startschutz/Updateablehnung
  bestanden. Neue Ansichten bei 1.024 Pixeln visuell geprüft; keine Browserfehler.
- 38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht unverändert. Keine Mods
  live angewendet. Desktop-Verknüpfung zeigt auf v0.5.7, eigene Testinstanz beendet.
- EXE: 16.133.632 Bytes, SHA-256 `248109d7c629b658109303f39dd11955e8eb719682be5225d08289e27543fd7e`.
  Nachweise: `.local/phase5-v8-validation.json`, [Prüfplan](TESTING.md).
- Phase 6 und Runtime-Phase D nicht begonnen. Manuelle Tests auf Nutzerwunsch
  verschoben. Die laufende Installation und Saveinhalte bleiben unberührt.

## Phase 5: v0.5.6 – Skill-Basisdaten und vollständiger struktureller Reader (vorheriger Stand)

- Alle 2.069 Skills zeigen jetzt zusätzlich 87.697 benannte Header-/Suffixfelder:
  Voraussetzungen, Ressourcen, Referenzen, Flags und Texte. Eigene Suche nach
  Feldnamen oder Werten; Referenzen/unklare Werte bleiben schreibgeschützt.
  Bestehende Skill- und Buffänderungen bleiben kompatibel und werden beim
  Filtern nicht verworfen. Originalwerte sind als solche gekennzeichnet.
- Der komplette Skill-Reader folgt der belegten Struktur. Die bisher noch
  verwendete Suche nach wiederholten Schlüsselbytes entfällt. `skillGroupKey`
  ist eine Gruppenreferenz und wird nicht mehr mit der Record-ID gleichgesetzt.
  Ungültige Matrizen werden ohne Such-Fallback abgelehnt.
- Alle 2.069 vollständigen Skill-Records aus angezeigten Feldern, Matrixcounts
  und Buffwerten bytegenau rekonstruiert. Neue Regressionen für abweichende
  Gruppenreferenzen, Kennungsbytes in Texten, Ressourcenlisten und Lesefehler.
- **Geprüft:** 175 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 59 UI-Flows; Fmt, Clippy `-D warnings`, TypeScript
  und Release-Build grün. Fertige EXE: 3.189 Änderungen über zwölf Tabellen mit
  Export und zwei Apply-/Restore-Zyklen an einer Projektkopie; Reapply, Recovery,
  Startschutz und Updateablehnung bestanden. Basisdatenansicht bei 1.024 Pixeln
  und fertige Vorschau visuell geprüft; keine Browserfehler.
- 38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht unverändert. Keine Mods
  live angewendet oder Saveinhalte gelesen. Eigene unsichtbare Testinstanz beendet.
  Die bestehende Desktop-Verknüpfung zeigt auf v0.5.6.
- EXE: 16.122.368 Bytes, SHA-256
  `61637a148446e1d0c49bcd41bbb7a81264af8fc634ea16c5334343d3887028f2`.
  Nachweise: `.local/phase5-v7-validation.json`, [Prüfplan](TESTING.md),
  [Feldnachweise](research/ADVANCED_TABLES.md).
- Weiter offen: allgemeine NPC-Respawn-Timer, kostenlose Reparatur,
  universelle Engine-/Save-Maxima und unbekannte Skill-Bedeutungen/Einheiten.
  Keine neue Editierfreigabe für Referenzen oder unbestätigte Zahlenformate.
  Phase 5 bleibt teilweise umgesetzt; Phase 6 und Runtime-Phase D nicht begonnen.
  Manuelle Spieltests bleiben auf Nutzerwunsch zurückgestellt.

## Phase 5: v0.5.5 – Reparaturkostenpfad geprüft und Nullsetzfehler entfernt (vorheriger Stand)

- Der gemeinsame und serverseitige Reparaturpfad nutzt die Materialkosten
  als Divisor. Nullsetzen ist damit kein geeigneter Weg zu kostenloser Reparatur.
  Der bisherige allgemeine Writer-Zweig wurde entfernt. Er war bei den aktuell
  leeren Reparaturlisten des unterstützten Builds bereits durch Core gesperrt.
- Globale und individuelle Reparaturflags werden jetzt immer und vor allen
  Advanced-Änderungen abgelehnt, auch bei synthetisch vorhandenen Regeln oder
  direkten API-Aufrufen. Die niedrige Format-API verweigert den Vorgang ebenfalls.
  Die Zahl gelesener Regeln gilt nicht mehr als Freigabe für den Schalter.
- Alte Vorlagen bleiben lesbar. Eine darin aktive Reparaturoption lässt sich
  gezielt entfernen; Stats, Buffs, Stapelgröße und Enchant-Kopien bleiben erhalten.
  Der getrennte Verschleißregler funktioniert unverändert.
- Ein weiterer Spawn-Intervall-Kandidat gehört zu Timeline-Beschwörungsereignissen;
  daraus folgt keine allgemeine NPC-Respawn-Abdeckung. Kostenfreie Reparatur,
  allgemeine NPC-Respawns, universelle Engine-/Save-Maxima und unbekannte Skill-
  Bedeutungen bleiben offen. Phase 6 und Runtime-Phase D wurden nicht begonnen.
- **Geprüft:** 173 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 58 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
  und Release-Build grün. Fertige EXE: globale und individuelle Reparatur-Requests
  explizit abgelehnt; 3.189 reguläre Änderungen über zwölf Tabellen mit Export
  und zwei Apply-/Restore-Zyklen an der Projektkopie. Reapply, Recovery,
  Startschutz und Updateablehnung bestanden.
- Reparaturansicht bei 1.024 Pixeln und fertige Vorschau visuell geprüft.
  Keine Browserfehler; 38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht
  unverändert. Keine Mods live angewendet und keine Saveinhalte gelesen. Eigene unsichtbare
  Testinstanz beendet; bestehende Desktop-Verknüpfung zeigt auf v0.5.5.
- EXE: 16.109.056 Bytes, SHA-256
  `553b18a22af5c0092b57db2b10671a418ac8dc7b64423f32ebd5babe577925a6`.
  Nachweis: `.local/phase5-v6-validation.json`, [Prüfplan](TESTING.md),
  [Feldnachweise](research/ADVANCED_TABLES.md). Manuelle Spieltests bleiben
  wie vereinbart zurückgestellt.

## Phase 5: v0.5.4 – weitere Skill-Payloads und Feldsuche (vorheriger Stand)

- Alle neun Summon-Payloads (auch mehrere pro Skill) werden in benannte Felder
  aufgeteilt: Charakter-/Gebietsreferenzen, Spawn-Angaben, Andockoptionen,
  Formation und weitere Strukturwerte. Der AddSubLevel-Payload zeigt seine
  Referenz und das folgende unbekannte Vier-Byte-Feld getrennt.
- Feldnamen sind im Skill-Browser durchsuchbar; lange Namen umbrechen.
  Die neuen Felder bleiben schreibgeschützt, da Name und Bytebreite allein
  keine Editierregeln oder Einheiten beweisen. Bestehende Buffänderungen
  verwenden unverändert dieselben Pfade und ersetzen jeweils acht Bytes.
- NPC-Recherche: `_respawnTimeSecond` gehört zu Save-Zustandsmetadaten;
  die selektiv gelesene NPC-Spawn-XML enthält keinen Timer. Keine Saveinhalte
  gelesen und daraus kein Tabellenregler abgeleitet. Reparaturkostenquelle
  außerhalb der leeren Itemlisten weiterhin nicht nachgewiesen.
- **Geprüft:** 171 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 57 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
  und Release-Build grün. Die fertige EXE prüft alle zehn neuen Payloads per IPC,
  Feldnamensuche und geschützte Referenzen sowie 3.189 Änderungen über zwölf
  Tabellen mit Export und zwei Apply-/Restore-Zyklen an der Projektkopie.
  Reapply, Recovery, Startschutz und Updateablehnung bestanden.
- Neue Detailansicht bei 1.024 Pixeln und fertige Vorschau visuell geprüft;
  kein horizontaler Überlauf, keine Browserfehler. 38 EXE-/Metadatenhashes
  und ursprünglicher Prüfbericht unverändert. Keine Live-Anwendung oder
  Saveinhalte gelesen. Eigene unsichtbare Testinstanz beendet; bestehende
  Desktop-Verknüpfung zeigt auf v0.5.4.
- EXE: 16.108.544 Bytes, SHA-256
  `ca9cc071204b5ecdab794597bc0cd891a86faaba2a3f5b3b11071aafd2b2657e`.
  Nachweis: `.local/phase5-v5-validation.json`, [Prüfplan](TESTING.md),
  [Feldnachweise](research/ADVANCED_TABLES.md).
- Phase 5 bleibt teilweise umgesetzt; allgemeine NPC-Respawn-Timer,
  Reparaturkostenpfade, universelle Engine-/Save-Maxima und unbekannte Skill-
  Bedeutungen bleiben offen. Manuelle Spieltests bleiben zurückgestellt.
  Phase 6 und Runtime-Phase D wurden nicht begonnen.

## Phase 5: v0.5.3 – Gebietswiederbesetzung und Eingabegrenzen (vorheriger Stand)

- 108 Wiederbesetzungsregeln für 125 verknüpfte Fraktionsgebiete: eigener
  Prozentfaktor und Einzelausnahmen im Welt-Browser. Ausschließlich `delayTime`
  ändert sich; Questbedingungen, Wahrscheinlichkeiten, andere Questzeitgeber
  und Gebietsverknüpfungen bleiben bytegleich. Keine allgemeine NPC-Respawn-
  Abdeckung oder bereits geprüfte Spielwirkung behauptet.
- Inventar und beide Lager erlauben vorläufig höchstens 1.460 Start-/Maximalplätze.
  Dieser Standarddeckel wurde in einem Inventar-Codepfad der gepinnten EXE
  nachgewiesen. Weitere Engine-/Save-Grenzen und mögliche Konfigurationsabweichungen
  bleiben ungeprüft; 65.535 wird nicht mehr als zulässige Eingabe angeboten.
- Ungültige sichtbare Zahlen sperren Export/Projektprobe und verwerfen eine
  bestehende Live-Dateifreigabe, auch wenn intern noch der letzte gültige Wert
  steht. Nach Korrektur kehrt die alte Live-Freigabe nicht automatisch zurück.
- **Geprüft:** 169 Rust-Tests, eine separat ausgeführte hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 56 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript
  und Release-Build grün. Nativer unsichtbarer EXE-Test: **3.189 Änderungen über
  zwölf Tabellen**, 108 Gebietswiederbesetzungswerte einschließlich Einzelausnahme,
  Export und zwei Apply-/Restore-Zyklen an der Projektkopie. Reapply, Recovery,
  Startschutz und Updateablehnung bestanden. Neue Weltansicht bei 1.024 Pixeln
  sowie Vorschau visuell geprüft; keine Browserfehler.
- Spiel lief während der Probe; 38 EXE-/Metadatenhashes und ursprünglicher
  Prüfbericht unverändert. Keine Live-Anwendung, keine Saveinhalte gelesen.
  Eigene unsichtbare Testinstanz beendet. Desktop-Verknüpfung zeigt auf v0.5.3.
- EXE: 16.115.200 Bytes, SHA-256
  `77580d1346dcc82eddb9edc97be131c447c635d3069c6f690afccba4fb48014d`.
  Nachweis: `.local/phase5-v4-validation.json`, [Prüfplan](TESTING.md),
  [Bedienung](ADVANCED_MODS.md), [Feldnachweise](research/ADVANCED_TABLES.md).
- **Weiter offen:** allgemeine NPC-Respawn-Timer außerhalb dieser belegten
  Patrouillen-/Gebietsregeln, weitere Reparaturkostenpfade, universelle
  Stack-/Inventar-/Save-Maxima und unbekannte Skill-Bedeutungen/opake Payloads.
  Manuelle Spieltests bleiben zurückgestellt. Phase 6 und Runtime-Phase D
  wurden nicht begonnen.

## Phase 5: v0.5.2 – Skillwerte, Patrouillen und Stadtflug (vorheriger Stand)

- Bestätigte i64-Werte in Skill-Buffmatrizen einzeln bearbeiten, einschließlich
  der drei Graphkomponenten. Typen, Referenzen, Counts, Kurventags und opake
  Payloads bleiben geschützt. Zusammenführung mit Cooldowns/Kosten im selben
  Vorschau-/B0-Plan; jeweils exakt acht Bytes pro Zahlenwert.
- Reset-Faktor und individuelle Sekundenwerte für zwei belegte Spawn-Patrouillen
  (Hidden Estate und Watergate), jeweils ursprünglich 259200. Die anderen
  52.078 Stages bleiben unverändert; keine Umdeutung sämtlicher Quest-Resets
  oder des bereits widerlegten Spawn-Prozentfelds zu NPC-Timern.
- Die Stadt-/Drachenoption deaktiviert gezielt Condition 1011130 für den
  Stadtflug-Abstieg. Aktuelle EXE-Serialisierung und falsche Ersatzbedingung
  separat geprüft. Andere 10.797 Bedingungen bleiben byteidentisch.
  Gemeinsame Regel für Flugreittiere, daher auch Auswirkung auf andere Flieger;
  Regionsliste 79 bleibt der zweite gezielte Sperrpfad.
- **Noch offene Entwicklung:** allgemeine NPC-Respawn-Timer, die tatsächliche
  Quelle weiterer Reparaturkosten, Engine-Maxima sowie unbekannte Bedeutungen
  und opake Skill-Payloads. Keine Abschaltung von Quest-/Zwischensequenz-
  Abstiegen oder maximale Flughöhe behauptet. Keine Runtime-Implementierung.
- **Geprüft:** 167 Rust-Tests, zusätzliche hashgeprüfte Tabellenprobe,
  sieben Frontend-Unit-Tests und 53 UI-Flows; Fmt, Clippy `-D warnings`,
  TypeScript und Release-Build grün. Unsichtbarer Test der fertigen EXE:
  3.081 Änderungen über elf Tabellen, Export und zwei Apply-/Restore-Zyklen
  nur an der Projektkopie, Reapply, Recovery, Startschutz und Updateablehnung.
  38 EXE-/Metadatenhashes und ursprünglicher Prüfbericht unverändert;
  Spiel lief, keine Live-Anwendung, keine Saveinhalte gelesen.
- EXE v0.5.2: 16.107.520 Bytes, SHA-256
  `1a5be87e93bdb15046f84b88e58a71058b830155d533519ebe50747a0f8127e4`.
  Desktop-Verknüpfung weiterhin gültig; eigene unsichtbare Testinstanz beendet.
  Nachweis: `.local/phase5-v3-validation.json`, [Prüfplan](TESTING.md),
  [Bedienung](ADVANCED_MODS.md), [Feldnachweise](research/ADVANCED_TABLES.md).

## Phase 5: v0.5.1 – Enchant-Zeilen und Skill-Details (vorheriger Stand)

- Neue Enchant-Stufen aus einer ausgewählten Originalstufe kopieren, mit
  vollständigen Stats/Buffs/Preisen, sortierter Einfügung und neuen Headeroffsets.
  Änderungen auf neuen Stufen und wiederverwendbare Vorlagen sind verfügbar.
  Ein Tabellenlevel schaltet keinen Upgradeweg frei und erhöht keine Savewerte.
- Alle 2.069 Skillmatrizen mit 4.607 Einträgen einschließlich null sind vollständig
  abgegrenzt. Detailansicht mit RTTI-Typnamen, Struktur-/Bytepositionen, Rohwerten
  und vollständigen Originalbytes. Unbekannte Bedeutungen/Einheiten und zwei
  nur als Rohblock abgegrenzte Payloadvarianten sind ausdrücklich markiert.
  Matrixbearbeitung ist nicht freigegeben.
- Stackschutz berücksichtigt neu eingeführte Buffs und Enchant-Zeilen.
  Doppelte Ziele, verkettete Quellen und unbestätigte Trennstrukturen werden
  abgelehnt. Bestehende Vorlagen und alte serialisierte Pläne bleiben kompatibel.
- Ein Respawn-Kandidat aus einer Referenzoberfläche wurde statisch widerlegt:
  Der aktuelle Serializer nennt das betreffende u64 `_spawnPercent`. Es wird
  nicht als Zeitgeber bearbeitet. Keine erfundene Funktion als Ersatz.
- **Weiter offen:** allgemeine NPC-Respawn-Zeiten, sämtliche Drachen-Dismount-
  Pfade, Reparaturkosten und vollständige semantische Skill-Buffbearbeitung.
  Engine-Maxima bleiben unbewiesen. Phase 6 und Runtime-Phase D sind nicht begonnen.
- **Geprüft:** 162 Rust-Tests, zusätzlich eine lokale hashgeprüfte Tabellenprobe
  mit Rückwandlung aller Matrixwerte in Originalbytes, sieben Frontend-Unit-Tests
  und 51 UI-Flows. Fmt, Clippy `-D warnings`, TypeScript und Release-Build grün.
  Nativer unsichtbarer Test: 3.077 Änderungen über neun Tabellen, neue Enchant-
  Stufe, Skillansicht, Export und zwei Apply-/Restore-Zyklen an Projektkopien.
  38 EXE-/Metadatendateien und der ursprüngliche Prüfbericht unverändert;
  Spiel lief, keine Live-Anwendung, kein Zugriff auf Saveinhalte.
- EXE v0.5.1: 16.037.376 Bytes, SHA-256
  `3e19c17e7a4f68e63cead2c5bf7810a670fa8d9b3febd9131dafa1c833f1a463`.
  Die bestehende Desktop-Verknüpfung führt weiterhin auf diese EXE.
  Nachweise: `.local/phase5-v2-validation.json`, [Prüfplan](TESTING.md),
  [Bedienung](ADVANCED_MODS.md), [Feldnachweise](research/ADVANCED_TABLES.md).
  Manuelle Spieltests bleiben wie vereinbart zurückgestellt und unbestanden.

## Phase 5: v0.5.0 – belegte Tabellenfunktionen (vorheriger Stand)

- 2.811 editierbare/anzeigbare Records, darunter alle 2.069 Skills, sowie
  Detailbearbeitung der 6.816 Items. Spawn-Gruppen, Stadt-/Reittierflags,
  Blackstar, drei Inventare, Stacks, Equip-Verschleiß, Skillkosten/-cooldowns
  und Stats/Buffs auf existierenden Enchant-Stufen. Benannte Item-Vorlagen
  bleiben lokal im App-Speicher erhalten.
- Änderungen laufen durch denselben Vorschau-/B0-Builder. Werte werden aus
  Originalen neu aufgebaut, Einzelausnahmen sind deterministisch, Array-
  Einfügungen/Entfernungen erhalten neue Header-Offsets. Experimentelle
  Stacks benötigen den ausdrücklichen Schalter.
- Neue begrenzte Reader sind gegen den aktuellen EXE-Serializer und die
  realen, hashgeprüften Tabellen geprüft; keine Übernahme veralteter
  Feldpositionen. Reparaturregeln sind in diesem Build leer: Option gesperrt.
- **Offen:** allgemeine NPC-Respawn-Zeiten, sämtliche Drachen-Dismount-Pfade,
  Reparaturkosten, vollständige Skill-Buffmatrix und neue Enchant-Zeilen.
  Engine-Maxima nicht belegt, Dateiformat-/Workbenchgrenzen klar getrennt.
  [Funktionsumfang und Bedienung](ADVANCED_MODS.md),
  [technische Recherche](research/ADVANCED_TABLES.md).
- Nativer Test der EXE v0.5.0: 3.076 Änderungen über neun Tabellen,
  Export und zwei Apply-/Restore-Zyklen ausschließlich in der Projektkopie,
  Reapply, Recovery, Start-/Backupschutz und Updateablehnung erfolgreich.
  38 EXE-/Metadatendateien und ursprünglicher Inhaltsbericht unverändert.
  Spiel lief; kein `.workbench` in der tatsächlichen Installation.
- Bestehende Desktop-Verknüpfung verweist weiter auf die aktualisierte EXE.
  Phase 6 und Runtime-Phase D wurden nicht begonnen. Offene Entwicklungspunkte
  sind keine bloß verschobenen Spieltests und werden nicht als erledigt geführt.

**Abschlussprüfung dieses Ausbaustands:** 158 Rust-Tests, zusätzlich eine
lokale hashgeprüfte Tabellenprobe, sechs Frontend-Unit-Tests und 49 UI-Flows
bestanden. Fmt, Clippy `-D warnings`, TypeScript und Release-Build erfolgreich.
EXE 15.938.560 Bytes, SHA-256
`b78a222e9611dd33bcf0b0eacaef9e845724c817b6307b738180d3a91888475f`.
Nachweis: `.local/phase5-validation.json`. Keine Freigabe fehlender Funktionen
oder Behauptung einer erfolgreichen manuellen Spielabnahme.

## Phase 4: v0.4.10 – bestätigte Fremddateien und Abschluss der Entwicklung

- Neue opt-in Ansicht für fremde Zusatzdateien mit Pfaden, Größen, SHA-256,
  ausdrücklicher Bestätigung und Widerruf. Bestätigung schreibt ausschließlich
  einen unveränderlichen Datensatz im Projekt; sie löst keinen Live-Vorgang aus.
- Genau bestätigte Zusätze bleiben bei Einrichtung, Apply/Reapply, Restore und
  Basiswechsel erhalten. Alle Inhalte werden vor Live-Schreibvorgängen frisch
  unter gehaltenem Schutz geprüft; zusätzliche EXEs sind im Startschutz enthalten.
  Änderungen, neue Kinder und Widerruf erzeugen keine stillschweigende Freigabe.
- Inventur und Inhaltsbericht trennen diese Zusätze von Originalen und eigenen
  Dateien. Ein optionaler Berichtsverweis benennt den Bestätigungsstand; alte
  Berichte bleiben lesbar. Originalpfade können niemals herausgefiltert werden.
- Fremde Registry-Registrierungen und Änderungen an Originalquellen bleiben
  gesperrt und benötigen zuerst Rücknahme/Steam-Prüfung. Keine Übernahme fremder
  Tabellenänderungen und keine behauptete Kompatibilität. Saves und Links bleiben
  ausgenommen. [Umfang und Bedienung](FOREIGN_FILES.md).
- Drei lokale IPCs und CLI `foreign-preview`, `foreign-confirm --preserve`,
  `foreign-revoke`. Live-Vorschauen binden die Bestätigung; Widerruf wird auch
  während geschützter Arbeit erkannt. Die App erlaubt während ihres Live-Auftrags
  keine parallele Änderung der Bestätigung.

**154 Rust-Tests, 44 Headless-UI-Flows und 4 Frontend-Unit-Tests bestanden.**
Fmt, Clippy mit `-D warnings`, TypeScript und Frontend-Build erfolgreich.
Neun neue Core-Szenarien prüfen bestätigtes Nebeneinander,
Inhaltserhalt, Fremdgruppen-Kollisionen, Widerruf, Quelländerungen, Ausschlussgrenzen,
Protokollmanipulation, konfigurierte Savepfade und Basiswechsel mit bestätigten
Zusätzen. Vier Echtdateiintegrationen bleiben eingeschlossen. Nach Ergänzung der
Protokollgrenze bestanden alle acht Fremddateitests erneut; der letzte Eintrag
bleibt für einen Widerruf verfügbar.

Windows-Release v0.4.10 und unsichtbare native WebView2-Prüfung bestanden.
Die Zusatzdateivorschau wurde am echten Spiel ausschließlich lesend aufgerufen;
keine Bestätigung, Live-Einrichtung oder Anwendung ausgelöst. Alle drei neuen
Sitzungssperren und sieben Live-Sitzungssperren geprüft; keine JavaScript-Fehler,
1024px-Ansicht kontrolliert. Inventur: 285 Originaldateien, keine Zusätze oder
Abweichungen. Die 38 EXE-/Metadatenhashes und der ursprüngliche Nutzerbericht
blieben unverändert. Kein vollständiger PAZ-Hashlauf und keine Spielsperre.

Release: `target/release/crimson-workbench.exe`, 15,507,968 Bytes,
SHA-256 `da9e49c8ba41413b6fbef63a89d2aa6051bc1fa6c84d2f825dc239929e7ab217`.
Die vorhandene Desktop-Verknüpfung wurde geprüft und zeigt auf diese EXE.
Nachweise: `.local/phase4-v11-validation.json`, `.local/phase4-v11-native/result.json`,
`.local/phase4-v11-rust-tests.log`, `.local/phase4-v11-foreign-final.log`,
`.local/phase4-v11-clippy.log`.

**Nächste Phase nach Review/Go: Phase 5 (B4–B11).** Manuelle Spieltests werden
weiterhin erst nach Abschluss aller Entwicklungsphasen erwartet. Das optionale
Runtime-/ASI-Thema bleibt separat und wurde nicht begonnen.

## Phase 4: v0.4.9 – Basiswechsel und eigene Dateien in der Inventur

- Die Inventur trennt nachgewiesene eigene Mod-, Sicherungs- und Archivdateien
  von unbekannten Zusätzen. Originaldateien bleiben vollständig prüfpflichtig.
  Die Zuordnung liest keine PAZ-Inhalte; fremde Kinder und defekte Nachweise
  werden nicht ausgeblendet. Aktive eigene Registry explizit sichtbar.
- Inhaltsprüfung und Ausgangsstände funktionieren auch nach einer eigenen
  Anwendung beziehungsweise Steam-Rücksetzung. Aktive Mod-Registry und offene
  Basiswechsel verhindern einen neuen Vanilla-Bericht.
- Neuer überprüfter Basiswechsel in Desktop und CLI. Neue Basis erfordert
  bekanntes Schema, passende neue Original-Registry, aktuelle Dateivorschau,
  erneute Herkunftsbestätigung und frische geschützte Originalhashes.
- Alte eigene Dateien und vollständige Transaktionshistorie werden bytegleich
  auf demselben Laufwerk archiviert; die neue Spiel-Registry wird nicht geschrieben.
  Ein unveränderliches Projektjournal aktiviert die neue Basis erst nach
  Abschluss. Unterbrechungen einschließlich partieller Protokolle und neuer
  Sicherungen sind mit der vorgesehenen Basis fortsetzbar.
- Neue IPC `live_update_preview`, vorhandener abbrechbarer Worker um Basiswechsel
  erweitert. CLI `live-update-preview` und `live-update`. Keine automatische
  Einrichtung, Anwendung oder Migration auf der echten Installation.

**145 Rust-Tests, 4 Frontend-Unit-Tests und 39 Headless-UI-Flows bestanden.**
Fmt, Clippy mit `-D warnings`, TypeScript und Frontend-Build erfolgreich.
Acht neue Core-Tests einschließlich sechs Migrations-Abbruchgrenzen und partieller
Protokollveröffentlichungen; fünf zusätzliche UI-Flows. Die vier Echtdateitests
und 69 bestehenden Transaktions-Abbruchpunkte bleiben eingeschlossen.
Windows-Release v0.4.9 und unsichtbare native WebView2-Prüfung bestanden.
Alle sieben Live-Befehle verweigern ungültige Sitzungen, keine JavaScript-Fehler;
1024px-Ansicht geprüft. Echte Inventur weiterhin 285 Originaldateien, keine
Live-Zulassung und kein Workbench-Ordner im Spiel. Die 38 EXE-/Metadatenhashes
sowie der ursprüngliche Nutzerbericht blieben unverändert. Kein vollständiger
PAZ-Hashlauf und keine Spielsperre während dieser Entwicklung.

Release: `target/release/crimson-workbench.exe`, 15,235,072 Bytes,
SHA-256 `b9f8ad207bbd16d573679d1913468ddfc195d028bcd33501d694f3eb8861a518`.
Die vorhandene Desktop-Verknüpfung zeigt auf diese EXE.
Nachweise: `.local/phase4-v10-validation.json`, `.local/phase4-v10-native/result.json`,
`.local/phase4-v10-rust-tests.log`, `.local/phase4-v10-ui-tests.log`.
[Bedienung und genaue Grenzen](LIVE_APPLY.md).

**Offen in Phase 4:** bestätigter Umgang mit fremden Mods/Managerresten;
aktuell konservative Ablehnung. Manuelle Spielabnahme weiterhin bewusst verschoben.

## Phase 4: v0.4.8 – Live-Anbindung mit geschützten Quellen

- Eigener Live-Adapter für Einrichtung, Dateivorschau, Apply/Reapply, Restore und
  Recovery. Desktop mit abbrechbarem Hintergrundauftrag und sechs neuen lokalen
  IPCs; CLI `live-status`, `live-setup-preview`, `live-setup`, `live-preview`,
  `live-execute`. Keine automatische Live-Einrichtung oder Anwendung.
- Zulassung erfordert ausdrücklich die Nutzerbestätigung, dass Steam Verify vor
  dem gewählten Inhaltsbericht abgeschlossen wurde. Der Bericht bleibt ein
  beobachteter Cachebericht. Die Herkunft ist damit nutzerbestätigt, nicht
  unabhängig kryptografisch zertifiziert. Alle erfassten Originalquellen werden
  vor jedem Schreibvorgang erneut vollständig unter gehaltenen Windows-Sperren
  gehasht; Registry, Backup und eigene Historie werden gesondert verifiziert.
- Alle erfassten EXEs werden vor dem Lesen großer Archive gegen einen neuen Start
  gehalten. Prozess-/Abbruchprüfung auch während des Hashens. Veraltete Vorschauen,
  fremde Dateien, geänderte Quellen und beschädigte Sicherungen verweigern Writes.
  Eigene Gruppen werden beim Reapply ersetzt; Originalarchive bleiben unverändert.
- Die rein lesende Originaldatenansicht erklärt eine eigene modifizierte Registry
  anhand von Backup, Historie und Overlaydateien. Neue Modpläne bauen dadurch
  weiterhin aus Originaltabellen. Der Inspektionskern kann nicht schreiben.
- Neue Transaktions-Intents enthalten einen Zeitstempel; alte Journale bleiben
  lesbar. Ein Workerabschluss behauptet bei verspätetem Abbruch keinen fehlgeschlagenen
  Commit. Unvollständige Einrichtung bleibt mit derselben Basis wiederaufnehmbar.

**137 Rust-Tests, 4 Frontend-Unit-Tests und 34 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` sowie Windows-Release-Build erfolgreich.
Fünf neue Core-Tests prüfen Live-Adapterabläufe an künstlichen Installationen:
mehrfaches Apply/Reapply mit Originaldatenansicht und Restore; veraltete Pläne,
fremde Dateien und gleich große Quelländerungen; Abbruch und Handlefreigabe;
Recovery vor/nach Registry-Commit; schreibunfähige Inspektion und beschädigtes Backup.
Neuer Workertest und erweiterte Sitzungsschutzprüfung. Die vier Echtdateitests
und 69 bestehenden Transaktions-Abbruchpunkte bleiben eingeschlossen.
Fünf neue UI-Flows prüfen Herkunftsbestätigung, laufendes Spiel, aktuelle Modwerte,
veraltete Antworten, fehlgeschlagene Starts und kooperativen Abbruch.

Native **Windows-Abnahme v0.4.8** bestanden: echte Installation und gespeicherten
Bericht in der Live-Vorschau gelesen, 285 Dateien erkannt, Herkunftsbestätigung
nicht gesetzt und Einrichtung bei laufendem Spiel gesperrt. Alle sechs neuen
Befehle verweigern ungültige Sitzungen. 1024px-Screenshot geprüft; keine
JavaScript-Fehler. Bestehende Mod-/Recovery-/Baselineproben ebenfalls bestanden.
Weder `.local/live` noch `<Spiel>/.workbench` wurden erzeugt; **kein Live-Apply,
keine Spielsperren, kein vollständiger Archivhashlauf während der Entwicklung**.
Alle 38 beobachteten EXE-/Metadatenhashes und der Nutzerbericht blieben unverändert.

Release: `target/release/crimson-workbench.exe`, 15,003,648 Bytes, SHA-256
`1db048d67eb73374608241c259d456f84b6930b025e731862dfb472ca30c3b31`.
Die vorhandene Desktop-Verknüpfung zeigt weiterhin auf diese aktualisierte EXE.
Nachweise: `.local/phase4-v9-validation.json`, `.local/phase4-v9-native/result.json`,
`.local/phase4-v9-rust-tests.log`, `.local/phase4-v9-live-setup-preview.json`.
[Bedienung, konkretes Vertrauensmodell und Grenzen](LIVE_APPLY.md).

**Nächster Entwicklungspunkt:** bereinigte Inventur nach eigener Live-Anwendung,
Basis-/Historienwechsel nach Spielupdates und definierter Umgang mit fremden Mods.
Phase 4 wird deshalb noch nicht als vollständig abgeschlossen markiert. Die
zurückgestellten manuellen Spieltests blockieren diese Arbeiten nicht.

## Phase 4: v0.4.7 – dauerhafter Ausgangsstand und Registry-Sicherung

- Exportierte vollständige Inhaltsberichte können nach erneutem Vergleich mit
  Installation, Build, Depotlisten, Dateibestand und Registry in die neue Ansicht
  **Ausgangsbasis & Registry-Sicherung** übernommen werden. Es erfolgt kein
  erneuter großer Archivhashlauf und keine Spielsperre.
- Die App speichert den unveränderten Bericht, die passende Registry-Kopie und
  ein Abschlussmanifest unter `.local/baselines/<id>`. Kopien werden nach dem
  Schreiben erneut geprüft; das Manifest wird zuletzt veröffentlicht. Fehlende
  Abschlüsse, beschädigte Dateien, unzulässige Eingaben und veraltete Vorschauen
  werden abgewiesen. Bestehende Stände werden niemals überschrieben.
- Gespeicherte Stände lassen sich nach Navigation oder Neustart erneut öffnen.
  Die App unterscheidet intakte Sicherung mit passenden aktuellen Metadaten,
  zwischenzeitliche Abweichungen und nicht prüfbare Installation. Alte Sicherungen
  bleiben bei Updates erhalten. Kein Restore in die Installation.
- Vier neue lokale IPCs sowie CLI `baseline-list`, `baseline-preview`,
  `baseline-capture --review-id` und `baseline-status`.
  [Bedienung, Persistenz und Vertrauensgrenzen](BASELINE.md).

**131 Rust-Tests, 4 Frontend-Unit-Tests und 29 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` sowie Release-Build erfolgreich. Sieben neue
Core-Tests prüfen Inhaltserhalt, Eingabegrenzen, Abbruch vor Abschluss, veraltete
Vorschauen, Manipulation/Hardlinks, Updatezustände und geschützte Pfade. Ein Test
weist ausdrücklich nach, dass gleich große Archivänderungen beim erneuten
Metadatenvergleich unentdeckt bleiben können; dadurch entsteht keine Schreibfreigabe.
Vier neue UI-Flows prüfen Speichern, Wiederöffnen, Fehler-/Updatezustände und
verspätete Antworten. Die vier Echtdateiintegrationen und 69 bestehenden
Transaktions-Abbruchpunkte bleiben eingeschlossen.

Native **Windows-Release-Abnahme v0.4.7** bestanden: Der vom Nutzer bereitgestellte
Bericht wurde unverändert übernommen, eine 679-Byte-Registry-Kopie erstellt,
beides geprüft und nach Ansichtswechsel erneut geladen. Alle vier neuen Befehle
verweigern ungültige Sitzungen und sind in der lokalen Capability eingetragen.
CLI `baseline-list` und `baseline-status` bestätigten den gespeicherten Stand
nach Ende der Testinstanz. Keine JavaScript-Fehler, Screenshot und 1024px geprüft;
bisherige Mod-/Recovery-/Updateproben ebenfalls erfolgreich.

Tatsächlich gespeicherter Stand: `.local/baselines/1789906043-17808-0`.
Prüfbericht: 285 Dateien / 195 PAZ-Archive / 154.097.618.899 Bytes, Inhaltsprüfung
vom 20.09.2026 um 13:25:45 Uhr (Europe/Vienna). SHA-256 der Registry-Sicherung:
`c03ced405f4c409f1472b85465dff763e0fd9ca9c68e29481510780494d4eaea`.
Das Spiel lief während der Abnahme weiter. Alle 38 zuvor beobachteten Spiel-
EXE-/Metadatenhashes sowie der Nutzerbericht blieben unverändert.

Release: `target/release/crimson-workbench.exe`, 14.376.448 Bytes, SHA-256
`691a2327ba988325fd471adf9c6a7e0099ce1893edf9f406c5a3397be6a96275`.
Nachweise: `.local/phase4-v8-validation.json`, `.local/phase4-v8-native/result.json`,
`.local/phase4-v8-rust-tests.log`, `.local/phase4-v8-baseline-status.json`.

**Entwicklungsstand:** Die beobachtete Ausgangsbasis und Registry-Sicherung sind
integriert. Noch offen sind die vertrauenswürdige Zulassung als Live-Basis und
die Live-Anbindung mit erneut geprüften Quell-/Startpfaden sowie Backup-/Update-
Integration. Ein gespeicherter Cachebericht wird nicht automatisch zu Vanilla.
Die manuelle In-game-Abnahme ist separat bis zum Ende aller Entwicklungsphasen
zurückgestellt; sie blockiert deren weitere Entwicklung nicht.

## Phase 4: v0.4.6 – geprüfte Rücknahme bestehender Projektproben

- Neue Desktopansicht **Sicherung & Wiederherstellung**: vorhandene Proben laden,
  eine geschützte v3-Probe auswählen, Sicherung und Quellen prüfen, konkrete
  Rücknahme mit Datei-Hashes ansehen und gezielt ausführen. Erfolgreich
  zurückgesetzte Proben sind als solche erkennbar; kein automatischer Restore.
- Bestehenden Zustand ohne Initialisierung öffnen: Vorschau und neuer Restore
  erstellen weder fehlende Backups noch eine verlorene Historie neu. Vor dem
  Schreiben wird die an Pfad, Marker, Backup, Historiengeneration, Commitzustand
  und vorhandene Dateien gebundene Vorschau unter den Schutzsperren erneut
  berechnet. Veraltete oder für andere Projektkopien erzeugte Pläne werden abgewiesen.
- Offene Transaktionen werden anhand ihrer tatsächlichen Registry vor/nach Commit
  eingeordnet. Fremde Änderungen, fehlende aktive Dateien, beschädigte Sicherungen
  oder Quellupdates führen zum Abbruch. Nach Restore werden Zustand und Hashes
  erneut geprüft; Sicherung und Historie bleiben erhalten.
- Neue CLI-Befehle `mod-backups`, `mod-restore-preview` und `mod-restore --review-id`.
  Drei neue lokale Tauri-Befehle mit Sitzungsprüfung; keine Live-Schreibfähigkeit.
  [Bedienung, Reihenfolge und Grenzen](BACKUP_RECOVERY.md).

**124 Rust-Tests, 4 Frontend-Unit-Tests und 25 verschiedene Headless-UI-Flows
bestanden.** Fmt und Clippy mit `-D warnings` fehlerfrei; vier Echtdateitests
eingeschlossen. Sechs neue Core-Tests prüfen die Rücknahme einschließlich
Fehler-/Updatefällen und rein lesender Vorschau. Zusätzlich sind die Vorher-/
Nachher-Hashes der Rücknahmepläne an allen 69 bisherigen Transaktions-Abbruchpunkten
gegen die tatsächlich verbleibenden Dateien geprüft. Vier neue UI-Flows bestehen
nach Korrektur zweier Testselektoren; die 21 bestehenden Flows blieben grün.

Die finale **Windows-Release v0.4.6** bestand die native WebView2-Abnahme:
alle drei neuen Befehle korrekt in der Capability, ungültige Sitzungen abgewiesen,
vorhandene Sicherung über die Oberfläche geprüft. In einer zusätzlichen Kopie
unserer Probe wurde ein Abschlussmarker gezielt entfernt; der sichtbare Ablauf
erkannte und beendete die offene Transaktion, ohne fremde Daten zurückzuschreiben.
Der erneute Abschluss entspricht dem ursprünglichen; Zustand danach als bereits
wiederhergestellt erkannt. CLI-Liste, Vorschau und erneute idempotente
Wiederherstellung derselben Kopie bestanden. 1024px ohne Überlaufen, Screenshot
geprüft, keine JavaScript-Fehler. Auch die bisherigen Modvorschauen, geschützten
Apply-/Reapply-/Restore-Proben und Updateablehnung bestanden erneut.

Das Spiel lief während der nativen Abnahme weiter. Alle 38 zuvor beobachteten
EXE-/Metadatenhashes sind unverändert; die aktuelle Dateiliste hat weiterhin
285 passende Einträge. Kein erneuter vollständiger PAZ-Hashlauf. Die vorhandene
Desktop-Verknüpfung zeigt auf die aktualisierte EXE.

Release: `target/release/crimson-workbench.exe`, 14.097.920 Bytes, SHA-256
`a03e8068a842f3450200cfa98284b35c53bb3f6fadcfc54ea6dd579a2c652614`.
Nachweise: `.local/phase4-v7-validation.json`, `.local/phase4-v7-native/result.json`,
`.local/phase4-v7-rust-tests.log`, `.local/phase4-v7-restore-preview.json`,
`.local/phase4-v7-cli-recovery.json`.

**Phase 4 bleibt offen.** Der vollständige Nutzer-Inhaltsvergleich aus v0.4.5 ist
erfolgreich abgeschlossen. Unabhängig bestätigte Vanilla-Herkunft, vollständige
Live-Start-/Ladepfadabsicherung, Live-Backup-/Updateintegration und In-game-Abnahme
fehlen weiterhin. Die neue Ansicht stellt ausschließlich Projektkopien wieder her.
Phase 5 wurde nicht begonnen.

## Phase 4: v0.4.5 – abbrechbare Inhaltsprüfung

**Nachtrag 20.09.2026:** Der Nutzer hat den vollständigen Originallauf in der
Desktop-App ausgeführt und den exportierten Bericht bereitgestellt. Von
13:23:25 bis 13:25:45 Uhr (Europe/Vienna) wurden 285 Dateien einschließlich
195 PAZ-Archiven mit zusammen 154.097.618.899 Bytes geprüft. Alle SHA-1-Werte
passen zu den lokalen Steam-Depotlisten; laut Bericht blieben die Metadaten
stabil. Die anschließende Berichtsauswertung bestätigt die interne Konsistenz
und alle 38 zuvor beobachteten EXE-/Metadaten-SHA-256-Werte. Dabei wurden keine
Spieldateien erneut gelesen oder verändert. Der vollständige lesende
Inhaltstest ist damit erfolgt; ein unabhängiger Vanilla-Nachweis und die
Live-Schreibfreigabe folgen daraus nicht.
Nachweise: `exports/installation-audit-1789903405-11284-1-0.json` und
`.local/user-content-audit-review.json`. SHA-256 des Originalberichts:
`f32c50a354dbd81e85889cb764b4ecdf6d83958ed09c9b9eef70077cc48a64dc`.

- Neuer vollständiger Inhaltsvergleich der installierten Depotdateien: SHA-1
  gegen den lokalen Steam-Cache und zusätzliche SHA-256-Aufzeichnung in einem
  gemeinsamen Lesedurchlauf. Größen, Zeitstempel, Dateikennungen, Registry und
  Depotbestand werden vor/nach dem Lauf erneut geprüft. Gleiche Dateigrößen
  reichen jetzt nicht mehr für einen erfolgreichen Inhaltsvergleich.
- Eigener Hintergrundauftrag mit Byte-/Dateifortschritt, Abbruch und Export
  vollständiger Berichte. Die Katalogsperre wird nicht während des Hashens gehalten.
  Ansichtswechsel lassen den Auftrag weiterlaufen; Installations-/Sprachwechsel
  brechen ihn ab. Höchstens ein Auftrag pro Appinstanz; keine Teilberichtfreigabe.
- Der Lauf ist bei laufendem oder unbekanntem Spielstatus gesperrt. Prozesschecks
  auch während des Lesens und am Ende; erkannter Spielstart beendet die Prüfung.
  Keine Startsperren, Game-Writes oder Savezugriffe. Handles erlauben weiterhin
  Schreiben/Löschen; tatsächliche Handlepfade, Typ und Linkzahl werden geprüft.
- CLI `installation-verify` ergänzt den getrennten Metadatenbefehl. Bei
  Inhaltsabweichungen gibt es einen Diagnosebericht und Exitcode 2; bei Abbruch
  keinen vollständigen Bericht. Berichte bleiben Beobachtungen des unbestätigten
  Steam-Caches: `certified_vanilla` und `can_apply` sind immer false.

**118 Rust-Tests, 4 Frontend-Unit-Tests und 21 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` fehlerfrei. Zehn neue Core-Tests decken Hashvektoren,
Mehrblock-/Leerdateien, gleich große Manipulationen, Abbruch, Prozessfehler/Start,
Quell-/Depotwechsel, Wachstum, fehlende/fremde/hart verlinkte Quellen, die echte
Prozessschranke und gleichzeitige Schreibbarkeit der Lesehandles ab. Zwei neue
Worker-Tests prüfen Exklusivität, Sitzungsgrenzen, Abbruch und Panikbehandlung.
Vier neue UI-Flows decken Sperre, Fortschritt, Ansichtswechsel, Abbruch, Fehler,
Abweichungen und Export ab. Die vier Echtdateitests und 69 bisherigen
Transaktions-Abbruchpunkte bleiben eingeschlossen.

Der erste Gesamtlauf fand eine Windows-Testkollision: Eine eigene `probe.exe`
eines anderen Tests endete zwischen Prozesssnapshot und Pfadabfrage (Fehler 87).
Die Prozessprüfung verweigerte korrekt den Zugriff. Der isolierte Test bestand;
die gleichnamigen Fixture-Lebenszeiten sind nun serialisiert. Alle unveränderten
Schutzprüfungen bestehen im erneuten parallelen Gesamtlauf. Zwei erste UI-Fälle
scheiterten an wiederverwendeten veränderlichen Mockobjekten; die Fixture bildet
jetzt die echten, bei jedem IPC deserialisierten Antworten ab.

Die finale **Release-EXE v0.4.5** bestand die native WebView2-Abnahme:
285 passende Metadateneinträge, korrekt gesperrter Inhaltsknopf während des Spiels,
alle neuen Befehle in der lokalen Capability und Ablehnung ungültiger Sitzungen.
Die bestehende Modvorschau mit 34.488 Änderungen, Export, geschützte Projektproben,
Wiederanwendung, Recovery, Updateablehnung und Händler-/Dropoptionen bleiben grün.
1024px ohne Überlaufen, Screenshot geprüft, keine JavaScript-Fehler.
CLI-Recovery derselben v3-Probe erfolgreich; nur die eigene unsichtbare
Testinstanz beendet. Alle 38 beobachteten EXE-/Metadatenhashes danach unverändert.
156 Git-sichtbare Dateien ohne Spiel-/Saveinhalte.

Release: `target/release/crimson-workbench.exe`, 13.844.992 Bytes, SHA-256
`3c78d6c0d86716695987256d753403c1961b1f36a0b08a6f9f2c08bcf2b52bc6`.
Nachweise: `.local/phase4-v6-validation.json`, `.local/phase4-v6-native/result.json`,
`.local/phase4-v6-rust-tests-final.log`, `.local/phase4-v6-installation.json`,
`.local/phase4-v6-cli-recovery.json`.
[Bedienung, Prüfumfang und Grenzen](CONTENT_AUDIT.md).

**Phase 4 bleibt offen.** Der Inhaltsprüfer ist implementiert; der Nutzer hat
den vollständigen 154-GB-Lauf anschließend erfolgreich ausgeführt (Nachtrag oben).
Unabhängiger Vanilla-Nachweis, abgesicherte Live-Start-/Ladepfade, Live-Backup-/
Updateworkflow und In-game-Abnahmen fehlen weiterhin. Kein Live-Schreibbefehl,
keine automatische Steam-Verifikation und kein Beginn von Phase 5.

## Phase 4: v0.4.4 – Steam-Depot- und Verzeichnisinventur

- Neue Aktion **Dateiliste prüfen** in der Modwerkstatt und CLI-Befehl
  `installation-check`: Verzeichnisbaum, alle installierten Depotlisten,
  Registry-Gruppen und sämtliche EXE-Funde innerhalb der Installation.
- Eigener begrenzter Depot-/Protobuf-Reader mit Payload-CRC32, Identitäts-/
  Größenprüfung und Konflikterkennung. Fehlende oder widersprüchliche Caches
  werden nicht als erfolgreicher Teilvergleich ausgegeben. Unsichere Pfade,
  Duplikate, verschlüsselte Namen und Links werden abgewiesen.
- Fehlende, zusätzliche, größenabweichende oder anders typisierte Einträge
  sind getrennt sichtbar. Verzeichnisverknüpfungen werden nicht verfolgt.
  Nicht installierte optionale Registry-Gruppen gelten nicht als Fehler.
- Der Bericht bleibt eine lesende Momentaufnahme: keine PAZ-/EXE-/DLL-Inhalte,
  keine Startsperre, keine Steam-Verifikation und keinerlei Schreibzugriff auf
  die Installation. Passende Namen/Größen bestätigen weder Inhalt noch Vanilla;
  sämtliche Vertrauens-/Applyflags bleiben false, auch bei Signaturblobs im Cache.

An der echten Installation: **285 erwartete und 285 gefundene Dateien**, alle
Namen/Größen passend, keine zusätzlichen oder fehlenden Einträge. Zwei installierte
Depots umfassen laut Metadaten 154.097.618.899 Bytes. Drei EXEs erfasst:
`CrimsonDesert.exe`, `crashpad_handler.exe`, `pers.exe` unter `bin64/`.
Das Spiel lief weiter; Registry entspricht der bisherigen Beobachtung.
[Bedienung, Grenzen und Quellen](INSTALLATION_CHECK.md).

**106 Rust-Tests, 4 Frontend-Unit-Tests und 17 Headless-UI-Flows bestanden.**
Fmt/Clippy mit `-D warnings` fehlerfrei. Neue Fälle: abgeschnittene/manipulierte
Depotlisten, falsche Identität/Größe, fehlende/widersprüchliche Caches, doppelte
VDF-Schlüssel, Update-Metadaten, beschädigte Registry, zusätzliche EXEs/Gruppen,
Typ-/Größenänderungen, absichtlich gleich große unerkannte Inhaltsänderungen
und eine echte Windows-Junction außerhalb des geprüften Baums.
Die vier Echtdateitests und bisherigen 69 Transaktions-Abbruchpunkte bleiben grün.

Die erste native Abnahme fand eine fehlende Tauri-ACL-Zuordnung für den neuen
Lesebefehl. `build.rs` und die lokale Main-Window-Capability wurden ergänzt;
kein allgemeiner oder Remote-Zugriff hinzugefügt. Der Planselektor im nativen
Test wurde außerdem auf die Modvorschau eingegrenzt, da die Installationsprüfung
nun ebenfalls einen Ergebniskopf verwendet.

Die finale **Release-EXE v0.4.4** bestand die erneute native WebView2-Abnahme:
285 passende Dateien, drei EXEs, keine Inventurabweichungen, bestehender Plan mit
34.488 Änderungen, Export, zwei geschützte Apply/Reapply/Restore-Zyklen,
Abbruch-Recovery und Updateablehnung. Tagesrefresh, Artikelergänzungen,
Chancen/Mengen/Garantien und Händlerausnahmen bleiben geprüft. 1024px ohne
Überlaufen, Screenshot geprüft, keine JavaScript-Fehler. CLI-Recovery derselben
v3-Probe ebenfalls erfolgreich. Nur die eigene unsichtbare Testinstanz beendet.
Alle 38 beobachteten EXE-/Metadatenhashes sind auch danach unverändert;
kein vollständiger PAZ-Hashlauf. 150 Git-sichtbare Dateien ohne Spiel-/Saveinhalte.

Release: `target/release/crimson-workbench.exe`, 13.586.944 Bytes, SHA-256
`589dba1b4ca254e23bf90845b97f5a2993f2aa1996d99284fca8994100a60aaa`.
Nachweise: `.local/phase4-v5-validation.json`,
`.local/phase4-v5-rust-tests-final.log`, `.local/phase4-v5-installation.json`,
`.local/phase4-v5-native/result.json`, `.local/phase4-v5-cli-recovery.json`.

**Phase 4 bleibt offen:** Die Verzeichnis-/Depotliste ist jetzt implementiert;
volle Inhaltsprüfung und vertrauenswürdiger Vanilla-Nachweis, Absicherung aller
tatsächlichen Live-Start-/Ladepfade, Live-Backup-/Updateworkflow und In-game-Abnahmen
fehlen weiterhin. Live-Schreiben und Phase 5 wurden nicht begonnen.

## Phase 4: v0.4.3 – gehaltene Sperren und Händlerdetails

- Neue v3-Projektproben: Eine private Sitzung hält Start-, Quell-, Backup- und
  Ordnersperren über Planung, Apply/Reapply, Restore und Recovery. Vor und nach
  dem Erwerb sowie an Schreibgrenzen werden tatsächliche Prozesspfade geprüft.
  Marker und Schutzmanifest sind über SHA-256 gebunden; der Herkunftstyp bleibt
  ausdrücklich `project-copy-only`, niemals zertifiziertes Vanilla.
- Vollständige Hashprüfung der erfassten Probequellen aus gehaltenen Handles.
  Änderungen an einer Ausgangsdatei verhindern Wiederherstellung über einen
  neuen/fremden Zustand, sowohl bei offener Transaktion als auch nach Commit.
  Backups werden erneut geprüft und über die Sitzung gegen Änderungen gehalten.
- Die Desktop-Probe verwendet ausschließlich eine Kopie unserer eigenen EXE
  und eine synthetische Quelldatei. Zwei Apply/Reapply/Restore-Zyklen, Recovery
  und eine absichtliche Quelländerung samt Ablehnung sind integriert. Windows
  ist erforderlich; kein ungesicherter Ersatzpfad. V2-Proben bleiben separat
  wiederherstellbar; ein v3-Downgrade wird abgewiesen.
- Händlerdetails: individuelle Artikelsets ersetzen die gemeinsame Auswahl,
  leere Sets unterdrücken globale Ergänzungen. Refresh je Händler kann täglich
  sein, das Originalintervall bewahren oder wieder global erben. Explizite
  Ausnahmen gelten auch außerhalb der globalen Händlerauswahl. Bestandsausnahmen
  bleiben separat; alle Änderungen werden aus dem Original neu aufgebaut.

**Prüfung: 96 Rust-Tests, 4 Frontend-Unit-Tests, 15 Headless-UI-Flows bestanden.**
Fmt und Clippy mit `-D warnings` fehlerfrei. Die vier Echtdateitests und die 69
bisherigen Transaktions-Abbruchpunkte sind eingeschlossen. Neue Schutztests
widerlegten zwei ursprüngliche Windows-Annahmen (laufende Images und reine
Attribut-Handles); zusätzlich wurde der Prozesspfadvergleich beidseitig
kanonisiert. Diese Fälle bestehen nach den Korrekturen unverändert.

Die finale **Release-EXE v0.4.3** bestand den nativen WebView2-Test: bestehender
34.488-Änderungen-Plan, geschützte Probezyklen, Abbruch-Recovery, Updateablehnung,
246 Tagesrefresh-Änderungen, wachsende Shoptabelle, Chancen/Mengen/Garantien und
individuelle Händlerauswahl (2200 ersetzt global gewähltes 50001). Keine
JavaScript-Fehler und 1024px ohne Überlaufen. Ein mehrdeutiger Testselektor wurde
präzisiert; dieselbe unveränderte App bestand den vollständigen Wiederholungslauf.
Die aktualisierte CLI stellte anschließend dieselbe v3-Probe erneut korrekt her.

Das Spiel lief währenddessen weiter. Ausschließlich eigene unsichtbare
Testinstanzen wurden gestartet/geschlossen; keine Spiel- oder Save-Sperren.
Alle 38 beobachteten EXE-/Metadaten-Dateien sind unverändert, kein vollständiger
PAZ-Hashvergleich. 145 Git-sichtbare Dateien ohne Spiel-/Save-/Cacheinhalte.

Release: `target/release/crimson-workbench.exe`, 13.431.296 Bytes, SHA-256
`341eca06ab9957a0a6007c3cf6c1a87b569e9051552f7526db0a63022990d13e`.
Nachweise: `.local/phase4-v4-validation.json`, `.local/phase4-v4-rust-tests.log`,
`.local/phase4-v4-native/result.json`, `.local/phase4-v4-cli-recovery.json`.
[Schutzablauf und Grenzen](PROTECTED_REHEARSALS.md).

**Phase 4 ist weiterhin nicht abgeschlossen.** Offen sind die vertrauenswürdig
bestätigte vollständige Spielbasis, das vollständige Live-Startpfad-/Quellverzeichnis,
der darauf aufbauende Live-Backup-/Updateworkflow und In-game-Abnahmen. Unbekannte
Sonderhändler und der vollständige Katalog bei allen Händlern gleichzeitig sind
weiterhin nicht freigegeben. Es gibt keinen Live-Schreibbefehl; Phase 5 wurde
nicht begonnen und keine Saveinhalte wurden gelesen.

## Phase 4: v0.4.2 – Sortimente, Tagesrefresh und Dropchancen (historisch)

- Zusatzartikel und Artikelsets aus allen 6.816 Items bei 208 unterstützten
  Händlern. Bestehende Positionen, Bedingungen und Save-Indizes bleiben erhalten;
  neue Positionen übernehmen normale Preisfaktoren desselben Händlers. Der
  Tabellenheader wird bei Wachstum neu aufgebaut. Ein vollständiger Katalog
  bei einem einzelnen Händler ist am echten Datenformat geprüft.
- Täglicher Refresh an den belegten Tagesfeldern: 369 unterstützte Intervalle,
  davon 246 Änderungen von 3/7 auf 1. Unbekannte/permanente Intervalle und
  einmalige Warenflags bleiben erhalten; die acht September-Bytes bleiben roh.
- Chancenmultiplikatoren und Einzelausnahmen für 12.306 unbedingte Item-Dropsets
  mit unabhängigem Rolltyp 0, begrenzt auf 100 %. Manuelle Garantieauswahl setzt
  die Basisrate auf 100 % und hat Vorrang vor Chancenfaktoren. Mengen und Chancen
  werden gemeinsam aus den Originaldaten neu berechnet.
- Eigene statische EXE-/Tabellenanalyse belegt Refresh-/Stock-Indizes und den
  Rolltyp-0-Nenner 1.000.000. Gewichtete und begrenzte Rolltypen werden nicht als
  unabhängige Prozentchancen interpretiert. Keine automatische Bossklassifikation.
- Steam-Cache genauer untersucht: Beide installierten Depotmanifeste enthalten
  Dateihashes, jedoch leere Signaturabschnitte. Sie zertifizieren die Basis nicht
  unabhängig. Keine Steam-Prüfung, kein Download oder Vollarchivhashlauf gestartet.

Prüfung: **87 Rust-Tests, 4 Frontend-Unit-Tests und 14 Headless-UI-Flows bestanden**,
Fmt und Clippy mit `-D warnings` fehlerfrei. Der erweiterte Echtdateitest für das
vollständige Sortiment bestand anschließend ebenfalls. Bisherige 69
Transaktions-Abbruchszenarien und Recovery-Tests bleiben eingeschlossen.

Die finale Release-EXE **v0.4.2** bestand den echten WebView2-Test: bestehender
34.488-Änderungen-Plan, Tagesrefresh mit 246 Änderungen, zwei neue Artikel bei
Händler 3101, Goblin-Rate 35.000 → 70.000 und Menge 1 → 3 sowie manuelle Basisrate
1.000.000. Export und zwei Apply/Reapply/Restore-Zyklen plus Recovery bestanden
auch mit gewachsener Shoptabelle. Keine JavaScript-Fehler, 1024px ohne Überlaufen.
Ein anfänglicher Umlautfehler im Testskript wurde korrigiert; die App-EXE blieb
dabei unverändert. Das Spiel lief weiter, nur die eigene unsichtbare Testinstanz
wurde geschlossen. Alle 38 beobachteten EXE-/Metadatenhashes sind unverändert.

Release: `target/release/crimson-workbench.exe`, 13.281.792 Bytes, SHA-256
`ac3edb3dc5824ef9bb966260917efdd33b13f828a2f72a08572912a71eceeff6`.
Nachweise: `.local/phase4-v3-validation.json`, `.local/phase4-v3-rust-tests.log`,
`.local/phase4-v3-all-items-test.log` und `.local/phase4-v3-native/result.json`.
Feldpositionen, Datenabdeckung und Quellen:
[B1/B2-Feldnachweise](research/PHASE4_FIELDS.md).

**Phase 4 bleibt offen.** B0 benötigt die vertrauenswürdig bestätigte Vanilla-Basis,
Live-Backup-/Updatepfade, integrierten Startschutz und manuelle In-game-Abnahme.
Noch keine vollständige Stromausfallabsicherung und kein Live-Schreibbefehl.
B1 unterstützt keine Ergänzungen bei allen Sonderhändlern, keine unterschiedlichen
Artikelsets je Händler innerhalb eines Plans und nicht alle Items bei allen
Händlern gleichzeitig: 100.000 Händler-/Itemkombinationen pro Vorschau sind eine
Workbench-Grenze, kein bestätigtes Engine-Maximum. B2-Garantien betreffen die
Einzelchance beim Settrigger, nicht übergeordnete Ereignisse/Laufzeitmodifikatoren.
Gameplaywirkung von B1–B3 und große Sortimente bleiben manuell zu prüfen.
Phase 5 wurde nicht begonnen; Spielstände wurden nicht geöffnet.

## Phase 4: v0.4.1 – Wiederanwendung und Recovery (historisch)

- Neuer privater Transaktionskern: bis zu acht getrennte Overlaygruppen,
  Reapply mit neuen IDs, Registry-Commit vor Entfernung alter Gruppen und
  vollständiger Dateiänderungsplan mit Vorher-/Nachherhashes.
- Unveränderliche Intents und Abschlüsse, Hashprüfung der Historie, verifizierte
  Registry-Sicherung, Erholung nach Teilwrites und erneuten Recovery-Abbrüchen.
  Unvollständige private Dateien bleiben zur Diagnose erhalten.
- Zwei echte Apply → Reapply → Restore-Zyklen plus absichtlicher Abbruch/Recovery
  pro Desktop-Probe; sechs einsehbare Dateiübergänge. CLI `mod-recover` stellt
  ausschließlich markierte Projektproben wieder her.
- Schutz gegen fremde Dateien, Registry-/Backupabweichungen, Hardlinks,
  Reparse-Points, Gruppenkollisionen und doppelte virtuelle Tabellenpfade.

Offen bleiben unabhängige vollständige Vanilla-Nachweise, Live-Backup-/Updatepfade,
integrierter Startschutz, Stromausfall- und In-game-Abnahme. B1/B2-Feldlücken aus
v0.4.0 bestehen weiter. **Phase 4 ist nicht abgeschlossen; kein Live-Schreibpfad.**
Der alte v1-Kern bleibt unverändert als Testreferenz erhalten; Proben werden nicht
automatisch migriert.

Prüfung: **83 Rust-Tests**, **4 Frontend-Unit-Tests**, **13 Headless-UI-Flows**
bestanden; Fmt und Clippy mit `-D warnings` fehlerfrei. Der vollständige
Workspace-Lauf umfasste 82 Tests, anschließend bestand die Formatsuite mit
zusätzlichem Längen-Regressionsfall (18 statt 17 Tests). Die 69 injizierten
Apply-/Reapply-/Restore-Abbrüche und unterbrochene Recovery sind eingeschlossen.

Die finale **Release-EXE v0.4.1** bestand den nativen WebView2-Test mit echten
Daten: 34.488 Feldänderungen, zwei Apply/Reapply/Restore-Zyklen, Recovery,
sechs Dateiübergänge, Export/Credits, unveränderte Trust-Strafe, veraltete
Vorschau gesperrt und 1024px-Darstellung ohne horizontales Überlaufen.
Keine JavaScript-Fehler. Das Spiel lief weiter; ausschließlich die eigene
unsichtbare Testinstanz wurde geschlossen. Der CLI-Recovery-Aufruf derselben
bereits wiederhergestellten Probe bestand ebenfalls.

Release: `target/release/crimson-workbench.exe`, 13.178.368 Bytes, SHA-256
`7cf40162604c74395ef818d0d23630f3bde1dacad3418b6569847e133b96d173`.
Alle 38 erfassten EXE-/Metadaten-Dateien stimmen mit dem Abschluss von v0.4.0
überein; keine vollständige PAZ-Hashprüfung und keine Vanilla-Zertifizierung.
140 Git-sichtbare Dateien ohne Spiel-/Save-/Cacheinhalte; 69 relative Dokumentlinks
geprüft. Nachweise: `.local/phase4-v2-validation.json`,
`.local/phase4-v2-rust-tests.log`, `.local/phase4-v2-format-tests.log`,
`.local/phase4-v2-native/result.json` und `.local/phase4-v2-cli-recovery.json`.

## Phase 4: v0.4.0 – erster Vorschau-Stand

- Desktopseite **Modwerkstatt**: Shopbestand vorhandener Positionen,
  Dropmengen und positive Trust-Zuwächse, Auswahl und Einzelausnahmen,
  überprüfbarer Feld-/Dateiplan sowie Export noch nicht angewendeter Overlays.
- Eigener deterministischer PAZ/PAMT-Builder mit Verschlüsselung, Kompression,
  Alignment und Prüfsummenkette; Registry-Eintrag vor Vanilla, kollisionsfreie
  Gruppenwahl unter Beachtung reservierter Gruppen. Keine CDUMM-Abhängigkeit.
- Privater Transaktionskern für **Projektkopien**: exklusiver Lock, überprüftes
  Registry-Backup, Journal, Registry zuletzt ersetzen, eigene Dateien entfernen,
  Hashvergleich und Wiederanlauf an getesteten Unterbrechungsgrenzen.
- Echte Datenabdeckung: 397 Händler / 6.376 Warenpositionen,
  12.736 Mengen-Dropsets, drei Friendly-Records. Weitere 39 Händler,
  1.709 Sonder-Dropsets sowie 299 leere/Sentinel-Item-Dropsets bleiben unverändert.
- Donate +50 und Talk +5 werden bei ×3 zu +150/+15; Threat −200 bleibt erhalten.
  Das ist ein bestätigter Byte-Nachweis, kein In-game-Wirkungstest.

**Offen vor Abschluss von Phase 4:** vollständige unabhängige Vanilla-Basis und
Live-Backup, integrierter Startschutz, umfassendere Staging-/Crash-Recovery,
produktives Reapply mit Gruppenwechsel, mehrere Gruppen und manuelle In-game-
Abnahme. B1: neue Artikel/Artikelsets/alles und täglicher Refresh. B2: Chancen-
und Rollsemantik sowie garantierte Bossdrops. A1/A2-Quellenmapping bleibt offen.
Die Benutzeroberfläche und CLI besitzen **keinen Live-Schreibbefehl**.

Prüfung: **74 Rust-Tests**, **4 Frontend-Unit-Tests**, **13 Headless-UI-Flows**
bestanden. Fmt und Clippy mit `-D warnings` grün. Reale Body-/Header-Roundtrips,
unveränderte Records außerhalb der Auswahl, unbekannte September-Bytes,
verschlüsseltes Overlay-Rücklesen und sechs Apply-/fünf Restore-Unterbrechungen
geprüft. Ein Startschutztest verwendet ausschließlich eine kopierte eigene
Test-EXE. Linux und echte Stromausfälle wurden nicht getestet.

Native Prüfung der **finalen Release-EXE v0.4.0** erfolgreich: Shops 999,
Dropmengen ×2, Trust ×3 ergeben 34.488 Änderungen in drei geplanten Dateien.
Zwei Apply-/Restore-Zyklen an einer Projektkopie, Export mit vollständigen
Änderungen/Credits, veraltete Vorschau gesperrt, 1024px-Darstellung und keine
JavaScript-Fehler. Das Spiel lief dabei weiter; die unsichtbare eigene
Workbench-Testinstanz wurde danach geschlossen. Ein erster Testfilter auf
„Friendly“ traf auch reguläre Item-Dropsets; mit dem korrekten Modulfilter
„trust“ bestand dieselbe unveränderte Release-EXE den vollständigen Test.

Release: `target/release/crimson-workbench.exe`, 13.021.696 Bytes, SHA-256
`0ed85abcd034c634780f080d25653120b566439b811e80245433a00e43814b5f`.
Nachweise unter `.local/phase4-rust-tests.log`, `.local/phase4-native/result.json`
und `.local/phase4-verification.jsonl`. [Bedienung und Grenzen](MODS.md).
Abschlussvergleich: alle 38 zuvor erfassten EXE-/Metadaten-Dateien unverändert;
kein vollständiger PAZ-Hashvergleich und keine Vanilla-Zertifizierung.
137 Git-sichtbare Dateien ohne Spiel-/Save-/Cacheinhalte, 69 gültige relative
Dokumentlinks. Zusammengefasster Nachweis: `.local/phase4-validation.json`.

Die folgenden Abschnitte bewahren den damaligen Stand früherer Phasen.

## Phase 3: aktueller Stand

- 18.576 Rezeptrecords und 1.602 Materialgruppen vollständig typisiert gelesen
  und byteidentisch rekonstruiert. 13.035 einfache Item-Dropsets ebenfalls;
  1.712 Sondervarianten bleiben ausdrücklich opak.
- **1.108 berechenbare Rezepte**; 17.069 Verstärkungs-/Sonderverbrauchszeilen und
  399 nicht freigegebene Ergebnisvarianten werden ausgeschlossen.
- Ziel und Menge, mehrstufiger Materialbaum, aggregierter Restbedarf, gemeinsame
  Vorräte, Wiederverwendung von Chargenüberschüssen, Rezept-/Materialwahl,
  manuelle Beschaffung von Zwischenprodukten und sichtbare Zykluswarnungen.
- Neue Desktopseite **Herstellungsplan**, echte Icons, Rezeptlinks in Itemdetails,
  sowie CLI-Kommandos `recipes` und `craft`. Mengen bleiben als Dezimalstrings exakt.
- Unbekannte Weltquellen und Händlerangaben bleiben sichtbar ungeklärt.
  Dropset-IDs sind keine behaupteten Gegner- oder Fundortzuordnungen.
- Voraussetzungen werden angezeigt, nicht gegen einen Save geprüft. Keine
  Bestpreisoptimierung, Mischung von Materialalternativen oder Save-Import.

Prüfung: **65 Rust-Tests**, **4 Frontend-Unit-Tests**, **9 Headless-UI-Flows**
bestanden. Darunter echte Roundtrips der aktuellen Installation und Planung
jedes freigegebenen Rezepts. Native WebView2-Prüfung mit separatem unsichtbarem
Workbench-Fenster: Pfeile 31 → 60, Vorräte, Materialalternativen, Item-Rezeptlinks
und 1024px-Darstellung erfolgreich; keine JavaScript-Fehler. Das eigene Testfenster
wurde anschließend beendet. Spiel und sichtbarer Desktop wurden nicht bedient.

[Format, Mengenbelege, Bedienung und Grenzen](CRAFTING.md).
Lokale Nachweise: `.local/phase3-rust-tests.log`, `.local/phase3-native/result.json`
und Screenshots in `.local/phase3-native/`.

Finale Release-Prüfung: `target/release/crimson-workbench.exe` v0.3.0,
12.508.672 Bytes, SHA-256
`d89c9eff1e91f7eca5225125a09e4b8151ec84b05486855adc92a164ec4914c2`.
Herstellungs-Smoke-Test mit genau dieser EXE erfolgreich. Der anschließende
Itembrowser-Regressionscheck (Item 2200, echtes Icon, Rohfelder, geschützter
JSON-Export, ger → eng) ebenfalls erfolgreich; Ergebnis unter
`.local/phase3-item-regression/result.json`. Das kombinierte Testskript benötigte
zunächst einen expliziten Wechsel zurück zur Itemseite; nach dieser reinen
Testkorrektur lief die unveränderte Release-App durch.

CLI-Planexport erfolgreich geprüft. Alle 38 ursprünglich erfassten
EXE-/Metadatenhashes sind weiterhin identisch; kein vollständiger PAZ-Hashvergleich.
123 Git-sichtbare Dateien, keine Spiel-/Save-/Cachedateien darunter.
`cargo fmt --all -- --check` und Clippy mit `-D warnings` grün. Die neue
Header-/Body-Roundtripprüfung wurde anschließend erneut mit echten Dateien
bestätigt. Linux wurde lokal nicht getestet.

**Haltepunkt zur Durchsicht:** Phase 4 / Apply wurde nicht begonnen. A2 ist beim
Quellenmapping noch unvollständig; dieser Punkt wird nicht als erfüllt verbucht.
Die folgenden Abschnitte bewahren den damaligen Stand der früheren Phasen.

## Phase 2: gelieferter Stand

- Tauri-2-App mit React, TypeScript, Vite, Zustand und virtualisierter TanStack-
  Tabelle. Dunkles Layout, Itemliste mit fester Detailansicht und Datenquellenseite.
- 6.816 reale Items, FTS5-Suche, kombinierte Typ-/Kategorie-/Tier-/Stapel-/Stat-ID-
  Filter, globale Sortierung, Nachladen und Tastaturbedienung.
- Echte lokale Icons: zusätzliches SHA-geprüftes `stringinfo`, **31.812** typisierte
  Stringrecords mit byteidentischem Body-/Header-Roundtrip, Item-Key → String →
  DDS → PNG. Reale DXT5-Textur von Item 2200 erfolgreich angezeigt.
- Detailübersicht, alle Rohfelder mit Typ/Offset/Hexbytes/Wert, Feldsuche und
  Filter für unbekannte Felder. Große Ganzzahlen bleiben im Frontend exakt.
- Direkte typisierte Itemreferenzen auf existierende Datensätze; keine Links aus
  beliebigen u32-Werten. JSON-Ausgabe als neue Datei ausschließlich unter `exports/`.
- 15 wählbare Sprachen. Sitzungs- und Anfrage-IDs verhindern veraltete Ergebnisse;
  Sprach-/Buildindexe sind getrennt. Aktuelles Core-Leseschema: `…gamedata-2.3-v2`.
- Lade-/Leer-/Fehlerzustände, gesperrte unbekannte Builds, Pfadauswahl und manuelles
  Neueinlesen. Sechs explizit erlaubte native Kommandos, keine FS-/Shell-/Apply-
  Plugins. Keine Produktionstestdaten oder Netzwerk-Assets in der Oberfläche.

**A1 ist noch nicht vollständig:** Händler/Stadt/Region/Preis/Bestand,
DropSet-Quellen und Rezeptbeziehungen fehlen. Der vorhandene MIT-Storeloader
parst nur Namen/Keys und dokumentiert selbst variable, nicht zuverlässig gelesene
Stockstrukturen. Ungeprüfte Quellen werden deshalb nicht aus Byteheuristiken
behauptet. Kategorien/Stats bleiben teilweise rohe IDs; ihre Gameplaybezeichnungen
und Einheiten sind nicht erfunden. [Details und Bedienung](DESKTOP.md).

| Aktuelle Prüfung | Ergebnis |
|---|---|
| Gesamter Rust-Workspace | **57 Tests bestanden**, 0 fehlgeschlagen: 1 App-, 3 CLI-, 36 Core-, 15 Format- und 2 tatsächlich ausgeführte Echtdateitests. |
| Frontend | **4 Unit-Tests bestanden**, TypeScript und Vite-Build erfolgreich. |
| Headless Edge UI | **6 End-to-End-Tests bestanden**: Suche/Details/Export/Links, Virtualisierung/Pagination/Filter, veraltete Antworten, unbekannte Builds, 1024px-Leerzustand und Browser ohne Desktopbridge. Fixtures sind ausdrücklich synthetisch und nur in Tests. |
| Echte Tauri-/WebView2-App | Unsichtbares separates Testfenster: reales Item 2200 „Stumpfpfeil“, echtes PNG-Icon, 210 Felder, `max_stack_count=100`, JSON-Ausgabe unter `exports/`, Sprachwechsel zu „Stub Arrow“; **keine JavaScript-Fehler**. |
| Finale Windows-EXE | `npm run desktop:build` erfolgreich; `target/release/crimson-workbench.exe`, 12.057.088 Bytes. Derselbe native UI-Test wurde anschließend auch mit genau dieser Release-EXE erfolgreich ausgeführt. |
| Formatierung / Clippy | `cargo fmt --all` und `cargo clippy --workspace --all-targets --locked -- -D warnings` erfolgreich. |
| Abschließender Installationsvergleich | Alle 38 ursprünglich erfassten EXE-/Metadatenhashes weiterhin identisch; kein vollständiger PAZ-Hashvergleich. |
| Repositoryprüfung | 109 Git-sichtbare Dateien, keine Spiel-/Save-/Cache-/Builddateien darunter; alle relativen Markdownlinks gültig. |
| Plattformgrenze | Windows tatsächlich gebaut und getestet. Linux-CI einschließlich Tauri-Systembibliotheken eingerichtet; lokal kein Linux-Testnachweis. |

Lokale Belege: `.local/phase2-rust-tests.log`, `.local/phase2-native/result.json`,
`.local/phase2-native/item-database.png`, `.local/phase2-native/data-sources.png`.
Release-Prüfprotokoll: `.local/phase2-native-release.log`.
Die echte UI-Prüfung lief unsichtbar und die eigene Testinstanz wurde danach
geschlossen. Spielprozess und sichtbarer Desktop wurden nicht bedient.

**Historischer Haltepunkt nach Phase 2:** Phase 3 war damals nicht begonnen. Das offene A1-Quellenmapping
ist kein als erfolgreich deklarierter Test und keine freigegebene Modifikation.
Der folgende Abschnitt bewahrt den Phase-1-Nachweis als Verlauf.

## Phase 1: Core und CLI – abgeschlossener Nachweis

- Rust-Workspace mit `cd-core`, `cd-cli` und einer begrenzten nativen
  [MIT-Formatportierung](FORMAT_PORT.md). Keine Python- oder Modmanagerabhängigkeit.
- Installationserkennung für Steam-Bibliotheken, Epic-Manifeste und Game Pass,
  einschließlich expliziter Auswahl, Xbox-Content-Verzeichnissen und Proton-Saves.
  Savepfade werden nur gefunden; Saveinhalte werden nicht gelesen.
- PAZ/PAMT/PAPGT lesen, Checksummen prüfen, unterstützte Codecs entschlüsseln und
  dekomprimieren. Unbekannte Codecs, beschädigte Strukturen und übergroße Eingaben
  werden abgelehnt. Kein vollständiges Entpacken der Installation nötig.
- Versioniertes Leseschema `steam-25381195-gamedata-2.3-v1`: EXE-Version, 38
  EXE-/Metadatendateien, alle 28 verwendeten Tabellendateien und die ausgewählte
  Item-PALOC müssen zu den geprüften SHA-256-Hashes passen. Unbekannte Builds
  erhalten nur Diagnosen, keine semantische Interpretation.
- 6.816 typisierte Items mit vollständigen Feldbereichen; 13 weitere Tabellen
  haben geprüfte Schlüssel-/Offsetindizes und unverändert erhaltene Rohdatensätze.
  Diese Rohdaten sind keine bestätigten Shop-, Rezept-, Quest- oder Skillschemata.
- Item-Lokalisierung in 15 verifizierten Sprachen, Deutsch als Standard.
  Numerische PALOC-Referenzen werden direkt aufgelöst; Fallbacks bleiben erkennbar.
- Transaktionaler SQLite-FTS5-Index für Namen, Beschreibungen und interne Schlüssel,
  Typ-/Kategorie-ID-Filter, Pagination und automatischer Neuaufbau bei anderem
  Fingerprint, Leseschema oder anderer Sprache.
- CLI: `detect`, `fingerprint`, `languages`, `tables`, `dump`, `roundtrip`,
  `index`, `search`, `item`, `diff`. [Anleitung und API](CORE.md).
- JSON-Dateien und SQLite-Caches ausschließlich unter `.local/` oder `exports/`
  im Projekt. Neue JSON-Ausgaben überschreiben nichts. Spiel-/Save-Pfade,
  Junction-Umleitungen und problematische Hardlinks/SQLite-Sidecars sind gesperrt.

## Ausgeführte Prüfungen

Windows, Rust 1.95; lokale Steam-App `3321460`, Build `25381195`, EXE `1.0.0.2944`.

| Prüfung | Ergebnis und Grenze |
|---|---|
| `cargo test --workspace --locked -- --test-threads=2` | **51 bestanden**, 0 fehlgeschlagen: 3 CLI-, 32 Core-, 15 Format- und 1 tatsächlich ausgeführter Echtdateitest. |
| `cargo fmt --all -- --check` | Erfolgreich. |
| `cargo clippy --workspace --all-targets --locked -- -D warnings` | Erfolgreich, keine Warnungen. |
| `cargo build --release --locked -p cd-cli` | Erfolgreich; ausführbar unter `target/release/cd-cli.exe`. |
| Release-CLI-End-to-End-Prüfung | 20 Prüfungen bestanden: Erkennung, Sprachen, Fingerprint, 14 Tabellen, Roundtrip, Indexaufbau/-wiederverwendung, deutsche FTS-Suche, Item-/Dumpdaten, Sprachwechsel, Builddiff, Ausgabeschutz, unbekannter Build und abschließende Metadatenhashes. |
| Echte Dateien parse → serialize | **34 byteidentische Prüfungen**: 14 Body-/Headerpaare, zusätzlicher typisierter Itembody/-header, PALOC-Payload/-Container, PAPGT und PAMT. Andere Tabellenbodys bleiben ausdrücklich opaque. |
| Vollständige Item-Feldabdeckung | Alle 6.816 Datensätze geprüft; nativer Formatverifier erfasste **1.763.837** zusammenhängende Feldbereiche ohne Lücken/Überlappungen. |
| Lokalisierungsverifier | Alle 15 Item-PALOCs gelesen, gehasht und verlustfrei erhalten; Deutsch enthält 13.575 Einträge. Payload wird neu serialisiert, ursprüngliche komprimierte Hülle unverändert erhalten. Keine Behauptung byteidentischer Neukompression. |
| Unveränderte Installationsmetadaten | Abschließend alle **38** erfassten EXE-/Metadatendateien weiterhin mit den Ausgangshashes identisch. Kein vollständiger Hashvergleich aller PAZ-Dateien. |
| Linux | Ubuntu-/Windows-CI konfiguriert; Linux wurde hier nicht ausgeführt. Kein laufender Docker-Daemon, kein lokaler Linux-Buildnachweis. |
| Frontend | Beginnt in Phase 2; noch keine Frontendtests anwendbar. |

Lokale, aus Git ausgeschlossene Belege: `.local/phase1-tests.log`,
`.local/phase1-smoke.log`, `.local/phase1-smoke-20260919-162802/`.
Forschungshilfen unter `.local/` sind keine Laufzeitabhängigkeiten.

## Offene Grenzen und Entscheidungen

- Der passende Lesebuild ist **keine Vanilla-Zertifizierung**.
  `certified_vanilla` bleibt falsch. Apply/Restore, Backups, Fremdmodbewertung,
  Prozesssperre, Crash-Recovery und manuelle In-game-Nachweise gehören zu Phase 4.
- Steam wurde auf dieser Maschine erkannt und benutzt. Epic-/Game-Pass-Erkennung
  ist mit synthetischen Manifesten geprüft; deren abweichende Datenbuilds sind
  nicht automatisch freigegeben. Weitere Builds benötigen eigene Hashes/Schemata.
- Itemstruktur und Feldbytes sind vollständig erfasst, aber nicht jede
  Feldbedeutung/Gameplaywirkung ist bestätigt. Unbekannte Felder sind roh markiert;
  Kategorien bleiben IDs, Icons bleiben Referenzen. Keine erfundenen Labels.
- Quest-/Stage-Indizes haben einen 32-Bit-Count; andere erfasste Indizes einen
  16-Bit-Count. Das wurde anhand der aktuellen Dateien geprüft. Storebody und seine
  September-Erweiterungen werden noch nicht semantisch interpretiert.
- Rezepte, Verkäufer-/Dropset-Beziehungen, Kartenatlas/Koordinaten, vollständige
  Collectibles, Inventar-/Stack-Maxima und verschiedene Spielwirkungen bleiben
  Gegenstand der späteren Phasen. `reference/mods/` enthält noch keine Mods.
- Ein unverändertes Cache-Indexschema wird wiederverwendet. Die CLI prüft trotzdem
  bei jedem Start die Installation; noch keine dauerhafte GUI-Sitzung oder Watcher.
- Die unveränderte crimson-rs-Gitdependency war wegen privater Module und fehlender
  `rlib`-API ungeeignet. Daher gezielte, dokumentierte MIT-Portierung statt fremdem
  Manager. Quellen und Einschränkungen stehen in [CREDITS.md](../CREDITS.md).

Es wurden keine Spiel-/Save-Dateien verändert, keine Saveinhalte gelesen, kein
Spielprozess gesteuert und weder Steam-Verify noch Apply/Restore ausgeführt.
Zwischenzeitliche Clippy-Befunde und zwei Review-Randfälle (Xbox-Content-Auswahl,
geschlossene stdout-Pipe) wurden behoben; der abschließende Stand ist grün.

## Phase 0 und nächster Haltepunkt

Phase 0 ist abgeschlossen. Ihre Recherche bleibt in [FORMATS.md](FORMATS.md),
[FEASIBILITY.md](FEASIBILITY.md), [APPLY_DESIGN.md](APPLY_DESIGN.md),
[ARCHIVES.md](research/ARCHIVES.md), [TABLES.md](research/TABLES.md) und
[APPLY.md](research/APPLY.md) erhalten. Die [Buildbeobachtung](builds/steam-25381195.observed.json)
enthält Hashes und damalige Befunde, keine Spielinhalte.

Auf das Go hin wurde Phase 2 wie oben dokumentiert umgesetzt. Modexport,
Apply-Engine und Save-Reader bleiben in ihrer vorgesehenen Phase;
Runtime-Phase D bleibt separat.
