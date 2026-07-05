# F. Troubleshooting Komponen Training Kit

Bagian ini berisi daftar kendala teknis (*troubleshooting*) yang sering ditemui selama perakitan, pemrograman, dan pengujian komponen pada Kit Trainer Sensor Flex. Untuk mempermudah proses identifikasi masalah, panduan berikut dikelompokkan berdasarkan komponen dengan format tabel yang memuat **Gejala**, **Kemungkinan Penyebab**, dan **Solusi**.

---

### 1. ESP32 DevKit V1 (Mikrokontroler)
ESP32 merupakan komponen utama pengendali seluruh sistem pada kit trainer ini. Apabila ESP32 mengalami gangguan, seluruh sistem tidak akan dapat berjalan.

**Tabel 1. Troubleshooting ESP32 DevKit V1**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Lampu power LED pada board ESP32 tidak menyala. | Kabel USB rusak, port USB komputer bermasalah, atau terjadi hubungan singkat (*short circuit*) pada board. | Ganti kabel USB, coba hubungkan ke port USB lain pada komputer, dan pastikan tidak ada kabel jumper yang salah pasang (saling bersentuhan). |
| Program gagal diupload (*Failed to connect to ESP32*). | Driver USB-to-UART (CP210x atau CH340) belum terinstal, port COM salah dipilih, atau board telat masuk ke mode bootloader. | Install driver chip UART yang sesuai di komputer, pastikan memilih port COM yang tepat di VS Code/PlatformIO, dan tekan serta tahan tombol **BOOT** pada ESP32 saat proses upload menampilkan status `Connecting...`. |
| Program berhasil terupload tetapi program tidak berjalan atau ESP32 mengalami *bootloop* (restart terus-menerus). | Arus dari port USB laptop/komputer kurang (khususnya port USB 2.0 yang hanya menyuplai 500mA), atau terdapat *short circuit* pada pin output. | Pindahkan koneksi ke port USB 3.0 (berwarna biru), gunakan adaptor charger HP eksternal (5V, 1A-2A), atau periksa kembali rangkaian kabel untuk memastikan tidak ada hubungan singkat. |

---

### 2. LCD I2C 16x4
LCD I2C 16x4 berfungsi sebagai antarmuka output untuk menampilkan status sistem, nilai flex, sudut, dan tegangan secara *real-time*.

**Tabel 2. Troubleshooting LCD I2C 16x4**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| LCD menyala (backlight aktif) tetapi tidak menampilkan karakter teks sama sekali. | Tingkat kontras layar terlalu rendah atau alamat alamat I2C pada kode program tidak tepat. | Putar trimpot berwarna biru di bagian belakang modul backpack I2C LCD menggunakan obeng minus kecil untuk mengatur kontras, dan jalankan program *I2C Scanner* untuk memastikan alamat I2C yang tepat (`0x27` atau `0x3F`). |
| Layar LCD sama sekali tidak menyala (mati total). | Jalur kabel daya VCC dan GND tidak terhubung dengan benar ke pin sumber daya. | Periksa kembali koneksi kabel jumper daya, pastikan pin VCC LCD terhubung ke pin `VIN` / `5V` ESP32, dan pin GND LCD terhubung ke GND ESP32. |
| Teks pada LCD muncul tetapi berupa karakter aneh, kotak-kotak hitam, atau kode acak. | Inisialisasi library gagal, library yang digunakan tidak sesuai, atau kabel SDA/SCL longgar. | Pastikan menggunakan library `LiquidCrystal_I2C` yang sesuai, periksa kekencangan kabel jumper pada pin SDA (GPIO 21) dan SCL (GPIO 22), lalu tekan tombol reset pada ESP32. |
| Layar LCD berkedip terus-menerus (*flickering*). | Catu daya tegangan ke modul LCD tidak stabil atau mengalami penurunan (*drop*). | Pasang kapasitor decoupling sebesar 100 µF di antara jalur VCC dan GND LCD untuk menstabilkan pasokan tegangan. |

---

### 3. Motor Servo
Motor servo digunakan untuk mensimulasikan gerakan mekanik (sudut) berdasarkan kelengkungan sensor flex.

