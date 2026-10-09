# Reparatur: lesender Spieladapter 0.3.0

Aktuelle Ergänzung 0.9.0: Der geschützte Pfad erfasst ausschließlich gehaltene
Besitzer und durchsucht die Registry nicht erneut. Seine Pflichtprüfungen und
die gemeinsame native Probe stehen in [PINNED_CAPTURE_INTEGRATION](PINNED_CAPTURE_INTEGRATION.md).
Die folgenden Adressen beschreiben weiterhin den historischen Build 2944.
Aktuelle 2949-Adressen und native Vergleichstests: [INVENTORY_2949](INVENTORY_2949.md).

Stand 21.09.2026, Steam 25381195 / EXE 1.0.0.2944. Ein eigener Leser für die
aktuelle Datenstruktur ist implementiert. Er liefert getrennte, besessene
Item-/Sockelabbilder an die Reparaturplanung. **Keine Spieltransaktion, kein
Loader und kein Zugriff auf das laufende Spiel.**

## Erfassung

`repair_reader.h`, `src/reader.cpp` und `src/reader_win.cpp` bilden die statische
Bibliothek `crimson_repair_reader.lib`. `LocalMemory` liest ausschließlich den
aufrufenden Prozess, prüft Seitengrenzen und fängt Fehler der eigenen Leseoperation
ab. Es gibt keine PID-Auswahl, keinen Prozesshandle und keine Schreibschnittstelle.
Die Tests verwenden ausschließlich ihr eigenes Prozess-/Fixturespeicherabbild.

Der Clientanker führt zum aktuell ausgewählten Charakter. Der Leser sucht dessen
Kennung gezielt in der Server-Registry; er durchsucht weder den Heap noch alle
Charaktere. Die vollständige Kennung muss übereinstimmen. Typprüfung,
Possessor-Rückverweis und Komponentenbesitz sind zusätzliche konservative
Prüfungen. Eine Übereinstimmung von Iteminhalten genügt nicht als Spieleridentität.

Erfasst werden nur der Character-Container und ausgerüstete Items. Andere
Container werden anhand ihres Typs übersprungen, ohne ihre Itemdaten zu lesen.
Leere Slots, Mengen <= 0 und die Ausschlussliste des Inventargetters werden
berücksichtigt. Inventarslots sind zusammenhängende `0xc8`-Byte-Einträge;
Ausrüstungseinträge umfassen `0xd0` Bytes einschließlich Slottag. Sockeldaten
werden getrennt kopiert; gespeicherte Pointer werden nie von der Aktion dereferenziert.

UID, Position und Haltbarkeits-/Sockeldaten müssen in beiden Repräsentationen
zusammenpassen. Doppelte IDs/Slots, unlesbare Bereiche, unbekannte Definitionen,
ungeeignete Geometrie oder fehlende Spielerkennung verwerfen die gesamte Ausgabe.
Alle gelesenen Bereiche werden vor Veröffentlichung erneut gelesen und verglichen.
**Dieser Vergleich ersetzt keine Engine-Sperre** und erkennt insbesondere keine
unbemerkten A→B→A-Wechsel zwischen den Lesezeitpunkten.

Budgets: 128 Inventarcontainer, 64 Ausrüstungseinträge, zusammen 2.048 vorhandene
Items je Repräsentation, 64 Sockelplätze je Item, 65.536 Registry-Buckets/-Indizes,
31 Einträge je Registry-Bucket, insgesamt 16 MiB und 65.536 Leseoperationen
einschließlich Kontrolllesen. Dies sind lokale Verarbeitungsgrenzen, keine
behaupteten maximalen Spiel-/Save-Kapazitäten.

## Statischer Nachweis im aktuellen Build

Vollständiger EXE-SHA-256:
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Alle Code-/Globaladressen sind RVAs. Die Quell-EXE wurde ausschließlich gelesen.

| Pfad | Befund und Grenze |
|---|---|
| Clientglobal `0x6d691b0` → `+0x30` → Manager `+0x50` | Statisch belegter Client-Kontext; Konstruktion bei `0xacf5f0`, Zugriff u. a. über `0x8b4480`. Die benachbarte globale Adresse `0x6d691b8` ist kein austauschbarer Serveranker. |
| Serverglobal `0x6d696c0` → `+0x48` | Serverseitiger Equipment-Aufrufer `0x2a94c40` benutzt diesen Manager für `0x2a82730`. |
| Server-Registry `0x2a82730` | Handleprüfung, Schlüssel `handle & 0xfffff`, Bucketwahl per Modulo, 256-Byte-Buckets; Key/Index ab `+8`, Node-Key `+4`, Charakterpointer `+8`. Erwirbt echte Engine-Zugriffe; im Test nur durch Stubs vertreten. |
| Actor `+0x60`, `+0x68`, `+0xa0` | Vollständige Kennung, Komponenten und Possessor; Possessor `+0xd0` muss zurückweisen. Komponenten: Inventar `+0xb8`, Ausrüstung `+0x38`. Besitzprüfung zusätzlich über Komponente `+8`. |
| Inventargetter `0x212f4a0` | Holder `+0x18` Pointerarray, `+0x20` Anzahl; Bucket `+0x10` Typ, `+0x0c` signed Slotgrenze, `+0` zusammenhängende Items. Ausschlussliste `+0x20/+0x28`, 12-Byte-Records mit Itemkey am Anfang. |
| Holdergetter `0x212a0f0` | Actor-/Possessor-/Komponentenpfad; je Charakterdefinition direkt oder über besessenen Actor. Native Tests prüfen direkte, umgeleitete und fehlende Besitzer. |
| Equipment-Wrapper `0x20c90e0` | Komponente `+0x90` → Tabelle `+8/+0x10`; Stride `0xd0`, Slottag `+0xc8`. |

