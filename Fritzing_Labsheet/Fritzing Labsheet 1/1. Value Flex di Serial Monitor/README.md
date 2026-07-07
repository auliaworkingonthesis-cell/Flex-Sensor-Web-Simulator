# 1. Value Flex di Serial Monitor

## 📝 DESKRIPSI PERCOBAAN
Praktikum dasar membaca nilai analog (ADC) sensor flex menggunakan mikrokontroler ESP32 dan menampilkannya di Serial Monitor Arduino IDE.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
1. Hubungkan Pin 3.3V ESP32 -> Salah satu kaki Sensor Flex.
2. Hubungkan kaki Sensor Flex lainnya -> Pin GPIO 34 (ADC1_CH6) ESP32 AND salah satu kaki Resistor 10kΩ.
3. Hubungkan kaki Resistor 10kΩ lainnya -> Pin GND ESP32.
4. Colok kabel USB dari ESP32 ke laptop untuk mengunggah program dan membuka Serial Monitor.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