**Tabel 3. Troubleshooting Motor Servo**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Motor servo tidak bergerak sama sekali. | Pin sinyal PWM salah dihubungkan, kabel sinyal putus, atau tegangan VCC tidak mencukupi. | Cek koneksi pin sinyal PWM (GPIO 18), pastikan kabel terhubung erat, dan pastikan servo mendapatkan suplai daya 5V yang cukup dari pin `VIN` ESP32 atau sumber daya eksternal. |
| Motor servo bergetar terus-menerus (*jitter*) atau tidak stabil di posisi tertentu. | Arus catu daya dari USB laptop kurang untuk menggerakkan motor servo. | Hubungkan kapasitor elektrolit (Elco) sebesar 100 µF - 1000 µF secara paralel pada jalur VCC dan GND servo, atau gunakan catu daya 5V eksternal yang terpisah dari laptop. |
| Servo bergerak tetapi sudut gerak tidak sesuai dengan sudut lekukan sensor flex. | Nilai pemetaan (*mapping*) sudut di dalam program tidak tepat. | Lakukan kalibrasi ulang nilai sudut batas bawah ($0^\circ$) dan batas atas ($180^\circ$) pada fungsi `map()` atau titik kalibrasi di program. |
| Motor servo berbunyi mendengung secara terus-menerus (*buzzing*). | Servo dipaksa berputar melebihi batas sudut mekanis fisiknya. | Batasi nilai sudut maksimal pada program (jangan melebihi batas fisik servo, biasanya maksimal $180^\circ$). |

---

### 4. Sensor Flex & Rangkaian Pembagi Tegangan
Sensor flex digunakan untuk mendeteksi tingkat kelengkungan jari/lengan dengan memanfaatkan perubahan nilai resistansi.

**Tabel 4. Troubleshooting Sensor Flex**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Pembacaan ADC sensor selalu bernilai 0 atau selalu bernilai 4095 (statis). | Terjadi kesalahan pemasangan kabel rangkaian pembagi tegangan (*voltage divider*) atau kabel input analog terputus. | Periksa kembali koneksi kabel rangkaian pembagi tegangan, pastikan kabel input analog terhubung ke pin ADC yang benar (GPIO 34 untuk Flex A, GPIO 35 untuk Flex B). |
| Rentang (*range*) perubahan nilai ADC sangat sempit atau tidak sensitif saat sensor ditekuk. | Nilai resistor pembagi tegangan (*pull-down*) terlalu kecil (di bawah 1k Ohm) atau terlalu besar (di atas 100k Ohm). | Gunakan resistor pembagi tegangan dengan nilai yang pas (sangat direkomendasikan **resistor 10k Ohm**) agar range perubahan tegangan analog yang dibaca ADC menjadi optimal dan sensitif. |
| Hasil pembacaan ADC melompat-lompat sangat tidak stabil (*noise* tinggi). | Adanya gangguan induksi elektromagnetik frekuensi tinggi atau fluktuasi tegangan referensi. | Pasang kapasitor filter non-polar 100 nF (0.1 µF) secara paralel di antara pin Analog Input (GPIO 34/35) dan GND, serta gunakan metode penyaringan software *moving average* pada program. |

---

### 5. Komunikasi Web Simulator (Serial & Wi-Fi)
Komunikasi ini digunakan untuk mengirimkan data sensor flex dari board ESP32 secara *real-time* ke Web Simulator.

**Tabel 5. Troubleshooting Komunikasi Web Simulator**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Web Simulator menampilkan pesan "Connection failed" saat mencoba terhubung via USB Serial. | Port COM sedang dikunci/dipakai oleh aplikasi lain (seperti Serial Monitor di Arduino IDE atau VS Code/PlatformIO). | Pastikan untuk menutup (*close/disconnect*) semua tab Serial Monitor pada editor pemrograman sebelum menekan tombol Connect di Web Simulator. |
| Web Simulator tidak dapat mendeteksi port USB ESP32 sama sekali. | Menggunakan kabel USB charger biasa yang tidak memiliki kabel data di dalamnya, atau driver USB UART belum terinstal. | Pastikan menggunakan kabel data USB asli yang mendukung transfer data, dan instal driver chip UART (CP2102/CH340) di komputer. |
| ESP32 tidak mendapatkan IP Address (indikator Wi-Fi gagal terhubung). | Nama SSID atau password Wi-Fi pada kode program salah, atau menggunakan Wi-Fi frekuensi 5 GHz. | Periksa kembali ketepatan penulisan SSID dan password Wi-Fi pada kode program, serta pastikan jaringan Wi-Fi menggunakan frekuensi 2.4 GHz. |
| Web Simulator gagal terhubung ke ESP32 meskipun terhubung ke Wi-Fi yang sama (mDNS `.local` error). | Jaringan Wi-Fi sekolah/kampus mengaktifkan fitur *AP Isolation* (memblokir komunikasi antar perangkat lokal), atau browser tidak mendukung resolusi mDNS. | Gunakan fitur Hotspot Seluler (Tethering) dari smartphone 2.4 GHz untuk menghubungkan laptop dan ESP32, atau bypass mDNS dengan memasukkan IP Address lokal ESP32 secara langsung di Web Simulator. |
