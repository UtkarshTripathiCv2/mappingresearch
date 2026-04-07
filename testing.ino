testing.ino#include <Wire.h>
#include <math.h>

// --- Configuration ---
const int MPU = 0x68;
const int HMC = 0x1E; // If this fails, try 0x0D (for QMC5883L clones)
const int TRIG = 5, ECHO = 18;
const int ENA = 32, IN1 = 25, IN2 = 26;
const int ENB = 33, IN3 = 27, IN4 = 14;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);

  Serial.println("\n==================================");
  Serial.println("   ROVER HARDWARE DIAGNOSTIC v2   ");
  Serial.println("==================================");

  // 1. MPU-6500 Wakeup
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); Wire.write(0); 
  if (Wire.endTransmission() == 0) Serial.println("[OK] MPU-6500: Connected");
  else Serial.println("[!!] MPU-6500: DISCONNECTED");

  // 2. HMC5883L Configuration
  Wire.beginTransmission(HMC);
  Wire.write(0x02); Wire.write(0x00); // Continuous mode
  if (Wire.endTransmission() == 0) Serial.println("[OK] HMC5883L: Connected");
  else Serial.println("[!!] HMC5883L: DISCONNECTED (Check address 0x1E or 0x0D)");
}

void loop() {
  Serial.println("\n--- Live Sensor Readings ---");

  // --- TEST 1: ULTRASONIC RANGE ---
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000); // 30ms timeout
  float distance = duration * 0.034 / 2;
  
  Serial.print("Distance: "); 
  if (distance > 400 || distance <= 0) Serial.println("Out of range / No Sensor");
  else { Serial.print(distance); Serial.println(" cm"); }

  // --- TEST 2: MPU-6500 GYRO (Rotation Test) ---
  Wire.beginTransmission(MPU);
  Wire.write(0x47); // Gyro Z register
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 2, true);
  int16_t gz = Wire.read() << 8 | Wire.read();
  Serial.print("Gyro Z (Rotate Rover): "); Serial.println(gz / 131.0);

  // --- TEST 3: HMC5883L (Compass Heading) ---
  Wire.beginTransmission(HMC);
  Wire.write(0x03); 
  Wire.endTransmission(false);
  Wire.requestFrom(HMC, 6, true);
  if(Wire.available() >= 6) {
    int16_t mx = Wire.read() << 8 | Wire.read();
    int16_t mz = Wire.read() << 8 | Wire.read();
    int16_t my = Wire.read() << 8 | Wire.read();
    
    // Calculate heading in degrees
    float heading = atan2(my, mx) * 180 / M_PI;
    if(heading < 0) heading += 360;
    Serial.print("Compass Heading: "); Serial.print(heading); Serial.println(" deg");
  }

  // --- TEST 4: MOTOR INDIVIDUAL DRIVE ---
  Serial.println("Testing Motor A...");
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, 160);
  delay(500); stopAll();
  
  Serial.println("Testing Motor B...");
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, 160);
  delay(500); stopAll();

  Serial.println("----------------------------------");
  delay(2000);
}

void stopAll() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
