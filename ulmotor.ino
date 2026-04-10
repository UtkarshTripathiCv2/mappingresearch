#include <Arduino.h>

// --- Ultrasonic Pins ---
const int TRIG = 5;
const int ECHO = 18;

// --- Motor Driver Pins ---
const int ENA = 32;
const int IN1 = 25;
const int IN2 = 26;

const int ENB = 33;
const int IN3 = 27;
const int IN4 = 14;

void setup() {
  Serial.begin(115200);

  // Ultrasonic
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Motors
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  Serial.println("Motor + Ultrasonic Test Start");
}

// --- Function to measure distance ---
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  float distance = duration * 0.034 / 2;

  return distance;
}

// --- Motor Functions ---
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void loop() {
  float distance = getDistance();

  Serial.print("Distance: ");
  if (distance <= 0 || distance > 400) {
    Serial.println("Out of range");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  // --- Logic ---
  if (distance > 20) {
    Serial.println("Moving Forward");
    forward();
  } else {
    Serial.println("Obstacle Detected - STOP");
    stopMotors();
  }

  delay(500);
}
