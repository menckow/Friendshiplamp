# 🌟 Freundschaftslampe — Anwenderdokumentation

**Version 2.2** · Stand: 2026-05

Diese Lampe verbindet dich über das Internet mit deinen Liebsten. Wenn du sie berührst und auf den Senden‑Knopf drückst, leuchten die verbundenen Lampen weltweit in deiner persönlichen Farbe auf — und umgekehrt.

---

## Inhalt

1. [Lieferumfang & Hardware-Übersicht](#1-lieferumfang--hardware-übersicht)
2. [Statusfarben (was die Lampe dir sagt)](#2-statusfarben-was-die-lampe-dir-sagt)
3. [Erstinbetriebnahme](#3-erstinbetriebnahme-schritt-für-schritt)
4. [Bedienung im Alltag](#4-bedienung-im-alltag)
5. [Konfigurations-Referenz (Webinterface)](#5-konfigurations-referenz-webinterface)
6. [Lichteffekte](#6-lichteffekte)
7. [Ruhemodus / Nicht stören](#7-ruhemodus--nicht-stören)
8. [Sicherheit (Admin-Passwort & TLS)](#8-sicherheit-admin-passwort--tls)
9. [Dashboard & OTA-Updates](#9-dashboard--ota-updates)
10. [Werkseinstellungen / Reset](#10-werkseinstellungen--reset)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Lieferumfang & Hardware-Übersicht

Eine vollständige Freundschaftslampe besteht aus:

| Komponente | Beschreibung |
|---|---|
| **ESP32 Dev Kit C** | Der "Kopf" der Lampe — WLAN-fähiger Mikrocontroller |
| **WS2812B / NeoPixel Ring** | Der Leuchtring (typisch 16, 24 oder 40 LEDs) |
| **Touch-Pad** | Eine kapazitive Fläche oder ein Draht an GPIO 32 |
| **Drehregler (Potentiometer)** | Für die lokale Farbauswahl |
| **Taster** | Zum Senden eines Signals |
| **Netzteil** | 5 V, mindestens 2 A empfohlen |
| **3D-gedrucktes Gehäuse** | siehe `lamp/Freundschaftslampe.stl` |

### Standard-Pinbelegung

| Funktion | ESP32-Pin |
|---|---|
| NeoPixel Daten (DIN) | GPIO 13 |
| Potentiometer (Signal) | GPIO 34 |
| Senden-Taster | GPIO 4 |
| Touch-Sensor | GPIO 32 |

> Hinweis Hardware: Für eine stabile Versorgung empfiehlt sich ein **Stützkondensator (≥ 470 µF)** parallel zur 5‑V‑Versorgung des LED‑Rings und ein **Vorwiderstand (~330–470 Ω)** seriell in der Datenleitung zwischen ESP32 und dem ersten Pixel. Beides ist im aktuellen Aufbau bereits vorgesehen.

---

## 2. Statusfarben (was die Lampe dir sagt)

| Lichtmuster | Bedeutung |
|---|---|
| 🟣 **Violett/Lila pulsierend** | Setup‑Modus: kein WLAN konfiguriert oder Heim‑WLAN nicht erreichbar |
| ⚪ **Kurz aufleuchten beim Start** | Boot abgeschlossen, Lampe startet ihre Dienste |
| 🎨 **Farbpuls in fremder Farbe** | Du hast ein Signal von einer befreundeten Lampe empfangen |
| 💡 **Konstantes Leuchten in deiner Farbe** | Normalbetrieb, Lampe ist eingeschaltet |
| 🚫 **Komplett dunkel** | Lampe ist über Touch ausgeschaltet ODER Ruhemodus aktiv |
| 🟢 **Fortschrittsbalken um den Ring** | Firmware‑Update (OTA) läuft — bitte nicht ausschalten |

---

## 3. Erstinbetriebnahme (Schritt für Schritt)

### 3.1 Strom anschließen

1. Verbinde die Lampe per USB‑Kabel mit dem mitgelieferten 5‑V‑Netzteil.
2. Nach ca. 5 Sekunden sollte die Lampe **violett** pulsieren — das ist der Setup‑Modus.

### 3.2 Mit der Lampe verbinden

1. Öffne auf deinem Smartphone oder Computer die **WLAN‑Einstellungen**.
2. Wähle das Netzwerk **`Freundschaftslampe-Setup`**.
3. Gib das Passwort ein: **`12345678`**
4. Normalerweise öffnet sich automatisch ein Konfigurationsfenster (Captive Portal).
   Falls nicht: Browser öffnen und **`http://192.168.4.1`** aufrufen.

### 3.3 Heim-WLAN eintragen

1. Im Abschnitt **WLAN‑Einstellungen** auf „Verfügbare WLANs" klicken — die Lampe scannt automatisch die Umgebung.
2. Dein Heim‑WLAN auswählen oder die SSID manuell eintragen.
3. WLAN‑Passwort eingeben.

### 3.4 MQTT-Broker eintragen

MQTT ist der Nachrichtendienst, über den die Lampen miteinander sprechen. Du brauchst die Zugangsdaten zu einem MQTT‑Broker.

**Empfohlen für Einsteiger:** [HiveMQ Cloud](https://www.hivemq.com/) — kostenlos für kleine Mengen, TLS standardmäßig aktiv.

| Feld | Beispiel | Bemerkung |
|---|---|---|
| MQTT Broker | `xxx.s2.eu.hivemq.cloud` | Hostname ohne `https://` |
| MQTT Port | `8883` | bei TLS, sonst `1883` |
| MQTT mit TLS/SSL | ✅ aktiviert | Empfohlen |
| Standard Root‑CA nutzen | ✅ aktiviert | Funktioniert mit HiveMQ und allen Let's‑Encrypt‑Brokern |
| MQTT Client ID | `Tom-Wohnzimmer` | Eindeutiger Name dieser Lampe |
| MQTT Topic | `freundschaftslampe/familie-meier` | Gleiches Topic für alle Lampen, die einander erreichen sollen |
| Benutzername / Passwort | (deine HiveMQ-Credentials) | |

> Wichtig: **Alle Lampen, die sich gegenseitig anleuchten sollen, müssen dasselbe Topic verwenden.**

### 3.5 Identität festlegen

Im Abschnitt **Lampen‑Einstellungen**:

* **Anzahl LEDs** auf den realen Wert deines Rings setzen (z. B. `16`, `24` oder `40`).
* **Touch‑Empfindlichkeit** vorerst auf `40` lassen (Standardwert).
* **Identitätsfarbe** wählen — diese Farbe sehen deine Freunde, wenn du ein Signal sendest.
* **Lichteffekt** wählen (siehe Kapitel 6).
* **Anzeigedauer** in Sekunden setzen (Standard: 10 s).

### 3.6 Admin-Passwort setzen

Im obersten Abschnitt **Administrator** trägst du ein **Webinterface-Passwort** ein. Dieses brauchst du später, wenn du die Lampe von deinem Heim‑WLAN aus konfigurieren willst. Lässt du das Feld leer (oder schreibst `none`), ist das Webinterface ohne Passwort erreichbar.

### 3.7 Speichern & Neustarten

Klicke unten auf **Speichern & Neustarten**.

* Die Lampe trennt das Setup‑Netzwerk und startet neu.
* Nach erfolgreichem Verbinden ist das violette Pulsieren weg.
* Sie ist nun bereit. 🎉

---

## 4. Bedienung im Alltag

### 👆 Touch-Sensor — Ein/Aus & Helligkeit

| Geste | Funktion |
|---|---|
| **Kurz tippen** (< 1 s) | Lampe **ein/aus** schalten |
| **Lang halten** (> 1 s) | **Helligkeit** stufenlos dimmen. Loslassen, wenn die Wunschhelligkeit erreicht ist. Bei der nächsten langen Berührung läuft die Richtung umgekehrt. |

> Spürt die Lampe deine Berührung nicht oder löst sie von allein aus? Siehe Abschnitt [Touch-Empfindlichkeit kalibrieren](#touch-empfindlichkeit-kalibrieren).

### 🎛️ Drehregler — Lokale Farbe

* Solange die Lampe **an** ist, kannst du am Drehregler deine *lokale* Leuchtfarbe ändern.
* Diese Farbe wird automatisch als deine **Identitätsfarbe** verwendet, wenn du das nächste Mal ein Signal sendest.

### 🔘 Senden-Taster — Gruß verschicken

* Einmal kurz drücken → die Lampe schickt ein Signal mit deiner aktuellen Farbe, dem gewählten Effekt und der eingestellten Dauer über MQTT raus.
* Alle Lampen, die dasselbe Topic abonniert haben, leuchten in deiner Farbe auf.

### 💌 Signal empfangen

Wenn jemand seine Lampe drückt:

1. Deine Lampe **unterbricht ihren aktuellen Zustand** und leuchtet in der Farbe des Senders.
2. Der eingestellte Effekt (Fade, Komet, …) läuft für die vom Sender festgelegte Dauer (Standard: 10 s).
3. Danach **kehrt deine Lampe automatisch in den vorherigen Zustand zurück** (aus = bleibt aus, an = leuchtet wieder in deiner Farbe).

---

## 5. Konfigurations-Referenz (Webinterface)

Erreichbar im Heim‑WLAN unter der IP, die dein Router der Lampe zugewiesen hat (z. B. `http://192.168.1.42`). Login: `admin` + dein Webinterface‑Passwort.

### Administrator

| Feld | Bedeutung |
|---|---|
| **Webinterface Passwort** | Schützt das Konfigurationsmenü. Leer oder `none` = kein Passwort. |

### WLAN-Einstellungen

| Feld | Bedeutung |
|---|---|
| **Verfügbare WLANs** | Live‑Scan; Auswahl füllt SSID automatisch aus |
| **WLAN SSID** | Name deines Heim‑WLANs |
| **WLAN Passwort** | Dazugehöriges Passwort |

### MQTT-Einstellungen

| Feld | Bedeutung |
|---|---|
| **MQTT Broker** | Hostname des Brokers |
| **MQTT Port** | `1883` (unverschlüsselt) oder `8883` (TLS) |
| **MQTT mit TLS/SSL** | Verschlüsselt aktivieren |
| **Standard Root‑CA nutzen** | ISRG Root X1 ist eingebaut → funktioniert mit HiveMQ & Let's Encrypt |
| **Eigenes CA Zertifikat** | Nur falls dein Broker ein anderes CA verwendet |
| **MQTT Client ID** | Eindeutige Kennung dieser Lampe |
| **MQTT Topic** | Gemeinsames Topic aller verbundenen Lampen |
| **Benutzername / Passwort** | Zugangsdaten zum Broker |

### Lampen-Einstellungen

| Feld | Bedeutung |
|---|---|
| **Anzahl LEDs** | Anzahl der WS2812B-Pixel im Ring |
| **Touch-Empfindlichkeit** | Schwellenwert (1–100). **Niedriger = unempfindlicher** (eher gegen Fehlauslösungen) |
| **Identitätsfarbe** | Deine persönliche Farbe |
| **Lichteffekt** | Animation (siehe Kapitel 6) |
| **Anzeigedauer** | Wie lange ein empfangenes Signal leuchtet (Sekunden) |

### Ruhemodus

| Feld | Bedeutung |
|---|---|
| **Aktivieren** | Eingehende Signale werden in einem Zeitfenster ignoriert |
| **Startzeit / Endzeit** | Stunde (0–23). Zeiträume über Mitternacht (z. B. 22 → 6) werden korrekt erkannt. |

---

## 6. Lichteffekte

| Effekt | Beschreibung |
|---|---|
| **Fade** | Sanftes Ein- und Ausblenden — der Klassiker |
| **Color Wipe** | Die Farbe „wischt" sich LED für LED um den Ring auf |
| **Theater Chase** | Wandernder Lauflicht‑Effekt |
| **Rainbow** | Volles Farbspektrum als Lauflicht (ignoriert deine Identitätsfarbe) |
| **Breathe** | Ruhiges, gleichmäßiges Pulsieren |
| **Feuer** | Dynamisches Flackern in deiner Farbe |
| **Komet** | Heller Punkt mit Schweif zieht um den Ring |

---

## 7. Ruhemodus / Nicht stören

Damit dich die Lampe nachts nicht weckt:

1. Im Webinterface den **Ruhemodus aktivieren**.
2. Start- und Endzeit setzen, z. B. **22 → 06**.
3. In diesem Zeitraum werden eingehende Signale **stillschweigend verworfen** — die Lampe bleibt komplett dunkel.

> Hinweis: Der Ruhemodus benötigt die per NTP synchronisierte Uhrzeit. Direkt nach Stromanschluss (Sekunden) ist diese noch nicht verfügbar — in dieser Phase werden Signale ohnehin sicherheitshalber ignoriert.

---

## 8. Sicherheit (Admin-Passwort & TLS)

* **Admin‑Passwort:** Im Heim‑WLAN ist das Webinterface durch HTTP‑Basic‑Auth geschützt. Setze ein starkes Passwort, sobald die Lampe online ist.
* **MQTT‑TLS:** Wenn deine Lampe über das Internet kommuniziert (z. B. HiveMQ Cloud), **immer TLS aktivieren**. Andernfalls könnten Dritte deine Signale mitlesen oder fälschen.
* **Topic:** Wähle ein nicht zu erratendes Topic — z. B. `freundschaftslampe/familie-meier-7c4f`, nicht nur `lampe`.

---

## 9. Dashboard & OTA-Updates

Wenn du mehrere Lampen verwaltest, gibt es ein zentrales Dashboard:

* **Datei:** `manager/dashboard_secure.html` lokal im Browser öffnen.
* **Live‑Status:** Alle Lampen werden mit Farbe, Firmware‑Version und Online/Offline angezeigt (Erkennung dank LWT).
* **OTA‑Updates:** Neue Firmware kann zentral per MQTT‑Befehl auf alle Lampen verteilt werden. Während des Updates läuft ein grüner Fortschrittsbalken um den Ring. Die Lampe prüft die MD5‑Summe der neuen Firmware — bei Fehler springt sie automatisch auf die alte Version zurück (Rollback‑Schutz).

---

## 10. Werkseinstellungen / Reset

Falls du WLAN-Daten oder das Admin‑Passwort vergessen hast:

**Variante A — über das Webinterface (wenn noch erreichbar):**
1. Felder für SSID + Passwort leeren, speichern.
2. Nach Neustart pulsiert die Lampe wieder violett (Setup‑Modus).

**Variante B — Firmware komplett neu flashen:**
1. Lampe per USB anschließen.
2. In VS Code mit PlatformIO: **Erase Flash** ausführen, dann neu Build & Upload.
3. Beim nächsten Start ist die Lampe wieder im Setup‑Modus.

---

## 11. Troubleshooting

### Lampe pulsiert dauerhaft violett

→ Sie kann das konfigurierte WLAN nicht erreichen. Prüfen:
* Stimmt die SSID exakt (Groß-/Kleinschreibung)?
* Ist das WLAN ein 2,4‑GHz‑Netz? **Der ESP32 unterstützt kein 5 GHz.**
* Ist die Signalstärke am Standort ausreichend?

### Lampe verbindet WLAN, aber empfängt keine Signale

→ MQTT‑Problem. Prüfen:
* Sind alle Lampen mit demselben **Topic** verbunden?
* Bei TLS: ist „Standard Root‑CA" aktiviert?
* Stimmen Benutzername/Passwort des Brokers?
* Sind die Lampen im Dashboard als `online` gelistet?

### Lampe geht zufällig an/aus oder flackert

Mögliche Ursachen, in der Reihenfolge der Wahrscheinlichkeit:

1. **Touch zu empfindlich** → Im Webinterface den Wert **Touch‑Empfindlichkeit** *verringern* (z. B. von 40 auf 25). Das macht den Sensor unempfindlicher gegen Störungen.
2. **Netzteil zu schwach / Datenleitung gestört** → Stützkondensator (≥ 470 µF) parallel zur 5‑V‑Versorgung des Rings und Vorwiderstand (~330 Ω) seriell in der NeoPixel‑Datenleitung einbauen.
3. **Direkt nach Stromanschluss** kurzes Aufleuchten in fremder Farbe → Das war eine *retained MQTT‑Message*; die aktuelle Firmware ignoriert diese, bis die Uhrzeit per NTP synchronisiert ist (wenige Sekunden).
4. **Drehregler verursacht Farb‑Flackern** → Mechanisch fest setzen und Lampe gegen Vibration sichern.

### Touch-Empfindlichkeit kalibrieren

* **Wert zu hoch** (z. B. 80): Lampe reagiert schon bei Annäherung oder von allein → **Wert senken** (z. B. 25–30).
* **Wert zu niedrig** (z. B. 10): Lampe reagiert nicht oder nur sehr verzögert → **Wert leicht erhöhen** (z. B. 35–45).
* Nach jeder Änderung das Webinterface speichern; die Lampe übernimmt den Wert ohne Neustart.

### LED-Ring zeigt falsche Anzahl oder bleibt teilweise dunkel

→ **Anzahl LEDs** im Webinterface muss exakt der Anzahl im Ring entsprechen.

### Webinterface erfragt Passwort, ich kenne es nicht

→ Siehe [Werkseinstellungen / Reset](#10-werkseinstellungen--reset).

### Firmware-Update bricht ab

→ Lampe startet automatisch mit alter Firmware neu (Rollback). Erneut über das Dashboard auslösen. Bleibt der Fehler: WLAN‑Signal am Lampen‑Standort prüfen.

### Lampe friert ein / reagiert nicht mehr

→ Kurz vom Strom trennen (5 Sek warten) und wieder anschließen. Wenn das Problem regelmäßig auftritt: Versorgung mit einem stärkeren Netzteil (≥ 2 A) und stabiler Verkabelung prüfen.

---

## 📄 Lizenz & Support

Entwickelt für die Community. Quelltext und Issues unter dem Projekt‑Repository. Viel Freude und herzliche Grüße ✨
