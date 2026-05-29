#include "Arduino.h"
#include <HCSR04.h>

const byte RED = 7;

// Initialize the ultrasonic sensor
const byte triggerPin = 12;
const byte echoPin = 11;
UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);


bool bombActive = true;

void setup() {
  Serial.begin(9600); // We initialize serial connection so that we could print values from sensor.
  pinMode(RED, INPUT_PULLUP);
}

void loop() {

  int redWire = !digitalRead(RED);
  float distance = distanceSensor.measureDistanceCm();

  if (redWire == 0 && bombActive) {
    bombActive = false;
    if (distance > 20 && distance < 30) {
      Serial.println("Bomb has been defused!");
    } else {
      Serial.println("BOOOOOOOOOOOOOOOOOOOM!");
    }
  }

  // Reset the bomb
  if (redWire == 1) {
    bombActive = true;
  }
}