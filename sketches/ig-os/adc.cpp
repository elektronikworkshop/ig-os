#include "adc.h"
#include "spi.h"

Adc adc;
  
void
Adc::begin()
{
  pinMode(SensorPowerPin, OUTPUT);
  digitalWrite(SensorPowerPin, LOW);
}
  
void
Adc::run()
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
  
bool
Adc::request(Channel channel)
{
  if (m_state != StateIdle) {
    return false;
  }

  digitalWrite(SensorPowerPin, HIGH);
  
  changeState(StatePowerUp);
}

void
Adc::changeState(State newState)
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

