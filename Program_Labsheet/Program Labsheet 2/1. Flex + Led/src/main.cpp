#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Led
 * ============================================================
 *  Deskripsi: Kontrol lampu LED (Merah, Kuning, Hijau) berdasarkan
 *             derajat lekukan sensor flex A secara non-blocking.
 *             Lurus (ADC >= 2910)        → Hijau (Pin 27)
 *             Bengkok ~90° (2795-2909)   → Kuning (Pin 26)
 *             Bengkok Maks (ADC < 2795)  → Merah (Pin 25)
 */

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Threshold kalibrasi ADC
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2795

unsigned long lastUpdate = 0;
const unsigned long interval = 50; // Update LED setiap 50ms

void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Konfigurasi pin LED sebagai OUTPUT
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    setLed(false, false, false);
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = analogRead(FLEX_A_PIN);
        
        // Logika pemilihan LED yang menyala
        if (rawADC >= THRESHOLD_GREEN) {
            setLed(false, false, true);   // Hijau aktif (Lurus)
        } 
        else if (rawADC >= THRESHOLD_YELLOW) {
            setLed(false, true, false);   // Kuning aktif (Bengkok ~90°)
        } 
        else {
            setLed(true, false, false);   // Merah aktif (Bengkok maksimal)
        }
    }
}
