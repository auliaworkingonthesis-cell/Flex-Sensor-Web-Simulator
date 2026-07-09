#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo + Lampu + LCD
 * ============================================================
 *  Deskripsi: Integrasi lengkap modul Servo Motor, indikator 3 LED, 
 *             dan penampilan data statusnya ke layar LCD 16x4 secara non-blocking.
 *             Hanya menggunakan Sensor Flex A.
 */

#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
// Nilai lurus (tidak bengkok) → ADC tinggi
// Nilai bengkok maksimal     → ADC rendah
#define FLEX_A_MIN  3000  // ADC saat lurus
#define FLEX_A_MAX  2723  // ADC saat bengkok maksimal

// Threshold LED
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2790

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 4);

int lastAngle = -1;
unsigned long lastServoUpdate = 0;
unsigned long lastLcdUpdate = 0;

// Fungsi untuk membaca rata-rata analog (Oversampling 20 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}

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
    
    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(180);

    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void loop() {
    unsigned long now = millis();
    
    // ── 1. Baca Sensor, Servo, & LED (Setiap 20ms) ───────────────────────────
    if (now - lastServoUpdate >= 20) {
        lastServoUpdate = now;
        
        int rawADC = readAverage(FLEX_A_PIN);

        // Gerakkan Servo Motor
        // Rangkaian Pembagi Tegangan: Vin=5V, R2=22kOhm, R1=Flex Sensor
        int lowLimit  = min(FLEX_A_MIN, FLEX_A_MAX);
        int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
        
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
        
        // Nyalakan Lampu LED
        if (rawADC >= THRESHOLD_GREEN) {
            setLed(false, false, true);   // Hijau
        } 
        else if (rawADC >= THRESHOLD_YELLOW) {
            setLed(false, true, false);   // Kuning
        } 
        else {
            setLed(true, false, false);   // Merah
        }
        
        // ── 2. Tampilkan Data di LCD 16x4 (Setiap 200ms) ──────────────────────
        if (now - lastLcdUpdate >= 200) {
            lastLcdUpdate = now;
            
            // Rangkaian Pembagi Tegangan: Vin=5V, R2=22kOhm, R1=Flex Sensor
            // Tegangan yang dibaca pada pin ADC ESP32 (range 0 - 3.3V):
            float voltA = (rawADC * 3.3) / 4095.0;
            
            char line0[17], line1[17], line2[17];
            snprintf(line0, sizeof(line0), "FA:%4d | %.2fV", rawADC, voltA);
            snprintf(line1, sizeof(line1), "Servo: %3d deg  ", angle);
            snprintf(line2, sizeof(line2), "LED: %-10s", 
                (rawADC >= THRESHOLD_GREEN) ? "HIJAU" : ((rawADC >= THRESHOLD_YELLOW) ? "KUNING" : "MERAH"));
            
            lcd.setCursor(0, 0);
            lcd.print(line0);
            
            lcd.setCursor(0, 1);
            lcd.print(line1);
            
            lcd.setCursor(0, 2);
            lcd.print(line2);
            
            lcd.setCursor(0, 3);
            lcd.print("Trainer : READY ");
            
            // Output Serial Debug
            Serial.printf("Flex A: %d | Volt A: %.2fV | Servo: %d deg\n", rawADC, voltA, angle);
        }
    }
}