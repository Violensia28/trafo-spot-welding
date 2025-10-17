/*
 * =================================================================
 * ESP32 MOT SPOT WELDER - FIRMWARE LENGKAP DENGAN WEB UI
 * =================================================================
 * Target: PlatformIO
 * File: src/main.cpp
 *
 * Fitur:
 * - Access Point (AP) Wi-Fi (Nama: "ESP32-Spot-Welder")
 * - Web Server di 192.168.4.1
 * - Kontrol Level (1-10) dan Trigger Las via Web UI
 * - Kontrol Phase Control (Anti-Jeglek) menggunakan ZCD
 * - Double Pulse (Pre-weld, Pause, Main-weld)
 *
 * Hardware Wajib:
 * 1. ESP32
 * 2. Modul ZCD (Zero-Crossing Detector)
 * 3. Random-Fire SSR (Solid State Relay)
 * 4. Trafo MOT
 *
 * Library yang Dibutuhkan (via platformio.ini):
 * 1. WiFi
 * 2. ESPAsyncWebServer
 * 3. AsyncTCP
 * =================================================================
 */

// PlatformIO akan otomatis menyertakan <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- KONFIGURASI PIN (SESUAIKAN) ---
#define SSR_PIN 5        // Pin Output ke Random-Fire SSR
#define ZCD_PIN 15       // Pin Input dari Modul ZCD (HARUS pin interrupt!)

// --- KONFIGURASI WI-FI ACCESS POINT ---
const char* ssid = "ESP32-Spot-Welder";
const char* password = "password123"; // Ganti password ini

// --- KONFIGURASI PARAMETER LAS (WAJIB TUNING) ---
// Delay dari ZCD (0V) ke Puncak Gelombang (50Hz = 5ms = 5000us)
#define PHASE_DELAY_US 5000 
// Durasi Pre-weld (dalam 1 cycle = 20ms)
#define PRE_WELD_CYCLES 1   
// Jeda antara 2 pulse (dalam 1 cycle = 20ms)
#define PAUSE_CYCLES 5      

// --- Web Server Setup ---
AsyncWebServer server(80); // Server berjalan di port 80

// --- VARIABEL GLOBAL UNTUK PROSES LAS ---
// 'volatile' WAJIB untuk variabel yang dipakai di dalam ISR (Interrupt)
volatile int pulseLevel = 1; // Level 1-10, dikontrol dari Web UI
volatile int main_weld_cycles = 2; // Nilai awal (akan di-map dari pulseLevel)

// State machine untuk proses las
enum WeldState { IDLE, PRE_WELD, PAUSE, MAIN_WELD };
volatile WeldState currentState = IDLE;

volatile int halfCycleCounter = 0; // Penghitung 10ms (half-cycle)
volatile int preWeldDuration = PRE_WELD_CYCLES * 2; // (kali 2 karena ZCD 10ms)
volatile int pauseDuration = PAUSE_CYCLES * 2;
volatile int mainWeldDuration = main_weld_cycles * 2;


