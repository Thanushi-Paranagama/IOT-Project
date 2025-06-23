#define TdsSensorPin A0

float adcVoltage = 0;

float tdsValue = 0;
 
void setup() {

  Serial.begin(9600);

}
 
void loop() {

  int analogValue = analogRead(TdsSensorPin);

  adcVoltage = analogValue * (3.3 / 1024.0); // Convert analog to voltage (ESP8266 = 10-bit ADC)

  // Formula: TDS (ppm) = (133.42*V^3 - 255.86*V^2 + 857.39*V) * 0.5

  // The "*0.5" is the TDS factor; you can calibrate it

  float voltage = adcVoltage;

  tdsValue = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;
 
  Serial.print("TDS Value: ");

  Serial.print(tdsValue);

  Serial.println(" ppm");
 
  delay(1000);

}

 
