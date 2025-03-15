#include "Servo.h"

#include "common.hpp"
#include "error.hpp"
#include "servo.hpp"

static Servo valveServo;

// Сглаженное по времени значение мощности печки
// Используется для плавного движения сервомашинки
static int currentPower = MIN_POWER;

void initServo()
{
  currentPower = getDesiredPower();

  if (valveServo.attach(SERVO_PIN) == INVALID_SERVO)
  {
    setError(SERVO_ERROR);
    return;
  }
}

void updateServo()
{
  if(getError() != NO_ERROR) return;
  if(currentPower > getDesiredPower()) currentPower--;
  if(currentPower < getDesiredPower()) currentPower++;
  int servoValue = map(currentPower, MIN_POWER, MAX_POWER, SERVO_MAX_VALUE, SERVO_MIN_VALUE);
  valveServo.write(servoValue);
}