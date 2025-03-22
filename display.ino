#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Wire.h"

#include "display.hpp"
#include "error.hpp"
#include "state.hpp"

static Adafruit_SSD1306 display(128, 64, &Wire, 4);
static bool displayIsInitialized = false;

// Время, оставшееся до перехода в обычный режим
static volatile unsigned long showDesiredTime = 0;
static volatile unsigned long showModeTime = 0;

void initDisplay()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        return;
    }

    display.setTextColor(SSD1306_WHITE);
    displayIsInitialized = true;
}

void setShowDesired()
{
    showDesiredTime = millis() + SHOW_DESIRED_INTERVAL;
}

void setShowMode()
{
    showModeTime = millis() + SHOW_MODE_INTERVAL;
}

void printError()
{
    // Ошибка моргает. Ничего не рисуем каждые 500мсек
    if((millis() / 500) % 2 == 0) return;

    display.setTextSize(2, 3);
    display.setCursor(5, 22);

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
    case FLOW_TEMPERATURE_SENSOR_ERROR:
        display.print("ERR F_TEM");
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

    if(getRegulatorMode() == MODE_POWER)
    {
        display.setCursor(14, 18);
        display.print("MANUAL");
    }
    else
    {
        display.setCursor(32, 18);
        display.print("AUTO");
    }
}

// Мощность печки либо желанная температура в зависимости от режима
// Отрисовка в течении некоторого времени после поворота крутилки
void showDesired()
{
    display.setTextSize(3, 4);

    if(getRegulatorMode() == MODE_POWER)
    {
        // Выводим настроенную мощность печки. Выравнивание по центру
        int value = getDesiredPower();
        int position = 48;
        if(value >= 10) position -= 9;
        if(value >= 100) position -= 9;
        display.setCursor(position, 18);
        display.print(value);
        display.print("%");
    }
    else
    {
        // Выводим настроенную температуру. Выравнивание по центру
        int value = getDesiredTemperature();
        int position = 39;
        if(value < 0) position -= 9;
        if(value <= -10) position -= 9;
        if(value >= 10) position -= 9;
        display.setCursor(position, 18);
        display.print(value);
        display.print(char(247));
        display.print("C");
    }
}

// Строка состояния внизу экрана в обычном режиме работы
void printStatusBar()
{
    display.setTextSize(2, 2);
    display.setCursor(0, 48);    

    // Переключение строки состояния по времени
    unsigned long currentTime = millis();
    unsigned int step = (currentTime / 3000) % 3;
    if(step == 0)
    {
        // Индикатор текущего режима
        if(getRegulatorMode() == MODE_POWER) display.print("Manual");
        else
        {
            display.print("Auto ");
            display.print(getDesiredTemperature());
            display.print(char(247));
            display.print("C");
        }
    }
    else if (step == 1)
    {
        // Текущая температура приточного воздуха
        display.print(round(getOutsideTemperature()));
        display.print(char(247));
        display.print("C");

        // Текущая мощность печки
        // Выравнивание по правой стороне
        int power = getDesiredPower();
        int position = 104;
        if(power >= 10) position -= 12;
        if(power >= 100) position -= 12;
        display.setCursor(position, 48);
        display.print(power);
        display.print("%");  
    }
    else
    {
        // Текущая температура воздуха после печки
        display.print("f");
        display.print(round(getFlowTemperature()));
        display.print("C");

        // Температура с салонного датчика
        // Выравнивание по правой стороне
        int temperature = round(getInsideTemperature());
        int position = 92;
        if(temperature <= 0) position -= 12;
        if(temperature <= -10) position -= 12;
        if(temperature >= 10) position -= 12;
        display.setCursor(position, 48);
        display.print("i");
        display.print(temperature);
        display.print("C");
    }
}

// Режим отрисовки, когда нет ошибок и нет никаких изменений настроек
void printCommonInfo()
{
    // Текущая ощущаемая температура в салоне
    display.setTextSize(2, 3);
    int showedTemperature = round(getFeltTemperature());    
    // Выравнивание по центру экрана
    int position = 48;
    if(showedTemperature < 0) position -= 6;
    if(showedTemperature <= -10) position -= 6;
    if(showedTemperature >= 10) position -= 6;
    display.setCursor(position, 12);
    display.print(showedTemperature);
    display.print(char(247));
    display.print("C");

    printStatusBar();
}

// Смотрим, в каком состоянии сейчас система и выводим
// нужный тип информации в зависимости от этого
void printState()
{
    if(showModeTime != 0)
    {
        // Недавно была смена режима и надо просигнализировать об этом
        printRegulatorMode();
        return;
    }

    if(showDesiredTime != 0)
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

    if(getFeltTemperature() == NO_TEMPERATURE)
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

    if(millis() >= showDesiredTime) showDesiredTime = 0;
    if(millis() >= showModeTime) showModeTime = 0;
}
