// src/main.cpp

#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>

// == Inisialisasi Hardware ==
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RotaryEncoder encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN, RotaryEncoder::LatchMode::FOUR_STEPS);

// == Variabel Status Global ==
// Variabel ini akan disinkronkan dengan Web UI di Fase 2
int currentLevel_1 = 3;   // Default 3 siklus (60ms)
int currentLevel_Jeda = 3; // Default 3 siklus (60ms)
int currentLevel_2 = 5;   // Default 5 siklus (100ms)
int weldMode = 0;         // 0 = Single, 1 = Double

// == Variabel Kontrol UI ==
volatile bool welding = false;          // Flag untuk mencegah las ganda (re-entrancy)
volatile bool settingsChanged = true; // Flag untuk me-refresh OLED
int activeSetting = 0;                  // 0=P1, 1=Jeda, 2=P2, 3=Mode
long lastEncoderCheck = 0;
long lastButtonCheck = 0;

// == Deklarasi Fungsi ==
void setupOLED();
void setupEncoder();
void updateDisplay();
void doWeld();
void checkEncoder();
void checkEncoderButton();
void checkTriggerButton();

// ===================================
// == SETUP ==
// ===================================
void setup() {
    Serial.begin(115200);
    Serial.println(F("MOT-CycleControl Booting..."));
    Serial.println(APP_VERSION);

    // Setup Pin
    pinMode(SSR_PIN, OUTPUT);
    digitalWrite(SSR_PIN, LOW); // Pastikan SSR mati saat boot
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    setupOLED();
    setupEncoder();

    updateDisplay(); // Tampilkan UI awal
    Serial.println(F("Setup selesai. Sistem Siap."));
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
// == FUNGSI-FUNGSI LOGIKA ==
// ===================================

/**
 * @brief Inisialisasi OLED
 */
void setupOLED() {
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

/**
 * @brief Inisialisasi Encoder
 */
void setupEncoder() {
    encoder.setPosition(currentLevel_1); // Atur posisi awal encoder ke nilai P1
}

/**
 * @brief Fungsi inti pengelasan (BLOKING)
 */
void doWeld() {
    // 1. Set flag - mencegah trigger ganda
    welding = true;
    settingsChanged = true; // Minta update display (untuk status "SPOT!")
    
    // Dry-run 7.3: loop() sekarang diblokir, Web Server (di Fase 2) tidak akan responsif.
    updateDisplay(); 

    // Konversi level (siklus) ke milidetik
    long p1_ms = (long)currentLevel_1 * MS_PER_CYCLE;
    long gap_ms = (long)currentLevel_Jeda * MS_PER_CYCLE;
    long p2_ms = (long)currentLevel_2 * MS_PER_CYCLE;

    if (weldMode == 0) {
        // --- MODE SINGLE PULSE ---
        Serial.printf("SPOT! Mode: SINGLE, P1: %d siklus (%ld ms)\n", currentLevel_1, p1_ms);
        
        digitalWrite(SSR_PIN, HIGH);
        delay(p1_ms); // <- BLOKING (Logika Inti Poin 4)
        digitalWrite(SSR_PIN, LOW);

    } else {
        // --- MODE DOUBLE PULSE ---
        Serial.printf("SPOT! Mode: DOUBLE, P1: %d (%ld ms), Jeda: %d (%ld ms), P2: %d (%ld ms)\n",
                      currentLevel_1, p1_ms, currentLevel_Jeda, gap_ms, currentLevel_2, p2_ms);

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

/**
 * @brief Memeriksa pemicu las fisik
 */
void checkTriggerButton() {
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
 */
void checkEncoder() {
    if (millis() - lastEncoderCheck < 50) return; // Cek tiap 50ms
    lastEncoderCheck = millis();

    encoder.tick();
    int newPos = encoder.getPosition();
    int *targetValue; // Pointer ke variabel yang sedang diedit

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
        Serial.printf("Encoder: Set %d -> %d\n", activeSetting, *targetValue);
    }
}

/**
 * @brief Memeriksa tombol encoder untuk mengganti setelan aktif
 */
void checkEncoderButton() {
    if (millis() - lastButtonCheck < 50) return; // Debounce check
    
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        lastButtonCheck = millis(); // Set timer debounce
        
        // Tahan sampai tombol dilepas
        while (digitalRead(ENCODER_SW_PIN) == LOW) {
            delay(10);
        }
        
        // Ganti setelan aktif
        activeSetting++;
        if (activeSetting > 3) activeSetting = 0; // 0=P1, 1=Jeda, 2=P2, 3=Mode

        // Atur posisi encoder ke nilai setelan baru
        switch (activeSetting) {
            case 0: encoder.setPosition(currentLevel_1); break;
            case 1: encoder.setPosition(currentLevel_Jeda); break;
            case 2: encoder.setPosition(currentLevel_2); break;
            case 3: encoder.setPosition(weldMode); break;
        }
        
        settingsChanged = true;
        Serial.printf("Tombol Encoder: Mengganti setelan aktif ke %d\n", activeSetting);
    }
}

/**
 * @brief Menggambar ulang seluruh UI di OLED
 */
void updateDisplay() {
    display.clearDisplay();

    if (welding) {
        display.setTextSize(3);
        display.setCursor(25, 20);
        display.print(F("SPOT!"));
        display.display();
        return;
    }

    // Tampilkan Mode
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Mode:"));
    display.setCursor(70, 0);
    display.print(weldMode == 0 ? F("SINGLE") : F("DOUBLE"));
    if (activeSetting == 3) { // Tunjukkan jika aktif
        display.drawRect(68, 0, 60, 10, SSD1306_WHITE);
    }

    // Tampilkan P1
    display.setCursor(0, 15);
    display.printf("P1  : %d siklus (%d ms)", currentLevel_1, currentLevel_1 * MS_PER_CYCLE);
    if (activeSetting == 0) {
        display.drawRect(0, 14, 128, 10, SSD1306_WHITE);
    }

    // Tampilkan Jeda (hanya jika mode Double)
    display.setCursor(0, 30);
    if (weldMode == 1) {
        display.printf("Jeda: %d siklus (%d ms)", currentLevel_Jeda, currentLevel_Jeda * MS_PER_CYCLE);
        if (activeSetting == 1) {
            display.drawRect(0, 29, 128, 10, SSD1306_WHITE);
        }
    } else {
        display.print(F("Jeda: N/A"));
    }

    // Tampilkan P2 (hanya jika mode Double)
    display.setCursor(0, 45);
    if (weldMode == 1) {
        display.printf("P2  : %d siklus (%d ms)", currentLevel_2, currentLevel_2 * MS_PER_CYCLE);
        if (activeSetting == 2) {
            display.drawRect(0, 44, 128, 10, SSD1306_WHITE);
        }
    } else {
        display.print(F("P2  : N/A"));
    }

    // Status
    display.setCursor(0, 56);
    display.print(F("Status: SIAP"));

    display.display();
}
