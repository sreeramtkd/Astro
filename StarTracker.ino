/*
 * High-Torque Variable Speed Control for 28BYJ-48 Stepper Motor using Arduino Nano
 * Range: 15ms (Fast) to 2000ms (Slow)
 * Pins: 7, 8, 9, 10
 * SPDT Toggle swtich for CW / CCW movement
 * Pin: 2
 */

#include <Stepper.h>
const float STEPS_PER_REV = 2048; // Full-Step Mode (High Torque)
#define DIRECTION_PIN 2
// --- Speed Settings (Time Delay in Milliseconds) ---
// Proven Fastest Speed (Potentiometer MAX)
const unsigned long DELAY_FAST = 15; 

// Slowest Possible Speed (Potentiometer MIN)
// 2000ms (2 seconds) delay between steps. One rotation takes over an hour.
const unsigned long DELAY_SLOW = 500; 

// --- Hardware Pins (Using your specified pins) ---
const int potPin = A0;
const int buzzerPin = 4;

// Motor Pins: 7, 8, 9, 10. (Sequence 7, 9, 8, 10 for 1-3-2-4 order)
Stepper myStepper(STEPS_PER_REV, 7, 9, 8, 10);

void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(DIRECTION_PIN, INPUT_PULLUP);
  // 🔊 Start-up Tone
  tone(buzzerPin, 1000, 200);
  delay(250);
  tone(buzzerPin, 2000, 400);
  
  Serial.println("--- Variable Speed Control Initialized ---");
  Serial.print("Speed Range (Delay ms): "); Serial.print(DELAY_FAST); Serial.print(" to "); Serial.println(DELAY_SLOW);
}

void loop() {
  // 1. Read Potentiometer
  int dirState = digitalRead(DIRECTION_PIN);
  int potValue = analogRead(potPin);
  int stepSize = 1;
  unsigned long currentDelay = map(potValue, 0, 1023, DELAY_SLOW, DELAY_FAST);
  if (dirState == LOW) {
    stepSize = -1;
  }
  myStepper.step(stepSize); 
  delay(currentDelay);

  // Optional Debugging
  // Serial.print("Delay: "); Serial.println(currentDelay);
}
