const int pin0 = A0;
const int pin1 = A1;

float Vs = 5.007;
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
}

void loop() {
  // Read and convert voltages
  Vo0 = analogRead(pin0) * (Vs / 1023.0);
  P0 = (((Vo0 / Vs) - 0.04) / 0.09)+0.259;

  Vo1 = analogRead(pin1) * (Vs / 1023.0);
  P1 = (((Vo1 / Vs) - 0.04) / 0.09)+0.248;

  float diff = P1 - P0;
  time = millis()/60000.0;
  Serial.print(time,5);
  // Serial.print(",");
  // Serial.print(0);
  // Serial.print(P0,5);
  Serial.print(",");
  // Serial.print(P1,5);
  // Serial.print(0.2);
  // Serial.print(",");
  Serial.println(diff,5);
  delay(100);
  // diffSum += diff;
  // sampleCount++;

  // delay(sampleInterval);

  // After 5 seconds, print average and reset
  // if (millis() - startTime >= interval) {
  //   float averageDiff = diffSum / sampleCount;
  //   Serial.print(averageDiff, 5);
  //   Serial.print("   ");
  //   Serial.print(P0, 5);
  //   Serial.print("   ");
  //   Serial.println(P1, 5);

  //   // Reset for next cycle
  //   diffSum = 0.0;
  //   sampleCount = 0;
  //   startTime = millis();
  // }
}
