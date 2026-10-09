# Geschützter Referenzerwerb auf Build 2949 – Modul 0.8.0

Historischer Stand dieses Dokuments. Die Erweiterung um echte Besitzersperren
und direkte Erfassung gehaltener Objekte ist in
[Modul 0.9.0](PINNED_CAPTURE_INTEGRATION.md) dokumentiert.

Stand 22.09.2026. Neuer isolierter Ausführungsnachweis für Steam 25455892 /
EXE 1.0.0.2949. **Weiterhin kein installierbarer Reparaturmod.**
Die Workbench bleibt v0.5.9; die Reparatur bleibt in der App gesperrt.

## Gemeinsamer Erwerbsweg

`repair-registry-native.exe` verbindet den vorhandenen `registry::Source`-Adapter
und `reference::Pair` mit Kopien der aktuellen Client-/Server-Lookups, den
konkreten Normal-/User-Actor-Referenzmethoden und der PaPtr-Freigabe. Alle Manager,
Actors, Buckets, Knoten und Belege gehören ausschließlich dem eigenen Testprozess.

Die Registry-Lookups werden nicht mehr durch künstliche Erwerbs-Callbacks ersetzt.
Der Test führt beide geschützten Suchen mit vollständiger Kennung aus. Ihre
virtuellen Referenzaufrufe werden gemessen und an die unveränderten nativen
Normal-/User-Wrapper weitergeleitet. Diese wählen unterschiedliche Parameter:
NormalActor verwendet die Zustandsmaske `0x10`, UserActor die Art `4`.
Die passenden Referenzmethoden und der tatsächliche Actor-Release laufen gemeinsam.

Die originale WindowsRWLock-Acquire-/Release-Implementierung verwendet echte
Windows-SRW-Imports an privaten Sperrobjekten. Die Registry fordert Lesesperren
an. Im Test muss ein Schreibversuch während des Referenzerwerbs scheitern und
danach wieder möglich sein. Vier zusätzliche Threadfälle halten die Registry
exklusiv: Der Lookup wartet; eine Entfernung während dieser Sperre wird danach
beachtet. Beim zweiten fehlenden Besitzer wird die erste Referenz zurückgegeben.

## Build- und Ausführungsgrenzen

Der Dateileser besitzt zwei ausdrücklich getrennte Build-Selektoren. Der alte
Host verwendet weiter ausschließlich den ursprünglichen Hash und lehnt die neue
EXE ab. Nur der neue Registry-Test wählt 2949. Unbekannte EXEs bleiben gesperrt.

15 vollständige Codebereiche sind zusätzlich zum vollständigen EXE-Hash gepinnt.
Neun davon besitzen exakt geprüfte pdata-Grenzen und erhalten ihre originalen
Stack-Unwind-Codes. Bekannte sprachspezifische Cleanup-Handler werden nur im
privaten Testhost entfernt; die getesteten Abhängigkeiten werfen nicht.
Die kurzen Leaf-/Tail-Wrapper und der Typhelfer samt Sprungtabelle haben getrennt
geprüfte Größen. Die konkreten Actor-Release-Slots sind an fünf VTables kontrolliert.

Vier neun Byte lange GS-Lesezugriffe der Referenzmethoden werden durch
register- und flagserhaltende LEA-Anweisungen auf einen privaten TLS-Vektor
umgeleitet. Alle übrigen Funktionsbytes bleiben unverändert. Die Registry nutzt
`shared=true`; der exklusive TLS-Zweig der Lock-Acquire-Funktion wird nicht
ausgeführt. Host-TEB/TLS wird nicht verändert. Code liegt auf privaten RX-Seiten,
Daten auf schreibgeschützten Seiten; keine Seite ist zugleich schreibbar und ausführbar.

## Bestandene Prüfungen

- Sechs CTest-Suiten und MSVC Release mit `/W4 /WX`.
- **30 native Szenarien, 263 native Aufrufe, 3.490 Prüfbedingungen.**
- Normal-/User-Varianten; beide direkten Referenzmodi und registrierter Modus;
  vollständige Kennung einschließlich Generationsbits; gültige hohe Typbits.
