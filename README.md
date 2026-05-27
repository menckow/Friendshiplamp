# Friendship Lamp (V2.3 — Family Schema)

![Friendship Lamp](https://github.com/menckow/Friendshiplamp/blob/main/Screenshot_20260404-204033~2.png)

The **Friendship Lamp** is an ESP32-based ambient light that connects friends through synchronized colors. When one person touches their lamp, all connected lamps across the world pulse in their unique identity color via MQTT.

Version **V2.3** introduces the **family circle schema (v2)** — a single lamp can belong to multiple family circles simultaneously, and the central Device Manager Dashboard groups all devices by family.

## 🚀 Key Features

*   **Modular Architecture**: Clean separation of hardware control, network logic, and configuration.
*   **Family Circles (v2 Schema)**: Configure one or more `Familienkreise` (comma-separated) per lamp. The lamp publishes button presses to every family it belongs to and receives signals from all of them. Multi-membership is supported.
*   **Source-Aware Signaling**: Each signal carries a `sender_type` field. Lamps ignore signals originating from boxes (so a box's motion sensor doesn't wake the lamp), while boxes show the lamp's color in a special complementary-LED mode when a lamp triggered the signal.
*   **Local Button Feedback**: Pressing the button lights the lamp itself in the identity color — visual confirmation that the press was registered, independent of the MQTT round-trip.
*   **Robust OTA Updates (two channels)**:
    *   **MQTT-triggered OTA**: per-device (`fl/device/<id>/update/trigger`), per-family (`fl/family/<f>/update/trigger/lamp`), or global (`fl/_global/update/trigger/lamp`). All with 32-character **MD5 validation** and dynamic LED progress bar.
    *   **Web Upload OTA**: Browse to the configuration page and upload a `.bin` directly; the lamp validates MD5 (optional), writes via chunked `Update.write()` and reboots automatically.
*   **Smart Security**: Built-in standard Root CA (ISRG Root X1) for seamless out-of-the-box support for HiveMQ and other secure public brokers—no manual certificate pasting required.
*   **Advanced Lighting Engine**: Smooth 32-bit gamma correction with effects like *Fade, Rainbow, Breathe, Fire, Comet,* and more.
*   **Intuitive Web Configuration & Remote Control**:
    *   **Captive Portal**: Mobile-friendly local portal for Wi-Fi setup, MQTT credentials, family-circle assignment, and hardware calibration.
    *   **Dedicated Web App**: A standalone modern [Web App](manager/Friendshiplamp_Web_App_V4_Final.html) for remote triggering of the lamp with custom color and **selectable effects** (Fade, Wipe, Chase, Rainbow, etc.).
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
5.  **Enter Family Circles** (comma-separated list, e.g. `schmidt,lieblings`). The lamp will subscribe to one signal topic per family and fan out its own button presses to all of them. Whitespace and case are normalized automatically.

## 📡 MQTT Integration & Monitoring

The project works with a central [Device Manager Dashboard](manager/dashboard_secure.html), which groups all devices by family circle and shows a green `v2` pill next to family-aware devices.

### Topic schema (v2)

| Purpose | Topic | Retained |
|---|---|---|
| Family signal (publisher + subscriber) | `fl/family/<familyId>/signal` | no |
| Device status (JSON) | `fl/device/<deviceId>/status` | **yes** |
| OTA per single device | `fl/device/<deviceId>/update/trigger` | no |
| OTA per family (lamps only) | `fl/family/<familyId>/update/trigger/lamp` | no |
| Global emergency OTA (lamps only) | `fl/_global/update/trigger/lamp` | no |
| OTA progress backchannel | `fl/device/<deviceId>/update/status` | no |

### Status payload (retained JSON)

```json
{
  "type": "lamp",
  "fw": "V2.3.0",
  "state": "online",
  "color": "#FFAA00",
  "families": ["schmidt", "lieblings"]
}
```

### Signal payload (on button press)

```json
{
  "client_id": "Schmidt-Wohnzimmer",
  "sender_type": "lamp",
  "color": "#FFAA00",
  "effect": "fade",
  "duration": 30000,
  "ts": 1716729600
}
```

The receiving devices apply a **self-filter** (ignore messages where `client_id` matches their own ID, important for multi-family membership) and an **NTP age filter** (older than 60 seconds → ignored, to prevent retained messages from re-triggering after reconnect).

### OTA payload

```json
{
  "url": "https://your-server.com/firmware.bin",
  "version": "V2.3.1",
  "md5": "b3e3e3b3e3e3b3e3e3b3e3e3b3e3e3b3",
  "target_type": "lamp"
}
```

`target_type` is optional defense-in-depth — devices verify it matches their own type before accepting the update.

### Web Upload OTA

Open the lamp's configuration page and use the **Firmware-Update** section to upload a `.bin` file directly. The lamp validates MD5 (optional 32-char hex), writes the firmware, and reboots automatically. The dashboard sees status changes via MQTT during the upload.

## 📄 License & Credits
Developed for the community. Feel free to fork and enhance!
