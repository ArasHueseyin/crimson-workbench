# Reparatur 0.15.0 – vorbereitete Kopien und gemeinsamer Ablauf

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE-Hash:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**Entwicklungsmodul; die Reparatur ist weiterhin nicht in der App benutzbar.**
Workbench v0.5.9, Spielinstallation und Saves bleiben unverändert.

## Implementiert

`crimson_repair_transaction.lib` verbindet die vorhandene Aktionsschnittstelle
mit Erfassung, nativen Itemkopien, Feldschreiber und bereichsabhängigen Meldungen.
Der Host muss Build, Engine-Thread, TLS, Manager-/Kataloglebensdauer und Sitzung
bereits abgesichert haben. Der neue Adapter stellt diese Spielanbindung nicht her.

Der gemeinsame Ablauf lautet:

1. Beide Besitzerreferenzen und Sperren erwerben; daraus den Reparaturplan bilden.
2. Den gesamten Plan und die Meldungswege aller betroffenen Bereiche zulassen.
   Ein fehlender Inventarweg verwirft auch einen gemischten Auftrag vollständig.
3. Erneut lesen; Zusatzvektoren und optionale Itemdaten vor dem nativen Kopieren
   auf lesbare initialisierte Bereiche und ein gemeinsames Kopierbudget prüfen.
   Das Budget von 16 MiB ist eine Implementierungsgrenze, kein Engine-Maximum.
4. Für jedes betroffene Item beide nativen Nachherkopien vollständig vorbereiten.
   `item::Value::prepare_repair` prüft Vorherdaten, nutzt die native tiefe Zuweisung
   und ändert ausschließlich Haltbarkeitswerte der eigenen Kopie.
5. Erst danach den vollständigen Feldschritt ausführen und nachprüfen.
6. Alle Änderungen unter den Sperren markieren. Danach beide Sperren lösen,
   während Besitzerreferenzen und vorbereitete Kopien gültig bleiben.
7. Meldungen synchron übermitteln, Kopien zerstören, Referenzen zurückgeben.
   Das Ergebnis bleibt `pending`, bis neue Lesedaten und zugeordnete Rückmeldungen
   gemeinsam den Auftrag bestätigen.

Fehler vor einem Schreibversuch ergeben `rejected`; Fehler ab dem ersten
Schreibversuch ergeben `unknown`. Wiederholung oder automatische Rücknahme gibt
es nicht. Ein fremder Thread darf weder native Methoden noch Besitz verwenden.
Die Meldungsadapter dürfen keine geliehenen Zeiger über ihren Aufruf hinaus halten.

## Zwei korrigierte Randfälle

Die Zahl logischer Sockelplätze (`+0x70`) darf größer sein als gespeicherte Anzahl
(`+0x68`) und Allokationskapazität (`+0x6C`). Originale Kopier-/Zugriffsfunktionen
behandeln fehlende Datensätze getrennt. Leser, Plan und Schreiber verarbeiten jetzt
genau die gespeicherten Datensätze. Leere Vektoren mit verfügbaren Sockelplätzen
benötigen keinen Puffer; uninitialisierte Kapazitätsreste werden nicht gelesen.
Das ist anhand der bereits gepinnten Funktionen und neuer nativer Fälle geprüft.

Die Zusatzvektor-Strides in der Überschneidungsprüfung waren vertauscht:
`+0x78` besitzt 16-Byte-Datensätze, `+0xA8` besitzt 6-Byte-Datensätze. Korrigiert
und durch einen gezielten Fall abgesichert, dessen Überlappung erst im hinteren
Teil des 16-Byte-Datensatzes liegt. Frühere Modulberichte sind historische Stände.

## Ausgeführte Prüfungen

**Neun CTest-Suiten**, MSVC Release mit `/W4 /WX`, bestanden:

