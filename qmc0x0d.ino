#include <Wire.h>

// I2C Addresses
const int HMC_ADDR = 0x1E; 
const int QMC_ADDR = 0x0D;

int activeAddr = 0;
bool isQMC = false;

// Calibration variables
int16_t xMin = 32767, xMax = -32768;
int16_t yMin = 32767, yMax = -32768;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(500);

  Serial.println("\n--- Magnetometer Standalone Test ---");

  // Detection Logic
  Wire.beginTransmission(HMC_ADDR);
  if (Wire.endTransmission() == 0) {
    activeAddr = HMC_ADDR;
    isQMC = false;
    Serial.println("Detected: Original Honeywell HMC5883L (0x1E)");
    setupHMC();
  } else {
    Wire.beginTransmission(QMC_ADDR);
    if (Wire.endTransmission() == 0) {
      activeAddr = QMC_ADDR;
      isQMC = true;
      Serial.println("Detected: QMC5883L / Clone (0x0D)");
      setupQMC();
    } else {
      Serial.println("Error: No magnetometer found at 0x1E or 0x0D!");
      while (1);
    }
  }
  Serial.println("ACTION: Rotate your rover 360 degrees to calibrate...");
}

void loop() {
  int16_t x, y, z;
  readData(x, y, z);

  // Update Calibration (Hard-Iron)
  if (x < xMin) xMin = x; if (x > xMax) xMax = x;
  if (y < yMin) yMin = y; if (y > yMax) yMax = y;

  // Apply Offset
  float offsetX = (xMax + xMin) / 2.0;
  float offsetY = (yMax + yMin) / 2.0;
  float correctedX = x - offsetX;
  float correctedY = y - offsetY;

  // Calculate Heading
  float heading = atan2(correctedY, correctedX) * 180 / M_PI;
  if (heading < 0) heading += 360;

  Serial.print("Heading: "); Serial.print(heading);
  Serial.print("° | Raw X: "); Serial.print(x);
  Serial.print(" Y: "); Serial.println(y);
  
  delay(150);
}

void setupHMC() {
  Wire.beginTransmission(HMC_ADDR);
  Wire.write(0x00); Wire.write(0x70); // 8-average, 15Hz
  Wire.endTransmission();
  Wire.beginTransmission(HMC_ADDR);
  Wire.write(0x01); Wire.write(0x20); // Gain
  Wire.endTransmission();
  Wire.beginTransmission(HMC_ADDR);
  Wire.write(0x02); Wire.write(0x00); // Continuous mode
  Wire.endTransmission();
}

void setupQMC() {
  Wire.beginTransmission(QMC_ADDR);
  Wire.write(0x0B); Wire.write(0x01); // Define SET/RESET period
  Wire.endTransmission();
  Wire.beginTransmission(QMC_ADDR);
  Wire.write(0x09); Wire.write(0x1D); // Continuous mode, 200Hz, 8G, 512 oversample
  Wire.endTransmission();
}

void readData(int16_t &x, int16_t &y, int16_t &z) {
  if (!isQMC) {
    Wire.beginTransmission(HMC_ADDR);
    Wire.write(0x03); 
    Wire.endTransmission(false);
    Wire.requestFrom(HMC_ADDR, 6);
    x = Wire.read() << 8 | Wire.read();
    z = Wire.read() << 8 | Wire.read(); // HMC orders X, Z, Y
    y = Wire.read() << 8 | Wire.read();
  } else {
    Wire.beginTransmission(QMC_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(QMC_ADDR, 6);
    x = Wire.read() | Wire.read() << 8; // QMC orders X, Y, Z (LSB first)
    y = Wire.read() | Wire.read() << 8;
    z = Wire.read() | Wire.read() << 8;
  }
}
