#include <Wire.h>

const int MPU_ADDR = 0x68; // Standard I2C address for MPU-6500/6050

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C on Pins 4 (SDA) and 15 (SCL)
  Wire.begin(4, 15); 
  
  Serial.println("\n--- MPU-6500 Simple Test (Pins 4/15) ---");

  // Check if MPU is physically connected
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("[OK] MPU-6500 Detected!");
    
    // Wake up the MPU (it starts in sleep mode)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); // Power Management register
    Wire.write(0);    // Set to 0 to wake it up
    Wire.endTransmission();
  } else {
    Serial.println("[!!] MPU-6500 NOT FOUND. Check wiring to Pins 4 and 15.");
    while(1); // Stop here if sensor isn't found
  }
}

void loop() {
  // Read Gyroscope Z-axis data
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x47); // Gyro Z high byte register
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true);

  if (Wire.available() >= 2) {
    int16_t gz = Wire.read() << 8 | Wire.read();
    float gyroZ = gz / 131.0; // Convert to degrees per second

    Serial.print("Gyro Z: ");
    Serial.print(gyroZ);
    Serial.println(" °/s");
  }

  delay(200);
}
