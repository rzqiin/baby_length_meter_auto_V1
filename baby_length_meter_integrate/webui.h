/*
 * webui.h
 * HTML Web Interface untuk Baby Length Meter
 * Disajikan oleh ESP32 Web Server - diakses via browser HP
 *
 * Fitur:
 *   - Form input data pasien (nama, usia, gender)
 *   - Tampilan realtime nilai sensor (update setiap 500ms via fetch API)
 *   - Tombol START MEASURE dan SAVE DATA
 *   - Tombol RESET ENCODERa
 *   - Desain modern, dark theme, mobile-friendly
 *   - Animasi dan feedback visual
 */

#ifndef WEBUI_H
#define WEBUI_H

#include <Arduino.h>

// ================================================================
//  FUNGSI: getMainPage()
//  Mengembalikan HTML lengkap halaman web sebagai String
// ================================================================
String getMainPage(String name, String age, String gender,
                    float ultrasonicVal, float encoderVal, float babyVal,
                    float weightVal,
                    String sBBU, String sPBU, String sIMTU)
{
  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
<title>Baby Length Meter</title>
<style>
  :root {
    --bg:       #07090f;
    --surface:  #0d1117;
    --card:     #111827;
    --border:   #1f2937;
    --accent:   #06b6d4;
    --accent2:  #f472b6;
    --green:    #10b981;
    --orange:   #f59e0b;
    --yellow:   #fde047;
    --red:      #ef4444;
    --text:     #f1f5f9;
    --muted:    #64748b;
    --radius:   14px;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
    padding: 0 0 40px;
  }

  /* HEADER */
  .header {
    background: linear-gradient(135deg, #0a1628 0%, #0d1f3c 60%, #0a1628 100%);
    border-bottom: 2px solid var(--accent);
    padding: 18px 20px 14px;
    text-align: center;
    position: relative;
  }
  .header::after {
    content: '';
    position: absolute;
    bottom: -1px; left: 0; right: 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, var(--accent), var(--accent2), transparent);
  }
  .header h1 {
    font-size: 1.35rem;
    font-weight: 700;
    letter-spacing: 0.05em;
    color: var(--text);
  }
  .header h1 span { color: var(--accent); }
  .header p {
    font-size: 0.75rem;
    color: var(--muted);
    margin-top: 3px;
  }
  .wifi-badge {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-size: 0.7rem;
    background: rgba(16, 185, 129, 0.12);
    border: 1px solid rgba(16, 185, 129, 0.3);
    color: var(--green);
    padding: 3px 9px;
    border-radius: 20px;
    margin-top: 6px;
  }
  .wifi-badge .dot {
    width: 7px; height: 7px;
    background: var(--green);
    border-radius: 50%;
    animation: pulse 2s infinite;
  }
  @keyframes pulse {
    0%,100% { opacity: 1; }
    50% { opacity: 0.3; }
  }

  /* CONTAINER */
  .container { max-width: 480px; margin: 0 auto; padding: 16px 14px; }

  /* CARDS */
  .card {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 16px;
    margin-bottom: 14px;
  }
  .card-title {
    font-size: 0.68rem;
    font-weight: 600;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 12px;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .card-title::before {
    content: '';
    width: 3px; height: 14px;
    border-radius: 2px;
    background: var(--accent);
    display: block;
  }

  /* SENSOR GRID */
  .sensor-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
    margin-bottom: 14px;
  }
  .sensor-box {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 14px 12px;
    text-align: center;
    position: relative;
    overflow: hidden;
  }
  .sensor-box::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
  }
  .sensor-box.us::before  { background: var(--green); }
  .sensor-box.enc::before { background: var(--orange); }

  .sensor-label {
    font-size: 0.62rem;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 6px;
  }
  .sensor-value {
    font-size: 1.7rem;
    font-weight: 700;
    line-height: 1;
    font-variant-numeric: tabular-nums;
  }
  .sensor-box.us  .sensor-value { color: var(--green); }
  .sensor-box.enc .sensor-value { color: var(--orange); }
  .sensor-unit {
    font-size: 0.72rem;
    color: var(--muted);
    margin-top: 3px;
  }

  /* RESULT BOX */
  .result-box {
    background: linear-gradient(135deg, #0d1f17 0%, #0d1a2a 100%);
    border: 2px solid var(--accent2);
    border-radius: var(--radius);
    padding: 20px;
    text-align: center;
    margin-bottom: 14px;
    position: relative;
    overflow: hidden;
  }
  .result-box::after {
    content: '';
    position: absolute;
    inset: 0;
    background: radial-gradient(ellipse at center, rgba(244,114,182,0.05) 0%, transparent 70%);
    pointer-events: none;
  }
  .result-label {
    font-size: 0.7rem;
    letter-spacing: 0.14em;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 8px;
  }
  .result-value {
    font-size: 3.2rem;
    font-weight: 800;
    color: var(--yellow);
    line-height: 1;
    font-variant-numeric: tabular-nums;
    transition: color 0.3s;
  }
  .result-value.zero { color: var(--muted); }
  .result-unit {
    font-size: 1rem;
    color: var(--muted);
    margin-top: 4px;
  }

  /* FORM */
  .form-group { margin-bottom: 14px; }
  .form-label {
    display: block;
    font-size: 0.75rem;
    color: var(--muted);
    margin-bottom: 6px;
    letter-spacing: 0.03em;
  }
  .form-input {
    width: 100%;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 9px;
    color: var(--text);
    font-size: 0.95rem;
    padding: 11px 14px;
    outline: none;
    transition: border-color 0.2s, box-shadow 0.2s;
    -webkit-appearance: none;
  }
  .form-input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 3px rgba(6, 182, 212, 0.12);
  }
  .gender-group {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
  }
  .gender-btn {
    padding: 11px;
    border: 1.5px solid var(--border);
    border-radius: 9px;
    background: var(--surface);
    color: var(--muted);
    font-size: 0.9rem;
    cursor: pointer;
    text-align: center;
    transition: all 0.2s;
    -webkit-tap-highlight-color: transparent;
  }
  .gender-btn.active {
    border-color: var(--accent);
    background: rgba(6, 182, 212, 0.1);
    color: var(--accent);
    font-weight: 600;
  }

  /* BUTTONS */
  .btn {
    display: block;
    width: 100%;
    padding: 14px;
    border: none;
    border-radius: 10px;
    font-size: 0.95rem;
    font-weight: 600;
    letter-spacing: 0.04em;
    cursor: pointer;
    transition: all 0.2s;
    -webkit-tap-highlight-color: transparent;
    margin-bottom: 10px;
    position: relative;
    overflow: hidden;
  }
  .btn:active { transform: scale(0.97); }

  .btn-primary {
    background: linear-gradient(135deg, #0891b2, #06b6d4);
    color: #fff;
    box-shadow: 0 4px 15px rgba(6, 182, 212, 0.25);
  }
  .btn-primary:hover {
    box-shadow: 0 6px 20px rgba(6, 182, 212, 0.4);
  }
  .btn-save {
    background: linear-gradient(135deg, #059669, #10b981);
    color: #fff;
    box-shadow: 0 4px 15px rgba(16, 185, 129, 0.25);
  }
  .btn-save:hover {
    box-shadow: 0 6px 20px rgba(16, 185, 129, 0.4);
  }
  .btn-save:disabled {
    background: #1f2937;
    color: var(--muted);
    box-shadow: none;
    cursor: not-allowed;
  }
  .btn-reset {
    background: transparent;
    border: 1px solid var(--border);
    color: var(--muted);
    font-size: 0.85rem;
    padding: 10px;
  }
  .btn-reset:hover { border-color: var(--orange); color: var(--orange); }

  /* STATUS TOAST */
  .toast {
    display: none;
    position: fixed;
    bottom: 20px; left: 50%;
    transform: translateX(-50%);
    padding: 12px 24px;
    border-radius: 25px;
    font-size: 0.88rem;
    font-weight: 600;
    z-index: 1000;
    animation: fadeUp 0.3s ease;
    white-space: nowrap;
  }
  .toast.show { display: block; }
  .toast.success { background: var(--green); color: #fff; }
  .toast.error   { background: var(--red);   color: #fff; }
  .toast.info    { background: var(--accent); color: #fff; }
  @keyframes fadeUp {
    from { opacity: 0; transform: translateX(-50%) translateY(10px); }
    to   { opacity: 1; transform: translateX(-50%) translateY(0); }
  }

  /* LOADING SPINNER */
  .spinner {
    display: inline-block;
    width: 16px; height: 16px;
    border: 2px solid rgba(255,255,255,0.3);
    border-top-color: #fff;
    border-radius: 50%;
    animation: spin 0.7s linear infinite;
    vertical-align: middle;
    margin-right: 6px;
  }
  @keyframes spin { to { transform: rotate(360deg); } }

  /* MEASURING INDICATOR */
  .measuring-badge {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-size: 0.68rem;
    color: var(--accent);
    margin-top: 8px;
  }
  .measuring-dot {
    width: 6px; height: 6px;
    background: var(--accent);
    border-radius: 50%;
    animation: pulse 1s infinite;
  }

  select.form-input { cursor: pointer; }
</style>
</head>
<body>

<!-- HEADER -->
<div class="header">
  <h1>&#x1F476; BABY <span>LENGTH</span> METER</h1>
  <p>Pengukur Panjang Bayi Digital IoT</p>
  <div class="wifi-badge">
    <span class="dot"></span>
    ESP32 Connected
  </div>
</div>

<div class="container">

  <!-- SENSOR READINGS -->
  <div class="sensor-grid">
    <div class="sensor-box us">
      <div class="sensor-label">Ultrasonik<br>Kepala</div>
      <div class="sensor-value" id="us-val">--.-</div>
      <div class="sensor-unit">cm</div>
    </div>
    <div class="sensor-box enc">
      <div class="sensor-label">Encoder<br>Kaki</div>
      <div class="sensor-value" id="enc-val">--.-</div>
      <div class="sensor-unit">cm</div>
    </div>
  </div>

  <!-- BERAT BAYI -->
  <div class="sensor-grid" style="grid-template-columns:1fr;margin-bottom:14px;">
    <div class="sensor-box" style="border-top:2px solid var(--accent2);">
      <div class="sensor-label">Load Cell - Berat Bayi</div>
      <div class="sensor-value" id="w-val" style="color:var(--accent2);">--.-</div>
      <div class="sensor-unit">kg</div>
    </div>
  </div>

  <!-- HASIL PANJANG BAYI -->
  <div class="result-box">
    <div class="result-label">&#x1F4CF; Panjang Bayi</div>
    <div class="result-value zero" id="baby-val">--.-</div>
    <div class="result-unit">centimeter</div>
    <div class="measuring-badge">
      <span class="measuring-dot"></span>
      Realtime update
    </div>
  </div>

  <!-- STATUS FUZZY -->
  <div class="card">
    <div class="card-title">Analisis Status Gizi (Fuzzy Logic)</div>
    <div style="display:grid;gap:8px;">
      <div style="display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:var(--surface);border-radius:9px;border:1px solid var(--border);">
        <span style="font-size:0.8rem;color:var(--muted);">BB/U (Berat/Usia)</span>
        <span id="f-bbu" style="font-size:0.85rem;font-weight:600;color:var(--muted);">-</span>
      </div>
      <div style="display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:var(--surface);border-radius:9px;border:1px solid var(--border);">
        <span style="font-size:0.8rem;color:var(--muted);">PB/U (Panjang/Usia)</span>
        <span id="f-pbu" style="font-size:0.85rem;font-weight:600;color:var(--muted);">-</span>
      </div>
      <div style="display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:var(--surface);border-radius:9px;border:1px solid var(--border);">
        <span style="font-size:0.8rem;color:var(--muted);">IMT/U (IMT/Usia)</span>
        <span id="f-imtu" style="font-size:0.85rem;font-weight:600;color:var(--muted);">-</span>
      </div>
    </div>
    <p style="font-size:0.68rem;color:var(--muted);margin-top:8px;text-align:center;">Dihitung otomatis saat klik SAVE</p>
  </div>

  <!-- DATA PASIEN -->
  <div class="card">
    <div class="card-title">Data Pasien</div>

    <div class="form-group">
      <label class="form-label">Nama Bayi</label>
      <input type="text" class="form-input" id="inp-name"
             placeholder="Masukkan nama bayi..." maxlength="30"
             value=")rawhtml";
  html += name;
  html += R"rawhtml(">
    </div>

    <div class="form-group">
      <label class="form-label">Usia (bulan)</label>
      <input type="number" class="form-input" id="inp-age"
             placeholder="Contoh: 3" min="0" max="60"
             value=")rawhtml";
  html += age;
  html += R"rawhtml(">
    </div>

    <div class="form-group">
      <label class="form-label">Jenis Kelamin</label>
      <div class="gender-group">
        <div class="gender-btn )rawhtml";
  html += (gender == "L" || gender == "") ? "active" : "";
  html += R"rawhtml(" id="gender-l" onclick="selectGender('L')">
          &#x1F466; Laki-laki
        </div>
        <div class="gender-btn )rawhtml";
  html += (gender == "P") ? "active" : "";
  html += R"rawhtml(" id="gender-p" onclick="selectGender('P')">
          &#x1F467; Perempuan
        </div>
      </div>
    </div>

    <button class="btn btn-primary" onclick="savePatient()">
      &#x270F;&#xFE0F; Simpan Data Pasien
    </button>
  </div>

  <!-- TOMBOL AKSI -->
  <div class="card">
    <div class="card-title">Pengukuran</div>

    <button class="btn btn-save" id="btn-save" onclick="saveToSheet()" disabled>
      &#x1F4BE; SAVE KE SPREADSHEET
    </button>

    <button class="btn btn-reset" onclick="resetEncoder()">
      &#x21BA; Reset Encoder (posisikan slider ke ujung kaki)
    </button>
  </div>

  <!-- INFO -->
  <div class="card" style="padding: 12px 16px;">
    <div style="font-size: 0.72rem; color: var(--muted); line-height: 1.7;">
      <strong style="color: var(--accent);">Cara Penggunaan:</strong><br>
      1. Isi data pasien & klik simpan<br>
      2. Letakkan bayi di atas papan<br>
      3. Geser slider hingga menyentuh kaki bayi<br>
      4. Lihat hasil di atas, klik SAVE jika sesuai<br><br>
      <strong style="color: var(--accent);">Formula:</strong>
      Panjang = Posisi Encoder &minus; Jarak Ultrasonik
    </div>
  </div>

  <!-- GANTI WIFI -->
  <div style="text-align:center; padding-bottom: 8px;">
    <a href="/wifi" style="font-size:0.75rem; color: var(--muted);
       text-decoration:none; display:inline-flex; align-items:center; gap:5px;">
      <span>&#x1F4F6;</span> Ganti WiFi
    </a>
  </div>

</div><!-- /container -->

<!-- TOAST NOTIFICATION -->
<div class="toast" id="toast"></div>

<script>
// ================================================================
//  STATE
// ================================================================
let selectedGender = ')rawhtml";
  html += (gender == "P") ? "P" : "L";
  html += R"rawhtml(';
let patientSaved = )rawhtml";
  html += (name != "") ? "true" : "false";
  html += R"rawhtml(;
let isUpdating = false;

// ================================================================
//  REALTIME DATA FETCH (setiap 500ms)
// ================================================================
async function fetchSensorData() {
  try {
    const res = await fetch('/api/data');
    if (!res.ok) return;
    const d = await res.json();

    // Update ultrasonik
    const usEl = document.getElementById('us-val');
    usEl.textContent = d.ultrasonic < 0 ? 'ERR' : parseFloat(d.ultrasonic).toFixed(1);

    // Update encoder
    document.getElementById('enc-val').textContent =
      parseFloat(d.encoder).toFixed(1);

    // Update berat
    if (document.getElementById('w-val')) {
      document.getElementById('w-val').textContent =
        parseFloat(d.weight || 0).toFixed(2);
    }

    // Update status fuzzy
    if (d.bbu && document.getElementById('f-bbu')) {
      var cm = {'BB Normal':'#10b981','PB Normal':'#10b981','Gizi Baik':'#10b981',
                'Tinggi':'#f59e0b','Risiko BB Lebih':'#f59e0b','Risiko Gizi Lebih':'#f59e0b',
                'Risiko BB Kurang':'#ef4444','Risiko Stunting':'#ef4444','Risiko Wasting':'#ef4444'};
      var b=document.getElementById('f-bbu'),p=document.getElementById('f-pbu'),i=document.getElementById('f-imtu');
      b.textContent=d.bbu; b.style.color=cm[d.bbu]||'#f1f5f9';
      p.textContent=d.pbu; p.style.color=cm[d.pbu]||'#f1f5f9';
      i.textContent=d.imtu; i.style.color=cm[d.imtu]||'#f1f5f9';
    }

    // Update hasil panjang bayi
    const babyEl = document.getElementById('baby-val');
    const len = parseFloat(d.length);
    babyEl.textContent = len.toFixed(1);
    babyEl.classList.toggle('zero', len <= 0);

    // Aktifkan tombol save jika ada data valid
    const canSave = patientSaved && len > 0;
    document.getElementById('btn-save').disabled = !canSave;

  } catch(e) {
    // Diam saja jika gagal fetch
  }
}

// Mulai polling realtime
setInterval(fetchSensorData, 500);
fetchSensorData(); // Langsung fetch saat load

// ================================================================
//  PILIH GENDER
// ================================================================
function selectGender(g) {
  selectedGender = g;
  document.getElementById('gender-l').classList.toggle('active', g === 'L');
  document.getElementById('gender-p').classList.toggle('active', g === 'P');
}

// ================================================================
//  SIMPAN DATA PASIEN
// ================================================================
async function savePatient() {
  const name = document.getElementById('inp-name').value.trim();
  const age  = document.getElementById('inp-age').value.trim();

  if (!name) { showToast('Masukkan nama bayi!', 'error'); return; }
  if (!age)  { showToast('Masukkan usia bayi!', 'error'); return; }

  const form = new URLSearchParams();
  form.append('name',   name);
  form.append('age',    age);
  form.append('gender', selectedGender);

  try {
    const res = await fetch('/api/patient', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: form.toString()
    });
    const d = await res.json();
    if (d.status === 'ok') {
      patientSaved = true;
      showToast('Data pasien tersimpan!', 'success');
    }
  } catch(e) {
    showToast('Gagal menyimpan pasien', 'error');
  }
}

// ================================================================
//  SAVE KE SPREADSHEET
// ================================================================
async function saveToSheet() {
  const btn = document.getElementById('btn-save');
  btn.disabled = true;
  btn.innerHTML = '<span class="spinner"></span> Menyimpan...';

  try {
    const res = await fetch('/api/save', { method: 'POST' });
    const d = await res.json();
    if (d.status === 'ok') {
      showToast('&#x2705; Data berhasil disimpan ke Spreadsheet!', 'success');
    } else {
      showToast('&#x274C; Gagal: ' + d.msg, 'error');
    }
  } catch(e) {
    showToast('&#x274C; Error koneksi ke ESP32', 'error');
  }

  // Restore tombol
  btn.innerHTML = '&#x1F4BE; SAVE KE SPREADSHEET';
  btn.disabled = false;
}

// ================================================================
//  RESET ENCODER
// ================================================================
async function resetEncoder() {
  try {
    await fetch('/api/reset', { method: 'POST' });
    showToast('Encoder direset', 'info');
  } catch(e) {}
}

// ================================================================
//  SHOW TOAST NOTIFICATION
// ================================================================
function showToast(msg, type = 'info') {
  const t = document.getElementById('toast');
  t.innerHTML = msg;
  t.className = 'toast show ' + type;
  clearTimeout(t._timer);
  t._timer = setTimeout(() => t.classList.remove('show'), 3000);
}
</script>
</body>
</html>
)rawhtml";

  return html;
}

#endif // WEBUI_H
