#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Wire.h"

#include "display.hpp"
#include "error.hpp"
#include "state.hpp"

static Adafruit_SSD1306 display(128, 32, &Wire, 4);
static bool displayIsInitialized = false;

void initDisplay()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        return;
    }

    display.setTextColor(SSD1306_WHITE);
    displayIsInitialized = true;
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

// Большая надпись с режимом работы
// Выводится некоторое время после смена режима
void printRegulatorMode()
{
    display.setTextSize(3, 4);
    display.setCursor(5, 5);

    if(getRegulatorMode() == MODE_POWER) display.print("MANUAL");
    else display.print("AUTO");
}

// Мощность печки либо желанная температура в зависимости от режима
// Отрисовка в течении некоторого времени после поворота крутилки
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

// Режим отрисовки, когда нет ошибок и нет никаких изменений настроек
void printCommonInfo()
{
    display.setTextSize(2, 3);
    display.setCursor(8, 8);

    // Моргающая буква - индикатор текущего режима
    unsigned long currentTime = millis();
    if((currentTime / 1000) % 2 == 0)
    {
        // Не выводим каждую четную секунду
        display.print(" ");
    }
    else
    {
        if(getRegulatorMode() == MODE_POWER) display.print("m");
        else display.print("a");
    }

    // Текущая температура в салоне
    int showedTemperature = round(getInsideTemperature());
    if(showedTemperature >= 0) display.print(" ");
    display.print(showedTemperature);
    display.print("C");

    // Текущая температура приточного воздуха
    display.setTextSize(2, 2);
    display.setCursor(80, 0);
    display.print(round(getOutsideTemperature()));
    display.print("C");

    // Текущая мощность печки
    display.setTextSize(2, 2);
    display.setCursor(80, 16);
    display.print(getDesiredPower());
    display.print("%");  
}

// Смотрим, в каком состоянии сейчас система и выводим
// нужный тип информации в зависимости от этого
void printState()
{
    if(needShowRegulatorMode())
    {
        // Недавно была смена режима и надо просигнализировать об этом
        printRegulatorMode();
        return;
    }

    if(needShowDesired())
    {
        // Недавно была смена настроек по температуре, выводим новые настройки
        showDesired();
        return;
    }

    if(getError() != NO_ERROR)
    {
        // В системе ошибка, выводим её
        printError();
        return;
    }

    if(getInsideTemperature() == NO_TEMPERATURE)
    {
        // В системе нет ошибки, но ещё нет измерений температуры
        // Это начальная инициализация, выведем режим регулирования, раз больше
        // нечего выводить
        printRegulatorMode();
        return;
    }

    // Система работает штатно, просто выводим инфу о текущем состоянии
    printCommonInfo();
}

void updateDisplay()
{
    if(!displayIsInitialized) return;

    display.clearDisplay();
    printState();
    display.display();
}
