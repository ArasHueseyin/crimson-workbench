# Reparatur: isolierter nativer Ausführungsnachweis

## Aktueller Stand: Inventar-Paketserializer, Modul 0.23.0

Originalen Sender und Streamauswahl auf privaten Daten ausgeführt. 36 neue Fälle,
davon zehn mit Transaktion und nativem Client-Ack. Insgesamt 454 native Szenarien,
4.812 Aufrufe, neun CTest-Suiten. Zehn Nutzbytes enthalten Actor, Container, Slot
und Hauptwert; keine Item-UID, Sockel oder Auftragskennung. Der eigene Ablauf
verweigert Erfolg bei Zustellfehlern oder falschen/frisch geänderten Identitäten.
Transport, erfolgreicher Poolzweig und Server-/Speicherabschluss bleiben offen.
[Nachweis](../../runtime/repair/INVENTORY_PACKET_2949.md).

## Historischer Stand: native Pufferaufbereitung, Modul 0.22.0

Originalen Encoder, Pufferverwaltung und LZ4-Kompression auf privaten Daten
ausgeführt. 66 neue Fälle; insgesamt 418 native Szenarien/4.289 Aufrufe und neun
CTest-Suiten bestanden. Inhalte, Header, Lebensdauer und Wiederholung nach
Öffnungsfehler geprüft. Wiederholung kann doppelt komprimieren; neuer Rohdaten-
aufbau ist erforderlich. Nachgelagerte Verarbeitung und Dateioperationen bleiben
Callbacks; vollständige Integrität/Verschlüsselung, Speicherung und Spielhost offen.
[Nachweis](../../runtime/repair/SAVE_ENCODING_2949.md).

## Historischer Stand: Dateischreibhelfer, Modul 0.21.0

Originalen Eintragsschreiber und zwei Bereinigungshelfer auf privaten Puffern
ausgeführt. 34 neue Fälle, insgesamt 352 native Szenarien/4.095 Aufrufe und neun
CTest-Suiten bestanden. Flush-/Closefehler können trotzdem Erfolg, Payload-Freigabe
und gelöschtes Dirty erzeugen. Datei-/Encoderempfänger sind eigene Callbacks;
die Integration ersetzt den Queue-/Backupweg durch eine einzelne Eintragsbrücke.
Echter Encoder und Schreibwrapper statisch zugeordnet. Vollständige Kodierung,
Abschlussbestätigung und Spielanbindung bleiben offen.
[Nachweis](../../runtime/repair/SAVE_FILE_2949.md).

## Historischer Stand: Speicherdispatcher und Warteschlangen, Modul 0.20.0

Originaldispatcher, Steam-Zulassungsprüfung und Queue-Marker auf privaten
Objekten ausgeführt. 39 neue Fälle; insgesamt 318 native Szenarien/4.039 Aufrufe
und neun CTest-Suiten bestanden. Skip-Gates können Erfolg ohne Backendarbeit
melden; ein vorheriger Erfolgsstatus bleibt dabei erhalten. Alle vier Fehlerstufen,
Quota-Grenzen sowie drei Queue-Arten in beiden Allokationsmodi sind geprüft.
Datei- und Plattformempfänger bleiben eigene Callbacks. Konkrete Dateihelfer
und Timer sind statisch zugeordnet; der Schreibhelfer ignoriert den Flush-Status.
Native Dateipufferverarbeitung, reguläre Speicherung/Bestätigung und tatsächlicher
Spielhost bleiben offen. [Nachweis](../../runtime/repair/SAVE_BACKEND_2949.md).

## Historischer Stand: native Speicherobjekt-Umwandlung, Modul 0.19.0

Originale Item-/ItemSaveData-Konverter und ihre Lebensdauerabhängigkeiten sind
auf privaten Objekten ausgeführt: 38 neue Fälle, davon 14 mit tatsächlicher
Reparaturplanung/Nachherkopie. Haupt-/Sockelwerte, native Normalisierung bei
0/No-Wear, Färbedaten, Reset und Wiederverwendung geprüft. Insgesamt 279 native
Szenarien/3.950 Aufrufe und neun CTest-Suiten bestanden.
Das belegt Umwandlungen im Arbeitsspeicher; reguläre Serialisierung, Speicherabschluss,
weitere optionale Daten und vollständige Spielanbindung bleiben offen.
[Nachweis](../../runtime/repair/ITEM_SAVE_2949.md).

