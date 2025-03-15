#pragma once
// Сервомашинка, управляющая краном печки.
// Берет desiredPower из состояния(state.hpp) и управляет сервой

#define SERVO_MIN_VALUE 0
#define SERVO_MAX_VALUE 110

// Инициализация. Вызывается 1 раз на старте.
void initServo();
// Апдэйт. Вызывается периодически в основном цикле.
void updateServo();
