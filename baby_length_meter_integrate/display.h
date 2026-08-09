/*
 * display.h
 * Modul tampilan TFT ST7789 (2.8" 240x320) - semua fungsi menggambar layar
 *
 * Layout TFT (320x240 Landscape):
 * ┌────────────────────────────────┐  y=0
 * │  HEADER: BABY LENGTH METER     │  h=38
 * ├──────────────┬─────────────────┤  y=40
 * │ ULTRASONIK   │  ENCODER        │  h=52
 * ├──────────────┴─────────────────┤  y=94
 * │      PANJANG BAYI (BESAR)      │  h=82
 * ├────────────────────────────────┤  y=178
 * │ WiFi: ●  IP: 192.168.x.x      │  h=30
 * │ [Pasien] Status Save           │  h=28
 * └────────────────────────────────┘  y=238
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Lebar/Tinggi layar (landscape)
#define SCREEN_W  320
#define SCREEN_H  240

// ================================================================
//  INISIALISASI DISPLAY
// ================================================================
void initDisplay(Adafruit_ST7789& tft) {
  // Panel fisik ST7789 2.8" ini native-nya 240x320 (portrait).
  // init() WAJIB dikasih tahu resolusi panel sebelum di-rotate,
  // beda dengan ILI9341 yang cukup pakai begin().
  tft.init(240, 320);
  tft.setRotation(1); // Landscape: 320x240

  // Kalau warna kebalik (misal background yang harusnya hitam jadi
  // putih, atau merah jadi cyan), un-comment baris di bawah ini:
  // tft.invertDisplay(true);

  tft.fillScreen(C_BG);
}

// ================================================================
//  SPLASH SCREEN
// ================================================================
void drawSplash(Adafruit_ST7789& tft) {
  tft.fillScreen(C_BG);

  // Lingkaran dekoratif background
  tft.drawCircle(160, 100, 60, C_ACCENT);
  tft.drawCircle(160, 100, 55, C_HEADER);

  // Ikon bayi sederhana - kepala (lingkaran)
  tft.fillCircle(160, 90, 28, C_ACCENT);
  tft.fillCircle(160, 90, 23, C_BG);
  tft.fillCircle(160, 90, 14, C_YELLOW);

  // Badan (persegi panjang rounded)
  tft.fillRoundRect(145, 120, 30, 22, 6, C_YELLOW);

  // Teks
  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(30, 158);
  tft.print("BABY LENGTH METER");

  tft.setTextSize(1);
  tft.setTextColor(C_GRAY);
  tft.setCursor(68, 182);
  tft.print("Pengukur Panjang Bayi IoT");

  tft.setCursor(108, 198);
  tft.print("ESP32 v2.0");

  // Loading bar animasi
  tft.drawRoundRect(50, 215, 220, 14, 7, C_ACCENT);
  for (int w = 0; w <= 216; w += 4) {
    tft.fillRoundRect(52, 217, w, 10, 5, C_ACCENT);
    delay(12);
  }
  tft.setTextColor(C_GREEN);
  tft.setCursor(132, 232);
  tft.print("READY");
}

// ================================================================
//  LAYAR CONNECTING (saat setup WiFi)
// ================================================================
// Helper: gambar setengah lingkaran atas (ikon WiFi) menggunakan
// drawCircle + masker persegi bawah. Adafruit_ST7789 tidak punya drawArc.
void drawWiFiArc(Adafruit_ST7789& tft, int cx, int cy,
                  int r, uint16_t color) {
  // Gambar lingkaran penuh lalu tutup bagian bawah dengan warna BG
  tft.drawCircle(cx, cy, r, color);
  // Tutup 60% bawah agar terlihat seperti busur atas
  tft.fillRect(cx - r - 2, cy, (r + 2) * 2 + 4, r + 2, C_BG);
}

void drawConnecting(Adafruit_ST7789& tft) {
  tft.fillScreen(C_BG);

  // Ikon WiFi: 3 busur konsentrik (simulasi dengan drawCircle + mask)
  int cx = 160, cy = 118;
  drawWiFiArc(tft, cx, cy, 52, C_GRAY);
  drawWiFiArc(tft, cx, cy, 36, C_GRAY);
  drawWiFiArc(tft, cx, cy, 20, C_GRAY);
  tft.fillCircle(cx, cy + 2, 5, C_GRAY);

  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(72, 155);
  tft.print("Menghubungkan ke WiFi...");

  tft.setTextColor(C_GRAY);
  tft.setCursor(38, 172);
  tft.print("Jika perlu, buka hotspot:");
  tft.setTextColor(C_ACCENT);
  tft.setCursor(52, 188);
  tft.print(AP_SSID);
}

// ================================================================
//  LAYAR AP MODE (portal setup WiFi aktif)
// ================================================================
void drawAPMode(Adafruit_ST7789& tft, const char* ssid) {
  tft.fillScreen(C_BG);

  // Header
  tft.fillRect(0, 0, SCREEN_W, 36, C_ORANGE);
  tft.setTextColor(C_BG);
  tft.setTextSize(2);
  tft.setCursor(22, 9);
  tft.print("SETUP MODE - WiFi");

  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);

  tft.setCursor(20, 48);
  tft.print("Hubungkan HP ke hotspot:");
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(30, 65);
  tft.print(ssid);

  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, 96);
  tft.print("Lalu buka browser, ketik:");
  tft.setTextColor(C_ACCENT);
  tft.setCursor(20, 112);
  tft.print("192.168.4.1");

  tft.setTextColor(C_GRAY);
  tft.setCursor(20, 135);
  tft.print("Pilih jaringan WiFi rumah Anda");
  tft.setCursor(20, 150);
  tft.print("dan masukkan password.");

  // Baris instruksi
  tft.fillRect(0, 175, SCREEN_W, 1, C_DARK_GRAY);
  tft.setTextColor(C_DARK_GRAY);
  tft.setCursor(20, 182);
  tft.print("Timeout: 3 menit");
}

// ================================================================
//  LAYAR WIFI CONNECTED (singkat sebelum main)
// ================================================================
void drawWiFiConnected(Adafruit_ST7789& tft, String ip) {
  tft.fillScreen(C_BG);

  tft.fillCircle(160, 95, 40, C_BOX_BG);
  tft.drawCircle(160, 95, 42, C_GREEN);

  // Checkmark
  tft.drawLine(140, 95, 155, 110, C_GREEN);
  tft.drawLine(140, 96, 155, 111, C_GREEN);
  tft.drawLine(155, 110, 180, 78, C_GREEN);
  tft.drawLine(155, 111, 180, 79, C_GREEN);

  tft.setTextColor(C_GREEN);
  tft.setTextSize(2);
  tft.setCursor(75, 150);
  tft.print("WiFi Terhubung!");

  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(80, 175);
  tft.print("IP: " + ip);

  tft.setTextColor(C_GRAY);
  tft.setCursor(50, 195);
  tft.print("Buka browser, ketik IP di atas");
}

// ================================================================
//  LAYOUT UTAMA TFT
// ================================================================
void drawMainLayout(Adafruit_ST7789& tft, SystemStatus& status) {
  tft.fillScreen(C_BG);

  // ========================
  //  HEADER (y: 0-37)
  // ========================
  tft.fillRect(0, 0, SCREEN_W, 38, C_HEADER);
  tft.fillRect(0, 36, SCREEN_W, 2, C_ACCENT);

  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(28, 7);
  tft.print("BABY LENGTH METER");

  tft.setTextColor(C_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(98, 26);
  tft.print("Pengukur Panjang Bayi");

  // ========================
  //  BOX ULTRASONIK (y: 40-91)
  // ========================
  tft.drawRect(4, 40, 152, 52, C_ACCENT);
  tft.fillRect(5, 41, 150, 50, C_BOX_BG);

  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(10, 47);
  tft.print("ULTRASONIK (Kepala)");

  // Nilai placeholder
  tft.setTextColor(C_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 62);
  tft.print("--.- cm");

  // ========================
  //  BOX ENCODER (y: 40-91)
  // ========================
  tft.drawRect(164, 40, 152, 52, C_ACCENT);
  tft.fillRect(165, 41, 150, 50, C_BOX_BG);

  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(170, 47);
  tft.print("ENCODER (Kaki)");

  tft.setTextColor(C_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(170, 62);
  tft.print("--.- cm");

  // ========================
  //  SEPARATOR
  // ========================
  tft.fillRect(0, 94, SCREEN_W, 1, C_DARK_GRAY);
  tft.setTextColor(C_DARK_GRAY);
  tft.setTextSize(1);
  tft.setCursor(120, 98);
  tft.print("HASIL UKUR");

  // ========================
  //  BOX PANJANG BAYI (y: 107-188)
  // ========================
  tft.drawRect(8, 107, 304, 72, C_ACCENT2);   // Outer border
  tft.drawRect(10, 109, 300, 68, C_ACCENT);   // Inner border
  tft.fillRect(11, 110, 298, 66, C_RESULT_BG);

  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(126, 116);
  tft.print("PANJANG BAYI");

  tft.setTextColor(C_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(50, 134);
  tft.print("--- . - cm");

  // ========================
  //  FOOTER STATUS (y: 182-238)
  // ========================
  tft.fillRect(0, 182, SCREEN_W, 1, C_DARK_GRAY);

  // WiFi status
  tft.setTextSize(1);
  if (status.wifiConnected) {
    tft.fillCircle(12, 192, 5, C_GREEN);
    tft.setTextColor(C_GREEN);
    tft.setCursor(22, 188);
    tft.print("WiFi  IP:");
    tft.setCursor(22, 200);
    tft.print(status.ipAddress);
  } else {
    tft.fillCircle(12, 192, 5, C_RED);
    tft.setTextColor(C_RED);
    tft.setCursor(22, 188);
    tft.print("No WiFi");
  }

  // Save status (kanan footer)
  tft.setTextColor(C_DARK_GRAY);
  tft.setCursor(170, 188);
  tft.print("Save: --");

  // Pasien info
  tft.setTextColor(C_DARK_GRAY);
  tft.setCursor(4, 215);
  tft.print("Pasien: (belum diisi)");

  tft.setTextColor(C_DARK_GRAY);
  tft.setCursor(200, 215);
  tft.print("100cm base");
}

// ================================================================
//  UPDATE NILAI SENSOR (realtime, anti-flicker)
// ================================================================
void updateDisplayValues(Adafruit_ST7789& tft,
                          MeasurementData& data,
                          SystemStatus& status)
{
  // --- Update Ultrasonik (jika berubah > 0.1 cm) ---
  if (abs(data.ultrasonicCm - data.lastUltrasonic) > 0.1f) {
    // Hapus area nilai lama
    tft.fillRect(6, 60, 148, 22, C_BOX_BG);

    tft.setTextSize(2);
    if (data.ultrasonicCm < 0) {
      tft.setTextColor(C_RED);
      tft.setCursor(10, 62);
      tft.print("ERR!  ");
    } else {
      tft.setTextColor(C_GREEN);
      tft.setCursor(10, 62);
      if (data.ultrasonicCm < 10.0f) tft.print(" ");
      tft.print(data.ultrasonicCm, 1);
      tft.setTextColor(C_GRAY);
      tft.print(" cm");
    }
    data.lastUltrasonic = data.ultrasonicCm;
  }

  // --- Update Encoder (jika berubah > 0.1 cm) ---
  if (abs(data.encoderCm - data.lastEncoder) > 0.1f) {
    tft.fillRect(166, 60, 148, 22, C_BOX_BG);

    tft.setTextSize(2);
    tft.setTextColor(C_ORANGE);
    tft.setCursor(170, 62);
    if (data.encoderCm < 10.0f) tft.print(" ");
    tft.print(data.encoderCm, 1);
    tft.setTextColor(C_GRAY);
    tft.print(" cm");

    data.lastEncoder = data.encoderCm;
  }

  // --- Update Panjang Bayi (jika berubah > 0.1 cm) ---
  if (abs(data.babyLength - data.lastBaby) > 0.1f) {
    tft.fillRect(12, 130, 296, 40, C_RESULT_BG);

    tft.setTextSize(3);
    uint16_t col = (data.babyLength > 0) ? C_YELLOW : C_RED;
    tft.setTextColor(col);
    tft.setCursor(42, 134);

    // Format: XXX.X cm
    if (data.babyLength < 10.0f)       tft.print("  ");
    else if (data.babyLength < 100.0f) tft.print(" ");
    tft.print(data.babyLength, 1);

    tft.setTextColor(C_GRAY);
    tft.print(" cm");

    data.lastBaby = data.babyLength;
  }
}

// ================================================================
//  UPDATE STATUS SAVE
// ================================================================
void showSavingStatus(Adafruit_ST7789& tft) {
  tft.fillRect(160, 184, 156, 16, C_BG);
  tft.setTextColor(C_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(170, 188);
  tft.print("Save: MENYIMPAN...");
}

void updateSaveStatus(Adafruit_ST7789& tft, bool success) {
  tft.fillRect(160, 184, 156, 16, C_BG);
  tft.setTextSize(1);
  tft.setCursor(170, 188);
  if (success) {
    tft.setTextColor(C_GREEN);
    tft.print("Save: BERHASIL!");
  } else {
    tft.setTextColor(C_RED);
    tft.print("Save: GAGAL!");
  }
}

// ================================================================
//  UPDATE INFO PASIEN DI FOOTER
// ================================================================
void updatePatientStatus(Adafruit_ST7789& tft, PatientData& patient) {
  tft.fillRect(0, 212, 198, 14, C_BG);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(4, 215);
  String info = patient.name;
  if (info.length() > 12) info = info.substring(0, 12) + "..";
  info += " " + patient.age + "bl " + patient.gender;
  tft.print(info);
}

// ================================================================
//  UPDATE BERAT BADAN (area footer kiri TFT)
// ================================================================
/*
 * Menampilkan berat bayi dari load cell di bawah box hasil panjang.
 * Layout: [Berat: X.XX kg] di baris footer kiri
 */
