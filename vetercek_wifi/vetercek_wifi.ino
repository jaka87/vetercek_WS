#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <math.h>
#include <string.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <HardwareSerial.h>

// ===================== USER SETTINGS =====================
#define SERVER_URL          "https://vetercek.com/api/post_ws.php"
#define OPENVPN
#define DEBUG

// ---------- Device credentials ----------
// DEVICE_ID : integer id from podatki.id
// IMEI is derived from the ESP32 eFuse MAC at runtime as a DECIMAL number
// (no leading zeros, e.g. "189374628491" or "281474976710655").
// During boot the derived IMEI is printed to Serial so you can paste it into
// the "imei" field on lastne.php for this station.
#define DEVICE_ID           120

// ---------- WiFiManager behaviour ----------
#define WM_AP_NAME          "Vetercek-Setup"
#define WIFI_CONNECT_TIMEOUT_MS   15000UL   // try saved WiFi for this long first
#define PORTAL_TIMEOUT_S          300       // config portal timeout (seconds)
#define WIFI_LOST_RESTART_MS      300000UL  // 5 min without WiFi -> reboot

// ---------- Optional factory reset button ----------
#define RESET_BTN_PIN       9
#define RESET_HOLD_MS       3000

// ESP32-C3 GPIO numbers; change to match your wiring
#define UZ_RX_PIN           20
#define UZ_TX_PIN           21
#define UZ_BAUD             9600
#define UZ_CONFIG           SERIAL_8E1

#define HUMIDITY            31

#define DEFAULT_WHEN_SEND   3
#define DEFAULT_CUTOFF      0
#define DEFAULT_VANE_OFFSET 0
#define DEFAULT_SLEEP_SEC   2

// ===================== GLOBALS =====================
HardwareSerial ultrasonic(1);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

int  vaneOffset   = DEFAULT_VANE_OFFSET;
int  whenSend     = DEFAULT_WHEN_SEND;
byte sleepBetween = DEFAULT_SLEEP_SEC;

// Credentials — id from #define, imei from eFuse MAC (computed in setup()).
int    deviceId = DEVICE_ID;
String imei     = "";   // populated in setup() from ESP.getEfuseMac()

long  windAvr = 0;
float windGust[3] = {0, 0, 0};
float windGustAvg = 0;
int   measureCount = 0;
float windAvgX = 0, windAvgY = 0;
int   windDir = 0;
float windSpeed = 0;

float temp = 99.0;
byte  battLevel = 0;
byte  sig = 0;
byte  humidity = 0;
byte  sonicError = 0;
byte  resetReason = 0;
bool  enableHum = false;

unsigned long lastMeasure = 0;
unsigned long wifiLostSince = 0;

// ===================== HELPERS =====================

// Returns true if the reset button is held at boot.
bool resetButtonHeld() {
  if (RESET_BTN_PIN < 0) return false;

  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  if (digitalRead(RESET_BTN_PIN) != LOW) return false;

  Serial.println(F("Reset button held - keep holding to wipe WiFi settings..."));
  unsigned long t0 = millis();
  while (digitalRead(RESET_BTN_PIN) == LOW) {
    if (millis() - t0 >= RESET_HOLD_MS) return true;
    delay(50);
  }
  Serial.println(F("Released early - continuing normal boot"));
  return false;
}

// Derive a stable, unique, DECIMAL IMEI-like string from the ESP32 eFuse MAC.
// Returns plain digits with NO leading zeros, e.g. "189374628491"
// (up to 15 digits: max 48-bit MAC = 281474976710655).
// Stable across reboots, reflashes, and WiFi reconfigurations.
String getDeviceImei() {
  uint64_t chipId = ESP.getEfuseMac();   // 48-bit factory MAC
  char buf[21];                          // 20 digits + null (safe margin)
  snprintf(buf, sizeof(buf), "%llu", (unsigned long long)chipId);
  return String(buf);
}

