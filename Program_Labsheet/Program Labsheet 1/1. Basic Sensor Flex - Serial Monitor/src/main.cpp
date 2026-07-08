#include <Arduino.h>

/**
 * ============================================================
 *  Program Labsheet 1: Value Flex di Serial Monitor (Tanpa Average)
 * ============================================================
 *  Deskripsi: Membaca nilai ADC analog mentah secara langsung dari Sensor Flex A
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
    
    
    Serial.println("Program Labsheet 1 - Serial Monitor Ready (Tanpa Average)");
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        // Baca nilai analog mentah langsung tanpa filter/average
        int rawADC = analogRead(FLEX_A_PIN);
        
        // Rangkaian Pembagi Tegangan (Voltage Divider):
        // Supply Vin = 5.0V
        // R1 = Sensor Flex (kebengkokan naik -> resistansi naik)
        // R2 = Resistor Tetap 22k Ohm (ke GND)
        // Vout = Vin * (R2 / (R1 + R2)) -> Dihubungkan ke pin ADC ESP32
        // Tegangan yang dibaca pada pin ADC ESP32 (range 0 - 3.3V):
        float voltA = (rawADC * 3.3) / 4095.0;
        
        // Cetak ke Serial Monitor
        Serial.print("Flex A ADC Value (Direct): ");
        Serial.print(rawADC);
        Serial.print(" | Voltage: ");
        Serial.print(voltA, 2);
        Serial.println(" V");
    }
}
