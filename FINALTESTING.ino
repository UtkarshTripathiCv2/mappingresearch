#include <Wire.h>
#include <Arduino.h>

// ========= I2C BUSES =========
TwoWire I2C_MPU = TwoWire(0);   // Bus 0
TwoWire I2C_QMC = TwoWire(1);   // Bus 1

#define MPU_ADDR 0x68
#define QMC_ADDR 0x0D

// ========= ULTRASONIC =========
const int TRIG = 5;
const int ECHO = 18;

// ========= MOTOR DRIVER =========
const int ENA = 32;
const int IN1 = 25;
const int IN2 = 26;

const int ENB = 33;
const int IN3 = 27;
const int IN4 = 14;

// ========= SETUP =========
void setup() {
  Serial.begin(115200);

  // --- Init I2C buses ---
  I2C_MPU.begin(4, 15);     // MPU → Pins 4,15
  I2C_QMC.begin(21, 22);    // QMC → Pins 21,22

  Serial.println("===== FULL HARDWARE TEST START =====");

  // --- MPU6050 ---
  I2C_MPU.beginTransmission(MPU_ADDR);
  if (I2C_MPU.endTransmission() == 0) {
    Serial.println("[OK] MPU6050 Found");

    I2C_MPU.beginTransmission(MPU_ADDR);
    I2C_MPU.write(0x6B);
    I2C_MPU.write(0);
    I2C_MPU.endTransmission();
  } else {
    Serial.println("[ERROR] MPU6050 NOT FOUND");
  }

  // --- QMC5883L ---
  I2C_QMC.beginTransmission(QMC_ADDR);
  if (I2C_QMC.endTransmission() == 0) {
    Serial.println("[OK] QMC5883L Found");

    // Reset
    I2C_QMC.beginTransmission(QMC_ADDR);
    I2C_QMC.write(0x0B);
    I2C_QMC.write(0x01);
    I2C_QMC.endTransmission();

    // Continuous mode
    I2C_QMC.beginTransmission(QMC_ADDR);
    I2C_QMC.write(0x09);
    I2C_QMC.write(0x1D);
    I2C_QMC.endTransmission();
  } else {
    Serial.println("[ERROR] QMC5883L NOT FOUND");
  }

  // --- Ultrasonic ---
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // --- Motors ---
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  Serial.println("===== SETUP DONE =====");
}

// ========= DISTANCE =========
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  return duration * 0.034 / 2;
}

// ========= MOTOR CONTROL =========
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 150);
  analogWrite(ENB, 150);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ========= LOOP =========
void loop() {

  // ===== MPU6050 =====
  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(0x47);
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom(MPU_ADDR, 2);

  float gyroZ = 0;
  if (I2C_MPU.available() >= 2) {
    int16_t gz = I2C_MPU.read() << 8 | I2C_MPU.read();
    gyroZ = gz / 131.0;
  }

  // ===== QMC5883L =====
  float headingDeg = 0;

  I2C_QMC.beginTransmission(QMC_ADDR);
  I2C_QMC.write(0x00);
  I2C_QMC.endTransmission();

  I2C_QMC.requestFrom(QMC_ADDR, 6);

  if (I2C_QMC.available() == 6) {
    int16_t x = I2C_QMC.read() | (I2C_QMC.read() << 8);
    int16_t y = I2C_QMC.read() | (I2C_QMC.read() << 8);

    float heading = atan2(y, x);
    if (heading < 0) heading += 2 * PI;
    headingDeg = heading * 180 / PI;
  }

  // ===== ULTRASONIC =====
  float distance = getDistance();

  // ===== PRINT EVERYTHING =====
  Serial.print("GyroZ: "); Serial.print(gyroZ);
  Serial.print(" | Heading: "); Serial.print(headingDeg);
  Serial.print(" | Distance: "); Serial.print(distance);
  Serial.println(" cm");

  // ===== MOTOR LOGIC =====
  if (distance > 20) {
    Serial.println("Moving Forward");
    forward();
  } else {
    Serial.println("STOP - Obstacle");
    stopMotors();
  }

  Serial.println("--------------------------");
  delay(500);
}
