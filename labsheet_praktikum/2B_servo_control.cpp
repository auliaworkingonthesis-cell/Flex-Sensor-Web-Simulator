/**
 * ============================================================
 *  Labsheet 2 - Percobaan B: Kontrol Posisi Servo Motor
 * ============================================================
 *  Tujuan: Memetakan (mapping) lekukan sensor flex secara linear
 *          untuk mengontrol sudut pergerakan servo motor (0° - 180°).
 */

#include <Arduino.h>
#include <ESP32Servo.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Kalibrasi ADC pembagi tegangan 10K
#define FLEX_A_MIN  1320  // ADC saat lurus (0 derajat)
#define FLEX_A_MAX  1198  // ADC saat bengkok maksimal (180 derajat)

Servo myServo;
int lastAngle = -1;

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Hubungkan objek servo ke pin fisiknya
    myServo.attach(SERVO_PIN);
    myServo.write(0);  // Set posisi awal servo di 0 derajat
}

void loop() {
    int rawADC = analogRead(FLEX_A_PIN);
    
    // Sortir limit terkecil dan terbesar agar fungsi constrain tidak error
    int lowLimit = min(FLEX_A_MIN, FLEX_A_MAX);
    int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
    
    // Batasi pembacaan agar selalu berada dalam range kalibrasi
    int constrainedADC = constrain(rawADC, lowLimit, highLimit);
    
    // Map nilai ADC (1320 s.d 1198) ke sudut servo (0 s.d 180 derajat)
    int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
    
    // Tulis ke servo jika ada perubahan sudut
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
        
        Serial.print("Flex ADC: "); Serial.print(rawADC);
        Serial.print(" | Sudut Servo: "); Serial.print(angle);
        Serial.println(" deg");
    }
    
    delay(20);  // Update cepat untuk responsivitas aktuator
}
