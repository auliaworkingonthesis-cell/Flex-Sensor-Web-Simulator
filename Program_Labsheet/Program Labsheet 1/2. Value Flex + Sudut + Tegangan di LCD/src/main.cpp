#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 1: Value Flex + Sudut + Tegangan di LCD
 * ============================================================
 *  Deskripsi: Menampilkan nilai ADC, estimasi sudut linear, dan
 *             nilai tegangan analog ke layar LCD 16x4 secara non-blocking.
 */

#include <LiquidCrystal_I2C.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  3054  // ADC saat lurus (0 derajat)
#define FLEX_A_MAX  2766  // ADC saat bengkok maksimal (180 derajat)

// LCD I2C alamat 0x27 (lebar 16 kolom, 4 baris)
LiquidCrystal_I2C lcd(0x27, 16, 4);

unsigned long lastUpdate = 0;
const unsigned long interval = 200; // Update LCD setiap 200ms

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = analogRead(FLEX_A_PIN);
        
        // Estimasi tegangan
        float voltage = (rawADC * 3.465) / 4095.0;
        
        // Urutkan limit constrain agar tidak error
        int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
        int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        
        // Mapping sudut linear 0 s.d 180 derajat
        int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
        
        // Tampilkan di LCD
        lcd.setCursor(0, 0);
        lcd.print("Flex ADC: ");
        lcd.print(rawADC);
        lcd.print("    "); 
        
        lcd.setCursor(0, 1);
        lcd.print("Sudut   : ");
        lcd.print(angle);
        lcd.print(" deg ");
        
        lcd.setCursor(0, 2);
        lcd.print("Tegangan: ");
        lcd.print(voltage, 2);
        lcd.print(" V  ");
        
        lcd.setCursor(0, 3);
        lcd.print("LCD Readout: OK ");
    }
}
