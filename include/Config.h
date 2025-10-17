// include/Config.h

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// == Versi Firmware ==
#define APP_VERSION "v1.0-Fase1"

// == Pengaturan Inti ==
#define MS_PER_CYCLE 20   // 50Hz -> 1 siklus = 20ms
#define MIN_LEVEL 1
#define MAX_LEVEL 99

// == Pinout Hardware ==
// -- UI Fisik
#define OLED_SDA 21         // Pin I2C SDA
#define OLED_SCL 22         // Pin I2C SCL
#define ENCODER_CLK_PIN 19  // Encoder CLK (Pin A)
#define ENCODER_DT_PIN 18   // Encoder DT (Pin B)
#define ENCODER_SW_PIN 5    // Tombol Push Encoder (SW)
#define TRIGGER_PIN 23      // Pemicu (Foot Switch / Tombol)

// -- Kontrol SSR
#define SSR_PIN 26          // Pin kontrol ke jalur 5V SSR (sesuai spek)

// == Pengaturan OLED ==
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // Ganti ke 32 jika Anda pakai 128x32
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C // Alamat I2C OLED (biasanya 0x3C atau 0x3D)

#endif // CONFIG_H
