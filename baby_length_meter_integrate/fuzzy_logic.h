/*
 * fuzzy.h
 * Modul Fuzzy Logic untuk penilaian status gizi bayi
 * Diadopsi dari program referensi - rule base 27 aturan
 *
 * Library yang dibutuhkan: "Fuzzy" by Marcos Toledo
 * Install via Library Manager: search "Fuzzy"
 *
 * INPUT  : Berat (kg), Panjang (cm), IMT (kg/m2), Usia (bulan)
 * OUTPUT : Status BB/U, PB/U, IMT/U
 */

#ifndef FUZZY_LOGIC_H
#define FUZZY_LOGIC_H

#include <Fuzzy.h>  // WAJIB: install library "Fuzzy" by Marcos Toledo

// Fuzzy.h sudah di-include di file utama (.ino) sebelum header ini
// #include <Fuzzy.h> dipindah ke .ino untuk menghindari konflik urutan compile
#include "config.h"

// ================================================================
//  OBJEK FUZZY GLOBAL
// ================================================================
Fuzzy* fuzzy = new Fuzzy();

// ================================================================
//  STRUCT HASIL FUZZY
// ================================================================
struct FuzzyResult {
  float   bbu       = 0.0f;  // nilai defuzzify BB/U
  float   pbu       = 0.0f;  // nilai defuzzify PB/U
  float   imtu      = 0.0f;  // nilai defuzzify IMT/U
  float   bmi       = 0.0f;  // IMT = berat(kg) / tinggi(m)^2
  String  statusBBU = "-";   // "Risiko BB Kurang" / "BB Normal" / "Risiko BB Lebih"
  String  statusPBU = "-";   // "Risiko Stunting"  / "PB Normal" / "Tinggi"
  String  statusIMTU= "-";   // "Risiko Wasting"   / "Gizi Baik" / "Risiko Gizi Lebih"
};

// ================================================================
//  setupFuzzy() - Inisialisasi membership functions & rule base
// ================================================================
/*
 * MEMBERSHIP FUNCTIONS (Trapesium):
 *   FuzzySet(a, b, c, d)
 *   a = awal naik, b = puncak kiri, c = puncak kanan, d = akhir turun
 *
 * Untuk set paling kiri  : a == b (langsung puncak dari 0)
 * Untuk set paling kanan : c == d (tetap di puncak sampai akhir)
 */
