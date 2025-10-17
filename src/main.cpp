// src/main.cpp - Versi Final Lengkap

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>

// == KONFIGURASI ==
#define APP_VERSION "v4.0-Success" 
#define WIFI_SSID "MOT-CycleControl"
#define WIFI_PASS "motspotweld"
#define MS_PER_CYCLE 20
#define MIN_LEVEL 1
#define MAX_LEVEL 99
#define OLED_SDA 21
#define OLED_SCL 22
#define ENCODER_CLK_PIN 19
#define ENCODER_DT_PIN 18
#define ENCODER_SW_PIN 5
#define TRIGGER_PIN 23
#define SSR_PIN 26
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C

// == KODE WEB UI ==
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>MOT-CycleControl</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background: #f0f2f5; color: #333; margin: 0; padding: 20px; } .container { max-width: 500px; margin: 0 auto; background: #fff; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.05); } .header { background: #007bff; color: white; padding: 20px; border-radius: 8px 8px 0 0; text-align: center; } .header h2 { margin: 0; } .header p { margin: 5px 0 0; opacity: 0.8; } .content { padding: 20px; } .status { background: #eee; padding: 10px 15px; border-radius: 5px; text-align: center; font-weight: bold; margin-bottom: 20px; } .status.ready { background: #28a745; color: white; } .status.welding { background: #dc3545; color: white; animation: pulse 1s infinite; } @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } } .control-group { margin-bottom: 20px; } .control-group label { display: block; font-weight: bold; margin-bottom: 10px; } .slider-container { display: flex; align-items: center; } .slider-container input[type=range] { flex-grow: 1; margin: 0 10px; } .slider-container span { min-width: 60px; text-align: right; font-family: 'Courier New', Courier, monospace; } .mode-toggle { display: flex; align-items: center; justify-content: space-between; } .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; } .toggle-switch input { opacity: 0; width: 0; height: 0; } .toggle-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; } .toggle-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; } input:checked + .toggle-slider { background-color: #007bff; } input:checked + .toggle-slider:before { transform: translateX(26px); } .btn-spot { display: block; width: 100%; padding: 15px; font-size: 20px; font-weight: bold; color: white; background-color: #dc3545; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; } .btn-spot:hover { background-color: #c82333; } .presets { margin-top: 20px; } .presets select, .presets button { width: 100%; padding: 10px; margin-top: 10px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 5px; } .footer { text-align: center; margin-top: 20px; font-size: 12px; color: #888; } .footer a { color: #007bff; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header"> <h2 id="title">MOT-CycleControl</h2> <p id="version">v?.?</p> </div>
        <div class="content">
            <div id="status" class="status">Connecting...</div>
            <div class="mode-toggle control-group"> <label for="weldMode">Mode: <span id="modeLabel">SINGLE</span></label> <label class="toggle-switch"> <input type="checkbox" id="weldMode" onchange="updateSettings()"> <span class="toggle-slider"></span> </label> </div>
            <div id="controls-single"> <div class="control-group"> <label for="p1">Pulsa 1 (Siklus)</label> <div class="slider-container"> <span>1</span> <input type="range" id="p1" min="1" max="99" value="3" oninput="updateSliderLabel(this)" onchange="updateSettings()"> <span>99</span> </div> <div style="text-align: center; font-weight: bold; margin-top: 5px;"> <span id="p1_val">3</span> Siklus (<span id="p1_ms">60</span> ms) </div> </div> </div>
            <div id="controls-double" style="display: none;"> <div class="control-group"> <label for="jeda">Jeda (Siklus)</label> <div class="slider-container"> <span>1</span> <input type="range" id="jeda" min="1" max="99" value="3" oninput="updateSliderLabel(this)" onchange="updateSettings()"> <span>99</span> </div> <div style="text-align: center; font-weight: bold; margin-top: 5px;"> <span id="jeda_val">3</span> Siklus (<span id="jeda_ms">60</span> ms) </div> </div> <div class="control-group"> <label for="p2">Pulsa 2 (Siklus)</label> <div class="slider-container"> <span>1</span> <input type="range" id="p2" min="1" max="99" value="5" oninput="updateSliderLabel(this)" onchange="updateSettings()"> <span>99</span> </div> <div style="text-align: center; font-weight: bold; margin-top: 5px;"> <span id="p2_val">5</span> Siklus (<span id="p2_ms">100</span> ms) </div> </div> </div>
            <button id="btnSpot" class="btn-spot" onclick="triggerSpot()">SPOT!</button>
            <div class="presets"> <label for="presetSelect">Saran Pengaturan</label> <select id="presetSelect"> <option value="">-- Pilih Material --</option> <option value="n15_n">Nikel 0.15mm ke Nikel</option> <option value="n15_b">Nikel 0.15mm ke Baterai</option> <option value="n20_b">Nikel 0.20mm ke Baterai</option> <option value="ss10">Stainless 0.10mm</option> </select> <button onclick="applyPreset()">Terapkan Saran</button> </div>
            <div class="footer"> <a href="/update" target="_blank">Firmware Update (OTA)</a> </div>
        </div>
    </div>
    <script>
        const presets = { "n15_n": { mode: 1, p1: 2, jeda: 3, p2: 4 }, "n15_b": { mode: 1, p1: 3, jeda: 3, p2: 5 }, "n20_b": { mode: 1, p1: 4, jeda: 4, p2: 6 }, "ss10":  { mode: 0, p1: 3, jeda: 1, p2: 1 } }; let state = {}; let isWelding = false; let pollInterval;
        function updateSliderLabel(slider) { const valId = slider.id + "_val"; const msId = slider.id + "_ms"; const val = slider.value; document.getElementById(valId).innerText = val; document.getElementById(msId).innerText = val * 20; }
        function updateUI(data) { if (!data) return; state = data; document.getElementById("version").innerText = data.version || "v?.?"; const modeToggle = document.getElementById("weldMode"); const modeLabel = document.getElementById("modeLabel"); const p1 = document.getElementById("p1"); const jeda = document.getElementById("jeda"); const p2 = document.getElementById("p2"); modeToggle.checked = (data.mode == 1); modeLabel.innerText = (data.mode == 1) ? "DOUBLE PULSE" : "SINGLE PULSE"; document.getElementById("controls-double").style.display = (data.mode == 1) ? "block" : "none"; p1.value = data.p1; jeda.value = data.jeda; p2.value = data.p2; updateSliderLabel(p1); updateSliderLabel(jeda); updateSliderLabel(p2); isWelding = data.welding; updateStatus(data.welding ? "Welding..." : "Ready"); }
        function updateStatus(text) { const statusEl = document.getElementById("status"); statusEl.innerText = text; if (isWelding) { statusEl.className = "status welding"; document.getElementById("btnSpot").disabled = true; } else { statusEl.className = "status ready"; document.getElementById("btnSpot").disabled = false; } }
        async function updateSettings() { const mode = document.getElementById("weldMode").checked ? 1 : 0; const p1 = parseInt(document.getElementById("p1").value); const jeda = parseInt(document.getElementById("jeda").value); const p2 = parseInt(document.getElementById("p2").value); updateUI({ ...state, mode, p1, jeda, p2, welding: isWelding }); try { await fetch('/api/level', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ mode, p1, jeda, p2 }) }); } catch (e) { console.error("Update Gagal:", e); updateStatus("Error Sync!"); } }
        async function triggerSpot() { if (isWelding) return; isWelding = true; updateStatus("Welding..."); try { const response = await fetch('/api/spot', { method: 'POST' }); if (!response.ok) { const text = await response.text(); throw new Error(text); } } catch (e) { console.error("Spot Gagal:", e); updateStatus("Error Spot!"); isWelding = false; } }
        function applyPreset() { const presetVal = document.getElementById("presetSelect").value; if (!presetVal) return; const preset = presets[presetVal]; if (preset) { document.getElementById("weldMode").checked = (preset.mode == 1); document.getElementById("p1").value = preset.p1; document.getElementById("jeda").value = preset.jeda; document.getElementById("p2").value = preset.p2; updateSettings(); } }
        async function fetchState() { try { const response = await fetch('/api/state'); if (!response.ok) throw new Error('Network Gagal'); const data = await response.json(); if (JSON.stringify(data) !== JSON.stringify(state) && !isWelding) { updateUI(data); } else if (isWelding && !data.welding) { isWelding = false; updateUI(data); } else if (!isWelding && data.welding) { isWelding = true; updateUI(data); } } catch (error) { console.error('Fetch Gagal:', error); updateStatus("Offline"); } }
        window.onload = () => { fetchState(); pollInterval = setInterval(fetchState, 1000); };
    </script>
