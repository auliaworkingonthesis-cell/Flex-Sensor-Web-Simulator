/**
 * ============================================================
 *  Labsheet 2 - Percobaan D: Integrasi Servo, LED, & LCD 16x4
 * ============================================================
 *  Tujuan: Menjalankan kontrol servo motor dan indikator LED 
 *          sekaligus menampilkan data statusnya ke layar LCD 16x4.
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 10K
#define FLEX_A_MIN  1320  // ADC lurus (0 derajat)
#define FLEX_A_MAX  1198  // ADC bengkok maksimal (180 derajat)

// Batas threshold warna LED
#define THRESHOLD_GREEN   1260
#define THRESHOLD_YELLOW  1210

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 4);

int lastAngle = -1;
unsigned long lastLcdUpdate = 0;

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
    myServo.write(0);

    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // ── 1. Update Posisi Servo ───────────────────────────────────────────────
    int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
    int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
    int constrainedADC = constrain(rawADC, lowLimit, highLimit);
    int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
    
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
    }
    
    // ── 2. Update Indikator LED ──────────────────────────────────────────────
    const char* statusStr = "";
    if (rawADC >= THRESHOLD_GREEN) {
        setLed(false, false, true);   // Hijau
        statusStr = "Lurus  [Hijau]";
    } 
    else if (rawADC >= THRESHOLD_YELLOW) {
        setLed(false, true, false);   // Kuning
        statusStr = "Tekuk  [Kuning]";
    } 
    else {
        setLed(true, false, false);   // Merah
        statusStr = "Bengkok [Merah]";
    }
    
    // ── 3. Update LCD 16x4 (Setiap 200ms agar layar tidak berkedip) ───────────
    unsigned long now = millis();
    if (now - lastLcdUpdate >= 200) {
        lastLcdUpdate = now;
        
        // Baris 0: Nilai ADC Sensor Flex
        lcd.setCursor(0, 0);
        lcd.print("Flex ADC: ");
        lcd.print(rawADC);
        lcd.print("    ");
        
        // Baris 1: Sudut Servo
        lcd.setCursor(0, 1);
        lcd.print("Servo   : ");
        lcd.print(angle);
        lcd.print(" deg ");
        
        // Baris 2: Kondisi LED
        lcd.setCursor(0, 2);
        lcd.print("LED     : ");
        lcd.print(statusStr);
        lcd.print("       ");
        
        // Baris 3: Status Trainer
        lcd.setCursor(0, 3);
        lcd.print("Trainer : ONLINE");
    }
    
    delay(10);
}
