const int pin0 = A0;
const int pin1 = A1;

float Vs = 5.007;
float Vo0 = 0.0, Vo1 = 0.0;
float P0 = 0.0, P1 = 0.0;

// change this to float!
float timeMin = 0.0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Read and convert voltages
  Vo0 = analogRead(pin0) * (Vs / 1023.0);
  P0  = (((Vo0 / Vs) - 0.5) / 0.2);

  Vo1 = analogRead(pin1) * (Vs / 1023.0);
  P1  = (((Vo1 / Vs) - 0.5) / 0.2);

  float diff = P0 - P1;

  // millis() returns unsigned long, dividing by float gives float
  timeMin = millis() / 60000.0;  

  // now timeMin is a float, so Serial.print will respect the precision
  // Serial.print(timeMin, 5);
  Serial.print(0);
  Serial.print(", ");
  Serial.print(diff, 5);
  Serial.print(", ");
  Serial.println(analogRead(pin0));
  delay(50);
}
