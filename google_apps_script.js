/**
 * ================================================================
 *  Google Apps Script - Baby Length Meter Logger
 * ================================================================
 *  Script ini menerima data dari ESP32 via HTTP GET,
 *  lalu menulisnya ke Google Spreadsheet.
 *
 *  CARA SETUP (WAJIB DIBACA):
 *  1. Buka https://script.google.com
 *  2. Klik "New Project"
 *  3. Hapus kode default, paste seluruh kode ini
 *  4. Ganti SPREADSHEET_ID di bawah dengan ID spreadsheet Anda
 *  5. Klik Deploy > New Deployment
 *  6. Pilih Type: Web App
 *  7. Execute as: Me
 *  8. Who has access: Anyone
 *  9. Klik Deploy, copy URL yang diberikan
 * 10. Paste URL ke GOOGLE_SCRIPT_URL di config.h ESP32
 *
 *  CARA DAPAT SPREADSHEET_ID:
 *  Buka Google Spreadsheet Anda, lihat URL:
 *  https://docs.google.com/spreadsheets/d/[SPREADSHEET_ID]/edit
 *  Salin bagian [SPREADSHEET_ID]
 * ================================================================
 */

// ================================================================
//  KONFIGURASI - GANTI INI
// ================================================================
var SPREADSHEET_ID = "YOUR_SPREADSHEET_ID_HERE"; // <-- Ganti ini!
var SHEET_NAME     = "Data Bayi";                // Nama sheet/tab

// ================================================================
//  doGet() - Handler HTTP GET dari ESP32
// ================================================================
/**
 * Fungsi ini otomatis dipanggil oleh Google saat ada HTTP GET request.
 *
 * Parameter yang diterima dari ESP32:
 *   ?name=NamaBayi&age=3&gender=L&length=52.5&ultrasonic=12.3&encoder=64.8
 *
 * @param {Object} e - Event object dari HTTP request
 * @return {TextOutput} Response JSON
 */
