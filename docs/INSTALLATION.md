# Crimson Workbench installieren

## Windows-Setup

1. Auf [GitHub Releases](https://github.com/ArasHueseyin/crimson-workbench/releases)
   das gewünschte Release öffnen und `Crimson-Workbench_<Version>_x64-setup.exe`
   herunterladen. Die automatisch angebotenen Source-code-ZIPs sind für Entwickler.
2. Spiel und Workbench schließen und die Setup-Datei starten. **Live-Items und
   Zusatzsockel sind bereits enthalten und zur Installation ausgewählt.** Das
   Setup erkennt Steam-Spielordner automatisch; bei mehreren oder nicht erkannten
   Installationen den Ordner mit `bin64` direkt im Setup auswählen. Für die
   Spielmodule kann Windows Administratorrechte anfordern. Wer nur die Workbench
   möchte, kann die Spielmodule abwählen. Kein separates ZIP und keine Befehle nötig.
   Die Workbench selbst installiert nur für dein Windows-Benutzerkonto
   und bietet Deutsch/Englisch an. Falls WebView2 fehlt, lädt das Setup die
   Microsoft-Laufzeit nach; dafür ist eine Internetverbindung nötig.
3. Crimson Workbench über das Startmenü öffnen. Unter **Datenquellen** die eigene
   Crimson-Desert-Installation auswählen und bei Bedarf den eigenen Save-Ordner
   angeben. Auf **Einstellungen speichern** klicken, damit die Auswahl beim
   nächsten Start wieder verfügbar ist.

Benötigt werden Windows x64 und eine eigene kompatible Spielinstallation.
Rust, Node.js, GitHub-Konto und GitHub CLI sind für die fertige App nicht nötig.
Die Pakete sind derzeit nicht mit einem kommerziellen Windows-Zertifikat signiert.
Windows kann deshalb einen unbekannten Herausgeber melden. Quelle und die
mitgelieferten SHA-256-Prüfsummen vor der Ausführung kontrollieren.

## Portables Paket und eigene Daten

Alternativ `Crimson-Workbench_<Version>_windows-x64-portable.zip` in einen eigenen
Ordner entpacken und `crimson-workbench.exe` starten. WebView2 muss installiert
sein. Die App legt Einstellungen, Cache, Sicherungen und Exporte standardmäßig
unter `%LOCALAPPDATA%\CrimsonWorkbench` ab. Sie benötigt den Quellcodeordner nicht.
Mit `--project "D:\Meine Workbench-Daten"` kann ein anderer beschreibbarer
Datenordner gewählt werden. Ein bestehender Entwicklungscheckout und dessen
`.env` bleiben weiterhin nutzbar.

Die Workbench-Deinstallation entfernt die App. Deine Benutzerdaten sowie die
Spielinstallation bleiben erhalten. Vor einem Update die Workbench schließen;
ein neuer Installer aktualisiert die Anwendung am vorhandenen Installationsort.

## Kompatibilität

Die Katalogdaten unterstützen diese beobachteten Steam-Versionen:

| Steam-Build | EXE-Version |
| --- | --- |
| 25381195 | 1.0.0.2944 |
| 25455892 | 1.0.0.2949 |
| 25477059 | 1.0.0.2976 |

Zusätzlich müssen die geprüften Tabellen-/Dateistände passen. Nach Spielupdates
können Funktionen gesperrt sein, bis der neue Build geprüft ist. Die automatische
Erkennung von Epic/Game Pass bedeutet keine vollständige Abnahme dieser Versionen.

## Optionale Live-Items und Zusatzsockel

Ab Version 0.6.2 erledigt die einzelne Setup-EXE die Einrichtung dieser Module.
Ein erneuter Durchlauf erkennt identische installierte Module und erhält deren
Konfiguration. Andere oder veränderte Mods werden weiterhin nicht überschrieben;
bei einem Konflikt zeigt das Setup die Ursache und bricht die Moduleinrichtung ab.
Die Workbench-App bleibt dabei installiert. Die App-Deinstallation entfernt keine
Dateien aus dem Spiel; für den gezielten Rückbau liegt im App-Ordner unter
`runtime-mods` das ursprüngliche `Uninstall-RuntimeMods.ps1` bei.

Für portable Nutzer und den gezielten Rückbau enthält das weiterhin verfügbare
`Crimson-Workbench_<Version>_optional-runtime-mods.zip` die
beiden Laufzeitmods, deren Installations-/Rückbauskripte, Lizenztexte und eine
neutrale Sockelkonfiguration.
Entpacke das Paket und folge seiner `README.md`. Die Skripte verlangen die eigene
Spielinstallation und prüfen die freigegebene EXE, alle Paketdateien sowie
vorhandene Mods, bevor sie etwas schreiben. Einen vorhandenen Loader nicht ersetzen.

Diese optionalen Module unterstützen ausschließlich die geprüfte EXE
**1.0.0.2976**, SHA-256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
Ihre echte Spielabnahme steht noch aus; die erste Verteilung ist daher ein
Preview-Release. Spiel vor Installation, Sockeländerungen und Rückbau speichern
und schließen. Eigene Saves und bestehende Konfigurationsdateien nicht durch
Dateien anderer Nutzer ersetzen.

## Lizenz

Der eigene Workbench-Code steht unter der mitgelieferten MIT-Lizenz. Du darfst
ihn nutzen, ändern und weitergeben; Copyright- und Lizenzhinweise bleiben erhalten.
Fremdkomponenten behalten ihre Lizenzen in `THIRD_PARTY_NOTICES.md` und `licenses/`.
Spielinhalte gehören nicht zum Paket; sie werden aus deiner eigenen Installation
gelesen. Es ist kein Kauf einer zusätzlichen Workbench-Lizenz nötig.

## Download über GitHub CLI (optional)

Mit installierter GitHub CLI beispielsweise für die Preview-Version 0.6.2:

```powershell
gh release download v0.6.2 --repo ArasHueseyin/crimson-workbench --pattern '*-setup.exe'
```

Danach die heruntergeladene Setup-Datei ausführen. Das Downloadkommando selbst
installiert nichts. Prüfsumme einer Datei: `Get-FileHash .\<Dateiname> -Algorithm SHA256`.
