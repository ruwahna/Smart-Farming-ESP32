# Smart Farming ESP32 (IoT Integration)

Proyek **Smart Farming ESP32** adalah sistem pemantauan (monitoring) lingkungan dan penyiraman tanaman otomatis berbasis IoT. Proyek ini mengintegrasikan **ESP32** dengan berbagai sensor, modul relay pompa air, **Web API Server (Dashboard)**, serta notifikasi interaktif via **Telegram Bot**.

Sistem ini bersifat non-blocking (multitasking menggunakan `millis()`) sehingga semua sensor, pengiriman data, sinkronisasi pengaturan, dan proteksi pompa dapat berjalan secara paralel tanpa hambatan *delay*.

---

## 🚀 Fitur Utama

- **Monitoring Lingkungan Real-Time**: 
  - Suhu & kelembapan udara menggunakan sensor **DHT22**.
  - Kelembapan tanah (*soil moisture*) menggunakan sensor analog.
  - Intensitas cahaya menggunakan sensor **LDR**.
- **Otomatisasi Penyiraman Pintar**:
  - Pompa menyala otomatis jika kelembapan tanah berada di bawah batas (*threshold*).
  - Pompa memiliki masa jeda (*cooldown*) setelah penyiraman untuk membiarkan air meresap ke tanah.
- **Integrasi Web API Server (Dashboard)**:
  - Mengirimkan data sensor secara periodik untuk riwayat dan grafik web.
  - Mengirimkan data detak jantung sistem (*heartbeat*) sebagai tanda perangkat sedang online.
  - Menyimpan log aktivitas penyiraman otomatis ke server cloud.
- **Sinkronisasi Pengaturan Dinamis**:
  - Mengunduh konfigurasi batas kelembaban, durasi pompa, jeda pompa, dan status fitur langsung dari Web Dashboard secara dinamis setiap 30 detik tanpa perlu flash ulang ESP32.
- **Kontrol Remote Web (Command Center)**:
  - Menerima perintah penyiraman manual jarak jauh dari dashboard web.
- **Notifikasi Telegram Bot Aktif**:
  - Pesan sambutan saat alat pertama kali aktif.
  - Laporan rutin kondisi sensor setiap 2 menit.
  - Laporan instan ketika pompa menyala (AKTIF) atau mati (STANDBY).
  - Peringatan keamanan jika pompa menyala melebihi batas waktu maksimal (*timeout safety*).

---

## 🔌 Koneksi Pinout ESP32

| Komponen Fisik | Pin ESP32 | Jenis Pin | Keterangan |
|---|:---:|:---:|---|
| **Sensor DHT22 (Suhu/Hum)** | **GPIO 16** | Digital Input | Memerlukan resistor pull-up 4.7k - 10k antara VCC & Data |
| **Sensor Kelembapan Tanah** | **GPIO 32** | ADC Analog Input | Pembacaan analog 12-bit (0 - 4095) |
| **Sensor Cahaya (LDR)** | **GPIO 35** | ADC Analog Input | Pembacaan intensitas cahaya analog |
| **Modul Relay Pompa** | **GPIO 5** | Digital Output | Menggunakan logika *Active Low* (LOW = Menyala, HIGH = Mati) |

---

## 📐 Kalibrasi Sensor Tanah

Nilai analog ADC ESP32 berkisar antara **0 - 4095**. Kalibrasi dilakukan agar konversi ke nilai persentase (%) akurat.

Di dalam `src/main.cpp`:
- `NILAI_KERING = 3800` (Nilai ADC saat sensor kering di udara bebas)
- `NILAI_BASAH  = 1800` (Nilai ADC saat sensor basah terendam air)

Jika pembacaan sensor Anda berbeda, Anda bisa menyesuaikan kedua konstanta ini di bagian atas file `src/main.cpp`.

---

## 🛠️ Arsitektur & Interval Waktu (Millis)

