#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 1: Value Flex di Serial Monitor
 * ============================================================
 *  Deskripsi: Membaca nilai ADC analog mentah dari Sensor Flex A
 *             dan menampilkannya di Serial Monitor secara non-blocking.
 */

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A

unsigned long lastUpdate = 0;
const unsigned long interval = 100; // Interval pembacaan (ms)

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi resolusi ADC 12-bit (0 - 4095)
    analogReadResolution(12);
    // Set atenuasi 11dB agar ADC bisa membaca tegangan 3.3V
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    Serial.println("Program Labsheet 1 - Serial Monitor Ready (Non-blocking)");
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        // Baca nilai analog mentah
        int rawADC = analogRead(FLEX_A_PIN);
        
        // Cetak ke Serial Monitor
        Serial.print("Flex A ADC Value: ");
        Serial.println(rawADC);
    }
}
