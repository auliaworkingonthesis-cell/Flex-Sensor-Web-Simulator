#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 3: Flex + Wifi + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel menggunakan Wi-Fi Web Server.
 *             ESP32 terhubung ke jaringan Wi-Fi, menyediakan endpoint JSON (/data),
 *             dan mendukung sinkronisasi kalibrasi dinamis secara online via endpoint /config.
 *             Semua proses berjalan secara non-blocking menggunakan millis().
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
 *    - Flex B akan dikirim sebagai grip/data tambahan ke web
 */

// ── PILIH SENSOR YANG DIGUNAKAN ──────────────────────────────────────────────
#define USE_FLEX_A              // Aktifkan Sensor Flex A (Pan + Servo)
#define USE_FLEX_B           // Non-aktif: aktifkan jika Flex B disambungkan

// ── PIN HARDWARE ─────────────────────────────────────────────────────────────
#define FLEX_A_PIN       34     // Pin ADC Sensor Flex A
#define FLEX_B_PIN       35     // Pin ADC Sensor Flex B
#define SERVO_PIN        18     // Pin PWM Servo Motor

// ── KONFIGURASI WI-FI ────────────────────────────────────────────────────────
#define WIFI_SSID        "FLEXIBLE"
#define WIFI_PASSWORD    "FLEX12345"
#define MDNS_NAME        "flex-kelompok1"   // Akses via http://flex-kelompok1.local

// ── KALIBRASI ADC ────────────────────────────────────────────────────────────
// Nilai default ini akan otomatis diperbarui dari Web Simulator saat connect.
// Tidak perlu ubah & upload ulang — cukup atur di web lalu klik Connect WiFi.
#ifdef USE_FLEX_A
    int flexA_min = 3040;       // ADC saat Flex A lurus   (default web: 3040)
    int flexA_max = 2800;       // ADC saat Flex A bengkok (default web: 2800)
#endif

#ifdef USE_FLEX_B
    int flexB_min = 3040;       // ADC saat Flex B lurus   (default web: 3040)
    int flexB_max = 2800;       // ADC saat Flex B bengkok (default web: 2800)
#endif
// ─────────────────────────────────────────────────────────────────────────────

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <WebSocketsServer.h>

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Servo myServo;
int lastAngle = -1;
int webServoAngle = 90; // Sudut dari web (diupdate via WebSocket SERVO: command)
unsigned long lastServoUpdate = 0;

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

void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleData() {
    sendCorsHeaders();
    
#ifdef USE_FLEX_A
    int rawA = readAverage(FLEX_A_PIN);
    int panPct = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
#else
    int rawA = 0, panPct = 0;
#endif

#ifdef USE_FLEX_B
    int rawB    = readAverage(FLEX_B_PIN);
    int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
#else
    int rawB = 0, gripPct = 0;
#endif
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d}",
        rawA, rawB, panPct, (float)webServoAngle, gripPct);
        
    server.send(200, "application/json", json);
}

void handleSetConfig() {
    sendCorsHeaders();
#ifdef USE_FLEX_A
    if (server.hasArg("minA")) flexA_min = server.arg("minA").toInt();
    if (server.hasArg("maxA")) flexA_max = server.arg("maxA").toInt();
#endif
#ifdef USE_FLEX_B
    if (server.hasArg("minB")) flexB_min = server.arg("minB").toInt();
    if (server.hasArg("maxB")) flexB_max = server.arg("maxB").toInt();
#endif
    
    Serial.println("System: Calibration updated via Web!");
    server.send(200, "text/plain", "OK");
}

// Helper flat JSON parser
int parseVal(String json, String key, int currentVal) {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return currentVal;
    int start = idx + key.length() + 3;
    int end = json.indexOf(",", start);
    if (end == -1) end = json.indexOf("}", start);
    if (end == -1) return currentVal;
    return json.substring(start, end).toInt();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        msg.trim();
        if (msg.startsWith("SERVO:")) {
            webServoAngle = constrain(msg.substring(6).toInt(), 0, 180);
        } else if (msg.startsWith("SET:")) {
            String jsonStr = msg.substring(4);
#ifdef USE_FLEX_A
            flexA_min = parseVal(jsonStr, "minA", flexA_min);
            flexA_max = parseVal(jsonStr, "maxA", flexA_max);
#endif
#ifdef USE_FLEX_B
            flexB_min = parseVal(jsonStr, "minB", flexB_min);
            flexB_max = parseVal(jsonStr, "maxB", flexB_max);
#endif
            Serial.println("System: Calibration updated via WebSocket!");
        }
    }
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
    server.on("/data",   HTTP_GET,     handleData);
    server.on("/data",   HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
    server.on("/config", HTTP_GET,     handleSetConfig);
    server.begin();

    // Inisialisasi WebSocket Server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("WebSocket Server started on port 81");
}

void loop() {
    server.handleClient();
    webSocket.loop();
    
    unsigned long now = millis();
    
    // ── Update sensor & servo fisik & kirim data (Setiap 20ms) ──────────────
    if (now - lastServoUpdate >= 20) {
        lastServoUpdate = now;
        
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
        
        // Kirim Stream JSON untuk Web Simulator via WebSocket
        char json[128];
        snprintf(json, sizeof(json),
            "{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d}",
            rawA, rawB, panPct, (float)webServoAngle, gripPct);
        webSocket.broadcastTXT(json);
    }
}

