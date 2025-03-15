#include "avr/wdt.h"

#include "display.hpp"
#include "encoder.hpp"
#include "error.hpp"
#include "regulator.hpp"
#include "sensors.hpp"
#include "state.hpp"

// Задержка в основном цикле программы
#define STEP_DELAY_TIME 100

//-----------------------------------------------------------------------------------
//Main
void setup()
{
    initState();
    initDisplay();
    initSensors();
    initServo();
    initEncoder();

    wdt_enable(WDTO_8S);
}

void loop()
{
    updateSensors();
    updateState();
    updateRegulator();
    updateServo();
    updateDisplay();

    if(getError() == NO_ERROR) wdt_reset();
    delay(STEP_DELAY_TIME);
}
