# Reparatur 0.23.0 – originaler Inventar-Paketserializer

Stand 23.09.2026, Steam 25455892 / EXE 1.0.0.2949. Entwicklungsmodul;
die Desktop-App bleibt v0.5.9 und bietet noch keine kostenlose Reparatur an.
Diese Proben lesen nur die gepinnte EXE und führen Funktionskopien auf privaten
Daten aus. Der separat beauftragte Offline-Inventarauftrag gehört nicht dazu.

## Was hinzugekommen ist

Der originale Sender `0x29ec2e0` und seine Streamauswahl `0x2961c30` laufen
im privaten Testhost. Zehn Fälle verbinden den Sender mit Reparaturplanung,
nativen Nachherkopien, Feldschreiber, ursprünglichem Client-Ack und neuer
geschützter Erfassung. Die Verbindung zwischen Sender und Ack besteht aus
einem eigenen begrenzten Decoder und Testempfängern. Sie ist kein echter
Engine-Transport und kein Nachweis einer Speicherung.

Der Sender erzeugt exakt folgende Nutzdaten in vier Schreibaufrufen:

| Offset | Größe | Inhalt |
|---|---:|---|
| 0 | 4 Byte | Actor-Kennung aus `actor+0x60`, bei Null-Actor 0 |
| 4 | 2 Byte | übersetzter Inventarschlüssel |
| 6 | 2 Byte | Slot |
| 8 | 2 Byte | absolute Haupt-Haltbarkeit |

Der Paketkopf erhält `uint16_t 0x0c02` und anschließend Byte `0xff`.
Die Nutzdaten enthalten **weder Item-UID noch Sockeldaten noch Auftragskennung**.
Die eigenen Identitäts-, Sockel- und Auftragsprüfungen bleiben daher notwendig.
Die bisherige Client-Probe ist unter [Inventar-Ack](INVENTORY_EVENTS_2949.md)
dokumentiert. Der ursprüngliche äußere Paketleser wird weiterhin nicht ausgeführt;
sein äußerer Erfolgsstatus verschluckt den inneren Fehler und reicht nicht als Beleg.

Die Streamauswahl liest ausschließlich ein eigenes TLS-Objekt mit 0x300 Byte.
Bei `TLS+0x250` liegt dessen privater Kontext. Der Sender leert zuerst die
Empfängerlänge bei `context+0x2b8`; die virtuelle Deskriptormethode füllt sie.
Ohne Empfänger kehrt er zurück. Die Auswahl berücksichtigt den Aktivierungswert,
die Kanaltypen und den virtuellen Deskriptorvergleich; ein unpassender erster
Kanal verhindert nicht die Wahl eines passenden zweiten Kanals.

## Fehler und verbleibende Grenzen

- Ohne Empfänger entsteht kein Paket. Ein fehlender Stream-Puffer erzeugt nur
  eine Diagnose. Die Rückgabe des Senders ist `void` und enthält keinen Beleg.
- Auch wenn der private Schreiber die Bytes nicht annimmt, setzt der Originalcode
  den Kopf und ruft die Finalisierung auf. Ein solcher Aufruf ist kein Zustellbeleg.
- Ohne passenden Stream wird der ursprüngliche Poolzweig erreicht. Hier wird nur
  der Fall einer fehlgeschlagenen privaten Allokation ausgeführt. Erfolgreiche
  Poolallokation, Serializer, Transport, Referenzabgabe und Freigabe dieses Zweigs
  sind noch offen. Beide TLS-Modi sind daher kein Test beider Pool-Freigabewege.
- Empfängeraufbau, Deskriptorvergleich, Stream-Puffer, Byteempfänger, Finalisierung
  und Diagnosen sind eigene Callbacks. Ihre Lebensdauer und Threadbindung im
  laufenden Spiel sind nicht belegt.
- Servermarkierung und reguläres Speichern bleiben Testempfänger. Die zehn
  integrierten Fälle beweisen weiterhin keine vollständige Server-Reparatur.
- Fehlende oder doppelte Zustellung, falscher Actor, Fehler im inneren Ack und
  UID-Wechsel nach dem Feldschritt ergeben keinen Erfolg. Die Fehlersperre
  verhindert automatische Wiederholung nach einer möglichen Teiländerung.

## Gepinnte Funktionen und Prüfung

EXE-SHA-256:
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.

| Funktion | RVA / Länge | SHA-256 |
|---|---|---|
| Paketserializer | `0x29ec2e0` / `0x4c0` | `b227df1159b017f454a9362c085861f99697e526398de71a3f1fc738cf039d7f` |
| Streamauswahl | `0x2961c30` / `0xab` | `98a78f3849be23c8fdf87be14801564a00b6f54f0438c40140a5130e5bd0842b` |

Beide vollständigen Funktionsgrenzen sind durch pdata geprüft. Zwei TLS-Stellen
werden auf den privaten Kontext umgeleitet; alle übrigen Codebytes bleiben
gleich. Der Serializer behält seine Stack-Unwinddaten, verwendet aber keinen
Sprach-Cleanuphandler aus dem Spiel. Private Codebereiche sind RX, Daten R-only.

**36 neue Fälle:** 13 direkte Szenarien in zwei TLS-Modi und zehn Integrationen.
Direkt geprüft: exakte Feldbreiten/Bytefolge, Null-Actor, Null-/Maximalwerte,
keine/mehrere Empfänger, fehlender Puffer, verworfene Bytes, deaktivierte
Streamauswahl, Typabweichung, negativer Vergleich und Auswahl des zweiten Kanals.
Integrationen: gewöhnliche Reparatur, nur beschädigte Sockel, No-Wear, fehlender
Empfänger/Puffer, fehlerhafte Containerübersetzung, ausbleibende/doppelte
Zustellung, UID-Wechsel und falsche Actor-Kennung.

MSVC Release `/W4 /WX` und neun CTest-Suiten bestanden. Gesamt:
**454 native Szenarien, 4.812 gezählte Aufrufe, 206.786 Bedingungen**.
Viele Bedingungen sind wiederholte Byteprüfungen und keine unabhängigen Fälle.
102 Codebereiche, 127 pdata-Fragmente, 60 ersetzte TLS-Stellen sowie weiterhin
ein unveränderter Stackprobe-Zugriff auf den eigenen Windows-Thread.
CTest-Bedingungen: 11/1.529/519/25/55/612/29/202/273.

Die nächsten praktischen Lücken bleiben der tatsächliche Server-/Speicherabschluss,
Spieler- und Managerlebensdauer, Engine-Thread/Queue, Eingabe, Loader und B0.