Sistem bekerja secara multitasking non-blocking dengan interval berikut:
- **Membaca Sensor**: Tiap **2 detik** (`SENSOR_INTERVAL = 2000`)
- **Kirim Data Sensor**: Tiap **10 detik** (`API_INTERVAL = 10000`)
- **Cek Perintah Web (Manual)**: Tiap **30 detik** (`COMMAND_POLL_INTERVAL = 30000`)
- **Cek Pengaturan Web (Dashboard)**: Tiap **30 detik** (`SETTINGS_CHECK_INTERVAL = 30000`)
- **Kirim Laporan Telegram**: Tiap **2 menit** (`TELEGRAM_INTERVAL = 120000`) atau instan jika status pompa berubah.

---

## 📂 Struktur File Proyek

- **`src/main.cpp`**  
  Logika utama sistem: manajemen Wi-Fi, pembacaan sensor, komunikasi API klien, logika otomatisasi pompa, dan pengiriman notifikasi Telegram.
- **`src/rahasia.h`**  
  File konfigurasi kredensial rahasia (tidak di-upload ke GitHub publik):
  - Daftar SSID & Password Wi-Fi (`wifiList`).
  - Token Bot Telegram (`TELEGRAM_BOT_TOKEN`).
  - Chat ID Telegram Penerima (`TELEGRAM_CHAT_ID`).
  - Alamat API Server Cloud (`API_SERVER`).
- **`platformio.ini`**  
  Konfigurasi board ESP32 (`esp32dev`) dan daftar dependensi library C++ Arduino.

---

## 📦 Dependensi Library (platformio.ini)

Library berikut akan otomatis diunduh saat melakukan build proyek:
- `Adafruit DHT Sensor Library`
- `Adafruit Unified Sensor Library`
- `UniversalTelegramBot`
- `ArduinoJson`

---

## ⚙️ Cara Instalasi & Penggunaan

### 1. Persiapan Software
- Pasang **VS Code** beserta ekstensi **PlatformIO IDE**.

### 2. Konfigurasi Kredensial
Edit file **`src/rahasia.h`**:
- Masukkan daftar jaringan Wi-Fi Anda pada `wifiList`.
- Isi kredensial Telegram Bot Anda:
  ```cpp
  #define TELEGRAM_BOT_TOKEN "TOKEN_BOT_TELEGRAM_ANDA"
  #define TELEGRAM_CHAT_ID "ID_CHAT_TELEGRAM_ANDA"
  #define API_SERVER "https://apiapimonitoringplant.vigian-ai.my.id"
  ```

### 3. Upload Program ke ESP32
Sambungkan ESP32 Anda ke komputer via USB, lalu jalankan perintah PlatformIO:
- **Build**: Klik ikon `Checkmark` (✓) di bagian bawah VS Code, atau ketik `pio run` di terminal.
- **Upload**: Klik ikon `Arrow Right` (→) di bagian bawah VS Code, atau ketik `pio run --target upload`.
- **Serial Monitor**: Klik ikon `Plug` di bagian bawah VS Code, atau ketik `pio device monitor`.

---

## ⚠️ Troubleshooting & Keamanan Pompa

- **Brownout Detector/ESP32 Restart Berulang**:
  Saat pompa menyala, dinamo pompa menarik arus listrik yang sangat besar secara tiba-tiba. Jika Anda memberikan daya pompa langsung dari pin 5V ESP32, ESP32 akan mengalami *crash/restart*. **Solusi**: Pisahkan catu daya (power supply) antara ESP32 dan relay/pompa. Gunakan adaptor eksternal 5V/12V (sesuai voltase pompa) untuk menyuplai daya pompa.
- **Pesan Telegram "Pompa menyala terlalu lama"**:
  Ini adalah fitur pengaman otomatis. Jika pompa menyala melampaui `pump_max_duration` (default 20 detik) dan tanah belum kunjung basah, sistem akan mematikan paksa pompa dan mengirim peringatan ke Telegram demi mencegah kebanjiran air atau dinamo pompa terbakar karena kehabisan air.
