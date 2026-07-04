/**
 * ============================================================
 *  Flex Sensor Trainer Kit — ESP32 Firmware
 *  Platform : PlatformIO + Arduino Framework
 *  Board    : ESP32 DevKit V1
 * ============================================================
 *
 *  Hardware yang digunakan:
 *  - 2x Flex Sensor (voltage divider ke ADC ESP32)
 *  - 1x LCD 16x4 via I2C backpack (SDA = GPIO 21, SCL = GPIO 22)
 *  - 1x RGB LED common-cathode (3 pin terpisah: R, G, B)
 *  - 1x Servo motor
 *  - Wi-Fi + mDNS → tersambung ke Web Simulator
 *
 *  Voltage Divider (3.3V supply):
 *  ┌── 3.3V
 *  │
 *  [Flex Sensor ~16.6kΩ–20kΩ]
 *  │
 *  ├── GPIO ADC pin     ← ADC ~2130 (lurus) .. ~1940 (melengkung)
 *  │                       (makin bengkok = Rflex naik = ADC TURUN)
 *  [R2 = 10kΩ]  ← Menggunakan resistor 10kΩ (Coklat Hitam Orange Emas)
 *  │
 *  └── GND
 *
 *  Tambahkan kapasitor 100nF paralel dengan R2 ke GND untuk filter noise.
 *
 *  ============================================================
 *  KONFIGURASI UTAMA — Ubah di sini sebelum upload
 *  ============================================================
 */


// ── Wi-Fi Credentials ─────────────────────────────────────────────────────────
#define WIFI_SSID        "NAMA_WIFI"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define MDNS_NAME        "flex-kelompok1"   // Akses: http://flex-kelompok1.local

// ── Pin Mapping ───────────────────────────────────────────────────────────────
#define FLEX_A_PIN       34    // ADC1_CH6 — Flex Sensor A (Servo & Rack)
#define FLEX_B_PIN       35    // ADC1_CH7 — Flex Sensor B (Grip & Audio)
#define SERVO_PIN        18    // PWM output untuk servo motor
#define LED_RED_PIN      25    // LED Merah  — GPIO 25
#define LED_YELLOW_PIN   26    // LED Kuning — GPIO 26
#define LED_GREEN_PIN    27    // LED Hijau  — GPIO 27
// LCD I2C: SDA = GPIO 21, SCL = GPIO 22 (default Arduino ESP32)

// ── Flex Sensor Calibration variables (initially loaded from Preferences) ────



int flexA_min = 3054;  // ADC saat lurus (0°)  — diukur dari sensor
int flexA_max = 2766;  // ADC saat bengkok 180° — = 3054 - 2*(3054-2910)
int flexB_min = 3054;
int flexB_max = 2766;

int rgb_green_threshold  = 1260; // Dihitung dinamis: minA - 50% delta
int rgb_yellow_threshold = 1210; // Dihitung dinamis: minA - 90% delta

// ── Servo Angle Range ─────────────────────────────────────────────────────────
#define SERVO_ANGLE_MIN  0     // Sudut minimum servo (derajat)
#define SERVO_ANGLE_MAX  180   // Sudut maksimum servo (derajat)

// ── RGB LED — pilih sensor yang dikontrol LED ────────────────────────────────
// Ganti nilai di bawah sesuai kebutuhan:
//   LED_SOURCE_FLEX_A  → LED mengikuti Flex A (servo & rack)
//   LED_SOURCE_FLEX_B  → LED mengikuti Flex B (grip & audio)
#define LED_SOURCE_FLEX_A  0
#define LED_SOURCE_FLEX_B  1
#define LED_FOLLOWS        LED_SOURCE_FLEX_A   // ← GANTI DI SINI

// LED indikator dikontrol mengikuti logic warna dinamis (Hijau, Kuning, Merah)
// Ambang batas warna sekarang otomatis ter-update saat kalibrasi disinkronkan.

// ── Sampling ──────────────────────────────────────────────────────────────────
#define SAMPLE_COUNT          10     // Jumlah sampel moving average – ubah nilai ini untuk eksperimen (default 10)
#define SENSOR_INTERVAL_MS    20      // Interval baca sensor (ms) – slower sampling for stability
#define SERIAL_INTERVAL_MS    50     // Interval print serial human-readable (ms)
#define SERIAL_JSON_INTERVAL_MS 20   // Interval output JSON ke Web Serial API (ms)
#define LCD_INTERVAL_MS       100    // Interval update LCD (ms)
#define WIFI_RETRY_MS         5000   // Interval retry Wi-Fi (ms)

