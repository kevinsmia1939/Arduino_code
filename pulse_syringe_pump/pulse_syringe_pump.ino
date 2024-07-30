int value = 0;
float voltage;

void setup(){
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // Set the built-in LED pin as an output
}

void loop(){
  value = analogRead(A0);
  voltage = value * 5.0 / 1023.0;
  Serial.print("Voltage= ");
  Serial.println(voltage, 4);
  delay(100);
  
  if (voltage >= 3.0) { // Check if voltage is greater than or equal to 3V
    digitalWrite(LED_BUILTIN, HIGH); // Turn on the built-in LED
  } else {
    digitalWrite(LED_BUILTIN, LOW); // Turn off the built-in LED
  }
}
