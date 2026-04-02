# Anwenderhandbuch: Freundschaftslampe

Willkommen zu deiner neuen **Freundschaftslampe**! Diese Lampe ermöglicht es dir, über weite Entfernungen hinweg mit deinen Liebsten in Kontakt zu bleiben, indem ihr euch farbige Lichtgrüße sendet. 

Hier erfährst du, wie du deine Lampe einrichtest und bedienst.

---

## 1. Die Ersteinrichtung (WLAN verbinden)

Damit die Lampe Grüße senden und empfangen kann, muss sie mit deinem WLAN-Netzwerk verbunden werden.

1. **Einrichtungsmodus erkennen:** Wenn die Lampe nach dem Einschalten **lila leuchtet**, konnte sie keine bestehende WLAN-Verbindung finden. Sie befindet sich nun im Einrichtungmodus (Access-Point-Modus).
2. **Mit der Lampe verbinden:** Nimm ein Smartphone, Tablet oder einen Laptop und suche nach neuen WLAN-Netzwerken in deiner Umgebung. Verbinde dich mit dem WLAN namens **"Freundschaftslampe-Setup"**.
3. **Konfigurationsseite öffnen:** Meistens öffnet sich automatisch eine Seite (das sogenannte Captive Portal). Passiert dies nicht, öffne einfach deinen Webbrowser und gib eine beliebige Internetadresse ein. Du wirst nun auf die Einstellungsseite der Lampe umgeleitet.
4. **Daten eintragen:**
   - Trage unter **WLAN-Einstellungen** den Namen (SSID) und das Passwort deines **Heim-WLANs** ein.
   - Trage unter **MQTT-Einstellungen** die Verbindungsdaten für den Kommunikationsserver (Broker) ein.
   - Wähle unter **Lampen-Einstellungen** deine persönliche **Identitätsfarbe** aus (diese Farbe leuchtet bei deinem Gegenüber auf, wenn du einen Gruß sendest).
5. **Speichern:** Klicke auf **"Speichern & Neustarten"**. Die Lampe startet nun neu und verbindet sich automatisch mit deinem Heim-WLAN. Das lila Licht sollte nun verschwinden.

---

## 2. Die Bedienung der Lampe

### 💡 Lampe ein- und ausschalten
- **Kurzes Berühren des Touch-Sensors:** Tippe den Sensor kurz an, um die Lampe ein- oder auszuschalten.

### ☀️ Helligkeit einstellen
- **Langes Berühren des Touch-Sensors:** Halte den Finger auf dem Touch-Sensor, um die Helligkeit zu stufenlos anzupassen. Lasse los, sobald das Licht stark oder schwach genug ist. Bei der nächsten langen Berührung ändert sich die Helligkeit in die jeweils andere Richtung.

### 🎨 Eigene Farbe anpassen
- **Am Potentiometer (Drehknopf) drehen:** Wenn die Lampe eingeschaltet ist, kannst du mit dem Drehknopf die lokale Farbe des Lichtrings ganz nach Belieben einstellen. 

### 💌 Einen farbigen Gruß versenden
- **Den Taster drücken:** Drücke den Taster (Knopf) an der Lampe einmal kurz. Deine Lampe sendet nun deine oben eingestellte *Identitätsfarbe* über das Internet. Die verbundene Lampe deines Gegenübers leuchtet in dieser Sekunde auf!

---

## 3. Einen Gruß empfangen

Wenn jemand den Knopf an seiner Lampe drückt, passiert Folgendes bei dir:
- Deine Lampe unterbricht ihr aktuelles Licht und leuchtet sofort für **10 Sekunden** in der Farbe des Absenders auf.
- Nach Ablauf der 10 Sekunden verschwindet das empfangene Licht wieder. Die Lampe kehrt ganz automatisch zu der Farbe und dem Zustand (Ein oder Aus) zurück, den sie vorher hatte.

---

## 4. Nachtruhe (Ruhemodus)

Damit du nachts nicht durch unerwartet aufleuchtende Grüße geweckt wirst, verfügt die Lampe über eine Zeitsteuerung.

- Du kannst den **Ruhemodus** auf der WLAN-Einrichtungsseite der Lampe (siehe Schritt 1) aktivieren.
- Lege dort einfach eine **Startzeit** (z.B. 22 Uhr) und eine **Endzeit** (z.B. 6 Uhr) fest.
- In diesem Zeitraum werden eingehende Grüße von der Lampe **ignoriert**. Die Lampe leuchtet dann nicht auf.
