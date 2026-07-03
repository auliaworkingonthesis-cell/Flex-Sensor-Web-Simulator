/**
 * ============================================================
 *  Labsheet 2 - Percobaan C: Integrasi Servo Motor & 3 LED
 * ============================================================
 *  Tujuan: Mengintegrasikan pergerakan servo motor dan indikator LED
 *          berdasarkan pembacaan sensor flex yang sama.
 */

#include <Arduino.h>
#include <ESP32Servo.h>

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
int lastAngle = -1;

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
    if (rawADC >= THRESHOLD_GREEN) {
        setLed(false, false, true);   // Hijau
    } 
    else if (rawADC >= THRESHOLD_YELLOW) {
        setLed(false, true, false);   // Kuning
    } 
    else {
        setLed(true, false, false);   // Merah
    }
    
    delay(20); // Delay kecil untuk stabilitas loop
}
