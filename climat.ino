#include "avr/wdt.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Servo.h"
#include "Wire.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include "common.hpp"
#include "error.hpp"
#include "state.hpp"

//-----------------------------------------------------------------------------------
// Temp sensors
#define DATA_SENSOR_PIN 5
#define VCC_SENSOR_PIN 7
#define GND_SENSOR_PIN 6
OneWire temperatureSensorsWire(DATA_SENSOR_PIN);
DallasTemperature temperatureSensors(&temperatureSensorsWire);

DeviceAddress insideThermometer;
DeviceAddress outsideThermometer;

#define TEMPERATURE_MEASURE_TIME 1500
volatile unsigned long temperatureMeasureFinishTime = 0;

#define NO_TEMPERATURE -300
float insideTemperature = NO_TEMPERATURE;
float outsideTemperature = NO_TEMPERATURE;

void initTemperatureSensors()
{
  //Serial.println("Initializing temperature sensors...");
  
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
    //Serial.println("ERROR: Sensors count is not 2");
    setError(TEMPERATURE_SENSORS_NOT_FOUND);
    return;
  }

  if (!temperatureSensors.getAddress(insideThermometer, 0))
  {
    //Serial.println("Unable to find address for inside termometer");
    setError(INSIDE_TEMPERATURE_SENSOR_ERROR);
    return;
  }
  if (!temperatureSensors.getAddress(outsideThermometer, 1))
  {
    //Serial.println("Unable to find address for outside termomenter");
    setError(OUTSIDE_TEMPERATURE_SENSOR_ERROR);
    return;
  }

  //Serial.println("The themperature sensors have been initialized.");
}

void updateTemperatures()
{
  if(getError() != NO_ERROR) return;

  if(temperatureMeasureFinishTime == 0)
  {
    temperatureSensors.requestTemperatures(); // Send the command to get temperatures
    temperatureMeasureFinishTime = millis() + TEMPERATURE_MEASURE_TIME;
  }
  else
  {
    if(millis() >= temperatureMeasureFinishTime)
    {
      temperatureMeasureFinishTime = 0;

      insideTemperature = temperatureSensors.getTempC(insideThermometer);
      if(insideTemperature == DEVICE_DISCONNECTED_C)
      {
        //Serial.println("ERROR: Inside thermometer is disconnected");
        insideTemperature = NO_TEMPERATURE;
        outsideTemperature = NO_TEMPERATURE;
        setError(INSIDE_TEMPERATURE_SENSOR_ERROR);
      }

      outsideTemperature = temperatureSensors.getTempC(outsideThermometer);
      if(outsideTemperature == DEVICE_DISCONNECTED_C)
      {
        //Serial.println("ERROR: Outside thermometer is disconnected");
        insideTemperature = NO_TEMPERATURE;
        outsideTemperature = NO_TEMPERATURE;
        setError(OUTSIDE_TEMPERATURE_SENSOR_ERROR);
      }      
    }
  }
}

//-----------------------------------------------------------------------------------
//Servo
#define SERVO_PIN 11
#define SERVO_MIN_VALUE 0
#define SERVO_MAX_VALUE 110
Servo valveServo;

int currentPower = MIN_POWER;

void initServo()
{
  currentPower = getDesiredPower();
  //Serial.println("Initializing servo...");

  if (valveServo.attach(SERVO_PIN) == INVALID_SERVO)
  {
    //Serial.println("Unable to init servo");
    setError(SERVO_ERROR);
    return;
  }

  //Serial.println("Servo has been initialized.");
}

void updateServo()
{
  if(getError() != NO_ERROR) return;
  if(currentPower > getDesiredPower()) currentPower--;
  if(currentPower < getDesiredPower()) currentPower++;
  int servoValue = map(currentPower, MIN_POWER, MAX_POWER, SERVO_MAX_VALUE, SERVO_MIN_VALUE);
  valveServo.write(servoValue);
}

//-----------------------------------------------------------------------------------
//Encoder
#define ENCODER_MAIN_PIN 2
#define ENCODER_SECOND_PIN 4
#define ENCODER_KEY_PIN 3
#define POWER_STEP ((MAX_POWER - MIN_POWER) / 20)
#define SUPRESSION_INTERVAL 1000

volatile unsigned long lastModeChangeTime = 0;

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
  //Serial.println("Initializing encoder...");

  pinMode(ENCODER_MAIN_PIN, INPUT);
  pinMode(ENCODER_SECOND_PIN, INPUT);
  uint8_t interruptNumber = digitalPinToInterrupt(ENCODER_MAIN_PIN);
  attachInterrupt(interruptNumber, &updateEncoder, FALLING);

  pinMode(ENCODER_KEY_PIN, INPUT);
  interruptNumber = digitalPinToInterrupt(ENCODER_KEY_PIN);
  attachInterrupt(interruptNumber, &onEncoderPressed, FALLING);

  //Serial.println("Encoder has been initialized.");
}

