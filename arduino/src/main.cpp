#include "Arduino.h"
#include <HCSR04.h>

// Initialize the wires
const byte RED_WIRE = 7;
const byte YELLOW_WIRE = 8;
const byte BLUE_WIRE = 9;

// Initialize the ultrasonic sensor
const byte TRIGGER_PIN = 12;
const byte ECHO_PIN = 11;
UltraSonicDistanceSensor distanceSensor(TRIGGER_PIN, ECHO_PIN);

// Initialize the joystick
const byte JOYSTICK_X = 1;
const byte JOYSTICK_Y = 0;

// Game logic
bool bombActive = true;
bool redWireCut = false;
bool yellowWireCut = false;
bool blueWireCut = false;

// Joystick variables
String sequence = "";

void processUltrasonic(int wire) {
  if (redWireCut) {
    return;
  }

  float distance = distanceSensor.measureDistanceCm();
  // Serial.println(distance);

  if (wire == 0 && !redWireCut) {
    redWireCut = true;
    if (distance > 20 && distance < 30) {
      Serial.println("Red Wire has been defused!");
    } else {
      Serial.println("BOOOM!");
      Serial.println(distance);
      bombActive = false;
    }
  }
}

void processJoystick(int x, int y, int wire) {
  if (yellowWireCut) {
    return;
  }

  if (wire == 0 && !yellowWireCut) {
    yellowWireCut = true;
    if (sequence == "TBLR") {
      Serial.println("Yellow Wire has been defused!");
    } else {
      Serial.println("BOOOM!");
      Serial.println(sequence);
      bombActive = false;
    }
  }

  static bool wasTop = false;
  static bool wasBottom = false;
  static bool wasRight = false;
  static bool wasLeft = false;
  int minBoarder = 100;
  int maxBoarder = 900;

  // --- X-AXIS ---
  if (x == 0) {
    return;
  }
  if (x > maxBoarder) {
    if (!wasRight) {
      sequence += "T";
      wasRight = true;
    }
  } else if (x < minBoarder) {
    if (!wasLeft) {
      sequence += "B";
      wasLeft = true;
    }
  } else {
    wasRight = false;
    wasLeft = false;
  }

  // --- Y-AXIS ---
  if (y == 0) {
    return;
  }
  if (y > maxBoarder) {
    if (!wasTop) {
      sequence += "R";
      wasTop = true;
    }
  } else if (y < minBoarder) {
    if (!wasBottom) {
      sequence += "L";
      wasBottom = true;
    }
  } else {
    wasTop = false;
    wasBottom = false;
  }

  // Serial.println(sequence);
}

void processHumidity(int humidity, int wire) {
  if (blueWireCut) {
    return;
  }

  if (wire == 0 && !blueWireCut) {
    blueWireCut = true;
    if (humidity > 70.0) {
      Serial.println("Blue Wire has been defused!");
    } else {
      Serial.println("BOOOM!");
      Serial.println(humidity);
      bombActive = false;
    }
  }
}

void setup() {
  Serial.begin(9600); // We initialize serial connection so that we could print
                      // values from sensor.
  pinMode(RED_WIRE, INPUT_PULLUP);
  pinMode(YELLOW_WIRE, INPUT_PULLUP);

  Serial.println("--------------------");
  Serial.println("Bomb has been armed!");

}

void loop() {

  int redWire = !digitalRead(RED_WIRE);
  int yellowWire = !digitalRead(YELLOW_WIRE);
  int blueWire = !digitalRead(BLUE_WIRE);

  // Logicpuzzle Joystick
  double xAxis = analogRead(JOYSTICK_X);
  double yAxis = analogRead(JOYSTICK_Y);

  processUltrasonic(redWire);
  processJoystick(xAxis, yAxis, yellowWire);

  // Reset the bomb
  if (bombActive && redWireCut && yellowWireCut) {
    bombActive = false;
    Serial.println("Bomb has been defused!");
  }
}
