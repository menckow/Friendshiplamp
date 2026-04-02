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
