# 1. Flex + Serial + Servo

## 📝 DESKRIPSI PERCOBAAN
Koneksi data sensor flex ke Web Simulator menggunakan kabel USB (Serial) untuk mengendalikan visualisasi 3D lengan SCARA secara real-time.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Servo Motor (SG90 / MG90S)
- 1x Kabel USB Data
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor & Servo (Sama dengan Labsheet 2 B):
  - Pin 3.3V -> Sensor Flex -> GPIO 34 (dengan Resistor 10kΩ ke GND).
  - Servo VCC -> VIN / 5V, GND -> GND, Signal -> GPIO 18.

Komunikasi Data:
  - Hubungkan kabel USB dari port ESP32 ke port USB laptop Anda.
  - Buka browser Chrome/Edge, masuk ke Web Simulator, lalu klik tombol 'Connect Serial' untuk sinkronisasi data sensor flex fisik.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
