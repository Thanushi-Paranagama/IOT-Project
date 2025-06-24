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
const float maxHeight = 14.0;  // Tank height in cm

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

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
  } else if (distance <= 5.0) {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Water at 9cm or more - Motor OFF");
  } else {
    Serial.println("Water rising... holding motor state.");
  }

  delay(1000);
}
