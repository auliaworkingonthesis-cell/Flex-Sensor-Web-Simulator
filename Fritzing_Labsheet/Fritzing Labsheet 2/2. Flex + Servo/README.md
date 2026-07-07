# 2. Flex + Servo

## 📝 DESKRIPSI PERCOBAAN
Pemetaan pergerakan lekukan sensor flex (0° s.d 180°) ke posisi putaran servo motor secara langsung.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Servo Motor (SG90 / MG90S)
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor Flex:
  - Hubungkan Pin 3.3V ESP32 -> Salah satu kaki Sensor Flex.
  - Hubungkan kaki Sensor Flex lainnya -> Pin GPIO 34 ESP32 AND kaki Resistor 10kΩ.
  - Hubungkan kaki Resistor 10kΩ lainnya -> Pin GND ESP32.

Rangkaian Servo Motor:
  - Hubungkan Kabel Merah (VCC) Servo -> Pin VIN / 5V ESP32.
  - Hubungkan Kabel Cokelat/Hitam (GND) Servo -> Pin GND ESP32.
  - Hubungkan Kabel Oranye/Kuning (Signal) Servo -> Pin GPIO 18 ESP32.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