## Historischer Stand: SQL-Ausführungs-Shim, Modul 0.18.0

Vollständiger Ausführungskörper und gemeinsamer Request-Erwerb auf privaten
Daten geprüft. 19 neue Fälle; insgesamt 241 native Szenarien/3.674 Aufrufe und
neun CTest-Suiten bestanden. Der Shim kann Erfolg liefern, ohne den Request zu
lesen; er führt selbst keinen Datenbankauftrag aus. Alte Request-Fehler bleiben
im Umgehungszweig erhalten. Weder Rückgabe 0 noch geleerte Slot-Markierungen
belegen eine Speicherung. Die nachgelagerte alternative Verarbeitung und
Speicher-/Ladesemantik bleiben offen. Tatsächlicher Schalterwert im Spiel unbekannt.
[Nachweis](../../runtime/repair/SQL_DISPATCH_2949.md).
Die konkrete alternative Transaktionsmethode meldet ebenfalls nur Erfolg.
Separat sind Item-/ItemSaveData-Umwandlungen für Hauptwert und Sockel statisch
belegt. Ihre native Ausführung und tatsächliche Speicherung stehen aus.
[Speicherobjekt-Nachweis](../../runtime/repair/ITEM_SAVE_2949.md).

## Vorheriger Stand: Inventar-Client-Rückmeldung, Modul 0.17.0

Originaler Client-Ack für Inventarreparatur auf privaten Quellen und im gemeinsamen
Adapter ausgeführt. 36 neue Fälle, gesamt 222 native Szenarien mit 3.526 Aufrufen;
neun CTest-Suiten bestanden. Der Ack setzt Hauptwerte absolut, prüft keine UID
und verändert keine Sockel. Unabhängige Bestätigung verweigert UID-Wechsel,
fehlende/doppelte Meldungen und Abschlussfehler. Inventar-Server, tatsächliche
Persistenz-/Sockel-Ladesemantik und echte UI-/Effekt-/Transportwege bleiben offen.
Der untersuchte Socket-SQL-Helper speichert eine ItemInfo-Kennung, keinen belegten
Haltbarkeitswert. [Nachweis](../../runtime/repair/INVENTORY_EVENTS_2949.md).

## Vorheriger Stand: Slot-Verarbeitung und Batchabschluss, Modul 0.16.0

Der gemeinsame Ablauf schließt erst nach allen Itemmeldungen ab. Originaler
Slot-Verbraucher, wiederverwendbares Clear und native Tabellenfreigabe sind auf
privaten Komponenten integriert. Neun CTest-Suiten, 43 Ablaufszenarien und
186 native Szenarien mit 2.826 Aufrufen bestanden, einschließlich 24 Verbraucher-
und 18 Ausrüstungsfällen. Persistenzfehler nach geleerter Liste und erfolgreichem
Client-Ack erzeugen keinen Erfolg. Der Persistenzempfänger ist künstlich; der
belegte Weg überträgt keine Sockeldatensätze. Tatsächliche Persistenz samt
Sockelweg, Inventarereignisse, echte Empfänger/Rückmeldungen und Spielhost fehlen.
[Nachweis](../../runtime/repair/PERSISTENCE_2949.md).

## Vorheriger Stand: gemeinsamer Ablauf, Modul 0.15.0

Vorbereitete native Nachherkopien, Feldschreiber und Meldungsadapter verbunden;
die 15 Ausrüstungsfälle nutzen den gemeinsamen Produktionsadapter. Neun CTest-
Suiten, 40 Ablaufszenarien und 159 native Szenarien mit 2.023 Aufrufen bestanden.
Sockelanzahl getrennt von logischer Platzgrenze, Zusatzvektor-Strides korrigiert.
Konkrete Inventarereignisse, Slot-Verbraucher, echte Empfänger/Rückmeldezuordnung
und Spielhost fehlen weiterhin. [Nachweis](../../runtime/repair/TRANSACTION_2949.md).

## Vorheriger Stand: Haltbarkeits-Feldschreiber, Modul 0.14.0

Eigener vollständiger Feldschritt für Inventar und Ausrüstung, beide
Zustandskopien, mit durchgehend gehaltenen Besitzern/Sperren implementiert.
Gesamte Vorprüfung, nur geänderte 16-Bit-Felder, vollständiges Kontrolllesen und
unklares Ergebnis bei Teilfehlern. Acht CTest-Suiten, 34 Schreibszenarien und
141 native Szenarien bestanden. Die Engine-Transaktion mit vorbereiteten nativen
Kopien und vollständigen Ereignissen bleibt offen, ebenso der Spielhost.
[Nachweis](../../runtime/repair/FIELD_WRITER_2949.md).