void setupFuzzy() {

  // ========================
  //  INPUT 1: Berat Badan (kg)
  // ========================
  FuzzyInput* Berat = new FuzzyInput(1);
  FuzzySet* BKurang = new FuzzySet(3.4,  3.4,  5.1,  5.8);
  FuzzySet* BNormal = new FuzzySet(5.1,  5.8,  6.7,  8.8);
  FuzzySet* BLebih  = new FuzzySet(6.7,  8.8, 10.8, 10.8);
  Berat->addFuzzySet(BKurang);
  Berat->addFuzzySet(BNormal);
  Berat->addFuzzySet(BLebih);
  fuzzy->addFuzzyInput(Berat);

  // ========================
  //  INPUT 2: Panjang Badan (cm)
  // ========================
  FuzzyInput* Panjang = new FuzzyInput(2);
  FuzzySet* PKurang = new FuzzySet(50.8, 50.8, 60.6, 61.4);
  FuzzySet* PNormal = new FuzzySet(60.6, 61.4, 71.0, 71.9);
  FuzzySet* PLebih  = new FuzzySet(71.0, 71.9, 83.0, 83.0);
  Panjang->addFuzzySet(PKurang);
  Panjang->addFuzzySet(PNormal);
  Panjang->addFuzzySet(PLebih);
  fuzzy->addFuzzyInput(Panjang);

  // ========================
  //  INPUT 3: IMT / BMI (kg/m2)
  // ========================
  FuzzyInput* IMT = new FuzzyInput(3);
  FuzzySet* IKurang = new FuzzySet(12.4, 12.4, 13.7, 14.8);
  FuzzySet* INormal = new FuzzySet(13.7, 14.8, 16.3, 17.3);
  FuzzySet* ILebih  = new FuzzySet(16.3, 17.3, 18.8, 18.8);
  IMT->addFuzzySet(IKurang);
  IMT->addFuzzySet(INormal);
  IMT->addFuzzySet(ILebih);
  fuzzy->addFuzzyInput(IMT);

  // ========================
  //  INPUT 4: Usia (bulan)
  // ========================
  FuzzyInput* Usia = new FuzzyInput(4);
  FuzzySet* Neonatal     = new FuzzySet(0, 0, 1, 2);
  FuzzySet* infantAwal   = new FuzzySet(1, 2, 6, 7);
  FuzzySet* infantLanjut = new FuzzySet(6, 7, 12, 12);
  Usia->addFuzzySet(Neonatal);
  Usia->addFuzzySet(infantAwal);
  Usia->addFuzzySet(infantLanjut);
  fuzzy->addFuzzyInput(Usia);

  // ========================
  //  OUTPUT 1: BB/U (Berat Badan per Usia)
  //  Threshold: <3=Kurang, 3-8=Normal, >=8=Obesitas
  // ========================
  FuzzyOutput* BB_U = new FuzzyOutput(1);
  FuzzySet* BBKurang   = new FuzzySet(0, 0,  2,  3);
  FuzzySet* BBNormal   = new FuzzySet(2, 3,  7,  8);
  FuzzySet* BBObesitas = new FuzzySet(7, 8, 10, 10);
  BB_U->addFuzzySet(BBKurang);
  BB_U->addFuzzySet(BBNormal);
  BB_U->addFuzzySet(BBObesitas);
  fuzzy->addFuzzyOutput(BB_U);

  // ========================
  //  OUTPUT 2: PB/U (Panjang Badan per Usia)
  //  Threshold: <3=Stunting, 3-8=Normal, >=8=Tinggi
  // ========================
  FuzzyOutput* PB_U = new FuzzyOutput(2);
  FuzzySet* Stunting = new FuzzySet(0, 0, 2,  3);
  FuzzySet* PBNormal = new FuzzySet(2, 3, 7,  8);
  FuzzySet* Tinggi   = new FuzzySet(7, 8, 10, 10);
  PB_U->addFuzzySet(Stunting);
  PB_U->addFuzzySet(PBNormal);
  PB_U->addFuzzySet(Tinggi);
  fuzzy->addFuzzyOutput(PB_U);

  // ========================
  //  OUTPUT 3: IMT/U (IMT per Usia)
  //  Threshold: <2=Wasting, 2-8=GiziBaik, >=8=Obesitas
  // ========================
  FuzzyOutput* IMT_U = new FuzzyOutput(3);
  FuzzySet* Wasting   = new FuzzySet(0, 0, 1,  2);
  FuzzySet* GiziBaik  = new FuzzySet(1, 2, 6,  8);
  FuzzySet* IObesitas = new FuzzySet(6, 8, 10, 10);
  IMT_U->addFuzzySet(Wasting);
  IMT_U->addFuzzySet(GiziBaik);
  IMT_U->addFuzzySet(IObesitas);
  fuzzy->addFuzzyOutput(IMT_U);

  // ================================================================
  //  RULE BASE BB/U (9 aturan): Berat x Usia -> BB/U
  // ================================================================
  // R1: Berat Kurang + Neonatal -> BB Normal
  FuzzyRuleAntecedent* ant1 = new FuzzyRuleAntecedent();
  ant1->joinWithAND(BKurang, Neonatal);
  FuzzyRuleConsequent* con_BBNormal = new FuzzyRuleConsequent();
  con_BBNormal->addOutput(BBNormal);
  fuzzy->addFuzzyRule(new FuzzyRule(1, ant1, con_BBNormal));

  // R2: Berat Kurang + infantAwal -> BB Kurang
  FuzzyRuleAntecedent* ant2 = new FuzzyRuleAntecedent();
  ant2->joinWithAND(BKurang, infantAwal);
  FuzzyRuleConsequent* con_BBKurang = new FuzzyRuleConsequent();
  con_BBKurang->addOutput(BBKurang);
  fuzzy->addFuzzyRule(new FuzzyRule(2, ant2, con_BBKurang));

  // R3: Berat Kurang + infantLanjut -> BB Kurang
  FuzzyRuleAntecedent* ant3 = new FuzzyRuleAntecedent();
  ant3->joinWithAND(BKurang, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(3, ant3, con_BBKurang));

  // R4: Berat Normal + Neonatal -> BB Obesitas
  FuzzyRuleAntecedent* ant4 = new FuzzyRuleAntecedent();
  ant4->joinWithAND(BNormal, Neonatal);
  FuzzyRuleConsequent* con_BBObesitas = new FuzzyRuleConsequent();
  con_BBObesitas->addOutput(BBObesitas);
  fuzzy->addFuzzyRule(new FuzzyRule(4, ant4, con_BBObesitas));

  // R5: Berat Normal + infantAwal -> BB Normal
  FuzzyRuleAntecedent* ant5 = new FuzzyRuleAntecedent();
  ant5->joinWithAND(BNormal, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(5, ant5, con_BBNormal));

  // R6: Berat Normal + infantLanjut -> BB Kurang
  FuzzyRuleAntecedent* ant6 = new FuzzyRuleAntecedent();
  ant6->joinWithAND(BNormal, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(6, ant6, con_BBKurang));

  // R7: Berat Lebih + Neonatal -> BB Obesitas
  FuzzyRuleAntecedent* ant7 = new FuzzyRuleAntecedent();
  ant7->joinWithAND(BLebih, Neonatal);
  fuzzy->addFuzzyRule(new FuzzyRule(7, ant7, con_BBObesitas));

  // R8: Berat Lebih + infantAwal -> BB Obesitas
  FuzzyRuleAntecedent* ant8 = new FuzzyRuleAntecedent();
  ant8->joinWithAND(BLebih, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(8, ant8, con_BBObesitas));

  // R9: Berat Lebih + infantLanjut -> BB Normal
  FuzzyRuleAntecedent* ant9 = new FuzzyRuleAntecedent();
  ant9->joinWithAND(BLebih, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(9, ant9, con_BBNormal));

  // ================================================================
  //  RULE BASE PB/U (9 aturan): Panjang x Usia -> PB/U
  // ================================================================
  // R10: Panjang Kurang + Neonatal -> PB Normal
  FuzzyRuleAntecedent* ant10 = new FuzzyRuleAntecedent();
  ant10->joinWithAND(PKurang, Neonatal);
  FuzzyRuleConsequent* con_PBNormal = new FuzzyRuleConsequent();
  con_PBNormal->addOutput(PBNormal);
  fuzzy->addFuzzyRule(new FuzzyRule(10, ant10, con_PBNormal));

  // R11: Panjang Kurang + infantAwal -> Stunting
  FuzzyRuleAntecedent* ant11 = new FuzzyRuleAntecedent();
  ant11->joinWithAND(PKurang, infantAwal);
  FuzzyRuleConsequent* con_Stunting = new FuzzyRuleConsequent();
  con_Stunting->addOutput(Stunting);
  fuzzy->addFuzzyRule(new FuzzyRule(11, ant11, con_Stunting));

  // R12: Panjang Kurang + infantLanjut -> Stunting
  FuzzyRuleAntecedent* ant12 = new FuzzyRuleAntecedent();
  ant12->joinWithAND(PKurang, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(12, ant12, con_Stunting));

  // R13: Panjang Normal + Neonatal -> Tinggi
  FuzzyRuleAntecedent* ant13 = new FuzzyRuleAntecedent();
  ant13->joinWithAND(PNormal, Neonatal);
  FuzzyRuleConsequent* con_Tinggi = new FuzzyRuleConsequent();
  con_Tinggi->addOutput(Tinggi);
  fuzzy->addFuzzyRule(new FuzzyRule(13, ant13, con_Tinggi));

  // R14: Panjang Normal + infantAwal -> PB Normal
  FuzzyRuleAntecedent* ant14 = new FuzzyRuleAntecedent();
  ant14->joinWithAND(PNormal, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(14, ant14, con_PBNormal));

  // R15: Panjang Normal + infantLanjut -> Stunting
  FuzzyRuleAntecedent* ant15 = new FuzzyRuleAntecedent();
  ant15->joinWithAND(PNormal, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(15, ant15, con_Stunting));

  // R16: Panjang Lebih + Neonatal -> Tinggi
  FuzzyRuleAntecedent* ant16 = new FuzzyRuleAntecedent();
  ant16->joinWithAND(PLebih, Neonatal);
  fuzzy->addFuzzyRule(new FuzzyRule(16, ant16, con_Tinggi));

  // R17: Panjang Lebih + infantAwal -> Tinggi
  FuzzyRuleAntecedent* ant17 = new FuzzyRuleAntecedent();
  ant17->joinWithAND(PLebih, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(17, ant17, con_Tinggi));

  // R18: Panjang Lebih + infantLanjut -> PB Normal
  FuzzyRuleAntecedent* ant18 = new FuzzyRuleAntecedent();
  ant18->joinWithAND(PLebih, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(18, ant18, con_PBNormal));

  // ================================================================
  //  RULE BASE IMT/U (9 aturan): IMT x Usia -> IMT/U
  // ================================================================
  // R19: IMT Kurang + Neonatal -> Gizi Baik
  FuzzyRuleAntecedent* ant19 = new FuzzyRuleAntecedent();
  ant19->joinWithAND(IKurang, Neonatal);
  FuzzyRuleConsequent* con_GiziBaik = new FuzzyRuleConsequent();
  con_GiziBaik->addOutput(GiziBaik);
  fuzzy->addFuzzyRule(new FuzzyRule(19, ant19, con_GiziBaik));

  // R20: IMT Kurang + infantAwal -> Wasting
  FuzzyRuleAntecedent* ant20 = new FuzzyRuleAntecedent();
  ant20->joinWithAND(IKurang, infantAwal);
  FuzzyRuleConsequent* con_Wasting = new FuzzyRuleConsequent();
  con_Wasting->addOutput(Wasting);
  fuzzy->addFuzzyRule(new FuzzyRule(20, ant20, con_Wasting));

  // R21: IMT Kurang + infantLanjut -> Wasting
  FuzzyRuleAntecedent* ant21 = new FuzzyRuleAntecedent();
  ant21->joinWithAND(IKurang, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(21, ant21, con_Wasting));

  // R22: IMT Normal + Neonatal -> Gizi Baik
  // (program referensi: thenIMT_UObesitas tapi addOutput GiziBaik - ikuti output aktualnya)
  FuzzyRuleAntecedent* ant22 = new FuzzyRuleAntecedent();
  ant22->joinWithAND(INormal, Neonatal);
  fuzzy->addFuzzyRule(new FuzzyRule(22, ant22, con_GiziBaik));

  // R23: IMT Normal + infantAwal -> Gizi Baik
  FuzzyRuleAntecedent* ant23 = new FuzzyRuleAntecedent();
  ant23->joinWithAND(INormal, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(23, ant23, con_GiziBaik));

  // R24: IMT Normal + infantLanjut -> Gizi Baik
  FuzzyRuleAntecedent* ant24 = new FuzzyRuleAntecedent();
  ant24->joinWithAND(INormal, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(24, ant24, con_GiziBaik));

  // R25: IMT Lebih + Neonatal -> Obesitas
  FuzzyRuleAntecedent* ant25 = new FuzzyRuleAntecedent();
  ant25->joinWithAND(ILebih, Neonatal);
  FuzzyRuleConsequent* con_IObesitas = new FuzzyRuleConsequent();
  con_IObesitas->addOutput(IObesitas);
  fuzzy->addFuzzyRule(new FuzzyRule(25, ant25, con_IObesitas));

  // R26: IMT Lebih + infantAwal -> Gizi Baik
  FuzzyRuleAntecedent* ant26 = new FuzzyRuleAntecedent();
  ant26->joinWithAND(ILebih, infantAwal);
  fuzzy->addFuzzyRule(new FuzzyRule(26, ant26, con_GiziBaik));

  // R27: IMT Lebih + infantLanjut -> Gizi Baik
  FuzzyRuleAntecedent* ant27 = new FuzzyRuleAntecedent();
  ant27->joinWithAND(ILebih, infantLanjut);
  fuzzy->addFuzzyRule(new FuzzyRule(27, ant27, con_GiziBaik));

  Serial.println("[OK] Fuzzy: 27 rules loaded.");
}

