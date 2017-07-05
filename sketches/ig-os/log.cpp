
#include "log.h"
#include "system.h"

Logger::Logger(WaterCircuit& circuit, Settings& settings)
  : m_circuit(circuit)
  , m_state(StateIdle)
  , m_settings(settings)
  , m_previousLogTime(0)
{ }

void
Logger::run()
{
  switch (m_state) {
    case StateIdle:
      if (isDue()) {
        m_state = StateWaitSensor;
        Serial.println("Logger started");
      }
      break;
    case StateWaitSensor:
    {
      Sensor& sensor = m_circuit.getSensor();
      if (sensor.getState() == Sensor::StateIdle) {
        sensor.enable();
        m_state = StateSetupSensor;
      }
      break;
    }
    case StateSetupSensor:
    {
      Sensor& sensor = m_circuit.getSensor();
      sensor.run();
      if (sensor.getState() == Sensor::StateReady) {
        m_humidity = sensor.read();
        sensor.disable();
        m_state = StateWaitReservoir;
      }
      break;
    }
    case StateWaitReservoir:
    {
      Sensor& reservoir = m_circuit.getReservoir();
      if (reservoir.getState() == Sensor::StateIdle) {
        reservoir.enable();
        m_state = StateSetupReservoir;
      }
      break;
    }
    case StateSetupReservoir:
    {
      Sensor& reservoir = m_circuit.getReservoir();
      reservoir.run();
      if (reservoir.getState() == Sensor::StateReady) {
        m_reservoir = reservoir.read();
        reservoir.disable();

        if (log(m_humidity, m_reservoir, m_circuit.getPump().getTotalEnabledSeconds())) {
          Serial << "logged data for circuit " << m_circuit.getId() << " at " << timeClient.getFormattedTime() << "\n";
        }

        m_previousLogTime = millis();

        m_state = StateIdle;
      }
      break;
    }
  }
}

// TODO: reset logger as reset in water circuit

void
Logger::trigger()
{
  /* Shifting previous log time into past such that it triggers the logger */
  m_previousLogTime = millis() - (unsigned long)m_settings.m_intervalMinutes * 60UL * 1000UL;
}

bool
Logger::isDue() const
{
  if (m_settings.m_intervalMinutes == 0 or not m_circuit.isEnabled()) {
    return false;
  }
  return millis() - m_previousLogTime > (unsigned long)m_settings.m_intervalMinutes * 60UL * 1000UL;
}

#include "ThingSpeak.h"

// TODO: handle network disconnect

ThingSpeakLogger::ThingSpeakLogger(WaterCircuit& circuit,
                                   TslSettings& settings)
  : Logger(circuit, settings.m_settings)
{ }

void
ThingSpeakLogger::begin()
{
  ThingSpeak.begin(m_client);
}

bool
ThingSpeakLogger::log(uint8_t humidity, uint8_t reservoir, unsigned long pumpSeconds)
{
  if (getTslSettings().m_channelId) {
    ThingSpeak.setField((unsigned int)1, humidity);
    ThingSpeak.setField((unsigned int)2, reservoir);
    ThingSpeak.setField((unsigned int)3, static_cast<long>(pumpSeconds));
    ThingSpeak.writeFields(getTslSettings().m_channelId, getTslSettings().m_writeApiKey);

    return true;
  }
  return false;
}

