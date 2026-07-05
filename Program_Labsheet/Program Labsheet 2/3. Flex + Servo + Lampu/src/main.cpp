#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo + Lampu
 * ============================================================
 *  Deskripsi: Integrasi pergerakan Servo Motor dan indikator 
 *             3 buah lampu LED berdasarkan lekukan Sensor Flex A
 *             secara non-blocking.
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN      35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor

// Konfigurasi Input Kontrol Servo
// Set ke true jika Servo dikendalikan Flex A, set ke false untuk Flex B
#define USE_FLEX_A_FOR_SERVO  true
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  3054
#define FLEX_A_MAX  2766
#define FLEX_B_MIN  3000
#define FLEX_B_MAX  2700

// Threshold LED
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2795

Servo myServo;
int lastAngle = -1;
unsigned long lastUpdate = 0;
const unsigned long interval = 20; // Check interval 20ms

void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    
    // Konfigurasi pin LED sebagai OUTPUT
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    setLed(false, false, false);
    
    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(180);
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = USE_FLEX_A_FOR_SERVO ? analogRead(FLEX_A_PIN) : analogRead(FLEX_B_PIN);
        int flexMin = USE_FLEX_A_FOR_SERVO ? FLEX_A_MIN : FLEX_B_MIN;
        int flexMax = USE_FLEX_A_FOR_SERVO ? FLEX_A_MAX : FLEX_B_MAX;
        
        // ── 1. Gerakkan Servo Motor ──────────────────────────────────────────────
        int lowLimit = min(flexMin, flexMax);
        int highLimit = max(flexMin, flexMax);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        int angle = map(constrainedADC, flexMin, flexMax, 0, 180);
        
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
        
        // ── 2. Nyalakan Lampu LED Sesuai Kondisi ──────────────────────────────────
        if (rawADC >= THRESHOLD_GREEN) {
            setLed(false, false, true);   // Hijau (Lurus)
        } 
        else if (rawADC >= THRESHOLD_YELLOW) {
            setLed(false, true, false);   // Kuning (Bengkok ~90°)
        } 
        else {
            setLed(true, false, false);   // Merah (Bengkok maksimal)
        }
    }
}
