#include "error.hpp"
#include "state.hpp"

// Коэффициенты PID регулятора
#define K_P 2.f
#define K_I 0.15f
#define K_D 40.f

// Таблица, которая определяет зависимость
// "насколько надо нагреть воздух" -> "Какую мощность печки выставить"
struct PowerTableRecord
{
  int deltaTemp;    // Разность температур входящего воздуха и
                    // желанной температуры
  int power;        // Какую мощность выставить при этой разницу
};
PowerTableRecord powerTable[] = { {0, 0},
                                  {1, 15},
                                  {5, 20},
                                  {7, 30},
                                  {30, 40},
                                  {50, 60},
                                  {58, 70},
                                  {62, 80}};
const int powerTableSize = sizeof(powerTable) / sizeof(powerTable[0]);

// Индекс последнего обработанного измерения.
// По нему пропускаем такты, где нет новых данных
unsigned int lastMeasureIndex = 0;

// Значение интеграла по ошибке температуры.
float errorIntegral = 0;

// Ошибка температуры на прошлом шаге регулирования
float lastError = NO_TEMPERATURE;
// Целевая температура на предыдущем шаге регулирования
float lastDesiredTemperature = NO_TEMPERATURE;
// Сглаженное значение производной от ошибки
float avgDError = 0;

void reset()
{
    errorIntegral = 0;
    lastError = NO_TEMPERATURE;
    lastDesiredTemperature = NO_TEMPERATURE;
    avgDError = 0;
}

// Вычислить температуру, до которой должен нагреваться воздух за отопителем
float getDesiredFlowTemperature()
{
    // На выходе печки поддерживаем температуру тем выше, чем больше разница
    // между реальной температурой в салоне  и желанной
    float needToAdd = getDesiredTemperature() - getInsideTemperature();

    // Если салона надо нагревать, то дополнительно усиливаем эффект
    if(needToAdd > 0) needToAdd *= 2;

    return getDesiredTemperature() + needToAdd;
}

// Первичное выставление мощности по температуре
float getPowerByOutsideDesiredDelta(float desiredTemperature)
{
    // Разница температур приточного воздуха и желанной температуры
    int delta = desiredTemperature - getOutsideTemperature();
    
    // Разница температур отрицательная, воздух греть не надо
    if(delta <= powerTable[0].deltaTemp)
    {
        return powerTable[0].power;
    }

    // Очень большая разница, выставляем мощность на максимум
    if(delta >= powerTable[powerTableSize - 1].deltaTemp)
    {
        return powerTable[powerTableSize - 1].power;
    }

    // Ищем в таблице 2 записи, между которыми находится
    // текущая разница температур
    int i = 1;
    for(; i < powerTableSize - 1; i++)
    {
        if(powerTable[i].deltaTemp > delta) break;
    }

    // Линейная интерполяция между двумя найденными температурами
    return map( delta,
                powerTable[i-1].deltaTemp,
                powerTable[i].deltaTemp,
                powerTable[i-1].power,
                powerTable[i].power);
}

void updateRegulator()
{
    if(getError() != NO_ERROR) return;

    if(lastMeasureIndex == getMeasureIndex()) return;
    lastMeasureIndex = getMeasureIndex();

    if(getRegulatorMode() != MODE_TEMPERATURE)
    {
        reset();
        return;
    }

    float currentFlowTemperature = getFlowTemperature();
    if(currentFlowTemperature == NO_TEMPERATURE) return;

    float currentDesiredTemperature = getDesiredFlowTemperature();
    float temperatureError = currentDesiredTemperature - currentFlowTemperature;

    // Первичное выставление по таблице
    float newPower = getPowerByOutsideDesiredDelta(currentDesiredTemperature);

    // Пропорциональное регулирование
    newPower += temperatureError * K_P;

    // Интегральное регулирование
    errorIntegral *= 0.95f;                  // Ослабляем влияние истории
    errorIntegral += temperatureError;
    newPower += errorIntegral * K_I;

    // Дифференциальное интегрирование
    if(lastError != NO_TEMPERATURE && lastDesiredTemperature != NO_TEMPERATURE)
    {
        float dError = temperatureError - lastError;
        // Защита от изменения целевой температуры
        dError -= currentDesiredTemperature - lastDesiredTemperature;
        // Сглаживание dError, чтобы избежать резких бросков
        avgDError += 0.5f * dError;
        avgDError /= 1.5f;
        newPower += avgDError * K_D;
    }

    setDesiredPower(round(newPower));

    lastError = temperatureError;
    lastDesiredTemperature = currentDesiredTemperature;
}