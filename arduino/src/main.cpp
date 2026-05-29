#include "Arduino.h"
#include <HCSR04.h>

const int nWires = 2;
bool wireStates[nWires] = {0, 0};
bool wireStateChanges[nWires] = {0, 0};
const int wireNumbers[nWires] = {12, 11};

const byte triggerPin = 12;
const byte echoPin = 11;
UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);

enum Wires
{
  RED = 0,
  BLUE = 1
};

void SetPinStates()
{
  for (int i = 0; i < nWires; i++)
  {
    bool state = digitalRead(wireNumbers[i]);
    wireStateChanges[i] = state != wireStates[i];
    wireStates[i] = state;
  }
}

void initPinStates()
{
  for (int i = 0; i < nWires; i++)
  {
    pinMode(wireNumbers[i], INPUT_PULLUP);
  }
}

void setup()
{
  Serial.begin(9600);
  initPinStates();
}

float getDistance()
{
  return distanceSensor.measureDistanceCm();
}

void checkDistance()
{
  if (wireStates[Wires::RED] && wireStateChanges[Wires::RED])
  {
    if (abs(getDistance() - 25) < 5)
    {
      Serial.println("Bomb has been defused!");
    }
    else
    {
      Serial.println("BOOOOOOOOOOOOOOOOOOOM!");
    }
  }
}

void checkResistance()
{
  float voltage = analogRead(A0) * 2 / 1023.0;
  Serial.println(voltage);
}

void loop()
{
  SetPinStates();

  //checkDistance();

  checkResistance();
}
