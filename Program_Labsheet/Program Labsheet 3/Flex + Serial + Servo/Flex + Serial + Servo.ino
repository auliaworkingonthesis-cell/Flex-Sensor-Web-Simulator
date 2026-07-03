/**
 * ============================================================
 *  Program Labsheet 3: Flex + Serial + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel/kabel menggunakan Web Serial API.
 *             Membaca Sensor Flex A, mengendalikan Servo fisik, dan mengirimkan
 *             data stream format JSON (DATA:{"flexA":..., "servo":...}) ke Serial Port.
 *             Dilengkapi fitur auto-sinkronisasi kalibrasi dinamis dari web ke flash ESP32.
 */

#include <ESP32Servo.h>
#include <Preferences.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN  35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor

Preferences preferences;

// Kalibrasi ADC default
int flexA_min = 3040;
int flexA_max = 2800;
int flexB_min = 3040;
int flexB_max = 2800;

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
    
    // Load kalibrasi tersimpan
    preferences.begin("calib", false);
    flexA_min = preferences.getInt("minA", 1320);
    flexA_max = preferences.getInt("maxA", 1198);
    flexB_min = preferences.getInt("minB", 1320);
    flexB_max = preferences.getInt("maxB", 1198);
    preferences.end();

    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);
}

void loop() {
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // ── 1. Gerakkan Servo Fisik ──────────────────────────────────────────────
    int angle = mapClamped(rawA, flexA_min, flexA_max, 0, 180);
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
    }
    
    // ── 2. Kirim Stream JSON untuk Web Simulator (Setiap 20ms) ────────────────
    unsigned long now = millis();
    if (now - lastSerialUpdate >= 20) {
        lastSerialUpdate = now;
        
        int panPct  = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
        int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
        
        char json[128];
        snprintf(json, sizeof(json),
            "DATA:{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d,\"phrase\":\"\"}",
            rawA, rawB, panPct, (float)angle, gripPct);
        Serial.println(json);
    }
    
    // Helper flat JSON parser
    auto parseVal = [](String json, String key, int currentVal) -> int {
        int idx = json.indexOf("\"" + key + "\":");
        if (idx == -1) return currentVal;
        int start = idx + key.length() + 3;
        int end = json.indexOf(",", start);
        if (end == -1) end = json.indexOf("}", start);
        if (end == -1) return currentVal;
        return json.substring(start, end).toInt();
    };

    // ── 3. Terima Perintah Kalibrasi Baru dari Web ───────────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("SET:")) {
            String json = line.substring(4);
            flexA_min = parseVal(json, "minA", flexA_min);
            flexA_max = parseVal(json, "maxA", flexA_max);
            flexB_min = parseVal(json, "minB", flexB_min);
            flexB_max = parseVal(json, "maxB", flexB_max);
            
            preferences.begin("calib", false);
            preferences.putInt("minA", flexA_min);
            preferences.putInt("maxA", flexA_max);
            preferences.putInt("minB", flexB_min);
            preferences.putInt("maxB", flexB_max);
            preferences.end();
            Serial.println("System: Calibration updated via Serial!");
        }
    }
    
    delay(5);
}
