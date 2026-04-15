#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Freundschaftslampe Konfiguration</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: #f0f2f5; color: #333; margin: 0; padding: 20px; display: flex; justify-content: center; }
    .container { width: 100%%; max-width: 700px; background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
    h1 { text-align: center; color: #1c1e21; font-size: 2em; margin-bottom: 1em; }
    form { display: flex; flex-direction: column; }
    fieldset { border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin-top: 20px; background-color: #fafafa; }
    legend { font-size: 1.2em; font-weight: 600; padding: 0 10px; color: #007bff; margin-left: 10px; }
    label { font-weight: 600; margin-top: 15px; margin-bottom: 5px; }
    input[type='text'], input[type='password'], input[type='number'], textarea, select { padding: 12px; margin-top: 5px; background-color: #fff; color: #333; border: 1px solid #ccc; border-radius: 6px; font-size: 1em; width: 100%%; box-sizing: border-box; }
    input:focus, textarea:focus, select:focus { border-color: #007bff; box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.25); outline: none; }
    textarea { font-family: monospace; resize: vertical; min-height: 120px; }
    .checkbox-container { display: flex; align-items: center; margin-top: 15px; }
    input[type='checkbox'] { margin-right: 10px; width: 18px; height: 18px; }
    input[type='color'] { width: 100%%; height: 45px; padding: 5px; border-radius: 6px; border: 1px solid #ccc; }
    input[type='submit'] { background-color: #007bff; color: white; cursor: pointer; margin-top: 30px; padding: 15px; font-size: 1.1em; font-weight: 600; border: none; border-radius: 6px; }
    input[type='submit']:hover { background-color: #0056b3; }
    .help-text { font-size: 0.9em; color: #666; margin-top: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Freundschaftslampe</h1>
    <form action='/save' method='POST'>
      <fieldset><legend>Administrator</legend>
        <label for='admin_pass'>Webinterface Passwort:</label>
        <input type='password' id='admin_pass' name='admin_pass' value='%ADMIN_PASS%'>
      </fieldset>
      <fieldset><legend>WLAN-Einstellungen</legend>
        <label for='ssid_select'>Verfügbare WLANs:</label>
        <select id='ssid_select' onchange='if(this.value) document.getElementById("ssid").value = this.value;'><option value=''>Suche WLANs...</option></select>
        <label for='ssid'>WLAN SSID:</label>
        <input type='text' id='ssid' name='ssid' value='%SSID%'>
        <label for='password'>WLAN Passwort:</label>
        <input type='password' id='password' name='password' value='%WIFI_PASS%'>
      </fieldset>
      <fieldset><legend>MQTT-Einstellungen</legend>
        <label for='mqtt'>MQTT Broker:</label>
        <input type='text' id='mqtt' name='mqtt' value='%MQTT_SERVER%'>
        <label for='mqtt_port'>MQTT Port:</label>
        <input type='number' id='mqtt_port' name='mqtt_port' value='%MQTT_PORT%'>
        <div class="checkbox-container"><input type='checkbox' id='mqtt_tls' name='mqtt_tls' %MQTT_TLS_CHECKED% onchange="toggleCaFields()"><label for='mqtt_tls'>MQTT mit TLS/SSL</label></div>
        <div id="ca_settings" style="display: none; margin-left: 20px;">
          <div class="checkbox-container">
            <input type='checkbox' id='mqtt_standard_ca' name='mqtt_standard_ca' %MQTT_STANDARD_CA_CHECKED% onchange="toggleCaFields()">
            <label for='mqtt_standard_ca'>Standard Root-CA nutzen (HiveMQ/Let's Encrypt)</label>
          </div>
          <div id="custom_ca_field" style="display: none;">
            <label for='mqtt_ca'>Eigenes CA Zertifikat (PEM):</label>
            <textarea id='mqtt_ca' name='mqtt_ca'>%MQTT_CA%</textarea>
          </div>
        </div>
        <label for='mqtt_client_id'>MQTT Client ID:</label>
        <input type='text' id='mqtt_client_id' name='mqtt_client_id' value='%CLIENT_ID%'>
        <label for='mqtt_topic'>MQTT Topic:</label>
        <input type='text' id='mqtt_topic' name='mqtt_topic' value='%TOPIC%'>
        <label for='mqtt_user'>Benutzername:</label>
        <input type='text' id='mqtt_user' name='mqtt_user' value='%MQTT_USER%'>
        <label for='mqtt_pass'>Passwort:</label>
        <input type='password' id='mqtt_pass' name='mqtt_pass' value='%MQTT_PASS%'>
      </fieldset>
      <fieldset><legend>Lampen-Einstellungen</legend>
        <label for='num_pixels'>Anzahl LEDs:</label>
        <input type='number' id='num_pixels' name='num_pixels' value='%NUM_PIXELS%'>
        <label for='touch_threshold'>Touch-Empfindlichkeit (niedriger = empfindlicher):</label>
        <input type='number' id='touch_threshold' name='touch_threshold' value='%TOUCH_THRESHOLD%' min='1' max='100'>
        <label for='color'>Deine Identitätsfarbe:</label>
        <input type='color' id='color' name='color' value='%COLOR%'>
        <label for='effect'>Lichteffekt:</label>
        <select id='effect' name='effect'>
          <option value='fade' %FX_FADE%>Fade</option>
          <option value='color_wipe' %FX_WIPE%>Color Wipe</option>
          <option value='theater_chase' %FX_CHASE%>Theater Chase</option>
          <option value='rainbow_cycle' %FX_RAINBOW%>Rainbow</option>
          <option value='breathe' %FX_BREATHE%>Breathe</option>
          <option value='fire' %FX_FIRE%>Feuer</option>
          <option value='comet' %FX_COMET%>Komet</option>
        </select>
        <label for='duration'>Anzeigedauer (Sekunden):</label>
        <input type='number' id='duration' name='duration' value='%DURATION%'>
      </fieldset>
      <fieldset><legend>Ruhemodus</legend>
        <div class="checkbox-container"><input type='checkbox' id='quiet_enabled' name='quiet_enabled' %QUIET_CHECKED%><label for='quiet_enabled'>Aktivieren</label></div>
        <label for='quiet_start'>Startzeit (0-23):</label>
        <input type='number' id='quiet_start' name='quiet_start' value='%QUIET_START%'>
        <label for='quiet_end'>Endzeit (0-23):</label>
        <input type='number' id='quiet_end' name='quiet_end' value='%QUIET_END%'>
      </fieldset>
      <input type='submit' value='Speichern & Neustarten'>
    </form>
  </div>
  <script>
    function scanWlan() {
      fetch('/scan').then(r => r.json()).then(nets => {
        if(!nets) return;
        let s = document.getElementById('ssid_select');
        s.innerHTML = '<option value="">-- WLAN auswählen --</option>';
        nets.forEach(n => { let o = document.createElement('option'); o.value = n.ssid; o.textContent = n.ssid + " (" + n.rssi + " dBm)"; s.appendChild(o); });
      });
    }
    
    function toggleCaFields() {
      const tls = document.getElementById('mqtt_tls').checked;
      const standard = document.getElementById('mqtt_standard_ca').checked;
      document.getElementById('ca_settings').style.display = tls ? 'block' : 'none';
      document.getElementById('custom_ca_field').style.display = (tls && !standard) ? 'block' : 'none';
    }
    
    scanWlan();
    toggleCaFields();
  </script>
</body>
</html>
)rawliteral";

const char SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Gespeichert!</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>body { font-family: sans-serif; background: #f0f2f5; text-align: center; padding: 50px; } .card { background: white; padding: 30px; border-radius: 8px; max-width: 500px; margin: 0 auto; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }</style>
</head>
<body><div class="card"><h1>Gespeichert!</h1><p>Die Lampe startet neu...</p><a href="/">Zurück</a></div>
<script>setTimeout(() => { fetch('/reboot'); }, 1000);</script>
</body>
</html>
)rawliteral";

#endif
