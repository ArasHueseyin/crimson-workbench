# Reparatur 0.17.0 – Client-Rückmeldung für mitgeführte Items

Stand 22.09.2026, Steam 25455892 / EXE 1.0.0.2949, SHA-256
`a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.
**Entwicklungsmodul, weiterhin keine nutzbare Reparatur in der App.** Die
Desktop-App bleibt v0.5.9. Keine Spielprozess-, Installations- oder Saveänderung.

## Neue Verbindung

Der originale Inventar-Reparatur-Ack `0xa11ca0` ist jetzt in einer privaten
Inventarroute des gemeinsamen Transaktionsadapters ausgeführt. Originale Itemkopien,
Feldschreiber und Referenz-/Sperrfunktionen arbeiten dabei auf denselben eigenen
Quellen. Der Serverteil dieser Inventarroute und ihr Abschluss bleiben eigene
Testcallbacks; eine vollständige Engine-Transaktion wird damit nicht behauptet.

Der Ack nimmt einen Inventarschlüssel, Slot und **absoluten 16-Bit-Hauptwert** an.
Er übersetzt den Schlüssel, sucht den Inventarbesitzer, erwirbt dessen exklusive
Sperre und prüft den Slot auf vorhandenes Item, gültigen Key und positive Menge.
Nach dem Hauptwert-Store gibt er die Sperre frei und meldet der Oberfläche
`Zielwert - vorheriger Wert` als 16-Bit-Differenz. Sockel werden weder verändert
noch in dieser Meldung übertragen. Definitionen, Schlüsselübersetzung,
Namensformatierung mit leerem Testnamen und Oberfläche sind begrenzte Testempfänger.
Nichtleere Namen und deren native String-Allokationszweige sind hier nicht ausgeführt.

Der gemeinsame Schreiber hat vor der Benachrichtigung bereits beide Zustandskopien
aktualisiert. Der native Ack schreibt deshalb denselben Hauptwert und meldet in
dieser Integrationsprobe Delta 0. Die tatsächliche Reaktion der Oberfläche auf
Delta 0 bleibt offen; der UI-Testempfänger prüft ausschließlich die Aufrufdaten.

## Warum eine Rückmeldung allein nicht genügt

- **Keine UID-Prüfung:** Der Ack akzeptiert ein anderes vorhandenes Item im
  gleichen Slot. Der native Direktfall weist dieses Verhalten nach; in der
  Integration verweigert anschließend eine neue geschützte Erfassung den Erfolg.
  Die spätere Spielroute braucht zusätzlich eine Identitätsprüfung vor Zustellung.
- **Keine Zielwert-Zulassung:** Auch 0 und 65.535 werden unverändert geschrieben.
  Nur der geprüfte eigene Reparaturplan darf den übergebenen Zielwert bestimmen.
- **Keine Speicherung:** Der Ack ändert die Clientdarstellung und ersetzt weder
  die Serveränderung noch den Persistenzauftrag oder dessen Bestätigung.
- **Äußerer Paketstatus reicht nicht:** Der statisch geprüfte Paketverteiler
  `0xbbee00` ruft den inneren Ack auf, setzt danach aber seinen äußeren Ergebniscode
  unabhängig davon auf 0. Der Host darf diesen äußeren Erfolg nicht als Beleg
  für eine erfolgreiche Reparatur verwenden. Dieser Verteiler wurde nicht ausgeführt.

## Buildbelege

| Bereich | RVA / Länge | SHA-256 |
|---|---|---|
| Vollständiger nativer Client-Ack | `0xa11ca0` / `0x322` | `391c116f3606894b6a2a4de7623e51a56d8e05111099597c12a04b998486abe4` |
| Statischer Paketverteiler-Anker | `0xbbee00` / `0x379` | `5ac29646aa893ceb1983e76c63425142ca493bfe464a398f663bfb5840a3505d` |

RTTI `TrocTrRepairItemToInventoryAck`: vtable `0x5609728`, Methode `+0x10`
führt zum Verteiler. Dieser verwendet Actor-Komponenten `+0xd8` für den
Reparaturbaustein. Die Probe prüft diesen Zugriff bytegenau. Das ist kein
Nachweis der tatsächlichen Lebensdauer einer geladenen Spielkomponente.

Der private Host hat jetzt 56 vollständig gepinnte Codebereiche, 70 pdata-
Fragmente und 35 private TLS-Stellen. Die drei neuen TLS-Stellen liegen im Ack;
unveränderte Bytes außerhalb der erlaubten Umleitungen werden geprüft.

## Prüfung

Neun CTest-Suiten und MSVC Release `/W4 /WX` bestanden. Die bestehenden
Produktionssuiten bleiben bei 43 Transaktionsszenarien/273 Bedingungen und
35 Schreibszenarien/202 Bedingungen.

**36 neue native Inventarereignisfälle:**

- 14 Direktfälle in beiden TLS-Allokationsmodi: beschädigt, Haltbarkeit 0,
  bereits voller Wert, Zielwert 0/65.535, ungültige Übersetzung, fehlender
  Container, negative/zu große Slots, ungültiger Key, Menge 0/-1, ausgeschlossene
  Items und eine ausgetauschte UID. Die beiden Modi erreichen bei leerem Namen
  keine String-Allokation; sie sind kein Test beider String-Allokationszweige.
- Acht Integrationsfälle: normal, ausschließlich Sockel beschädigt, No-Wear,
  fehlerhafte Containerübersetzung nach Feldschritt, UID-Wechsel, fehlende
  Zustellung, fehlgeschlagener Abschluss und doppelte Rückmeldung.
- Exakte Änderung ausschließlich des Hauptworts im Direktfall; Inventarmetadaten,
  Sockel, UID, Menge und Serverdaten bleiben sonst unverändert.
- Formatierung unter Client-Sperre, UI-Aufruf nach Freigabe, gehaltene Besitzer,
  native Kopierlebensdauer und vollständige Bereinigung geprüft.
- Alle Fehler nach dem Feldschritt erzeugen ein unklares Ergebnis ohne Replay.
  Frische Daten und vollständige künstliche Belege sind für privaten Erfolg nötig.

Gesamt **222 native Szenarien, 3.526 gezählte Aufrufe, 125.524 Bedingungen**.
Viele Bedingungen sind wiederholte Tabellenprüfungen, keine unabhängigen Fälle.

## Weitere Erkenntnis zum Sockel-Speicherweg

Die nur lesende Analyse findet `sql->updateItemSocket` im Helper `0x2aefef0`
(Länge `0x1d6`, SHA-256
`0b47ab9486b4a2f8de6a5efe1b5ff3146c420ac2cf3cabe36f169998b1178490`).
Die belegten Aufrufer `0x2ae91a0` und `0x2aea3a0` übergeben dort eine 32-Bit-
ItemInfo-Kennung und den Sockelindex. Die Haltbarkeit steht separat im temporären
24-Byte-Datensatz bei `+0x10`; die SQL-Übergabe liest dessen Kennung bei `+0x14`.
Anschließend setzt `0x240e310` Key und Haltbarkeit im 6-Byte-Sockeldatensatz.

Dieser SQL-Helper ist damit **kein belegter Speicheraufruf für Sockelhaltbarkeit**.
Er wird nicht als vermeintliche Reparatur eingebunden. Daraus folgt noch keine
Aussage, ob oder wo andere Speicherwege Sockelhaltbarkeit sichern bzw. beim
Laden neu initialisieren. Das muss anhand der tatsächlichen Schreib-/Lesepfade
weiter belegt werden; Save-Dateien werden dafür nicht geöffnet.

## Offen

Konkrete serverseitige Inventarmeldungen und Persistenz, vollständige Sockel-
Persistenz-/Ladesemantik, echte UI-/Effekt-/Transportverarbeitung und zugeordnete
Auftragsbelege. Danach bleiben Spielhost/Thread/TLS/Weltwechsel, Eingabe, Loader
und B0-Einbau. Allgemeine NPC-Respawnzeiten bleiben ebenfalls ein Restpunkt von Phase 5.

[Native Inventarproben](tests/inventory_events_native.inl),
[Phasenstand](../../PHASENSTATUS.md), [Testcheckliste](../../TESTCHECKLISTE.md).
Nachweise: `.local/repair-runtime-v17-{build-test.log,native-result.json,validation.json}`.
Recherche: `.local/repair-runtime-v17-{rtti,socket-references,callers}.json`.
