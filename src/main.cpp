#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "rahasia.h"

// --- Pin Configuration ---
#define PIN_DHT    16
#define PIN_SOIL   32
#define PIN_LIGHT  35
#define PIN_PUMP   5

// --- Sensor Setup ---
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// --- ADC Calibration ---
const int NILAI_KERING = 3800;
const int NILAI_BASAH  = 1800;

// --- WiFi & API ---
WiFiMulti wifiMulti;
WiFiClientSecure client;
HTTPClient http;

const char* API_SERVER = "https://apiapimonitoringplant.vigian-ai.my.id";
const unsigned long API_INTERVAL = 10000;
const unsigned long SETTINGS_CHECK_INTERVAL = 30000;
const unsigned long COMMAND_POLL_INTERVAL = 30000;
const unsigned long TELEGRAM_INTERVAL = 120000;

// --- Settings Structure ---
struct DeviceSettings {
  int soil_threshold = 30;
  int pump_max_duration = 3;
  int pump_cooldown = 20;
  bool telegram_enabled = true;
  bool auto_water_enabled = true;
  unsigned long last_watering = 0;
};

DeviceSettings deviceSettings;

// --- Time Tracking ---
unsigned long lastApiTime = 0;
unsigned long lastSettingsCheck = 0;
unsigned long lastCommandCheck = 0;
unsigned long pumpStart = 0;
unsigned long lastTelegramTime = 0;

bool pumpStatus = false;

// --- OOP Sensor Classes ---
class SensorReader {
public:
  float readTemperature() {
    float temp = dht.readTemperature();
    return isnan(temp) ? 0 : temp;
  }

  float readHumidity() {
    float hum = dht.readHumidity();
    return isnan(hum) ? 0 : hum;
  }

  int readSoilMoisture() {
    int raw = analogRead(PIN_SOIL);
    int pct = map(raw, NILAI_KERING, NILAI_BASAH, 0, 100);
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
    return pct;
  }

  int readLightIntensity() {
    int raw = analogRead(PIN_LIGHT);
    return map(raw, 0, 4095, 0, 100);
  }
};

class WaterPumpController {
private:
  uint8_t pin;
  bool running = false;

public:
  WaterPumpController(uint8_t pumpPin) : pin(pumpPin) {}

  void begin() {
    pinMode(pin, OUTPUT);
    stop();
  }

  void start() {
    digitalWrite(pin, LOW);
    pumpStart = millis();
    running = true;
  }

  void stop() {
    digitalWrite(pin, HIGH);
    pumpStart = 0;
    running = false;
  }

  bool isRunning() {
    return running;
  }

  bool shouldStop(int maxDuration) {
    return isRunning() && (millis() - pumpStart) >= (maxDuration * 1000);
  }
};

class APIClient {
private:
  HTTPClient& http;
  WiFiClientSecure& client;

public:
  APIClient(HTTPClient& httpClient, WiFiClientSecure& wifiClient)
    : http(httpClient), client(wifiClient) {}

  bool sendSensorData(float temp, float hum, int light, int soil, bool pumpStatus) {
    if (WiFi.status() != WL_CONNECTED) return false;

    String url = String(API_SERVER) + "/api/sensor";
    String json = "{\"device_id\":\"esp32-001\"";
    json += ",\"temperature\":" + String(temp, 1);
    json += ",\"humidity\":" + String(hum, 1);
    json += ",\"light_intensity\":" + String(light);
    json += ",\"soil_moisture\":" + String(soil);
    json += ",\"pump_status\":" + String(pumpStatus ? 1 : 0);
    json += ",\"wifi_ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"firmware_version\":\"1.0.0\"";
    json += "}";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    http.end();
    return code == 200 || code == 201;
  }

  bool sendHeartbeat() {
    if (WiFi.status() != WL_CONNECTED) return false;
    String url = String(API_SERVER) + "/api/device/heartbeat";
    String json = "{\"device_id\":\"esp32-001\"";
    json += ",\"wifi_ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"firmware_version\":\"1.0.0\"";
    json += "}";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    http.end();
    return code == 200;
  }

