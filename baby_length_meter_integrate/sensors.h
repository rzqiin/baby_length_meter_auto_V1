/*
 * sensors.h
 * Modul pembacaan sensor ultrasonik HC-SR04 dan rotary encoder
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include <HX711_ADC.h>

// ================================================================
//  OBJEK LOAD CELL
// ================================================================
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

// ================================================================
//  VARIABEL ENCODER (volatile karena diakses dari ISR)
// ================================================================
volatile long    encoderCount = 0;
volatile uint8_t encoderState = 0;  // 2-bit quadrature state: bit1=CLK, bit0=DT

// ================================================================
//  HELPER: Baca pin GPIO via register langsung (aman di ISR)
// ================================================================
/*
 * ESP32 memiliki DUA register GPIO input:
 *   GPIO_IN_REG  -> untuk GPIO  0 - 31  (baca bit ke-N)
 *   GPIO_IN1_REG -> untuk GPIO 32 - 39  (baca bit ke-(N-32))
 *
 * Pin yang digunakan:
 *   ENCODER_CLK = GPIO 25 -> GPIO_IN_REG,  bit 25
 *   ENCODER_DT  = GPIO 33 -> GPIO_IN1_REG, bit 1  (33-32)
 *
 * PENTING: Jangan pakai formula seragam (pin-32) untuk semua pin!
 * GPIO 25 ada di bank 0 (0-31), bukan bank 1 (32-39).
 */
static inline int IRAM_ATTR readCLK() {
  // GPIO 25 -> GPIO_IN_REG, bit 25
  return (REG_READ(GPIO_IN_REG) >> ENCODER_CLK) & 1;
}

static inline int IRAM_ATTR readDT() {
  // GPIO 33 -> GPIO_IN1_REG, bit (33-32) = bit 1
  return (REG_READ(GPIO_IN1_REG) >> (ENCODER_DT - 32)) & 1;
}

// ================================================================
//  ISR - ROTARY ENCODER: Quadrature State Machine
// ================================================================
/*
 * Quadrature decoder menggunakan lookup table 4x4.
 *
 * State 2-bit: bit1=CLK, bit0=DT
 * Urutan CW  (maju): 00 -> 01 -> 11 -> 10 -> 00
 * Urutan CCW (mundur): 00 -> 10 -> 11 -> 01 -> 00
 *
 * Tabel [prev_state * 4 + new_state]:
 *   +1 = transisi CW valid
 *   -1 = transisi CCW valid
 *    0 = tidak bergerak / noise / bouncing (diabaikan)
 */
static const int8_t ENCODER_TABLE[16] = {
  //curr: 00   01   10   11
          0,   1,  -1,   0,   // prev=00
         -1,   0,   0,   1,   // prev=01
          1,   0,   0,  -1,   // prev=10
          0,  -1,   1,   0    // prev=11
};

void IRAM_ATTR encoderISR() {
  // Baca kedua pin via register hardware (cepat & reliable di ISR)
  int clk = readCLK();
  int dt  = readDT();

  // Bangun state baru dan lookup arah
  uint8_t newState = (clk << 1) | dt;
  uint8_t idx      = (encoderState << 2) | newState;
  int8_t  dir      = ENCODER_TABLE[idx];

  if (dir != 0) {
    encoderCount += dir;
  }

  encoderState = newState;
}

// ================================================================
//  INISIALISASI SENSOR
// ================================================================
void initSensors() {
  // --- Ultrasonik HC-SR04 ---
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // --- Rotary Encoder ---
  // GPIO 25 & 33: GPIO normal yang MENDUKUNG INPUT_PULLUP internal.
  // Tidak perlu resistor pull-up eksternal (berbeda dengan GPIO 34/35).
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT,  INPUT_PULLUP);

  // Inisialisasi state awal quadrature
  int clk0 = digitalRead(ENCODER_CLK);
  int dt0  = digitalRead(ENCODER_DT);
  encoderState = (clk0 << 1) | dt0;

  // Interrupt pada KEDUA pin (CLK & DT) untuk resolusi penuh
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT),  encoderISR, CHANGE);
}

// ================================================================
//  FUNGSI: readUltrasonic()
// ================================================================
/**
 * Membaca jarak dari sensor ultrasonik HC-SR04.
 * Mengambil rata-rata dari US_SAMPLES pengukuran untuk stabilitas.
 *
 * Prinsip kerja:
 *   1. Kirim pulsa TRIG 10μs
 *   2. Ukur durasi pulsa ECHO
 *   3. Jarak (cm) = durasi(μs) × 0.0343 / 2
 *      (0.0343 cm/μs = kecepatan suara; /2 karena pulang-pergi)
 *
 * @return float Jarak dalam cm, atau -1.0 jika error/timeout
 */
float readUltrasonic() {
  float totalDist   = 0.0f;
  int   validCount  = 0;

  for (int i = 0; i < US_SAMPLES; i++) {
    // Trigger: LOW -> HIGH (10μs) -> LOW
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Baca durasi echo (timeout = US_TIMEOUT_US)
    long duration = pulseIn(ECHO_PIN, HIGH, US_TIMEOUT_US);

    // Konversi ke cm
    float dist = (duration * 0.0343f) / 2.0f;

    // Validasi rentang
    if (duration > 0 && dist >= US_MIN_DIST && dist <= US_MAX_DIST) {
      totalDist += dist;
      validCount++;
    }

    delay(10); // Jeda antar sampel mencegah echo saling ganggu
  }

  if (validCount > 0) {
    return totalDist / (float)validCount;
  }
  return -1.0f; // Error: tidak ada pembacaan valid
}

