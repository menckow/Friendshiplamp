# Friendship Lamp

![Friendship Lamp](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)

This project is a "Friendship Lamp" connected to other lamps via Wi-Fi and MQTT. When you send a signal, all connected lamps light up in your personal identity color.

## New Features (V2.1)
*   **Secure OTA Update**: Update your firmware over-the-air via MQTT. Downloads are handled via HTTPS (GitHub/GitLab are automatically supported, no manual certificate setup required).
*   **Advanced Lighting Effects**: Choose from various effects including *Fade, Color Wipe, Theater Chase, Rainbow Cycle, Breathe, Fire Effect,* and *Comet*.
*   **Custom Duration**: The display time for received colors is now configurable.
*   **Wi-Fi Scanning**: The setup portal automatically scans for available local Wi-Fi networks.
*   **Enhanced Color Accuracy**: Integrated Gamma correction for vibrant and true-to-life colors.
*   **Touch Sensitivity Control**: Configure the sensitivity of your touch sensor directly in the web UI.
*   **Memory Optimization**: Use of `PROGMEM` for web templates to prevent heap fragmentation and improve overall stability.

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

## Hardware

*   ESP32 Dev Kit C (or ESP32-S3)
*   Adafruit NeoPixel Ring / Strip
*   Potentiometer (for color selection in normal mode)
*   Button (to send the identity color)
*   Capacitive Touch Sensor (On/Off & Brightness)

## Wiring

| Component           | Pin on ESP32 |
| ------------------- | ------------ |
| Potentiometer (Signal) | GPIO 34      |
| Button              | GPIO 4       |
| NeoPixel Ring (DIN) | GPIO 13      |
| Touch Sensor (State)| GPIO 32      |

## Setup

1.  **Flash Firmware**: Use PlatformIO to upload the code to your ESP32.
2.  **Config Mode**: If no Wi-Fi is found, the lamp lights up purple/violet and opens an Access Point.
3.  **Configure**: Connect to "Freundschaftslampe-Setup" and go to `http://192.168.4.1`.
4.  **Personalize**: Set your identity color, select your favorite effect, and save!

---

# User Manual

## 1. Setup
Connect to the Wi-Fi **"Freundschaftslampe-Setup"** (Password: `12345678`). Your browser should automatically open the configuration page. There you can select your home Wi-Fi and personalize your lamp.

### 2. Bedienung
*   **💡 Ein/Aus**: Den Touch-Sensor kurz berühren.
*   **☀️ Helligkeit**: Den Touch-Sensor lang berühren. Die Lampe ändert stufenlos die Helligkeit.
*   **📉 Empfindlichkeit**: In den Einstellungen kann die Touch-Empfindlichkeit justiert werden. Ein niedrigerer Wert macht den Schalter empfindlicher.
*   **🎨 Farbwahl**: Am Potentiometer drehen, um die lokale Farbe der Lampe zu ändern.
*   **💌 Senden**: Den Taster drücken. Deine Lampe sendet nun deine Farbe und den gewählten Effekt an deine Freunde.

### 3. Effekte
You can choose from various animations in the settings. The **Fire Effect** flickers in your color, while the **Comet** draws a light trail around the ring. The **Rainbow** is an exception and always shows the full color spectrum.
