#include "encoder.hpp"
#include "pins.hpp"

// Изменение мощности печки при повороте крутилки на 1 щелчек
// Весь диапазон за 20 щелчков
#define POWER_STEP ((MAX_POWER - MIN_POWER) / 20)

// Защита от дребезга контактов на кнопке
// Режим нельзя переключать чаще чем SUPRESSION_INTERVAL
#define SUPRESSION_INTERVAL 1000

// Время, когда в последний раз было переключен режим.
// Защита от дребезга контактов
volatile unsigned long lastModeChangeTime = 0;

// Обработка поворота крутилки
// Срабатывает по прерыванию от смены сотояния на выводе контроллера
void updateEncoder()
{
    if(digitalRead(ENCODER_SECOND_PIN) == LOW)
    {
        if(getRegulatorMode() == MODE_POWER) setDesiredPower(getDesiredPower() + POWER_STEP);
        else setDesiredTemperature(getDesiredTemperature() + 1);
    }
    else
    {
        if(getRegulatorMode() == MODE_POWER) setDesiredPower(getDesiredPower() - POWER_STEP);
        else setDesiredTemperature(getDesiredTemperature() - 1);
    }

    sheduleSaveState();
}

// Обработка нажатия на крутилку.
// Срабатывает по прерыванию от смены состояния на выводе контроллера
void onEncoderPressed()
{
    unsigned long currentTime = millis();
    if(currentTime > lastModeChangeTime + SUPRESSION_INTERVAL || currentTime < lastModeChangeTime)
    {
        changeRegulatorMode();
        lastModeChangeTime = millis();
    }
}

void initEncoder()
{
    pinMode(ENCODER_MAIN_PIN, INPUT);
    pinMode(ENCODER_SECOND_PIN, INPUT);
    uint8_t interruptNumber = digitalPinToInterrupt(ENCODER_MAIN_PIN);
    attachInterrupt(interruptNumber, &updateEncoder, FALLING);

    pinMode(ENCODER_KEY_PIN, INPUT);
    interruptNumber = digitalPinToInterrupt(ENCODER_KEY_PIN);
    attachInterrupt(interruptNumber, &onEncoderPressed, FALLING);
}