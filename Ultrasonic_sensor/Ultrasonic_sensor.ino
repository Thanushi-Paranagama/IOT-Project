//  Relay motor Configuration 
#define RELAY_PIN D1  // GPIO5 on NodeMCU

// Flow Sensor Configuration
const byte flowSensorPin = D2;       // GPIO4 (D2 on NodeMCU)
volatile int pulseCount = 0;
const float calibrationFactor = 7.5; // Sensor-specific
float flowRate;     // Liters per minute
float volume;       // Total volume in liters
unsigned long lastFlowCalcTime = 0;

//defining the trigger pin and echo pin 
const int trigPin = D5;
const int echoPin = D6;
long duration;
float distance;

//Flow Sensor Interrupt Handler
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}


void setup() {
  Serial.begin(9600);  //start serial communication

  // Setup Relay (Motor)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Turn motor ON (start refilling)
  Serial.println("Motor turned ON to refill tank.");

    // Setup Flow Sensor
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  lastFlowCalcTime = millis();
  volume = 0;

    // Setup Ultrasonic Sensor
  pinMode(trigPin, OUTPUT); //set trigger pin as output
  pinMode(echoPin, INPUT); //set echo pin as input
}

void loop() {
  unsigned long currentMillis = millis();

  //Flow Sensor Reading Every 1 Second
  if (currentMillis - lastFlowCalcTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));  // Temporarily disable interrupts

    flowRate = pulseCount / calibrationFactor;              // Calculate flow rate
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

    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);  // Re-enable interrupts
  }

  //Ultrasonic Sensor Reading 
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Water Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Stop Motor If Water Reaches 3 cm or Less
  if (distance <= 3.0) {
    digitalWrite(RELAY_PIN, LOW);  // Turn OFF motor
    Serial.println("Tank full — Motor stopped.");
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // Ensure motor is ON (if not already)
    Serial.println("Refilling tank...");
  }

  delay(500);
}