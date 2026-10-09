# Quellen und Lizenzen

Stand: 19.09.2026, Phase 2. Die gezielte native MIT-Portierung aus crimson-rs liegt
unter [`vendor/crimson-format`](vendor/crimson-format/), mit vollständiger
[MIT-Lizenz](vendor/crimson-format/LICENSE), gepinntem Ursprungscommit und
[Änderungs-/Dateiverzeichnis](docs/FORMAT_PORT.md). Sie wird als lokale Rust-Crate
eingebunden. Es gibt keine Python-, CDUMM- oder Save-Writer-Laufzeitabhängigkeit.
Andere Referenzquellen bleiben in `.research/` und sind nicht übernommen.
Lizenzangaben wurden an Dateien geprüft, nicht nur an GitHub-Labels oder READMEs.

Die Desktopoberfläche verwendet reguläre Abhängigkeiten statt kopierter
Community-Editoren: Tauri 2, React, TypeScript, Vite, Zustand, TanStack Table/Virtual,
Lucide, Vitest und Playwright. Exakte Versionen/Lizenzen stehen in
`app/package-lock.json` und `Cargo.lock`; DDS/PNG-Decodierung nutzt Rust `image`
ohne dessen vollständiges Standardfeaturepaket. Das C-Appsymbol ist eine eigene
SVG-Grafik; die PNG/ICO-Varianten wurden mit Tauri daraus generiert. Itemicons
werden ausschließlich lokal aus der Nutzerinstallation gelesen und nicht gebündelt.

