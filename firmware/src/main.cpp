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
 *  [R2 = 18kΩ]  ← R_optimal = sqrt(16600*20000) ≈ 18.2kΩ
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
#define LED_RED_PIN      25    // RGB LED — pin Merah
#define LED_GREEN_PIN    26    // RGB LED — pin Hijau
#define LED_BLUE_PIN     27    // RGB LED — pin Biru
// LCD I2C: SDA = GPIO 21, SCL = GPIO 22 (default Arduino ESP32)

// ── Flex Sensor A (Servo & Rack pan) — ADC range ─────────────────────────────
// CATATAN: Rflex naik saat bengkok → Vout TURUN → ADC TURUN
// Sehingga: MIN = ADC saat melengkung (bengkok penuh), MAX = ADC saat lurus
// Dengan R2=18kΩ dan Rflex 16.6k–20k:
//   Lurus   : ADC ≈ 2130  (Rflex=16.6kΩ, Vout=1.72V)
//   Bengkok : ADC ≈ 1940  (Rflex=20kΩ,   Vout=1.56V)
// → map(flexA, FLEX_A_MIN, FLEX_A_MAX, ...) = 0 saat lurus, 100 saat bengkok
#define FLEX_A_MIN       2130  // ADC saat sensor LURUS  (Rflex=16.6kΩ, Vout max)
#define FLEX_A_MAX       1940  // ADC saat sensor BENGKOK (Rflex=20kΩ, Vout min)
// Tip: Baca nilai Serial Monitor "FlexA:xxxx" untuk kalibrasi aktual

// ── Flex Sensor B (Grip & Audio) — ADC range ─────────────────────────────────
#define FLEX_B_MIN       2130  // ADC saat sensor LURUS
#define FLEX_B_MAX       1940  // ADC saat sensor BENGKOK

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

// ── RGB LED Threshold (berlaku untuk sensor yang dipilih di LED_FOLLOWS) ──────
// INGAT: ADC TURUN saat semakin bengkok!
// LED Hijau  : ADC >= RGB_GREEN_THRESHOLD  (sensor hampir lurus)
// LED Kuning : RGB_YELLOW_THRESHOLD <= ADC < RGB_GREEN_THRESHOLD  (~90 derajat)
// LED Merah  : ADC < RGB_YELLOW_THRESHOLD  (melengkung penuh)
// Dengan range ~1940–2130, bagi 3 zona:
//   Hijau  : ADC >= 2070  (0–33% bengkok)
//   Kuning : 2000 <= ADC < 2070  (33–66% bengkok)
//   Merah  : ADC < 2000   (66–100% bengkok)
#define RGB_GREEN_THRESHOLD   2070   // Batas atas zona hijau (ADC naik = lurus)
#define RGB_YELLOW_THRESHOLD  2000   // Batas atas zona kuning

// ── Sampling ──────────────────────────────────────────────────────────────────
#define SAMPLE_COUNT          20     // Jumlah sampel moving average
#define SENSOR_INTERVAL_MS    5      // Interval baca sensor (ms)
#define SERIAL_INTERVAL_MS    50     // Interval print serial (ms)
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
    val = constrain(val, inMin, inMax);
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

// ─────────────────────────────────────────────────────────────────────────────
//  RGB LED
// ─────────────────────────────────────────────────────────────────────────────
void setLed(bool r, bool g, bool b) {
    digitalWrite(LED_RED_PIN,   r ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN, g ? HIGH : LOW);
    digitalWrite(LED_BLUE_PIN,  b ? HIGH : LOW);
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

    if (ledAdc >= RGB_GREEN_THRESHOLD) {
        setLed(false, true, false);   // HIJAU  — posisi awal / lurus
    } else if (ledAdc >= RGB_YELLOW_THRESHOLD) {
        setLed(true, true, false);    // KUNING — melengkung sedang (~90°)
    } else {
        setLed(true, false, false);   // MERAH  — melengkung penuh
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SERVO
// ─────────────────────────────────────────────────────────────────────────────
int lastServoAngle = -1;

/** Perbarui posisi servo berdasarkan Flex A */
void updateServo() {
    int angle = mapClamped(flexA, FLEX_A_MIN, FLEX_A_MAX, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
    if (angle != lastServoAngle) {
        myServo.write(angle);
        lastServoAngle = angle;
    }
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
    // ── Baris 0: Flex A ──────────────────────────────────────────────────────
    lcd.setCursor(0, 0);
    lcd.print("Flex A:");
    lcdPrintInt(7, 0, flexA, 4);
    lcd.setCursor(11, 0);
    lcd.print("     ");   // Clear sisa kolom 11–15

    // ── Baris 1: Flex B ──────────────────────────────────────────────────────
    lcd.setCursor(0, 1);
    lcd.print("Flex B:");
    lcdPrintInt(7, 1, flexB, 4);
    lcd.setCursor(11, 1);
    lcd.print("     ");

    // ── Baris 2: Servo Angle ─────────────────────────────────────────────────
    int angle = mapClamped(flexA, FLEX_A_MIN, FLEX_A_MAX, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
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
    int panPct  = mapClamped(flexA, FLEX_A_MIN, FLEX_A_MAX, -100, 100);
    int gripPct = mapClamped(flexB, FLEX_B_MIN, FLEX_B_MAX, 0, 100);
    int angle   = mapClamped(flexA, FLEX_A_MIN, FLEX_A_MAX, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);

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

void startWebServer() {
    startMdns();
    server.on("/", HTTP_GET, []() {
        sendCors();
        server.send(200, "text/plain", "Flex Sensor ESP32 Online");
    });
    server.on("/data", HTTP_OPTIONS, []() { sendCors(); server.send(204); });
    server.on("/data", HTTP_GET, handleData);
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

    // ── RGB LED pins ──────────────────────────────────────────────────────────
    pinMode(LED_RED_PIN,   OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN,  OUTPUT);
    setLed(false, true, false);   // Default: hijau = posisi awal

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

    // ── Serial print (setiap 50 ms) ───────────────────────────────────────────
    if (now - lastSerial >= SERIAL_INTERVAL_MS) {
        lastSerial = now;
        int angle = mapClamped(flexA, FLEX_A_MIN, FLEX_A_MAX, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
        Serial.print("FlexA:"); Serial.print(flexA);
        Serial.print(" FlexB:"); Serial.print(flexB);
        Serial.print(" Angle:"); Serial.println(angle);
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