// ================================================================
//  FUNGSI: readEncoder()
// ================================================================
/**
 * Membaca posisi slider encoder dan mengkonversi ke centimeter.
 *
 * Koordinat sistem:
 *   - Encoder di ujung KAKI papan (nilai 0 = slider di posisi awal)
 *   - Slider digeser ke arah KEPALA -> encoderCount bertambah
 *   - posisi_dari_kaki = encoderCount / PULSE_PER_CM
 *   - posisi_dari_kepala = BASE_LENGTH - posisi_dari_kaki
 *
 * Mengapa BASE_LENGTH - posisi?
 *   Karena kita butuh jarak slider dari sisi KEPALA papan,
 *   bukan dari sisi kaki.
 *
 * @return float Posisi slider dalam cm dari sisi kepala (0 - 100)
 */
float readEncoder() {
  // Baca atomic (nonaktifkan interrupt sementara)
  noInterrupts();
  long count = encoderCount;
  interrupts();

  // Konversi pulse -> cm
  float cmFromFoot = (float)count / PULSE_PER_CM;

  // Konversi ke posisi dari sisi kepala
  float cmFromHead = BASE_LENGTH - cmFromFoot;

  // Clamp ke rentang valid
  cmFromHead = constrain(cmFromHead, 0.0f, BASE_LENGTH);

  return cmFromHead;
}

// ================================================================
//  FUNGSI: calculateBabyLength()
// ================================================================
/**
 * Menghitung panjang bayi dari data sensor.
 *
 * Formula: panjang_bayi = posisi_encoder - jarak_ultrasonik
 *
 * Visualisasi:
 *   [US Sensor] <--US_dist--> [Kepala] ......... [Kaki] <--[Encoder]
 *   |<---------- encoderPos (dari kepala) ----------->|
 *                             |<---- babyLength ------>|
 *
 * Validasi:
 *   - Jika ultrasonik error (< 0): return 0
 *   - Hasil negatif: clamp ke 0
 *   - Hasil > 100 cm: clamp ke 100
 *
 * @param ultrasonicDist Jarak ultrasonik ke kepala (cm)
 * @param encoderPos     Posisi slider dari sisi kepala (cm)
 * @return float         Panjang bayi (0.0 - 100.0 cm)
 */
float calculateBabyLength(float ultrasonicDist, float encoderPos) {
  // Jika ultrasonik error, return 0
  if (ultrasonicDist < 0.0f) return 0.0f;

  float length = encoderPos - ultrasonicDist;

  // Validasi: tidak boleh negatif
  if (length < MIN_BABY_LENGTH) length = MIN_BABY_LENGTH;

  // Validasi: tidak boleh melebihi panjang papan
  if (length > MAX_BABY_LENGTH) length = MAX_BABY_LENGTH;

  return length;
}

// ================================================================
//  FUNGSI: resetEncoder()
// ================================================================
/**
 * Reset hitungan encoder ke nol.
 * Panggil ini saat slider dikembalikan ke posisi awal (ujung kaki).
 */
void resetEncoder() {
  noInterrupts();
  encoderCount = 0;
  interrupts();
}

// ================================================================
//  FUNGSI: initLoadCell()
// ================================================================
/*
 * Inisialisasi HX711 ADC untuk load cell.
 * Lakukan tare otomatis saat startup (pastikan timbangan kosong).
 *
 * Library: HX711_ADC by Olav Kallhovd
 * Install via Library Manager: search "HX711_ADC"
 */
void initLoadCell() {
  LoadCell.begin();
  LoadCell.start(2000, true); // Stabilisasi 2 detik + tare otomatis

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("[HX711] TIMEOUT! Cek wiring HX711.");
  } else {
    LoadCell.setCalFactor(LOADCELL_CALIB);
    Serial.println("[OK] HX711 Load Cell siap.");
  }

  // Tunggu data pertama tersedia
  while (!LoadCell.update());
}

// ================================================================
//  FUNGSI: readLoadCell()
// ================================================================
/*
 * Membaca berat dari HX711 load cell.
 * HX711_ADC menggunakan pola update non-blocking:
 *   - LoadCell.update() dipanggil setiap loop
 *   - LoadCell.getData() ambil nilai saat update() return true
 *
 * @return float Berat dalam kilogram (>= 0)
 */
float readLoadCell() {
  if (LoadCell.update()) {
    float raw = LoadCell.getData(); // Nilai dalam gram
    float kg  = raw / 1000.0f;
    if (kg < 0.0f) kg = 0.0f;      // Validasi tidak boleh negatif
    return kg;
  }
  return -1.0f; // -1 = belum ada data baru
}

// ================================================================
//  FUNGSI: tareLoadCell()
// ================================================================
/*
 * Reset tare / nol-kan timbangan.
 * Panggil saat timbangan kosong sebelum letakkan bayi.
 */
void tareLoadCell() {
  LoadCell.tareNoDelay();
  Serial.println("[HX711] Tare dimulai...");
}

// ================================================================
//  FUNGSI: isTareComplete()
// ================================================================
bool isTareComplete() {
  if (LoadCell.getTareStatus()) {
    Serial.println("[HX711] Tare selesai.");
    return true;
  }
  return false;
}

#endif // SENSORS_H
