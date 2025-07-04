//resource code
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi credentials
#define WIFI_SSID "H M T's A55"
#define WIFI_PASSWORD "qwer1234"

// Firebase project details
#define API_KEY "AIzaSyD6Cs9LM-y6gBkvDGlDAyn9VPi3F8Wd3uw"
#define DATABASE_URL "https://monitoring-water-system-ca40a-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase authentication credentials
#define USER_EMAIL "admin@gmail.com"
#define USER_PASSWORD "123456"

// Pin Definitions
#define TDS_PIN A0               // TDS Sensor Pin
#define RELAY_PIN D1             // GPIO5 on NodeMCU - Motor Control
const byte flowSensorPin = D2;   // GPIO4 (D2 on NodeMCU) - Flow Sensor

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST offset: +5:30

// === Flow Sensor Configuration ===
volatile int pulseCount = 0;
const float calibrationFactor = 7.5;
float flowRate;
float volume;
unsigned long lastFlowCalcTime = 0;

// === Timing Variables ===
unsigned long lastSendTime = 0;
unsigned long lastPumpCheck = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  // === Pin Configurations ===
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with motor ON (back to original)

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Email/password authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)) {
    Serial.println("Firebase authentication successful.");
  } else {
    Serial.printf("Firebase authentication error: %s\n", config.signer.signupError.message.c_str());
    Serial.println("Note: This error may occur if the account already exists, but authentication should still work.");
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  timeClient.begin();
  timeClient.update();
}

void loop() {
  unsigned long currentMillis = millis();

  // === Flow Sensor Calculation Every 1 Second ===
  if (currentMillis - lastFlowCalcTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));

    flowRate = pulseCount / calibrationFactor;
    float litersPerSecond = flowRate / 60.0;
    volume += litersPerSecond;

    pulseCount = 0;
    lastFlowCalcTime = currentMillis;

    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  }

  // === Check Tank Level for Pump Control ===
  if (Firebase.ready() && (currentMillis - lastPumpCheck > 2000)) { // Check every 2 seconds
    lastPumpCheck = currentMillis;
    
    Serial.println("Checking pump control from Firebase...");
    if (Firebase.RTDB.getBool(&fbdo, "/pump_control/should_run")) {
      bool shouldRun = fbdo.boolData();
      bool currentPumpState = digitalRead(RELAY_PIN);
      
      Serial.printf("Firebase says pump should: %s\n", shouldRun ? "RUN" : "STOP");
      Serial.printf("Current pump state: %s\n", currentPumpState ? "ON" : "OFF");
      
      if (shouldRun && !currentPumpState) {
        digitalWrite(RELAY_PIN, HIGH); // Turn pump ON (back to original)
        Serial.println("Pump turned ON - Water level low");
      } else if (!shouldRun && currentPumpState) {
        digitalWrite(RELAY_PIN, LOW); // Turn pump OFF (back to original)
        Serial.println("Pump turned OFF - Water level sufficient");
      } else {
        Serial.println("Pump state unchanged");
      }
    } else {
      Serial.println("Failed to read pump control from Firebase");
      Serial.println(fbdo.errorReason());
    }
  } else if (!Firebase.ready()) {
    Serial.println("Firebase not ready for pump control check");
  }

  // === Send Data Every 60 Seconds ===
  if (millis() - lastSendTime > 60000) {
    lastSendTime = millis();

    timeClient.update();

    // Get time key like "202506261830"
    time_t now = timeClient.getEpochTime();
    struct tm* t = localtime(&now);
    char key[20];
    snprintf(key, sizeof(key), "%04d%02d%02d%02d%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);

    String path = "/tds_data/" + String(key);

    int analogValue = analogRead(TDS_PIN);
    float voltage = analogValue * (1.0 / 1023.0);
    float tdsValue = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;

    // Check if document exists and update
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
      // Document exists, add source_tds
      Firebase.RTDB.setFloat(&fbdo, (path + "/source_tds").c_str(), tdsValue);
      Firebase.RTDB.setFloat(&fbdo, (path + "/flow_rate").c_str(), flowRate);
      Firebase.RTDB.setFloat(&fbdo, (path + "/total_volume").c_str(), volume);
      Firebase.RTDB.setBool(&fbdo, (path + "/pump_status").c_str(), digitalRead(RELAY_PIN));
    } else {
      // Create new document
      FirebaseJson json;
      json.set("source_tds", tdsValue);
      json.set("flow_rate", flowRate);
      json.set("total_volume", volume);
      json.set("pump_status", digitalRead(RELAY_PIN));
      char timestamp[25];
      snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
      json.set("timestamp", timestamp);

      Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
    }

    Serial.printf("Data sent - TDS: %.2f, Flow: %.2f L/min, Volume: %.2f L\n", 
                  tdsValue, flowRate, volume);
  }
}