// ===================== WIFI (WiFiManager) =====================
bool connectWiFi() {
  WiFiManager wm;

  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_MS / 1000);

  Serial.println(F("WiFi: attempting saved network first..."));
  bool ok = wm.autoConnect(WM_AP_NAME);

  if (!ok) {
    Serial.println(F("WiFi: portal timed out / no connection - rebooting"));
    delay(500);
    ESP.restart();
  }

  Serial.print(F("WiFi OK, IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("RSSI: "));
  Serial.println(WiFi.RSSI());
  return true;
}

// ===================== SEND OVER HTTPS =====================
bool SendData() {
  GetHumidity();
  computeAverages();

  float spd = (float)windSpeed;
  float gst = (float)windGustAvg;
  float tmp = temp;
  float hum = humidity;

  Serial.println();
  Serial.println(F("========== HTTP SEND =========="));
  Serial.print(F("WiFi: "));
  Serial.println(WiFi.isConnected() ? F("CONNECTED") : F("NOT CONNECTED"));
  Serial.print(F("Local IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("RSSI: "));
  Serial.println(WiFi.RSSI());
  Serial.print(F("Device ID: "));
  Serial.println(deviceId);
  Serial.print(F("IMEI: "));
  Serial.println(imei);

  if (!WiFi.isConnected()) {
    Serial.println(F("ERROR: WiFi not connected"));
    return false;
  }
  if (deviceId == 0 || imei.length() < 1 || imei.length() > 15) {
    Serial.println(F("ERROR: deviceId/imei not configured"));
    return false;
  }

  String url = String(SERVER_URL)
             + "?id="  + String(deviceId)
             + "&imei="+ imei
             + "&d="   + String(windDir)
             + "&s="   + String(spd, 1)
             + "&g="   + String(gst, 1)
             + "&t="   + String(tmp, 1)
             + "&h="   + String(hum, 1)
             + "&b="   + String(battLevel)
             + "&sig=" + String(sig)
             + "&c="   + String(measureCount)
             + "&r="   + String(resetReason);

  Serial.print(F("GET "));
  Serial.println(url);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(5000);

  HTTPClient http;
  http.setTimeout(5000);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println(F("ERROR: http.begin() failed"));
    return false;
  }

  int httpCode = http.GET();
  Serial.print(F("HTTP code: "));
  Serial.println(httpCode);

  bool ok = false;

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print(F("Response: "));
    Serial.println(payload);

    int w  = jsonInt(payload, "w");
    int o  = jsonInt(payload, "o");
    int wd = jsonInt(payload, "s");

    if (w >= 0) whenSend = w;
    if (o != -9999) vaneOffset = o;
    if (wd >= 0 && wd <= 60) sleepBetween = wd;

    Serial.printf("Parsed: whenSend=%d vaneOffset=%d sleep=%d\n",
                  whenSend, vaneOffset, sleepBetween);

    measureCount = 0;
    windAvr = 0;
    windGustAvg = 0;
    memset(windGust, 0, sizeof(windGust));
    windAvgX = windAvgY = 0;
    sonicError = 0;
    resetReason = 0;

    ok = true;
  } else {
    Serial.print(F("HTTP error: "));
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
  Serial.println(F("=============================="));
  return ok;
}

// ===================== SETUP =====================
void setup() {
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);

  Serial.begin(115200);
  delay(300);
  Serial.println(F("\nVetercek ESP32-C3 + SHT31 (HTTPS)"));

  // ---------- Optional WiFi factory reset ----------
  if (resetButtonHeld()) {
    Serial.println(F("Wiping WiFiManager settings..."));
    WiFiManager wm;
    wm.resetSettings();
    delay(500);
    ESP.restart();
  }

  // ---------- Derive IMEI from eFuse MAC ----------
  imei = getDeviceImei();

  Serial.println(F("---------------------------------------------"));
  Serial.print  (F("Configured Device ID : ")); Serial.println(deviceId);
  Serial.print  (F("Device IMEI (decimal): ")); Serial.println(imei);
  Serial.println(F("---------------------------------------------"));

  // ---------- WiFi ----------
  connectWiFi();

  // ---------- SHT31 ----------
  Wire.begin();
  if (sht31.begin(0x44)) {
    enableHum = true;
    Serial.println(F("SHT31 found"));
  } else {
    enableHum = false;
    Serial.println(F("SHT31 not found - continuing without humidity"));
  }

  // ---------- Ultrasonic anemometer UART ----------
  ultrasonic.begin(UZ_BAUD, UZ_CONFIG, UZ_RX_PIN, UZ_TX_PIN);
  delay(100);
  while (ultrasonic.available()) ultrasonic.read();

  lastMeasure = millis();
}

// ===================== LOOP =====================
void loop() {
  // --- WiFi watchdog ---
  if (!WiFi.isConnected()) {
    if (wifiLostSince == 0) {
      wifiLostSince = millis();
      Serial.println(F("WiFi lost - attempting background reconnect..."));
    }

    WiFi.reconnect();

    unsigned long lostFor = millis() - wifiLostSince;
    if (lostFor > WIFI_LOST_RESTART_MS) {
      Serial.println(F("WiFi down too long - restarting to re-enter setup flow"));
      delay(500);
      ESP.restart();
    }
  } else {
    if (wifiLostSince != 0) {
      Serial.print(F("WiFi recovered, IP: "));
      Serial.println(WiFi.localIP());
      wifiLostSince = 0;
    }
  }

  // --- Measurement loop ---
  if (millis() - lastMeasure >= (unsigned long)sleepBetween * 1000UL) {
    lastMeasure = millis();

    UltrasonicAnemometer();

    bool shouldSend = (measureCount >= whenSend) || (measureCount > 500);
    if (shouldSend && WiFi.isConnected()) {
      sig = (byte)constrain(WiFi.RSSI() + 100, 0, 100);
      SendData();
    }
  }

  delay(1000UL * sleepBetween);
}

// ===================== AVERAGE + DIRECTION =====================
void computeAverages() {
  windDir = (int)(atan2(windAvgY, windAvgX) / PI * 180.0);
  if (windDir < 0) windDir += 360;
  windSpeed = (measureCount > 0) ? (windAvr / measureCount) : 0;

#ifdef DEBUG
  Serial.printf("Avg: dir=%d speed=%d gust=%d hum=%d temp=%.1f\n",
                windDir, windSpeed, windGustAvg, humidity, temp);
#endif
}

// ===================== TINY JSON INT PARSER =====================
int jsonInt(const String &json, const char *key) {
  String pat = String("\"") + key + "\":";
  int p = json.indexOf(pat);
  if (p < 0) return -9999;
  p += pat.length();
  while (p < (int)json.length() && (json[p] == ' ' || json[p] == '"')) p++;
  int end = p;
  while (end < (int)json.length() && (json[end] == '-' || isDigit(json[end]))) end++;
  if (end == p) return -9999;
  return json.substring(p, end).toInt();
}

// ===================== SHT31 =====================
void GetHumidity() {
  if (!enableHum) return;

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temp     = t;
    humidity = (byte)constrain(round(h), 0, 100);
#ifdef DEBUG
    Serial.printf("SHT31  T:%.1f C  H:%d%%\n", temp, humidity);
#endif
  } else {
#ifdef DEBUG
    Serial.println(F("SHT31 read failed"));
#endif
  }
}