| Prüfung | Umfang |
|---|---|
| Aktion | 1.529 Bedingungen |
| Leser / Sperrgruppe / Registry | 519 / 25 / 55 Bedingungen |
| Itembesitzer | 29 Bedingungen |
| Feldschreiber | 35 Szenarien, 202 Bedingungen |
| Gemeinsamer Ablauf | 40 Szenarien, 247 Bedingungen |
| Native Gesamtprobe | 159 Szenarien, 2.023 gezählte Aufrufe, 119.953 Bedingungen |

Die neue Ablaufprüfung umfasst beide Bereiche und Zustandskopien, Einzelitem,
No-Wear, leere und teilweise gespeicherte Sockel, fehlende Meldungswege, spätere
Kopierfehler, fehlerhafte Kopierresultate, ungültige Zusatzdaten, Budgetüberschreitung,
Schreibabbrüche, Markierungs-/Meldungsfehler, Sitzungswechsel, fremden Thread,
Wiederholung und unabhängige Abschlussprüfung. Vor dem ersten Store müssen alle
Kopien vorliegen; Meldungen erfolgen erst nach dem Lösen beider Sperren.

Die **15 nativen Ausrüstungsereignisfälle** verwenden jetzt diesen gemeinsamen
Adapter und den tatsächlichen Feldschreiber. Der frühere manuelle Feldaufbau ist
entfernt. Originale Itemkopien, Referenzen, Windows-Sperren, Slot-Markierung,
Server-Notifier und Client-Ack laufen auf eigenen privaten Objekten zusammen.
Die Bestätigung erwirbt anschließend neue Besitzerreferenzen und liest erneut.
Absichtlich veränderte Slots bzw. widersprüchliche Zustandskopien werden dabei
bereits vom Leser verworfen und können keinen Auftrag erfolgreich abschließen.

**32 native Itemfälle** prüfen zusätzlich leere/teilweise gespeicherte Sockel und
vorbereitete Nachherkopien beider Zustandsseiten mit beiden TLS-Allokationswegen.
Keine Quelländerung oder fremde Freigabe; eigener Testheap vollständig aufgeräumt.
Es bleiben 51 gepinnte Codebereiche, 57 pdata-Fragmente und 26 private TLS-Stellen.
Die Bedingungszahl ist keine Fallzahl; sie enthält viele wiederholte Tabellenprüfungen.
Gegenüber 0.14.0 entfallen Vergleiche ungespeicherter Sockel und doppelte Prüfungen
aus dem ersetzten manuellen Ereignisaufbau. Die geringeren Summen sind kein
Nachweis weniger abgedeckter Fehlerfälle.

## Noch offen

Konkrete Inventarereignisse für mitgeführte Items, Verbraucher und native
Bereinigung der Slot-Meldungen, echte Effekt-/Transport-/UI-Verarbeitung sowie
eindeutige Zuordnung tatsächlicher Rückmeldungen zum Reparaturauftrag fehlen.
Diese Empfänger bleiben künstliche Testabhängigkeiten. Ebenso fehlen Spielhost,
Eingabe, Loader und B0-Einbau. Es gibt keinen bestätigten Live-Engine-Commit.

Die Tests führen ausschließlich Funktionskopien in einem eigenen Testprozess aus.
Kein Zugriff auf den laufenden Spielprozess, keine Spiel-/Save-Schreibzugriffe.
Unveränderte Rust-/UI-Teile wurden nicht erneut getestet. Manuelle Abnahme später,
insbesondere [R01–R13](../../TESTCHECKLISTE.md).

Nachweise: `.local/repair-runtime-v15-{build-test.log,native-result.json,validation.json}`.
[Adapter](src/transaction.cpp), [Schnittstelle](include/repair_transaction.h),
[Kopien](src/item.cpp), [Ablauftests](tests/transaction_tests.cpp),
[native Ausrüstungsprobe](tests/equipment_events_native.inl),
[native Itemproben](tests/item_native.inl).
Vorgänger: [Feldschreiber 0.14.0](FIELD_WRITER_2949.md).
