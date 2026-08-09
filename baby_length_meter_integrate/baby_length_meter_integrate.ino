/*
 * ================================================================
 *  BABY LENGTH METER - Pengukur Panjang Bayi Digital IoT
 * ================================================================
 *  ESP32 + TFT ILI9341 + HC-SR04 + Rotary Encoder
 *  + WiFiManager + WebServer + Google Spreadsheet Logger
 *
 *  Arsitektur Pengukuran:
 *    [Sensor US] <--jarak_US--> [Kepala] ........... [Kaki] <--[Encoder Slider]
 *    |<------- posisi_encoder (dari sisi kepala) -------->|
 *                               |<-- panjang_bayi ------->|
 *
 *  Formula: panjang_bayi = posisi_encoder - jarak_ultrasonik
 *  Base papan = 100 cm
 *
 *  FILE STRUCTURE:
 *    baby_length_meter.ino  <- file utama ini
 *    config.h               <- konfigurasi pin & konstanta
 *    display.h/.cpp         <- fungsi TFT
 *    sensors.h/.cpp         <- ultrasonik & encoder
 *    webui.h                <- HTML web interface
 *    spreadsheet.h/.cpp     <- Google Sheets logger
 *
 *  LIBRARY (install via Library Manager):
 *    - WiFiManager by tzapu  (v2.0.x)
 *    - Adafruit GFX Library
 *    - Adafruit ILI9341
 *
 *  Author  : Baby Length Meter IoT Project
 *  Version : 2.0
 * ================================================================
 */

// ================================================================
//  LIBRARY SYSTEM (urutan penting: sistem dulu, lokal kemudian)
// ================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <HX711_ADC.h>
#include <Fuzzy.h>

// ================================================================
//  HEADER LOKAL (urutan: config -> sensors -> display -> ui -> net)
// ================================================================
#include "config.h"
#include "fuzzy_logic.h"
#include "sensors.h"
#include "display.h"
#include "spreadsheet.h"
#include "webui.h"

// ================================================================
//  OBJEK GLOBAL
// ================================================================
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
WebServer        server(80);
WiFiManager      wifiManager;

// ================================================================
//  DATA GLOBAL PENGUKURAN & PASIEN
// ================================================================
MeasurementData measurement;   // struct: ultrasonik, encoder, panjang, berat
PatientData     patient;       // struct: nama, usia, gender
SystemStatus    sysStatus;     // struct: wifi, save state, dll
FuzzyAnalysis   fuzzyResult;   // struct: hasil analisis fuzzy logic

// ================================================================
//  TIMING
// ================================================================
unsigned long lastSensorRead  = 0;
unsigned long lastDisplayRefresh = 0;
const unsigned long SENSOR_INTERVAL   = 150;   // ms - baca sensor
const unsigned long DISPLAY_INTERVAL  = 300;   // ms - refresh TFT

// ================================================================
//  SETUP
// ================================================================
// ================================================================
//  PIN TOMBOL RESET WIFI
// ================================================================
#define BTN_WIFI_RESET  13   // GPIO 13 - tombol reset WiFi fisik
                              // Wiring: GPIO13 -- Tombol -- GND
                              // (INPUT_PULLUP: normal HIGH, tekan = LOW)

// Timing tombol
unsigned long btnPressStart = 0;
bool          btnWasPressed = false;
#define BTN_HOLD_MS  2000    // Tahan 2 detik untuk reset WiFi

// ================================================================
//  FUNGSI: checkWiFiResetButton()
// ================================================================
/*
 * Cek tombol reset WiFi setiap loop.
 * Wiring: GPIO13 -> Tombol -> GND (INPUT_PULLUP aktif)
 * Tahan 2 detik -> reset WiFi & restart ESP32
 */
