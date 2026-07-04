# Manager — Browser-basierte Tools

In diesem Verzeichnis liegen zwei eigenständige HTML-Werkzeuge, die direkt im Browser laufen — kein Webserver, keine Installation. Beide Dateien öffnest Du per Doppelklick oder hostest sie statisch (z.B. GitHub Pages).

| Datei | Zweck |
|---|---|
| [`dashboard_secure.html`](dashboard_secure.html) | **Device Manager Dashboard** — zentrale Übersicht aller Lampen + Boxen, gruppiert nach Familienkreis, mit OTA-Steuerung. |
| [`Friendshiplamp_Web_App_V4_Final.html`](Friendshiplamp_Web_App_V4_Final.html) | Standalone-Remote für eine einzelne Lampe — Farbe und Effekt manuell auslösen. |

---

## 📊 Device Manager Dashboard

`dashboard_secure.html` ist die Leitstelle für alle Geräte in Deiner MQTT-Topologie. Es verbindet sich per **MQTT-over-WebSockets (TLS)** mit Deinem Broker und abonniert sowohl das alte v1-Schema als auch das neue v2-Familien-Schema parallel — Du siehst also Bestand und migrierte Geräte gleichzeitig.

### Login

Beim Öffnen erscheint eine Login-Maske. Trage ein:

* **MQTT Broker** — z.B. `xxx.s1.eu.hivemq.cloud` (ohne `wss://`, ohne Trailing-Slash)
* **Port** — `8884` für HiveMQ Cloud WebSockets (`8883` wird automatisch zu `8884` korrigiert, weil Browser TCP-MQTT nicht direkt sprechen können)
* **Username / Passwort** — die MQTT-Credentials Deines Brokers

Broker, User und Port werden für den nächsten Besuch im `localStorage` gespeichert. Das **Passwort nicht** — es bleibt nur in der Sitzung.

### Geräteliste

Sobald die Verbindung steht, abonniert das Dashboard:

* `+/status/#` (altes v1-Schema, z.B. `freundschaftslampe/status/<id>` und `zwitscherbox/status/<id>`)
* `fl/device/+/status` (v2-Schema mit JSON-Payload)
* `fl/device/+/update/status` (v2-OTA-Backchannel)

Jede empfangene Status-Nachricht wird zur Tabelle zugefügt oder aktualisiert. Geräte erscheinen **gruppiert nach Familienkreis** — v2-Geräte landen in der Gruppe ihrer im Payload genannten `families`, mehrfach falls in mehreren Kreisen. Geräte ohne Familienzuordnung (alle v1-Geräte und v2-Geräte ohne `families[]`) landen in der Sammelgruppe **„Bestand (altes Schema, ohne Familienzuordnung)"**.

Jede Gerätezeile zeigt:

| Spalte | Bedeutung |
|---|---|
| ID | Identitätsfarbe als Kreis + Client-ID + Schema-Pille (grün `v2` oder gelb `v1`) |
| Typ | `Lampe`, `Box (V6)`, `Box (A1S)` oder `Box (v1)`. Die beiden Box-Varianten brauchen unterschiedliche Firmware-Binaries — der Typ ergibt sich aus dem `type`-Feld der Status-JSON (`lamp`, `box`, `box-a1s`). |
| Version | Firmware-Version aus `fw` (v2) bzw. erstem Feld des `:`-Payload (v1) |
| Status | `online` / `offline` / Update-Status (gelb hinterlegt wenn `update` im Text) |
| Aktion | **Update**-Button für Einzelgerät |

### OTA-Karten

Im Grid unter der Tabelle gibt es vier Update-Karten:

| Karte | Zielgruppe | Topic |
|---|---|---|
| **Update: Lampen (v1)** | Alle v1-Lampen | `freundschaftslampe/update/trigger` |
| **Update: Zwitscherboxen (v1)** | Alle v1-Boxen | `zwitscherbox/update/trigger` |
| **Update: Pro Familie (v2)** | Lampen, V6-Boxen oder A1S-Boxen einer Familie | `fl/family/<id>/update/trigger/<type>` |
| **Update: Globaler Notfall-Push (v2)** | Alle Lampen, V6-Boxen oder A1S-Boxen | `fl/_global/update/trigger/<type>` |

