#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "rahasia.h"

// --- 1. PINOUT (KABEL SAMBUNGAN ESP32) ---
#define PIN_TANAH  32  
#define PIN_LDR    33  
#define PIN_DHT    16  
#define PIN_RELAY  5   

// --- 2. SETUP SENSOR DHT ---
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// --- 3. ANGKA KALIBRASI SENSOR TANAH ---
const int KERING = 3800; 
const int BASAH  = 1800; 

// --- 4. PENGATURAN KONEKSI & TELEGRAM ---
WiFiClientSecure client;
HTTPClient https;

String botToken = TELEGRAM_BOT_TOKEN;
String chatId = TELEGRAM_CHAT_ID;

// Variabel tracking waktu (Millis)
unsigned long lastTelegramTime = 0;
const unsigned long TELEGRAM_INTERVAL = 120000; // Rutin kirim Telegram tiap 2 menit
bool pompaStatus = false;

// --- [BARU] VARIABEL UNTUK BATASAN WAKTU POMPA ---
unsigned long waktuPompaMulai = 0;
const unsigned long MAKSIMAL_WAKTU_POMPA = 20000; // Batas pompa menyala: 20 detik
bool pompaKenaTimeout = false; 

// Fungsi untuk mengirim pesan ke Telegram
void sendTelegram(String pesan) {
  if (WiFi.status() == WL_CONNECTED) {
    pesan.replace(" ", "%20");
    pesan.replace("\n", "%0A");

    String trimmedChat = chatId;
    trimmedChat.trim();

    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + trimmedChat + "&text=" + pesan;

    https.begin(client, url);
    int httpCode = https.GET();
    https.end();
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIN_RELAY, OUTPUT);
  
  // Amankan posisi awal relay (MATI)
  digitalWrite(PIN_RELAY, HIGH); 
  
  Serial.println("\n--- SISTEM AKTIF: SMART FARMING UPB ---");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  client.setInsecure(); 
  if (WiFi.status() == WL_CONNECTED) {
    sendTelegram("=== SISTEM SMART FARMING UPB AKTIF ===");
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  static unsigned long lastSensorReadTime = 0;
  const unsigned long SENSOR_INTERVAL = 2000; // Baca sensor tiap 2 detik

  if (currentTime - lastSensorReadTime >= SENSOR_INTERVAL) {
    lastSensorReadTime = currentTime; 

    // Baca Nilai Sensor
    int rawTanah = analogRead(PIN_TANAH);
    int rawLDR = analogRead(PIN_LDR);
    float suhu = dht.readTemperature();
    
    // Konversi Persentase
    int persenCahaya = map(rawLDR, 4095, 0, 0, 100);
    if(persenCahaya > 100) persenCahaya = 100; if(persenCahaya < 0) persenCahaya = 0;

    int persenTanah = map(rawTanah, KERING, BASAH, 0, 100);
    if(persenTanah > 100) persenTanah = 100; if(persenTanah < 0) persenTanah = 0;

    bool statusPompaSebelumnya = pompaStatus; 

    // --- LOGIKA PENYIRAMAN BARU ---
    if (persenTanah >= 80) {
      if (pompaStatus) {
        digitalWrite(PIN_RELAY, HIGH); // Matikan pompa (Relay OFF)
        pompaStatus = false;
        Serial.println(">> STATUS LOG: Penyiraman selesai, kelembaban cukup (>= 80%).");
      }
    } 
    else { // Jika persenTanah < 80
      if (!pompaStatus) {
        digitalWrite(PIN_RELAY, LOW); // Nyalakan pompa (Relay ON)
        pompaStatus = true;
        waktuPompaMulai = currentTime; 
        Serial.println(">> STATUS LOG: Tanah kering (< 80%). Pompa menyala (20 detik)...");
      } else {
        // Pompa sedang menyala, cek apakah 20 detik sudah berlalu dengan millis()
        if (currentTime - waktuPompaMulai >= MAKSIMAL_WAKTU_POMPA) {
          digitalWrite(PIN_RELAY, HIGH); // Matikan sementara (Relay OFF)
          pompaStatus = false;
          Serial.println(">> STATUS LOG: Pemompaan 20 detik selesai. Jeda untuk evaluasi sensor...");
        }
      }
    }

    // [BARU] Cek apakah pompa menyala melebihi batas waktu pengaman (Timeout)
    if (pompaStatus && (currentTime - waktuPompaMulai >= MAKSIMAL_WAKTU_POMPA)) {
      digitalWrite(PIN_RELAY, HIGH); // Paksa MATI pompanya
      pompaStatus = false;
      pompaKenaTimeout = true; // Kunci status agar tidak menyala lagi sebelum tanah basah/direset
      
      Serial.println("⚠️ WARNING: Pompa dimatikan paksa oleh sistem karena batas waktu habis!");
      sendTelegram("⚠️ PERINGATAN: Pompa menyala terlalu lama (Overtime)! Dimatikan otomatis demi keamanan alat. Cek kondisi air atau sensor!");
    }

    // Monitoring Terminal
    Serial.print("[DATA] TANAH: "); Serial.print(persenTanah); Serial.print("%");
    Serial.print(" | CAHAYA: "); Serial.print(persenCahaya); Serial.print("%");
    
    String txtSuhu = isnan(suhu) ? "ERROR" : String(suhu, 1) + " C";
    Serial.print(" | SUHU: "); Serial.print(txtSuhu);
    Serial.print(" | POMPA: "); 
    Serial.println(pompaStatus ? "AKTIF" : "NONAKTIF");

    // Pengiriman Notifikasi Ke Telegram Rutin / Berubah Status
    if ((pompaStatus != statusPompaSebelumnya) || (currentTime - lastTelegramTime >= TELEGRAM_INTERVAL)) {
      String pesan = "SMART FARMING UPDATE\n";
      pesan += "---------------------\n";
      pesan += "Kelembaban Tanah: " + String(persenTanah) + " %\n";
      pesan += "Suhu Udara: " + txtSuhu + "\n";
      pesan += "Status Pompa: " + String(pompaStatus ? "AKTIF (MENYIRAM)" : "NONAKTIF (STANDBY)") + "\n";
      pesan += "---------------------";
      
      sendTelegram(pesan);
      lastTelegramTime = currentTime; 
    }
    Serial.println("-----------------------------------------------------------------");
  } 
}