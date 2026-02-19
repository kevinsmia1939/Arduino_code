#include <Arduino.h>
#include <math.h>

const float   SAMPLE_INTERVAL_MS = 20.0;   // 50 Hz update rate
const float   FREQ_HZ            = 1.0;    // sine wave frequency
const float   AMPLITUDE          = 2.0;    // peak amplitude of sine
unsigned long lastMicros        = 0;

void setup() {
  Serial.begin(9600);
  delay(200);
  while(!Serial){}  // wait for Serial on some boards
  lastMicros = micros();
}

void loop() {
  unsigned long now = micros();
  float dt = (now - lastMicros) / 1000.0;     // delta in ms
  if (dt < SAMPLE_INTERVAL_MS) return;        // wait until next sample
  lastMicros = now;

  float t = now / 1e6;                        // time in seconds since start
  float y = AMPLITUDE * sin(2.0 * PI * FREQ_HZ * t);

  // Print: <time>, <sine value>
  Serial.print(t, 1);
  Serial.print(", ");
  Serial.println(y, 2);
  delay(100);
}
