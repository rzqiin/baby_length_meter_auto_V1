/*
 * spreadsheet.h
 * Modul pengiriman data ke Google Spreadsheet via Google Apps Script
 *
 * PERBAIKAN:
 *   - Tambah WiFiClientSecure untuk HTTPS
 *   - Set certificate fingerprint / insecure mode untuk Google SSL
 *   - Handle redirect manual karena Google Script selalu 302 redirect
 *   - Timeout lebih panjang (15 detik) karena Google server bisa lambat
 */

#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include "config.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

// ================================================================
//  FUNGSI: saveToSpreadsheet()
// ================================================================
/*
 * KENAPA PERLU WiFiClientSecure?
 *   - Google Apps Script URL = https:// (wajib SSL/TLS)
 *   - HTTPClient biasa (http.begin(url)) tidak handle SSL dengan benar
 *   - Tanpa WiFiClientSecure, koneksi langsung ditolak / error -1
 *
 * KENAPA setInsecure()?
 *   - Verifikasi certificate Google butuh root CA bundle ~200KB
 *   - Flash ESP32 tidak cukup untuk simpan semua root CA
 *   - setInsecure() skip verifikasi certificate tapi tetap enkripsi SSL
 *   - Aman untuk penggunaan IoT internal seperti ini
 *
 * KENAPA followRedirects?
 *   - Google Apps Script SELALU reply 302 redirect ke URL lain
 *   - Tanpa follow redirect, response selalu kosong / error
 */
bool saveToSpreadsheet(String name, String age, String gender,
                        float length, float ultrasonic, float encoder,
                        float weight, float bmi,
                        String statusBBU, String statusPBU, String statusIMTU)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Sheets] ERROR: WiFi tidak terhubung!");
    return false;
  }

  // --- Encode parameter URL ---
  String encodedName = name;
  encodedName.replace(" ", "%20");
  encodedName.replace("&", "%26");
  encodedName.replace("'", "%27");
  encodedName.replace("\"", "%22");

  // Encode status strings
  String encBBU  = statusBBU;  encBBU.replace(" ", "%20");
  String encPBU  = statusPBU;  encPBU.replace(" ", "%20");
  String encIMTU = statusIMTU; encIMTU.replace(" ", "%20");

  String url = String(GOOGLE_SCRIPT_URL);
  url += "?name="       + encodedName;
  url += "&age="        + age;
  url += "&gender="     + gender;
  url += "&length="     + String(length, 1);
  url += "&weight="     + String(weight, 2);
  url += "&bmi="        + String(bmi, 1);
  url += "&ultrasonic=" + String(ultrasonic, 1);
  url += "&encoder="    + String(encoder, 1);
  url += "&bbu="        + encBBU;
  url += "&pbu="        + encPBU;
  url += "&imtu="       + encIMTU;

  Serial.println("[Sheets] Sending...");
  Serial.println("[Sheets] URL: " + url);

  // --- Setup HTTPS client ---
  WiFiClientSecure client;
  client.setInsecure(); // Skip SSL certificate verification

  HTTPClient http;
  http.begin(client, url);                                    // Gunakan secure client
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);    // Ikuti redirect Google
  http.setTimeout(15000);                                     // 15 detik timeout
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.GET();

  Serial.print("[Sheets] HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[Sheets] Response: " + response);
    http.end();

    // Sukses jika 200 OK
    if (httpCode == 200) {
      Serial.println("[Sheets] BERHASIL disimpan!");
      return true;
    } else {
      Serial.println("[Sheets] GAGAL - HTTP " + String(httpCode));
      return false;
    }
  } else {
    // httpCode negatif = error koneksi
    Serial.print("[Sheets] ERROR koneksi: ");
    Serial.println(http.errorToString(httpCode));
    Serial.println("[Sheets] Kemungkinan: SSL gagal / timeout / DNS error");
    http.end();
    return false;
  }
}

#endif // SPREADSHEET_H
