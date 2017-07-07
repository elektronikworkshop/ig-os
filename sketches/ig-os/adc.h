#ifndef EW_IG_ADC_H
#define EW_IG_ADC_H

#include <Arduino.h>

class Adc
{
public:
  typedef enum
  {
    StateIdle = 0,
    StatePowerUp,
    StateAdcSetup,
    StateReady,
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
  
  void begin();  
  void run();  
  bool request(Channel channel);

  State getState() const
  {
    return m_state;
  }
  bool isReady() const
  {
    return m_state == StateReady;
  }
  uint16_t read() const;
  void reset()
  {
      changeState(StateIdle);
  }
private:
  void changeState(State newState);
  
  State m_state;
  Channel m_channel;
  unsigned long m_lastStateChangeMs;
};

extern Adc adc;

#endif /* EW_IG_ADC_H */

