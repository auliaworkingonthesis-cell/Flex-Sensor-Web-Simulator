#include <Arduino.h>

// Fungsi untuk membaca rata-rata analog (Oversampling 10 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}

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

#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Threshold kalibrasi ADC
#define THRESHOLD_GREEN   3027
#define THRESHOLD_YELLOW  2780

unsigned long lastUpdate = 0;
const unsigned long interval = 50; // Update LED setiap 50ms

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
        
        int rawADC = readAverage(FLEX_A_PIN);
        
        // Logika pemilihan LED yang menyala
        Serial.printf("Flex A: %d | LED: %s\n", rawADC, (rawADC >= THRESHOLD_GREEN) ? "HIJAU" : ((rawADC >= THRESHOLD_YELLOW) ? "KUNING" : "MERAH"));
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
