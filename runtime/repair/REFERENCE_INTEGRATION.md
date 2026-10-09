# Actor-Referenzen und Lebensdauer 0.6.0

Stand 22.09.2026. Separates Entwicklungsmodul; Workbench weiterhin v0.5.8.
Kein installierbarer Mod, kein Zugriff auf den Spielprozess oder Saves.

## Implementiert

`crimson_repair_reference.lib` verwaltet zwei native 32-Byte-Referenzbelege
(`PaPtr`) als nicht kopierbare/verschiebbare Eigentümer. Ein Host muss sie über
geprüfte Acquire-/Release-Callbacks liefern. Eine ungültige Referenz kann einen
nichtleeren Actorpointer enthalten: Maßgeblich ist das Gültigkeitsbyte `+0x10`.
Die Quellen benötigen dieselbe vollständige Charakterkennung; gleiche Actor-
Adressen für Client und Server werden abgewiesen. Alle Quellen werden vorab geprüft.

Scheitert die zweite Quelle, werden ihr Ergebnis und die erste Referenz in
umgekehrter Reihenfolge aufgeräumt. Öffentliche Aufrufe auf einem fremden Thread
werden abgewiesen. Zerstörung eines noch haltenden Eigentümers auf einem fremden
Thread ist ein Programmierfehler und führt zu `std::terminate`. Der Host muss
zusätzlich den TLS-Referenzmodus während des gesamten Besitzes unverändert halten;
gleiche Thread-ID allein beweist das nicht.

`capture_with_references` hält beide Referenzen über die vollständige geschützte
Inventarerfassung. Der innere Leser gibt zuerst die Inventarsperren zurück,
danach werden die Referenzen freigegeben; das gilt auch für Ablehnung und
C++-Exception. Unter den Sperren werden die vollständige Kennung, `Alive+0x4a`
und der Zerstörungsindikator `+0x4b` vor und nach der Erfassung geprüft.
Das Ergebnis enthält private Datenkopien und keine anhaltende Schreibfreigabe.

## Native Befunde und Funktionsgrenzen

Gepinnte EXE 1.0.0.2944 / Steam 25381195, SHA-256
`6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7`.
Alle Adressen als RVA:

| Funktion | Bytes | SHA-256 |
|---|---:|---|
| NormalActor-Zugriffswrapper `0x4f9000` | 12 | `90521d0c1b64f081e463c6d47ce1ce830aadf6a417b7607269319860382e5eb6` |
| Kennungsgebundener Erwerb `0x1436bf0` | 246 | `30eec32cccb2b3447885935521b2c92e9c951f9a7d6fb729b09f81833beca0d9` |
| Vorhandene Referenz prüfen `0x1436990` | 152 | `17015ad6da39a35b7248364f8e9622fe0d6e221284e201acb38a08f3abc0386b` |
| Konkrete Actor-Freigabe `0x1436690` | 757 | `0a7467851808949a1dddfbbda6f4d1270cefe1262ce80c796da571a171a7b8ad` |
| PaPtr-Freigabe `0x1434900` | 156 | `58962428c45832afd42ef53dbda8feeef6d8bebb53c709f1d79c683bbdc49860` |

Die überprüften Common-/Client-/Server-Actor-VTables zeigen bei `+8` auf
`0x1436690`. Der einfachere Basispfad `0x14345c0` ist **kein Ersatz**: Der
konkrete Pfad kann zusätzlich Zustand `0x20` nach `0x40` überführen und bei
Serverobjekten eine ausstehende Bereinigung aufrufen. Der PaPtr-Destruktor ruft
die virtuelle Freigabe auf und schützt die abschließende Zerstörung mit einem
atomaren Vergleich von `Actor+0x4b`. Das Gültigkeitsbyte wird anschließend gelöscht.
Der gesonderte PaPtr-Modus `+0x11` hat eine eigene Freigabeverzweigung.