</body>
</html>
)rawliteral";

// == Inisialisasi Hardware & Variabel Global ==
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Encoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);
AsyncWebServer server(80);
volatile int currentLevel_1 = 3; volatile int currentLevel_Jeda = 3; volatile int currentLevel_2 = 5;
volatile int weldMode = 0; volatile bool welding = false; volatile bool settingsChanged = true;
int activeSetting = 0; long oldEncoderValue = -999; long lastButtonCheck = 0;

// == Deklarasi Fungsi ==
void setupOLED(); void setupEncoder(); void updateDisplay(); void doWeld(); void checkEncoder();
void checkEncoderButton(); void checkTriggerButton(); void setupWiFi(); void setupWebServer();
void handleGetState(AsyncWebServerRequest *request);
void handleSetLevel(AsyncWebServerRequest *request, const char* jsonString);
void handleSpot(AsyncWebServerRequest *request);

void setup() {
    Serial.begin(115200); Serial.println(F("MOT-CycleControl Booting..."));
    pinMode(SSR_PIN, OUTPUT); digitalWrite(SSR_PIN, LOW); pinMode(TRIGGER_PIN, INPUT_PULLUP); pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    setupOLED(); setupEncoder(); updateDisplay(); setupWiFi(); setupWebServer();
    Serial.println(F("Setup selesai. Sistem Siap."));
}

