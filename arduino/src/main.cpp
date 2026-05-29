#include "Arduino.h"
#include <HCSR04.h>

const byte RED = 7;

// Initialize the ultrasonic sensor
const byte TRIGGER_PIN = 12;
const byte ECHO_PIN = 11;
UltraSonicDistanceSensor distanceSensor(TRIGGER_PIN, ECHO_PIN);

// Initialize the joystick
const byte JOYSTICK_X = 1;
const byte JOYSTICK_Y = 0;


bool bombActive = true;

int xSteps = 0;
bool resetX = true;
int ySteps = 0;
bool resetY = true;

void setup() {
  Serial.begin(9600); // We initialize serial connection so that we could print values from sensor.
  pinMode(RED, INPUT_PULLUP);
}

void loop() {

  int redWire = !digitalRead(RED);

  // Logicpuzzle Ultrasonic
  // float distance = distanceSensor.measureDistanceCm();

  // if (redWire == 0 && bombActive) {
  //   bombActive = false;
  //   if (distance > 20 && distance < 30) {
  //     Serial.println("Bomb has been defused!");
  //   } else {
  //     Serial.println("BOOOOOOOOOOOOOOOOOOOM!");
  //   }
  // }

  // Logicpuzzle Joystick
  double xAxis = analogRead(JOYSTICK_X);
  double yAxis = analogRead(JOYSTICK_Y);

  if (resetX) {
    if (xAxis > 800) {
      xSteps++;
      resetX = false;
    }
    if (xAxis < 200) {
      xSteps--;
      resetX = false;
    }
  }

  if (xAxis > 400 && xAxis < 600) {
    resetX = true;
  }

  if (resetY) {
    if (yAxis > 800) {
      ySteps++;
      resetY = false;
    }
    if (yAxis < 200) {
      ySteps--;
      resetY = false;
    }
  }

  if (yAxis > 400 && yAxis < 600) {
    resetY = true;
  }

  // Reset the bomb
  if (redWire == 1) {
    bombActive = true;
  }
}