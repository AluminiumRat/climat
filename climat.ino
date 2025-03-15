#include "avr/wdt.h"

#include "common.hpp"
#include "display.hpp"
#include "encoder.hpp"
#include "error.hpp"
#include "sensors.hpp"
#include "state.hpp"

//-----------------------------------------------------------------------------------
//Auto regulator
#define PROPORTIONAL_REGULATOR_DIAPASON 6

struct PowerTableRecord
{
  int deltaTemp;
  int power;
};
PowerTableRecord powerTable[] = { {0, 0},
                                  {1, 5},
                                  {20, 40},
                                  {40, 100}};
const int powerTableSize = sizeof(powerTable) / sizeof(powerTable[0]);

void updateByOutsideDesiredDelta()
{
  int delta = getDesiredTemperature() - getOutsideTemperature();
  if(delta <= powerTable[0].deltaTemp)
  {
    setDesiredPower(powerTable[0].power);
    return;
  }

  if(delta >= powerTable[powerTableSize - 1].deltaTemp)
  {
    setDesiredPower(powerTable[powerTableSize - 1].power);
    return;
  }

  int i = 1;
  for(; i < powerTableSize - 1; i++)
  {
    if(powerTable[i].deltaTemp > delta) break;
  }

  setDesiredPower(map(delta,
                      powerTable[i-1].deltaTemp,
                      powerTable[i].deltaTemp,
                      powerTable[i-1].power,
                      powerTable[i].power));
}

void correctByInsideDesiredDelta()
{
  float delta = getDesiredTemperature() - getInsideTemperature();
  int deltaPower = MAX_POWER * (delta / PROPORTIONAL_REGULATOR_DIAPASON);
  setDesiredPower(getDesiredPower() + deltaPower);
}

void updateRegulator()
{
  if(getError() != NO_ERROR) return;
  if(getRegulatorMode() != MODE_TEMPERATURE) return;
  if(getInsideTemperature() == NO_TEMPERATURE) return;
  if(getOutsideTemperature() == NO_TEMPERATURE) return;

  updateByOutsideDesiredDelta();

  correctByInsideDesiredDelta();
}

//-----------------------------------------------------------------------------------
//Main
#define STEP_DELAY_TIME 100
#define SCREEN_UPDATE_RATE 5
int stepIndex = 0;

void setup()
{
  //Serial.begin(9600);
  //Serial.println("Starting...");

  initState();
  initDisplay();
  initSensors();
  initServo();
  initEncoder();

  wdt_enable(WDTO_8S);

  //Serial.println("Started");
}

void loop()
{
  updateSensors();
  updateState();
  updateRegulator();
  updateServo();
  if((stepIndex % SCREEN_UPDATE_RATE) == 0) updateDisplay();

  if(getError() == NO_ERROR) wdt_reset();
  delay(STEP_DELAY_TIME);
  stepIndex++;
}
