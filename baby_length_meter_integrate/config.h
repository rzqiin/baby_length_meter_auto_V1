/*
 * config.h
 * Konfigurasi pin, konstanta sistem, dan struct data
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ================================================================
//  PIN KONFIGURASI - TFT ILI9341
// ================================================================
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
// MOSI -> GPIO 23 (SPI default ESP32)
// SCK  -> GPIO 18 (SPI default ESP32)
// VCC  -> 3.3V | GND -> GND | LED -> 3.3V

// ================================================================
//  PIN KONFIGURASI - SENSOR ULTRASONIK HC-SR04
// ================================================================
#define TRIG_PIN  26
#define ECHO_PIN  27

// ================================================================
//  PIN KONFIGURASI - LOAD CELL HX711
// ================================================================
#define HX711_DOUT  34    // Data out HX711 -> GPIO 16
#define HX711_SCK   32    // Clock HX711    -> GPIO 17
// CATATAN: Pin 23 (MOSI TFT) dan 22 tidak bisa dipakai untuk HX711
// Kalibrasi: ukur benda berat diketahui, sesuaikan LOADCELL_CALIB
#define LOADCELL_CALIB   -23.0f  // Faktor kalibrasi (sama dengan referensi)
                                  // Negatif = output HX711 terbalik
                                  // Cara kalibrasi: timbang benda 1kg,
                                  // sesuaikan nilai ini sampai baca 1000g

// ================================================================
//  PIN KONFIGURASI - ROTARY ENCODER INCREMENTAL
// ================================================================
#define ENCODER_CLK  25   // GPIO 25 - mendukung INPUT_PULLUP internal
#define ENCODER_DT   33   // GPIO 33 - mendukung INPUT_PULLUP internal
// GPIO 25 ada di register GPIO_IN_REG  (bank 0: GPIO 0-31)
// GPIO 33 ada di register GPIO_IN1_REG (bank 1: GPIO 32-39)
// Tidak perlu resistor pull-up eksternal.

// ================================================================
//  KONFIGURASI PENGUKURAN
// ================================================================
#define BASE_LENGTH       100.0f  // Panjang total papan pengukuran (cm)
#define PULSE_PER_CM       20.0f  // Pulse encoder per 1 cm (kalibrasi sesuai hardware)
                                  // Cara kalibrasi: geser slider 10cm, catat pulse,
                                  // bagi pulse/10 -> isi nilai ini.

#define US_SAMPLES           5    // Jumlah sampel rata-rata ultrasonik
#define US_MAX_DIST       200.0f  // Jarak maksimum valid ultrasonik (cm)
#define US_MIN_DIST         2.0f  // Jarak minimum valid ultrasonik (cm)
#define US_TIMEOUT_US     30000   // Timeout pulseIn (30ms = ~500cm)

#define MAX_BABY_LENGTH   100.0f  // Batas maksimum panjang bayi (cm)
#define MIN_BABY_LENGTH     0.0f  // Batas minimum panjang bayi (cm)

// ================================================================
//  KONFIGURASI WIFI
// ================================================================
#define AP_SSID      "BABY_MEASURE_SETUP"  // Nama hotspot saat setup WiFi
#define AP_PASSWORD  ""                    // Kosong = hotspot terbuka

// ================================================================
//  KONFIGURASI GOOGLE SPREADSHEET
// ================================================================
// Ganti dengan URL Web App Google Apps Script Anda setelah deploy!
// Format: https://script.google.com/macros/s/SCRIPT_ID/exec
#define GOOGLE_SCRIPT_URL  "https://script.google.com/macros/s/AKfycbw3G_KR_cF4pnG24WXz-JNJpscdBeFmfbj3VRikQqg4KsnCXqoIK_qVYS-d2wMTe8G4/exec"

// ================================================================
//  WARNA TFT (RGB565)
// ================================================================
#define C_BG          0x0010   // Background utama - biru sangat gelap
#define C_HEADER      0x0228   // Header - biru medium
#define C_ACCENT      0x07FF   // Aksen - cyan
#define C_ACCENT2     0xF81F   // Aksen2 - magenta
#define C_WHITE       0xFFFF   // Putih
#define C_GRAY        0x7BEF   // Abu-abu
#define C_DARK_GRAY   0x39E7   // Abu-abu gelap
#define C_GREEN       0x07E0   // Hijau - ultrasonik / OK
#define C_ORANGE      0xFD20   // Oranye - encoder
#define C_YELLOW      0xFFE0   // Kuning - panjang bayi
#define C_RED         0xF800   // Merah - error / warning
#define C_BOX_BG      0x0451   // Background box sensor
#define C_RESULT_BG   0x1082   // Background box hasil
#define C_TEAL        0x0410   // Teal gelap

// ================================================================
//  STRUCT DATA
// ================================================================

// Data pengukuran sensor
struct MeasurementData {
  float ultrasonicCm  = 0.0f;
  float encoderCm     = 0.0f;
  float babyLength    = 0.0f;
  float weightKg      = 0.0f;   // Berat bayi dari load cell (kg)
  bool  ultrasonicErr = false;
  bool  weightReady   = false;  // Flag HX711 sudah ada data

  // Nilai terakhir ditampilkan (untuk deteksi perubahan)
  float lastUltrasonic = -999.0f;
  float lastEncoder    = -999.0f;
  float lastBaby       = -999.0f;
  float lastWeight     = -999.0f;
};

// Hasil analisis fuzzy logic
struct FuzzyAnalysis {
  float   bmi        = 0.0f;
  float   bbu        = 0.0f;
  float   pbu        = 0.0f;
  float   imtu       = 0.0f;
  String  statusBBU  = "-";
  String  statusPBU  = "-";
  String  statusIMTU = "-";
  bool    hasResult  = false;  // true jika sudah pernah dihitung
};

// Data pasien
struct PatientData {
  String name   = "";
  String age    = "";
  String gender = "L";
};

// Status sistem
struct SystemStatus {
  bool   wifiConnected = false;
  String ipAddress     = "0.0.0.0";
  bool   patientSet    = false;
  bool   lastSaveOk    = false;
  bool   saving        = false;
};

#endif // CONFIG_H
