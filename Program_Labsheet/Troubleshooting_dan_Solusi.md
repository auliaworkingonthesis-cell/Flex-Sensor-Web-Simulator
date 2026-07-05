# F. Troubleshooting Komponen Training Kit

Bagian ini berisi daftar kendala teknis (*troubleshooting*) yang sering ditemui selama perakitan, pemrograman, dan pengujian komponen pada Kit Trainer Sensor Flex. Untuk mempermudah proses identifikasi masalah, panduan berikut dikelompokkan berdasarkan komponen dengan format tabel yang memuat **Gejala**, **Kemungkinan Penyebab**, dan **Solusi**.

---

### 1. ESP32 DevKit V1 (Mikrokontroler)
ESP32 merupakan komponen utama pengendali seluruh sistem pada kit trainer ini. Apabila ESP32 mengalami gangguan, seluruh sistem tidak akan dapat berjalan.

**Tabel 1. Troubleshooting ESP32 DevKit V1**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Lampu power LED pada board ESP32 tidak menyala. | Port USB komputer bermasalah, kabel USB rusak, atau tidak menyuplai daya ke board. | Coba ganti ke port USB lain pada komputer, atau ganti kabel USB dengan yang baru. |
| Board ESP32 tidak terbaca oleh komputer (tidak terdeteksi). | Koneksi kabel USB longgar, kabel USB hanya tipe charger, atau port serial belum terdaftar di sistem operasi. | Pasang ulang kabel USB secara erat dan lakukan pengecekan pada menu **Device Manager** untuk memastikan port serial terdeteksi oleh komputer. |
| Program tidak bisa di-upload ke board ESP32. | Driver USB-to-UART (CP2102/CH340) belum terinstal, port COM salah dipilih, atau terjadi error code tertentu saat upload. | Periksa *error code* yang muncul di konsol uploader untuk mencari akar masalah, pastikan driver UART terinstal, dan pastikan port COM yang aktif dipilih dengan benar di editor. |
| ESP32 mengalami restart sendiri secara terus-menerus (*Brownout*). | Tegangan input drop di bawah ambang batas minimal saat komponen lain (seperti motor servo) menarik arus listrik yang besar. | Cek tegangan input dan output ESP32 menggunakan multimeter, gunakan catu daya eksternal yang stabil untuk servo, atau gunakan program/konfigurasi *Brownout Detector* pada firmware untuk mengantisipasi kegagalan sistem. |

---

### 2. LCD I2C 16x4
LCD I2C 16x4 berfungsi sebagai antarmuka output untuk menampilkan status sistem, nilai flex, sudut, dan tegangan secara *real-time*.

**Tabel 2. Troubleshooting LCD I2C 16x4**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| LCD menyala (backlight aktif) tetapi tidak menampilkan karakter teks sama sekali. | Tingkat kontras layar terlalu rendah atau alamat alamat I2C pada kode program tidak tepat. | Putar trimpot berwarna biru di bagian belakang modul backpack I2C LCD menggunakan obeng minus kecil untuk mengatur kontras, dan jalankan program *I2C Scanner* untuk memastikan alamat I2C yang tepat (`0x27` atau `0x3F`). |
| Layar LCD sama sekali tidak menyala (mati total). | Jalur kabel daya VCC dan GND tidak terhubung dengan benar ke pin sumber daya. | Periksa kembali koneksi kabel jumper daya, pastikan pin VCC LCD terhubung ke pin `VIN` / `5V` ESP32, dan pin GND LCD terhubung ke GND ESP32. |
| Teks pada LCD muncul tetapi berupa karakter aneh, kotak-kotak hitam, atau kode acak. | Inisialisasi library gagal, library yang digunakan tidak sesuai, atau kabel SDA/SCL longgar. | Pastikan menggunakan library `LiquidCrystal_I2C` yang sesuai, periksa kekencangan kabel jumper pada pin SDA (GPIO 21) dan SCL (GPIO 22), lalu tekan tombol reset pada ESP32. |
| Layar LCD berkedip terus-menerus (*flickering*). | Catu daya tegangan ke modul LCD tidak stabil atau mengalami penurunan (*drop*). | Pasang kapasitor decoupling sebesar 100 µF di antara jalur VCC and GND LCD untuk menstabilkan pasokan tegangan. |

---

### 3. LED Traffic Light
Modul LED Traffic Light (Merah, Kuning, Hijau) digunakan sebagai indikator visual level tekukan sensor flex (aktif pada Labsheet 2).

