# Reparatur 0.13.0 – Slot-Markierung und gehaltene Erfassung

Historischer Entwicklungsstand; ergänzt durch den
[Feldschreiber 0.14.0](FIELD_WRITER_2949.md).

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949. EXE-Hash:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**Entwicklungsmodul; weiterhin keine installierbare Reparatur.** App v0.5.9 bleibt
unverändert. Die Reparaturoption ist weiterhin gesperrt.

## Implementierung

`reader::HeldCapture` hält beide nativen Besitzerreferenzen und exklusiven
Besitzersperren nach der Erfassung weiter. Planung und erneute Vorprüfung können
damit innerhalb derselben ununterbrochenen Sperrphase stattfinden. Die bisherigen
kurzen `capture_with_references`-Aufrufe verwenden denselben Weg, geben aber wie
bisher alles vor der Rückkehr frei.

`refresh` verlangt den vom Host unabhängig beobachteten aktuellen Frame. Andere
Weltgeneration, Katalogrevision, Imagebasis oder Build werden abgewiesen. Jeder
Refresh prüft erneut Auswahl, vollständige Kennung, lebendige Besitzer,
Sperrbindungen und das vollständige Itemabbild samt Kontrolllesen. Fehler löschen
alte Ergebnisse und schließen die Besitzphase. Es gibt keine zweite ungeschützte
Registry-Suche. `release_locks` beendet die Lesephase endgültig, hält die Referenzen
aber bis `close`/Destruktion für nachfolgende Benachrichtigungen. Wiedererwerb und
Nutzung auf einem anderen Thread sind ausgeschlossen.

Dies ist eine synchrone Erfassung mit gehaltenen Besitzern, **noch kein nativer
Schreiber**. Der Host muss Manager, Kontext und Katalog gültig halten,
Weltübergänge ausschließen und auf dem bestätigten Engine-Thread mit passendem TLS
laufen. Die Klasse erkennt solche Übergänge nicht selbst und darf keine Frames
überspannen. Ein vorbereiteter Plan wird vor einer Anwendung weiterhin vollständig
gegen die frischen Daten geprüft; getestet ist dies am vorhandenen privaten Backend.

## Neue native Nachweise

Die Server-Ausrüstungs-VTable `0x5b28f80`, Eintrag `+0x198`, verweist auf
`0x2ae0650`. Diese Funktion trägt den Slot in die Tabelle an Komponente `+0x1e8`
ein. Der vollständige Weg einschließlich Kollisionen, verschobener Buckets und
Neuaufbau der Tabelle läuft jetzt auf privaten Komponenten im eigenen Testprozess.

| Funktion | RVA | Vollständiger Codebereich |
|---|---|---|
| Slot-Markierung | `0x2ae0650` | `0x32` Bytes |
| Insert-Sprung | `0x20cd620` | `0x5` Bytes |
| Insert | `0xe1db910` | `0x20c` Bytes, drei pdata-Fragmente |
| Platzsuche / Kollisionsverschiebung | `0x20cd9e0` | `0x287` Bytes, drei pdata-Fragmente |
| Vergrößern / Rehash | `0x20cdc70` | `0x4be` Bytes |
| Bucket-Verknüpfung | `0x411bc0` | `0x77` Bytes |

Alle Bereiche besitzen eigene SHA-256-Werte im
[Manifest](tests/dirty_functions.inl). Die Platzsuche wird vollständig kopiert,
einschließlich ihrer beiden hinteren Fragmente. Der bytegleiche Unwind-Alias
`0x6106998` → `0x17147c54` wird nur für die exakt geprüfte Insert-Elternkette
normalisiert. Die ursprünglichen TLS-Lesestellen `0xe1dba5f` und `0x20cdcb6`
werden ausschließlich in den privaten Funktionskopien umgeleitet.

Der Testheap unterstützt jetzt die originale 32-Byte-Ausrichtung der Knoten.
Schutzmarkierungen, eindeutiger Besitz der Payloads, ausgeglichene Freigaben,
Slot-/Serienwerte sowie sämtliche Vorwärts-/Rückwärtsverkettungen werden geprüft.
Kopierpuffer und Slot-Payloads bleiben getrennte Allokationen.

## Ausgeführte Tests

- Sieben CTest-Suiten, MSVC Release mit `/W4 /WX` bestanden.
- 519 Leser-/Referenzbedingungen, davon neue Fälle für gehaltene Erfassung,
  Wiedererwerb, falschen Thread, Fehler-/Exception-Freigabe und Zustandswechsel.
  Ein veralteter Batch mit einem erst später geänderten Item verändert im
  privaten Backend auch die früheren Items nicht.
- 16 neue native Slot-Fälle: leer, Einzeleintrag, Duplikate, verschobene
  Kollisionsgruppen, 64 Einträge aufsteigend/absteigend, stark kollidierende und
  hochbitige Slotwerte, jeweils beide TLS-Allokationswege. Hochbitige Werte sind
  ein Hash-/ABI-Test, keine Freigabe solcher Ausrüstungsslots.
- Alle 15 vorhandenen Ereignisfälle markieren jetzt den geänderten Slot unter
  beiden Besitzersperren. Die originale Server-Benachrichtigung läuft danach auf
  derselben privaten Komponente mit der nativen Nachherkopie. Die Empfänger prüfen,
  dass der Eintrag vorhanden, die Sperren frei und die Referenzen noch gehalten sind.
- Ein konkurrierender Thread kann nach Rückkehr aus `HeldCapture::open` keine
  der beiden echten Windows-Besitzersperren erwerben. Refresh benötigt keinen
  erneuten Sperr-/Referenzerwerb; Entsperren hält die Actor-Referenzen weiterhin.

Insgesamt **133 native Szenarien, 1.296 gezählte Aufrufe und 119.997 Bedingungen**.
Der größere Bedingungszähler entsteht überwiegend durch erneute Prüfung aller
Tabelleneinträge nach jeder Einfügung. Er entspricht nicht dieser Anzahl
unabhängiger Szenarien. Manifest: 51 Codebereiche, 57 pdata-Fragmente und 26
private TLS-Lesestellen; ohne EXE nur 306 Manifestbedingungen.

## Weiterhin offene Integration

Die Slot-Markierung allein bestätigt weder UI-Aktualisierung noch Persistenz.
Der spätere Verbraucher dieser Liste und ihre native Bereinigung sind noch nicht
angebunden; die Probe räumt ihre privaten Allokationen selbst auf. Engine-Allocator
und dessen Fehlerwege, Effektverarbeitung, Transport, UI und Teile der Kindobjekt-
Verwaltung bleiben Testabhängigkeiten. Die Probe führt keine Allokationsfehler
in fremdem Code herbei.

Es fehlen der vollständige Schreib-/Ereignisweg für mitgeführte **und** ausgerüstete
Items, eindeutige echte Rückmeldungen, Spieler-/Manager-/Thread-/Weltbindung,
Eingabe, Loader und B0-Installation. `HeldCapture` ist keine solche Freigabe.
Keine Ausführung im Spielprozess, keine Savezugriffe, keine Spielinstallation
beschrieben. Rust-/Frontendcode wurde nicht geändert oder erneut getestet.

Nachweise: `.local/repair-runtime-v13-{build-test.log,native-result.json,validation.json}`.
Vorgänger: [Itemkopien 0.12.0](ITEM_LIFECYCLE_2949.md).
Spätere manuelle Abnahme: [TESTCHECKLISTE](../../TESTCHECKLISTE.md), insbesondere
R02, R05, R06, R10 und R12; erst nach vollständiger Implementierung.