`<type>` ist `lamp`, `box` (V6) oder `box-a1s` (A1S). Die drei Geräte-Typen brauchen unterschiedliche Firmware-Images — deshalb gibt es bewusst kein „Alles auf einmal", sondern einen Typ-Selektor in jeder Karte. Das Familien-Dropdown füllt sich automatisch aus den gesehenen Geräten — also: wenn keine Familie in der Liste, hat noch keine v2-Box/-Lampe sich gemeldet.

Jeder Update-Vorgang fragt vorher per Modal um Bestätigung und zeigt das tatsächlich verwendete Topic an.

### Einzel-Update

Klick auf **Update** in einer Gerätezeile öffnet die Bestätigung. Das Topic wird schema-bewusst gewählt:

* **v2-Geräte** → `fl/device/<id>/update/trigger`
* **v1-Lampen** → `freundschaftslampe/update/trigger/<id>` (Per-Device-Topic, das die migrierte Lampen-FW seit der ersten Phase abonniert)
* **v1-Boxen** → `zwitscherbox/update/trigger` mit `target: <id>` im Payload (Broadcast-Filter, der seit jeher auch von alter Box-FW respektiert wird — daher universell kompatibel)

Die URL, Version und MD5 werden aus der passenden v1-Karte oder den Familien-/Global-Feldern gezogen.

### Payload-Format

Alle OTA-Triggers verwenden das gleiche JSON:

```json
{
  "url": "https://your-server.com/firmware.bin",
  "version": "V2.3.1",
  "md5": "b3e3e3b3e3e3b3e3e3b3e3e3b3e3e3b3"
}
```

Bei v1-Box-Einzel-Updates kommt zusätzlich `"target": "<deviceId>"` dazu, damit nur die adressierte Box updatet. Bei v2-Topics nicht nötig — das Topic adressiert direkt.

### Sicherheits-Tipp

Das Dashboard ist eine reine Client-HTML — alle Credentials und Befehle laufen über Deinen Browser zum MQTT-Broker. Wenn Du das auf einem öffentlich erreichbaren Server hostest:

* Stelle sicher, dass nur autorisierte Personen Zugriff haben (z.B. via Reverse-Proxy mit Basic Auth).
* Nutze einen MQTT-User mit ACLs, die nur auf die nötigen Topics zugreifen können.
* Der WebSocket geht über `wss://` (TLS) — der Broker muss das unterstützen (HiveMQ Cloud tut das).

---

## 💡 Standalone Lamp Remote

`Friendshiplamp_Web_App_V4_Final.html` ist ein simples Remote-Frontend für eine **einzelne** Lampe. Du wählst Farbe, Effekt und Dauer, und das Tool publisht einen einzelnen Signal-Payload an das Signal-Topic. Nutzbar als „Lampe ohne Button" — z.B. wenn Du jemandem aus der Ferne ein bestimmtes Licht-Signal schicken willst.

Hinweis: Dieses Werkzeug stammt aus der v1-Ära und schreibt aktuell ans alte `mqttTopic`. Für v2 müsste es entsprechend auf `fl/family/<id>/signal` umgestellt werden — das ist noch nicht passiert. Wenn Du das brauchst, sag Bescheid, dann ziehen wir das Tool ebenfalls nach.

---

## Lokal öffnen vs. hosten

Beide HTML-Dateien sind self-contained (CDN-Abhängigkeit: `mqtt.js` für WebSocket-MQTT). Du kannst sie:

* **Doppelklicken** und im Browser direkt benutzen — funktioniert für die meisten modernen Browser.
* **Statisch hosten** (GitHub Pages, Caddy, Nginx, ein S3-Bucket usw.) — empfohlen, wenn Du sie öfter brauchst, weil sich Bookmarks/`localStorage` so sauber zuordnen lassen.

Kein Build-Schritt nötig.
