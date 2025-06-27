#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// Wi-Fi credentials
#define WIFI_SSID "H M T's A55"
#define WIFI_PASSWORD "qwer1234"

// Firebase project details
#define API_KEY "AIzaSyD6Cs9LM-y6gBkvDGlDAyn9VPi3F8Wd3uw"
#define DATABASE_URL "https://monitoring-water-system-ca40a-default-rtdb.asia-southeast1.firebasedatabase.app/"

// TDS Sensor Pin
#define TDS_PIN A0

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Time client for IST (UTC +5:30)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // IST offset: +5:30

unsigned long lastTdsUpload = 0;
const unsigned long TDS_UPLOAD_INTERVAL = 60000; // 1 minute

void setup() {
  Serial.begin(9600);

  // === Connect to WiFi ===
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  // === Initialize NTP Time ===
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // === Firebase Configuration ===
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Anonymous sign-in
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Anonymous sign-in successful.");
  } else {
    Serial.printf("Sign-up error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  unsigned long nowMillis = millis();

  if (Firebase.ready() && (nowMillis - lastTdsUpload > TDS_UPLOAD_INTERVAL)) {
    lastTdsUpload = nowMillis;

    // === Read TDS from Home Sensor ===
    int analogValue = analogRead(TDS_PIN);
    float voltage = analogValue * (1.0 / 1023.0); // ADC range for ESP8266 (0–1V)
    float tdsValue = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;

    // === Get Proper Time ===
    timeClient.update();
    time_t now = timeClient.getEpochTime();
    struct tm* t = localtime(&now);

    // === Build timestamp key ===
    char docKey[20];
    snprintf(docKey, sizeof(docKey), "%04d%02d%02d%02d%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min);

    String path = "/tds_data/" + String(docKey);

    Serial.print("Checking if Firebase key exists: ");
    Serial.println(path);

    // === Check if key exists ===
    delay(20000);
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
      Serial.println("Key exists. Updating only home_tds...");
      if (Firebase.RTDB.setFloat(&fbdo, (path + "/home_tds").c_str(), tdsValue)) {
        Serial.println("home_tds updated successfully.");
      } else {
        Serial.print("Update error: ");
        Serial.println(fbdo.errorReason());
      }
    } else {
      Serial.println("Key does not exist. Creating new entry with home_tds...");
      FirebaseJson json;
      json.set("home_tds", tdsValue);

      if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("New document with home_tds created.");
      } else {
        Serial.print("Create error: ");
        Serial.println(fbdo.errorReason());
      }
    }
  }

  delay(1000);
}
