//defining the trigger pin and echo pin 
const int trigPin = D5;
const int echoPin = D6;

long duration;
float distance;


void setup() {
  Serial.begin(9600);  //start serial communication
  pinMode(trigPin, OUTPUT); //set trigger pin as output
  pinMode(echoPin, INPUT); //set echo pin as input
}

void loop() {
  //clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);


  //send 10ms pulse to trigger the ultrasonic burst
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  //measure the echo return time in microseconds
  duration = pulseIn(echoPin,HIGH);

  //calculate the distance in cm: speed of sound = 34300 cm/s
  distance = (duration * 0.0343) /2;

  //output the distance to the serial monitor
  Serial.print("Distnace: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(500); //wait before next measurement

}
