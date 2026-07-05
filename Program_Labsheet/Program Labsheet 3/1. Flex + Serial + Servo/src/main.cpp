#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 3: Flex + Serial + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel/kabel menggunakan Web Serial API.
 *             Membaca Sensor Flex A, mengendalikan Servo fisik, dan mengirimkan
 *             data stream format JSON (DATA:{"flexA":..., "servo":...}) ke Serial Port.
 *             Semua proses dijalankan secara non-blocking menggunakan millis().
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN  35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

// Konfigurasi Input Kontrol Servo
// Set ke true jika Servo dikendalikan Flex A, set ke false untuk Flex B
#define USE_FLEX_A_FOR_SERVO  true

// Kalibrasi ADC default (Langsung edit di variabel ini)
int flexA_min = 3054;
int flexA_max = 2766;
int flexB_min = 3054;
int flexB_max = 2766;

Servo myServo;
int lastAngle = -1;
unsigned long lastSensorUpdate = 0;

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
    myServo.write(180);
}

void loop() {
    unsigned long now = millis();
    
    // ── 1. Baca Sensor, Servo, & Kirim JSON Serial (Setiap 20ms) ─────────────
    if (now - lastSensorUpdate >= 20) {
        lastSensorUpdate = now;
        
        int rawA = analogRead(FLEX_A_PIN);
        int rawB = analogRead(FLEX_B_PIN);
        
        // Gerakkan Servo Fisik
        int angle = USE_FLEX_A_FOR_SERVO ? 
                    mapClamped(rawA, flexA_min, flexA_max, 0, 180) :
                    mapClamped(rawB, flexB_min, flexB_max, 0, 180);
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
        
        // Kirim Stream JSON untuk Web Simulator
        int panPct  = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
        int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
        
        char json[128];
        snprintf(json, sizeof(json),
            "DATA:\"{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d,\"phrase\":\"\"}\"",
            rawA, rawB, panPct, (float)angle, gripPct);
        Serial.println(json);
    }
    
    // Helper flat JSON parser
    auto parseVal = [](String json, String key, int currentVal) -> int {
        int idx = json.indexOf(""" + key + "":");
        if (idx == -1) return currentVal;
        int start = idx + key.length() + 3;
        int end = json.indexOf(",", start);
        if (end == -1) end = json.indexOf("}", start);
        if (end == -1) return currentVal;
        return json.substring(start, end).toInt();
    };

    // ── 2. Terima Perintah Kalibrasi Baru dari Web ───────────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("SET:")) {
            String json = line.substring(4);
            flexA_min = parseVal(json, "minA", flexA_min);
            flexA_max = parseVal(json, "maxA", flexA_max);
            flexB_min = parseVal(json, "minB", flexB_min);
            flexB_max = parseVal(json, "maxB", flexB_max);
            
            Serial.println("System: Calibration updated via Serial!");
        }
    }
}
