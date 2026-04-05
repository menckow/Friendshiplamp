# Friendshiplamp
![Freundschaftslampe] (https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033.png)
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
