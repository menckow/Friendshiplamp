# 🌟 Freundschaftslampe - Anwenderdokumentation

Willkommen zur Anleitung für deine neue **Freundschaftslampe (Version 2.1)**! 
Diese Lampe verbindet dich über das Internet (WLAN & MQTT) mit deinen Liebsten. Wenn du an sie denkst und den Knopf drückst, leuchtet ihre Lampe bei ihnen zuhause in deiner persönlichen Farbe auf – und umgekehrt.

---

## 🛠️ 1. Die erste Einrichtung (Setup)

Wenn du die Lampe zum ersten Mal mit Strom versorgst oder sie dein Heim-WLAN nicht findet, leuchtet sie **Violett/Lila**. Das bedeutet, sie befindet sich im **Einrichtungs-Modus (Setup)**.

1. Öffne die WLAN-Einstellungen deines Smartphones oder Computers.
2. Suche nach dem Netzwerk mit dem Namen **`Freundschaftslampe-Setup`** und verbinde dich damit.
3. Gib das Standard-Passwort ein: **`12345678`**
4. Normalerweise öffnet sich nun automatisch ein Konfigurationsfenster (Captive Portal). Falls nicht, öffne deinen Webbrowser und gib die Adresse **`http://192.168.4.1`** ein.
5. In der Weboberfläche kannst du nun:
   * Dein **Heim-WLAN** auswählen (die Lampe scannt automatisch nach verfügbaren Netzwerken) und das zugehörige Passwort eingeben.
   * Die **Anzahl der LEDs** in deiner Lampe festlegen.
   * Deine **Identitätsfarbe** bestimmen (diese Farbe sehen deine Freunde, wenn du ihnen ein Signal schickst).
   * Einen **Lichteffekt** auswählen (z.B. Fade, Komet oder Feuer-Effekt).
   * Die **Anzeigedauer** einstellen (wie lange ein empfangenes Signal leuchten soll).
   * Einen **Ruhemodus** (Nachtschaltung) aktivieren.
6. Klicke auf **Speichern & Neustarten**. Die Lampe verbindet sich nun mit deinem Heim-WLAN.

---

## 💡 2. Bedienung im Alltag

Die Lampe ist so konzipiert, dass sie einfach und intuitiv bedient werden kann. Sie besitzt folgende Steuerungselemente:

### 👆 Der Touch-Sensor (Ein/Aus & Helligkeit)
* **Kurzes Berühren:** Schaltet die Lampe ein oder aus.
* **Langes Berühren (Gedrückt halten):** Verändert die Helligkeit. Die Lampe dimmt stufenlos hoch oder runter. Lass den Sensor einfach los, sobald die gewünschte Helligkeit erreicht ist.

### 🎛️ Der Drehregler (Potentiometer)
* **Farbe ändern:** Durch Drehen des Reglers kannst du deine *lokale* Leuchtfarbe (deine Identitätsfarbe) jederzeit anpassen. Diese Farbe wird beim nächsten Senden an deine Freunde übermittelt.

### 🔘 Der Senden-Taster
* **Knopf drücken:** Wenn du diesen Knopf drückst, schickt deine Lampe ein Signal an alle anderen Lampen in deinem Netzwerk. Sie leuchten dann für die eingestellte Dauer in **deiner** aktuell gewählten Farbe und mit dem von dir eingestellten Effekt auf.

---

## ✨ 3. Besondere Lichteffekte

Die Freundschaftslampe (ab V2.1) bietet nicht nur statisches Licht, sondern auch verschiedene, flüssige Animationen, die du im Webinterface auswählen kannst:

* **Fade:** Ein sanftes Ein- und Ausblenden der Farbe.
* **Color Wipe:** Die Farbe "wischt" sich LED für LED um den Ring auf.
* **Theater Chase:** Ein klassischer, wandernder Lauflicht-Effekt.
* **Breathe:** Die Lampe "atmet" sanft in deiner Farbe (ruhiges Pulsieren).
* **Feuer-Effekt:** Ein dynamisches Flackern in deiner eingestellten Farbe.
* **Komet:** Ein heller Punkt, der einen Lichtschweif um den Ring zieht.
* **Rainbow Cycle:** Ein Sonder-Effekt, der unabhängig von deiner Identitätsfarbe das volle Farbspektrum (Regenbogen) als Lauflicht anzeigt.

---

## 📡 4. Für Administratoren: Dashboard & Updates

Falls du mehrere Lampen (oder Zwitscherboxen) in deiner Familie verwaltest, bietet das Projekt ein zentrales Dashboard.

* **Zentrales Management:** Öffne die Datei `manager/dashboard.html` in deinem Browser. Du siehst eine Echtzeit-Übersicht aller Geräte.
* **Live-Status:** Fällt eine Lampe aus (WLAN-Abbruch), wird sie im Dashboard dank LWT (Last Will and Testament) sofort als `offline` markiert.
* **Updates "Over the Air" (OTA):** Du musst die Lampe für neue Software-Updates nicht mehr per USB an den PC anschließen. Die Lampe kann ihre neue Software dank SSL-Unterstützung sicher direkt über das Internet (z.B. von GitHub) beziehen. Schlägt ein Update fehl, kehrt sie automatisch zur letzten funktionierenden Version zurück (Rollback-Schutz).