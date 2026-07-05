#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 3: Flex + Wifi + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel menggunakan Wi-Fi Web Server.
 *             ESP32 terhubung ke jaringan Wi-Fi, menyediakan endpoint JSON (/data),
 *             dan mendukung sinkronisasi kalibrasi dinamis secara online via endpoint /config.
 *             Semua proses berjalan secara non-blocking menggunakan millis().
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
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
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}


#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN       35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor

// Konfigurasi Wi-Fi (Ganti dengan SSID dan Password Anda)
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

// Kalibrasi ADC default
int flexA_min = 3054;
int flexA_max = 2766;
int flexB_min = 3054;
int flexB_max = 2766;

WebServer server(80);
Servo myServo;
int lastAngle = -1;
unsigned long lastServoUpdate = 0;

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
    
    int rawA = readAverage(FLEX_A_PIN);
    int rawB = readAverage(FLEX_B_PIN);
    
    int panPct  = mapClamped(rawA, flexA_min, flexA_max, 100, -100);
    int gripPct = mapClamped(rawB, flexB_min, flexB_max, 100, 0);
    int angle   = USE_FLEX_A_FOR_SERVO ? 
                  mapClamped(rawA, flexA_min, flexA_max, 0, 180) :
                  mapClamped(rawB, flexB_min, flexB_max, 0, 180);
    
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
    
    Serial.println("System: Calibration updated via Web!");
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);

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
    server.on("/data", HTTP_GET, handleData);
    server.on("/data", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
    server.on("/config", HTTP_GET, handleSetConfig);
    server.begin();
}

void loop() {
    server.handleClient();
    
    unsigned long now = millis();
    
    // ── Update servo fisik mengikuti lekukan Sensor Flex A (Setiap 20ms) ──────
    if (now - lastServoUpdate >= 20) {
        lastServoUpdate = now;
        
        int rawA = readAverage(FLEX_A_PIN);
        int rawB = readAverage(FLEX_B_PIN);
        int angle = USE_FLEX_A_FOR_SERVO ? 
                    mapClamped(rawA, flexA_min, flexA_max, 0, 180) :
                    mapClamped(rawB, flexB_min, flexB_max, 0, 180);
        Serial.printf("Flex A: %d | Flex B: %d | Servo: %d deg\n", rawA, rawB, angle);
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
    }
}
