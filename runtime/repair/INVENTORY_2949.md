# Reparatur 0.10.0 – Inventar- und Ausrüstungszugriffe für Build 2949

Historischer Layoutnachweis. Ergänzende Ereignisprüfung:
[Entwicklungsmodul 0.11.0](EQUIPMENT_EVENTS_2949.md).

Stand 22.09.2026. Steam 25455892 / EXE 1.0.0.2949, vollständiger SHA-256:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
Weiterhin Entwicklungsmodul ohne Spielhost, Loader oder installierbare Reparatur.
Die Desktop-App bleibt v0.5.9, die Reparaturoption gesperrt.

## Neu implementiert und belegt

Der Leser verlangt jetzt ausdrücklich einen bekannten Build in `Frame`.
Nicht gesetzte und unbekannte Selektoren werden vor Referenzerwerb, Sperren oder
Speicherlesen abgewiesen. Der Host muss die vollständige EXE und Tabellen selbst
prüfen; das Setzen eines Enumwerts ersetzt keine Buildzulassung.

Die Feldstruktur ist für 2949 separat geprüft. Die bekannten Globaladressen
und Feldoffsets bleiben gleich; mehrere Funktionsadressen und Hashes ändern sich.
Es wurde keine pauschale Verschiebung oder Übernahme alter Funktionsbytes verwendet.

| Funktion / Beobachtung | RVA | Umfang |
|---|---|---|
| Inventarslot | `0x212f4b0` | 226 Bytes, vier vollständig erhaltene pdata-/Unwindfragmente |
| Inventarbesitzer | `0x212a100` | 127 Bytes; direkte, umgeleitete und fehlende Besitzer |
| Aktuelle Clientauswahl | `0x8b4480` | 140 Bytes; eigener nativer Referenzbeleg, kein Registry-Lookup |
| Haltbarkeits-Updater | `0x240d660` | 405 Bytes; Item- und Sockelfelder |
| Besitzer-Sperrbeleg | `0x393650` | 104 Bytes; Lock-Referenz und Erwerb |
| Ausrüstungsänderung | `0x20c90f0` | 840 Bytes; Slotwahl und ausschließlich der Zweig mit Delta 0 geprüft |
| Prüfung unbegrenzter Haltbarkeit | `0x240d4c0` → `0xf31ffd0` | Originaler 5-Byte-Sprung und vollständiger 297-Byte-Zielkörper |
| Zwei Clientanker-Aufrufer | `0x39b670`, `0x39ec90` | Ganze Funktionen statisch gehasht; gleicher Global-/Managerpfad |
| Server-/Equipment-Aufrufer | `0x2a94c50` | Ganze Funktion statisch gehasht; Servermanager und Equipmentpfad |

Die vollständigen Funktionshashes stehen im Testmanifest und in
`tests/inventory_native.inl`. Die drei Anker-Aufrufer werden nur statisch geprüft.
Die übrigen ausgewählten Funktionen werden als private Kopien ausgeführt.
Insgesamt umfasst das Manifest 24 Codebereiche und 20 pdata-Fragmente;
sechs TLS-Lesestellen sind auf private Daten umgeleitet. Die originale
WindowsRWLock-Acquire-Funktion wird jetzt auch im exklusiven Zweig ausgeführt.

## Bestätigte Felder

- Clientglobal `0x6d691b0` → Kontext `+0x30` → Manager `+0x50`.
  Zwei unabhängige Aufrufer stimmen überein. Die aktuelle Auswahl erwirbt über
  den Actor-Slot `+0xc0` einen Referenzbeleg; Pointer ohne Gültigkeitsbyte reichen nicht.
- Serverglobal `0x6d696c0` → Kontext `+0x48`; Actor `+0x68` → Equipment `+0x38`.
  Manager-/Actor-Kennung und Referenzlayout sind bereits separat nativ geprüft.
- Holdergetter: Actor `+0x68`, Komponenten `+0xb8`; je Charakterdefinition
  direkte Nutzung oder Umleitung über Actor `+0xa0` → Possessor `+0xd0`.
  Die Reader-Rückverweisprüfung bleibt bewusst konservativ.
- Holder `+0x18/+0x20` → Containerarray; Containertyp `+0x10`, signed Slotgrenze
  `+0x0c`, Itemarray `+0`, Itemstride `0xc8`; Ausschlussliste `+0x20/+0x28`
  mit 12-Byte-Einträgen. Key `+8` und positive Menge `+0x10` kennzeichnen vorhandene Items.