Implementierungsreferenzen: [Tauri-Kommandos](https://v2.tauri.app/develop/calling-rust/),
[Tauri-Capabilities](https://v2.tauri.app/security/capabilities/),
[Vite-Konfiguration](https://v2.tauri.app/start/frontend/vite/),
[TanStack-Virtualisierung](https://tanstack.com/table/latest/docs/framework/react/guide/virtualization),
[DDS-Header](https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header).
Stringinfo-Formatwissen stammt aus `src/string_info/mod.rs` der unten gepinnten
MIT-crimson-rs-Revision; `cd-core/src/icons.rs` implementiert den begrenzten Parser
selbst und prüft den aktuellen Body-/Header-Roundtrip.

| Quelle / Revision | Festgestellte Lizenz und vorgesehener Umgang |
|---|---|
| [bbfox0703/crimson-rs](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0), `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0` (18.09.2026) | [MIT](https://github.com/bbfox0703/crimson-rs/blob/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0/LICENSE), Copyright Tommy Tran. Archiv-, Save-, PALOC- und Tabellenreferenz; lokal als Referenz gebaut/getestet. Bei Übernahme Lizenztext/Notices mitliefern. |
| [lazorr410/crimson-desert-unpacker](https://github.com/lazorr410/crimson-desert-unpacker/tree/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e), `f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e` (23.03.2026) | [MIT](https://github.com/lazorr410/crimson-desert-unpacker/blob/f0b6fef67a3ecf5d00d90c7aab4bbde7403a9d7e/LICENSE), Copyright lazorr410. PAZ/PAMT, Crypto, Kompression und Repacking. Lizenzkonforme selektive Übernahme grundsätzlich möglich; noch nicht erfolgt. |
| [NattKh/CrimsonDesertModdingTools](https://github.com/NattKh/CrimsonDesertModdingTools/tree/1a9c3f10440713ee1b8f0bb88f66091376b433b5), `1a9c3f10440713ee1b8f0bb88f66091376b433b5` (11.04.2026) | README nennt MIT, aber im geprüften Git-Baum fehlt eine LICENSE-Datei. Gemäß Spec nur Formatdokumentation, kein Kopieren von Parsern oder Schemadateien. |
| [NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/tree/96e7f78fcb00d6cb5615171a7f359f0c2de5b114), `96e7f78fcb00d6cb5615171a7f359f0c2de5b114` (06.09.2026, Autorenzeitzone) | [MPL-2.0](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS/blob/96e7f78fcb00d6cb5615171a7f359f0c2de5b114/LICENSE.txt), Copyright RicePaddySoftware. Hier Format-/Fehlermodusreferenz. Übernommene/angepasste MPL-Dateien müssten ihre Lizenz-/Quellcodepflichten erfüllen. |
| [GildyBoye/CrimsonDesertShopEditor](https://github.com/GildyBoye/CrimsonDesertShopEditor/tree/c92d9153055c7a371c2c0cca51fc49d7c1c23d53), `c92d9153055c7a371c2c0cca51fc49d7c1c23d53` (16.08.2026) | Keine LICENSE-Datei gefunden; [TermsOfService.md](https://github.com/GildyBoye/CrimsonDesertShopEditor/blob/c92d9153055c7a371c2c0cca51fc49d7c1c23d53/TermsOfService.md) enthält Mod-Attributionsbedingungen. Nur Format-/Verhaltensreferenz, keine Codeübernahme. |
| [faisalkindi/CrimsonDesert-UltimateModsManager](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/tree/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6), `b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6` (13.09.2026) | Root-[MIT](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager/blob/b5de424f39ac95e2ef322dd4ed1953ecdd0c5ee6/LICENSE), Copyright faisalkindi, aber einzelne Parser/Schema- und DDS-Teile tragen MPL-2.0-Hinweise. Keine pauschale MIT-Freigabe für jede Datei. Nur Referenz für B0, keine Managerabhängigkeit. |

Die übrigen Zeilen der obigen Rechercheentscheidungen beschreiben ihren
Phase-0-Stand; tatsächlich portiert wurde ausschließlich die in `FORMAT_PORT.md`
aufgelistete MIT-Untermenge von crimson-rs. Die Versionen der Rust-Abhängigkeiten
werden durch `Cargo.lock` festgehalten. SQLite wird über rusqlite mit der
`bundled`-Option eingebunden ([rusqlite-Dokumentation](https://docs.rs/rusqlite/0.40.2/rusqlite/)).

Ergänzung 03.10.2026: Der Save-Codec aus derselben MIT-crimson-rs-Revision wurde
für gespeicherte Wissensanzeigen und die separate Stallregistrierung portiert.
Copyright und MIT-Text werden zusätzlich in `vendor/crimson-format/LICENSE-save`
mitgeführt. Authentifizierungs- und Größenprüfungen wurden verschärft; die
Transaktion, Vorlagenprüfung und Erhaltung vorhandener Savebereiche sind eigene
Core-Implementierungen. Details: [FORMAT_PORT](docs/FORMAT_PORT.md) und
[Reittiere](docs/REITTIERE.md).

## Zuschreibung bei später erzeugten Mods

GildyBoyes Bedingungen fordern nachvollziehbare Urheberschaft und eine offene
Beschreibung der Änderungen; bearbeitete fremde Mods müssen als solche kenntlich
bleiben und ihre ursprünglichen Credits behalten. Die Spec erweitert diese
Anforderung auf Mods, die aus Erkenntnissen dieses Editors entstehen.

Workbench muss deshalb später im Änderungsmanifest und in mitgegebenen
Modbeschreibungen die benutzten Quellen ausweisen, insbesondere GildyBoye für
entsprechende Shop-/Drop-/Trust-/Skill-Erkenntnisse. Änderungen einschließlich
relevanter Werte werden beschrieben; fremde Mods werden nicht als eigene Arbeit
ausgegeben. Dies ersetzt keine Erlaubnis, den Editorcode zu kopieren.

## Spiel und Referenzmods

Crimson Desert und seine Inhalte gehören Pearl Abyss bzw. den jeweiligen
Rechteinhabern. Dieses Projekt ist unabhängig. Spielarchive, Tabellen, Texturen,
Spielstände und Inhalte fremder Nexus-Mods werden nicht mitgeliefert. Der lokale
Ordner `reference/mods/` enthielt beim Recherchebeginn keine Referenzmods.

Vor jeder tatsächlichen Codeübernahme werden die Lizenz der konkreten Datei,
deren Herkunft und die erforderlichen Notices erneut geprüft. Die hier getroffene
Implementierungsentscheidung ist: eigene Core-Fassaden aus belegten
Formaterkenntnissen plus die dokumentierte MIT-Teilportierung von crimson-rs.

## Phase 3: Rezeptrechner

Neue originale Reader/Writer in `crates/cd-core/src/crafting/formats.rs` und
Materialplaner in `planner.rs`. Formatkandidaten aus den oben gepinnten NattKh-
Unterlagen und MIT-crimson-rs; Dropset-Formatvergleich mit NattKhs MPL-Editor.
Keine Codeübernahme aus diesen Python-Dateien. Aktuelle Bytebreiten wurden an
der lokalen Installation vermessen und vollständig roundtrip-geprüft; bekannte
Abweichungen und Grenzen stehen in [docs/CRAFTING.md](docs/CRAFTING.md).

## Phase 4: Modvorschau und Transaktionsprobe

Originale Rust-Implementierungen in `mods/`, `apply/` und
`vendor/crimson-format/src/overlay.rs`. Archivformatwissen und vorhandene
MIT-Kryptoportierung aus crimson-rs, nach erneutem Lesen der MIT-Lizenz.
Store-/Friendly-Formathinweise aus den oben gepinnten Referenzen, keine Übernahme
von CDUMM-, NattKh- oder Gildy-Quellcode. Die proprietär gekennzeichnete
`dmm_parser`-Implementierung wird nicht eingebunden oder nachgebildet; beim
Lizenzhinweis wurde die weitere Untersuchung dieses Unterpakets beendet.
Exporte enthalten ausdrücklich GildyBoye mit Quelllink und benennen Workbench
als eigenen Erzeuger. Alle Record-/Layoutbelege sind in [MODS.md](docs/MODS.md)
von noch unbestätigten Feldbedeutungen getrennt.

Für v0.4.2 wurden Refresh-/Stock-Indizes und unabhängige Dropchancen zusätzlich
durch eigene statische Analyse der lokalen EXE und der gebundenen Tabellen
ermittelt. Es wurden keine Spielbytes oder Disassembly in das Repository übernommen.
Der SteamKit-Reader diente nur zum Vergleich der Manifestabschnittsstruktur,
der Daily-Refresh-Mod nur zur Plausibilisierung der Anzahl. Quellen, Adressen
und Grenzen: [B1/B2-Feldnachweise](docs/research/PHASE4_FIELDS.md).

## Steam-Depotinventur (v0.4.4)

Eigener begrenzter Rust-Parser, nur Formatwissen aus
[SteamKit DepotManifest](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs)
und der [Protobuf-Definition](https://github.com/SteamDatabase/Protobufs/blob/master/steam/content_manifest.proto).
Kein SteamKit-/DepotDownloader-Code übernommen, keine neue Laufzeitabhängigkeit,
keine Authentifizierung oder Downloads. Siehe [Installationsprüfung](docs/INSTALLATION_CHECK.md).

## Vollständiger Inhaltsvergleich (v0.4.5)

[RustCrypto `sha1` 0.10.6](https://docs.rs/sha1/0.10.6/sha1/) als reguläre Cargo-
Abhängigkeit, `MIT OR Apache-2.0` laut geprüftem Paketmanifest. SHA-1 ausschließlich
für den vorhandenen Steam-Hashvergleich; SHA-256 weiterhin über `sha2`.
Keine kryptografische Eigenimplementierung und keine Herkunftszertifizierung.
Der eigene Reader-/Worker-Code ist in [CONTENT_AUDIT.md](docs/CONTENT_AUDIT.md) beschrieben.

## Phase 5 (v0.5.0)

Eigene begrenzte Reader für bestätigte Welt-/Skill-/Equip-Felder und eigene
In-Memory-Fassade über den vorhandenen MIT-Item-Port. Formatwissen aus den
oben gepinnten Referenzen und statische Prüfung der lokalen EXE-Serialisierung;
kein übernommener NattKh-Parsercode. Der Item-Port behält seine MIT-Notices.
Keine neuen Bibliotheken, Spielinhalte oder Disassemblierungen im Repository.
[Feldnachweise und Einschränkungen](docs/research/ADVANCED_TABLES.md).

v0.5.1 ergänzt eigene Enchant-Zeilenkopien und schreibgeschützte Skill-Buffinspektion.
Die gemeinsame Buffstruktur wurde mit der gepinnten MIT-Quelle und dem lokalen
Serializer verglichen; variable Payloads wurden am lokalen EXE-Reader geprüft.
Keine Übernahme des heuristischen upstream-Tail-Suchalgorithmus. RTTI-Namen und
abgeleitete Formatbreiten sind Strukturmetadaten; private Werte/Disassembly
bleiben ausschließlich in ignorierten Forschungsdateien.

v0.5.3 ergänzt einen eigenen vollständigen Reader für die 108 Wiederbesetzungs-
regeln und eine Inventar-Prüfgrenze aus statisch gelesenen EXE-Standardwerten.
Keine neue Fremdimplementierung, keine EXE- oder Runtime-Patches; Nachweise in
`docs/research/ADVANCED_TABLES.md`.

v0.5.2 ergänzt eigene feste Skill-Zahlenänderungen, zwei Stage-Reset-Reader
und eine einzelne Stadtflug-Bedingung. Den konkreten Hinweis auf Condition
1011130 liefert [Flight Freedom](https://github.com/shin2344234/flight-freedom/blob/d61a9c37fc2fc0970d155329ddbf143038f0910d/mod/src/game/signatures.h),
Commit `d61a9c37fc2fc0970d155329ddbf143038f0910d`, MIT. Keine Plugin-, Hook-
oder Patch-Codeübernahme; die Datenserialisierung und boolesche Ersatzbedingung
wurden separat an der lokalen EXE-Datei verifiziert. Quelle und Belegadressen
stehen im oben verlinkten Feldnachweis.


v0.5.4 ergänzt eine eigene, benannte Aufteilung der Summon-/AddSubLevel-Payloads
aus statischer Reader-/Serializeranalyse der gepinnten EXE-Datei. Nur
Strukturmetadaten und eigene Reader im Projekt; keine neue Fremdimplementierung,
keine Spielwerte oder Disassemblierungen im Repository, kein Prozesszugriff.


v0.5.5 entfernt einen eigenen, bislang ungenutzten Reparatur-Nullsetzpfad nach
statischem Nachweis ungeschützter Divisionen im lokalen Engine-Code. Keine
Fremdimplementierung oder Laufzeitänderung; Quellen bleiben die gepinnte EXE
und die bereits portierte Itemstruktur. Neue Regressionstests und Vorlagen-
Korrektur sind eigene Implementierungen.


v0.5.6 ergänzt eigene vollständige Skill-Header-/Suffix-Inspektion anhand der
statisch gelesenen gepinnten EXE und entfernt die bisherige Suche nach gleichen
Kennungsbytes. Keine neue Fremdimplementierung oder Quelldatenübernahme;
Adressen, Feldnamen und Grenzen stehen in `docs/research/ADVANCED_TABLES.md`.


v0.5.7 ergänzt eine eigene Sentinel-Änderung für Item-Haltbarkeit und einen
eigenen Inventar-Move-Reader. Den Hinweis auf 65.535 liefert die Dokumentation
von [NattKh/RicePaddySoftware](https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS)
(`FIELD_JSON_V3_SPEC.md`, `ITEMBUFFS_FEATURE_AUDIT.md`, MPL-2.0). Kein MPL-Code
übernommen. Semantik und Bytebreiten wurden unabhängig an der gepinnten lokalen
EXE geprüft; implementiert über den bestehenden MIT-Item-Port. Fremde Mods
wurden weder übernommen noch ausgeliefert.


v0.5.8 ergänzt einen eigenen BuffInfo-Reader und zusätzliche Eigenkosten.
Der [DMM-Autoren-Changelog](https://www.nexusmods.com/crimsondesert/mods/633)
lieferte den Hinweis auf zuvor fehlende Ausrüstungskosten. Keine Abhängigkeit,
kein Download/Einbau von DMM oder Moddateien; Format und Felder wurden separat
an der gepinnten lokalen EXE und den Tabellen nachgewiesen. Bereits bekannte
Referenzschemas dienten ausschließlich als Formatvergleich; kein Fremdcode
kopiert. Spielinhalte und Disassembly bleiben in ignorierten lokalen Dateien.


Reparatur-Laufzeitprototyp 0.1.0: eigene C++-Implementierung und eigener nativer
Testhost; keine Übernahme von ASI-Loader-, Hook- oder Modcode. Die Schnittstellen
und Rechenpfade wurden aus der gepinnten lokalen EXE abgeleitet. Originalcode
wird nur beim privaten Test dynamisch gelesen und nicht eingebettet/ausgeliefert.
Win32-Nutzung nach [Microsoft VirtualProtect](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect)
und [DLL Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).
Der optionale Lese-Audit nutzt lokal installiertes pefile 2024.8.26 (MIT) und
Capstone 5.0.9 (BSD-Lizenz); kein Paketcode wurde kopiert oder in die DLL eingebaut.

Eigene Reparaturaktion 0.2.0: neue eigene C++-Implementierung, Warteschlange und
Tests. [Trinity](https://github.com/XeTrinityz/Trinity/tree/70c9a00dd6e10b2081d706a837756844c11f5c2b),
Commit `70c9a00dd6e10b2081d706a837756844c11f5c2b`, MIT, Copyright 2026 XeTrinityz,
diente nach Lizenzprüfung als Referenz für getrennte Client-/Server-Itemkopien.
Keine Code-, Hook-, Loader- oder Binärübernahme. Die älteren Equipment-Offsets
passen nicht zum lokalen Build und wurden nicht übernommen; die verwendeten
Item-/Sockelfelder wurden aus der lokalen EXE unabhängig verifiziert und durch
isolierte native Aufrufe geprüft. Forschungscheckout nur unter `.research/`.

Lesender Reparaturadapter 0.3.0: eigene Implementierung für die aktuelle
Client-/Server-Registry, Inventar-/Equipmentstrukturen und begrenzte lokale
Lesezugriffe. Dieselbe MIT-Referenz Trinity gab Struktur-/Spielerhinweise;
keine Implementierung übernommen. Aktuelle Globals, Slotgrenzen und Strides
wurden an der gepinnten EXE neu geprüft. Inventar-/Registry-Funktionskopien
werden ausschließlich im eigenen Testhost aus der lokalen EXE gelesen; keine
Originalbytes werden ausgeliefert. Native Lock-/Actor-Leases sind eigene
Test-Stubs und werden nicht als nachgewiesene Engine-Synchronisierung ausgegeben.

Reparatursteuerung 0.4.0: eigene asynchrone Abschlusssteuerung und Nachherwert-
Prüfung. Server-/Client-Ereignispfade und destruktive Sockelfunktion unabhängig
an derselben gepinnten EXE untersucht. Der private Testhost ersetzt ausschließlich
einen TLS-Lesezugriff der Client-Kopie durch künstliche Daten; UI, Allocator und
virtuelle Locks sind eigene Stubs. Keine Originalcode-Auslieferung, kein neuer
Fremdcode und keine Übernahme eines Loaders oder Hooks.

Besitzer-Sperren 0.5.0: eigene Sperrgruppe und Erweiterung des lesenden Adapters.
WindowsRWLock-Methoden, Imports, Actor-Zugriff und TLS-Zweige unabhängig an der
gepinnten EXE geprüft; Trinity blieb dieselbe MIT-Strukturreferenz ohne
Codeübernahme. Native Proben rufen echte Windows-SRW-Funktionen auf eigenen
Lock-Objekten auf. Ein TLS-Zugriff der privaten Try-Kopie wird umgeleitet.
Keine Originalbytes ausgeliefert; keine neue Fremdcode-Abhängigkeit.

Referenzverwaltung 0.6.0: eigene PaPtr-Eigentümer und Integration in den Leser.
Native Erwerbs-/Freigabewege samt konkretem Actor-Override und Methodentabellen
unabhängig an derselben gepinnten EXE geprüft. Threadmap, eingebettete Sperren,
Bereinigung und Zerstörung sind eigene Testabhängigkeiten. Drei TLS-Lesezugriffe
werden ausschließlich in privaten Funktionskopien umgeleitet. Keine neue
Fremdcode-Übernahme, Originalcode-Auslieferung oder Modmanager-Abhängigkeit.

Registry-Quellenadapter 0.7.0: eigene C++-Implementierung und synthetische Tests.
Die installierte EXE 1.0.0.2949 wurde ausschließlich lesend erfasst. Bisherige
native Nachweise für 1.0.0.2944 werden nicht auf den neuen Build übertragen.
Keine neue Fremdcode-Abhängigkeit und keine ausgelieferten Spielbytes.

Registry-/Referenzintegration 0.8.0: eigener, getrennt gepinnter Testhost für
EXE 1.0.0.2949. Die 15 Funktionsbereiche stammen ausschließlich aus der lokalen
Installation und werden nur für den privaten Test dynamisch gelesen; keine
Originalcode-Auslieferung. Echte Windows-SRW-Imports arbeiten an eigenen
Testobjekten. Vier TLS-Lesezugriffe sind auf private Daten umgeleitet; Threadmap
und finale Objektbereinigung sind eigene Fixture-Abhängigkeiten. Keine neue
Fremdcode-Übernahme oder Loader-Abhängigkeit.

Geschützte Erfassung 0.9.0: eigener Leserumbau, eigene Regressionstests und
Erweiterung der privaten Probe um den originalen 2949-WindowsRWLock-Try-Pfad.
Insgesamt 16 dynamisch gelesene Funktionsbereiche; keine ausgelieferten Spielbytes.
Das Inventarlayout der kombinierten Probe bleibt ein eigenes Legacy-Fixture.
Trinitys Bewegungstreiber wurde als Strukturhinweis gelesen, nicht übernommen
oder als nachgewiesener Spieler-/Engine-Dispatch freigegeben. Keine neue
Fremdcode-Abhängigkeit.

Inventarzugriffe 0.10.0: eigene neue Buildzuordnung und private Vergleichsproben
für Inventar, Besitzer, aktuelle Auswahl, Equipment und Haltbarkeitsfelder.
24 ausgewählte Codebereiche werden ausschließlich aus der lokalen EXE gelesen;
keine Spielbytes ausgeliefert. Sechs private TLS-Umleitungen. Tabellenresolver
und leere temporäre Items sind eigene Testabhängigkeiten. Kein neuer Fremdcode,
Loader oder Zugriff auf den laufenden Spielprozess.

Ausrüstungsereignisse 0.11.0: eigene Vorbereitung aus Reparaturplänen und eigene
private Integrationsfixtures. Vier weitere Originalfunktionen werden nur aus
der lokalen EXE gelesen, keine Spielbytes ausgeliefert. Server-Notifier und
Client-Ack besitzen jeweils eine private TLS-Umleitung. Effekt-/Transport-/UI-
Empfänger, Kind-Liste, Uhr und Speicherfreigabe sind eigene Testabhängigkeiten.
Keine neue Fremdcode-, Loader- oder Modmanager-Abhängigkeit.

Itemkopien 0.12.0: eigene Besitzverwaltung und Tests. 17 zusätzliche Funktionen
werden ausschließlich aus der lokalen EXE in den privaten Testhost gelesen,
einschließlich Konstruktion, Zuweisung, Destruktion und Listenhelfern. Eigener
Windows-Heap mit Schutzmarkierungen ersetzt nur die Allokationsschnittstellen.
16 zusätzliche TLS-Lesestellen privat umgeleitet; ein byteidentischer Unwind-Alias
für die eigene Kette normalisiert. Keine Originalcode-Auslieferung oder neue
Fremdcode-Abhängigkeit, keine Änderung an Spielprozess oder Installation.

Slot-Markierung 0.13.0: sechs zusätzliche Originalfunktionen ausschließlich aus
der lokalen EXE im privaten Testhost ausgeführt, keine Spielbytes ausgeliefert.
Zwei weitere private TLS-Umleitungen, ein byteidentisch geprüfter Unwind-Alias
und eigener ausgerichteter Testheap. Die gehaltene Erfassung und Testfixtures
sind eigene Implementierungen; keine neue Fremdcode- oder Loader-Abhängigkeit.

Feldschreiber 0.14.0: eigene C++-Implementierung für ganze Reparaturbatches und
eigenen lokalen Speicherzugriff, keine weitere Originalfunktion hinzugefügt.
Native Referenz-/Sperrfunktionen werden nur für private Integrationsfixtures
verwendet. Keine neue Fremdcode-/Loader-Abhängigkeit, keine Originalcode-Auslieferung
und keine Schreibausführung im Spielprozess.

Transaktionsadapter 0.15.0: eigene C++-Verbindung von Planung, nativen Kopien,
Feldschreiber und Meldungsadaptern mit eigenen Fehler-/Lebensdauertests. Keine
zusätzlichen Originalfunktionen oder Fremdcode-Abhängigkeiten. Die vorhandenen
gepinnten Funktionskopien laufen nur auf privaten Testobjekten. Effekt-/Transport-/
UI-Empfänger bleiben eigene Testcallbacks; keine Spielbytes ausgeliefert.

Slot-Verarbeitung 0.16.0: eigener Batchabschluss und private Persistenzfixtures.
Vier zusätzliche Originalfunktionen (Verbraucher, Iterator, Clear, Destruktor)
werden ausschließlich aus der lokalen EXE in den privaten Testhost gelesen;
keine Originalcode-Auslieferung. Sechs weitere private TLS-Umleitungen und zehn
separat geprüfte Unwindfragmente der Bereinigungsfunktionen. Persistenzempfänger
und Fehlerdiagnostik sind eigene Testcallbacks. Keine neue Fremdcode-/Loader-
Abhängigkeit, keine Spielprozess-, Installations- oder Saveänderung.

Inventar-Client-Ack 0.17.0: eine zusätzliche Originalfunktion ausschließlich
aus der lokalen EXE im privaten Testhost ausgeführt; keine Spielbytes ausgeliefert.
Drei weitere private TLS-Stellen. Eigene Inventar-/UI-/Namensfixtures und
Integrationstests; Paketverteiler und Socket-SQL-Aufrufer nur statisch gelesen.
Keine neue Fremdcode- oder Loader-Abhängigkeit, keine Spielprozess-/Savezugriffe.

SQL-Auftragsweg 0.18.0: zwei zusätzliche vollständige Originalfunktionen aus
der lokalen EXE ausschließlich auf privaten Requests ausgeführt. Fünf verkettete
Unwindfragmente des Ausführungs-Shims geprüft; keine zusätzlichen TLS-Ersetzungen.
Uhr und Diagnose sowie Teile des kombinierten Verbrauchertests sind eigene
Testcallbacks. RTTI und alternative Ergebnisverarbeitung nur lesend untersucht.
Keine Originalcode-Auslieferung, neue Fremdcode-Abhängigkeit oder Spiel-/Savezugriffe.

Item-/Speicherobjekt-Konverter 0.19.0: 24 zusätzliche vollständige Funktionen
werden ausschließlich aus der lokalen, hashgeprüften EXE auf private Testobjekte
kopiert. 32 weitere pdata-Fragmente und 15 private TLS-Umleitungen, davon eine
RCX-Ladestelle. Eigene Katalog-, Reflexions-, Uhr-, Richtlinien- und Heap-Fixtures;
keine neue Fremdcode-/Loader-Abhängigkeit oder Originalcode-Auslieferung.
Keine Spielprozess-, Installations- oder Savezugriffe.

Speicherdispatcher 0.20.0: sechs zusätzliche Codebereiche ausschließlich aus
der lokalen, hashgeprüften EXE auf privaten Objekten ausgeführt. Sieben weitere
pdata-Fragmente, vier geprüfte Unwindketten mit bytegleichem Alias und eine
zusätzliche private TLS-Umleitung. Datei-, Plattform- und Diagnoseempfänger sind
eigene Testcallbacks; konkrete Dateihelfer und Timer wurden nur statisch gelesen.
Keine Originalcode-Auslieferung, neue Fremdcode-Abhängigkeit oder Spiel-/Savezugriffe.


Dateischreibhelfer 0.21.0: drei zusätzliche Originalfunktionen aus der lokal
hashgeprüften EXE auf privaten Testobjekten ausgeführt, drei pdata-Einträge und
vier private TLS-Stellen. Eigene Formatierungs-, Encoder-, Diagnose- und Datei-
Callbacks sowie eine Eintragsbrücke zum Dispatcher. Der Test ersetzt Flush/Close
nur im privaten Testadressraum; kein echter Dateihandle, keine Originalcode-
Auslieferung, neue Fremdcode-Abhängigkeit oder Spiel-/Savezugriffe.


Speicherpuffer 0.22.0: neun weitere Originalfunktionen aus der hashgeprüften
lokalen EXE auf private Objekte kopiert, sieben pdata-Einträge und drei weitere
private TLS-Ersetzungen. Die Stackprobe verwendet den eigenen Windows-Testthread.
Eigene Nachverarbeitungs-/Dateicallbacks; eigener begrenzter Testdecoder nach der
[LZ4-Blockbeschreibung](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md).
Kein LZ4-Quellcode übernommen, keine neue Laufzeitabhängigkeit oder Spielbytes
ausgeliefert. Keine Spielprozess-, Installationsschreib- oder Savezugriffe.

Inventar-Paketserializer 0.23.0: zwei zusätzliche Originalfunktionen ausschließlich
aus der lokalen, hashgeprüften EXE auf private Objekte kopiert. Zwei pdata-Einträge
und zwei weitere private TLS-Ersetzungen. Eigene Empfänger-/Vergleichs-/Stream-
und Diagnosecallbacks sowie begrenzter eigener Decoder zum bisherigen Client-Ack.
Keine zusätzliche Fremdcode-Abhängigkeit oder Auslieferung von Spielbytes.

Separater Offline-Inventarauftrag vom 23.09.2026: die bereits gepinnte MIT-Referenz
crimson-rs wurde für Lesen/Schemaeinfügung/Relokation und Save-Verpackung verwendet.
Eigene Zusatzprüfungen vergleichen vorhandene Feldbytes, Zeiger, UIDs, Standardwerte
und Integrität. Keine neue Save-Writer-Funktion in der veröffentlichten Workbench.

Ergänzung 04.10.2026, Basiseinträge für Reittiere: Eigene Konstruktion aus dem
Feldnamen-/Typ-Schema des lokalen privaten Saves, ohne kopierte Bytevorlagen.
NattKhs oben gepinnter Editor dient als Verhaltens-/Formatreferenz für einfache
Tierregistrierung; die [Analyse der Mount-Reparatur](https://github.com/Atomic-Flip/crimson-mount-repair/blob/main/docs/technical.md)
und [CrimsonAtomtics Mount-Katalog](https://github.com/bbfox0703/CrimsonAtomtic/blob/main/src/CrimsonAtomtic.Ui/Services/MountCatalog.cs)
dokumentieren Schemaänderungen und Besitz-/Beschwörungsfelder. Daraus folgt keine
Bestätigung des neu gebauten Initialisierungswegs im aktuellen Spiel. Kein Code,
keine fremden Savebytes und keine Wissens-/Questpakete dieser Quellen übernommen.
