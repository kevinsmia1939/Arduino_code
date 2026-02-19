const int pinA0 = A0;
const int pinA1 = A1;

float Vr = 5.018; //reference voltage
unsigned long lastBlinkTime = 0;
bool ledOn = false;                // current LED state
unsigned long ledOnStart = 0;      // when we turned LED on
unsigned long ledOnDuration = 1; // how long LED stays ON (ms)
const int LED_pin = 9;
const int motor_trigger_pin = 10;

void setup() {
  Serial.begin(57600);
  pinMode(LED_pin, OUTPUT);
  //pinMode(motor_trigger_pin, OUTPUT);

  digitalWrite(motor_trigger_pin, HIGH); //Trigger the motor on startup
  delay(10);
  digitalWrite(motor_trigger_pin, LOW);
  delay(90);
  digitalWrite(LED_pin, HIGH);
  delay(1);
  digitalWrite(LED_pin, LOW);
  Serial.print("T: ");
  Serial.print(millis());
  Serial.println(" LED");
  Serial.print("T: ");
  Serial.print(millis());
  Serial.println(" LED");
  // 100 ms has passed and pressure logging start
}

void loop() {
  float P0 = 12.5*(analogRead(pinA0)/Vr/1023.0)-1.25*Vr; //bar
  float P1 = 12.5*(analogRead(pinA1)/Vr/1023.0)-1.25*Vr; //bar
  float dP = (P0-P1)*100; //bar to kPa
  unsigned long time = millis();
  Serial.print(time);
  Serial.print(",");
  Serial.println(dP, 5);
  delay(10);
}
