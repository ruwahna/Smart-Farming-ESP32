# Laporan Proyek: Smart Farming ESP32

## BAB I: PENDAHULUAN

### 1.1 Latar Belakang
Indonesia merupakan negara agraris dimana sektor pertanian memegang peranan yang sangat penting bagi ketahanan pangan dan perekonomian. Namun, seiring dengan perubahan iklim global yang menyebabkan cuaca semakin tidak menentu, para penggiat tanaman seringkali kesulitan dalam menjaga kondisi lingkungan yang ideal untuk pertumbuhan tanaman. Salah satu aspek paling krusial dalam perawatan tanaman adalah proses penyiraman atau irigasi.

Hingga saat ini, sebagian besar proses penyiraman tanaman masih dilakukan secara konvensional atau manual. Pendekatan manual ini memiliki banyak kelemahan, di antaranya adalah kurang efisien dari segi waktu dan tenaga, serta tingginya potensi *human error*. Seringkali tanaman mendapatkan pasokan air yang berlebihan sehingga menyebabkan pembusukan akar, atau sebaliknya, tanaman kekurangan air karena kelalaian atau jadwal penyiraman yang tidak teratur. Selain itu, penggunaan air secara berlebihan juga berdampak pada pemborosan sumber daya air.

Di era digital saat ini, perkembangan teknologi *Internet of Things* (IoT) dan mikrokontroler menawarkan solusi inovatif untuk mengatasi permasalahan tersebut. Konsep *Smart Farming* atau pertanian pintar memungkinkan kita untuk memonitor dan mengontrol lahan pertanian secara otomatis. Dengan memanfaatkan berbagai sensor, sistem dapat membaca kondisi fisik lingkungan secara *real-time* dan mengambil keputusan secara mandiri.

Berdasarkan permasalahan tersebut, proyek ini bertujuan untuk mengembangkan sebuah sistem *Smart Farming* berbasis mikrokontroler ESP32. Sistem ini dirancang untuk dapat memonitor kondisi vital lingkungan, yaitu suhu udara, kelembapan udara, kelembapan tanah, serta intensitas cahaya matahari. Fokus utama dari alat ini adalah mengotomatisasi penyiraman air secara presisi yang dipicu secara langsung oleh data tingkat kekeringan dari sensor kelembapan tanah. Dengan demikian, diharapkan tanaman selalu mendapatkan pasokan air yang ideal sesuai kebutuhannya, pertumbuhan tanaman menjadi lebih optimal, serta penggunaan air dan tenaga menjadi jauh lebih efisien.

### 1.2 Rumusan Masalah
1. Bagaimana merancang sistem monitoring lingkungan (suhu udara, kelembapan udara, kelembapan tanah, dan intensitas cahaya) secara *real-time* menggunakan ESP32?
2. Bagaimana merancang sistem penyiraman otomatis yang dapat menghidupkan dan mematikan pompa air berdasarkan persentase kelembapan tanah?
3. Bagaimana mengkalibrasi pembacaan nilai analog dari sensor kelembapan tanah agar persentase yang dihitung akurat?

### 1.3 Tujuan Proyek
1. Membuat purwarupa sistem *Smart Farming* berbasis ESP32 dan berbagai sensor lingkungan (DHT22, Soil Moisture, LDR).
2. Membangun sistem aktuasi yang dapat mengontrol modul relay dan pompa air secara otomatis sesuai batas ambang (*threshold*) kelembapan tanah.
3. Menampilkan status lingkungan dan aktuator secara *real-time* melalui Serial Monitor.

### 1.4 Manfaat Proyek
1. Membantu mengotomatisasi proses penyiraman tanaman, sehingga menghemat waktu dan tenaga.
2. Meningkatkan efisiensi penggunaan air karena penyiraman hanya dilakukan saat tanah benar-benar kering.
3. Menjadi dasar pijakan (*prototype*) untuk pengembangan sistem pertanian pintar dengan fitur IoT yang lebih luas (seperti notifikasi ke Telegram atau aplikasi web).

---

## BAB II: LANDASAN TEORI

### 2.1 Mikrokontroler ESP32
ESP32 adalah mikrokontroler berbiaya rendah dengan konsumsi daya rendah yang dikembangkan oleh Espressif Systems. Modul ini memiliki mikrokontroler dual-core lengkap dengan dukungan Wi-Fi dan Bluetooth built-in, serta pin I/O analog dan digital yang sangat cukup untuk membaca berbagai sensor.

### 2.2 Sensor DHT22
DHT22 adalah sensor digital dasar yang sangat baik untuk mengukur suhu dan kelembapan udara lingkungan. Sensor ini menggunakan termistor kapasitif dan sensor suhu untuk mengukur udara di sekitarnya dan memuntahkan data digital pada pin datanya.