//-----------------------------------------------------------------------------------
//Display
Adafruit_SSD1306 display(128, 32, &Wire, 4);
bool displayIsInitialized = false;

void initDisplay()
{
  //Serial.println("Initializing display...");

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    //Serial.println("WARNING: Unable to initialize display.");
    return;
  }

  display.setTextColor(SSD1306_WHITE);

  displayIsInitialized = true;
  //Serial.println("Display has been initialized.");
}

void printError()
{
  display.setTextSize(2, 3);
  display.setCursor(5, 8);

  switch(getError())
  {
  case NO_ERROR:
    break;
  case TEMPERATURE_SENSORS_NOT_FOUND:
    display.print("NO SENSORS");
    break;
  case INSIDE_TEMPERATURE_SENSOR_ERROR:
    display.print("ERR I_TEM");
    break;
  case OUTSIDE_TEMPERATURE_SENSOR_ERROR:
    display.print("ERR O_TEM");
    break;
  case SERVO_ERROR:
    display.print("ERR SERVO");
    break;
  default:
    display.print("ERROR: ");
    display.print(getError());
  }
}

void printRegulatorMode()
{
  display.setTextSize(3, 4);
  display.setCursor(5, 5);

  if(getRegulatorMode() == MODE_POWER) display.print("MANUAL");
  else display.print("AUTO");
}

void showDesired()
{
  display.setTextSize(2, 3);
  display.setCursor(5, 8);

  if(getRegulatorMode() == MODE_POWER)
  {
    display.print("Power ");
    display.print(getDesiredPower());
    display.print("%");
  }
  else
  {
    display.print("DesT ");
    display.print(getDesiredTemperature());
    display.print("C");
  }
}

void printCommonInfo()
{
  display.setTextSize(2, 3);
  display.setCursor(8, 8);

  // Mode. Blinking for work indication
  unsigned long currentTime = millis();
  if((currentTime / 1000) % 2 == 0)
  {
    // Hide mode each even second.
    display.print(" ");
  }
  else
  {
    if(getRegulatorMode() == MODE_POWER) display.print("m");
    else display.print("a");
  }

  // Display current inside temperature
  int showedTemperature = round(insideTemperature);
  if(showedTemperature >= 0) display.print(" ");
  display.print(showedTemperature);
  display.print("C");

  // Display current outside temperature
  display.setTextSize(2, 2);
  display.setCursor(80, 0);
  display.print(round(outsideTemperature));
  display.print("C");

  // Display current power
  display.setTextSize(2, 2);
  display.setCursor(80, 16);
  display.print(getDesiredPower());
  display.print("%");  
}

void printState()
{
  if(needShowRegulatorMode())
  {
    printRegulatorMode();
    return;
  }

  if(needShowDesired())
  {
    showDesired();
    return;
  }

  if(getError() != NO_ERROR)
  {
    printError();
    return;
  }

  // Initialization. No errors but no meassures.
  if(insideTemperature == NO_TEMPERATURE)
  {
    printRegulatorMode();
    return;
  }

  printCommonInfo();
}

void updateScreen()
{
  if(!displayIsInitialized) return;

  display.clearDisplay();

  printState();

  display.display();
}

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
  int delta = getDesiredTemperature() - outsideTemperature;
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
  float delta = getDesiredTemperature() - insideTemperature;
  int deltaPower = MAX_POWER * (delta / PROPORTIONAL_REGULATOR_DIAPASON);
  setDesiredPower(getDesiredPower() + deltaPower);
}

void updateRegulator()
{
  if(getError() != NO_ERROR) return;
  if(getRegulatorMode() != MODE_TEMPERATURE) return;
  if(insideTemperature == NO_TEMPERATURE) return;
  if(outsideTemperature == NO_TEMPERATURE) return;

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
  initTemperatureSensors();
  initServo();
  initEncoder();

  wdt_enable(WDTO_8S);

  //Serial.println("Started");
}

void loop()
{
  updateTemperatures();
  updateState();
  updateRegulator();
  updateServo();
  if((stepIndex % SCREEN_UPDATE_RATE) == 0) updateScreen();

  if(getError() == NO_ERROR) wdt_reset();
  delay(STEP_DELAY_TIME);
  stepIndex++;
}