// ================================================================
//  runFuzzy() - Jalankan inferensi fuzzy
// ================================================================
/*
 * @param weightKg  Berat bayi dalam kilogram
 * @param lengthCm  Panjang bayi dalam centimeter
 * @param ageMonth  Usia bayi dalam bulan
 * @return FuzzyResult struct berisi nilai & status ketiga output
 */
FuzzyResult runFuzzy(float weightKg, float lengthCm, int ageMonth) {
  FuzzyResult result;

  // Hitung IMT (Indeks Massa Tubuh = kg / m^2)
  float heightM = lengthCm / 100.0f;
  result.bmi = (heightM > 0.0f) ? (weightKg / (heightM * heightM)) : 0.0f;

  // Set input fuzzy
  fuzzy->setInput(1, weightKg);
  fuzzy->setInput(2, lengthCm);
  fuzzy->setInput(3, result.bmi);
  fuzzy->setInput(4, (float)ageMonth);

  // Proses fuzzifikasi + inferensi + defuzzifikasi
  fuzzy->fuzzify();

  result.bbu  = fuzzy->defuzzify(1);
  result.pbu  = fuzzy->defuzzify(2);
  result.imtu = fuzzy->defuzzify(3);

  // Konversi nilai crisp ke label status
  // BB/U
  if      (result.bbu < 3.0f) result.statusBBU = "Risiko BB Kurang";
  else if (result.bbu < 8.0f) result.statusBBU = "BB Normal";
  else                         result.statusBBU = "Risiko BB Lebih";

  // PB/U
  if      (result.pbu < 3.0f) result.statusPBU = "Risiko Stunting";
  else if (result.pbu < 8.0f) result.statusPBU = "PB Normal";
  else                         result.statusPBU = "Tinggi";

  // IMT/U
  if      (result.imtu < 2.0f) result.statusIMTU = "Risiko Wasting";
  else if (result.imtu < 8.0f) result.statusIMTU = "Gizi Baik";
  else                          result.statusIMTU = "Risiko Gizi Lebih";

  return result;
}

#endif // FUZZY_LOGIC_H
