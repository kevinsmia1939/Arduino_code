#include <AccelStepper.h>

#define DIR_PIN           2
#define STEP_PIN          3
#define MOTOR_INTERFACE   1  // using a driver (step+dir)

AccelStepper stepper(MOTOR_INTERFACE, STEP_PIN, DIR_PIN);

// timing & speed control
const unsigned long speedInterval = 5000;  // 5 s
unsigned long       lastSpeedUpdate = 0;
const int           speedIncrement  = 400;
const int           maxSpeed        = 1200;
int                 currentSpeed    = 0;
const int           startspeed = 400;
// trigger input
const int triggerPin = A0;
bool      motorStarted = false;

void setup() {
  Serial.begin(9600);
  pinMode(triggerPin, INPUT);

  // prepare the stepper but do not start it yet
  stepper.setMaxSpeed(startspeed);        // start at zero
  stepper.setAcceleration(1000); // smooth ramping
  // set a huge move target so run() will spin it continuously once started
  stepper.moveTo(1L << 30);
}

void loop() {
  // if not yet started, watch for a HIGH on A0
  if (!motorStarted && digitalRead(triggerPin) == HIGH) {
    motorStarted = true;
    lastSpeedUpdate = millis();
    currentSpeed = startspeed;
    stepper.setMaxSpeed(startspeed);
    Serial.println("Trigger detected — motor starting");
  }

  // once started, handle speed ramp & stepping regardless of A0
  if (motorStarted) {
    unsigned long now = millis();
    if (now - lastSpeedUpdate >= speedInterval) {
      lastSpeedUpdate = now;
      currentSpeed += speedIncrement;
      if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
      stepper.setMaxSpeed(currentSpeed);
      Serial.print("Speed updated to ");
      Serial.print(currentSpeed);
      Serial.println(" steps/s");
    }

    // non-blocking run
    stepper.run();
  }
}