// ─────────────────────────────────────────────────────────────────────────────
//  END OF CONFIG — Tidak perlu ubah di bawah baris ini untuk pemakaian normal
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

// ── Objects ───────────────────────────────────────────────────────────────────
WebServer       server(80);
Servo           myServo;
LiquidCrystal_I2C lcd(0x27, 16, 4);   // Alamat I2C 0x27 (scan dulu kalau tidak jalan)

// ── Moving average ring buffer ────────────────────────────────────────────────
int   readingsA[SAMPLE_COUNT] = {};
int   readingsB[SAMPLE_COUNT] = {};
int   readIndex    = 0;
long  totalA       = 0;
long  totalB       = 0;
int   flexA        = 0;    // hasil rata-rata Flex A
int   flexB        = 0;    // hasil rata-rata Flex B

// ── Timestamps (non-blocking) ─────────────────────────────────────────────────
unsigned long lastSensorRead  = 0;
unsigned long lastSerial      = 0;
unsigned long lastSerialJson  = 0;
unsigned long lastLcd         = 0;
unsigned long lastWifiRetry   = 0;

// ── State flags ───────────────────────────────────────────────────────────────
bool serverStarted = false;
bool mdnsStarted   = false;

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────

/** Clamp + map integer value from one range to another */
int mapClamped(int val, int inMin, int inMax, int outMin, int outMax) {
    int low = min(inMin, inMax);
    int high = max(inMin, inMax);
    val = constrain(val, low, high);
    return map(val, inMin, inMax, outMin, outMax);
}

/** Inisialisasi semua slot ring buffer dengan nilai ADC saat ini */
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

/** Baca satu sampel baru dan perbarui rata-rata (non-blocking) */
#define FLEX_B_ENABLED 0 // 1 = read Flex B (when wired), 0 = ignore (pin unconnected)

void updateFlexReadings() {
    // ------- Flex A (always used) -------
    totalA -= readingsA[readIndex];
    readingsA[readIndex] = analogRead(FLEX_A_PIN);
    totalA += readingsA[readIndex];
    flexA = (int)(totalA / SAMPLE_COUNT);

#if FLEX_B_ENABLED
    // ------- Flex B (optional) -------
    totalB -= readingsB[readIndex];
    readingsB[readIndex] = analogRead(FLEX_B_PIN);
    totalB += readingsB[readIndex];
    flexB = (int)(totalB / SAMPLE_COUNT);
#else
    // When Flex B is not connected, keep a stable dummy value
    flexB = 0;
#endif

    readIndex = (readIndex + 1) % SAMPLE_COUNT;
}

// ─────────────────────────────────────────────────────────────────────────────
//  LEDS (Red, Yellow, Green — Separate pins)
// ─────────────────────────────────────────────────────────────────────────────
void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

/** Perbarui warna LED berdasarkan nilai ADC Flex A atau Flex B
 *  (pilih via #define LED_FOLLOWS di bagian konfigurasi atas)
 *  Ingat: ADC TURUN saat makin bengkok
 *  Hijau  : ADC >= RGB_GREEN_THRESHOLD  (hampir lurus)
 *  Kuning : RGB_YELLOW_THRESHOLD <= ADC < RGB_GREEN_THRESHOLD  (~90 derajat)
 *  Merah  : ADC < RGB_YELLOW_THRESHOLD  (melengkung penuh)
 */