void checkWiFiResetButton(unsigned long now) {
  bool pressed = (digitalRead(BTN_WIFI_RESET) == LOW);

  if (pressed && !btnWasPressed) {
    btnPressStart = now;
    btnWasPressed = true;
    Serial.println("[BTN] Tahan 2 detik untuk reset WiFi...");

  } else if (pressed && btnWasPressed) {
    unsigned long held = now - btnPressStart;

    // Countdown di footer TFT
    if (held > 300) {
      int detik = (int)((BTN_HOLD_MS - held) / 1000) + 1;
      if (detik < 1) detik = 1;
      tft.fillRect(200, 212, 118, 14, C_BG);
      tft.setTextColor(C_ORANGE);
      tft.setTextSize(1);
      tft.setCursor(202, 215);
      tft.print("WiFi reset: ");
      tft.print(detik);
      tft.print("s...");
    }

    if (held >= BTN_HOLD_MS) {
      Serial.println("[BTN] Reset WiFi dikonfirmasi!");
      tft.fillScreen(C_BG);
      tft.fillRect(0, 0, 320, 40, C_ORANGE);
      tft.setTextColor(C_BG);
      tft.setTextSize(2);
      tft.setCursor(30, 10);
      tft.print("RESET WIFI...");
      tft.setTextColor(C_WHITE);
      tft.setTextSize(1);
      tft.setCursor(20, 58);
      tft.print("Sambungkan HP ke hotspot:");
      tft.setTextColor(C_YELLOW);
      tft.setTextSize(2);
      tft.setCursor(10, 76);
      tft.print(AP_SSID);
      tft.setTextColor(C_WHITE);
      tft.setTextSize(1);
      tft.setCursor(20, 112);
      tft.print("Lalu buka browser, ketik:");
      tft.setTextColor(C_ACCENT);
      tft.setTextSize(2);
      tft.setCursor(40, 130);
      tft.print("192.168.4.1");
      delay(2000);
      wifiManager.resetSettings();
      ESP.restart();
    }

  } else if (!pressed && btnWasPressed) {
    btnWasPressed = false;
    Serial.println("[BTN] Dilepas, reset dibatalkan.");
    tft.fillRect(200, 212, 118, 14, C_BG);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n================================================");
  Serial.println("  BABY LENGTH METER - IoT System v2.0");
  Serial.println("================================================");

  // --- Init TFT lebih awal agar bisa tampil progress ---
  initDisplay(tft);
  drawSplash(tft);
  delay(1500);
  drawConnecting(tft);

  // --- Init Sensor ---
  initSensors();
  Serial.println("[OK] Sensors initialized.");

  // --- Init Load Cell HX711 ---
  initLoadCell();

  // --- Init Fuzzy Logic ---
  setupFuzzy();

  // --- Init Tombol Reset WiFi ---
  pinMode(BTN_WIFI_RESET, INPUT_PULLUP);
  Serial.println("[OK] WiFi reset button ready (GPIO 13, tahan 2 detik).");

  // --- Init WiFiManager ---
  setupWiFi(tft, sysStatus);

  // --- Setup Web Server Routes ---
  setupWebServer();

  server.begin();
  Serial.println("[OK] Web server started.");
  Serial.print("[INFO] IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Gambar layout utama TFT ---
  drawMainLayout(tft, sysStatus);

  Serial.println("[READY] System ready for measurement.");
  Serial.println("------------------------------------------------");
}

// ================================================================
//  LOOP UTAMA
// ================================================================
void loop() {
  server.handleClient();

  unsigned long now = millis();

  // --- Cek tombol reset WiFi ---
  checkWiFiResetButton(now);

  // --- Baca sensor setiap SENSOR_INTERVAL ---
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    measurement.ultrasonicCm = readUltrasonic();
    measurement.encoderCm    = readEncoder();
    measurement.babyLength   = calculateBabyLength(
      measurement.ultrasonicCm,
      measurement.encoderCm
    );

    // Baca load cell (non-blocking, update jika ada data baru)
    float w = readLoadCell();
    if (w >= 0.0f) {
      measurement.weightKg    = w;
      measurement.weightReady = true;
    }
  }

  // --- Update TFT setiap DISPLAY_INTERVAL ---
  if (now - lastDisplayRefresh >= DISPLAY_INTERVAL) {
    lastDisplayRefresh = now;
    updateDisplayValues(tft, measurement, sysStatus);
    updateWeightDisplay(tft, measurement);
    if (fuzzyResult.hasResult) updateFuzzyFooter(tft, fuzzyResult);
  }
}

// ================================================================
//  POINTER GLOBAL UNTUK CALLBACK WIFIMANAGER
//  WiFiManager setAPCallback tidak mendukung lambda dengan capture.
//  Gunakan pointer global + fungsi biasa sebagai workaround.
// ================================================================
Adafruit_ST7789* g_tft_ptr = nullptr;

void wifiAPCallback(WiFiManager* wm) {
  Serial.println("[WiFi] Config portal started.");
  Serial.print("[WiFi] AP SSID: ");
  Serial.println(AP_SSID);
  if (g_tft_ptr) drawAPMode(*g_tft_ptr, AP_SSID);
}

// ================================================================
//  SETUP WIFI DENGAN WIFIMANAGER
// ================================================================
void setupWiFi(Adafruit_ST7789& tft, SystemStatus& status) {
  Serial.println("[WiFi] Starting WiFiManager...");

  // Simpan pointer TFT agar dapat diakses di dalam callback
  g_tft_ptr = &tft;

  // Kustomisasi halaman portal
  wifiManager.setTitle("Baby Length Meter");
  wifiManager.setConfigPortalTimeout(180); // Timeout 3 menit

  // Callback saat masuk config portal (fungsi biasa, bukan lambda)
  wifiManager.setAPCallback(wifiAPCallback);

  // Callback saat WiFi berhasil connect
  wifiManager.setSaveConfigCallback([]() {
    Serial.println("[WiFi] New WiFi config saved.");
  });

  // Coba connect - jika gagal, buka portal BABY_MEASURE_SETUP
  bool connected = wifiManager.autoConnect(AP_SSID, AP_PASSWORD);

  if (connected) {
    status.wifiConnected = true;
    status.ipAddress     = WiFi.localIP().toString();
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP: ");
    Serial.println(status.ipAddress);
    drawWiFiConnected(tft, status.ipAddress);
    delay(1500);
  } else {
    // Timeout portal - lanjut tanpa WiFi
    status.wifiConnected = false;
    Serial.println("[WiFi] Portal timeout. Running without WiFi.");
  }
}

// ================================================================
//  SETUP WEB SERVER - ROUTING
// ================================================================
void setupWebServer() {
  // Halaman utama - web UI
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getMainPage(
      patient.name,
      patient.age,
      patient.gender,
      measurement.ultrasonicCm,
      measurement.encoderCm,
      measurement.babyLength,
      measurement.weightKg,
      fuzzyResult.statusBBU,
      fuzzyResult.statusPBU,
      fuzzyResult.statusIMTU
    ));
  });

  // API: ambil data sensor realtime (JSON)
    server.on("/api/data", HTTP_GET, []() {
    String json = "{";
    json += "\"ultrasonic\":" + String(measurement.ultrasonicCm, 1) + ",";
    json += "\"encoder\":" + String(measurement.encoderCm, 1) + ",";
    json += "\"length\":" + String(measurement.babyLength, 1) + ",";
    json += "\"weight\":" + String(measurement.weightKg, 2) + ",";
    json += "\"wifi\":" + String(sysStatus.wifiConnected ? "true" : "false") + ",";
    json += "\"ip\":\"" + sysStatus.ipAddress + "\"";
    if (fuzzyResult.hasResult) {
      json += ",\"bbu\":\"" + fuzzyResult.statusBBU + "\"";
      json += ",\"pbu\":\"" + fuzzyResult.statusPBU + "\"";
      json += ",\"imtu\":\"" + fuzzyResult.statusIMTU + "\"";
    }
    json += "}";
    server.send(200, "application/json", json);
  });

  // API: set data pasien dari form web
  server.on("/api/patient", HTTP_POST, []() {
    if (server.hasArg("name"))   patient.name   = server.arg("name");
    if (server.hasArg("age"))    patient.age    = server.arg("age");
    if (server.hasArg("gender")) patient.gender = server.arg("gender");

    Serial.println("[Patient] Data updated:");
    Serial.println("  Nama  : " + patient.name);
    Serial.println("  Usia  : " + patient.age);
    Serial.println("  Gender: " + patient.gender);

    // Update status TFT
    sysStatus.patientSet = true;
    updatePatientStatus(tft, patient);

    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // API: simpan data ke Google Spreadsheet
  server.on("/api/save", HTTP_POST, []() {
    if (!sysStatus.wifiConnected) {
      server.send(503, "application/json",
        "{\"status\":\"error\",\"msg\":\"No WiFi\"}");
      return;
    }

    if (patient.name == "" || measurement.babyLength <= 0) {
      server.send(400, "application/json",
        "{\"status\":\"error\",\"msg\":\"Data tidak lengkap\"}");
      return;
    }

    // Jalankan fuzzy logic dengan data terkini
    if (measurement.babyLength > 0 && measurement.weightKg > 0) {
      int ageInt = patient.age.toInt();
      FuzzyResult fr = runFuzzy(measurement.weightKg,
                                measurement.babyLength,
                                ageInt);
      fuzzyResult.bmi        = fr.bmi;
      fuzzyResult.bbu        = fr.bbu;
      fuzzyResult.pbu        = fr.pbu;
      fuzzyResult.imtu       = fr.imtu;
      fuzzyResult.statusBBU  = fr.statusBBU;
      fuzzyResult.statusPBU  = fr.statusPBU;
      fuzzyResult.statusIMTU = fr.statusIMTU;
      fuzzyResult.hasResult  = true;

      Serial.println("[Fuzzy] BB/U : " + fuzzyResult.statusBBU);
      Serial.println("[Fuzzy] PB/U : " + fuzzyResult.statusPBU);
      Serial.println("[Fuzzy] IMT/U: " + fuzzyResult.statusIMTU);
      Serial.println("[Fuzzy] IMT  : " + String(fuzzyResult.bmi, 1));
    }

    // Tampilkan status "Saving..." di TFT
    showSavingStatus(tft);

    // Kirim ke Google Spreadsheet (termasuk berat dan hasil fuzzy)
    bool success = saveToSpreadsheet(
      patient.name,
      patient.age,
      patient.gender,
      measurement.babyLength,
      measurement.ultrasonicCm,
      measurement.encoderCm,
      measurement.weightKg,
      fuzzyResult.bmi,
      fuzzyResult.statusBBU,
      fuzzyResult.statusPBU,
      fuzzyResult.statusIMTU
    );

    sysStatus.lastSaveOk = success;
    updateSaveStatus(tft, success);

    if (success) {
      Serial.println("[Sheets] Data saved successfully!");
      server.send(200, "application/json",
        "{\"status\":\"ok\",\"msg\":\"Data tersimpan!\"}");
      // Tampilkan hasil fuzzy di TFT selama 5 detik
      if (fuzzyResult.hasResult) {
        showFuzzyResult(tft, fuzzyResult, measurement);
        drawMainLayout(tft, sysStatus);  // Kembali ke layout utama
      }
    } else {
      Serial.println("[Sheets] Save FAILED!");
      server.send(500, "application/json",
        "{\"status\":\"error\",\"msg\":\"Gagal menyimpan\"}");
    }
  });

  // API: reset encoder
  server.on("/api/reset", HTTP_POST, []() {
    resetEncoder();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    Serial.println("[Encoder] Reset.");
  });

  // Halaman ganti WiFi - buka portal WiFiManager
  server.on("/wifi", HTTP_GET, []() {
    String page = "";
    page += "<!DOCTYPE html><html><head>";
    page += "<meta charset='UTF-8'>";
    page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    page += "<title>Ganti WiFi</title>";
    page += "<style>";
    page += "body{font-family:system-ui;background:#07090f;color:#f1f5f9;";
    page += "display:flex;align-items:center;justify-content:center;";
    page += "min-height:100vh;margin:0;}";
    page += ".box{background:#111827;border:1px solid #1f2937;";
    page += "border-radius:16px;padding:28px 24px;max-width:340px;";
    page += "width:90%;text-align:center;}";
    page += "h2{color:#06b6d4;margin:0 0 8px;}";
    page += "p{color:#64748b;font-size:14px;margin:0 0 20px;line-height:1.5;}";
    page += ".ssid{color:#fde047;font-weight:bold;}";
    page += ".ip{color:#06b6d4;font-weight:bold;}";
    page += ".ok{color:#10b981;}";
    page += ".msg{color:#f1f5f9;font-size:14px;line-height:1.6;}";
    page += ".warn{background:#1c1008;border:1px solid #92400e;";
    page += "border-radius:10px;padding:12px;color:#fbbf24;";
    page += "font-size:13px;margin-bottom:20px;}";
    page += "button{background:linear-gradient(135deg,#0891b2,#06b6d4);";
    page += "color:#fff;border:none;border-radius:10px;padding:14px;";
    page += "width:100%;font-size:15px;font-weight:600;cursor:pointer;}";
    page += "button:active{opacity:0.85;}";
    page += "#result{display:none;}";
    page += "#main{}";
    page += "</style></head><body>";
    page += "<div class='box'>";
    page += "<div id='main'>";
    page += "<h2>Ganti WiFi</h2>";
    page += "<p>ESP32 akan restart dan membuka hotspot.<br>";
    page += "Sambungkan HP ke:<br>";
    page += "<span class='ssid'>BABY_MEASURE_SETUP</span></p>";
    page += "<div class='warn'>Alat akan offline sebentar.<br>";
    page += "Buka 192.168.4.1 setelah connect.</div>";
    page += "<button onclick='doReset()'>Reset dan Ganti WiFi</button>";
    page += "</div>";
    page += "<div id='result'>";
    page += "<h2 class='ok'>Berhasil!</h2>";
    page += "<p class='msg'>ESP32 sedang restart...<br><br>";
    page += "Sambungkan HP ke WiFi:<br>";
    page += "<span class='ssid'>BABY_MEASURE_SETUP</span><br><br>";
    page += "Lalu buka browser, ketik:<br>";
    page += "<span class='ip'>192.168.4.1</span></p>";
    page += "</div>";
    page += "</div>";
    page += "<script>";
    page += "function doReset(){";
    page += "fetch('/api/reset-wifi',{method:'POST'})";
    page += ".then(function(){";
    page += "document.getElementById('main').style.display='none';";
    page += "document.getElementById('result').style.display='block';";
    page += "})";
    page += ".catch(function(){alert('Gagal, coba lagi');});";
    page += "}";
    page += "</script>";
    page += "</body></html>";
    server.send(200, "text/html", page);
  });

  // API: reset WiFi dan restart ESP32
  server.on("/api/reset-wifi", HTTP_POST, []() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    Serial.println("[WiFi] Resetting WiFi settings...");
    wifiManager.resetSettings();
    delay(200);
    ESP.restart();
  });

  // Handle 404
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
}
