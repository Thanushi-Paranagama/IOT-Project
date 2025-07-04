//Home tank code
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

// Firebase authentication credentials
#define USER_EMAIL "user@gmail.com"
#define USER_PASSWORD "123456"

// Pin Definitions
#define TDS_PIN A0               // TDS Sensor Pin
const int trigPin = D5;          // Ultrasonic Trigger Pin
const int echoPin = D6;          // Ultrasonic Echo Pin

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Time client for IST (UTC +5:30)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

// === Ultrasonic Sensor Variables ===
long duration;
float distance;

// === Tank Constants ===
const float maxHeight = 14.0;  // Tank height in cm

// === Timing Variables ===
unsigned long lastTdsUpload = 0;
const unsigned long TDS_UPLOAD_INTERVAL = 60000; // 1 minute

void setup() {
  Serial.begin(9600);
  
  // === Pin Configurations ===
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

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
}

void loop() {
  unsigned long nowMillis = millis();

  // === Ultrasonic Distance Measurement ===
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  // === Calculate Tank Fill Percentage ===
  float waterLevel = maxHeight - distance;
  if (waterLevel < 0) waterLevel = 0;
  if (waterLevel > maxHeight) waterLevel = maxHeight;
  float percentageFull = (waterLevel / maxHeight) * 100;

  // Send tank level to Firebase for pump control
  if (Firebase.ready()) {
    String levelPath = "/tank_level/current";
    if (Firebase.RTDB.setFloat(&fbdo, levelPath.c_str(), percentageFull)) {
      Serial.println("Tank level uploaded to Firebase");
    } else {
      Serial.println("Failed to upload tank level");
    }
    
    String distancePath = "/tank_level/distance";
    if (Firebase.RTDB.setFloat(&fbdo, distancePath.c_str(), distance)) {
      Serial.println("Distance uploaded to Firebase");
    } else {
      Serial.println("Failed to upload distance");
    }
    
    // Send pump control signal based on water level
    String pumpControlPath = "/pump_control/should_run";
    bool pumpShouldRun;
    if (distance > 10.0) {
      pumpShouldRun = true;
      Serial.printf("Distance: %.2f cm - Pump should RUN\n", distance);
    } else if (distance <= 5.0) {
      pumpShouldRun = false;
      Serial.printf("Distance: %.2f cm - Pump should STOP\n", distance);
    } else {
      // Don't change pump state in middle range
      Serial.printf("Distance: %.2f cm - Holding pump state\n", distance);
      return; // Exit without updating Firebase
    }
    
    if (Firebase.RTDB.setBool(&fbdo, pumpControlPath.c_str(), pumpShouldRun)) {
      Serial.printf("Pump control signal (%s) sent to Firebase\n", pumpShouldRun ? "RUN" : "STOP");
    } else {
      Serial.println("Failed to send pump control signal");
    }
  } else {
    Serial.println("Firebase not ready");
  }

  if (Firebase.ready() && (nowMillis - lastTdsUpload > TDS_UPLOAD_INTERVAL)) {
    lastTdsUpload = nowMillis;

    // === Read TDS from Home Sensor ===
    int analogValue = analogRead(TDS_PIN);
    float voltage = analogValue * (1.0 / 1023.0);
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

    // === Check if key exists ===
    
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
      if (Firebase.RTDB.setFloat(&fbdo, (path + "/home_tds").c_str(), tdsValue)) {
        Serial.println("home_tds updated successfully.");
      }
    } else {
      FirebaseJson json;
      json.set("home_tds", tdsValue);
      
      if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("New document with home_tds created.");
      }
    }
  }

  delay(1000);
}
