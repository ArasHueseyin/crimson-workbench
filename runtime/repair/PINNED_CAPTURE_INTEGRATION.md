# Reparatur 0.9.0 – Erfassung aus gehaltenen Referenzen

Historischer Stand. Eigene 2949-Nachweise der hier noch als Legacy-Fixtures
bezeichneten Lesefelder sind inzwischen in [Modul 0.10.0](INVENTORY_2949.md) ergänzt.

Stand: 22.09.2026. Entwicklungsmodul, weiterhin **kein installierbarer Mod**.
Desktop-App v0.5.9 und deren gesperrte Reparaturoption bleiben unverändert.

## Behobene Lücke

Der bisherige geschützte Leser erwarb beide Actor-Referenzen und Besitzersperren,
rief anschließend aber den allgemeinen Diagnose-Leser auf. Dieser suchte die
Server-Registry erneut ohne deren eigene Sperre ab. Eine Actor-Sperre schützt
weder Registryknoten noch einen dort inzwischen eingetragenen anderen Actor.

`capture_with_locks` erfasst jetzt ausschließlich das bereits gehaltene Paar.
Eine gültige vollständige Kennung ist zwingend; Alive-/Zerstörungszustand werden
vor und nach der Erfassung geprüft. Die Clientauswahl muss dem gehaltenen Client
entsprechen. Eine andere Auswahl wird vor dem Lesen ihrer Charakterdaten
abgewiesen. Der Clientanker und alle gelesenen Itemdaten bleiben Teil des
Kontrolllesens; eine erkannte Änderung verwirft die gesamte Ausgabe.

Der allgemeine `capture`-Pfad bleibt für Diagnose-/Offlinefixtures bestehen und
ist ausdrücklich keine Live-Erwerbsquelle. Beide Wege verwenden dieselbe
Item-, UID-, Positions-, Definitions- und Spiegelprüfung. Keine Leseoperation
ändert Haltbarkeit oder löst Spielereignisse aus.

## Gemeinsamer nativer Test

Die Probe aus [0.8.0](REGISTRY_NATIVE_INTEGRATION.md) ist um die vollständige
WindowsRWLock-Try-Funktion des Builds 1.0.0.2949 erweitert:

| Eigenschaft | Nachweis |
|---|---|
| RVA / Größe | `0x1371c30` / 174 Bytes |
| SHA-256 | `f928b8b10862b8348823b946709b6d75d7eed1f2a6665350a0cfa8d9cecbf606` |
| pdata / Unwind | Exakte Grenze bis `0x1371cde`; Header `11140800` |
| TLS-Lesezugriff | Nur neun Bytes bei `0x1371c5f` auf privaten TLS-Vektor umgeleitet |
| Abhängigkeiten | Echte Windows-TryAcquireSRW-/ReleaseSRW-Funktionen und GetCurrentThreadId; ausschließlich eigene Sperrobjekte |

Insgesamt sind jetzt 16 Codebereiche, zehn pdata-Grenzen und fünf TLS-Lesestellen
gepinnt. Die bisherigen originalen Referenzmethoden und Freigaben bleiben Teil
derselben Probe. Es wird weder Spielcode in einen laufenden Prozess geladen noch
dessen Speicher gelesen. Der vollständige Quelldateihash wird vorher und nachher
geprüft; Codekopien liegen auf privaten RX-Seiten, Daten auf getrennten Seiten.

18 zusätzliche Szenarien verbinden geschützte Registry-Suche, echte Actor-
Referenzen, tatsächliche nicht wartende Besitzersperren, Leser und Reparaturplanung:

- Normal-/User-Actors und direkter/registrierter Referenzmodus.
- Beide Besitzer besitzen beim Itemlesen gültige Referenzen und echte exklusive
  Windows-Sperren; auch das Kontrolllesen erfüllt diese Bedingung.
- Ein konkurrierender Leser blockiert jeweils eine Besitzersperre. Die Erfassung
  meldet `lock_busy`, liest keine Items und gibt bereits erworbene Mittel zurück.
- Fehlender zweiter Besitz, alte Generationskennung, Tod und Zerstörungszustand.
- Andere/fehlende Clientauswahl und Auswahlwechsel beim Kontrolllesen.
- Ungültiger Sockelpointer, abweichende Item-UID und unzulässige Haltbarkeit.
- Ein nach Referenzerwerb ersetzter Registryeintrag wird nicht erneut gelesen.
  Das gehaltene Objekt bleibt in diesem Fixture lebendig; dies ist kein Beleg,
  dass ein abgemeldetes Objekt noch geändert werden darf.
- Sperren werden vor den Referenzen freigegeben; kein Restbesitz und keine
  blockierte Windows-Sperre nach Erfolg oder Fehler. Quelldaten bleiben bytegleich.

**Wichtige Grenze:** Inventar-, Equipment- und Clientankerlayout dieser kombinierten
Probe sind private Fixtures des dokumentierten 2944-Lesers. Die native Erwerbs-
und Sperrseite stammt aus 2949. Der gemeinsame Erfolg beweist das Zusammenspiel
der Komponenten, **keine Live-Freigabe des Inventarlayouts für 2949**.

## Ergebnisse

- Sechs CTest-Suiten, MSVC Release mit `/W4 /WX` bestanden.
- 362 synthetische Leser-/Referenzbedingungen; darunter ausdrücklich unlesbare
  Registrybereiche und unbesessene Ersatzobjekte, auf die kein Leseversuch erfolgt.
- 48 native Szenarien, 503 gezählte native Aufrufe, 3.867 Prüfbedingungen.
  Die ursprünglichen 30 Registry-/Referenzfälle sind darin enthalten.
- 1.481 Aktions-, 25 Sperrgruppen- und 55 Quellenadapterbedingungen bleiben bestanden.
- Der Test ohne EXE prüft 48 Manifestbedingungen, führt aber keinen nativen
  Spielcode aus. Die native Probe benötigt weiterhin den ausdrücklichen EXE-Pfad.

Reproduktion:

```powershell
./runtime/repair/Test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

Nachweise: `.local/repair-runtime-v9-build-test.log`, `-native-result.json`,
`-reader-result.json`, `-validation.json` mit demselben Präfix.

## Noch offen

Manager-/Kontextlebensdauer, tatsächliche aktuelle Spielerauswahl, Engine-Thread
und TLS-Modus müssen vom Spielhost gesichert werden. Referenzen auf Actors halten
ihre Manager nicht automatisch am Leben. Vergleichslesen erkennt keinen
unbemerkten A→B→A-Wechsel; jeder Lade-/Weltwechsel braucht ein echtes Hostsignal
und eine neue Epoche, auch bei wiederverwendeten Adressen und Kennungen.

Der untersuchte Bewegungshook der MIT-Referenz Trinity beschreibt selbst seine
Spielerzuordnung als praktische Beobachtung ohne vollständige statische Kette.
Er wurde deshalb nicht als bewiesener Dispatchpunkt übernommen. Die eigene
Reparatur darf nicht bloß auf irgendeinem Bewegungs- oder Renderthread laufen.

Neue Inventar-/Ereignisnachweise, vollständige Änderungstransaktion und bestätigte
Effekte, Eingabe, Loader sowie B0-Einbau fehlen weiterhin. Die Engine-Threadmap,
Parent-Referenzen und endgültige Bereinigung sind nach wie vor Testabhängigkeiten.
Manuelle Spieltests folgen wie vereinbart erst nach der Entwicklung aller Phasen;
die Fälle stehen in der [Testcheckliste](../../TESTCHECKLISTE.md).