## Vorheriger Stand: Slot-Markierung und gehaltene Erfassung, Modul 0.13.0

Durchgehend gehaltene Besitzerreferenzen und Sperren für Erfassung, Planung
und Refresh implementiert. Originale Ausrüstungs-Slot-Markierung samt Platzsuche,
Kollisionen und Rehash auf privaten Komponenten ausgeführt und vor der
Benachrichtigung in die Ereignisprobe eingebunden. Sieben CTest-Suiten,
519 Leserbedingungen und 133 native Szenarien mit 1.296 Aufrufen bestanden.
Slot-Consumer, vollständiger Schreiber, Inventarereignisse und Host bleiben offen.
[Nachweis](../../runtime/repair/DIRTY_SLOTS_2949.md).

## Vorheriger Stand: native Itemkopien für 2949, Modul 0.12.0

Native Lebensdauerverwaltung je Itemwert, mit originalen Konstruktor-, Kopier-
und Destruktorfunktionen sowie allen verwendeten Listenhelfern geprüft. Die
Ereignisprobe nutzt diese Nachherkopien. Sieben CTest-Suiten, 30 Besitzerbedingungen
und 117 native Szenarien mit 851 Aufrufen und 13.639 Bedingungen bestanden.
Der eigene Heap und Effekt-/Transport-/UI-Empfänger bleiben Testabhängigkeiten.
Schreibweg, Slot-Aktualisierung, Inventarereignisse und Spielhost bleiben offen.
[Nachweis](../../runtime/repair/ITEM_LIFECYCLE_2949.md).

## Vorheriger Stand: Ausrüstungsereignisse für 2949, Modul 0.11.0

Eigene Ereignisvorbereitung aus dem Plan und native Verbindung von Server-Notifier,
Client-Ack und Sockel-Collector auf privaten reparierten Items. Sechs CTest-Suiten,
1.537 Aktionsbedingungen, 103 native Szenarien mit 758 Aufrufen und 9.311 Bedingungen
bestanden. Effekt-/Kind-/Transport-/UI-Aufrufentscheidungen geprüft; ihre tatsächliche
Engine-Verarbeitung bleibt durch Testempfänger ersetzt. Kein vollständiger Commit,
Spielhost oder installierbarer Mod. [Nachweis](../../runtime/repair/EQUIPMENT_EVENTS_2949.md).

## Vorheriger Stand: Inventarzugriffe für 2949, Modul 0.10.0

Eigene Belege für aktuelle Inventar-/Besitzergetter, Clientauswahl, Equipment-
Slotwahl und Sockelfelder. Sechs CTest-Suiten, 385 Leserbedingungen und 88 native
Szenarien mit 578 Aufrufen und 6.219 Bedingungen bestanden. Originalgetter und
Leser stimmen auf denselben privaten Quellen überein; Leser-Build muss explizit
bekannt sein. Der Equipment-Wrapper ist mit Delta 0 geprüft, nicht als vollständiger
Schreib-/Ereignisweg. Host-/Threadbindung und Reparaturtransaktion bleiben offen.
[Nachweise und Grenzen](../../runtime/repair/INVENTORY_2949.md).

## Vorheriger Stand: direkte Erfassung aus gehaltenen Referenzen 0.9.0

Die geschützte Erfassung liest keine ungeschützte Registry mehr. Der neue
gemeinsame Nachweis verbindet originale 2949-Referenz-/Sperrmethoden und den
Inventarleser auf privaten Legacy-Fixtures: 48 Szenarien, 503 native Aufrufe,
3.867 Bedingungen; zusätzlich 362 Leserbedingungen und sechs CTest-Suiten.
Inventarlayout-/Spieler-/Threadbindung und Transaktion bleiben offen.
[Implementierung und genaue Grenzen](../../runtime/repair/PINNED_CAPTURE_INTEGRATION.md).

## Vorheriger Stand: gemeinsamer geschützter Erwerb 0.8.0

