/**
 * ============================================================
 *  Labsheet 3 - Percobaan C: Integrasi Dua Sensor Flex
 * ============================================================
 *  Tujuan: Membaca 2 sensor flex secara bersamaan.
 *          Sensor A (Bracket)       → Kontrol Rotasi / Pan Lengan (0°=Kanan, 90°=Kiri)
 *          Sensor B (Project Board) → Kontrol Bukaan Gripper (0°=Tutup, 90°=Buka)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

#define FLEX_A_PIN       34  // Sensor Flex A (Rotasi/Pan)
#define FLEX_B_PIN       35  // Sensor Flex B (Bukaan Gripper/Grip)
#define SERVO_PIN        18  // Pin PWM Servo

// Wi-Fi & mDNS
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

// Kalibrasi ADC pembagi tegangan 10K (Lurus: 1320, 90 derajat: 1240)
// Di percobaan ini, rentang lekukan dibatasi dari 0° s.d 90° (ADC: 1320 s.d 1240)
#define FLEX_MIN  1320  // ADC Lurus (0°)
#define FLEX_MAX  1240  // ADC Melengkung 90° (90°)

WebServer server(80);
Servo myServo;

int lastAngle = -1;

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
    
    // Pemetaan data lekukan ke parameter web simulator:
    // Flex A: Lurus (0°) -> Kanan (pan = 100%), Bengkok 90° -> Kiri (pan = -100%)
    int panPct  = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 100, -100);
    // Flex B: Lurus (0°) -> Tertutup (grip = 0%), Bengkok 90° -> Terbuka (grip = 100%)
    int gripPct = mapClamped(rawB, FLEX_MIN, FLEX_MAX, 0, 100);
    // Servo mengikuti Flex A (pan sudut 0 s.d 90 derajat)
    int angle   = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 90);
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d}",
        rawA, rawB, panPct, (float)angle, gripPct);
        
    server.send(200, "application/json", json);
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
    
    // Koneksi Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    
    if (MDNS.begin(MDNS_NAME)) {
        Serial.println("mDNS: http://" + String(MDNS_NAME) + ".local");
    }
    
    server.on("/data", HTTP_GET, handleData);
    server.on("/data", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
    server.begin();
}

void loop() {
    server.handleClient();
    
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // Update servo fisik mengikuti Flex A (Rotasi)
    int angle = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 90);
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
    }
    
    // Kirim debug data ke Web Serial
    // Format: DATA:{"flexA":xxx,"flexB":xxx,"pan":xxx,"servo":xxx,"grip":xxx}
    int panPct  = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 100, -100);
    int gripPct = mapClamped(rawB, FLEX_MIN, FLEX_MAX, 0, 100);
    char json[128];
    snprintf(json, sizeof(json),
        "DATA:{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d}",
        rawA, rawB, panPct, (float)angle, gripPct);
    Serial.println(json);
    
    delay(20);
}
