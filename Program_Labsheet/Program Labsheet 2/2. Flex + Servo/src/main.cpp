#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo
 * ============================================================
 *  Deskripsi: Menggerakkan Servo Motor secara linear berdasarkan
 *             derajat lekukan Sensor Flex A secara non-blocking.
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
// Nilai lurus (tidak bengkok) → ADC tinggi
// Nilai bengkok maksimal     → ADC rendah
#define FLEX_A_MIN  3000  // ADC saat lurus
#define FLEX_A_MAX  2780  // ADC saat bengkok maksimal

Servo myServo;
int lastAngle = -1;
unsigned long lastUpdate = 0;
const unsigned long interval = 20; // Update servo setiap 20ms

// Fungsi untuk membaca rata-rata analog (Oversampling 20 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}

void setup() {
    Serial.begin(115200);
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Hubungkan servo
    myServo.attach(SERVO_PIN);
    myServo.write(180); // Default lurus
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = readAverage(FLEX_A_PIN);

        // Rangkaian Pembagi Tegangan: Vin=5V, R2=22kOhm, R1=Flex Sensor
        int lowLimit  = min(FLEX_A_MIN, FLEX_A_MAX);
        int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        
        // Mapping ke sudut servo 0 s.d 180 derajat
        int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
        
        Serial.printf("Flex: %d | Servo: %d deg\n", rawADC, angle);
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
    }
}
