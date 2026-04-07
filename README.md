# Friendshiplamp
![Freundschaftslampe](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)
Dieses Projekt ist eine "Freundschaftslampe", die über WLAN und MQTT mit anderen Lampen verbunden ist. Berührt man eine Lampe, leuchten alle anderen Lampen in der gleichen Farbe auf.

## Funktionalität

*   **Web-basierte Konfiguration**: Nach dem ersten Start spannt die Lampe einen eigenen Access-Point (AP) mit dem Namen "Freundschaftslampe-Setup" auf. Verbinde dich mit diesem WLAN, um die Lampe zu konfigurieren.
*   **WLAN & MQTT-Konfiguration**: Über die Weboberfläche können die Zugangsdaten für das heimische WLAN sowie die Daten für den MQTT-Broker (Server, Port, TLS, Topic, Benutzer, Passwort) und ein CA-Zertifikat für TLS-Verbindungen hinterlegt werden.
*   **Farbauswahl**: Die Farbe der Lampe kann im Normalbetrieb über ein Potentiometer frei gewählt werden.
*   **Ein-/Ausschalten**: Die Lampe kann durch Berühren eines kapazitiven Touch-Sensors ein- und ausgeschaltet werden.
*   **Helligkeitssteuerung**: Die Helligkeit der Lampe kann durch langes Berühren des Ein/Ausschalters eingestellt werden.
*   **Freundschaftssignal Senden**: Durch Drücken eines Tasters wird die aktuell eingestellte "Identitätsfarbe" über MQTT an alle anderen Lampen gesendet.
*   **Freundschaftssignal Empfangen**: Empfängt die Lampe eine Farbe über MQTT, wird diese für eine bestimmte Zeit angezeigt, bevor die Lampe wieder in ihren vorherigen Zustand zurückkehrt.

## Hardware

*   ESP32 Dev Kit C
*   Adafruit NeoPixel Ring (16 LEDs)
*   Potentiometer
*   Taster
*   Kapazitive Touch-Sensoren (können einfache Drähte oder Metallflächen sein)

## Verkabelung

| Bauteil             | Pin am ESP32 |
| ------------------- | ------------ |
| Potentiometer (Signal) | GPIO 34      |
| Taster              | GPIO 4       |
| NeoPixel Ring (DIN) | GPIO 13      |
| Touch-Sensor (Ein/Aus/Helligkeit) | GPIO 32      |

**Wichtiger Hinweis:** Die meisten ESP32-Entwicklungsboards haben Pins, die nur als Eingänge verwendet werden können (z.B. GPIO 34-39). GPIO 34 ist ein solcher Pin und eignet sich gut für das Potentiometer.

## Setup

1.  **Flashen der Firmware**: Kompiliere und lade die Firmware mit PlatformIO auf den ESP32.
2.  **Konfigurationsmodus**: Nach dem ersten Start oder wenn keine WLAN-Daten hinterlegt sind, startet die Lampe im Access-Point-Modus.
    *   Suche nach einem WLAN mit der SSID "Freundschaftslampe-Setup" und verbinde dich damit.
    *   Nach der Verbindung öffnet sich normalerweise automatisch ein Captive-Portal mit der Konfigurationsseite. Falls nicht, öffne einen Browser und gehe auf eine beliebige Webseite (z.B. `http://192.168.4.1`).
3.  **Konfiguration**: Gib auf der Webseite deine WLAN-Zugangsdaten und die deines MQTT-Brokers ein.
    *   Wenn dein MQTT-Broker TLS verwendet, aktiviere die Checkbox und füge bei Bedarf das CA-Zertifikat deines Brokers ein.
    *   Wähle deine persönliche "Identitätsfarbe". Diese Farbe wird gesendet, wenn du den Taster drückst.
4.  **Neustart**: Speichere die Konfiguration und starte die Lampe neu. Sie sollte sich nun mit deinem WLAN und dem MQTT-Broker verbinden.

# User Manual: Friendship Lamp

Welcome to your new **Friendship Lamp**! This lamp allows you to stay connected with your loved ones over long distances by sending each other colorful light greetings.

Here is how you set up and operate your lamp.

---

## 1. Initial Setup (Wi-Fi Connection)

For the lamp to send and receive greetings, it needs to be connected to your Wi-Fi network.

1. **Recognizing Setup Mode:** If the lamp **lights up purple** when turned on, it couldn't find an existing Wi-Fi connection. It is now in setup mode (Access Point mode).
2. **Connecting to the Lamp:** Use a smartphone, tablet, or laptop and search for available Wi-Fi networks in your area. Connect to the network named **"Freundschaftslampe-Setup"**.
3. **Opening the Configuration Page:** Usually, a page opens automatically (the so-called Captive Portal). If this doesn't happen, simply open your web browser and enter any web address. You will now be redirected to the lamp's settings page.
4. **Entering Data:**
   - Under **WLAN-Einstellungen** (Wi-Fi Settings), enter the name (SSID) and password of your **home Wi-Fi**.
   - Under **MQTT-Einstellungen** (MQTT Settings), enter the connection details for the communication server (broker).
   - Under **Lampen-Einstellungen** (Lamp Settings), choose your personal **identity color** (this is the color that will light up on your partner's lamp when you send a greeting).
5. **Saving:** Click on **"Speichern & Neustarten"** (Save & Restart). The lamp will now restart and automatically connect to your home Wi-Fi. The purple light should now disappear.

---

## 2. Operating the Lamp

### 💡 Turning the Lamp On and Off
- **Short Touch:** Briefly tap the touch sensor to turn the lamp on or off.

### ☀️ Adjusting Brightness
- **Long Touch:** Keep your finger on the touch sensor to smoothly adjust the brightness. Release as soon as the light is bright or dim enough. With the next long press, the brightness will change in the opposite direction.

### 🎨 Customizing Your Local Color
- **Turning the Potentiometer (Knob):** When the lamp is turned on, you can turn the knob to set the local color of the light ring exactly to your liking.

### 💌 Sending a Colorful Greeting
- **Pressing the Button:** Press the button on the lamp once briefly. Your lamp will now send your previously set *identity color* over the internet. The connected lamp of your counterpart will light up that very second!

---

## 3. Receiving a Greeting

When someone presses the button on their lamp, the following happens on yours:
- Your lamp interrupts its current light and immediately lights up in the sender's color for **10 seconds**.
- After the 10 seconds have elapsed, the received light disappears again. The lamp completely automatically returns to the color and state (On or Off) it had before.

---

## 4. Quiet Mode (Do Not Disturb)

To ensure you aren't awakened at night by unexpectedly lighting up greetings, the lamp features a time control.

- You can enable the **Quiet Mode** (Ruhemodus) on the lamp's Wi-Fi setup page (see Step 1).
- Simply set a **Start Time** (e.g., 22:00 / 10 PM) and an **End Time** (e.g., 06:00 / 6 AM).
- During this period, incoming greetings are **ignored** by the lamp. The lamp will not light up.

