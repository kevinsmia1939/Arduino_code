int value = 0;
float voltage;
int ledPin1 = 7;

void setup(){
  Serial.begin(9600);
  pinMode(ledPin1, OUTPUT);
}

void loop(){
  value = analogRead(A0);
  voltage = value * 5.0 / 1023.0;
  Serial.print("Voltage= ");
  Serial.println(voltage, 4);
  delay(100);
  
  if (voltage >= 3.0) { // Check if voltage is greater than or equal to 3V
    digitalWrite(ledPin1, HIGH); // Turn on the LED
  } else {
    digitalWrite(ledPin1, LOW); // Turn off the LED
  }
}