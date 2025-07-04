#include <ESP8266WiFi.h>

// Relay pin connected to motor
#define RELAY_PIN D1

bool motorOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with motor OFF
  Serial.println("Motor Test Started...");
}

void loop() {
  motorOn = !motorOn; // Toggle motor state

  if (motorOn) {
    digitalWrite(RELAY_PIN, HIGH);  // Turn motor ON
    Serial.println("Motor Status: ON");
  } else {
    digitalWrite(RELAY_PIN, LOW);   // Turn motor OFF
    Serial.println("Motor Status: OFF");
  }

  delay(2000); // Wait 2 seconds
}
