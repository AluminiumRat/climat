#include "Servo.h"

#include "error.hpp"
#include "pins.hpp"
#include "servo.hpp"

// Максимальный и минимальный углы поворота сервомашинки
#define SERVO_MIN_VALUE 0
#define SERVO_MAX_VALUE 110

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