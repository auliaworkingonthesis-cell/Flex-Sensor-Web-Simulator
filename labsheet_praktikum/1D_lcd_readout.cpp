/**
 * ============================================================
 *  Labsheet 1 - Percobaan D: Pembacaan Sensor Flex ke LCD 16x4
 * ============================================================
 *  Tujuan: Menampilkan nilai ADC, estimasi sudut lekukan, dan
 *          tegangan output sensor flex ke layar LCD 16x4.
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define FLEX_A_PIN  34  // Pin ADC1_CH6 untuk Sensor Flex A

// Batas kalibrasi ADC berdasarkan pengukuran fisik (resistor 10k)
#define FLEX_A_MIN  1320  // ADC saat sensor LURUS
#define FLEX_A_MAX  1198  // ADC saat sensor BENGKOK PENUH

// Inisialisasi LCD I2C alamat 0x27 (lebar 16 kolom, 4 baris)
LiquidCrystal_I2C lcd(0x27, 16, 4);

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    lcd.setCursor(0, 0);
    lcd.print("Labsheet 1D: LCD");
    delay(1000);
    lcd.clear();
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // Hitung estimasi tegangan pembagi (3.3V ref, 12-bit ADC)
    float voltage = (rawADC * 3.3) / 4095.0;
    
    // Batasi nilai ADC untuk pengolahan sudut
    int constrainedADC = constrain(rawADC, min(FLEX_A_MIN, FLEX_A_MAX), max(FLEX_A_MIN, FLEX_A_MAX));
    // Mapping sudut linear: 1320 -> 0 derajat, 1198 -> 180 derajat
    int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
    
    // Tampilkan di LCD
    // Baris 0: Nilai ADC
    lcd.setCursor(0, 0);
    lcd.print("ADC Val: ");
    lcd.print(rawADC);
    lcd.print("    "); // Menghapus karakter sisa
    
    // Baris 1: Tegangan
    lcd.setCursor(0, 1);
    lcd.print("Voltage: ");
    lcd.print(voltage, 2);
    lcd.print(" V  ");
    
    // Baris 2: Estimasi Sudut
    lcd.setCursor(0, 2);
    lcd.print("Angle  : ");
    lcd.print(angle);
    lcd.print(" deg ");
    
    // Baris 3: Status
    lcd.setCursor(0, 3);
    lcd.print("Status : OK      ");
    
    delay(200); // Update tampilan tiap 200ms
}
