#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "rahasia.h"

// --- 1. PINOUT (PASTIKAN KABEL SESUAI INI) ---
#define PIN_TANAH  32  // Pindahkan kabel sensor tanah ke GPIO 32 (ADC1)
#define PIN_LDR    33  // Pindahkan kabel LDR ke GPIO 33 (ADC1)
#define PIN_DHT    16  // Kabel data DHT ke D16 (disamakan dengan komentar)
#define PIN_RELAY  5   // Kabel Relay ke D5

// --- 2. SETUP SENSOR ---
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// --- 3. ANGKA KALIBRASI (SESUAIKAN DENGAN ALATMU) ---
const int KERING = 3800; // Angka saat sensor di udara (Tinggi = Kering)
const int BASAH  = 1800; // Angka saat sensor di tanah basah (Rendah = Basah)
// --- PENGATURAN WIFI ---
WiFiClientSecure client;
HTTPClient https;

//-- PENGATURAN TELEGRAM ---
String botToken = TELEGRAM_BOT_TOKEN;
String chatId = TELEGRAM_CHAT_ID;

// Variable untuk tracking notifikasi
unsigned long lastTelegramTime = 0;
const unsigned long TELEGRAM_INTERVAL = 60000; // Kirim Telegram setiap 1 menit (60.000 milidetik)
bool pompaStatus = false;

// Fungsi untuk mengirim pesan ke Telegram
void sendTelegram(String pesan) {
  if (WiFi.status() == WL_CONNECTED) {

    // Ubah enter dan spasi agar terbaca oleh URL HTTP GET
    pesan.replace(" ", "%20");
    pesan.replace("\n", "%0A");

    // Rakit URL API Telegram
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + pesan;

    https.begin(client, url);
    int httpCode = https.GET();

    if (httpCode > 0) {
      Serial.printf("[Telegram] Terkirim! Kode: %d\n", httpCode);
    } else {
      Serial.printf("[Telegram] Error HTTP: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.println("[Telegram] Gagal: WiFi Terputus!");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(PIN_RELAY, OUTPUT);
  
  // Set Awal: Pastikan Pompa MATI
  // Jika pompa malah NYALA saat dicolok, ganti HIGH di bawah jadi LOW
  digitalWrite(PIN_RELAY, HIGH); 
  
  Serial.println("\n--- SISTEM AKTIF: SMART FARMING UPB ---");
  delay(1000);
  
  // --- KONEKSI WIFI ---
  Serial.print("📡 Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Terhubung!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Gagal Terhubung (Mode Offline)");
  }
  
  // --- SETUP SSL CERTIFICATE ---
  client.setInsecure(); // Untuk development (tidak aman untuk produksi)
  // Atau gunakan: client.setCACert(telegram_root_ca); jika punya certificate
  
  delay(2000);
  Serial.println("--- SISTEM SIAP ---\n");
}

void loop() {
  // BACA DATA
  int rawTanah = analogRead(PIN_TANAH);
  float suhu = dht.readTemperature();
  int rawLDR = analogRead(PIN_LDR);
  
  // Konversi LDR ke persentase cahaya (0-100%)
  int persenCahaya = map(rawLDR, 4095, 0, 0, 100); // Sesuaikan range jika perlu

  // HITUNG PERSENTASE (0-100%)
  int persenTanah = map(rawTanah, KERING, BASAH, 0, 100);
  
  // Keamanan: Batasi angka agar tidak aneh
  if(persenTanah > 100) persenTanah = 100;
  if(persenTanah < 0) persenTanah = 0;

  // MONITORING KE TERMINAL
  Serial.print(" | RAW TANAH: "); Serial.print(rawTanah);
  Serial.print(" | RAW LDR: "); Serial.print(rawLDR);
  Serial.print(" | TANAH: "); Serial.print(persenTanah); Serial.print("%");
  Serial.print(" | CAHAYA: "); Serial.print(persenCahaya); Serial.print("%");
  
  if (isnan(suhu)) {
    Serial.print(" | SUHU: ERROR (Cek Pin D16)");
  } else {
    Serial.print(" | SUHU: "); Serial.print(suhu); Serial.print("°C");
  }
  Serial.println("");

  // --- LOGIKA POMPA (STOP JALAN SENDIRI) ---
  bool pompaAktif = false;
  if (persenTanah < 30) {
    // Jika tanah sangat kering (< 30%), pompa nyala
    // Jika pompanya terbalik (mati saat harusnya nyala), ganti LOW jadi HIGH
    digitalWrite(PIN_RELAY, LOW); 
    pompaAktif = true;
    Serial.println(">> POMPA: NYALA (Menyiram)");
  } 
  else if (persenTanah > 80) {
    // Jika sudah basah (> 80%), pompa baru berhenti
    digitalWrite(PIN_RELAY, HIGH); 
    pompaAktif = false;
    Serial.println(">> POMPA: MATI (Tanah Cukup Basah)");
  }

  // --- KIRIM NOTIFIKASI TELEGRAM ---
  unsigned long currentTime = millis();
  
  // Kirim notifikasi jika ada perubahan status pompa atau setiap 5 menit
  if ((pompaAktif != pompaStatus) || (currentTime - lastTelegramTime >= TELEGRAM_INTERVAL)) {
    String pesan = "🌱 SMART FARMING UPDATE\n";
    pesan += "━━━━━━━━━━━━━━━━━━━━━\n";
    pesan += "Kelembaban Tanah: " + String(persenTanah) + "%\n";
    pesan += "Suhu: " + String(suhu, 1) + "°C\n";
    pesan += "Intensitas Cahaya: " + String(persenCahaya) + "%\n";
    pesan += "Status Pompa: " + String(pompaAktif ? "NYALA ✅" : "MATI ⛔") + "\n";
    pesan += "━━━━━━━━━━━━━━━━━━━━━\n";
    
    sendTelegram(pesan);
    
    pompaStatus = pompaAktif;
    lastTelegramTime = currentTime;
  }

  Serial.println("--------------------------------------");
  delay(2000); // Tunggu 2 detik tiap bacaan
}