- Equipment `+0x90` → Tabelle mit Array `+8` und Anzahl `+0x10`;
  Einträge `0xd0` Bytes, Slottag `+0xc8`. Der native Pfad greift im Test tatsächlich
  auf den zweiten passenden Slot zu; Definitionsabfragen protokollieren dessen Adresse.
- Haupt-Haltbarkeit `+0x40`; Sockelpointer `+0x60`, Anzahl `+0x68`, Kapazität
  `+0x70`; Sockelstride 6 Bytes, Haltbarkeit `+2`.

## Automatisierter Nachweis

**Sechs CTest-Suiten, MSVC Release `/W4 /WX`, 385 Leserbedingungen sowie
88 native Szenarien mit 578 gezählten Aufrufen und 6.219 Prüfbedingungen bestanden.**

Zusätzlich zu den 48 bisherigen Erwerbs-/Erfassungsfällen:

- 21 Inventar-/Besitzerfälle: negative und zu große Slots, falscher Containertyp,
  leere/verbrauchte Items, zweite Filterzeile, direkte und umgeleitete Besitzer.
  Der eigene Leser und der originale Getter stimmen nach Presence-Normalisierung
  überein. Ein absichtlich unzugänglicher Slot im eigenen Prozess prüft die
  vollständige native Unwind-Kette und die saubere Ablehnung im Leser.
- Acht Fälle der aktuellen Clientauswahl: Normal-/User-Varianten, direkter und
  registrierter Referenzmodus, fehlende Auswahl, Zustandsmasken und Alive-Zustand.
  Direkter nativer Besitz kann Alive=0 akzeptieren; der Leser lehnt Reparaturerfassung
  für solche Besitzer weiterhin gesondert ab.
- Acht Ausrüstungsfälle: tatsächliche Auswahl des zweiten Slottags, fehlender
  Slot, leere Tabelle, leeres/verbrauchtes Item, unbegrenztes Item, Sockel und
  verschiedene Haltbarkeit. Originaler Sperrbeleg sowie Acquire/Release halten
  eine echte Windows-Sperre und geben Referenzzahl und Rekursion vollständig zurück.
- Drei Updaterfälle: Delta 0 und Sentinel bleiben stabil. **Positives Delta 65
  erhöht die Haupt-Haltbarkeit 35 → 100, setzt den beschädigten Sockel 5 aber auf 0.**
  Dieser Originalpfad bleibt deshalb ungeeignet für unsere eigene Sockelreparatur.
- Die erfolgreichen kombinierten Erfassungen werden zusätzlich mit den aktuellen
  nativen Holder-/Slotgettern auf exakt denselben privaten Quellen verglichen.

Die Aktions-, Sperrgruppen- und Quellenadapter-Suiten bestehen weiterhin mit
1.481, 25 und 55 Bedingungen. Ohne EXE führt der native Host nur 72
Manifestbedingungen aus; daraus wird kein nativer Erfolg abgeleitet.

## Grenzen und nächster Schritt

Die bisher nur als Legacy-Fixtures eingeordneten Lesefelder besitzen jetzt eigene
2949-Belege. **Das ist noch kein vollständig angebundener Spielhost.** Managerleben,
aktueller Engine-Thread/TLS-Modus, Weltwechsel und echte Sitzungs-Epochen fehlen.
Die aktuelle Clientauswahl-Funktion synchronisiert ihren Managerpointer nicht
selbst; sie ist kein Ersatz für diese Hostpflichten.

Die Ausrüstungsprobe führt ausschließlich Delta 0 beziehungsweise früh abgewiesene
Items aus. Die leere temporäre Itemkonstruktion/-destruktion und Tabellenresolver
sind ausdrücklich Fixture-Abhängigkeiten. Nichtzero-Delta-Änderung, Itemkopie,
Sockelentfernung, Ereignisse und abgeleitete Effekte sind damit **nicht** bewiesen.
Threadmap und endgültige Actorbereinigung bleiben ebenfalls Testabhängigkeiten.
Es wurden keine Spiel-/Savezugriffe oder Installationsänderungen vorgenommen.

Der nächste Integrationsschritt muss den eigenen Reparaturschreibweg samt
Inventar-/Equipment-Ereignissen belegen und an einen nachgewiesenen Spielablauf
binden. Originale Felder einfach zweimal zu überschreiben reicht dafür nicht.

Reproduktion:

```powershell
./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

Nachweise: `.local/repair-runtime-v10-build-test.log`, `-native-result.json`,
`-reader-result.json`, `-validation.json` unter demselben Präfix.
Manuelle Tests bleiben bis nach Entwicklung aller Phasen zurückgestellt:
[Testcheckliste](../../TESTCHECKLISTE.md), [Phasenstatus](../../PHASENSTATUS.md).