**Tabel 3. Troubleshooting LED Traffic Light**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Lampu LED Traffic Light menyala sangat redup (kurang terang). | Modul menggunakan konfigurasi *Common Cathode* (hanya ada pin R, Y, G, GND tanpa pin VCC). LED dinyalakan langsung oleh logika HIGH dari GPIO ESP32 yang hanya bertegangan **3.3V**, sementara resistor pembatas arus bawaan modul dirancang untuk tegangan **5V**. | 1. Gunakan modul **Logic Level Converter (Level Shifter 3.3V ke 5V)** di antara pin GPIO ESP32 dan pin R, Y, G modul LED agar sinyal output dinaikkan menjadi 5V.<br>2. Atau, modifikasi nilai resistor pembatas arus SMD pada modul LED (ganti resistor bawaan 330/220 Ohm dengan nilai yang lebih kecil seperti 100/47 Ohm) agar arus yang mengalir pada tegangan 3.3V menjadi lebih besar dan LED menyala terang. |
| Lampu LED Traffic Light mati total meskipun nilai flex pada monitor berubah. | Kabel jumper terbalik antara pin data dan Ground, atau pin GPIO yang digunakan salah didefinisikan dalam kode program. | Periksa kembali kesesuaian pin data (GPIO 25, 26, 27), pastikan pin GND modul LED terhubung kuat ke GND ESP32, dan cek ketepatan nomor pin di kode program. |

---

### 4. Motor Servo
Motor servo digunakan untuk mensimulasikan gerakan mekanik (sudut) berdasarkan kelengkungan sensor flex.

**Tabel 4. Troubleshooting Motor Servo**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Motor servo tidak bergerak sama sekali. | Pin sinyal PWM salah dihubungkan, kabel sinyal putus, atau tegangan VCC tidak mencukupi. | Cek koneksi pin sinyal PWM (GPIO 18), pastikan kabel terhubung erat, dan pastikan servo mendapatkan suplai daya 5V yang cukup dari pin `VIN` ESP32 atau sumber daya eksternal. |
| Motor servo bergetar terus-menerus (*jitter*) atau tidak stabil di posisi tertentu. | Arus catu daya dari USB laptop kurang untuk menggerakkan motor servo. | Hubungkan kapasitor elektrolit (Elco) sebesar 100 µF - 1000 µF secara paralel pada jalur VCC dan GND servo, atau gunakan catu daya 5V eksternal yang terpisah dari laptop. |
| Servo bergerak tetapi sudut gerak tidak sesuai dengan sudut lekukan sensor flex. | Nilai pemetaan (*mapping*) sudut di dalam program tidak tepat. | Lakukan kalibrasi ulang nilai sudut batas bawah ($0^\circ$) dan batas atas ($180^\circ$) pada fungsi `map()` atau titik kalibrasi di program. |
| Motor servo berbunyi mendengung secara terus-menerus (*buzzing*). | Servo dipaksa berputar melebihi batas sudut mekanis fisiknya. | Batasi nilai sudut maksimal pada program (jangan melebihi batas fisik servo, biasanya maksimal $180^\circ$). |

---

### 5. Sensor Flex & Rangkaian Pembagi Tegangan
Sensor flex digunakan untuk mendeteksi tingkat kelengkungan jari/lengan dengan memanfaatkan perubahan nilai resistansi.

**Tabel 5. Troubleshooting Sensor Flex**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Pembacaan ADC sensor selalu bernilai 0 atau selalu bernilai 4095 (statis). | Terjadi kesalahan pemasangan kabel rangkaian pembagi tegangan (*voltage divider*) atau kabel input analog terputus. | Periksa kembali koneksi kabel rangkaian pembagi tegangan, pastikan kabel input analog terhubung ke pin ADC yang benar (GPIO 34 untuk Flex A, GPIO 35 untuk Flex B). |
| Rentang (*range*) perubahan nilai ADC sangat sempit atau tidak sensitif saat sensor ditekuk. | Nilai resistor pembagi tegangan (*pull-down*) terlalu kecil (di bawah 1k Ohm) atau terlalu besar (di atas 100k Ohm). | Gunakan resistor pembagi tegangan dengan nilai yang pas (sangat direkomendasikan **resistor 10k Ohm**) agar range perubahan tegangan analog yang dibaca ADC menjadi optimal dan sensitif. |
| Hasil pembacaan ADC melompat-lompat sangat tidak stabil (*noise* tinggi). | Adanya gangguan induksi elektromagnetik frekuensi tinggi atau fluktuasi tegangan referensi. | Pasang kapasitor filter non-polar 100 nF (0.1 µF) secara paralel di antara pin Analog Input (GPIO 34/35) dan GND, serta gunakan metode penyaringan software *moving average* pada program. |

---

### 6. Komunikasi Web Simulator (Serial & Wi-Fi)
Komunikasi ini digunakan untuk mengirimkan data sensor flex dari board ESP32 secara *real-time* ke Web Simulator.