Die neue isolierte Probe verbindet originale Client-/Server-Lookups auf 1.0.0.2949
mit konkreten Normal-/User-Referenzmethoden und Actor-/PaPtr-Freigabe. Tatsächliche
Windows-SRW-Sperren schützen den Erwerb. 30 Szenarien, 263 native Aufrufe, 3.490
Bedingungen und sechs CTest-Suiten bestanden. Dazu gehören konkurrierende
Registry-Schreibsperren und zwischenzeitlich entfernte Einträge.
Der direkte Erwerb akzeptiert Alive=0; der separate Leser muss dies zusätzlich
unter den Besitzersperren abweisen. Vollständige Live-Auflösung, Threadbindung,
Änderungstransaktion und Ereignisse fehlen. Threadmap/Bereinigung bleiben
Fixture-Abhängigkeiten. [Details](../../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

## Quellenadapter 0.7.0 (vorheriger Stand)

Manager-Lookup und PaPtr-Freigabe sind als kontextgebundene Quellen implementiert.
55 synthetische Adapterbedingungen und insgesamt fünf CTest-Suiten bestanden.
Die installierte EXE wurde inzwischen als 1.0.0.2949 / Steam 25455892 erkannt.
Der bisherige Host verweigert sie vor Ausführung. Neue Untersuchungen dürfen
nicht mit den nativen Nachweisen für 1.0.0.2944 vermischt werden.
[Adapter, Updategrenze und verbleibende Arbeit](../../runtime/repair/REGISTRY_INTEGRATION.md).

Nachtrag zu v0.5.9: Tabellen des neuen Builds sind separat geprüft und unterstützt.
Acht relevante native VTables wurden erneut aus der exakt hashgeprüften EXE
zugeordnet. Die neue Server-Lookup-Adresse ist `0x2a82300`; frühere vorläufige
v7-Adressen werden nicht übernommen. Keine neue native Ausführung erfolgt.
[Reproduzierbare statische Beobachtung](../../runtime/repair/BUILD_2949.md).

## Referenzverwaltung 0.6.0 (vorheriger Stand)

Native Referenzbelege werden während der Inventarerfassung gehalten und erst
nach den Inventarsperren freigegeben. Alive-/Zerstörungszustand und vollständige
Kennung werden unter Sperren kontrolliert. Der originale direkte Erwerb kann
Alive=0 akzeptieren; ein gültiger Beleg allein erlaubt noch keine Reparatur.

Die konkreten Actor-VTables verwenden für Release `0x1436690`, nicht den
Basispfad `0x14345c0`. Der Override kann Zustand 0x20 → 0x40 und serverseitige
Bereinigung auslösen. Native Proben prüfen diese Wege mit dokumentierten
Fixture-Abhängigkeiten. Vier CTest-Suiten, 249 Leser-/Referenzbedingungen,
203 native Aufrufe und 3.859 Bedingungen bestanden. Geschützte Live-Quellen,
Hostbindung, Transaktion und Installation fehlen weiterhin.
[Adressen, Hashes, Testgrenzen](../../runtime/repair/REFERENCE_INTEGRATION.md).

## Besitzer-Sperren 0.5.0 (vorheriger Stand)

Nicht wartende Sperrgruppe und darunter erfasste Inventarkopien implementiert.
Try `0x1371c20` und Release `0x1371bf0` sind im privaten Testhost mit echten
Windows-SRW-Imports geprüft. TLS der Try-Kopie wird an genau einem Lesezugriff
auf private Thread-Daten umgeleitet; keine Änderung eines Spielprozesses.

Vier CTest-Suiten, 1.481 Aktionsbedingungen, 163 Leserbedingungen, 25
Gruppenbedingungen; 140 native Aufrufe und 2.408 Bedingungen bestanden.
Die separat untersuchte Actor-Referenzverwaltung ist noch nicht integriert.
Sperrgruppe und Leser setzen bereits gegen Freigabe geschützte Besitzer voraus.
Auch vollständige Engine-Transaktion und Installation fehlen.
[Methoden, Hashes und Grenzen](../../runtime/repair/LOCK_INTEGRATION.md).

## Abschlusssteuerung 0.4.0 (vorheriger Stand)

Asynchrone Annahme ist jetzt vom bestätigten Reparaturerfolg getrennt. Queue und
Nachherwertprüfung verlangen eine passende Rückmeldung für Auftrag und Sitzung,
beide aktualisierten Itemabbilder und bestätigte Engine-Ereignisse. Teilfehler,
Weltwechsel und Timeout sperren weitere Aufträge ohne Retry oder Rücksetzung.

Der Client-Ack `0x98fb70` verändert Daten vor dem Vergleich mit der Servermeldung.
Die Funktion `0x240e3e0` liest nicht bloß Sockel: Sie entfernt verbrauchte Einsätze
und liefert deren Liste zurück. Ein späterer Fehler lässt diese Änderungen
bestehen. Sechs native Fälle prüfen das, korrekte vorab reparierte Abbilder sowie
Fehlercode 0 ohne Reparatur. Originaler Lock-Konstruktor und Sockelentferner,
Ack mit genau einer TLS-Umleitung; UI-/Allocator-/Lock-Abhängigkeiten sind Stubs.

1.481 Aktionsbedingungen, 124 Leserbedingungen, 122 native Aufrufe mit 2.221
Bedingungen bestanden. Kein Engine-Ereignis tatsächlich verschickt; keine
Spieltransaktion oder Installation. Methodentabelle, Hashes, Änderungen der
privaten Funktionskopie und Grenzen: [ACK_INTEGRATION.md](../../runtime/repair/ACK_INTEGRATION.md).

## Lesender Adapter 0.3.0 (vorheriger Stand)

Der eigene Leser folgt der aktuellen Client-/Server-Struktur und stellt der
Aktion getrennte Inventar-/Equipmentabbilder bereit. Charakterkennung, Besitzer,
UID, Position und beide Itemkopien werden geprüft; Kontrolllesen erkennt
währenddessen geänderte Daten. Unbekannte/mehrdeutige Zustände werden abgewiesen.

Inventargetter `0x212f4a0` und eigener Leser liefern für dieselben künstlichen
Container identische vorhandene Slots. Der Holdergetter `0x212a0f0` bestätigt
Actor-/Possessor-/Komponentenpfade. Serverlookup `0x2a82730` und Typgetter
`0x466060` sind isoliert mit künstlichen Registry-/Actor-Locks geprüft. Der
Lookup kann bei ungültiger Lease einen Pointer behalten; Validitätsbyte erforderlich.
Keine echten Engine-Locks oder Spieltransaktionen implementiert.

124 Leserprüfungen, 1.377 Aktionsprüfungen und 116 native Aufrufe mit 1.387
Bedingungen bestanden. Aktuelle Globals, strukturierte Pfade, Funktionshashes,
Unwind-Ketten und ausdrücklich unbestätigte Integrationsannahmen:
[READER_LAYOUT.md](../../runtime/repair/READER_LAYOUT.md).

## Eigene Aktion 0.2.0 (vorheriger Stand)

Zusätzlich zum unten dokumentierten Kosten-Prototyp ist die vom Nutzer
freigegebene eigene Reparaturaktion entwickelt. Sie repariert vorhandene
Hauptitems und Sockeleinsätze auf privaten Speicherabbildern; keine Reparatur-
regel und kein Material erforderlich. 1.377 Aktionsprüfungen bestanden.

Der Testhost lädt zusätzlich eine hashgebundene Kopie des Item-Updaters
`0x240d650` (405 Bytes). Positive Delta-Arithmetik repariert Sockel nicht
zuverlässig; Delta +65 setzt einen 10/100-Sockel auf 0. Die eigene Aktion
setzt jeden Wert unabhängig. Originalcode bestätigt anschließend die volle
Haltbarkeit, normalen Verschleiß und die Kombination mit No-Wear.
Damit insgesamt 83 native Aufrufe und 1.252 Bedingungen bestanden.

Der Spieladapter ist noch nicht implementiert: weder eigene Bedieneingabe noch
Spielerresolver, vollständige Engine-Transaktion/Benachrichtigung oder Loader.
Die verschobene manuelle Spielabnahme ist davon getrennt. Technische Details,
Layoutabweichungen zu älteren Quellen und Integrationsvertrag:
[ACTION_INTEGRATION.md](../../runtime/repair/ACTION_INTEGRATION.md).

## Kosten-Prototyp 0.1.0 (vorheriger Stand)

21.09.2026. Vom Nutzer freigegebene Laufzeitentwicklung für B8.
Quellcode: `runtime/repair/`; kein Eingriff in die laufende Installation.
Die Resultate ergänzen die statischen [Feldnachweise](ADVANCED_TABLES.md).

## Bindung an den Build

EXE-SHA-256: `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Alle folgenden Adressen sind **RVAs**, ohne die bevorzugte Basis `0x140000000`.

| Funktion | RVA | Länge | SHA-256 des vollständigen Körpers |
|---|---:|---:|---|
| Gemeinsamer Reparaturhelper | 0x240dfb0 | 437 | 61e7429f0415e591df434017a4500a6d93a4e059b11155741fd18ef5d4733aaf |
| Serverseitiger Reparaturhelper | 0x2be3ee0 | 691 | 3f90895c1cb5e7fcdaa2599f74b18a0e6eba0b6b867f7668de2708e491d13bd5 |
| Materialauftrag vorbereiten | 0x27b04c0 | 280 | Über vollständigen EXE-Hash und pdata-Grenzen gebunden |
| Materialauftrag ausführen | 0x27b05e0 | 630 | Über vollständigen EXE-Hash und pdata-Grenzen gebunden |

Im Repository stehen ausschließlich Strukturinformationen, Hashes und eigene
Ersatzanweisungen. Originalcode wird erst im privaten Testhost aus der lokalen
EXE gelesen. Keine Originalbytes oder Spieltabellen werden ausgeliefert.

## Koordinierter Kandidat

| Bereich, Ende exklusiv | Änderung |
|---|---|
| 0x240e0d1–0x240e0ef | Negativprüfungen, Kostenregister EBX auf 0, Materialdivision/-klemme umgehen |
| 0x2be4052–0x2be4077 | Entsprechende Prüfung und Kostenregister R14 auf 0 |
| 0x2be40bf–0x2be4162 | Keinen Materialauftrag anfügen; Erfolg über das originale Epilog melden |

Beide vollständigen Funktionskopien werden gemeinsam geprüft und vorbereitet.
Der Kostenhelper `0x2411f20` bleibt unverändert. Maximalhaltbarkeit, Reparaturmenge,
Ressourcenregel, Fehlerpfade und die ursprünglichen Prologe/Epiloge bleiben
außerhalb der drei Bereiche bytegleich. Bereits vorhandene Materialaufträge
werden nicht entfernt oder geändert.

Die beiden Material-Transaktionsfunktionen akzeptieren im nativen Test eine
leere Liste ohne weitere Inventarzugriffe. Der Commit prüft Count bei
`0x27b0618` und gelangt bei 0 nach `0x27b07dc`, wo Erfolg geschrieben wird.
Dies bestätigt die kostenlose Materialseite innerhalb dieses Ausschnitts.

## Ausführung und Grenzen

79 native Funktionsaufrufe bestanden: 63 Aufrufe der originalen/veränderten
Reparaturhelper und 16 Prepare-/Commit-Aufrufe für leere Materiallisten.
Abgedeckt sind volle Reparatur, Restmengen, Schrittweiten, große Kosten,
Original-Mengensonderwert -1, fehlendes Material, leere Listen, unpassende Regel,
volle/unbegrenzte Haltbarkeit, negative Mengen/Kosten, Autorisierungsfehler,
vorhandene Materialaufträge und Rücksetzen auf den Originalcode.

Die Originalhelper erzeugen bei künstlichem Preis 0 reproduzierbar eine
Integer-Division-durch-null-Exception im Testhost. Der koordinierte Kandidat
passiert dieselben Eingaben mit korrekter berechneter Haltbarkeit und Kosten 0.
Die Fehler wurden nie im Spiel ausgelöst.

Itemgetter, Kostenresolver und Autorisierungsservice sind Stubs; Errorcodes
sind künstliche Konstanten. Reale Tabellen enthalten weiterhin keine positive
Reparaturregel. Die Tests beweisen weder die Erreichbarkeit der Reparatur-UI
noch tatsächliche Aktualisierung/Synchronisierung einer Spielinstanz.
Ein allgemeiner Hook, RepairAll, sichere Startaktivierung und B0-Installation
sind ausdrücklich nicht implementiert. Daher keine freigegebene Modfunktion.

Der erneute Aufrufer-Audit findet 16 direkte Referenzen. Im früheren privaten
Scanner konnte eine vorhergehende opcodeähnliche Bytefolge einen echten E8
verschlucken; der neue Lookahead-Scanner mit Instruktionsprüfung findet auch
`0x240e0cc`. Drei synthetische Regressionen sichern dieses Verhalten ab.
Indirekte Aufrufe bleiben außerhalb des Auditumfangs.

Nachweise: `.local/repair-runtime-v1-{build-test.log,audit.json,validation.json}`.
Technische Grundlage für Seitenschutz und Cachepflege:
[Microsoft VirtualProtect](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect).
Für einen späteren Loader gilt außerdem die
[Microsoft-Dokumentation zur DLL-Initialisierung](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).
