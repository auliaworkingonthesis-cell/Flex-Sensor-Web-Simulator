/**
 * ============================================================
 *  Program Labsheet 3: Flex + Serial + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel/kabel menggunakan Web Serial API.
 *             Membaca Sensor Flex A, mengendalikan Servo fisik, dan mengirimkan
 *             data stream format JSON (DATA:{"flexA":..., "servo":...}) ke Serial Port
 *             agar terintegrasi dengan Web Simulator.
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN  35  // Pin ADC untuk Sensor Flex B (disediakan untuk Web)
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Kalibrasi ADC pembagi tegangan 10K
#define FLEX_MIN  1320  // ADC lurus
#define FLEX_MAX  1198  // ADC bengkok maks

Servo myServo;
int lastAngle = -1;
unsigned long lastSerialUpdate = 0;

int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    
    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);
}

void loop() {
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // ── 1. Gerakkan Servo Fisik ──────────────────────────────────────────────
    int angle = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 180);
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
    }
    
    // ── 2. Kirim Stream JSON untuk Web Simulator (Setiap 20ms) ────────────────
    unsigned long now = millis();
    if (now - lastSerialUpdate >= 20) {
        lastSerialUpdate = now;
        
        int panPct  = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 100, -100);  // Arah Pan
        int gripPct = mapClamped(rawB, FLEX_MIN, FLEX_MAX, 100, 0);     // Arah Grip
        
        // Format DATA JSON untuk Web Serial API
        char json[128];
        snprintf(json, sizeof(json),
            "DATA:{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d,\"phrase\":\"\"}",
            rawA, rawB, panPct, (float)angle, gripPct);
        Serial.println(json);
    }
    
    delay(5);  // Loop responsif
}
