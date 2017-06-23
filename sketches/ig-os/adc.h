#ifndef EW_IG_ADC_H
#define EW_IG_ADC_H

#include "spi.h"

class Adc
{
public:
  typedef enum
  {
    ChSensor1 = 0,
    ChSensor2,
    ChSensor3,
    ChSensor4,
    ChReservoir,
  } Channel;

  /* Pin to enable the analog power supply */
  static const unsigned int SensorPowerPin = D4; /* TODO USE CORRECT PIN AND GLOBAL CONFIG HEADER */

  typedef enum
  {
    StateOff,
    StatePower,
  };
private:
}


#endif /* EW_IG_ADC_H */

