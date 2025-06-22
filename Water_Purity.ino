#include <ESP8266WiFi.h>

#include <Firebase_ESP_Client.h>
 
// Wi-Fi credentials
#define WIFI_SSID "H M T's A55"
#define WIFI_PASSWORD "qwer1234"
 
// Firebase project details

#define API_KEY "AIzaSyD6Cs9LM-y6gBkvDGlDAyn9VPi3F8Wd3uw"
#define DATABASE_URL "https://monitoring-water-system-ca40a-default-rtdb.asia-southeast1.firebasedatabase.app/"  // MUST end with '/'
 
// TDS Sensor Pin

#define TDS_PIN A0
 
// Firebase objects

FirebaseData fbdo;

FirebaseAuth auth;

FirebaseConfig config;
 
unsigned long lastSendTime = 0;
 
void setup() {

  Serial.begin(9600);
 
  // Connect to Wi-Fi

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

  }

  Serial.println("\nWiFi connected.");
 
  // Firebase Configuration

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

  if (millis() - lastSendTime > 10000) {

    lastSendTime = millis();
 
    int analogValue = analogRead(TDS_PIN);

    float voltage = analogValue * (1.0 / 1023.0);  // For ESP8266 (0-1V ADC)

    float tdsValue = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;
 
    String path = "/tds_data/" + String(millis());
 
    Serial.print("Sending TDS: ");

    Serial.println(tdsValue);
 
    if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), tdsValue)) {

      Serial.println("TDS value uploaded successfully.");

    } else {

      Serial.print("Firebase error: ");

      Serial.println(fbdo.errorReason());

    }

  }

}
