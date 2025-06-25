#include <ESP8266WiFi.h>

#define RELAY_PIN D1  

const byte flowSensorPin = D2;       
volatile int pulseCount = 0;
const float calibrationFactor = 7.5; 
float flowRate;     
float volume;       
unsigned long lastFlowCalcTime = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  Serial.println("Motor turned ON permanently.");

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;
}

void loop() {
  unsigned long currentMillis = millis();

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
}