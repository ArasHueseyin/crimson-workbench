# Registry-Adapter 0.7.0 und erkannter Spielbuildwechsel

Stand 22.09.2026, Entwicklungsnachweis für Modul 0.7.0. Kein installierbarer
Reparaturmod. Nachtrag: App v0.5.9 unterstützt inzwischen die Tabellen des neuen
Builds; die unten beschriebenen nativen Grenzen bleiben bestehen.
[Tabellenfreigabe](../../docs/BUILD_SUPPORT.md).
Weiterentwicklung: [gemeinsame native Registry-/Referenzprobe 0.8.0](REGISTRY_NATIVE_INTEGRATION.md).
Die folgenden Ergebnisse beschreiben den früheren Stand 0.7.0.

## Implementierter Adapter

`crimson_repair_registry.lib` verbindet einen vom Host bereitgestellten Manager
und dessen native Lookup-/PaPtr-Freigabemethode mit `reference::Pair`.
Der Lookup erhält die vollständige Charakterkennung und den 32-Byte-Ausgabebeleg.
Der Adapter verwendet keine vorab gelesene Actoradresse als Erwerbsquelle.

Jede Quelle behält ihre eigene Methodenbindung. Dafür erhält der Release-Callback
jetzt denselben Kontext wie der Acquire-Callback. Es gibt keine globale
Freigabefunktion, die ein zweiter Host/Manager ersetzen könnte. Ungültige
Manager-/Methodenbindungen und Kennung 0 liefern keine verwendbare Quelle.
Der Quellenadapter ist nicht kopierbar oder verschiebbar und muss seine daraus
erworbenen Referenzen überleben.

Der native Lookup ist für die Registry-Sperre und den Erwerb vor Freigabe dieser
Sperre zuständig. Diese Aufrufe können warten; sie gehören ausschließlich auf
den noch zu belegenden Engine-Dispatch-Thread. Das ist eine andere Voraussetzung
als der nicht wartende Inventarsperrversuch aus 0.5.0.

## Aktuelle Prüfung

Fünf CTest-Suiten, MSVC Release `/W4 /WX`, 55 Registry-Adapterbedingungen,
249 Leser-/Referenzbedingungen, 25 Sperrgruppenbedingungen und 1.481
Aktionsbedingungen bestanden. Die neuen Tests verwenden ausdrücklich künstliche
Registry-Callbacks. Geprüft sind vollständige Kennung, Fehler beider Quellen,
gleiche Actoradresse in beiden Rollen, fehlende Methoden/Manager, kein Aufruf bei
ungültigen Bindungen, genau eine Freigabe je Ergebnis und umgekehrte Reihenfolge.
Zwei unterschiedliche Freigabefunktionen belegen die Bindung je Quelle.

## Wesentliche neue Grenze: Spielupdate

Während dieses Arbeitsschritts wurde die installierte EXE als neue Version erkannt:

| Merkmal | Bisher nachgewiesen | Jetzt installiert |
|---|---|---|
| Steam-Build | 25381195 | 25455892 |
| EXE-Version | 1.0.0.2944 | 1.0.0.2949 |
| EXE-Bytes | 397.242.776 | 385.363.864 |
| SHA-256 | `6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7` | `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a` |

24 der 38 bislang erfassten EXE-/Metadatendateien unterscheiden sich vom alten
Beobachtungsstand. Der neue [Beobachtungsbericht](../../docs/builds/steam-25455892.observed.json)
war zunächst keine Schemafreigabe. Die spätere Tabellenfreigabe in v0.5.9 ist
getrennt dokumentiert; weiterhin keine Vanilla-Zertifizierung, native Freigabe
oder vollständige Installationsinventur. Der ursprüngliche Build bleibt erhalten.

Der bisherige native Testhost lehnt die neue EXE vor Funktionsausführung ab.
Beim Entwicklungsstand v0.5.8 meldete die lesende CLI-Builddiagnose `metadata_matches=false` und
`read_schema_supported=false`. Der zusätzliche CLI-Tabellenaufruf scheitert
vorher an einer fehlenden Projektprobensicherung; dieser Aufruf zählt deshalb
nicht als Nachweis der passenden Build-Fehlermeldung. Die Fingerprint-Diagnose
liefert den eigenständigen Versionsnachweis.

Die 203 nativen Aufrufe aus 0.6.0 bleiben ein historischer Nachweis für 1.0.0.2944.
Für 0.7.0 wird **kein neuer nativer Erfolgsnachweis** behauptet. Die zunächst
gefundenen Client-Registry-Funktionen sind Forschungsbefunde, noch keine freigegebene
Methodenbindung für den neuen Build. Adressen, Unwind-Grenzen, Abhängigkeiten und
Tabellen müssen gemeinsam neu geprüft werden; bloß den EXE-Hash zu ersetzen
wäre falsch.

## Noch fehlend

- Vollständige Unterstützung und isolierter nativer Nachweis des neuen Builds.
- Aktueller Spieler/Manager, gesicherte Managerlebensdauer und belegte
  Engine-Thread-/TLS-Bindung. Der Adapter löst diese Hostvoraussetzungen nicht selbst.
- Zusammengeführter Nachweis echter Client-/Server-Registry-Lookups samt
  Referenzerwerb und späterer Freigabe.
- Vollständige Reparaturtransaktion, Ereigniszuordnung, Eingabe, Loader und B0-Einbau.

Keine Spieldateien oder Saves geschrieben; kein Spielprozesszugriff.
Manuelle Tests sind weiter zurückgestellt. Die nutzerlesbare Abnahmeliste steht
in [TESTCHECKLISTE.md](../../TESTCHECKLISTE.md).

Nachweise: `.local/repair-runtime-v7-build-test.log`,
`.local/repair-runtime-v7-build-refusals.json`,
`.local/repair-runtime-v7-current-fingerprint.json`,
`.local/repair-runtime-v7-validation.json`.
