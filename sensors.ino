#include "OneWire.h"
#include "DallasTemperature.h"

#include "error.hpp"
#include "pins.hpp"
#include "sensors.hpp"

// Интервал времени между измерениями температуры. Миллисекунды
#define TEMPERATURE_MEASURE_TIME 5000

// Минимальная температура салона, при которой температура воздуха
// за печкой начинает влиять на ощущаемую температуру
#define MIN_FLOW_INFLUENCE_TEMPERATURE 10.f
// Максимальная температура салона, при которой температура салона
// влияет на ощущаеммую температуру
#define MAX_INSIDE_INFLUENCE_TEMPERATURE 20.f 

static OneWire temperatureSensorsWire(DATA_SENSOR_PIN);
static DallasTemperature temperatureSensors(&temperatureSensorsWire);

// Датчик температуры в салоне
static DeviceAddress insideThermometer;
float insideTemperature = NO_TEMPERATURE;

// Датчик температуры приточного воздуха
static DeviceAddress outsideThermometer;
float outsideTemperature = NO_TEMPERATURE;

// Датчик температуры воздуха на выходе из печки
static DeviceAddress flowThermometer;
float flowTemperature = NO_TEMPERATURE;

// Ощущаемая температура. Эвристика, вычисленная по результатам
// измерений. Должна отражать то, что чувствует человек.
float feltTemperature = NO_TEMPERATURE;

// Время, когда надо будет опрашивать датчики о проделанных измерениях
static unsigned long temperatureMeasureFinishTime = 0;

float getInsideTemperature()
{
    return insideTemperature;
}

float getOutsideTemperature()
{
    return outsideTemperature;
}

float getFlowTemperature()
{
    return flowTemperature;
}

float getFeltTemperature()
{
    return feltTemperature;
}

void initSensors()
{
    // GND power pin
    pinMode(GND_SENSOR_PIN, OUTPUT);
    digitalWrite(GND_SENSOR_PIN, LOW);

    // +5V power pin
    pinMode(VCC_SENSOR_PIN, OUTPUT);
    digitalWrite(VCC_SENSOR_PIN, HIGH);

    temperatureSensors.begin();

    int sensorsCount = temperatureSensors.getDeviceCount();
    if(sensorsCount != 3)
    {
        setError(TEMPERATURE_SENSORS_NOT_FOUND);
        return;
    }

    if (!temperatureSensors.getAddress(insideThermometer, 0))
    {
        setError(INSIDE_TEMPERATURE_SENSOR_ERROR);
        return;
    }
    if (!temperatureSensors.getAddress(outsideThermometer, 1))
    {
        setError(OUTSIDE_TEMPERATURE_SENSOR_ERROR);
        return;
    }
    if (!temperatureSensors.getAddress(flowThermometer, 2))
    {
        setError(FLOW_TEMPERATURE_SENSOR_ERROR);
        return;
    }
}

void updateFeltTemperature()
{
    if(insideTemperature == NO_TEMPERATURE || flowTemperature == NO_TEMPERATURE)
    {
        feltTemperature = NO_TEMPERATURE;
        return;
    }

    // Ощущаеммая температура получается смешиванием
    // измеренной салонной температуры и температуры воздуха на
    // выходе из печки
    if(insideTemperature < MIN_FLOW_INFLUENCE_TEMPERATURE)
    {
        feltTemperature = insideTemperature;
        return;
    }
    if(insideTemperature > MAX_INSIDE_INFLUENCE_TEMPERATURE)
    {
        feltTemperature = flowTemperature;
        return;
    }

    float mixFactor = insideTemperature - MIN_FLOW_INFLUENCE_TEMPERATURE;
    mixFactor /= MAX_INSIDE_INFLUENCE_TEMPERATURE - MIN_FLOW_INFLUENCE_TEMPERATURE;
    feltTemperature = mixFactor * flowTemperature;
    feltTemperature += (1.f - mixFactor) * insideTemperature;
}

void updateSensors()
{
    if(getError() != NO_ERROR) return;

    if(temperatureMeasureFinishTime == 0)
    {
        // Первый вход, либо измерение было закончено на предыдущем цикле
        // Посылаем датчикам запрос на измерение
        temperatureSensors.requestTemperatures();
        temperatureMeasureFinishTime = millis() + TEMPERATURE_MEASURE_TIME;
    }
    else
    {
        if(millis() >= temperatureMeasureFinishTime)
        {
            // Вышло время ожидания. Пора получать результаты измерений
            temperatureMeasureFinishTime = 0;

            insideTemperature =
                temperatureSensors.getTempC(insideThermometer);
            if(insideTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                flowTemperature = NO_TEMPERATURE;
                setError(INSIDE_TEMPERATURE_SENSOR_ERROR);
            }

            outsideTemperature =
                temperatureSensors.getTempC(outsideThermometer);
            if(outsideTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                flowTemperature = NO_TEMPERATURE;
                setError(OUTSIDE_TEMPERATURE_SENSOR_ERROR);
            }
            
            flowTemperature =
                temperatureSensors.getTempC(flowThermometer);
            if(flowTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                flowTemperature = NO_TEMPERATURE;
                setError(FLOW_TEMPERATURE_SENSOR_ERROR);
            }                
        }
        updateFeltTemperature();
    }
}