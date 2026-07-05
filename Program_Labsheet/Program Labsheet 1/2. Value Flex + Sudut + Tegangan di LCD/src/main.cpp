#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 1: Value Flex + Sudut + Tegangan di LCD
 * ============================================================
 *  Deskripsi: Menampilkan nilai ADC, estimasi sudut linear, dan
 *             nilai tegangan analog ke layar LCD 16x4 secara non-blocking.
 */

#include <LiquidCrystal_I2C.h>

#ifndef FLEX_B_PIN
#define FLEX_B_PIN 35
#endif

#ifndef USE_FLEX_A_FOR_SERVO
#define USE_FLEX_A_FOR_SERVO true
#endif

// Fungsi untuk membaca rata-rata analog (Oversampling 10 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 10;
}


#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN  35  // Pin ADC untuk Sensor Flex B

// LCD I2C alamat 0x27 (lebar 16 kolom, 4 baris)
LiquidCrystal_I2C lcd(0x27, 16, 4);

unsigned long lastUpdate = 0;
const unsigned long interval = 200; // Update LCD setiap 200ms

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    
    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = readAverage(FLEX_A_PIN);
        int rawADC_B = readAverage(FLEX_B_PIN);
        
        // Estimasi tegangan
        float voltA = (rawADC * 3.465) / 4095.0;
        float voltB = (rawADC_B * 3.465) / 4095.0;
        
        // Tampilkan di LCD
        // Baris 0: Flex A ADC
        lcd.setCursor(0, 0);
        lcd.print("Flex A  : ");
        lcd.print(rawADC);
        lcd.print("    "); 
        
        // Baris 1: Flex B ADC
        lcd.setCursor(0, 1);
        lcd.print("Flex B  : ");
        lcd.print(rawADC_B);
        lcd.print("    ");
        
        // Baris 2: Tegangan Flex A
        lcd.setCursor(0, 2);
        lcd.print("Volt A  : ");
        lcd.print(voltA, 2);
        lcd.print(" V  ");
        
        // Baris 3: Tegangan Flex B
        lcd.setCursor(0, 3);
        lcd.print("Volt B  : ");
        lcd.print(voltB, 2);
        lcd.print(" V  ");
    }
}
