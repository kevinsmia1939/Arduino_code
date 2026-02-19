const int liquidSensorPin = A0;
const int threshold = 100;   
int sensorValue = 0;    

void setup() {
  Serial.begin(9600);        
  pinMode(liquidSensorPin, INPUT);
}

void loop() {
  sensorValue = analogRead(liquidSensorPin);

  // Print the raw sensor value to the serial monitor
  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  // Check if the liquid is detected
  if (sensorValue > threshold) {
    Serial.println("Liquid detected!");
  } else {
    Serial.println("No liquid detected.");
  }

  delay(100);
}
