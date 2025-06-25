// Constants
const byte flowSensorPin = 2;  // D2 pin
volatile int pulseCount = 0;

const float calibrationFactor = 7.5; 

// Variables
float flowRate;     // Liters per minute
float volume;       // Total volume in liters
unsigned long oldTime = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);

  oldTime = millis();
  volume = 0;
}

void loop() {
  if (millis() - oldTime >= 1000) {  // Every 1 second
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));

    // Calculate flow rate in L/min
    flowRate = (pulseCount / calibrationFactor);

    // Convert flow rate (L/min) to volume in this 1-second interval
    float litersPerSecond = flowRate / 60.0;
    volume += litersPerSecond;

    // Display on Serial Monitor
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.print(" L/min\t");

    Serial.print("Total Volume: ");
    Serial.print(volume, 3);  // 3 decimal places
    Serial.println(" L");

    pulseCount = 0;
    oldTime = millis();

    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);
  }
}


