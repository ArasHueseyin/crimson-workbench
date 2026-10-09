# Zusatzsockel für drei Gegenstände – Build 1.0.0.2976

## Korrektur und allgemeine Auswahl

Version 2 unterstützt bis zu 256 individuelle Ausrüstungsinstanzen; bestehende
Version 1 mit den drei unten genannten UIDs bleibt lesbar. Workbench liest die
eigenen Instanzen aus Inventar/Ausrüstung/registrierten Charakteren eines
ausgewählten Saves, prüft einzeln gespeicherte Items, fünf Paddingeinträge,
0–5 offene Basissockel und leeres gesperrtes Padding. Ein Gegenstand wird einmal
um zehn zusätzliche Plätze erweitert. Spielstände und ItemInfo bleiben dabei
unverändert. Die neue Konfiguration ist nicht an die drei alten UIDs gebunden.

Die erste Implementierung übersah Runtime→InitData RVA `0x240cac0`. Diese
Routine hat fünf inline Sechs-Byte-Einträge bei `+0x40`; der sechste überschreibt
den Zähler bei `+0x5e`. Deshalb entstand im Client Prefix 255 und eine beschädigte
Anzeige. Auch dieser Konverter erhält nun eine geliehene normalisierte Item-
Kopie. Nachfolgende Client-Konstruktion lädt dieselben zehn Extras. Die Mod
prüft den LiveItems-Peer und wartet auf dessen aktivierten Taskhook, bevor sie
den von LiveItems vorab attestierten Konverter umleitet. Alle drei Hooks werden
gemeinsam aktiviert. Der vorhandene Erzeugungsweg behält seine Prüfungen.

Isoliert geprüft: die komplette Originalroutine, alle Basissockel 0–5,
leere/bestückte Extras und abweichende Prefixwerte 0/255 mit Speicher-Canaries,
unveränderten Mengen/Flags/Verfeinerung/Haltbarkeit und anschließendem Client-
Erweiterungsschritt. Der erneute echte Spieltest bleibt erforderlich.

Zehn zusätzliche native Sockel: Frostfluch-Plattenpanzer UID 1001416 (insgesamt 10), Schattenhelm UID 1003059 (11), Elektro-Mecha-Langschwert UID 1003664 (15). Kein globales ItemInfo-Patching; keine Änderung an bestehenden Steinen oder Verfeinerung.

Die ASI erweitert den engine-eigenen Sechs-Byte-Sockelvektor nach der originalen Konstruktion. Der nachfolgende Save-Ladepfad benutzt die bereits dynamische native Kopierroutine. Originale fünf Sockel-Paddingeinträge und geöffnete Basissockel bleiben im Spielstand. Beim Speichern erhält die originale native Schreibroutine eine geliehene vollständige Item-Kopie mit unverändertem restlichem Inhalt und fünf Basiseinträgen. Der laufende Gegenstand bleibt unverändert. Zehn Zusatzsockel je Gegenstand werden getrennt in `bin64/CrimsonExtraSockets.dat` gespeichert (SHA-256, genaue UIDs/Keys, zehn Einträge, validierte Abyss-Steine). Dadurch bleibt der normale Save ohne diese Mod lesbar.

Workbench → **Zusatzsockel** erlaubt Auswahl und direkte Bestückung. Die normale Hexen-/Tooltip-Oberfläche hat weiterhin höchstens fünf Zeilen; der Panzer ist normalerweise nicht sockelbar. Änderungen in Workbench erfordern ein geschlossenes Spiel, aktuelle Dateirevision und erzeugen eine Sicherung. Speichermeldungen sind Diagnose; tatsächliche Wirkung und Laden/Speichern sind im Spiel zu prüfen.

Build: CMake, Visual Studio 2022 x64, statische MSVC-Runtime, unverändertes MinHook 1.3.4 über `-DMINHOOK_ROOT=…`. Pins und erlaubte Stein-Keys sind für die SHA-geprüfte EXE 2976 generiert. Fremde EXEs, abweichende Funktionen und beschädigte Konfiguration deaktivieren die Mod vor den Hooks. ASI-Hash in `cd-core/src/extra_sockets.rs` muss beim Release der tatsächlichen Ausgabe entsprechen.

Private native Originalroutine-/Policy-/Codec-/Loader-Prüfungen und Installationsnachweise: `.local/extra-sockets-plus10-20261004/`. Frühere reine Zehn-Eintrag-Save-Proben dürfen nicht installiert werden.
