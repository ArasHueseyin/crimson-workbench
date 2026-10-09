# Native Zuordnung für EXE 1.0.0.2949

Stand 22.09.2026. Diese Seite beschreibt den rein lesenden Forschungsnachweis.
Darauf aufbauend bestehen die [isolierte Ausführungsprobe 0.8.0](REGISTRY_NATIVE_INTEGRATION.md)
und ihre [Erweiterung um Besitzersperren und Leser 0.9.0](PINNED_CAPTURE_INTEGRATION.md).
Eigene Nachweise der Inventar-/Equipment-/Auswahlzugriffe ergänzt
[Modul 0.10.0](INVENTORY_2949.md).
Ereignisvorbereitung und native Server-/Client-Probe auf privaten Items ergänzt
[Modul 0.11.0](EQUIPMENT_EVENTS_2949.md); Transport und Effekt-/UI-Empfänger sind Fixtures.
Native Itemkopien und deren Lebensdauer ergänzt
[Modul 0.12.0](ITEM_LIFECYCLE_2949.md), weiterhin mit eigenem Test-Allocator.
Gehaltene Erfassung und native Ausrüstungs-Slot-Markierung ergänzt
[Modul 0.13.0](DIRTY_SLOTS_2949.md); der spätere Slot-Consumer ist noch nicht angebunden.
Der vollständige Feldschreiber für beide Bereiche/Zustandskopien ergänzt
[Modul 0.14.0](FIELD_WRITER_2949.md); die gemeinsame Engine-Transaktion bleibt offen.
Weiterhin kein installierbarer Mod. Die App unterstützt die Tabellen dieses Builds seit
v0.5.9; eine vollständige Reparatur-Runtime ist damit nicht freigegeben.

`tools/observe_registry_build.py` prüft vor und nach dem Lesen den vollständigen
EXE-Hash `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
Es löst acht primäre VTables über Typnamen und selbstreferenzierende MSVC-RTTI-
Locatoren auf, verlangt eindeutige Ergebnisse und Methodenadressen in ausführbaren
PE-Sektionen. Es liest weder Prozesse noch Saves und führt keine EXE-Funktionen aus.
Der Bericht enthält Funktionsgrößen aus pdata, Hashes, Unwind-Köpfe und statische
Sprung-/Aufrufziele. Ein fehlender pdata-Eintrag bekommt keine erfundene Grenze.

| Methode | RVA im neuen Build | Belegte Größe |
|---|---|---:|
| ClientActorManager, Slot `0x28` | `0x8ae140` | 485 Bytes |
| ServerActorManager, Slot `0x28` | `0x2a82300` | 542 Bytes |
| Actor-Freigabe, Slot `0x08` | `0x14366a0` | 757 Bytes |
| WindowsRWLock Acquire, Slot `0x18` | `0x1371b60` | 155 Bytes |
| WindowsRWLock Try, Slot `0x28` | `0x1371c30` | 174 Bytes |

Die konkrete Actor-Freigabe stimmt in CommonActor sowie den vier geprüften
Client-/Server-Normal-/User-Typen überein. Kurze Acquire-Wrapper und die
Lock-Release-Methode besitzen hier keinen eigenen pdata-Eintrag; ihre Grenzen
sind durch diese Probe nicht nachgewiesen.

Die vollständigen Lookup-Hashes:

- Client: `854519c59e00f271c9a5eb4635a29065aeb7ab3e07a89fc18c58eff34d762253`
- Server: `cda4603cc3a252f9d40f9fec9bd40882a0dd65eff422d612e457c8d3e9f29bcc`

Die neue Serveradresse wurde aus der aktuellen VTable gewonnen. Frühere
vorläufige v7-Adressen aus der Update-Untersuchung dürfen nicht übernommen
werden. Der Clientpfad ruft den Registryhelfer `0x464290` und Actor-Slot `0xc0`
auf; der Serverpfad benutzt den Typhelfer `0x466060` und Actor-Slot `0xb8`.
Die Methoden sind deshalb nicht durch einen gemeinsamen angenommenen Wrapper
ersetzbar. Die geschützten Referenzerwerbe, Thread-/TLS-Voraussetzungen und
Freigabeabhängigkeiten müssen vor einer neuen Ausführungsprobe gemeinsam
gebunden und geprüft werden.

Aufruf aus dem Projekt:

```powershell
python runtime/repair/tools/observe_registry_build.py
```

Lokaler Bericht: `.local/repair-build25455892-registry-observation.json`.
Der historische native Host wird nicht gelockert und verweigert diese EXE
weiterhin. Der neue Registry-Host ist getrennt gepinnt und führt die geschützten
Client-/Server-Lookups samt Referenzerwerb und Freigabe auf privaten Testobjekten aus.
Live-Auflösung und Reparaturtransaktion bleiben weitere Aufgaben.
