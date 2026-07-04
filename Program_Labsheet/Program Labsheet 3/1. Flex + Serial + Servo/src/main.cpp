#include <Arduino.h>
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


#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN  35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN   18  // Pin PWM untuk Servo Motor



// Kalibrasi ADC default
int flexA_min = 3054;
int flexA_max = 2766;
int flexB_min = 3054;
int flexB_max = 2766;

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
    

    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);
}

void loop() {
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // â”€â”€ 1. Gerakkan Servo Fisik â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    int angle = mapClamped(rawA, flexA_min, flexA_max, 0, 180);
    if (angle != lastAngle) {
        myServo.write(180 - angle); // Inversi hardware servo fisik
        lastAngle = angle;
    }
    
    // â”€â”€ 2. Kirim Stream JSON untuk Web Simulator (Setiap 20ms) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

    // â”€â”€ 3. Terima Perintah Kalibrasi Baru dari Web â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
    
    delay(5);
}
