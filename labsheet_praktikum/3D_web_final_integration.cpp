/**
 * ============================================================
 *  Labsheet 3 - Percobaan D: Integrasi Penuh Sistem Trainer Kit
 * ============================================================
 *  Tujuan: Menjalankan seluruh modul Trainer Kit secara simultan:
 *          - Pembacaan 2x Sensor Flex (Moving Average Filter)
 *          - Output Sudut Servo (Piecewise-Linear 0° -> 90° -> 180°)
 *          - Output LCD 16x4 Real-time Data
 *          - Output Indikator 3 LED Terpisah (Hijau, Kuning, Merah)
 *          - Komunikasi Dual-Mode: Wi-Fi Web Server (HTTP JSON) &
 *            Web Serial API (DATA JSON Stream).
 *          - Fitur sinkronisasi kalibrasi realtime dari web disimpan ke flash ESP32.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

// ── Wi-Fi Credentials & mDNS ─────────────────────────────────
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"

// ── Pin Mapping ───────────────────────────────────────────────
#define FLEX_A_PIN       34    // Sensor Flex A (Rotasi & Lengan)
#define FLEX_B_PIN       35    // Sensor Flex B (Bukaan Gripper)
#define SERVO_PIN        18    // Servo Motor
#define LED_RED_PIN      25    // LED Merah  (Tekuk Maks)
#define LED_YELLOW_PIN   26    // LED Kuning (Tekuk ~90°)
#define LED_GREEN_PIN    27    // LED Hijau  (Lurus)

// ── Kalibrasi ADC (Preferences) ──────────────────────────────
Preferences preferences;

int flexA_min = 3040;  // ADC lurus (0 derajat)
int flexA_max = 2800;  // ADC bengkok maksimal (180 derajat)
int flexB_min = 3040;
int flexB_max = 2800;

int rgb_green_threshold  = 2920;
int rgb_yellow_threshold = 2824;

// ── Pilih Sensor Untuk Mengontrol LED ─────────────────────────
#define LED_SOURCE_FLEX_A  0
#define LED_SOURCE_FLEX_B  1
#define LED_FOLLOWS        LED_SOURCE_FLEX_A

// ── Sampling & Intervals ──────────────────────────────────────
#define SAMPLE_COUNT            20   // Jumlah sampel moving average
#define SENSOR_INTERVAL_MS      5    // Interval baca sensor
#define SERIAL_INTERVAL_MS      50   // Interval serial monitor biasa
#define SERIAL_JSON_INTERVAL_MS 20   // Interval data JSON untuk web
#define LCD_INTERVAL_MS         100  // Interval update LCD

// ── Objects ───────────────────────────────────────────────────
WebServer       server(80);
Servo           myServo;
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ── Ring Buffer Moving Average ────────────────────────────────
int   readingsA[SAMPLE_COUNT] = {};
int   readingsB[SAMPLE_COUNT] = {};
int   readIndex    = 0;
long  totalA       = 0;
long  totalB       = 0;
int   flexA        = 0;
int   flexB        = 0;

// ── Timestamps (Non-blocking) ─────────────────────────────────
unsigned long lastSensorRead  = 0;
unsigned long lastSerial      = 0;
unsigned long lastSerialJson  = 0;
unsigned long lastLcd         = 0;
unsigned long lastWifiRetry   = 0;

// ── Flags ─────────────────────────────────────────────────────
bool serverStarted = false;
bool mdnsStarted   = false;

// ──────────────────────────────────────────────────────────────
//  HELPERS
// ──────────────────────────────────────────────────────────────
int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

void initRingBuffer() {
    int a = analogRead(FLEX_A_PIN);
    int b = analogRead(FLEX_B_PIN);
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        readingsA[i] = a;
        readingsB[i] = b;
    }
    totalA = (long)a * SAMPLE_COUNT;
    totalB = (long)b * SAMPLE_COUNT;
}

void updateFlexReadings() {
    totalA -= readingsA[readIndex];
    totalB -= readingsB[readIndex];
    readingsA[readIndex] = analogRead(FLEX_A_PIN);
    readingsB[readIndex] = analogRead(FLEX_B_PIN);
    totalA += readingsA[readIndex];
    totalB += readingsB[readIndex];
    readIndex = (readIndex + 1) % SAMPLE_COUNT;
    flexA = (int)(totalA / SAMPLE_COUNT);
    flexB = (int)(totalB / SAMPLE_COUNT);
}

// ──────────────────────────────────────────────────────────────
//  LEDS (Red, Yellow, Green)
// ──────────────────────────────────────────────────────────────
void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void updateLed() {
#if LED_FOLLOWS == LED_SOURCE_FLEX_B
    int ledAdc = flexB;
#else
    int ledAdc = flexA;
#endif

    if (ledAdc >= rgb_green_threshold) {
        setLed(false, false, true);   // HIJAU
    } else if (ledAdc >= rgb_yellow_threshold) {
        setLed(false, true, false);   // KUNING
    } else {
        setLed(true, false, false);   // MERAH
    }
}

// ──────────────────────────────────────────────────────────────
//  SERVO (Piecewise-Linear 0° -> 90° -> 180°)
// ──────────────────────────────────────────────────────────────
int lastServoAngle = -1;

int getServoAngle(int adcVal) {
    int flexA_mid = flexA_min - (int)((flexA_min - flexA_max) * 0.65);
    if (adcVal >= flexA_mid) {
        return mapClamped(adcVal, flexA_min, flexA_mid, 0, 90);
    } else {
        return mapClamped(adcVal, flexA_mid, flexA_max, 90, 180);
    }
}

void updateServo() {
    int angle = getServoAngle(flexA);
    if (angle != lastServoAngle) {
        myServo.write(180 - angle); // Hardware inverted (reversed ruler)
        lastServoAngle = angle;
    }
}

// ──────────────────────────────────────────────────────────────
//  LCD 16x4 DISPLAY
// ──────────────────────────────────────────────────────────────
void lcdPrintInt(int col, int row, int value, int width) {
    lcd.setCursor(col, row);
    char buf[17];
    snprintf(buf, sizeof(buf), "%*d", width, value);
    lcd.print(buf);
}

void updateLcd() {
    float voltA = (flexA * 3.428) / 4095.0;
    float voltB = (flexB * 3.428) / 4095.0;

    // Baris 0: Flex A
    lcd.setCursor(0, 0);
    char bufA[17];
    snprintf(bufA, sizeof(bufA), "A:%4d V:%5.3fV", flexA, voltA);
    lcd.print(bufA);
    
    // Baris 1: Flex B
    lcd.setCursor(0, 1);
    char bufB[17];
    snprintf(bufB, sizeof(bufB), "B:%4d V:%5.3fV", flexB, voltB);
    lcd.print(bufB);

    // Baris 2: Sudut Servo
    int angle = getServoAngle(flexA);
    lcd.setCursor(0, 2);
    lcd.print("Angle :");
    lcdPrintInt(7, 2, angle, 3);
    lcd.setCursor(10, 2);
    lcd.print(" deg   ");
}

void lcdShowStatus(const char* msg) {
    lcd.setCursor(0, 3);
    char buf[17];
    snprintf(buf, sizeof(buf), "%-16s", msg);
    lcd.print(buf);
}

// ──────────────────────────────────────────────────────────────
//  WEB SERVER
// ──────────────────────────────────────────────────────────────
void sendCors() {
    server.sendHeader("Access-Control-Allow-Origin",  "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleData() {
    sendCors();
    int panPct  = mapClamped(flexA, flexA_min, flexA_max, -100, 100);
    int gripPct = mapClamped(flexB, flexB_min, flexB_max, 0, 100);
    int angle   = getServoAngle(flexA);

    char json[256];
    snprintf(json, sizeof(json),
        "{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d,\"phrase\":\"\"}",
        flexA, flexB, panPct, (float)angle, gripPct);
    server.send(200, "application/json", json);
}

void handleSetConfig() {
    sendCors();
    int minA = server.hasArg("minA") ? server.arg("minA").toInt() : flexA_min;
    int maxA = server.hasArg("maxA") ? server.arg("maxA").toInt() : flexA_max;
    int minB = server.hasArg("minB") ? server.arg("minB").toInt() : flexB_min;
    int maxB = server.hasArg("maxB") ? server.arg("maxB").toInt() : flexB_max;
    
    flexA_min = minA;
    flexA_max = maxA;
    flexB_min = minB;
    flexB_max = maxB;
    
    int deltaA = flexA_min - flexA_max;
    rgb_green_threshold = flexA_min - (int)(deltaA * 0.50);
    rgb_yellow_threshold = flexA_min - (int)(deltaA * 0.90);
    
    preferences.begin("calib", false);
    preferences.putInt("minA", flexA_min);
    preferences.putInt("maxA", flexA_max);
    preferences.putInt("minB", flexB_min);
    preferences.putInt("maxB", flexB_max);
    preferences.end();
    
    Serial.println("System: Calibration updated via Web!");
    server.send(200, "text/plain", "OK");
}

void startWebServer() {
    if (MDNS.begin(MDNS_NAME)) {
        MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
        Serial.println("mDNS: http://" + String(MDNS_NAME) + ".local");
    }
    server.on("/data", HTTP_GET, handleData);
    server.on("/data", HTTP_OPTIONS, []() { sendCors(); server.send(204); });
    server.on("/config", HTTP_GET, handleSetConfig);
    server.begin();
    serverStarted = true;

    String ip = WiFi.localIP().toString();
    lcdShowStatus(ip.c_str());
}

// ──────────────────────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);

    // ── Load stored calibration ──────────────────────────────────────────────
    preferences.begin("calib", false);
    flexA_min = preferences.getInt("minA", 3040);
    flexA_max = preferences.getInt("maxA", 2800);
    flexB_min = preferences.getInt("minB", 3040);
    flexB_max = preferences.getInt("maxB", 2800);
    
    // Jika memori flash masih menyimpan nilai kalibrasi lama (< 2000), timpa paksa ke 3040-2800
    if (flexA_min < 2000) {
        flexA_min = 3040;
        flexA_max = 2800;
        flexB_min = 3040;
        flexB_max = 2800;
        preferences.putInt("minA", 3040);
        preferences.putInt("maxA", 2800);
        preferences.putInt("minB", 3040);
        preferences.putInt("maxB", 2800);
        Serial.println("System: Stored preferences detected old range. Force updated to 3040-2800!");
    }
    
    int deltaA = flexA_min - flexA_max;
    rgb_green_threshold = flexA_min - (int)(deltaA * 0.50);
    rgb_yellow_threshold = flexA_min - (int)(deltaA * 0.90);
    preferences.end();

    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    setLed(false, false, true); // Default: Hijau (lurus)

    myServo.attach(SERVO_PIN);
    myServo.write(0);

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Flex Sensor Kit ");
    lcd.setCursor(0, 1); lcd.print("Initializing... ");
    lcdShowStatus("Connecting WiFi ");

    initRingBuffer();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ──────────────────────────────────────────────────────────────
//  LOOP
// ──────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── 1. Baca sensor (setiap 5 ms) ─────────────────────────
    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        updateFlexReadings();
        updateServo();
        updateLed();
    }

    // ── 2. Serial Debug (setiap 50 ms) ───────────────────────
    if (now - lastSerial >= SERIAL_INTERVAL_MS) {
        lastSerial = now;
        int angle = getServoAngle(flexA);
        Serial.print("FlexA:"); Serial.print(flexA);
        Serial.print(" FlexB:"); Serial.print(flexB);
        Serial.print(" Angle:"); Serial.println(angle);
    }

    // ── 3. Serial JSON untuk Web Serial (setiap 20 ms) ───────
    if (now - lastSerialJson >= SERIAL_JSON_INTERVAL_MS) {
        lastSerialJson = now;
        int   angle   = getServoAngle(flexA);
        int   panPct  = mapClamped(flexA, flexA_min, flexA_max, 100, -100);
        int   gripPct = mapClamped(flexB, flexB_min, flexB_max, 100, 0);
        char  json[128];
        snprintf(json, sizeof(json),
            "DATA:{\"flexA\":%d,\"flexB\":%d,\"pan\":%d,\"servo\":%.1f,\"grip\":%d,\"phrase\":\"\"}",
            flexA, flexB, panPct, (float)angle, gripPct);
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

    // ── 4. Listen for SET: calibration commands via Serial ──────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("SET:")) {
            String json = line.substring(4);
            flexA_min = parseVal(json, "minA", flexA_min);
            flexA_max = parseVal(json, "maxA", flexA_max);
            flexB_min = parseVal(json, "minB", flexB_min);
            flexB_max = parseVal(json, "maxB", flexB_max);
            
            int deltaA = flexA_min - flexA_max;
            rgb_green_threshold = flexA_min - (int)(deltaA * 0.50);
            rgb_yellow_threshold = flexA_min - (int)(deltaA * 0.90);
            
            preferences.begin("calib", false);
            preferences.putInt("minA", flexA_min);
            preferences.putInt("maxA", flexA_max);
            preferences.putInt("minB", flexB_min);
            preferences.putInt("maxB", flexB_max);
            preferences.end();
            Serial.println("System: Calibration updated via Serial!");
        }
    }

    // ── 5. Update LCD 16x4 (setiap 100 ms) ───────────────────
    if (now - lastLcd >= LCD_INTERVAL_MS) {
        lastLcd = now;
        updateLcd();
    }

    // ── 6. Wi-Fi Status Check ────────────────────────────────
    if (WiFi.status() == WL_CONNECTED) {
        if (!serverStarted) {
            startWebServer();
        }
        server.handleClient();
    } else {
        if (serverStarted) {
            serverStarted = false;
            lcdShowStatus("WiFi Disconnected");
        }
    }
}
