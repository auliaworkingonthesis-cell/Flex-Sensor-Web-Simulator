# 1. Flex + Led

## 📝 DESKRIPSI PERCOBAAN
Klasifikasi lekukan sensor flex untuk menyalakan LED status secara bergantian: Hijau (0°/Lurus), Kuning (90°/Sedang), dan Merah (180°/Bengkok Maksimal).

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 3x LED (Hijau, Kuning, Merah)
- 3x Resistor 220Ω (Pembatas arus LED)
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor Flex:
  - Hubungkan Pin 3.3V ESP32 -> Salah satu kaki Sensor Flex.
  - Hubungkan kaki Sensor Flex lainnya -> Pin GPIO 34 ESP32 AND kaki Resistor 10kΩ.
  - Hubungkan kaki Resistor 10kΩ lainnya -> Pin GND ESP32.

Rangkaian LED Indikator:
  - Hubungkan Anode (+) LED Hijau -> Resistor 220Ω -> Pin GPIO 25 ESP32.
  - Hubungkan Anode (+) LED Kuning -> Resistor 220Ω -> Pin GPIO 26 ESP32.
  - Hubungkan Anode (+) LED Merah -> Resistor 220Ω -> Pin GPIO 27 ESP32.
  - Hubungkan seluruh Katode (-) ketiga LED ke GND ESP32.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
