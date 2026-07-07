# 4. Flex + Servo + Lampu + LCD

## 📝 DESKRIPSI PERCOBAAN
Integrasi sistem tertanam lengkap (Labsheet 2) untuk membaca sensor flex, mengendalikan servo motor, menyalakan LED status, dan menampilkan data numerik di layar LCD 16x4.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Servo Motor (SG90 / MG90S)
- 3x LED (Hijau, Kuning, Merah)
- 3x Resistor 220Ω (LED)
- 1x LCD Display 16x4 I2C
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor Flex:
  - Pin 3.3V -> Sensor Flex -> GPIO 34 (dengan Resistor 10kΩ ke GND).

Rangkaian Servo Motor:
  - VCC -> VIN / 5V ESP32, GND -> GND ESP32, Signal -> GPIO 18.

Rangkaian LED Indikator:
  - LED Hijau -> GPIO 25, LED Kuning -> GPIO 26, LED Merah -> GPIO 27 (masing-masing via resistor 220Ω ke GND).

Rangkaian LCD 16x4 I2C:
  - VCC -> VIN / 5V, GND -> GND, SDA -> GPIO 21, SCL -> GPIO 22.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
