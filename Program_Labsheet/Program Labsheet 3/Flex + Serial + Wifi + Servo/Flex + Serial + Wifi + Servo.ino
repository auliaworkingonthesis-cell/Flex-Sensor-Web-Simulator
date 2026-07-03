/**
 * ============================================================
 *  Program Labsheet 3: Flex + Serial + Wifi + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data ganda/dual-mode secara bersamaan.
 *             Membaca Sensor Flex A, mengendalikan Servo fisik, dan
 *             mengirimkan data ke Web Simulator melalui dua jalur sekaligus:
 *             1. Jalur Kabel: Web Serial API (DATA JSON stream via USB)
 *             2. Jalur Nirkabel: Wi-Fi Web Server (HTTP GET /data)
 *             Serta mendukung sinkronisasi kalibrasi dinamis secara online via Preferences.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <Preferences.h>

#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN       35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor

// Konfigurasi Wi-Fi (Ganti dengan SSID dan Password Anda)
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

Preferences preferences;

// Kalibrasi ADC default
int flexA_min = 3040;
int flexA_max = 2800;
int flexB_min = 3040;
int flexB_max = 2800;

WebServer server(80);
Servo myServo;
int lastAngle = -1;
unsigned long lastSerialUpdate = 0;

int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleData() {
    sendCorsHeaders();
    
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    int panPct  = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
    int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
    int angle   = mapClamped(rawA, flexA_min, flexA_max, 0, 180);
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d}",
        rawA, rawB, panPct, (float)angle, gripPct);
        
    server.send(200, "application/json", json);
}

void handleSetConfig() {
    sendCorsHeaders();
    int minA = server.hasArg("minA") ? server.arg("minA").toInt() : flexA_min;
    int maxA = server.hasArg("maxA") ? server.arg("maxA").toInt() : flexA_max;
    int minB = server.hasArg("minB") ? server.arg("minB").toInt() : flexB_min;
    int maxB = server.hasArg("maxB") ? server.arg("maxB").toInt() : flexB_max;
    
    flexA_min = minA;
    flexA_max = maxA;
    flexB_min = minB;
    flexB_max = maxB;
    
    preferences.begin("calib", false);
    preferences.putInt("minA", flexA_min);
    preferences.putInt("maxA", flexA_max);
    preferences.putInt("minB", flexB_min);
    preferences.putInt("maxB", flexB_max);
    preferences.end();
    
    Serial.println("System: Calibration updated via Web!");
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    
    // Load kalibrasi
    preferences.begin("calib", false);
    flexA_min = preferences.getInt("minA", 1320);
    flexA_max = preferences.getInt("maxA", 1198);
    flexB_min = preferences.getInt("minB", 1320);
    flexB_max = preferences.getInt("maxB", 1198);
    preferences.end();

    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);
    
    // Hubungkan ke Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Inisialisasi mDNS (akses via http://flex-kelompok1.local)
    if (MDNS.begin(MDNS_NAME)) {
        Serial.println("mDNS responder started: http://" + String(MDNS_NAME) + ".local");
    }
    
    // Routing Web Server
    server.on("/data", HTTP_GET, handleData);
    server.on("/data", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
    server.on("/config", HTTP_GET, handleSetConfig);
    server.begin();
}

void loop() {
    server.handleClient();
    
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // ── 1. Update Posisi Servo Fisik ─────────────────────────────────────────
    int angle = mapClamped(rawA, flexA_min, flexA_max, 0, 180);
    if (angle != lastAngle) {
        myServo.write(180 - angle); // Inversi hardware servo fisik
        lastAngle = angle;
    }
    
    // ── 2. Kirim Stream JSON untuk Web Serial USB (Setiap 20ms) ───────────────
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

    // ── 3. Terima Perintah Kalibrasi Baru dari Web Serial ────────────────────
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
