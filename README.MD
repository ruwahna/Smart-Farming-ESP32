# Smart Farming ESP32

Proyek **Smart Farming ESP32** adalah sistem monitoring dan penyiraman otomatis berbasis **ESP32**, **sensor DHT22**, **sensor kelembapan tanah**, **sensor cahaya (LDR)**, dan **relay pompa**.  
Sistem ini membaca kondisi lingkungan, menampilkan data ke Serial Monitor, dan menyalakan pompa saat tanah terlalu kering.

## Fitur

- Monitoring suhu dan kelembapan udara menggunakan **DHT22**
- Monitoring kelembapan tanah menggunakan **sensor soil moisture**
- Monitoring intensitas cahaya menggunakan **LDR**
- Kontrol pompa air otomatis melalui **relay**
- Output status sensor ke **Serial Monitor**
- Siap dikembangkan untuk notifikasi **Telegram**

## Teknologi

- **ESP32**
- **Arduino Framework**
- **PlatformIO**
- Library:
  - `DHT sensor library`
  - `Adafruit Unified Sensor`

## Struktur File

- `src/main.cpp`  
  Berisi logika utama pembacaan sensor dan kontrol relay.

- `src/rahasia.h`  
  Berisi konfigurasi rahasia seperti token bot Telegram dan chat ID.

- `platformio.ini`  
  Berisi konfigurasi board dan dependensi library.

## Koneksi Pin

| Komponen | Pin ESP32 | Keterangan |
|---|---:|---|
| Sensor tanah | D15 | Input analog |
| LDR | D4 | Input analog |
| DHT22 | D19 | Data sensor suhu/kelembapan |
| Relay pompa | D5 | Output kontrol pompa |

## Kalibrasi Sensor Tanah

Nilai sensor tanah harus dikalibrasi agar hasil persentase akurat.

Di `src/main.cpp` terdapat:

- `NILAI_KERING = 3200`
- `NILAI_BASAH = 1500`

### Cara kalibrasi
1. Baca nilai sensor saat sensor **kering**.
2. Baca nilai sensor saat sensor **terendam air / sangat basah**.
3. Sesuaikan nilai pada kode jika hasil pembacaan berbeda.

## Cara Kerja Sistem

1. ESP32 membaca:
   - suhu udara
   - kelembapan udara
   - nilai sensor tanah
   - nilai cahaya
2. Nilai tanah dikonversi menjadi persentase.
3. Jika kelembapan tanah di bawah **30%**, pompa akan menyala.
4. Jika kelembapan tanah di atas **70%**, pompa dimatikan.
5. Semua data ditampilkan ke Serial Monitor.

## Konfigurasi Telegram

File `src/rahasia.h` digunakan untuk menyimpan data sensitif:

```cpp
#define TELEGRAM_BOT_TOKEN "BOT_TOKEN_HERE"
#define TELEGRAM_CHAT_ID "CHAT_ID_HERE"
```

Ganti dengan:
- token bot Telegram Anda
- chat ID tujuan

> Catatan: fungsi pengiriman Telegram sudah disiapkan di kode, tetapi belum dipanggil di `loop()`.

## Instalasi

### 1. Install PlatformIO
Gunakan **Visual Studio Code** dengan ekstensi **PlatformIO**.

### 2. Buka proyek
Buka folder repository ini di VS Code.

### 3. Install dependensi
PlatformIO akan mengunduh library sesuai isi `platformio.ini`:

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`

### 4. Konfigurasi rahasia
Edit file berikut:

`src/rahasia.h`

Isi token bot Telegram dan chat ID Anda.

### 5. Upload ke ESP32
Gunakan perintah PlatformIO:

```bash
pio run
pio run --target upload
pio device monitor
```

## Output Serial Monitor

Contoh output:

```text
SUHU: 28.50°C | UDARA: 70.00% | CAHAYA: 1800 | TANAH: 45%
```

## Catatan Penting

- Relay pada kode menggunakan logika **active low**:
  - `LOW` = pompa menyala
  - `HIGH` = pompa mati
- Pastikan catu daya pompa sesuai kebutuhan perangkat.
- Pastikan sensor tanah, DHT22, dan relay terhubung dengan benar.
- Nilai analog dapat berbeda tergantung modul sensor yang digunakan.

## Troubleshooting

### Data sensor tidak muncul
- Cek wiring sensor
- Pastikan pin sesuai dengan kode
- Pastikan sensor mendapat tegangan yang benar

### Relay tidak bekerja
- Pastikan modul relay mendukung **active low**
- Cek sumber daya pompa
- Pastikan pin `D5` terhubung dengan benar

### Nilai tanah tidak akurat
- Lakukan kalibrasi ulang `NILAI_KERING` dan `NILAI_BASAH`

### DHT22 membaca `nan`
- Cek koneksi data dan resistor pull-up jika diperlukan
- Pastikan sensor tidak rusak

## Pengembangan Lanjutan

Beberapa pengembangan yang bisa ditambahkan:

- Notifikasi Telegram saat tanah kering
- Tampilan data pada dashboard web
- Logging data ke database
- Kontrol manual pompa dari aplikasi mobile
- Mode auto/manual untuk penyiraman