void loop() {
    if (welding) return; checkEncoder(); checkEncoderButton(); checkTriggerButton();
    if (settingsChanged) { updateDisplay(); settingsChanged = false; }
}

void setupOLED() { if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) { Serial.println(F("Gagal SSD1306")); while (true); } display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setCursor(0, 0); display.print(F("MOT-CycleControl")); display.setCursor(0, 10); display.print(APP_VERSION); display.display(); delay(1500); }
void setupEncoder() { encoder.write(currentLevel_1 * 4); oldEncoderValue = encoder.read(); Serial.println("Encoder OK."); }
void doWeld() { if (welding) return; welding = true; settingsChanged = true; updateDisplay(); long p1_ms = (long)currentLevel_1 * MS_PER_CYCLE; long gap_ms = (long)currentLevel_Jeda * MS_PER_CYCLE; long p2_ms = (long)currentLevel_2 * MS_PER_CYCLE; if (weldMode == 0) { Serial.printf("SPOT! S, P1:%d(%ldms)\n", (int)currentLevel_1, p1_ms); digitalWrite(SSR_PIN, HIGH); delay(p1_ms); digitalWrite(SSR_PIN, LOW); } else { Serial.printf("SPOT! D, P1:%d(%ldms), J:%d(%ldms), P2:%d(%ldms)\n", (int)currentLevel_1, p1_ms, (int)currentLevel_Jeda, gap_ms, (int)currentLevel_2, p2_ms); digitalWrite(SSR_PIN, HIGH); delay(p1_ms); digitalWrite(SSR_PIN, LOW); delay(gap_ms); digitalWrite(SSR_PIN, HIGH); delay(p2_ms); digitalWrite(SSR_PIN, LOW); } Serial.println("Selesai."); welding = false; settingsChanged = true; }
void checkTriggerButton() { if (digitalRead(TRIGGER_PIN) == LOW) { delay(50); if (digitalRead(TRIGGER_PIN) == LOW) { doWeld(); while (digitalRead(TRIGGER_PIN) == LOW) delay(10); } } }
void checkEncoder() { long newEncoderValue = encoder.read(); if (newEncoderValue != oldEncoderValue) { volatile int *targetValue; switch (activeSetting) { case 0: targetValue = &currentLevel_1; break; case 1: targetValue = &currentLevel_Jeda; break; case 2: targetValue = &currentLevel_2; break; case 3: targetValue = &weldMode; break; default: return; } if (newEncoderValue > oldEncoderValue) (*targetValue)++; else (*targetValue)--; if (activeSetting == 3) { if (*targetValue < 0) *targetValue = 0; if (*targetValue > 1) *targetValue = 1; } else { if (*targetValue < MIN_LEVEL) *targetValue = MIN_LEVEL; if (*targetValue > MAX_LEVEL) *targetValue = MAX_LEVEL; } encoder.write(*targetValue * 4); oldEncoderValue = encoder.read(); settingsChanged = true; } }
void checkEncoderButton() { if (millis() - lastButtonCheck < 200) return; if (digitalRead(ENCODER_SW_PIN) == LOW) { lastButtonCheck = millis(); while (digitalRead(ENCODER_SW_PIN) == LOW) delay(10); activeSetting++; if (activeSetting > 3) activeSetting = 0; switch (activeSetting) { case 0: encoder.write(currentLevel_1 * 4); break; case 1: encoder.write(currentLevel_Jeda * 4); break; case 2: encoder.write(currentLevel_2 * 4); break; case 3: encoder.write(weldMode * 4); break; } oldEncoderValue = encoder.read(); settingsChanged = true; } }
void updateDisplay() { display.clearDisplay(); if (welding) { display.setTextSize(3); display.setCursor(25, 20); display.print(F("SPOT!")); display.display(); return; } display.setTextSize(1); display.setCursor(0, 0); display.print(F("Mode:")); display.setCursor(70, 0); display.print(weldMode == 0 ? F("SINGLE") : F("DOUBLE")); if (activeSetting == 3) display.drawRect(68, 0, 60, 10, SSD1306_WHITE); display.setCursor(0, 15); display.printf("P1  : %d s (%d ms)", (int)currentLevel_1, (int)currentLevel_1 * MS_PER_CYCLE); if (activeSetting == 0) display.drawRect(0, 14, 128, 10, SSD1306_WHITE); display.setCursor(0, 30); if (weldMode == 1) { display.printf("Jeda: %d s (%d ms)", (int)currentLevel_Jeda, (int)currentLevel_Jeda * MS_PER_CYCLE); if (activeSetting == 1) display.drawRect(0, 29, 128, 10, SSD1306_WHITE); } else display.print(F("Jeda: N/A")); display.setCursor(0, 45); if (weldMode == 1) { display.printf("P2  : %d s (%d ms)", (int)currentLevel_2, (int)currentLevel_2 * MS_PER_CYCLE); if (activeSetting == 2) display.drawRect(0, 44, 128, 10, SSD1306_WHITE); } else display.print(F("P2  : N/A")); display.setCursor(0, 56); display.print(F("Status: SIAP")); display.display(); }
void setupWiFi() { Serial.println("Menyiapkan SoftAP..."); WiFi.softAP(WIFI_SSID, WIFI_PASS); Serial.print("AP SSID: "); Serial.println(WIFI_SSID); }