// =================================================================
//  HALAMAN WEB (HTML, CSS, JS) TERTANAM DALAM KODE
// =================================================================
// Menggunakan R"rawliteral(...)rawliteral" agar bisa copy-paste HTML
// tanpa perlu 'escaping' karakter.
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Spot Welder</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
            background-color: #f0f2f5; 
            color: #333;
            display: flex; 
            justify-content: center; 
            align-items: center; 
            height: 100vh; 
            margin: 0;
            -webkit-tap-highlight-color: transparent;
        }
        .container { 
            background: #fff; 
            padding: 2rem; 
            border-radius: 10px; 
            box-shadow: 0 4px 12px rgba(0,0,0,0.1); 
            text-align: center; 
            width: 90%; 
            max-width: 400px;
        }
        h1 { 
            color: #111; 
            margin-top: 0;
        }
        #weld-button {
            background-color: #e74c3c; 
            color: white; 
            font-size: 1.5rem; 
            font-weight: bold;
            padding: 1rem 2rem; 
            border: none; 
            border-radius: 5px; 
            cursor: pointer; 
            width: 100%;
            transition: background-color 0.2s, transform 0.1s;
            user-select: none;
        }
        #weld-button:hover { 
            background-color: #c0392b; 
        }
        #weld-button:active {
            transform: scale(0.98);
        }
        #weld-button:disabled {
            background-color: #95a5a6;
            cursor: not-allowed;
        }
        .slider-container { 
            margin: 2rem 0; 
        }
        input[type=range] {
            width: 100%;
            cursor: pointer;
            height: 15px;
            -webkit-appearance: none;
            background: #ddd;
            border-radius: 5px;
        }
        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 25px;
            height: 25px;
            background: #3498db;
            border-radius: 50%;
            cursor: pointer;
        }
        input[type=range]::-moz-range-thumb {
            width: 25px;
            height: 25px;
            background: #3498db;
            border-radius: 50%;
            cursor: pointer;
        }
        #level-display {
            font-size: 2.5rem;
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 0.5rem;
        }
        #status {
            margin-top: 1rem;
            font-size: 1rem;
            color: #7f8c8d;
            height: 1.2em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Spot Welder</h1>
        
        <div id="level-display">
            Level <span id="level-value">1</span>
        </div>
        
        <div class="slider-container">
            <input type="range" min="1" max="10" value="1" id="level-slider">
        </div>
        
        <button id="weld-button">SPOT WELD</button>
        
        <div id="status" id="status-message"></div>
    </div>

    <script>
        // Ambil elemen UI
        const slider = document.getElementById('level-slider');
        const levelValue = document.getElementById('level-value');
        const weldButton = document.getElementById('weld-button');
        const statusMsg = document.getElementById('status');
        let currentLevel = 1;

        // Fungsi untuk fetch level saat pertama kali load
        window.onload = () => {
            fetch('/level')
                .then(response => response.text())
                .then(data => {
                    currentLevel = parseInt(data, 10);
                    slider.value = currentLevel;
                    levelValue.innerText = currentLevel;
                });
        };

        // Kirim level baru saat slider dilepas (onchange)
        slider.onchange = () => {
            const level = slider.value;
            levelValue.innerText = level;
            
            // Kirim data ke ESP32
            fetch('/setlevel?level=' + level, { method: 'POST' })
                .then(response => {
                    if(response.ok) {
                        currentLevel = level;
                        console.log("Level set to " + level);
                    }
                });
        };
        
        // Update tampilan angka saat slider digeser (oninput)
        slider.oninput = () => {
            levelValue.innerText = slider.value;
        };

        // Kirim perintah WELD saat tombol ditekan
        weldButton.onclick = () => {
            weldButton.disabled = true;
            weldButton.innerText = "WELDING...";
            statusMsg.innerText = "Proses las...";

            fetch('/weld', { method: 'POST' })
                .then(response => {
                    if (response.ok) {
                        return response.text();
                    } else {
                        throw new Error('Welder is busy.');
                    }
                })
                .then(data => {
                    console.log(data);
                    statusMsg.innerText = "Selesai!";
                })
                .catch(error => {
                    console.error('Error:', error);
                    statusMsg.innerText = "Gagal: Welder sedang sibuk.";
                })
                .finally(() => {
                    // Beri jeda sedikit sebelum mengaktifkan tombol lagi
                    setTimeout(() => {
                        weldButton.disabled = false;
                        weldButton.innerText = "SPOT WELD";
                        statusMsg.innerText = "";
                    }, 1000); // 1 detik
                });
        };
    </script>
</body>
</html>
)rawliteral";


// =================================================================
//  INTERRUPT SERVICE ROUTINE (ISR) - JANTUNG KENDALI
// =================================================================
// Fungsi ini dieksekusi setiap kali ZCD mendeteksi 0 Volt (100x per detik)
// ICACHE_RAM_ATTR -> Simpan fungsi ini di RAM agar eksekusi super cepat
void ICACHE_RAM_ATTR zcdInterrupt() {
  
  // Jika tidak sedang ngelas (IDLE), matikan SSR dan keluar
  if (currentState == IDLE) {
    digitalWrite(SSR_PIN, LOW);
    return;
  }
  
  // Hitung setiap deteksi zero-cross (setiap 10ms)
  halfCycleCounter++;

  // --- STATE MACHINE PROSES LAS ---

  // 1. STATE PRE-WELD
  if (currentState == PRE_WELD) {
    if (halfCycleCounter <= preWeldDuration) {
      // INI SOLUSINYA: Tembak SSR di puncak gelombang (setelah delay)
      delayMicroseconds(PHASE_DELAY_US);
      digitalWrite(SSR_PIN, HIGH);
    } else {
      // Selesai pre-weld, lanjut ke PAUSE
      digitalWrite(SSR_PIN, LOW);
      currentState = PAUSE;
      halfCycleCounter = 0;
    }
  } 
  
  // 2. STATE PAUSE
  else if (currentState == PAUSE) {
    digitalWrite(SSR_PIN, LOW); // Pastikan SSR mati selama jeda
    if (halfCycleCounter >= pauseDuration) {
      // Selesai jeda, lanjut ke MAIN-WELD
      currentState = MAIN_WELD;
      halfCycleCounter = 0;
    }
  } 
  
  // 3. STATE MAIN-WELD
  else if (currentState == MAIN_WELD) {
    if (halfCycleCounter <= mainWeldDuration) {
      // Tembak lagi SSR di puncak gelombang
      delayMicroseconds(PHASE_DELAY_US);
      digitalWrite(SSR_PIN, HIGH);
    } else {
      // Selesai! Kembali ke IDLE
      digitalWrite(SSR_PIN, LOW);
      currentState = IDLE;
      halfCycleCounter = 0;
    }
  }
}