Je nach TLS-Flags `+0x1d2`/`+0x1d4` erfolgt der Erwerb über den direkten
Referenzzähler `Actor+0x10` oder über Thread-Registrierung plus `Actor+0x48`
unter der eingebetteten Sperre `Actor+0x18`. Der direkte Pfad prüft **nicht**
das Alive-Feld. Die zusätzliche Ablehnung im Leser ist deshalb erforderlich.
NormalActor- und UserActor-Erwerb verwenden verschiedene virtuelle Wrapper;
der getestete Erwerbswrapper ist ausdrücklich der NormalActor-Pfad.

## Isolierte Ausführungsprobe und Grenzen

Die fünf obigen Funktionen werden aus der hashgeprüften Datei in privaten
Testprozess-Speicher kopiert. Drei neun Byte große TLS-Lesezugriffe werden per
registererhaltendem `lea` auf einen privaten Test-TLS-Vektor umgeleitet.
Alle übrigen Funktionsbytes bleiben unverändert. Keine echte TEB-/TLS-Änderung.
Spiel-Cleanup-Handler werden nur aus den privaten Unwind-Kopien entfernt;
Ausnahmen aus nativen Spielabhängigkeiten sind damit nicht als geprüft behauptet.

13 Szenarien prüfen vollständige Kennungen, abgewiesenen Zugriff, bestehende
äußere Referenzen, beide direkten TLS-Modi, Thread-Registrierungszweig,
abgemeldete Objekte, verzögerte Zerstörung und die zusätzliche Zustandsbereinigung.
Der Sonderbeleg und erneute Freigabe nach Invalidierung sind ebenfalls geprüft.
Ein ungültiger Beleg mit Adresse 1 bestätigt, dass kein Pointerzugriff stattfindet.

**Testabhängigkeiten:** Thread-Registrierung/Lookup, eingebettete Sperrmethoden,
Elternobjektauflösung, erneuter Erwerb bei Bereinigung, Bereinigung und endgültige
Zerstörung sind ausdrücklich Fixture-Callbacks. Die Tests beweisen die nativen
Entscheidungswege und balancierte Aufrufe, nicht die vollständige Engine-
Threadmap, tatsächliche Objektzerstörung oder reale Spielereignisse. Die separate
WindowsRWLock-Probe aus 0.5.0 verwendet weiterhin echte Windows-SRW-Imports.

## Ergänzung 0.7.0

Der Quellenadapter für Manager-Lookup und passende Freigabemethode ist inzwischen
implementiert und synthetisch geprüft. Die neue installierte EXE 1.0.0.2949
verhindert einen weiteren Nachweis mit dem bisher auf 1.0.0.2944 gepinnten Host.
[Registry-Adapter und Buildwechsel](REGISTRY_INTEGRATION.md).

## Was vor Nutzung im Spiel fehlt

Der Host muss aktuelle Client-/Server-Referenzen **aus geschützten Quellen**
erwerben. Eine ungeschützte Actoradresse an den Acquire-Callback zu übergeben
wäre bereits vor dem Referenzerwerb unsicher. Dieser Live-Quellenresolver,
konkrete Methodenbindung, initialisierter Engine-Thread und konstante TLS-Modi
sind noch nicht implementiert. Der bestehende lesende Layoutresolver ersetzt
keine Registry-Lebensdauersperre. Die C++-Callbacks sind kein fertiger Spielhost.

Danach fehlen weiterhin vollständige Reparaturtransaktion, Spielereignisse und
Auftragszuordnung, Eingabe, Loader und B0-Installation. Die Referenzbibliothek
ist nicht mit der Desktop-App verbunden; `installable` bleibt `false`.

## Prüfergebnis

Vier CTest-Suiten und MSVC Release `/W4 /WX` bestanden. 249 Leser-/Referenzbedingungen,
25 Sperrgruppenbedingungen, 1.481 Aktionsbedingungen. Nativer Host insgesamt
203 Aufrufe und 3.859 Bedingungen. Die Reader-Suite prüft unter anderem
Freigabereihenfolge, beide fehlgeschlagenen Erwerbe, Sperrkonflikt, falsche/alte
Kennung, beginnende Zerstörung während der Erfassung, Threadwechsel und Exceptions.
Nachweise: `.local/repair-runtime-v6-build-test.log` und
`.local/repair-runtime-v6-validation.json`.
