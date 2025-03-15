#include "common.hpp"
#include "error.hpp"
#include "state.hpp"

// Коэффициент для пропорционального регулирования
// Если отклонение салонной температуры от желанной меньше этой величины, 
// то поправка мощности будет пропорциональна отклонению и отскейлина по
// этой величине.
// При отклонении более чем на эту величину поправка мощности составит 100%
// (полностью включена или выключена)
#define PROPORTIONAL_REGULATOR_DIAPASON 6

// Таблица, которая определяет зависимость
// "насколько надо нагреть воздух" -> "Какую мощность печки выставить"
struct PowerTableRecord
{
  int deltaTemp;    // Разность температур входящего воздуха и
                    // желанной температуры
  int power;        // Какую мощность выставить при этой разницу
};
PowerTableRecord powerTable[] = { {0, 0},
                                  {1, 5},
                                  {20, 40},
                                  {40, 100}};
const int powerTableSize = sizeof(powerTable) / sizeof(powerTable[0]);

// Первичное выставление мощности по температуре
void updateByOutsideDesiredDelta()
{
  // Разница температур приточного воздуха и желанной температуры
  int delta = getDesiredTemperature() - getOutsideTemperature();
  
  // Разница температур отрицательная, воздух греть не надо
  if(delta <= powerTable[0].deltaTemp)
  {
    setDesiredPower(powerTable[0].power);
    return;
  }

  // Очень большая разница, выставляем мощность на максимум
  if(delta >= powerTable[powerTableSize - 1].deltaTemp)
  {
    setDesiredPower(powerTable[powerTableSize - 1].power);
    return;
  }

  // Ищем в таблице 2 записи, между которыми находится
  // текущая разница температур
  int i = 1;
  for(; i < powerTableSize - 1; i++)
  {
    if(powerTable[i].deltaTemp > delta) break;
  }

  // Линейная интерполяция между двумя найденными температурами
  setDesiredPower(map(delta,
                      powerTable[i-1].deltaTemp,
                      powerTable[i].deltaTemp,
                      powerTable[i-1].power,
                      powerTable[i].power));
}

// Дополнительная пропорциональная коррекция по разнице салонной температуры
// и желанной температуры
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