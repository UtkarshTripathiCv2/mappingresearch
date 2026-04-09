#include <Wire.h>

#define ADDR 0x0D

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Reset
  Wire.beginTransmission(ADDR);
  Wire.write(0x0B);
  Wire.write(0x01);
  Wire.endTransmission();

  // Continuous mode, 100Hz, 2G
  Wire.beginTransmission(ADDR);
  Wire.write(0x09);
  Wire.write(0x1D);
  Wire.endTransmission();

  Serial.println("QMC5883L Ready");
}

void loop() {
  Wire.beginTransmission(ADDR);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.requestFrom(ADDR, 6);

  if (Wire.available() == 6) {
    int16_t x = Wire.read() | (Wire.read() << 8);
    int16_t y = Wire.read() | (Wire.read() << 8);
    int16_t z = Wire.read() | (Wire.read() << 8);

    float heading = atan2(y, x);
    if (heading < 0) heading += 2 * PI;

    float deg = heading * 180 / PI;

    Serial.print("X: "); Serial.print(x);
    Serial.print(" Y: "); Serial.print(y);
    Serial.print(" Z: "); Serial.print(z);
    Serial.print(" Heading: ");
    Serial.println(deg);
  }

  delay(300);
}
