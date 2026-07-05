# Panduan Troubleshooting & Solusi: Trainer Kit Sensor Flex

Dokumen ini berisi daftar kendala teknis (troubles) yang sering ditemui selama perakitan dan pengujian Trainer Kit Sensor Flex beserta solusi praktisnya. Panduan ini disusun untuk membantu siswa SMK dan instruktur dalam mengatasi masalah hardware maupun software secara mandiri.

---

## 🚦 1. LED / Traffic Light Kurang Terang (Redup)

| Detail Kendala | Penjelasan |
| :--- | :--- |
| **Gejala Fisik** | LED pada modul Traffic Light menyala sangat redup atau tidak maksimal. |
| **Penyebab Utama** | Level tegangan logika output (High) dari GPIO ESP32 hanya **3.3V**, sedangkan modul LED Traffic Light dirancang untuk bekerja optimal pada tegangan **5V**. Selain itu, arus maksimal dari pin GPIO ESP32 sangat terbatas (sekitar 12mA - 20mA). |

### 🛠️ Solusi Teknis:
1. **Gunakan Catu Daya Eksternal (External Power):** Hubungkan pin VCC modul LED ke sumber tegangan eksternal 5V (misalnya dari pin `5V` / `VIN` ESP32 yang terhubung ke adaptor USB stabil, bukan dari pin 3.3V).
2. **Gunakan Driver Transistor / Level Shifter:** Jika modul LED memerlukan arus besar, gunakan rangkaian driver transistor (misal tipe NPN seperti BC547) atau modul *logic level converter* 3.3V ke 5V agar lampu LED menyala dengan kecerahan maksimal tanpa membebani pin GPIO ESP32.

---

## 📺 2. Karakter Aeh / Kode "Cacing" pada Layar LCD I2C

| Detail Kendala | Penjelasan |
| :--- | :--- |
| **Gejala Fisik** | Layar LCD menampilkan karakter kotak-kotak hitam penuh, simbol aneh, atau teks acak yang tidak sesuai program. |
| **Penyebab Utama** | Terjadi gangguan data (*glitch* atau *noise*) pada jalur komunikasi I2C (SDA di GPIO 21 & SCL di GPIO 22). Hal ini sering dipicu oleh koneksi kabel jumper yang longgar, induksi arus dari motor servo, atau kegagalan inisialisasi LCD saat pertama kali dinyalakan. |

### 🛠️ Solusi Teknis:
1. **Reset Board ESP32:** Tekan tombol **EN / RST** pada ESP32 untuk mengulang inisialisasi bus I2C dari awal.
2. **Periksa Jalur Ground (GND):** Pastikan pin GND LCD terhubung sangat erat dan memiliki *common ground* (ground yang menyatu) dengan pin GND ESP32.
3. **Atur Trimpot Kontras:** Putar potensiometer berwarna biru di bagian belakang modul backpack I2C LCD menggunakan obeng minus kecil untuk menyesuaikan kontras karakter hingga teks terlihat jelas.

---

## ⚙️ 3. Motor Servo Mengalami Jitter (Bergetar Terus-Menerus)

| Detail Kendala | Penjelasan |
| :--- | :--- |
| **Gejala Fisik** | Horn (lengan) servo bergetar, mengeluarkan suara mendengung (*buzzing*), atau bergerak tidak stabil saat mencapai sudut tertentu. |
| **Penyebab Utama** | Fluktuasi tegangan catu daya (*voltage drop*). Ketika servo mulai berputar, ia menarik arus instan yang besar (*inrush current*). Jika catu daya tidak sanggup menyuplai arus secara cepat, tegangan VCC akan turun sesaat dan menyebabkan rangkaian kontroler di dalam servo mengalami *reset* atau kehilangan sinyal PWM yang stabil. |

### 🛠️ Solusi Teknis:
1. **Pasang Elco (Kapasitor Elektrolit):** Pasang kapasitor elektrolit (Elco) dengan ukuran **100 µF hingga 1000 µF (rating tegangan 10V/16V)** secara paralel pada jalur daya servo (kaki positif Elco ke pin VCC Servo, kaki negatif Elco ke pin GND Servo). Kapasitor ini berfungsi sebagai penyimpan cadangan energi lokal untuk menyuplai arus kejut servo.
2. **Gunakan Sumber Daya 5V Terpisah:** Jangan menyuplai daya servo langsung dari pin 3.3V ESP32. Hubungkan pin VCC servo ke pin `VIN`/`5V` ESP32 yang terhubung ke adaptor USB minimal 2 Amper, atau gunakan power supply 5V eksternal dengan menggabungkan jalur ground-nya.

---

## 📈 4. Pembacaan Sensor Flex Tidak Stabil / Nilai ADC Berfluktuasi

| Detail Kendala | Penjelasan |
| :--- | :--- |
| **Gejala Fisik** | Nilai pembacaan ADC sensor Flex di Serial Monitor atau Web Simulator melompat-lompat dengan rentang yang lebar (misalnya dari 2800 tiba-tiba melompat ke 2950 meskipun sensor sedang diam). |
| **Penyebab Utama** | Gangguan induksi elektromagnetik (*high-frequency noise*) dari lingkungan sekitar dan ketidakstabilan tegangan referensi ADC pada chip ESP32. |

### 🛠️ Solusi Teknis:
1. **Pasang Kapasitor Filter (Hardware):** Hubungkan kapasitor non-polar (keramik/milar) berukuran **100 nF (0.1 µF)** secara paralel di dekat pin analog input ESP32 (hubungkan langsung di antara pin GPIO pembacaan Flex dan GND). Kapasitor ini berfungsi sebagai low-pass filter fisik untuk meredam noise frekuensi tinggi.
2. **Gunakan Filtering Software (Penyaringan Nilai):** Terapkan teknik *Oversampling* atau *Moving Average Filter* di dalam kode program untuk merata-ratakan hasil pembacaan sebelum ditampilkan. 
   *(Catatan: Metode penyaringan rata-rata 20 sampel data ini sudah terintegrasi secara bawaan di dalam program Labsheet dan Firmware Kit agar data yang dikirim ke web selalu mulus).*