function doGet(e) {
  try {
    // --- Ambil parameter dari URL ---
    var params = e.parameter;

    var name       = params.name       || "";
    var age        = params.age        || "";
    var gender     = params.gender     || "";
    var length     = params.length     || "0";
    var weight     = params.weight     || "0";
    var bmi        = params.bmi        || "0";
    var ultrasonic = params.ultrasonic || "0";
    var encoder    = params.encoder    || "0";
    var bbu        = params.bbu        || "-";
    var pbu        = params.pbu        || "-";
    var imtu       = params.imtu       || "-";

    // --- Validasi: nama wajib ada ---
    if (name === "") {
      return ContentService
        .createTextOutput(JSON.stringify({status: "error", msg: "Nama tidak boleh kosong"}))
        .setMimeType(ContentService.MimeType.JSON);
    }

    // --- Buka spreadsheet ---
    var ss    = SpreadsheetApp.openById(SPREADSHEET_ID);
    var sheet = ss.getSheetByName(SHEET_NAME);

    // Jika sheet belum ada, buat baru
    if (!sheet) {
      sheet = ss.insertSheet(SHEET_NAME);
      setupHeader(sheet); // Buat header kolom
    }

    // Jika sheet baru (kosong), buat header
    if (sheet.getLastRow() === 0) {
      setupHeader(sheet);
    }

    // --- Buat timestamp ---
    var now       = new Date();
    var timezone  = "Asia/Jakarta"; // WIB - sesuaikan jika perlu
    var timestamp = Utilities.formatDate(now, timezone, "dd/MM/yyyy HH:mm:ss");
    var dateOnly  = Utilities.formatDate(now, timezone, "dd/MM/yyyy");
    var timeOnly  = Utilities.formatDate(now, timezone, "HH:mm:ss");

    // --- Append baris data baru ---
    sheet.appendRow([
      sheet.getLastRow(),          // No.
      timestamp,                   // Timestamp
      dateOnly,                    // Tanggal
      timeOnly,                    // Jam
      name,                        // Nama bayi
      age,                         // Usia (bulan)
      gender,                      // L / P
      parseFloat(length),          // Panjang bayi (cm)
      parseFloat(weight),          // Berat bayi (kg)
      parseFloat(bmi),             // IMT/BMI (kg/m2)
      parseFloat(ultrasonic),      // Ultrasonik (cm)
      parseFloat(encoder),         // Encoder (cm)
      bbu,                         // Status BB/U
      pbu,                         // Status PB/U
      imtu                         // Status IMT/U
    ]);

    // --- Format sel ---
    var lastRow = sheet.getLastRow();
    var lengthCell = sheet.getRange(lastRow, 8);
    lengthCell.setFontWeight("bold");
    lengthCell.setBackground("#e8f5e9");

    // Warnai sel status fuzzy
    var bbuCell  = sheet.getRange(lastRow, 13);
    var pbuCell  = sheet.getRange(lastRow, 14);
    var imtuCell = sheet.getRange(lastRow, 15);
    [bbuCell, pbuCell, imtuCell].forEach(function(cell) {
      var val = cell.getValue();
      if (val.indexOf("Normal") >= 0 || val.indexOf("Baik") >= 0 || val == "Tinggi") {
        cell.setBackground("#e8f5e9"); // hijau
      } else if (val.indexOf("Risiko") >= 0 || val.indexOf("Wasting") >= 0 || val.indexOf("Stunting") >= 0) {
        cell.setBackground("#ffebee"); // merah muda
      }
    });

    Logger.log("Data saved: " + name + " - " + length + " cm");

    // --- Return sukses ---
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "ok",
        msg: "Data berhasil disimpan",
        row: lastRow,
        name: name,
        length: length,
        timestamp: timestamp
      }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (err) {
    Logger.log("ERROR: " + err.toString());
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "error",
        msg: err.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

// ================================================================
//  setupHeader() - Membuat baris header kolom
// ================================================================
/**
 * Membuat header tabel di baris pertama spreadsheet.
 *
 * @param {Sheet} sheet - Objek sheet Google Spreadsheet
 */
function setupHeader(sheet) {
  var headers = [
    "No.",
    "Timestamp",
    "Tanggal",
    "Jam",
    "Nama Bayi",
    "Usia (bulan)",
    "Gender",
    "Panjang Bayi (cm)",
    "Berat Bayi (kg)",
    "IMT (kg/m2)",
    "Ultrasonik (cm)",
    "Encoder (cm)",
    "Status BB/U",
    "Status PB/U",
    "Status IMT/U"
  ];

  sheet.appendRow(headers);

  // Format header: bold, background biru, teks putih
  var headerRange = sheet.getRange(1, 1, 1, headers.length);
  headerRange.setFontWeight("bold");
  headerRange.setBackground("#0d47a1");
  headerRange.setFontColor("#ffffff");
  headerRange.setHorizontalAlignment("center");

  // Freeze baris header
  sheet.setFrozenRows(1);

  // Set lebar kolom
  sheet.setColumnWidth(1, 45);   // No.
  sheet.setColumnWidth(2, 150);  // Timestamp
  sheet.setColumnWidth(3, 100);  // Tanggal
  sheet.setColumnWidth(4, 80);   // Jam
  sheet.setColumnWidth(5, 140);  // Nama
  sheet.setColumnWidth(6, 110);  // Usia
  sheet.setColumnWidth(7, 75);   // Gender
  sheet.setColumnWidth(8, 140);  // Panjang
  sheet.setColumnWidth(9, 110);  // Berat
  sheet.setColumnWidth(10, 90);  // IMT
  sheet.setColumnWidth(11, 110); // Ultrasonik
  sheet.setColumnWidth(12, 100); // Encoder
  sheet.setColumnWidth(13, 140); // BB/U
  sheet.setColumnWidth(14, 140); // PB/U
  sheet.setColumnWidth(15, 150); // IMT/U
}

// ================================================================
//  testRun() - Fungsi uji coba (jalankan dari editor)
// ================================================================
/**
 * Fungsi ini untuk menguji script tanpa ESP32.
 * Jalankan dari Google Apps Script Editor dengan Run > testRun.
 */
function testRun() {
  var testEvent = {
    parameter: {
      name:       "Bayi Ujicoba",
      age:        "3",
      gender:     "L",
      length:     "52.5",
      ultrasonic: "12.3",
      encoder:    "64.8"
    }
  };

  var result = doGet(testEvent);
  Logger.log(result.getContent());
}
