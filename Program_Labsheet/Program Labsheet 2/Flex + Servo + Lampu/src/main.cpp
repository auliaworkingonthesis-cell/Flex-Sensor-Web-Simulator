#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo + Lampu
 * ============================================================
 *  Deskripsi: Integrasi pergerakan Servo Motor dan indikator 
 *             3 buah lampu LED (Merah, Kuning, Hijau) berdasarkan 
 *             lekukan Sensor Flex A.
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  3054
#define FLEX_A_MAX  2766

// Threshold LED
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2795

Servo myServo;
int lastAngle = -1;

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
    myServo.write(0);
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // â”€â”€ 1. Gerakkan Servo Motor â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
    int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
    int constrainedADC = constrain(rawADC, lowLimit, highLimit);
    int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
    
    if (angle != lastAngle) {
        myServo.write(180 - angle); // Inversi hardware servo fisik
        lastAngle = angle;
    }
    
    // â”€â”€ 2. Nyalakan Lampu LED Sesuai Kondisi â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (rawADC >= THRESHOLD_GREEN) {
        setLed(false, false, true);   // Hijau (Lurus)
    } 
    else if (rawADC >= THRESHOLD_YELLOW) {
        setLed(false, true, false);   // Kuning (Bengkok ~90Â°)
    } 
    else {
        setLed(true, false, false);   // Merah (Bengkok maksimal)
    }
    
    delay(20);
}