// =================================================================
//  FUNGSI SETUP (DIJALANKAN SEKALI SAAT BOOT)
// =================================================================
void setup() {
  Serial.begin(115200);
  
  // Setup Pin I/O
  pinMode(SSR_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(SSR_PIN, LOW); // Pastikan SSR mati saat boot

  // --- Setup Wi-Fi Access Point (AP) ---
  Serial.println("\nStarting AP...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address (Buka di Browser HP): ");
  Serial.println(IP); // Ini adalah alamat yg dibuka di browser (default: 192.168.4.1)

  // --- Web Server Handlers (Rute URL) ---
  // Rute-rute ini menentukan apa yang terjadi jika browser
  // mengakses URL tertentu.

  // 1. Halaman Utama (/)
  // Saat browser buka http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Kirim halaman HTML yang sudah kita simpan di 'index_html'
    request->send_P(200, "text/html", index_html);
  });

  // 2. Endpoint untuk mengambil level saat ini (untuk sync UI)
  server.on("/level", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(pulseLevel));
  });

  // 3. Endpoint untuk mengatur level (dari slider)
  // Saat slider digeser, JS akan memanggil http://192.168.4.1/setlevel
  server.on("/setlevel", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("level")) {
      int newLevel = request->getParam("level")->value().toInt();
      
      // Batasi nilai antara 1 dan 10
      if (newLevel < 1) newLevel = 1;
      if (newLevel > 10) newLevel = 10;
      
      pulseLevel = newLevel;

      // Update durasi las (harus aman dari interrupt)
      noInterrupts(); // Matikan interrupt sementara
      // Map Level 1-10 ke 2-20 cycles (sesuaikan jika perlu)
      main_weld_cycles = map(pulseLevel, 1, 10, 2, 20); 
      mainWeldDuration = main_weld_cycles * 2;
      interrupts(); // Nyalakan lagi
      
      Serial.print("Level diatur ke: ");
      Serial.println(pulseLevel);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // 4. Endpoint untuk memicu las (dari tombol)
  // Saat tombol WELD ditekan, JS akan memanggil http://192.168.4.1/weld
  server.on("/weld", HTTP_POST, [](AsyncWebServerRequest *request){
    if (currentState == IDLE) {
      // Jika welder sedang nganggur, mulai proses las
      currentState = PRE_WELD;
      halfCycleCounter = 0;
      
      Serial.print("WELDING dipicu dari WEB! Level: ");
      Serial.println(pulseLevel);
      
      request->send(200, "text/plain", "Welding started...");
    } else {
      // Jika sedang sibuk, kirim error 'Conflict'
      Serial.println("Gagal trigger. Sedang proses las.");
      request->send(409, "text/plain", "Welder is busy.");
    }
  });

  // Handle 404 Not Found
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  // Mulai server
  server.begin();
  Serial.println("Web server started.");

  // --- Pasang Interrupt ZCD ---
  // Ini harus di akhir setup
  // Memanggil fungsi 'zcdInterrupt()' setiap kali pin ZCD_PIN 'naik' (RISING)
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), zcdInterrupt, RISING);
  Serial.println("ZCD Interrupt terpasang. Sistem Siap.");
}

// =================================================================
//  FUNGSI LOOP (DIJALANKAN BERULANG-ULANG)
// =================================================================
void loop() {
  // Tidak perlu melakukan apa-apa di sini.
  // Web Server (Async) berjalan di background.
  // Proses Las (Interrupt) berjalan independen.
}
