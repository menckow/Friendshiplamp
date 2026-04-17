# Friendship Lamp (V2.2)

![Friendship Lamp](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)

The **Friendship Lamp** is an ESP32-based ambient light that connects friends through synchronized colors. When one person touches their lamp, all connected lamps across the world pulse in their unique identity color via MQTT.

This version (V2.2) features a fully refactored, modular object-oriented architecture for maximum stability and easy extensibility.

## 🚀 Key Features

*   **Modular Architecture**: Clean separation of hardware control, network logic, and configuration.
*   **Robust OTA Updates**: Secure over-the-air firmware updates via MQTT using 32-character **MD5 validation** to ensure firmware integrity. Includes a dynamic LED ring visual progress bar and live `%` status reporting back to the remote Manager Dashboard.
*   **Smart Security**: Built-in standard Root CA (ISRG Root X1) for seamless out-of-the-box support for HiveMQ and other secure public brokers—no manual certificate pasting required.
*   **Advanced Lighting Engine**: Smooth 32-bit gamma correction with effects like *Fade, Rainbow, Breathe, Fire, Comet,* and more.
*   **Intuitive Web Configuration**: Mobile-friendly captive portal for Wi-Fi setup, MQTT credentials, and hardware calibration.
*   **Smart Hardware Control**:
    *   **Capacitive Touch**: For On/Off and continuous brightness dimming.
    *   **Potentiometer**: Direct local color selection.
    *   **Button**: Sending the pulse color to friends.
*   **Night Mode (Do Not Disturb)**: NTP-synced, scheduled quiet hours configurable via the Web UI to keep the room completely dark and suppress incoming color flashes during sleep.

## 🛠️ Hardware Requirements

*   **Microcontroller**: ESP32 Dev Kit C or ESP32-S3.
*   **LEDs**: WS2812B (NeoPixel) Ring or Strip.
*   **Inputs**:
    *   1x Potentiometer (Analog)
    *   1x Momentary Button
    *   1x Capacitive Touch sensor (or a simple wire to a touch pin)

### Default Pinout (Configurable)

| Component           | ESP32 Pin |
| ------------------- | --------- |
| NeoPixel DIN        | GPIO 13   |
| Potentiometer Signal| GPIO 34   |
| Send Button         | GPIO 4    |
| Touch Sensor        | GPIO 32   |

## 💻 Setup & Installation

### 1. Build & Upload
This project is professionally managed using **PlatformIO**. 
1.  Clone the repository.
2.  Open the folder in VS Code with the PlatformIO extension installed.
3.  Click **Build** and then **Upload**.

### 2. First-Time Configuration
1.  If no Wi-Fi is configured, the lamp will pulse **purple** and open a Wi-Fi hotspot named `Freundschaftslampe-Setup`.
2.  Connect to the hotspot (Password: `12345678`).
3.  Your browser should open `http://192.168.4.1` automatically.
4.  Enter your home Wi-Fi details and MQTT broker credentials.
    *   *Tip: If using HiveMQ, simply check the "Use Standard Root-CA" box.*

## 📡 MQTT Integration & Monitoring

The project is designed to work with a central [Device Manager Dashboard](manager/dashboard_secure.html).
*   **Status Reporting**: Lamps report their firmware version and online/offline status (using LWT) to `freundschaftslampe/status/<ClientID>`.
*   **Update Trigger**: Remote updates can be triggered via `freundschaftslampe/update/trigger` using a JSON payload containing the new `.bin` URL.

## 📄 License & Credits
Developed for the community. Feel free to fork and enhance!
