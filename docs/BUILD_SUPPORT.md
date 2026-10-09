# Unterstützung der Spielbuilds

Stand: 22.09.2026, Workbench v0.5.9.

| Steam-Build | EXE-Version | Leseschema | Stages | Quests |
|---|---|---|---:|---:|
| 25381195 | 1.0.0.2944 | `steam-25381195-gamedata-2.3-v2` | 52.080 | 1.097 |
| 25455892 | 1.0.0.2949 | `steam-25455892-gamedata-2.3-v2` | 52.082 | 1.098 |

Die Unterstützung umfasst Datenlesen, Modplanung und Proben an Projektkopien.
Sie ist keine Vanilla-Zulassung der Installation, kein Nachweis der Wirkung im
Spiel und keine Freigabe der nativen Reparaturfunktionen für den neuen Build.

## Nachweis des Updates

Die neue EXE hat SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
24 von 38 erfassten EXE-/Metadatendateien unterscheiden sich vom vorherigen Build.
Diese Zahl beschreibt die erfassten Dateien, keine vollständige Inhaltsprüfung
aller Spielarchive.

61 von der Workbench verwendete Tabellen-, Icon- und Sprachdateien wurden aus
den Archiven gelesen und mit den bisherigen vollständigen Hashes verglichen:
57 sind bytegleich. Nur Body und Header von `questinfo` und `stageinfo` ändern
sich. Alle 14 indizierten Tabellenpaare lassen sich bytegleich rekonstruieren.
Die Item-Lokalisierungen aller 15 Sprachen sowie die zusätzlich verwendeten
Crafting- und Advanced-Tabellen bleiben identisch.

- Stages: neue IDs `1001269` und `1003266`; kein bisheriger Datensatz geändert
  oder entfernt. Die beiden bearbeitbaren Patrouillen `1002224` und `1017811`
  bleiben vollständig bytegleich zum alten Build.
- Quests: neue ID `1000320`, geänderte ID `1000107`, keine entfernte ID.
  Die Workbench modifiziert diese Questdaten nicht.
- Der Mod-Integrationstest prüft jede Stage gegen ihre Originalbytes. Nur die
  beiden gewählten Patrouillen dürfen geändert werden. Beide neuen Stages
  müssen erhalten bleiben; Questdateien dürfen nicht im Overlay erscheinen.

Reproduzierbare lesende Vergleichsprobe:

```powershell
cargo run --locked -j 2 -p cd-core --example audit_build_transition -- "C:\Program Files (x86)\Steam\steamapps\common\Crimson Desert"
```

Der Datensatzvergleich benötigt die hashgeprüften alten Stage-/Questdateien in
`.local/archive-probe/extracted`. Sie werden nicht mit dem Projekt verteilt.
Der lokale Bericht liegt unter `.local/build-25455892-table-comparison.json`.
Die Probe schreibt keine Spieldateien und erteilt selbst keine Buildfreigabe.

## Auswahl und Abweisung

Die EXE-Version wählt einen bekannten Satz von Referenzhashes. Danach müssen
sämtliche erfassten EXE-/Metadatendateien und die verwendeten Tabellen zu genau
diesem Satz passen. Ein passender Versionsstring allein genügt nicht. Tabellen
werden an die bereits geprüfte Identität gebunden; ein späteres erneutes Lesen
der EXE-Version darf keinen anderen Satz auswählen.

Suchindex und JSON-Ausgaben führen die tatsächlich verwendete Schema-ID.
Sicherungsberichte benötigen Build-ID, Dateigrößen und Hashes eines vollständigen
bekannten Builds. Gemischte Metadaten beider Builds, fehlende Dateien und falsche
Größen werden abgewiesen. Ein alter Bericht wird nicht zur neuen B0-Basis erklärt.
Weitere unbekannte EXE-Versionen stoppen vor dem Öffnen einer Mod-Sicherung,
damit ein Spielupdate nicht als fehlende Projektprobe gemeldet wird.

Der historische native Reparaturhost bleibt an EXE 1.0.0.2944 gebunden und
verweigert die neue EXE. Ein getrennter Registry-/Referenzhost ist inzwischen
für 1.0.0.2949 geprüft: 30 Szenarien und 263 native Aufrufe auf privaten Daten.
Weitere Inventar-/Änderungs-/Ereignismethoden und die tatsächliche Spielanbindung
bleiben offen. [Getrennter nativer Nachweis](../runtime/repair/REGISTRY_NATIVE_INTEGRATION.md).

Weitere Nachweise: [TESTING](TESTING.md). Spätere manuelle Abnahmen:
[TESTCHECKLISTE](../TESTCHECKLISTE.md).
