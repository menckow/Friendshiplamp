# Friendshiplamp
![Freundschaftslampe](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)
Dieses Projekt ist eine "Freundschaftslampe", die über WLAN und MQTT mit anderen Lampen verbunden ist. Berührt man eine Lampe, leuchten alle anderen Lampen in der gleichen Farbe auf.

## Neue Funktionen (V2.0)
*   **Erweiterte Lichteffekte**: Wähle aus verschiedenen Effekten wie *Fade, Color Wipe, Theater Chase, Rainbow Cycle, Breathe, Feuer-Effekt* und *Komet*.
*   **Anzeigedauer**: Die Dauer, wie lange eine empfangene Farbe angezeigt wird, lässt sich nun im Webinterface einstellen.
*   **WLAN-Scan**: Die Weboberfläche sucht jetzt automatisch nach verfügbaren WLANs in der Umgebung.
*   **Optimierte Farbtreue**: Dank integrierter Gamma-Korrektur werden Farben (wie z.B. Lila/Violett) deutlich realistischer und satter auf den LEDs dargestellt.

## Funktionalität

*   **Web-basierte Konfiguration**: Nach dem ersten Start spannt die Lampe einen eigenen Access-Point (AP) mit dem Namen "Freundschaftslampe-Setup" auf.
*   **Sicheres Webinterface**: Die Weboberfläche ist mit einem Passwort geschützt (Standard: `12345678`).
*   **WLAN & MQTT-Konfiguration**: Unterstützung für MQTT mit TLS/SSL und CA-Zertifikaten.
*   **Dynamische LED-Anzahl**: Die exakte Anzahl der NeoPixel lässt sich im Webinterface einstellen.
*   **Helligkeitssteuerung**: Die Helligkeit kann direkt am Gerät (Langer Druck auf Touch) eingestellt werden und wird auch bei den Effekten berücksichtigt.
*   **Ein-/Ausschalten**: Per kurzem Touch steuerbar.
*   **Ruhemodus**: Zeitgesteuerte Deaktivierung für die Nacht.

## Hardware

*   ESP32 Dev Kit C (oder ESP32-S3)
*   Adafruit NeoPixel Ring / Strip
*   Potentiometer (zur Farbwahl im Normalbetrieb)
*   Taster (zum Senden der Identitätsfarbe)
*   Kapazitiver Touch-Sensor (Ein/Aus & Helligkeit)

## Verkabelung

| Bauteil             | Pin am ESP32 |
| ------------------- | ------------ |
| Potentiometer (Signal) | GPIO 34      |
| Taster              | GPIO 4       |
| NeoPixel Ring (DIN) | GPIO 13      |
| Touch-Sensor (Zustand)| GPIO 32      |

---

# Friendship Lamp (English)

This project is a "Friendship Lamp" connected to other lamps via Wi-Fi and MQTT. When you send a signal, all connected lamps light up in your personal identity color.

## New Features (V2.0)
*   **Advanced Lighting Effects**: Choose from various effects including *Fade, Color Wipe, Theater Chase, Rainbow Cycle, Breathe, Fire Effect,* and *Comet*.
*   **Custom Duration**: The display time for received colors is now configurable via the web interface.
*   **Wi-Fi Scanning**: The setup portal now automatically scans for available local Wi-Fi networks.
*   **Enhanced Color Accuracy**: Integrated Gamma correction ensures that colors (especially tricky ones like purple/violet) look vibrant and true-to-life on the LEDs.

## Functionality

*   **Web-based Configuration**: Captive portal for easy setup via the "Freundschaftslampe-Setup" Wi-Fi.
*   **Secure Web-UI**: Password protected access (Default: `12345678`).
*   **Wi-Fi & MQTT**: Full support for standard and secure (TLS) MQTT brokers.
*   **Dynamic LED Count**: Configure the number of LEDs directly in the UI.
*   **Brightness Control**: Adjustable via long-press on the touch sensor; settings are applied to all effects.
*   **Quiet Mode**: Schedule "Do Not Disturb" hours for nighttime.

## Setup

1.  **Flash Firmware**: Use PlatformIO to upload the code to your ESP32.
2.  **Config Mode**: If no Wi-Fi is found, the lamp lights up purple/violet and opens an Access Point.
3.  **Configure**: Connect to "Freundschaftslampe-Setup" and go to `http://192.168.4.1`.
4.  **Personalize**: Set your identity color, select your favorite effect, and save!

---

# User Manual / Bedienungsanleitung

## 1. Einrichten (Setup)
Verbinde dich mit dem WLAN **"Freundschaftslampe-Setup"** (Passwort: `12345678`). Dein Browser sollte automatisch die Konfigurationsseite öffnen. Dort kannst du dein Heim-WLAN auswählen und deine Lampe individualisieren.

## 2. Bedienung (Operation)
*   **💡 Ein/Aus**: Kurzer Touch auf den Sensor.
*   **☀️ Helligkeit**: Langer Touch auf den Sensor. Die Lampe regelt die Helligkeit hoch/runter. Loslassen, wenn es passt.
*   **🎨 Farbwahl**: Drehe am Potentiometer, um die lokale Farbe der Lampe zu ändern.
*   **💌 Senden**: Drücke den Taster. Deine Lampe sendet nun deine eingestellte Farbe und deinen gewählten Effekt an deine Freunde.

## 3. Effekte (Effects)
Du kannst in den Einstellungen aus verschiedenen Animationen wählen. Der **Feuer-Effekt** flackert in deiner Farbe, während der **Komet** einen Lichtschweif um den Ring zieht. Der **Regenbogen** ist eine Ausnahme und zeigt immer das volle Farbspektrum.