void setupWebServer() {
    // == PERBAIKAN FINAL DI SINI ==
    // Mengganti send_P yang usang dengan 'send' yang modern
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    server.on("/api/state", HTTP_GET, handleGetState);
    server.on("/api/level", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) { static char jsonBuffer[512]; if (index == 0) memset(jsonBuffer, 0, sizeof(jsonBuffer)); if (len > 0 && (index + len) < sizeof(jsonBuffer)) memcpy(jsonBuffer + index, data, len); if (index + len == total) handleSetLevel(request, jsonBuffer); } );
    server.on("/api/spot", HTTP_POST, handleSpot);
    AsyncElegantOTA.begin(&server);
    server.begin();
    Serial.println("Web Server & OTA dimulai.");
}

void handleGetState(AsyncWebServerRequest *request) { StaticJsonDocument<256> doc; doc["mode"] = (int)weldMode; doc["p1"] = (int)currentLevel_1; doc["jeda"] = (int)currentLevel_Jeda; doc["p2"] = (int)currentLevel_2; doc["welding"] = welding; doc["version"] = APP_VERSION; String output; serializeJson(doc, output); request->send(200, "application/json", output); }
void handleSetLevel(AsyncWebServerRequest *request, const char* jsonString) { if (welding) { request->send(503, "text/plain", "Sedang mengelas..."); return; } StaticJsonDocument<256> doc; DeserializationError error = deserializeJson(doc, jsonString); if (error) { request->send(400, "text/plain", "Invalid JSON"); return; } JsonObject obj = doc.as<JsonObject>(); if (obj.containsKey("mode")) { weldMode = obj["mode"].as<int>(); if (weldMode < 0) weldMode = 0; if (weldMode > 1) weldMode = 1; } if (obj.containsKey("p1")) { currentLevel_1 = obj["p1"].as<int>(); if (currentLevel_1 < MIN_LEVEL) currentLevel_1 = MIN_LEVEL; if (currentLevel_1 > MAX_LEVEL) currentLevel_1 = MAX_LEVEL; } if (obj.containsKey("jeda")) { currentLevel_Jeda = obj["jeda"].as<int>(); if (currentLevel_Jeda < MIN_LEVEL) currentLevel_Jeda = MIN_LEVEL; if (currentLevel_Jeda > MAX_LEVEL) currentLevel_Jeda = MAX_LEVEL; } if (obj.containsKey("p2")) { currentLevel_2 = obj["p2"].as<int>(); if (currentLevel_2 < MIN_LEVEL) currentLevel_2 = MIN_LEVEL; if (currentLevel_2 > MAX_LEVEL) currentLevel_2 = MAX_LEVEL; } settingsChanged = true; switch (activeSetting) { case 0: encoder.write(currentLevel_1 * 4); break; case 1: encoder.write(currentLevel_Jeda * 4); break; case 2: encoder.write(currentLevel_2 * 4); break; case 3: encoder.write(weldMode * 4); break; } oldEncoderValue = encoder.read(); handleGetState(request); }
void handleSpot(AsyncWebServerRequest *request) { if (welding) { request->send(503, "text/plain", "Operasi sedang berlangsung."); return; } doWeld(); request->send(200, "text/plain", "OK"); }
