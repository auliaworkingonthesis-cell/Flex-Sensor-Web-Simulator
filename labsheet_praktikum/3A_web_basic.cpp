/**
 * ============================================================
 *  Labsheet 3 - Percobaan A: Komunikasi Data Web (WiFi & Serial)
 * ============================================================
 *  Tujuan: Mengaktifkan Wi-Fi, mDNS, dan Web Server sederhana
 *          pada ESP32 untuk membagikan data sensor flex dalam
 *          format JSON (bisa diakses via http://<ip_esp>/data).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define FLEX_A_PIN       34  // Pin ADC untuk Sensor Flex A
#define FLEX_B_PIN       35  // Pin ADC untuk Sensor Flex B

// Konfigurasi Wi-Fi (Sesuaikan SSID dan Password Anda)
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

WebServer server(80);

// Kalibrasi ADC pembagi tegangan 10K
#define FLEX_MIN  1320
#define FLEX_MAX  1198

// Helper map clamped
int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

// Fungsi untuk menyertakan Header CORS agar Browser Web mengizinkan koneksi
void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Handler Request data JSON
void handleData() {
    sendCorsHeaders();
    
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    
    // Konversi nilai ADC ke persentase (-100% s.d 100% untuk pan, 0 s.d 100 untuk grip)
    int panPct  = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 100, -100); // Reversed
    int gripPct = mapClamped(rawB, FLEX_MIN, FLEX_MAX, 100, 0);    // Reversed
    int angle   = mapClamped(rawA, FLEX_MIN, FLEX_MAX, 0, 180);
    
    // Format payload JSON
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
    
    // Koneksi Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
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
    
    Serial.println("Web Server siap dijalankan.");
}

void loop() {
    // Jalankan service web server client handler
    server.handleClient();
    
    // Kirim data ke Serial Monitor untuk komunikasi serial
    int rawA = analogRead(FLEX_A_PIN);
    int rawB = analogRead(FLEX_B_PIN);
    Serial.print("FlexA:"); Serial.print(rawA);
    Serial.print(" FlexB:"); Serial.println(rawB);
    
    delay(50); // Loop berjalan setiap 50ms
}
