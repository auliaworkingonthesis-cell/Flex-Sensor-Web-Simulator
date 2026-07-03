/**
 * ============================================================
 *  Labsheet 1 - Percobaan C: Pembacaan Basic Sensor Flex
 * ============================================================
 *  Tujuan: Membaca nilai ADC (digital) mentah dari sensor flex
 *          dan menampilkannya di Serial Monitor.
 */

#include <Arduino.h>

#define FLEX_A_PIN  34  // Pin ADC1_CH6 untuk Sensor Flex A

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi resolusi ADC 12-bit (0 - 4095)
    analogReadResolution(12);
    // Set atenuasi 11dB agar ADC bisa membaca tegangan hingga 3.3V
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    Serial.println("Percobaan 1C: Pembacaan Basic Serial Mulai");
}

void loop() {
    // Baca nilai analog mentah (0 - 4095)
    int rawADC = analogRead(FLEX_A_PIN);
    
    // Tampilkan ke Serial Monitor
    Serial.print("Nilai ADC Flex A: ");
    Serial.println(rawADC);
    
    delay(100);  // Delay 100ms agar data tidak terlalu cepat bergulir
}
