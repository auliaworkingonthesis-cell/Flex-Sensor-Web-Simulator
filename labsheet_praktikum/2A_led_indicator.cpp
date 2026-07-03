/**
 * ============================================================
 *  Labsheet 2 - Percobaan A: Kontrol Indikator 3 LED Terpisah
 * ============================================================
 *  Tujuan: Menyalakan lampu LED sesuai sudut tekukan sensor flex.
 *          Lurus (0°)      → LED Hijau aktif
 *          Tekuk 90°       → LED Kuning aktif
 *          Tekuk Maks (180°)→ LED Merah aktif
 */

#include <Arduino.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Nilai threshold ADC hasil kalibrasi (Semakin bengkok, ADC menurun)
#define THRESHOLD_GREEN   1260  // ADC >= 1260 -> Hijau (Lurus s.d hampir 90°)
#define THRESHOLD_YELLOW  1210  // 1210 <= ADC < 1260 -> Kuning (Sekitar 90°)
                                // ADC < 1210 -> Merah (Bengkok maksimal)

void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Konfigurasi Pin LED sebagai OUTPUT
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    
    // Matikan semua LED di awal
    setLed(false, false, false);
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // Logika pembagian kondisi LED berdasarkan tingkat lekukan (ADC)
    if (rawADC >= THRESHOLD_GREEN) {
        // Posisi lurus / posisi awal (Hijau)
        setLed(false, false, true);
        Serial.println("Kondisi: LURUS (LED HIJAU)");
    } 
    else if (rawADC >= THRESHOLD_YELLOW) {
        // Melengkung sekitar 90 derajat (Kuning)
        setLed(false, true, false);
        Serial.println("Kondisi: TEKUK ~90° (LED KUNING)");
    } 
    else {
        // Melengkung maksimal / mendekati 180 derajat (Merah)
        setLed(true, false, false);
        Serial.println("Kondisi: BENGKOK MAKSIMAL (LED MERAH)");
    }
    
    delay(100);
}
