#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Wi-Fi credentials
#define WIFI_SSID "769 WIFI"
#define WIFI_PASSWORD "Vindula1241#@"

// Firebase project details
#define API_KEY "AIzaSyBE4NaupJW2MaNIvP5xFt0ufO-WjPIG79o"
#define DATABASE_URL "https://iot-project1-16488-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase Authentication credentials
#define USER_EMAIL "puneeshar@gmail.com"
#define USER_PASSWORD "puneesha0707"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define RELAY_PIN D1  // GPIO5 on NodeMCU

// === Flow Sensor Configuration ===
const byte flowSensorPin = D2;       // GPIO4 (D2 on NodeMCU)
volatile int pulseCount = 0;
const float calibrationFactor = 7.5;
float flowRate;
float volume;
unsigned long lastFlowCalcTime = 0;

// === Ultrasonic Sensor Pins ===
const int trigPin = D5;
const int echoPin = D6;
long duration;
float distance;

// === Tank Constants ===
const float maxHeight = 16.0;  // Tank height in cm

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

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
    Serial.println("Trying to sign in with existing account...");
    if (Firebase.ready()) {
      Serial.println("Firebase authentication successful with existing account.");
    }
  }

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup complete. Starting water monitoring system...");

  // === Hardware Pin Configuration ===
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with motor ON
  Serial.println("Motor turned ON to refill tank.");

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
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
  Serial.println(" cm");

  // === Calculate Tank Fill Percentage ===
  float waterLevel = maxHeight - distance;
  if (waterLevel < 0) waterLevel = 0;
  if (waterLevel > maxHeight) waterLevel = maxHeight;
  float percentageFull = (waterLevel / maxHeight) * 100;

  Serial.print("Tank Fill Level: ");
  Serial.print(percentageFull);
  Serial.println(" %");

  // === Motor Control Logic ===
  if (distance > 10.0) {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Water below 4cm - Motor ON");
  } else if (distance <= 4.0) {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Water at 9cm or more - Motor OFF");
  } else {
    Serial.println("Water rising... holding motor state.");
  }

  delay(1000);
}