  bool fetchSettings(DeviceSettings& settings) {
    if (WiFi.status() != WL_CONNECTED) return false;

    String url = String(API_SERVER) + "/api/settings";
    http.begin(client, url);
    int code = http.GET();

    if (code == 200) {
      String payload = http.getString();
      http.end();

      int soilIdx = payload.indexOf("\"soil_threshold\":");
      int pumpDurIdx = payload.indexOf("\"pump_max_duration\":");
      int pumpCoolIdx = payload.indexOf("\"pump_cooldown\":");
      int teleIdx = payload.indexOf("\"telegram_enabled\":");
      int autoIdx = payload.indexOf("\"auto_water_enabled\":");

      if (soilIdx != -1) {
        settings.soil_threshold = payload.substring(soilIdx + 17, payload.indexOf(',', soilIdx)).toInt();
      }
      if (pumpDurIdx != -1) {
        settings.pump_max_duration = payload.substring(pumpDurIdx + 20, payload.indexOf(',', pumpDurIdx)).toInt();
      }
      if (pumpCoolIdx != -1) {
        settings.pump_cooldown = payload.substring(pumpCoolIdx + 16, payload.indexOf(',', pumpCoolIdx)).toInt();
      }
      if (teleIdx != -1) {
        int endVal = payload.indexOf(',', teleIdx);
        if(endVal == -1) endVal = payload.indexOf('}', teleIdx);
        String val = payload.substring(teleIdx + 19, endVal);
        val.trim();
        settings.telegram_enabled = (val == "true");
      }
      if (autoIdx != -1) {
        int endVal = payload.indexOf(',', autoIdx);
        if(endVal == -1) endVal = payload.indexOf('}', autoIdx);
        String val = payload.substring(autoIdx + 21, endVal);
        val.trim();
        settings.auto_water_enabled = (val == "true");
      }

      Serial.printf("[DEBUG] Parsed - Soil: %d, Dur: %d, Cool: %d, Tele: %d, Auto: %d\n",
          settings.soil_threshold, settings.pump_max_duration, settings.pump_cooldown,
          settings.telegram_enabled, settings.auto_water_enabled);

      return true;
    }
    http.end();
    return false;
  }

  bool fetchPendingCommand(int& duration) {
    if (WiFi.status() != WL_CONNECTED) return false;

    String url = String(API_SERVER) + "/api/sensor/pending?limit=1";
    http.begin(client, url);
    int code = http.GET();

    if (code == 200) {
      String payload = http.getString();
      http.end();

      int typeIdx = payload.indexOf("\"type\"");
      int durationIdx = payload.indexOf("\"duration\"");

      if (typeIdx != -1 && durationIdx != -1) {
        int durEnd = payload.indexOf(',', durationIdx);
        if(durEnd == -1) durEnd = payload.indexOf('}', durationIdx);
        String durStr = payload.substring(durationIdx + 10, durEnd);
        durStr.trim();
        duration = durStr.toInt();
        Serial.printf("[COMMAND] Watering command received, duration: %d seconds\n", duration);
        return true;
      }
    }
    http.end();
    return false;
  }

  bool acknowledgeCommand(int id) {
    if (WiFi.status() != WL_CONNECTED) return false;
    String url = String(API_SERVER) + "/api/command/ack";
    String json = "{\"device_id\":\"esp32-001\",\"command_id\":" + String(id) + "}";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    http.end();
    return code == 200;
  }

  void logWatering(int duration) {
    if (WiFi.status() != WL_CONNECTED) return;

    String url = String(API_SERVER) + "/api/logs";
    String json = "{\"device_id\":\"esp32-001\",\"action\":\"auto_watering\",\"duration_seconds\":" + String(duration) + "}";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.POST(json);
    http.end();
  }
};

// --- Telegram Notification ---
void sendTelegram(String pesan) {
  if (WiFi.status() != WL_CONNECTED || !deviceSettings.telegram_enabled) return;

  pesan.replace("%", "%25");
  pesan.replace(" ", "%20");
  pesan.replace("\n", "%0A");

  String trimmedChat = String(TELEGRAM_CHAT_ID);
  trimmedChat.trim();

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage?chat_id=" + trimmedChat + "&text=" + pesan;

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("[Telegram] HTTP Status Code: %d\n", httpCode);
  } else {
    Serial.printf("[Telegram] Gagal! Error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// --- OOP Instances ---
SensorReader sensorReader;
WaterPumpController pump(PIN_PUMP);
APIClient api(http, client);

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();
  pump.begin();
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_LIGHT, INPUT);

  Serial.println("\n--- Smart Farming ESP32 Start ---");

  for (int i = 0; i < wifiListSize; i++) {
    wifiMulti.addAP(wifiList[i].ssid, wifiList[i].password);
  }

  Serial.println("Menghubungkan ke WiFi...");
  int attempts = 0;
  while (wifiMulti.run() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Terhubung ke WiFi: ");
    Serial.println(WiFi.SSID());
    client.setInsecure();
    sendTelegram("=== SISTEM SMART FARMING UPB AKTIF ===");
  } else {
    Serial.println("");
    Serial.println("Gagal terhubung ke WiFi.");
  }
}