void updateLed() {
#if LED_FOLLOWS == LED_SOURCE_FLEX_B
    int ledAdc = flexB;   // LED mengikuti Flex B
#else
    int ledAdc = flexA;   // LED mengikuti Flex A (default)
#endif

    if (ledAdc >= rgb_green_threshold) {
        setLed(false, false, true);   // HIJAU  — lurus
    } else if (ledAdc >= rgb_yellow_threshold) {
        setLed(false, true, false);   // KUNING — bengkok ~90° (nyalakan pin kuning saja)
    } else {
        setLed(true, false, false);   // MERAH  — bengkok penuh
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SERVO
// ─────────────────────────────────────────────────────────────────────────────
int lastServoAngle = -1;

/** Hitung sudut servo secara piecewise-linear agar:
 *  Lurus       → 0 derajat
 *  90 derajat  → 90 derajat
 *  Bengkok maks→ 180 derajat
 */
int getServoAngle(int adcVal) {
    // Simplified linear mapping: 0° at flexA_min (straight), 180° at flexA_max (fully bent)
    // This makes the servo point to ~90° when the flex sensor reading is roughly halfway between min and max.
    return mapClamped(adcVal, flexA_min, flexA_max, 0, 180);
}

/** Perbarui posisi servo berdasarkan Flex A */
void updateServo() {
    int angle = getServoAngle(flexA);
    const int HYSTERESIS_DEG = 2; // ignore changes smaller than 2°
    if (abs(angle - lastServoAngle) < HYSTERESIS_DEG) {
        return; // keep previous position
    }
    myServo.write(180 - angle); // Servo fisik terbalik: 0°=kiri, 180°=kanan
    lastServoAngle = angle;
}

// ─────────────────────────────────────────────────────────────────────────────
//  LCD 16x4
//  Baris 0 : "Flex A : XXXX"
//  Baris 1 : "Flex B : XXXX"
//  Baris 2 : "Angle  :  XXX deg"
//  Baris 3 : IP / Status Wi-Fi
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Tulis integer ke LCD di posisi tertentu.
 * Selalu cetak dengan lebar tetap (width karakter) agar tidak ada sisa teks lama.
 * Contoh: width=4 → "   5", " 390", "1025"
 */
void lcdPrintInt(int col, int row, int value, int width) {
    lcd.setCursor(col, row);
    // Buat string lebar tetap (right-aligned, padding spasi di kiri)
    char buf[17];
    snprintf(buf, sizeof(buf), "%*d", width, value);
    lcd.print(buf);
}

void updateLcd() {
    float voltA = (flexA * 3.465) / 4095.0;
    float voltB = (flexB * 3.465) / 4095.0;

    // ── Baris 0: Flex A ──────────────────────────────────────────────────────
    lcd.setCursor(0, 0);
    char bufA[17];
    snprintf(bufA, sizeof(bufA), "A:%4d V:%5.3fV", flexA, voltA);
    lcd.print(bufA);

    // ── Baris 1: Flex B ──────────────────────────────────────────────────────
    lcd.setCursor(0, 1);
    char bufB[17];
    snprintf(bufB, sizeof(bufB), "B:%4d V:%5.3fV", flexB, voltB);
    lcd.print(bufB);

    // ── Baris 2: Servo Angle ─────────────────────────────────────────────────
    int angle = getServoAngle(flexA);
    lcd.setCursor(0, 2);
    lcd.print("Angle :");
    lcdPrintInt(7, 2, angle, 3);
    lcd.setCursor(10, 2);
    lcd.print(" deg   ");

    // ── Baris 3: Wi-Fi status ─────────────────────────────────────────────────
    // Hanya di-update saat status berubah (dihandle di startWebServer / loop Wi-Fi)
}

void lcdShowStatus(const char* msg) {
    lcd.setCursor(0, 3);
    // Pad spasi sampai 16 karakter agar menghapus teks lama
    char buf[17];
    snprintf(buf, sizeof(buf), "%-16s", msg);
    lcd.print(buf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  WEB SERVER & mDNS
// ─────────────────────────────────────────────────────────────────────────────
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

void startMdns() {
    if (MDNS.begin(MDNS_NAME)) {
        MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
        Serial.println("mDNS: http://" + String(MDNS_NAME) + ".local");
    }
}

void handleSetConfig() {
    sendCors();
    int minA = server.hasArg("minA") ? server.arg("minA").toInt() : flexA_min;
    int maxA = server.hasArg("maxA") ? server.arg("maxA").toInt() : flexA_max;
    int minB = server.hasArg("minB") ? server.arg("minB").toInt() : flexB_min;
    int maxB = server.hasArg("maxB") ? server.arg("maxB").toInt() : flexB_max;
    
    // Save to preferences
    flexA_min = minA;
    flexA_max = maxA;
    flexB_min = minB;
    flexB_max = maxB;
    
    int deltaA = flexA_min - flexA_max;
    rgb_green_threshold = flexA_min - (int)(deltaA * 0.50);
    rgb_yellow_threshold = flexA_min - (int)(deltaA * 0.90);
    
    
    
    Serial.println("System: Calibration updated via Web!");
    server.send(200, "text/plain", "OK");
}

void startWebServer() {
    startMdns();
    server.on("/", HTTP_GET, []() {
        sendCors();
        server.send(200, "text/plain", "Flex Sensor ESP32 Online");
    });
    server.on("/data", HTTP_OPTIONS, []() { sendCors(); server.send(204); });
    server.on("/data", HTTP_GET, handleData);
    server.on("/config", HTTP_GET, handleSetConfig);
    server.begin();
    serverStarted = true;

    String ip = WiFi.localIP().toString();
    Serial.println("IP: http://" + ip);

    char ipbuf[17];
    snprintf(ipbuf, sizeof(ipbuf), "%-16s", ip.c_str());
    lcdShowStatus(ipbuf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // ── ADC config ────────────────────────────────────────────────────────────
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);

    // ── Load stored calibration ──────────────────────────────────────────────
    

    // ── LED pins ─────────────────────────────────────────────────────────────
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    setLed(false, false, true);   // Default: hijau = posisi awal (lurus)

    // ── Servo ─────────────────────────────────────────────────────────────────
    myServo.attach(SERVO_PIN);
    myServo.write(SERVO_ANGLE_MIN);

    // ── LCD ───────────────────────────────────────────────────────────────────
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Flex Sensor Kit ");
    lcd.setCursor(0, 1); lcd.print("Initializing... ");
    lcd.setCursor(0, 2); lcd.print("Connecting WiFi ");
    lcd.setCursor(0, 3); lcd.print(MDNS_NAME);

    // ── Ring buffer ───────────────────────────────────────────────────────────
    initRingBuffer();

    // ── Wi-Fi ─────────────────────────────────────────────────────────────────
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi: " + String(WIFI_SSID));
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Baca sensor (setiap 5 ms) ─────────────────────────────────────────────
    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        updateFlexReadings();
        updateServo();
        updateLed();
    }

    // ── Serial human-readable (setiap 50 ms) ─────────────────────────────────
    if (now - lastSerial >= SERIAL_INTERVAL_MS) {
        lastSerial = now;
        int angle = getServoAngle(flexA);
        Serial.print("FlexA:"); Serial.print(flexA);
        Serial.print(" FlexB:"); Serial.print(flexB);
        Serial.print(" Angle:"); Serial.println(angle);
    }

    // ── Serial JSON untuk Web Serial API (setiap 20 ms) ──────────────────────
    // Format: DATA:{...}\n  → dibaca langsung oleh browser via Web Serial API
    if (now - lastSerialJson >= SERIAL_JSON_INTERVAL_MS) {
        lastSerialJson = now;
        int   angle   = getServoAngle(flexA);
        int   panPct  = mapClamped(flexA, flexA_min, flexA_max, 100, -100);  // Reversed
        int   gripPct = mapClamped(flexB, flexB_min, flexB_max, 100, 0);     // Reversed
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

    // ── Listen for SET: calibration commands via Serial ──────────────────────
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.startsWith("SET:")) {
            String json = line.substring(4);
            int minA = parseVal(json, "minA", flexA_min);
            int maxA = parseVal(json, "maxA", flexA_max);
            int minB = parseVal(json, "minB", flexB_min);
            int maxB = parseVal(json, "maxB", flexB_max);
            
            flexA_min = minA;
            flexA_max = maxA;
            flexB_min = minB;
            flexB_max = maxB;
            
            int deltaA = flexA_min - flexA_max;
            rgb_green_threshold = flexA_min - (int)(deltaA * 0.50);
            rgb_yellow_threshold = flexA_min - (int)(deltaA * 0.90);
            
            
            Serial.println("System: Calibration updated via Serial!");
        }
    }

    // ── Update LCD (setiap 100 ms) ────────────────────────────────────────────
    if (now - lastLcd >= LCD_INTERVAL_MS) {
        lastLcd = now;
        updateLcd();
    }

    // ── Wi-Fi & Web Server handling ───────────────────────────────────────────
    if (WiFi.status() == WL_CONNECTED) {
        if (!serverStarted) {
            Serial.println("WiFi connected!");
            startWebServer();
        } else if (!mdnsStarted) {
            Serial.println("WiFi reconnected, restarting mDNS...");
            startMdns();
            lcdShowStatus(WiFi.localIP().toString().c_str());
        }
        server.handleClient();
    } else {
        if (mdnsStarted) {
            MDNS.end();
            mdnsStarted = false;
            lcdShowStatus("WiFi Disconnected");
        }
        if (now - lastWifiRetry >= WIFI_RETRY_MS) {
            lastWifiRetry = now;
            Serial.println("WiFi not connected, retrying...");
            WiFi.reconnect();
            lcdShowStatus("Reconnecting... ");
        }
    }
}