### 2.3 Sensor Kelembapan Tanah (Soil Moisture)
Sensor ini digunakan untuk mengukur jumlah air di dalam tanah. Cara kerjanya berdasarkan resistansi atau kapasitansi antara dua probe; semakin banyak air di dalam tanah, semakin baik konduktivitasnya (nilai analog rendah jika menggunakan tipe resistif tertentu).

### 2.4 Sensor Cahaya (LDR)
LDR (Light Dependent Resistor) adalah komponen elektronika yang nilai hambatannya berubah-ubah bergantung pada intensitas cahaya yang diterimanya.

### 2.5 Modul Relay dan Pompa Air
Relay adalah saklar elektronik yang dioperasikan dengan arus searah (DC) kecil, tetapi mampu mengontrol arus yang besar. Dalam proyek ini, relay berfungsi untuk memutus dan menyambungkan arus ke pompa air DC.

---

## BAB III: PERANCANGAN SISTEM

### 3.1 Blok Diagram Sistem
Berikut adalah blok diagram yang menggambarkan hubungan antara mikrokontroler, input (sensor), dan output (aktuator).

![Blok Diagram Sistem](https://mermaid.ink/img/eyJjb2RlIjogImdyYXBoIFREXG4gICAgQVtTZW5zb3IgREhUMjJdIC0tPnxTdWh1ICYgS2VsZW1iYXBhbiBVZGFyYXwgQyhFU1AzMilcbiAgICBCW1NlbnNvciBTb2lsIE1vaXN0dXJlXSAtLT58TmlsYWkgQW5hbG9nIFRhbmFofCBDKEVTUDMyKVxuICAgIERbU2Vuc29yIExEUl0gLS0+fE5pbGFpIEFuYWxvZyBDYWhheWF8IEMoRVNQMzIpXG4gICAgQyAtLT58U2lueWFsIERpZ2l0YWwgTE9XL0hJR0h8IEVbTW9kdWwgUmVsYXldXG4gICAgRSAtLT58T04gLyBPRkZ8IEZbUG9tcGEgQWlyIERDXVxuICAgIEMgLS0+fERhdGEgVGVrc3wgR1tTZXJpYWwgTW9uaXRvcl0iLCAibWVybWFpZCI6IHsidGhlbWUiOiAiZGVmYXVsdCJ9fQ==)

### 3.2 Perancangan Perangkat Keras (Hardware)
Koneksi komponen perangkat keras ke pin ESP32 diatur sebagai berikut:

| Komponen | Pin ESP32 | Keterangan / Fungsi |
| :--- | :---: | :--- |
| Sensor Tanah (Soil Moisture) | **D15** | Input analog dari kelembapan tanah |
| Sensor Cahaya (LDR) | **D4** | Input analog dari intensitas cahaya |
| Sensor DHT22 | **D19** | Komunikasi data digital (One-Wire) |
| Modul Relay Pompa | **D5** | Output digital untuk kontrol pompa (Active Low) |

### 3.3 Perancangan Perangkat Lunak (Flowchart)
Sistem ini menggunakan bahasa C++ (Arduino Framework). Berikut adalah alur logika (Flowchart) utama dari program:

![Flowchart Sistem](https://mermaid.ink/img/eyJjb2RlIjogImZsb3djaGFydCBURFxuICAgIFN0YXJ0KFtNdWxhaSBQcm9ncmFtXSkgLS0+IEluaXRbSW5pc2lhbGlzYXNpIFBpbiwgU2Vuc29yICYgU2VyaWFsXVxuICAgIEluaXQgLS0+IFJlYWRbQmFjYSBEYXRhOiBTdWh1LCBVZGFyYSwgQ2FoYXlhLCBBbmFsb2cgVGFuYWhdXG4gICAgUmVhZCAtLT4gQ29udmVydFtLYWxpYnJhc2kgJiBLb252ZXJzaSBOaWxhaSBUYW5haCBrZSBQZXJzZW50YXNlICVdXG4gICAgQ29udmVydCAtLT4gQ2Vre1RhbmFoIDw9IDMwJT99XG4gICAgQ2VrIC0tIFlhIEtlcmluZyAtLT4gTnlhbGFbTnlhbGFrYW4gUG9tcGEgPGJyLz5SZWxheSA9IExPV11cbiAgICBDZWsgLS0gVGlkYWsgLS0+IENlazJ7VGFuYWggPj0gNzAlP31cbiAgICBDZWsyIC0tIFlhIEJhc2FoIC0tPiBNYXRpW01hdGlrYW4gUG9tcGEgPGJyLz5SZWxheSA9IEhJR0hdXG4gICAgQ2VrMiAtLSBUaWRhayAtLT4gVGFoYW5bUGVydGFoYW5rYW4gU3RhdHVzIFBvbXBhIFNhYXQgSW5pXVxuICAgIE55YWxhIC0tPiBQcmludFtUYW1waWxrYW4gU2VtdWEgRGF0YSBrZSBTZXJpYWwgTW9uaXRvcl1cbiAgICBNYXRpIC0tPiBQcmludFxuICAgIFRhaGFuIC0tPiBQcmludFxuICAgIFByaW50IC0tPiBEZWxheVtUdW5nZ3UgMiBEZXRpa11cbiAgICBEZWxheSAtLT4gUmVhZCIsICJtZXJtYWlkIjogeyJ0aGVtZSI6ICJkZWZhdWx0In19)

**Kalibrasi Sensor Tanah:**
Untuk mendapatkan persentase tanah yang akurat (0 - 100%), nilai ADC pada saat kering (di udara terbuka) dicatat sebagai `NILAI_KERING = 3200`, dan saat terendam air dicatat sebagai `NILAI_BASAH = 1500`.

---

## BAB IV: IMPLEMENTASI DAN PENGUJIAN

### 4.1 Implementasi Kode Program
File utama berada di `src/main.cpp`. Logika penyiraman diimplementasikan dengan kode berikut:
```cpp
if (persenTanah <= 30) {
    digitalWrite(RELAY_PIN, LOW); // Pompa menyala
} else if (persenTanah >= 70) {
    digitalWrite(RELAY_PIN, HIGH); // Pompa mati
}
```
Untuk mengamankan rahasia jika dikembangkan dengan Telegram, kredensial dipisahkan di file `src/rahasia.h`.

### 4.2 Hasil Pengujian
Berdasarkan pengujian melalui alat dan PlatformIO Serial Monitor, didapatkan hasil:
1. **Pengujian Sensor:** Saat sistem dijalankan, Serial Monitor menampilkan _output_ yang stabil seperti:
   `SUHU: 28.50°C | UDARA: 70.00% | CAHAYA: 1800 | TANAH: 45%`
2. **Pengujian Penyiraman Otomatis:** 
   - Ketika probe tanah dicabut (kondisi kering, tanah < 30%), relay indikator menyala (pompa ON).
   - Ketika probe dimasukkan ke tanah basah / air (tanah > 70%), relay indikator mati (pompa OFF).
3. **Logika Relay:** Telah dikonfirmasi bahwa relay berjenis _active low_, sehingga pemberian tegangan LOW dari ESP32 justru mengaktifkan saklar relay.

### 4.3 Pembahasan
Berdasarkan implementasi dan hasil pengujian, berikut adalah evaluasi yang menjawab rumusan masalah pada awal proyek:
1. **Pemantauan Lingkungan secara *Real-Time*:** Sistem telah sukses dirancang menggunakan ESP32 yang terhubung ke sensor DHT22, LDR, dan Soil Moisture. Sistem mampu membaca dan mengirim data (suhu, kelembapan udara, intensitas cahaya, dan kondisi tanah) ke *Serial Monitor* secara terus-menerus dengan stabil, membuktikan bahwa kapabilitas monitoring *real-time* berhasil dicapai.
2. **Otomatisasi Penyiraman Pompa Air:** Sistem penyiraman otomatis bekerja secara presisi berdasarkan persentase kelembapan tanah. Saat persentase tanah jatuh di bawah batas ambang 30%, ESP32 berhasil memberikan sinyal LOW ke relay untuk menyalakan pompa. Sebaliknya, saat mencapai 70% atau lebih, pompa otomatis dimatikan. Hal ini menyelesaikan masalah penyiraman manual yang kurang efisien.
3. **Akurasi Kalibrasi Sensor Tanah:** Penentuan nilai kalibrasi sensor analog (`NILAI_KERING = 3200` dan `NILAI_BASAH = 1500`) yang kemudian dikonversi menjadi persentase 0-100% menggunakan fungsi pemetaan (`map()`) terbukti sangat krusial. Kalibrasi yang tepat ini berhasil membuat logika aktuasi pompa menjadi sangat akurat terhadap kondisi kelembapan tanah yang sebenarnya di lapangan.

---

## BAB V: PENUTUP

### 5.1 Kesimpulan
Sistem *Smart Farming* ESP32 yang dirancang telah beroperasi dengan baik. ESP32 secara sukses membaca data lingkungan dan mampu mengambil keputusan secara otonom untuk menyalakan atau mematikan pompa air berdasarkan kondisi kelembapan tanah, menjaga agar tanah tetap pada rentang kelembapan optimal (di atas 30% dan tidak berlebihan hingga > 70%).

### 5.2 Saran
1. Integrasi bot Telegram pada fungsi pengiriman pesan yang sudah ada kodenya dapat segera diaktifkan agar pengguna mendapat notifikasi jarak jauh.
2. Penambahan fungsi pembacaan arus untuk memastikan pompa air benar-benar berfungsi (tidak macet).
3. Mengembangkan antar muka pengguna (UI) melalui web server lokal atau _cloud_ (misal: platform Blynk atau Thingspeak).
