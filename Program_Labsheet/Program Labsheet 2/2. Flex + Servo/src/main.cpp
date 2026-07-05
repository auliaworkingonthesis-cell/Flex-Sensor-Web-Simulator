#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo
 * ============================================================
 *  Deskripsi: Menggerakkan Servo Motor secara linear berdasarkan
 *             derajat lekukan Sensor Flex A secara non-blocking.
 */

#include <ESP32Servo.h>

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
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Konfigurasi Input Kontrol Servo
// Set ke true jika Servo dikendalikan Flex A, set ke false untuk Flex B
#define USE_FLEX_A_FOR_SERVO  true

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
#define FLEX_A_MIN  3054  // ADC lurus (0 derajat)
#define FLEX_A_MAX  2766  // ADC bengkok maksimal (180 derajat)
#define FLEX_B_MIN  3000
#define FLEX_B_MAX  2700
#define FLEX_B_MIN  3000
#define FLEX_B_MAX  2700

Servo myServo;
int lastAngle = -1;
unsigned long lastUpdate = 0;
const unsigned long interval = 20; // Update servo setiap 20ms

void setup() {
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    
    // Hubungkan servo
    myServo.attach(SERVO_PIN);
    myServo.write(180); // Default lurus
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = USE_FLEX_A_FOR_SERVO ? readAverage(FLEX_A_PIN) : readAverage(FLEX_B_PIN);
        int flexMin = USE_FLEX_A_FOR_SERVO ? FLEX_A_MIN : FLEX_B_MIN;
        int flexMax = USE_FLEX_A_FOR_SERVO ? FLEX_A_MAX : FLEX_B_MAX;
        
        // Urutkan batas constrain
        int lowLimit = min(flexMin, flexMax);
        int highLimit = max(flexMin, flexMax);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        
        // Mapping ke sudut servo 0 s.d 180 derajat
        int angle = map(constrainedADC, flexMin, flexMax, 0, 180);
        
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
    }
}
