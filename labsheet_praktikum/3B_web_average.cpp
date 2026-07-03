/**
 * ============================================================
 *  Labsheet 3 - Percobaan B: Pemfilteran Data (Moving Average)
 * ============================================================
 *  Tujuan: Membandingkan data mentah (Raw Data) dengan data rata-rata
 *          bergerak (Moving Average) untuk meredam noise/jitter.
 */

#include <Arduino.h>

#define FLEX_A_PIN  34  // Pin ADC untuk Sensor Flex A

// Pengaturan Moving Average
#define SAMPLE_COUNT  20  // Jumlah sampel buffer (20 data)
int readingsA[SAMPLE_COUNT] = {0};  // Array penyimpanan sampel
int readIndex = 0;                  // Indeks pengisian data
long totalA = 0;                    // Total nilai seluruh sampel
int averagedFlexA = 0;              // Hasil rata-rata akhir

void setup() {
    Serial.begin(115200);
    
    // Konfigurasi ADC
    analogReadResolution(12);
    analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
    
    // Inisialisasi awal seluruh buffer dengan pembacaan pertama
    int initialVal = analogRead(FLEX_A_PIN);
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        readingsA[i] = initialVal;
    }
    totalA = (long)initialVal * SAMPLE_COUNT;
    averagedFlexA = initialVal;
    
    Serial.println("Percobaan 3B: Filter Moving Average Mulai");
}

void loop() {
    // ── PROSES FILTER MOVING AVERAGE (SAMPEL DATA) ───────────────────────────
    // 1. Kurangi total dengan nilai terlama (yang akan ditimpa)
    totalA -= readingsA[readIndex];
    
    // 2. Baca sampel baru dari ADC
    int rawValue = analogRead(FLEX_A_PIN);
    readingsA[readIndex] = rawValue;
    
    // 3. Tambahkan sampel baru ke total
    totalA += readingsA[readIndex];
    
    // 4. Geser indeks penunjuk buffer berikutnya (circular buffer)
    readIndex = (readIndex + 1) % SAMPLE_COUNT;
    
    // 5. Hitung nilai rata-rata sampel
    averagedFlexA = (int)(totalA / SAMPLE_COUNT);
    
    // ── TAMPILKAN PERBANDINGAN DI SERIAL MONITOR ─────────────────────────────
    // Format agar bisa dianalisis via Arduino Serial Plotter
    Serial.print("Mentah(Raw):");
    Serial.print(rawValue);
    Serial.print("   ");
    Serial.print("Filter(Averaged):");
    Serial.println(averagedFlexA);
    
    delay(5);  // Pembacaan sampel sensor setiap 5 milidetik
}