void updateWeightDisplay(Adafruit_ST7789& tft, MeasurementData& data) {
  if (abs(data.weightKg - data.lastWeight) > 0.01f) {
    tft.fillRect(0, 183, 160, 14, C_BG);
    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT);
    tft.setCursor(4, 186);
    tft.print("Berat: ");
    tft.setTextColor(C_WHITE);
    tft.print(data.weightKg, 2);
    tft.setTextColor(C_GRAY);
    tft.print(" kg");
    data.lastWeight = data.weightKg;
  }
}

// ================================================================
//  HELPER: getStatusColor() - warna berdasarkan status fuzzy
// ================================================================
uint16_t getStatusColor(String status) {
  if (status == "BB Normal" || status == "PB Normal" || status == "Gizi Baik") {
    return C_GREEN;
  } else if (status == "Tinggi" || status == "Risiko BB Lebih" || status == "Risiko Gizi Lebih") {
    return C_ORANGE;
  } else {
    return C_RED;
  }
}

// ================================================================
//  LAYAR FUZZY RESULT (layar kedua, dipanggil setelah SAVE)
// ================================================================
/*
 * Menampilkan hasil analisis fuzzy selama 5 detik di TFT,
 * lalu kembali ke layout utama.
 */
void showFuzzyResult(Adafruit_ST7789& tft,
                      FuzzyAnalysis& fa,
                      MeasurementData& data)
{
  tft.fillScreen(C_BG);

  // Header
  tft.fillRect(0, 0, SCREEN_W, 36, C_HEADER);
  tft.fillRect(0, 34, SCREEN_W, 2, C_ACCENT2);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(52, 8);
  tft.print("STATUS GIZI BAYI");

  // IMT label
  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(4, 44);
  tft.print("IMT: ");
  tft.setTextColor(C_ACCENT);
  tft.print(fa.bmi, 1);
  tft.print(" kg/m2");

  // Box BB/U
  tft.drawRect(4, 58, SCREEN_W - 8, 50, C_ACCENT);
  tft.fillRect(5, 59, SCREEN_W - 10, 48, C_BOX_BG);
  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(10, 65);
  tft.print("BB/U (Berat per Usia)");
  tft.setTextColor(getStatusColor(fa.statusBBU));
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.print(fa.statusBBU);

  // Box PB/U
  tft.drawRect(4, 114, SCREEN_W - 8, 50, C_ACCENT);
  tft.fillRect(5, 115, SCREEN_W - 10, 48, C_BOX_BG);
  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(10, 121);
  tft.print("PB/U (Panjang per Usia)");
  tft.setTextColor(getStatusColor(fa.statusPBU));
  tft.setTextSize(2);
  tft.setCursor(10, 136);
  tft.print(fa.statusPBU);

  // Box IMT/U
  tft.drawRect(4, 170, SCREEN_W - 8, 50, C_ACCENT);
  tft.fillRect(5, 171, SCREEN_W - 10, 48, C_BOX_BG);
  tft.setTextColor(C_GRAY);
  tft.setTextSize(1);
  tft.setCursor(10, 177);
  tft.print("IMT/U (Indeks Massa Tubuh per Usia)");
  tft.setTextColor(getStatusColor(fa.statusIMTU));
  tft.setTextSize(2);
  tft.setCursor(10, 192);
  tft.print(fa.statusIMTU);

  tft.setTextColor(C_DARK_GRAY);
  tft.setTextSize(1);
  tft.setCursor(80, 228);
  tft.print("Kembali dalam 5 detik...");

  delay(5000);
}

// ================================================================
//  UPDATE STATUS FUZZY DI FOOTER TFT (ringkas, 1 baris)
// ================================================================
void updateFuzzyFooter(Adafruit_ST7789& tft, FuzzyAnalysis& fa) {
  if (!fa.hasResult) return;
  tft.fillRect(0, 212, SCREEN_W, 14, C_BG);
  tft.setTextSize(1);

  // Tampilkan status paling kritis (warna merah jika ada risiko)
  bool anyRisk = (fa.statusBBU != "BB Normal") ||
                 (fa.statusPBU != "PB Normal" && fa.statusPBU != "Tinggi") ||
                 (fa.statusIMTU != "Gizi Baik");

  tft.setTextColor(anyRisk ? C_RED : C_GREEN);
  tft.setCursor(4, 215);

  String summary = fa.statusBBU.substring(0,6) + " | " +
                   fa.statusPBU.substring(0,6) + " | " +
                   fa.statusIMTU.substring(0,8);
  tft.print(summary);
}

#endif // DISPLAY_H
