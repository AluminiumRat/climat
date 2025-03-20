#include "EEPROM.h"

#include "error.hpp"
#include "state.hpp"

#define EEPROM_STATE_HEADER 11335
#define STATE_ADDRESS 0

static RegulatorMode regulatorMode = MODE_POWER;

static volatile int desiredPower = MIN_POWER;
static volatile float desiredTemperature = 20;

static volatile unsigned long saveTime = 0;

struct StateDataBlock
{
    int header;
    int desiredPower;
    float desiredTemperature;
    RegulatorMode regulatorMode;
};

void initState()
{
    StateDataBlock dataBlock;
    EEPROM.get(STATE_ADDRESS, dataBlock);

    if(dataBlock.header != EEPROM_STATE_HEADER) return;

    desiredPower = dataBlock.desiredPower;
    currentPower = desiredPower;
    desiredTemperature = dataBlock.desiredTemperature;
    regulatorMode = dataBlock.regulatorMode;
}

void updateState()
{
    if(saveTime != 0 && millis() >= saveTime)
    {
        StateDataBlock dataBlock;
        dataBlock.header = EEPROM_STATE_HEADER;
        dataBlock.desiredPower = desiredPower;
        dataBlock.desiredTemperature = desiredTemperature;
        dataBlock.regulatorMode = regulatorMode;
        EEPROM.put(STATE_ADDRESS, dataBlock);

        saveTime = 0;
    }
}

void sheduleSaveState()
{
    saveTime = millis() + SAVE_INTERVAL;
}

RegulatorMode getRegulatorMode()
{
    return regulatorMode;
}

void changeRegulatorMode()
{
    if(regulatorMode == MODE_POWER)
    {
        regulatorMode = MODE_TEMPERATURE;
    }
    else
    {
        regulatorMode = MODE_POWER;
        desiredPower = desiredPower / 5 * 5;
    }
    sheduleSaveState();
}

int getDesiredPower()
{
    return desiredPower;
}

void setDesiredPower(int newValue)
{
    if(newValue < MIN_POWER) newValue = MIN_POWER;
    if(newValue > MAX_POWER) newValue = MAX_POWER;

    desiredPower = newValue;
}

int getDesiredTemperature()
{
    return desiredTemperature;
}

void setDesiredTemperature(int newValue)
{
    if(newValue < MIN_DESIRED_TEMPERATURE) newValue = MIN_DESIRED_TEMPERATURE;
    if(newValue > MAX_DESIRED_TEMPERATURE) newValue = MAX_DESIRED_TEMPERATURE;

    desiredTemperature = newValue;
}