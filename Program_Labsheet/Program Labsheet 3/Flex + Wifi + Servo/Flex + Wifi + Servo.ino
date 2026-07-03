/**
 * ============================================================
 *  Program Labsheet 3: Flex + Wifi + Servo
 * ============================================================
 *  Deskripsi: Komunikasi data nirkabel menggunakan Wi-Fi Web Server.
 *             ESP32 terhubung ke jaringan Wi-Fi, mengaktifkan mDNS, dan
 *             menyediakan endpoint JSON (/data) yang diakses oleh Web Simulator
 *             sembari mengendalikan Servo fisik secara sinkron.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN       35  // Pin ADC untuk Sensor Flex B
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor

// Konfigurasi Wi-Fi (Ganti dengan SSID dan Password Anda)
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

// Kalibrasi ADC pembagi tegangan 10K
#define FLEX_MIN  1320
#define FLEX_MAX  1198

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
    
    int panPct  = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 100, -100);
    int gripPct = mapClamped(rawB, FLEX_MIN, FLEX_MAX, 100, 0);
    int angle   = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 180);
    
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
    server.begin();
}

void loop() {
    server.handleClient();
    
    int rawA = analogRead(FLEX_A_PIN);
    
    // Update servo fisik mengikuti lekukan Sensor Flex A
    int angle = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 180);
    if (angle != lastAngle) {
        myServo.write(angle);
        lastAngle = angle;
    }
    
    delay(20);
}
