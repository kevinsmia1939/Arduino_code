#include <SPI.h>

// —– USER CONFIGURATION —–
// SPI chip-select pin (you can use any digital pin; here we use the hardware SS pin)
const uint8_t CS_PIN = 10;

// Sensor pressure range, in psi
const float PRESSURE_MIN = 0.0f;
const float PRESSURE_MAX = 30.0f;

// The SSC digital output spans 14 bits (0..16383), with
// Pmin@10% and Pmax@90% of full-scale counts. 2^14 = 16384.
const float COUNTS_FULL_SCALE = 16384.0f;
const float COUNTS_OFFSET     = 0.10f * COUNTS_FULL_SCALE; // 10% point
const float COUNTS_SPAN       = 0.80f * COUNTS_FULL_SCALE; // 90%–10% = 80%

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  // Initialize SPI (mode 0: data valid on rising edge)
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  Serial.println("SSCDANT030PASA5 SPI Pressure Reader");
}

void loop() {
  // Perform a two-byte read of the compensated pressure
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);

  uint8_t hb = SPI.transfer(0x00);  // first data byte (status + MSBs)
  uint8_t lb = SPI.transfer(0x00);  // second data byte (LSBs)

  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();

  // Parse status bits (MSB two bits of hb)
  uint8_t status = (hb >> 6) & 0x03;
  // Mask off status (keep lower 6 bits) and assemble 14-bit count
  uint16_t rawCounts = (uint16_t)(hb & 0x3F) << 8 | lb;

  if (status == 0) {
    // Eq. 2: Pressure = ((Output – Outputmin)·(Pmax–Pmin)/(Outputmax–Outputmin)) + Pmin
    // Here Outputmin = 0.1·2^14, Outputmax = 0.9·2^14
    float p = (rawCounts - COUNTS_OFFSET)
            * (PRESSURE_MAX - PRESSURE_MIN)
            / COUNTS_SPAN
            + PRESSURE_MIN;
    //Serial.print("Pressure = ");
    Serial.println(p, 5);
    //Serial.println(" psi");
  }

  delay(100);
}
