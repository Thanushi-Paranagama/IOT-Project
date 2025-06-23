// === Relay Motor Configuration ===
#define RELAY_PIN D1  // GPIO5 on NodeMCU

// === Flow Sensor Configuration ===
const byte flowSensorPin = D2;       // GPIO4 (D2 on NodeMCU)
volatile int pulseCount = 0;
const float calibrationFactor = 7.5; // Sensor-specific
float flowRate;     // Liters per minute
float volume;       // Total volume in liters
unsigned long lastFlowCalcTime = 0;

// === Tank Levels (in inches) ===
const int tankBottom = 32;     // Sensor to tank bottom distance
const int fullLevel = 25;      // Tank height when full
const int sensorAtFull = tankBottom - fullLevel;  // 7 inches
const int refillThreshold = tankBottom - 5;       // 27 inches

// === Ultrasonic Sensor ===
const int trigPin = D5;
const int echoPin = D6;
long duration;
float distance;

// === Flow Sensor Interrupt Handler ===
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

  // Motor
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with motor ON
  Serial.println("Motor turned ON to refill tank.");

  // Flow Sensor
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;

  // Ultrasonic Sensor
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
  distance = (duration * 0.0135) / 2;  // Convert to inches

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" inches");

  // === Motor Control Logic ===
  if (distance >= refillThreshold) {
    // Water level below 5 inches → refill
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Tank LOW - Motor ON");
  } else if (distance <= sensorAtFull) {
    // Water at full level → stop motor
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Tank FULL - Motor OFF");
  }

  delay(1000);
}
