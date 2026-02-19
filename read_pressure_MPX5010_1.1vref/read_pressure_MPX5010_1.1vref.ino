const int pin0 = A0;
const int pin1 = A1;

// Internal reference voltage is ~1.1V
const float Vref    = 1.1;
const float Vsupply = 5.007;
float Vo0 = 0.0, Vo1 = 0.0;
float P0 = 0.0, P1 = 0.0;

float time;
const unsigned long interval = 1000; // 5 seconds
const int sampleInterval = 10;       // ms between samples
const int maxSamples = interval / sampleInterval;

float diffSum = 0.0;
int sampleCount = 0;

void setup() {
  Serial.begin(9600);
  analogReference(INTERNAL);  // <<< Use internal 1.1V reference
}

void loop() {
  float Vo0    = analogRead(pin0) * (Vref/1023.0);
  float P0     = ((Vo0 / Vsupply) - 0.04)/0.09 + 0.04;

  float Vo1    = analogRead(pin1) * (Vref/1023.0);
  float P1     = ((Vo1 / Vsupply) - 0.04)/0.09 - 0.01;

  float diff = P0 - P1;
  time = millis() / 60000.0;
  Serial.print(time, 4);
  // Serial.print(analogRead(pin0));
  // Serial.print(",");
  // Serial.print(analogRead(pin1));
  // Serial.print(",");
  // Serial.print(0);
  // Serial.print(",");
  // Serial.print(P0,5);
  // Serial.print(",");
  // Serial.print(P1,5);
  Serial.print(",");
  Serial.println(diff, 5);

  delay(100);
}