**Tabel 6. Troubleshooting Komunikasi Web Simulator**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Web Simulator menampilkan pesan "Connection failed" saat mencoba terhubung via USB Serial. | Port COM sedang dikunci/dipakai oleh aplikasi lain (seperti Serial Monitor di Arduino IDE atau VS Code/PlatformIO). | Pastikan untuk menutup (*close/disconnect*) semua tab Serial Monitor pada editor pemrograman sebelum menekan tombol Connect di Web Simulator. |
| Web Simulator tidak dapat mendeteksi port USB ESP32 sama sekali. | Menggunakan kabel USB charger biasa yang tidak memiliki kabel data di dalamnya, atau driver USB UART belum terinstal. | Pastikan menggunakan kabel data USB asli yang mendukung transfer data, dan instal driver chip UART (CP2102/CH340) di komputer. |
| ESP32 tidak mendapatkan IP Address (indikator Wi-Fi gagal terhubung). | Nama SSID atau password Wi-Fi pada kode program salah, atau menggunakan Wi-Fi frekuensi 5 GHz. | Periksa kembali ketepatan penulisan SSID dan password Wi-Fi pada kode program, serta pastikan jaringan Wi-Fi menggunakan frekuensi 2.4 GHz. |
| Web Simulator gagal terhubung ke ESP32 meskipun terhubung ke Wi-Fi yang sama (mDNS `.local` error). | Jaringan Wi-Fi sekolah/kampus mengaktifkan fitur *AP Isolation* (memblokir komunikasi antar perangkat lokal), atau browser tidak mendukung resolusi mDNS. | Gunakan fitur Hotspot Seluler (Tethering) dari smartphone 2.4 GHz untuk menghubungkan laptop and ESP32, atau bypass mDNS dengan memasukkan IP Address lokal ESP32 secara langsung di Web Simulator. |

---

### 7. Penggunaan Fungsi Blocking delay() pada Program C++
Pemrograman mikrokontroler dengan fungsi tunda waktu statis dapat menghambat jalannya multitasking program.

**Tabel 7. Troubleshooting Penggunaan Fungsi delay()**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Pengiriman data sensor ke Web Simulator terlambat (*delay* tinggi), tampilan LCD ter-update sangat lambat, atau koneksi Wi-Fi/Serial sering terputus secara acak. | Penggunaan fungsi `delay()` bawaan Arduino pada kode program ESP32 yang bersifat *blocking* (menghentikan seluruh kerja CPU selama beberapa milidetik), sehingga mikrokontroler tidak sempat mengeksekusi program komunikasi data secara *real-time*. | Ganti seluruh pemanggilan fungsi `delay()` yang bernilai besar dengan metode pencatatan waktu non-blocking menggunakan fungsi **`millis()`** (seperti yang dicontohkan di program Labsheet terbaru) agar ESP32 dapat membaca sensor dan mengirim data secara multitasking tanpa berhenti. |

---

### 8. Fitur Feedback Suara (TTS & Upload Audio)
Fitur ini mensimulasikan suara asisten virtual (dalam bahasa Indonesia) atau nada kustom saat lekukan sensor flex berada dalam rentang tertentu.

**Tabel 8. Troubleshooting Fitur Suara**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Suara tidak terdengar sama sekali saat nilai flex masuk ke rentang yang diatur. | Fitur suara belum diaktifkan di web, volume perangkat mati, atau browser memblokir fitur *Speech Synthesis* (kebijakan keamanan *Autoplay* browser). | Klik tombol **Enable Voice** pada halaman Web Simulator, periksa volume perangkat, dan lakukan klik sembarang pada halaman web terlebih dahulu untuk memberikan izin akses audio/suara (*user interaction*). |
| File audio kustom (.mp3/.wav) yang diunggah tidak berbunyi atau gagal diputar. | Format kompresi audio tidak didukung oleh browser, ukuran file melebihi kapasitas memori, atau file rusak. | Pastikan mengunggah file audio berformat standar MP3/WAV berukuran kecil (di bawah 1MB) yang telah diuji dapat diputar di media player laptop. |
| Suara TTS (Text-to-Speech) terdengar terlalu lambat atau terbata-bata. | Browser mengalami *lagging* atau beban memori komputasi laptop terlalu penuh. | Tutup tab browser lain yang tidak terpakai, bersihkan cache, atau gunakan browser modern yang ringan seperti Google Chrome. |

---

### 9. Tampilan Animasi 3D Lengan Gripper / SCARA
Layar 3D WebGL simulator menampilkan representasi gerakan mekanis 3D lengan gripper/SCARA secara *real-time*.

**Tabel 9. Troubleshooting Animasi 3D**
| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| Kanvas area model 3D lengan gripper berwarna hitam kosong (blank) atau tidak muncul sama sekali. | Browser tidak mendukung WebGL, atau fitur *Hardware Acceleration* dinonaktifkan pada setelan browser. | Aktifkan fitur *Hardware Acceleration* di setelan browser (Settings -> System -> Use graphics acceleration when available) lalu restart browser Anda. |
| Gerakan lengan 3D patah-patah (*lagging* / frame rate sangat rendah). | Kartu grafis (GPU) laptop kewalahan memproses rendering Three.js secara waktu nyata. | Update driver kartu grafis laptop, gunakan browser berbasis Chromium (Chrome/Edge) versi terbaru, dan pastikan laptop tidak berada dalam mode hemat daya baterai (*battery saver*). |
