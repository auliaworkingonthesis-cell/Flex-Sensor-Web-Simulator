# 3. Flex + Servo + Lampu

## 📝 DESKRIPSI PERCOBAAN
Penggabungan kontrol aktuator servo motor dan indikator LED status yang digerakkan secara bersamaan oleh lekukan 1 sensor flex.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Servo Motor (SG90 / MG90S)
- 3x LED (Hijau, Kuning, Merah)
- 3x Resistor 220Ω (LED)
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor Flex:
  - Pin 3.3V ESP32 -> Sensor Flex -> GPIO 34 ESP32 (dengan Resistor 10kΩ ke GND).

Rangkaian Servo Motor:
  - Kabel Merah (VCC) -> VIN / 5V ESP32.
  - Kabel Cokelat (GND) -> GND ESP32.
  - Kabel Oranye (Signal) -> Pin GPIO 18 ESP32.

Rangkaian LED Indikator:
  - LED Hijau -> Resistor 220Ω -> Pin GPIO 25.
  - LED Kuning -> Resistor 220Ω -> Pin GPIO 26.
  - LED Merah -> Resistor 220Ω -> Pin GPIO 27.
  - Semua Katode LED -> GND ESP32.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
