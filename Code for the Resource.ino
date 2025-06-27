//home sensor
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Wi-Fi credentials
#define WIFI_SSID "H M T's A55"
#define WIFI_PASSWORD "qwer1234"

// Firebase project details
#define API_KEY "AIzaSyD6Cs9LM-y6gBkvDGlDAyn9VPi3F8Wd3uw"
#define DATABASE_URL "https://monitoring-water-system-ca40a-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase Authentication credentials
#define USER_EMAIL "thanushiparanagama@gmail.com"
#define USER_PASSWORD "123456"

// Pin Definitions
#define TDS_PIN A0               // TDS Sensor Pin
#define RELAY_PIN D1             // GPIO5 on NodeMCU - Motor Control
const byte flowSensorPin = D2;   // GPIO4 (D2 on NodeMCU) - Flow Sensor
const int trigPin = D5;          // Ultrasonic Trigger Pin
const int echoPin = D6;          // Ultrasonic Echo Pin

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// === Flow Sensor Configuration ===
volatile int pulseCount = 0;
const float calibrationFactor = 7.5;
float flowRate;
float volume;
unsigned long lastFlowCalcTime = 0;

// === Ultrasonic Sensor Variables ===
long duration;
float distance;

// === Tank Constants ===
const float maxHeight = 14.0;  // Tank height in cm

// === Timing Variables ===
unsigned long lastSendTime = 0;
const unsigned long FIREBASE_INTERVAL = 10000; // Send data every 10 seconds

// === TDS Variables ===
float tdsValue = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  // === Pin Configurations ===
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with motor ON
  Serial.println("Motor turned ON to refill tank.");

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // === Wi-Fi Connection ===
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  
  // === Firebase Configuration ===
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Set user credentials for authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Sign in with email and password
  Serial.println("Signing in to Firebase...");
  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)) {
    Serial.println("Firebase sign-in successful.");
  } else {
    Serial.printf("Firebase sign-in error: %s\n", config.signer.signupError.message.c_str());
    
    // If sign-up fails, try to sign in (user might already exist)
    Serial.println("Trying to sign in with existing account...");
    if (Firebase.ready()) {
      Serial.println("Firebase authentication successful with existing account.");
    }
  }
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Setup complete. Starting water monitoring system...");
}

void loop() {
  unsigned long currentMillis = millis();

  // === Flow Sensor Calculation Every 1 Second ===
  if (currentMillis - lastFlowCalcTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));

    flowRate = pulseCount / calibrationFactor;
    float litersPerSecond = flowRate / 60.0;
    volume += litersPerSecond;

    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.print(" L/min\t");

    Serial.print("Total Volume: ");
    Serial.print(volume, 3);
    Serial.println(" L");

    pulseCount = 0;
    lastFlowCalcTime = currentMillis;

    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  }

  // === TDS Sensor Reading ===
  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * (1.0 / 1023.0);  // For ESP8266 (0-1V ADC)
  tdsValue = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;

  // === Ultrasonic Distance Measurement ===
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Distance from Sensor: ");
  Serial.print(distance);
  Serial.print(" cm\t");

  Serial.print("TDS: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  // === Calculate Tank Fill Percentage ===
  float waterLevel = maxHeight - distance;
  if (waterLevel < 0) waterLevel = 0;
  if (waterLevel > maxHeight) waterLevel = maxHeight;
  float percentageFull = (waterLevel / maxHeight) * 100;

  Serial.print("Tank Fill Level: ");
  Serial.print(percentageFull);
  Serial.println(" %");

  // === Motor Control Logic ===
  bool motorState = digitalRead(RELAY_PIN);
  if (distance > 10.0) {
    digitalWrite(RELAY_PIN, HIGH);
    if (!motorState) Serial.println("Water below 4cm - Motor ON");
  } else if (distance <= 5.0) {
    digitalWrite(RELAY_PIN, LOW);
    if (motorState) Serial.println("Water at 9cm or more - Motor OFF");
  } else {
    Serial.println("Water rising... holding motor state.");
  }

  // === Firebase Data Upload ===
  if (Firebase.ready() && (currentMillis - lastSendTime > FIREBASE_INTERVAL)) {
    lastSendTime = currentMillis;
    
    // Create timestamp for unique data entry
    String timestamp = String(currentMillis);
    
    // Create JSON object for all sensor data
    FirebaseJson json;
    json.set("timestamp", timestamp);
    json.set("tds_ppm", tdsValue);
    json.set("flow_rate_lpm", flowRate);
    json.set("total_volume_l", volume);
    json.set("distance_cm", distance);
    json.set("water_level_cm", waterLevel);
    json.set("tank_fill_percent", percentageFull);
    json.set("motor_state", digitalRead(RELAY_PIN) ? "ON" : "OFF");
    
    // Upload complete sensor data
    String dataPath = "/water_monitoring/" + timestamp;
    
    Serial.print("Uploading sensor data to Firebase... ");
    if (Firebase.RTDB.setJSON(&fbdo, dataPath.c_str(), &json)) {
      Serial.println("SUCCESS");
    } else {
      Serial.print("FAILED: ");
      Serial.println(fbdo.errorReason());
    }
    
    // Also upload individual TDS data to maintain compatibility
    String tdsPath = "/tds_data/" + timestamp;
    if (Firebase.RTDB.setFloat(&fbdo, tdsPath.c_str(), tdsValue)) {
      Serial.println("TDS data uploaded successfully.");
    } else {
      Serial.print("TDS upload error: ");
      Serial.println(fbdo.errorReason());
    }
    
  } else if (!Firebase.ready()) {
    Serial.println("Firebase not ready. Checking connection...");
  }

  delay(10000);
}
