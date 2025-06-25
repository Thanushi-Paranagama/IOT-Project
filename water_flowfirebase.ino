#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Wi-Fi credentials
#define WIFI_SSID "Ruwii's Oppo"
#define WIFI_PASSWORD "6789tfcs"

// Firebase credentials
#define API_KEY "AIzaSyDIRTwueywm2fcnBGfJa30IZ1ykQF0VwqA"
#define DATABASE_URL "https://waterflowconnect-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Flow sensor setup
const byte flowSensorPin = D2;
volatile int pulseCount = 0;
const float calibrationFactor = 7.5;

float flowRate = 0.0; // L/min
float volume = 0.0;   // Liters
unsigned long oldTime = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi connected!");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "ruwii1311@gmail.com";
  auth.user.password = "Isii1313@";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  oldTime = millis();
}

void loop() {
  if (millis() - oldTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));

    flowRate = pulseCount / calibrationFactor;
    float litersPerSecond = flowRate / 60.0;
    volume += litersPerSecond;

    Serial.printf("Flow Rate: %.2f L/min\tTotal Volume: %.3f L\n", flowRate, volume);

    // Push to Firebase
    Firebase.RTDB.setFloat(&fbdo, "/sensor/flowRate", flowRate);
    Firebase.RTDB.setFloat(&fbdo, "/sensor/volume", volume);

    pulseCount = 0;
    oldTime = millis();
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  }
}
