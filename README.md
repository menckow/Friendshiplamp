# Friendshiplamp
![Freundschaftslampe](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)
Dieses Projekt ist eine "Freundschaftslampe", die über WLAN und MQTT mit anderen Lampen verbunden ist. Drückt man den Knopf, leuchten alle anderen Lampen in der gleichen Farbe auf.

## Neue Funktionen (V2.1)
*   **Sicheres OTA-Update**: Die Firmware kann nun "Over-the-Air" via MQTT aktualisiert werden. Der Download erfolgt über HTTPS (GitHub/GitLab werden automatisch unterstützt, keine manuelle Zertifikatspflege nötig).
*   **Erweiterte Lichteffekte**: Wähle aus verschiedenen Effekten wie *Fade, Color Wipe, Theater Chase, Rainbow Cycle, Breathe, Feuer-Effekt* und *Komet*.
*   **Anzeigedauer**: Die Dauer, wie lange eine empfangene Farbe angezeigt wird, lässt sich nun im Webinterface einstellen.
*   **WLAN-Scan**: Die Weboberfläche sucht jetzt automatisch nach verfügbaren WLANs in der Umgebung.
*   **Optimierte Farbtreue**: Dank integrierter Gamma-Korrektur werden Farben (wie z.B. Lila/Violett) deutlich realistischer und satter auf den LEDs dargestellt.

## Funktionalität

*   **Versions-Reporting**: Die Lampe meldet ihre aktuelle Version beim Start via MQTT.
*   **Rollback-Schutz**: Dank dualer Partitionierung kehrt der ESP32 bei einem fehlgeschlagenen Update automatisch zur letzten funktionierenden Version zurück.
*   **Web-basierte Konfiguration**: Nach dem ersten Start spannt die Lampe einen eigenen Access-Point (AP) mit dem Namen "Freundschaftslampe-Setup" auf.
*   **Sicheres Webinterface**: Die Weboberfläche ist mit einem Passwort geschützt (Standard: `12345678`).
*   **WLAN & MQTT-Konfiguration**: Unterstützung für MQTT mit TLS/SSL und CA-Zertifikaten.
*   **Dynamische LED-Anzahl**: Die exakte Anzahl der NeoPixel lässt sich im Webinterface einstellen.
*   **Helligkeitssteuerung**: Die Helligkeit kann direkt am Gerät (Langer Druck auf Touch) eingestellt werden und wird auch bei den Effekten berücksichtigt.
*   **Ein-/Ausschalten**: Per kurzem Touch steuerbar.
*   **Ruhemodus**: Zeitgesteuerte Deaktivierung für die Nacht.

## 🖥️ Zentrales Management & Dashboard

Dieses Projekt beinhaltet ein modernes Web-Dashboard zur zentralen Verwaltung aller Lampen und Zwitscherboxen.

*   **Device Manager**: Über die Datei `manager/dashboard.html` (einfach im Browser öffnen) erhältst du eine Echtzeit-Übersicht aller Geräte.
*   **Live-Status (LWT)**: Dank der *Last Will and Testament*-Funktion wird ein Gerät im Dashboard sofort als `offline` markiert, wenn es die Verbindung verliert.
*   **Version-Reporting**: Jede Lampe meldet ihre Version beim Start. Dank *Retained Messages* ist diese Information im Dashboard sofort sichtbar, auch wenn die Lampe gerade nicht aktiv sendet.
*   **Status-Topic**: Jedes Gerät nutzt ein individuelles Topic: `freundschaftslampe/status/<ClientID>`.

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

## New Features (V2.1)
*   **Secure OTA Update**: Update your firmware over-the-air via MQTT. Downloads are handled via HTTPS (GitHub/GitLab are automatically supported, no manual certificate setup required).
*   **Advanced Lighting Effects**: Choose from various effects including *Fade, Color Wipe, Theater Chase, Rainbow Cycle, Breathe, Fire Effect,* and *Comet*.
*   **Custom Duration**: The display time for received colors is now configurable.
*   **Wi-Fi Scanning**: The setup portal automatically scans for available local Wi-Fi networks.
*   **Enhanced Color Accuracy**: Integrated Gamma correction for vibrant and true-to-life colors.

## Functionality

*   **Version Reporting**: The lamp reports its current version on startup via MQTT (`freundschaft/update/status`).
*   **Rollback Protection**: Using dual partitions, the ESP32 automatically reverts to the previous working version if an update fails.
*   **Web-based Configuration**: Captive portal for easy setup via the "Freundschaftslampe-Setup" Wi-Fi.
*   **Secure Web-UI**: Password protected access (Default: `12345678`).
*   **Wi-Fi & MQTT**: Full support for standard and secure (TLS) MQTT brokers.
*   **Dynamic LED Count**: Configure the number of LEDs directly in the UI.
*   **Brightness Control**: Adjustable via long-press on the touch sensor.
*   **Quiet Mode**: Schedule "Do Not Disturb" hours for nighttime.

## 🖥️ Central Device Management

This project includes a modern web dashboard for centralized management of all lamps and Zwitscherboxen.

*   **Device Manager**: Simply open `manager/dashboard.html` in your browser for a real-time overview of all devices.
*   **Live Status (LWT)**: Using *Last Will and Testament*, any device that loses its connection is immediately marked as `offline` in the dashboard.
*   **Version Reporting**: Every lamp reports its version on startup. This information is persistently stored on the broker (Retained Messages) and is instantly visible in the dashboard.
*   **Status Topic**: Each device uses a unique status topic: `freundschaftslampe/status/<ClientID>`.

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
