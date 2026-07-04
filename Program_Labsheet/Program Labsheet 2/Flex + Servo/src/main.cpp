#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo
 * ============================================================
 *  Deskripsi: Menggerakkan Servo Motor secara linear berdasarkan
 *             derajat lekukan Sensor Flex A (0 - 180 derajat).
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  3054  // ADC lurus (0 derajat)
#define FLEX_A_MAX  2766  // ADC bengkok maksimal (180 derajat)

Servo myServo;
int lastAngle = -1;

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Hubungkan servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // Urutkan batas constrain
    int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
    int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
    int constrainedADC = constrain(rawADC, lowLimit, highLimit);
    
    // Mapping ke sudut servo 0 s.d 180 derajat
    int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
    
    if (angle != lastAngle) {
        myServo.write(180 - angle); // Inversi hardware servo fisik
        lastAngle = angle;
    }
    
    delay(20);
}

