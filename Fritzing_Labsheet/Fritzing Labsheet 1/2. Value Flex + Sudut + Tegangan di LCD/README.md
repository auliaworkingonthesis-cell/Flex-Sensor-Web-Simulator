# 2. Value Flex + Sudut + Tegangan di LCD

## 📝 DESKRIPSI PERCOBAAN
Praktikum monitoring data sensor flex (nilai ADC, perhitungan tegangan listrik aktual, dan konversi sudut lekukan) secara mandiri di layar display LCD 16x4 I2C.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x LCD Display 16x4 I2C
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor Flex:
  - Hubungkan Pin 3.3V ESP32 -> Salah satu kaki Sensor Flex.
  - Hubungkan kaki Sensor Flex lainnya -> Pin GPIO 34 ESP32 AND kaki Resistor 10kΩ.
  - Hubungkan kaki Resistor 10kΩ lainnya -> Pin GND ESP32.

Rangkaian LCD 16x4 I2C:
  - Hubungkan Pin VCC LCD -> Pin VIN / 5V ESP32.
  - Hubungkan Pin GND LCD -> Pin GND ESP32.
  - Hubungkan Pin SDA LCD -> Pin GPIO 21 ESP32.
  - Hubungkan Pin SCL LCD -> Pin GPIO 22 ESP32.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
