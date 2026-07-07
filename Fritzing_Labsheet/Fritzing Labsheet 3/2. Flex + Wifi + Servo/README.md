# 2. Flex + Wifi + Servo

## 📝 DESKRIPSI PERCOBAAN
Koneksi data sensor flex ke Web Simulator secara nirkabel (Wi-Fi) dengan ESP32 sebagai Web Server lokal (domain http://flex-kelompok1.local) untuk sinkronisasi posisi 3D lengan SCARA.

## 🛠️ KOMPONEN YANG DIBUTUHKAN
- 1x ESP32 NodeMCU
- 1x Sensor Flex
- 1x Resistor 10kΩ (Pembagi Tegangan)
- 1x Servo Motor (SG90 / MG90S)
- 1x Project Board (Breadboard)
- Kabel Jumper secukupnya

## 🔌 PANDUAN WIRING / KONEKSI FISIK
Rangkaian Sensor & Servo (Sama dengan Labsheet 2 B):
  - Pin 3.3V -> Sensor Flex -> GPIO 34 (dengan Resistor 10kΩ ke GND).
  - Servo VCC -> VIN / 5V, GND -> GND, Signal -> GPIO 18.

Komunikasi Data Wi-Fi:
  - Pastikan laptop Anda terhubung ke jaringan Wi-Fi yang sama dengan yang diatur di dalam kode ESP32.
  - Buka Web Simulator lalu masukkan alamat IP ESP32 atau mDNS URL 'http://flex-kelompok1.local' untuk sinkronisasi data secara nirkabel.

## 💾 TEMPLATE FRITZING (.fzz)
Buka file project Fritzing `.fzz` di folder ini untuk mulai merangkai di software Fritzing. Template ini sudah berisi grid board dan layout standar untuk mempermudah praktikum.
