// src/main.cpp

#include <Arduino.h>
#include <Wire.h>
#include "Config.h"

// == Library Fase 1 ==
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>

// == Library Fase 2 ==
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>

// == Halaman Web UI (HTML, CSS, JS) ==
// Disimpan di PROGMEM (Flash) untuk menghemat RAM
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>MOT-CycleControl</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background: #f0f2f5; color: #333; margin: 0; padding: 20px; }
        .container { max-width: 500px; margin: 0 auto; background: #fff; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.05); }
        .header { background: #007bff; color: white; padding: 20px; border-radius: 8px 8px 0 0; text-align: center; }
        .header h2 { margin: 0; }
        .header p { margin: 5px 0 0; opacity: 0.8; }
        .content { padding: 20px; }
        .status { background: #eee; padding: 10px 15px; border-radius: 5px; text-align: center; font-weight: bold; margin-bottom: 20px; }
        .status.ready { background: #28a745; color: white; }
        .status.welding { background: #dc3545; color: white; animation: pulse 1s infinite; }
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }
        .control-group { margin-bottom: 20px; }
        .control-group label { display: block; font-weight: bold; margin-bottom: 10px; }
        .slider-container { display: flex; align-items: center; }
        .slider-container input[type=range] { flex-grow: 1; margin: 0 10px; }
        .slider-container span { min-width: 60px; text-align: right; font-family: 'Courier New', Courier, monospace; }
        .mode-toggle { display: flex; align-items: center; justify-content: space-between; }
        .toggle-switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .toggle-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .toggle-slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .toggle-slider { background-color: #007bff; }
        input:checked + .toggle-slider:before { transform: translateX(26px); }
        .btn-spot { display: block; width: 100%; padding: 15px; font-size: 20px; font-weight: bold; color: white; background-color: #dc3545; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }
        .btn-spot:hover { background-color: #c82333; }
        .presets { margin-top: 20px; }
        .presets select, .presets button { width: 100%; padding: 10px; margin-top: 10px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 5px; }
        .footer { text-align: center; margin-top: 20px; font-size: 12px; color: #888; }
        .footer a { color: #007bff; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h2 id="title">MOT-CycleControl</h2>
            <p id="version">v?.?</p>
        </div>
        <div class="content">
            <div id="status" class="status">Connecting...</div>
            
            <div class="mode-toggle control-group">
                <label for="weldMode">Mode: <span id="modeLabel">SINGLE</span></label>
                <label class="toggle-switch">
                    <input type="checkbox" id="weldMode" onchange="updateSettings()">
                    <span class="toggle-slider"></span>
                </label>
            </div>
            
            <div id="controls-single">
                <div class="control-group">
                    <label for="p1">Pulsa 1 (Siklus)</label>
                    <div class="slider-container">
                        <span>1</span>
                        <input type="range" id="p1" min="1" max="99" value="3" oninput="updateSliderLabel(this)" onchange="updateSettings()">
                        <span>99</span>
                    </div>
                    <div style="text-align: center; font-weight: bold; margin-top: 5px;">
                        <span id="p1_val">3</span> Siklus (<span id="p1_ms">60</span> ms)
                    </div>
                </div>
            </div>
            
            <div id="controls-double" style="display: none;">
                <div class="control-group">
                    <label for="jeda">Jeda (Siklus)</label>
                    <div class="slider-container">
                        <span>1</span>
                        <input type="range" id="jeda" min="1" max="99" value="3" oninput="updateSliderLabel(this)" onchange="updateSettings()">
                        <span>99</span>
                    </div>
                    <div style="text-align: center; font-weight: bold; margin-top: 5px;">
                        <span id="jeda_val">3</span> Siklus (<span id="jeda_ms">60</span> ms)
                    </div>
                </div>
                <div class="control-group">
                    <label for="p2">Pulsa 2 (Siklus)</label>
                    <div class="slider-container">
                        <span>1</span>
                        <input type="range" id="p2" min="1" max="99" value="5" oninput="updateSliderLabel(this)" onchange="updateSettings()">
                        <span>99</span>
                    </div>
                    <div style="text-align: center; font-weight: bold; margin-top: 5px;">
                        <span id="p2_val">5</span> Siklus (<span id="p2_ms">100</span> ms)
                    </div>
                </div>
            </div>
            
            <button id="btnSpot" class="btn-spot" onclick="triggerSpot()">SPOT!</button>
            
            <div class="presets">
                <label for="presetSelect">Saran Pengaturan (Mulai dari sini!)</label>
                <select id="presetSelect">
                    <option value="">-- Pilih Material --</option>
                    <option value="n15_n">Nikel 0.15mm ke Nikel 0.15mm</option>
                    <option value="n15_b">Nikel 0.15mm ke Baterai</option>
                    <option value="n20_b">Nikel 0.20mm ke Baterai</option>
                    <option value="ss10">Stainless 0.10mm</option>
                </select>
                <button onclick="applyPreset()">Terapkan Saran</button>
            </div>

            <div class="footer">
                <a href="/update" target="_blank">Firmware Update (OTA)</a>
            </div>
        </div>
    </div>

    <script>
        // Database Preset (Client-Side)
        const presets = {
            "n15_n": { mode: 1, p1: 2, jeda: 3, p2: 4 }, // Double, 40ms, 60ms, 80ms
            "n15_b": { mode: 1, p1: 3, jeda: 3, p2: 5 }, // Double, 60ms, 60ms, 100ms
            "n20_b": { mode: 1, p1: 4, jeda: 4, p2: 6 }, // Double, 80ms, 80ms, 120ms
            "ss10":  { mode: 0, p1: 3, jeda: 1, p2: 1 }  // Single, 60ms
        };
        
        let state = {};
        let isWelding = false;
        let pollInterval;

        function updateSliderLabel(slider) {
            const valId = slider.id + "_val";
            const msId = slider.id + "_ms";
            const val = slider.value;
            document.getElementById(valId).innerText = val;
            document.getElementById(msId).innerText = val * 20;
        }

        function updateUI(data) {
            if (!data) return;
            state = data;
            
            document.getElementById("version").innerText = data.version || "v?.?";
            
            const modeToggle = document.getElementById("weldMode");
            const modeLabel = document.getElementById("modeLabel");
            const p1 = document.getElementById("p1");
            const jeda = document.getElementById("jeda");
            const p2 = document.getElementById("p2");

            modeToggle.checked = (data.mode == 1);
            modeLabel.innerText = (data.mode == 1) ? "DOUBLE PULSE" : "SINGLE PULSE";
            
            if (data.mode == 1) {
                document.getElementById("controls-double").style.display = "block";
            } else {
                document.getElementById("controls-double").style.display = "none";
            }
            
            p1.value = data.p1;
            jeda.value = data.jeda;
            p2.value = data.p2;

            updateSliderLabel(p1);
            updateSliderLabel(jeda);
            updateSliderLabel(p2);
            
            isWelding = data.welding;
            updateStatus(data.welding ? "Welding..." : "Ready");
        }

        function updateStatus(text) {
            const statusEl = document.getElementById("status");
            statusEl.innerText = text;
            if (isWelding) {
                statusEl.className = "status welding";
                document.getElementById("btnSpot").disabled = true;
            } else {
                statusEl.className = "status ready";
                document.getElementById("btnSpot").disabled = false;
            }
        }

        async function updateSettings() {
            const mode = document.getElementById("weldMode").checked ? 1 : 0;
            const p1 = parseInt(document.getElementById("p1").value);
            const jeda = parseInt(document.getElementById("jeda").value);
            const p2 = parseInt(document.getElementById("p2").value);

            // Update UI langsung untuk responsifitas
            updateUI({ ...state, mode, p1, jeda, p2, welding: isWelding });
            
            // Kirim ke server
            try {
                await fetch('/api/level', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode, p1, jeda, p2 })
                });
            } catch (e) {
                console.error("Gagal update settings:", e);
                updateStatus("Error Sync!");
            }
        }

        async function triggerSpot() {
            if (isWelding) return;
            isWelding = true;
            updateStatus("Welding...");
            
            try {
                const response = await fetch('/api/spot', { method: 'POST' });
                if (!response.ok) {
                    const text = await response.text();
                    throw new Error(text);
                }
                // Server akan update state, polling akan mengambilnya
            } catch (e) {
                console.error("Gagal trigger spot:", e);
                updateStatus("Error Spot!");
                isWelding = false; // Reset jika gagal
            }
        }

        function applyPreset() {
            const presetVal = document.getElementById("presetSelect").value;
            if (!presetVal) return;
            
            const preset = presets[presetVal];
            if (preset) {
                // Update UI lokal
                document.getElementById("weldMode").checked = (preset.mode == 1);
                document.getElementById("p1").value = preset.p1;
                document.getElementById("jeda").value = preset.jeda;
                document.getElementById("p2").value = preset.p2;
                
                // Kirim update ke server
                updateSettings();
            }
        }

        async function fetchState() {
            try {
                const response = await fetch('/api/state');
                if (!response.ok) throw new Error('Network response was not ok');
                const data = await response.json();
                
                // Hanya update jika ada perubahan, kecuali sedang welding
                if (JSON.stringify(data) !== JSON.stringify(state) && !isWelding) {
                    console.log("State berubah, update UI.");
                    updateUI(data);
                } else if (isWelding && !data.welding) {
                    // Jika kita pikir sedang welding, tapi server bilang tidak (las selesai)
                    console.log("Weld selesai.");
                    isWelding = false;
                    updateUI(data);
                } else if (!isWelding && data.welding) {
                    // Jika tombol fisik ditekan
                    console.log("Weld fisik terdeteksi.");
                    isWelding = true;
                    updateUI(data);
                }

            } catch (error) {
                console.error('Gagal fetch state:', error);
                updateStatus("Offline");
            }
        }

        window.onload = () => {
            fetchState(); // Ambil state pertama kali
            pollInterval = setInterval(fetchState, 1000); // Polling state tiap 1 detik
        };
    </script>
</body>
</html>
)rawliteral";


// == Inisialisasi Hardware ==
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RotaryEncoder encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN, RotaryEncoder::LatchMode::FOUR_STEPS);
AsyncWebServer server(80); // Server di port 80

// == Variabel Status Global ==
// (Sama seperti Fase 1, tapi sekarang 'volatile' untuk akses aman dari ISR/server)
volatile int currentLevel_1 = 3;
volatile int currentLevel_Jeda = 3;
volatile int currentLevel_2 = 5;
volatile int weldMode = 0; // 0 = Single, 1 = Double

volatile bool welding = false;
volatile bool settingsChanged = true;
int activeSetting = 0;
long lastEncoderCheck = 0;
long lastButtonCheck = 0;

// == Deklarasi Fungsi (Fase 1) ==
void setupOLED();
void setupEncoder();
void updateDisplay();
void doWeld();
void checkEncoder();
void checkEncoderButton();
void checkTriggerButton();

// == Deklarasi Fungsi (Fase 2) ==
void setupWiFi();
void setupWebServer();
void handleGetState(AsyncWebServerRequest *request);
void handleSetLevel(AsyncWebServerRequest *request, JsonVariant &json);
void handleSpot(AsyncWebServerRequest *request);

// ===================================
// == SETUP ==
// ===================================
void setup() {
    Serial.begin(115200);
    Serial.println(F("MOT-CycleControl Booting..."));
    Serial.println(APP_VERSION);

    // Setup Pin
    pinMode(SSR_PIN, OUTPUT);
    digitalWrite(SSR_PIN, LOW);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    // Setup Fase 1
    setupOLED();
    setupEncoder();
    updateDisplay(); // Tampilkan UI awal

    // Setup Fase 2
    setupWiFi();
    setupWebServer();

    Serial.println(F("Setup selesai. Sistem Siap."));
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println(F("OTA di /update"));
}

// ===================================
// == LOOP UTAMA ==
// ===================================
void loop() {
    // ATURAN KRUSIAL: Jika sedang mengelas (delay() aktif), jangan lakukan apapun.
    if (welding) {
        return;
    }

    // Cek input dari UI Fisik
    checkEncoder();
    checkEncoderButton();
    checkTriggerButton();

    // Update OLED hanya jika ada perubahan
    if (settingsChanged) {
        updateDisplay();
        settingsChanged = false; // Reset flag
    }
}

// ===================================
// == FUNGSI-FUNGSI LOGIKA (Fase 1, sedikit modifikasi) ==
// ===================================

void setupOLED() { /* ... (Tidak berubah dari Fase 1) ... */ 
    if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
        Serial.println(F("Gagal inisialisasi SSD1306"));
        while (true); // Berhenti jika OLED gagal
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("MOT-CycleControl"));
    display.setCursor(0, 10);
    display.print(APP_VERSION);
    display.display();
    delay(1500);
}
void setupEncoder() { /* ... (Tidak berubah dari Fase 1) ... */ 
    encoder.setPosition(currentLevel_1); // Atur posisi awal encoder ke nilai P1
}

/**
 * @brief Fungsi inti pengelasan (BLOKING)
 * Sekarang juga memicu 'settingsChanged' untuk update Web UI
 */
void doWeld() {
    // 1. Set flag - mencegah trigger ganda (fisik & web)
    if (welding) return; // Proteksi re-entrancy
    welding = true;
    settingsChanged = true; // Minta update display (untuk status "SPOT!")
    
    // Dry-run 7.3: loop() sekarang diblokir, Web Server tidak akan responsif.
    updateDisplay(); // Update OLED
    // Web UI akan update via polling berikutnya (melihat 'welding: true')

    // Konversi level (siklus) ke milidetik
    long p1_ms = (long)currentLevel_1 * MS_PER_CYCLE;
    long gap_ms = (long)currentLevel_Jeda * MS_PER_CYCLE;
    long p2_ms = (long)currentLevel_2 * MS_PER_CYCLE;

    if (weldMode == 0) {
        // --- MODE SINGLE PULSE ---
        Serial.printf("SPOT! Mode: SINGLE, P1: %d siklus (%ld ms)\n", (int)currentLevel_1, p1_ms);
        
        digitalWrite(SSR_PIN, HIGH);
        delay(p1_ms); // <- BLOKING (Logika Inti Poin 4)
        digitalWrite(SSR_PIN, LOW);

    } else {
        // --- MODE DOUBLE PULSE ---
        Serial.printf("SPOT! Mode: DOUBLE, P1: %d (%ld ms), Jeda: %d (%ld ms), P2: %d (%ld ms)\n",
                      (int)currentLevel_1, p1_ms, (int)currentLevel_Jeda, gap_ms, (int)currentLevel_2, p2_ms);

        // Pulsa 1
        digitalWrite(SSR_PIN, HIGH);
        delay(p1_ms); // <- BLOKING
        digitalWrite(SSR_PIN, LOW);

        // Jeda
        delay(gap_ms); // <- BLOKING

        // Pulsa 2
        digitalWrite(SSR_PIN, HIGH);
        delay(p2_ms); // <- BLOKING
        digitalWrite(SSR_PIN, LOW);
    }

    Serial.println("Las Selesai. Siap.");

    // 2. Reset flag
    welding = false;
    settingsChanged = true; // Minta update display (untuk status "Ready")
}

void checkTriggerButton() { /* ... (Tidak berubah dari Fase 1) ... */ 
    if (digitalRead(TRIGGER_PIN) == LOW) {
        delay(50); // Debounce
        if (digitalRead(TRIGGER_PIN) == LOW) {
            doWeld();
            // Tahan sampai tombol dilepas
            while (digitalRead(TRIGGER_PIN) == LOW) {
                delay(10);
            }
        }
    }
}

/**
 * @brief Memeriksa putaran encoder untuk mengubah nilai
 * (Tidak berubah dari Fase 1)
 */
void checkEncoder() {
    if (millis() - lastEncoderCheck < 50) return; // Cek tiap 50ms
    lastEncoderCheck = millis();

    encoder.tick();
    int newPos = encoder.getPosition();
    volatile int *targetValue; // Pointer ke variabel volatile

    switch (activeSetting) {
        case 0: targetValue = &currentLevel_1; break;
        case 1: targetValue = &currentLevel_Jeda; break;
        case 2: targetValue = &currentLevel_2; break;
        case 3: targetValue = &weldMode; break;
        default: return;
    }

    if (*targetValue != newPos) {
        *targetValue = newPos;

        // Terapkan batasan (constraints)
        if (activeSetting == 3) { // Mode
            if (*targetValue < 0) *targetValue = 0;
            if (*targetValue > 1) *targetValue = 1;
        } else { // Level (P1, Jeda, P2)
            if (*targetValue < MIN_LEVEL) *targetValue = MIN_LEVEL;
            if (*targetValue > MAX_LEVEL) *targetValue = MAX_LEVEL;
        }
        
        encoder.setPosition(*targetValue); // Sinkronkan posisi encoder
        settingsChanged = true;
        Serial.printf("Encoder: Set %d -> %d\n", activeSetting, (int)*targetValue);
    }
}

void checkEncoderButton() { /* ... (Tidak berubah dari Fase 1) ... */ 
    if (millis() - lastButtonCheck < 50) return; // Debounce check
    
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        lastButtonCheck = millis(); // Set timer debounce
        
        while (digitalRead(ENCODER_SW_PIN) == LOW) delay(10); // Tahan sampai dilepas
