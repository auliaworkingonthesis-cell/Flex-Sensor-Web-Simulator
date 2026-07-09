#include <Arduino.h>
/**
 * ============================================================
 *  Program Labsheet 2: Flex + Servo + Lampu
 * ============================================================
 *  Deskripsi: Integrasi pergerakan Servo Motor dan indikator 
 *             3 buah lampu LED berdasarkan lekukan Sensor Flex A
 *             secara non-blocking.
 */

#include <ESP32Servo.h>

#define FLEX_A_PIN      34  // Pin ADC untuk Sensor Flex A
#define SERVO_PIN        18  // Pin PWM untuk Servo Motor
#define LED_RED_PIN      25  // LED Merah
#define LED_YELLOW_PIN   26  // LED Kuning
#define LED_GREEN_PIN    27  // LED Hijau

// Kalibrasi ADC pembagi tegangan 22K (5V supply)
// Nilai lurus (tidak bengkok) → ADC tinggi
// Nilai bengkok maksimal     → ADC rendah
#define FLEX_A_MIN  3054  // ADC saat lurus
#define FLEX_A_MAX  2766  // ADC saat bengkok maksimal

// Threshold LED
#define THRESHOLD_GREEN   2910
#define THRESHOLD_YELLOW  2795

Servo myServo;
int lastAngle = -1;
unsigned long lastUpdate = 0;
const unsigned long interval = 20; // Check interval 20ms

// Fungsi untuk membaca rata-rata analog (Oversampling 20 sampel untuk stabilitas)
int readAverage(int pin) {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(pin);
        delayMicroseconds(50);
    }
    return sum / 20;
}

void setLed(bool r, bool y, bool g) {
    digitalWrite(LED_RED_PIN,    r ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, y ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  g ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Konfigurasi pin LED sebagai OUTPUT
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    setLed(false, false, false);
    
    // Konfigurasi Servo
    myServo.attach(SERVO_PIN);
    myServo.write(180);
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        
        int rawADC = readAverage(FLEX_A_PIN);

        // ── 1. Gerakkan Servo Motor ──────────────────────────────────────────────
        // Rangkaian Pembagi Tegangan: Vin=5V, R2=22kOhm, R1=Flex Sensor
        int lowLimit  = min(FLEX_A_MIN, FLEX_A_MAX);
        int highLimit = max(FLEX_A_MIN, FLEX_A_MAX);
        int constrainedADC = constrain(rawADC, lowLimit, highLimit);
        int angle = map(constrainedADC, FLEX_A_MIN, FLEX_A_MAX, 0, 180);
        
        // Output Serial Debug
        Serial.printf("Flex: %d | Servo: %d deg | LED: %s\n", rawADC, angle,
            (rawADC >= THRESHOLD_GREEN) ? "HIJAU" : ((rawADC >= THRESHOLD_YELLOW) ? "KUNING" : "MERAH"));
        
        if (angle != lastAngle) {
            myServo.write(180 - angle); // Inversi hardware servo fisik
            lastAngle = angle;
        }
        
        // ── 2. Nyalakan Lampu LED Sesuai Kondisi ──────────────────────────────────
        if (rawADC >= THRESHOLD_GREEN) {
            setLed(false, false, true);   // Hijau (Lurus)
        } 
        else if (rawADC >= THRESHOLD_YELLOW) {
            setLed(false, true, false);   // Kuning (Bengkok ~90°)
        } 
        else {
            setLed(true, false, false);   // Merah (Bengkok maksimal)
        }
    }
}
