#include <Arduino.h>

float initialTime = 150.0;
float time = initialTime;

bool combinations[12][8] = {
  {1, 1, 1, 1, 1, 1, 0, 0},
  {0, 1, 1, 0, 0, 0, 0, 0},
  {1, 1, 0, 1, 1, 0, 1, 0},
  {1, 1, 1, 1, 0, 0, 1, 0},
  {0, 1, 1, 0, 0, 1, 1, 0},
  {1, 0, 1, 1, 0, 1, 1, 0},
  {1, 0, 1, 1, 1, 1, 1, 0},
  {1, 1, 1, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 0},
  {1, 1, 1, 1, 0, 1, 1, 0},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

void setup() {
  // put your setup code here, to run once:
  for (int i = 2; i < 14; i++)
  {
      pinMode(i, OUTPUT);
  }
  pinMode(0, INPUT_PULLUP);
}

void setCombination(bool combination[8])
{
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(i + 6, combination[i] ? HIGH : LOW);
  }
}

void lightDigit(int digit, int number)
{
  setCombination(combinations[number]);
  digitalWrite(digit+1, LOW);
  delay(1);
  time -= 0.001;
  digitalWrite(digit+1, HIGH);
}

void lightAll(int number)
{
  setCombination(combinations[number]);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  delay(1);
  time -= 0.001;
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
}

void loop() {
  if (digitalRead(0) == LOW)
  {
    time = initialTime;
  }
  int minutes = floor(max(0, ceil(time)) / 60.0);
  int seconds = (int)max(0, ceil(time)) % 60;
  lightDigit(1, floor(minutes / 10.0));
  lightDigit(2, minutes % 10);
  lightDigit(3, floor(seconds / 10.0));
  lightDigit(4, seconds % 10);
  if (time - floor(time) > 0.5) digitalWrite(13, HIGH);
  else digitalWrite(13, LOW);
  delay(1);
  if (time == 0)
  {
    digitalWrite(1, LOW);
  }
  else digitalWrite(1, HIGH);
}