Die Auswahl der Typ-Tags 1/4/9 berücksichtigt Hinweise der MIT-Referenz
[Trinity, Commit 70c9a00](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b)
zu alternativen Protagonisten. Der aktuelle Typgetter wurde isoliert ausgeführt;
Tag allein identifiziert keinen aktiven Spieler. Echte Charakterwechsel und die
Zulässigkeit der zusätzlichen Besitzprüfungen in allen Spielzuständen sind noch
nicht nachgewiesen. Kein fremder Resolver-/Hookcode wurde übernommen.

## Native Vergleichsprobe

| Funktion | Länge | SHA-256 |
|---|---:|---|
| Inventargetter `0x212f4a0` | 226 | `959437d3eff481d79aa0d6e9bb4ead3bc54ab2f408d80cddcf689c3b33506c9d` |
| Holdergetter `0x212a0f0` | 127 | `e4b46826485c932d72fb7c175b0e4c3943c8dee8e6cd01a5378ab170f17162c5` |
| Registrylookup `0x2a82730` | 542 | `df6ba99ef13614b5a8b8cbcd7c200127a176788905134ff9b445a91c6cf43c68` |
| Actor-Typgetter samt Sprungtabelle `0x466060` | 88 | `5f31dba7c7e552dc6983e9288297f246d87d5fb9bfb90506e905b121ecc916a8` |

Die unveränderte Inventarfunktion und der eigene Leser erhalten dieselben
künstlichen Container. Gültige, negative/zu große, leere, verbrauchte und
ausgefilterte Slots liefern nach gemeinsamer Presence-Normalisierung identische
Ergebnisse. Der native Getter kann für ein leeres Item weiterhin dessen Slotpointer
liefern; der aufrufende Pfad muss Key und Menge prüfen. Der eigene Leser gibt
für solche Einträge bereits Adresse 0 zurück und lehnt zusätzliche Mehrdeutigkeiten ab.

Der Inventargetter besteht aus vier `pdata`-Fragmenten. Der Testhost registriert
alle vier und erhält die geprüften `UNW_FLAG_CHAININFO`-Verweise auf den ersten
Prolog. Ein absichtlich unzugänglicher Slot im eigenen Testprozess bestätigt auch
die Exception-Rückkehr durch diese Kette. Kein Fault wurde im Spiel ausgelöst.

Zwölf native Registryfälle prüfen Hash-/Indexzugriff, ungültige Handles,
leere/falsche Einträge, Typablehnung, Zugriffablehnung und balancierte Lockaufrufe.
**Die Lock- und Actor-Zugriffsmethoden sind nichtwerfende Test-Stubs.** Der Test
beweist deren aufgerufene ABI, nicht die Implementierung einer echten Engine-Lease.
Das Ergebnis kann trotz ungültiger Lease einen Actorpointer enthalten: Das
Validitätsbyte `+0x10` muss ausgewertet werden. Der Testhost ruft keine Methoden
der vom Spielcode in die Ausgabe geschriebenen VTable-Adressen auf.

## Ergänzung 0.6.0

`capture_with_references` besitzt beide nativen Referenzen während der Erfassung
und gibt sie erst nach den Inventarsperren zurück. Unter den Sperren prüft es
zusätzlich vollständige Kennung sowie Alive-/Zerstörungszustand. Geschützte
Erwerbsquellen und Engine-TLS müssen noch vom Spielhost angebunden werden.
[Referenzvertrag und aktuelle Prüfung](REFERENCE_INTEGRATION.md).

## Verbleibende Integration

Ergänzung 0.5.0: `capture_with_locks` hält während der Erfassung beide vom Host
gebundenen Besitzer-Sperren. Originale Lock-Methoden sind separat mit echten
Windows-SRW-Sperren auf privaten Testobjekten geprüft. Der Lebensdauerschutz
der Besitzer bleibt Voraussetzung des Hosts und ist noch nicht integriert.
Beim Rücksprung sind die Sperren frei. [Aktueller Sperrvertrag](LOCK_INTEGRATION.md).
Die oben beschriebenen Registryproben behalten ihre Test-Stubs.

Vor Nutzung im Spiel braucht der Leser einen Host, der den vollständigen Build
und die ursprünglichen/aktiven Tabellen verifiziert, echte Engine-Leases hält
und bei jedem Lade-/Weltwechsel eine neue Epochennummer liefert. `Frame` enthält
diese Hostwerte; der Leser erfindet sie nicht und prüft selbst keine Datei-Hashes.
Die Übereinstimmung der vollständigen Client-/Server-Charakterkennung und der
Item-IDs ist konservativ verlangt, für alle echten Übergänge aber noch unbewiesen.

Danach fehlt weiterhin der bestätigte Engine-Commit: Haltbarkeit ändern,
Inventar-/Equipment-Ereignisse und abgeleitete Effekte aktualisieren, Ergebnis
bestätigen und Persistenz dem Spiel überlassen. Ein Vergleichslesen oder ein
doppelter Rohspeicherwrite erfüllt diesen Vertrag nicht. Bedieneingabe, Loader
und B0-Installation sind ebenfalls noch offen; `installable` bleibt `false`.

Historischer Stand 0.3.0: 124 Leserprüfungen, 1.377 Aktionsprüfungen und insgesamt 116 native Aufrufe mit
1.387 Prüfbedingungen bestanden. Reproduktion: [README](README.md).
Vollständiger Commitvertrag: [ACTION_INTEGRATION.md](ACTION_INTEGRATION.md).
