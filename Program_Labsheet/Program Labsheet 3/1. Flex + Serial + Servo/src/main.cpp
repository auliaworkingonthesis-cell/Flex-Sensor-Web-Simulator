#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 3: Flex + Serial + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel/kabel menggunakan Web Serial API.
 *             Membaca Sensor Flex, mengendalikan Servo fisik, dan mengirimkan
 *             data stream format JSON ke Serial Port.
 *             Semua proses dijalankan secara non-blocking menggunakan millis().
 * 
 *  ============================================================
 *  KONFIGURASI SENSOR (Edit bagian ini sesuai kebutuhan)
 *  ============================================================
 *  Untuk menggunakan HANYA Flex A:
 *    - Aktifkan  : #define USE_FLEX_A   (sudah aktif di bawah)
 *    - Non-aktifkan: // #define USE_FLEX_B
 *
 *  Untuk menggunakan Flex A + Flex B:
 *    - Aktifkan kedua-duanya
 *    - Servo tetap dikendalikan Flex A
 *    - Flex B akan dikirim sebagai grip/data tambahan
 */

// ── PILIH SENSOR YANG DIGUNAKAN ──────────────────────────────────────────────
#define USE_FLEX_A              // Aktifkan Sensor Flex A (Pan + Servo)
// #define USE_FLEX_B           // Non-aktif: aktifkan jika Flex B disambungkan

// ── PIN HARDWARE ─────────────────────────────────────────────────────────────
#define FLEX_A_PIN  34          // Pin ADC Sensor Flex A
#define FLEX_B_PIN  35          // Pin ADC Sensor Flex B
#define SERVO_PIN   18          // Pin PWM Servo Motor

// ── KALIBRASI ADC (Sesuaikan dengan hasil pengukuran sensor) ─────────────────
#ifdef USE_FLEX_A
    int flexA_min = 3054;       // ADC saat Flex A lurus
    int flexA_max = 2766;       // ADC saat Flex A bengkok maksimal
#endif

#ifdef USE_FLEX_B
    int flexB_min = 3054;       // ADC saat Flex B lurus
    int flexB_max = 2766;       // ADC saat Flex B bengkok maksimal
#endif
// ─────────────────────────────────────────────────────────────────────────────

#include <ESP32Servo.h>

Servo myServo;
int lastAngle = -1;
unsigned long lastSensorUpdate = 0;

// Fungsi untuk membaca rata-rata analog (Oversampling 20 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}

int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low  = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
#ifdef USE_FLEX_A
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
#endif
#ifdef USE_FLEX_B
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
#endif

    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(180);
}

void loop() {
    unsigned long now = millis();
    
    // ── 1. Baca Sensor, Servo, & Kirim JSON Serial (Setiap 20ms) ─────────────
    if (now - lastSensorUpdate >= 20) {
        lastSensorUpdate = now;
        
#ifdef USE_FLEX_A
        int rawA = readAverage(FLEX_A_PIN);
#else
        int rawA = 0;
#endif

#ifdef USE_FLEX_B
        int rawB = readAverage(FLEX_B_PIN);
#else
        int rawB = 0;
#endif

        // Gerakkan Servo Fisik (selalu ikut Flex A jika aktif)
#ifdef USE_FLEX_A
        int angle = mapClamped(rawA, flexA_min, flexA_max, 0, 180);
#elif defined(USE_FLEX_B)
        int angle = mapClamped(rawB, flexB_min, flexB_max, 0, 180);
#else
        int angle = 90;
#endif

        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
        
        // Kirim Stream JSON untuk Web Simulator
#ifdef USE_FLEX_A
        int panPct = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
#else
        int panPct = 0;
#endif

#ifdef USE_FLEX_B
        int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
#else
        int gripPct = 0;
#endif
        
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

    // ── 2. Terima Perintah Kalibrasi Baru dari Web ───────────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("SET:")) {
            String jsonStr = line.substring(4);
#ifdef USE_FLEX_A
            flexA_min = parseVal(jsonStr, "minA", flexA_min);
            flexA_max = parseVal(jsonStr, "maxA", flexA_max);
#endif
#ifdef USE_FLEX_B
            flexB_min = parseVal(jsonStr, "minB", flexB_min);
            flexB_max = parseVal(jsonStr, "maxB", flexB_max);
#endif
            Serial.println("System: Calibration updated via Serial!");
        }
    }
}
