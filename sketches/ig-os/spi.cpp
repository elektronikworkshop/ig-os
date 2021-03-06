#include "spi.h"

Spi spi;
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
    
    case StatePoweringUp:
      if (now - m_lastStateChangeMs > msPowerUp) {
        spi.setAdcChannel(m_channel);
        changeState(StateAdcSetup);
      }
      break;
    
    case StatePowerUpIdle:
      if (now - m_lastStateChangeMs > msPowerDown) {
        changeState(StateIdle);
      }
      break;
      
    case StateAdcSetup:
      if (now - m_lastStateChangeMs > msAdcSetup) {
        changeState(StateReady);
      }
      break;
    
    case StateReady:
      break;
  }
}
  
bool
Adc::request(Channel channel)
{
  switch (m_state) {
    
    case StateIdle:
      m_channel = channel;
      changeState(StatePoweringUp);
      return true;
      
    case StatePowerUpIdle:
      if (channel != m_channel) {
        m_channel = channel;
        spi.setAdcChannel(m_channel);
        changeState(StateAdcSetup);
      } else {
        changeState(StateReady);
      }
      return true;
      
    default:
      return false;
  }
}

void
Adc::reset()
{
  if (m_state > StatePoweringUp) {
    changeState(StatePowerUpIdle);
  } else {
    changeState(StateIdle);
  }
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
      Debug << F("adc idle\n");
      break;
    case StatePoweringUp:
      digitalWrite(SensorPowerPin, HIGH);
      Debug << F("adc powering up\n");
      break;
    case StatePowerUpIdle:
      Debug << F("adc power up idle\n");
      break;
    case StateAdcSetup:
      Debug << F("adc setup\n");
      break;
    case StateReady:
      Debug << F("adc ready\n");
      break;
  }
  m_state = newState;
  m_lastStateChangeMs = millis();
}

/*
    https://github.com/esp8266/Arduino/issues/2070
    
    adc_value = adc.read(0)
    ADC_SAMPLES_PER_READING = 10
    EMA_ALPHA = 10
    
    function read_adc()
        local lowest_sample, new_sample = adc.read(0)
        for i = 1, ADC_SAMPLES_PER_READING do
            new_sample = adc.read(0)
            if new_sample < lowest_sample then lowest_sample = new_sample end
        end
        adc_value = (EMA_ALPHA * lowest_sample + (100 - EMA_ALPHA) * adc_value) / 100;
    end
    
    tmr.alarm(2, 200, 1, function() read_adc() end)
 */

uint16_t
Adc::read() const
{
  return analogRead(SensorAdcPin);
}

