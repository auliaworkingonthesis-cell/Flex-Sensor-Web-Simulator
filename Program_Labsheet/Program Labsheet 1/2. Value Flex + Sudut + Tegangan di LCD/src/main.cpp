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
#define FLEX_A_MIN  3054
#define FLEX_A_MAX  2766
#define FLEX_B_MIN  3054
#define FLEX_B_MAX  2766

// LCD I2C alamat 0x27 (lebar 16 kolom, 4 baris)
LiquidCrystal_I2C lcd(0x27, 16, 4);

unsigned long lastUpdate = 0;
const unsigned long interval = 200; // Update LCD setiap 200ms

void setup() {
    Serial.begin(115200);
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
        
        // Estimasi sudut untuk S1 & S2
        int lowA = min(FLEX_A_MIN, FLEX_A_MAX);
        int highA = max(FLEX_A_MIN, FLEX_A_MAX);
        int angleA = map(constrain(rawADC, lowA, highA), FLEX_A_MIN, FLEX_A_MAX, 0, 180);
        
        int lowB = min(FLEX_B_MIN, FLEX_B_MAX);
        int highB = max(FLEX_B_MIN, FLEX_B_MAX);
        int angleB = map(constrain(rawADC_B, lowB, highB), FLEX_B_MIN, FLEX_B_MAX, 0, 180);
        
        // Tampilkan di LCD
        char line0[17], line1[17], line2[17];
        snprintf(line0, sizeof(line0), "FA:%4d | %.2fV", rawADC, voltA);
        snprintf(line1, sizeof(line1), "FB:%4d | %.2fV", rawADC_B, voltB);
        snprintf(line2, sizeof(line2), "S1:%3d\xDF   S2:%3d\xDF", angleA, angleB);
        
        lcd.setCursor(0, 0);
        lcd.print(line0);
        
        lcd.setCursor(0, 1);
        lcd.print(line1);
        
        lcd.setCursor(0, 2);
        lcd.print(line2);
        
        lcd.setCursor(0, 3);
        lcd.print("Trainer : READY ");
        Serial.printf("Flex A: %d | Flex B: %d | Volt A: %.2fV | Volt B: %.2fV\n", rawADC, rawADC_B, voltA, voltB);
    }
}
