#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo + Lampu + LCD
 * ============================================================
 *  Deskripsi: Integrasi lengkap modul Servo Motor, indikator 3 LED, 
 *             dan penampilan data statusnya ke layar LCD 16x4 secara non-blocking.
 */

#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  2930
#define FLEX_A_MAX  2630

// Threshold LED
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2795

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 4);

int lastAngle = -1;
unsigned long lastServoUpdate = 0;
unsigned long lastLcdUpdate = 0;

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
        
        int rawADC = analogRead(FLEX_A_PIN);
        
        // Gerakkan Servo Motor
        int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
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
            
            // Baris 0: Nilai ADC
            lcd.setCursor(0, 0);
            lcd.print("ADC Val : ");
            lcd.print(rawADC);
            lcd.print("    ");
            
            // Baris 1: Sudut Servo
            lcd.setCursor(0, 1);
            lcd.print("Servo   : ");
            lcd.print(angle);
            lcd.print(" deg ");
            
            // Baris 2: Status Trainer
            lcd.setCursor(0, 2);
            lcd.print("Trainer : READY ");
            
            // Baris 3: Kosong
            lcd.setCursor(0, 3);
            lcd.print("                ");
        }
    }
}