- Fehlender erster/zweiter Registryeintrag, leere Registries, falsche Knotenschlüssel,
  unpassender Actor-Typ, verlorene Zustandsmaske, unterschiedliche User-/Normalmasken.
- Ein fehlgeschlagener nativer Referenzerwerb darf einen Pointer im ungültigen
  Beleg behalten. Daraus entsteht kein Besitz; die Freigabe dereferenziert ihn nicht.
- Teilfehler, genaue Quellzuordnung und umgekehrte Freigabereihenfolge;
  verschachtelter Besitz bleibt erhalten. Registryeinträge werden nicht verändert.
- Alive=0 wird im direkten nativen Pfad tatsächlich akzeptiert. Dieser Besitz
  ist keine Reparaturfreigabe; der getrennte Inventarleser prüft Alive/Zerstörung
  zusätzlich unter den Besitzersperren. Registrierter Erwerb weist Alive=0 ab.
- Letzte Referenz und verzögerte serverseitige Bereinigung: Zustandswechsel
  `0x20 → 0x40`, Freigabe der eingebetteten Sperre vor Bereinigung/Zerstörung,
  genau ein Destruktor-Dispatch. Testobjekte bleiben zur Fehlererkennung allokiert.
- Tatsächliche konkurrierende Registry-Schreibsperren für Client und Server,
  jeweils mit unverändertem oder inzwischen entferntem Eintrag.
- Alter Host lehnt neue Spiel-EXE ab; neuer Host lehnt eine andere eigene PE-Testdatei
  vor nativer Ausführung ab. Die alte Freigabe wird nicht erweitert.

Die bisherigen eigenständigen Suiten bleiben erfolgreich: 1.481 Aktionsbedingungen,
249 Leser-/Referenzbedingungen, 25 Sperrgruppenbedingungen, 55 Adapterbedingungen.
Die 45 Manifestbedingungen des neuen Hosts laufen auch ohne Spiel-EXE; sie sind
kein nativer Erfolgsnachweis. Die 3.490 Bedingungen stammen aus dem gesonderten
Aufruf mit exakt gepinnter EXE.

## Noch zu implementieren

Die Manager werden im Test weiterhin bereitgestellt. Aktueller Spieler,
gesicherte Managerlebensdauer sowie geeigneter Engine-Thread und dessen TLS-Modus
müssen im echten Host aufgelöst werden. Der Inventarleser und weitere native
Änderungs-/Ereignismethoden benötigen ihre neuen Layout-/Buildnachweise.

Thread-Referenzmap, übergeordnete Referenzen und endgültige Bereinigung/Zerstörung
sind klar begrenzte Fixture-Abhängigkeiten. Diese Probe beansprucht keine
vollständige Engine-Lebensdauerverwaltung. Die vollständige Reparaturtransaktion,
Ereigniszuordnung, Eingabe, Loader und B0-Einbau fehlen weiterhin. Es werden keine
verschollenen oder zerstörten Items erzeugt.

## Reproduzieren

```powershell
# Alle synthetischen/Manifesttests; liest keine Spiel-EXE.
./runtime/repair/test.ps1

# Zusätzlich native Registry-/Referenzprobe auf privaten Testobjekten.
./runtime/repair/test.ps1 -RegistryGameExe "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
```

`-GameExe` bezeichnet unverändert den historischen 2944-Host. Für die neue
Registry-Probe ausschließlich `-RegistryGameExe` verwenden.
Nachweise: `.local/repair-runtime-v8-build-test.log`,
`.local/repair-runtime-v8-native-result.json`,
`.local/repair-runtime-v8-build-refusals.json`,
`.local/repair-runtime-v8-validation.json`.

Keine Veränderung der Installation oder Saves, kein Zugriff auf den laufenden
Spielprozess. [Offene Spielabnahmen](../../TESTCHECKLISTE.md).
