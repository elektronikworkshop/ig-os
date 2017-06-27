#ifndef EW_IG_ADC_H
#define EW_IG_ADC_H

#include "spi.h"

class Adc
{
public:
  typedef enum
  {
    StateIdle = 0,
    StatePowerUp,
    StateAdcSetup,
    StateDone,
  } State;
  typedef enum
  {
    ChSensor1 = 0,
    ChSensor2,
    ChSensor3,
    ChSensor4,
    ChReservoir,
  } Channel;

  static const unsigned int msPowerUp  = 1000;
  static const unsigned int msAdcSetup = 1000;

  Adc()
    : m_state(StateIdle)
  { }
  
  void begin()
  {
    pinMode(SensorPowerPin, OUTPUT);
    digitalWrite(SensorPowerPin, LOW);
  }
  
  void run()
  {
    auto now = millis();
    
    switch (m_state) {
      case StateIdle:
        break;
      case StatePowerUp:
        if (now - m_lastStateChangeMs > msPowerUp) {
          spi.setAdcChannel(m_channel);
          changeState(StateAdcSetup);
        }
        break;
      case StateAdcSetup:
        if (now - m_lastStateChangeMs > msAdcSetup) {
          m_result = analogRead(SensorAdcPin);
          changeState(StateDone);
        }
        break;
      case StateDone:
        break;
    }
  }
  
  bool request(Channel channel)
  {
    if (m_state != StateIdle) {
      return false;
    }

    digitalWrite(SensorPowerPin, HIGH);
    
    changeState(StatePowerUp);
  }
  
  unsigned int getResult()
  {
    changeState(StateIdle);
    
    return m_result;
  }
private:
  void changeState(State newState)
  {
    switch (newState) {
      /* Make sure that we power off with channel 0 selected to 
       *  avoid current through ESD protection diodes
       */
      case StateIdle:
        spi.setAdcChannel(0);
        digitalWrite(SensorPowerPin, LOW);
        break;
    }
    m_state = newState;
    m_lastStateChangeMs = millis();
  }
  State m_state;
  Channel m_channel;
  unsigned long m_lastStateChangeMs;
  unsigned int m_result;
};

Adc adc;

#endif /* EW_IG_ADC_H */

