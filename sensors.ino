#include "OneWire.h"
#include "DallasTemperature.h"

#include "error.hpp"
#include "pins.hpp"
#include "sensors.hpp"

unsigned int measureIndex = 0;

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

// Время, когда надо будет опрашивать датчики о проделанных измерениях
static unsigned long temperatureMeasureFinishTime = 0;

unsigned int getMeasureIndex()
{
    return measureIndex;
}

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

            insideTemperature = temperatureSensors.getTempC(insideThermometer);
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
            
            flowTemperature = temperatureSensors.getTempC(flowThermometer);
            if(flowTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                flowTemperature = NO_TEMPERATURE;
                setError(FLOW_TEMPERATURE_SENSOR_ERROR);
            }                

            measureIndex++;
        }
    }
}