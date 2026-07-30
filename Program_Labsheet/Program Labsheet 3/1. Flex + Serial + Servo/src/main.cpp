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
// #define USE_FLEX_B           // Non-aktif: hapus komentar jika Flex B disambungkan

// ── PIN HARDWARE ─────────────────────────────────────────────────────────────
#define FLEX_A_PIN  34          // Pin ADC Sensor Flex A
#define FLEX_B_PIN  35          // Pin ADC Sensor Flex B
#define SERVO_PIN   18          // Pin PWM Servo Motor

// ── KALIBRASI ADC ────────────────────────────────────────────────────────────
// Nilai default ini akan otomatis diperbarui dari Web Simulator saat connect.
// Tidak perlu ubah & upload ulang — cukup atur di web lalu klik Connect.
#ifdef USE_FLEX_A
    int flexA_min = 3040;       // ADC saat Flex A lurus   (default web: 3040)
    int flexA_max = 2800;       // ADC saat Flex A bengkok (default web: 2800)
#endif

#ifdef USE_FLEX_B
    int flexB_min = 3040;       // ADC saat Flex B lurus   (default web: 3040)
    int flexB_max = 2800;       // ADC saat Flex B bengkok (default web: 2800)
#endif
// ─────────────────────────────────────────────────────────────────────────────

#include <ESP32Servo.h>

Servo myServo;
int lastAngle  = -1;
int webServoAngle = 90; // Sudut dari web (diupdate via SERVO: command)
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

        // Servo fisik: ikut sudut dari web (sudah hitung kalibrasi)
        // Inversi karena servo terpasang terbalik secara fisik
        int physAngle = 180 - webServoAngle;
        if (physAngle != lastAngle) {
            myServo.write(physAngle);
            lastAngle = physAngle;
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
            rawA, rawB, panPct, (float)webServoAngle, gripPct);
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

    // ── 2. Terima Perintah dari Web ──────────────────────────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        // Terima sudut servo langsung dari web
        if (line.startsWith("SERVO:")) {
            webServoAngle = constrain(line.substring(6).toInt(), 0, 180);
        }

        // Terima update kalibrasi dari web
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
