#include "avr/wdt.h"

#include "common.hpp"
#include "display.hpp"
#include "encoder.hpp"
#include "error.hpp"
#include "regulator.hpp"
#include "sensors.hpp"
#include "state.hpp"

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