// ===================== CRC16 MODBUS =====================
uint16_t crc16(const uint8_t *data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

union FloatUnion {
  float f;
  uint8_t b[4];
} floatUnion;

// ===================== ULTRASONIC (Modbus RTU) =====================
void UltrasonicAnemometer() {
  const uint8_t  device_id     = 1;
  const uint8_t  function_code = 3;
  const uint16_t start_address = 0x0001;
  const uint16_t num_registers = 3;

  uint8_t frame[8];
  frame[0] = device_id;
  frame[1] = function_code;
  frame[2] = highByte(start_address);
  frame[3] = lowByte(start_address);
  frame[4] = highByte(num_registers);
  frame[5] = lowByte(num_registers);
  uint16_t crc = crc16(frame, 6);
  frame[6] = lowByte(crc);
  frame[7] = highByte(crc);

  while (ultrasonic.available()) ultrasonic.read();

  ultrasonic.write(frame, 8);
  delayMicroseconds(4000);

  uint8_t response[32];
  uint8_t idx = 0;
  const uint8_t expected_len = 5 + 2 * num_registers;

  unsigned long start = millis();
  while (millis() - start < 1000) {
    if (ultrasonic.available()) {
      response[idx++] = ultrasonic.read();
      if (idx >= expected_len) break;
    }
  }

  if (idx != expected_len) {
#ifdef DEBUG
    Serial.println(F("Modbus timeout"));
#endif
    sonicError++;
    return;
  }

  uint16_t recv_crc = response[idx - 2] | (response[idx - 1] << 8);
  if (recv_crc != crc16(response, idx - 2)) {
#ifdef DEBUG
    Serial.println(F("Modbus CRC error"));
#endif
    sonicError++;
    return;
  }

  if (response[1] & 0x80) {
    sonicError++;
    return;
  }

  uint8_t *payload = &response[3];

  uint16_t wind_dir_raw = (payload[0] << 8) | payload[1];

  floatUnion.b[3] = payload[4];
  floatUnion.b[2] = payload[5];
  floatUnion.b[1] = payload[2];
  floatUnion.b[0] = payload[3];
  float wind_speed_raw = floatUnion.f;

  int calDirection = wind_dir_raw + vaneOffset;
  if (calDirection < 0)    calDirection += 360;
  if (calDirection >= 360) calDirection -= 360;

  float theta = calDirection / 180.0 * PI;
  windAvgX = windAvgX * 0.75 + cos(theta) * 0.25;
  windAvgY = windAvgY * 0.75 + sin(theta) * 0.25;

  windSpeed = (float)(wind_speed_raw * 1.94384449);

  if (windSpeed >= windGust[2]) {
    windGust[0] = windGust[1];
    windGust[1] = windGust[2];
    windGust[2] = windSpeed;
  } else if (windSpeed >= windGust[1]) {
    windGust[0] = windGust[1];
    windGust[1] = windSpeed;
  } else if (windSpeed > windGust[0]) {
    windGust[0] = windSpeed;
  }
  windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;

  windAvr += windSpeed;
  measureCount++;

#ifdef DEBUG
  Serial.printf("Dir:%d Speed:%f Gust:%f Count:%d\n",
                calDirection, windSpeed, windGustAvg, measureCount);
#endif
}