// --- Loop ---
void loop() {
  unsigned long currentTime = millis();

  static unsigned long lastSensorReadTime = 0;
  const unsigned long SENSOR_INTERVAL = 2000;

  if (currentTime - lastSensorReadTime >= SENSOR_INTERVAL) {
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Peringatan: WiFi terputus, mencoba menghubungkan kembali...");
    }
    lastSensorReadTime = currentTime;

    float suhu = sensorReader.readTemperature();
    float hum = sensorReader.readHumidity();
    int soil = sensorReader.readSoilMoisture();
    int light = sensorReader.readLightIntensity();

    Serial.print("[SENSOR] Temp: "); Serial.print(suhu, 1); Serial.print("C | Hum: ");
    Serial.print(hum, 1); Serial.print("% | Soil: "); Serial.print(soil); Serial.print("% | Light: ");
    Serial.print(light); Serial.println("%");

    api.sendHeartbeat();

    if (currentTime - lastSettingsCheck >= SETTINGS_CHECK_INTERVAL) {
      lastSettingsCheck = currentTime;
      if (api.fetchSettings(deviceSettings)) {
        Serial.println("[API] Settings updated from server");
      }
    }

    if (currentTime - lastCommandCheck >= COMMAND_POLL_INTERVAL) {
      lastCommandCheck = currentTime;
      int cmdDuration = 0;
      if (api.fetchPendingCommand(cmdDuration)) {
        if (cmdDuration > 0 && !pump.isRunning()) {
          Serial.printf("[COMMAND] Executing watering command for %d seconds\n", cmdDuration);
          pump.start();
          deviceSettings.last_watering = currentTime;
        }
      }
    }

    if (currentTime - lastApiTime >= API_INTERVAL) {
      lastApiTime = currentTime;

      bool apiOk = api.sendSensorData(suhu, hum, light, soil, pump.isRunning());
      if (apiOk) {
        Serial.println("[API] Data sensor berhasil dikirim");
      } else {
        Serial.println("[API] Gagal kirim data sensor");
      }
    }

    if (!pump.isRunning()) {
      if (soil < deviceSettings.soil_threshold && deviceSettings.auto_water_enabled) {
        unsigned long timeSinceLastWater = (currentTime - deviceSettings.last_watering) / 1000;
        if (timeSinceLastWater >= deviceSettings.pump_cooldown) {
          Serial.println("[AUTO] Starting auto watering...");
          pump.start();
          deviceSettings.last_watering = currentTime;
          api.logWatering(deviceSettings.pump_max_duration);
        }
      }
    }

    if (pump.shouldStop(deviceSettings.pump_max_duration)) {
      Serial.println("[AUTO] Stopping pump after max duration");
      sendTelegram("⚠️ PERINGATAN: Pompa menyala terlalu lama! Dimatikan otomatis.");
      pump.stop();
    } else if (pump.isRunning() && soil >= deviceSettings.soil_threshold) {
      Serial.println("[AUTO] Stopping pump, soil is moist enough");
      pump.stop();
    }

    bool currentPumpStatus = pump.isRunning();
    if (currentPumpStatus != pumpStatus) {
      sendTelegram("SMART FARMING UPDATE\n----------------------\nKelembaban Tanah: " + String(soil) + "%\nStatus Pompa: " + String(currentPumpStatus ? "AKTIF (MENYIRAM)" : "NONAKTIF (STANDBY)"));
      pumpStatus = currentPumpStatus;
    } else if (currentTime - lastTelegramTime >= TELEGRAM_INTERVAL) {
      String pesan = "SMART FARMING UPDATE\n----------------------\nKelembaban Tanah: " + String(soil) + "%\nStatus Pompa: " + String(currentPumpStatus ? "AKTIF (MENYIRAM)" : "NONAKTIF (STANDBY)");
      sendTelegram(pesan);
      lastTelegramTime = currentTime;
    }

    Serial.println("---------------------------------");
  }
}