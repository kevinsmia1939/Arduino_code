float timeMin = 0.0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  timeMin = millis() / 60000.0;  
  Serial.print(timeMin, 4);
  Serial.print(", ");
  Serial.print(-2);
  Serial.print(", ");
  Serial.println(2);

  delay(50);
}
