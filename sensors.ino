#include "OneWire.h"
#include "DallasTemperature.h"

#include "common.hpp"
#include "error.hpp"
#include "sensors.hpp"

static OneWire temperatureSensorsWire(DATA_SENSOR_PIN);
static DallasTemperature temperatureSensors(&temperatureSensorsWire);

static DeviceAddress insideThermometer;
static DeviceAddress outsideThermometer;

static unsigned long temperatureMeasureFinishTime = 0;

float insideTemperature = NO_TEMPERATURE;
float outsideTemperature = NO_TEMPERATURE;

float getInsideTemperature()
{
    return insideTemperature;
}

float getOutsideTemperature()
{
    return outsideTemperature;
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
    if(sensorsCount != 2)
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
}

void updateSensors()
{
    if(getError() != NO_ERROR) return;

    if(temperatureMeasureFinishTime == 0)
    {
        temperatureSensors.requestTemperatures();
        temperatureMeasureFinishTime = millis() + TEMPERATURE_MEASURE_TIME;
    }
    else
    {
        if(millis() >= temperatureMeasureFinishTime)
        {
            temperatureMeasureFinishTime = 0;

            insideTemperature =
                temperatureSensors.getTempC(insideThermometer);
            if(insideTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                setError(INSIDE_TEMPERATURE_SENSOR_ERROR);
            }

            outsideTemperature =
                temperatureSensors.getTempC(outsideThermometer);
            if(outsideTemperature == DEVICE_DISCONNECTED_C)
            {
                insideTemperature = NO_TEMPERATURE;
                outsideTemperature = NO_TEMPERATURE;
                setError(OUTSIDE_TEMPERATURE_SENSOR_ERROR);
            }      
        }
    }
}