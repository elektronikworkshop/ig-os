
#include "log.h"
#include "system.h"

Logger::Logger(WaterCircuit& circuit, unsigned int intervalMinutes)
  : m_circuit(circuit)
  , m_intervalMinutes(intervalMinutes)
  , m_previousLogTime(0)
{ }

void
Logger::run()
{
  /* log only if watering circuit is enabled */
  if (not m_circuit.isEnabled() or not isDue()) {
    return;
  }

  /* if sensor is active we postpone logging until we are able to
   * get fresh data. The trigger condition will remain true.
   */
  Sensor& sensor = m_circuit.getSensor();
  if (sensor.getState() != Sensor::StateIdle) {
    return;
  }

  sensor.enable();
  while (sensor.getState() != Sensor::StateReady) {
    delay(500);
    sensor.run();
  }
  float humidity = sensor.read();
  sensor.disable();

  log(humidity, m_circuit.getPump().getTotalEnabledSeconds());

  Serial << "logged data for circuit " << m_circuit.getId() << " at " << timeClient.getFormattedTime() << "\n";

  m_previousLogTime = millis();
}

void
Logger::trigger()
{
  m_previousLogTime = millis() - m_intervalMinutes * 1000UL;
}

bool
Logger::isDue() const
{
  return millis() - m_previousLogTime > (unsigned long)m_intervalMinutes * 60UL * 1000UL;
}

#include "ThingSpeak.h"

ThingSpeakLogger::ThingSpeakLogger(WaterCircuit& circuit,
                                   unsigned int intervalMinutes,
                                   const unsigned long channelId,
                                   const char* writeApiKey)
  : Logger(circuit, intervalMinutes)
  , m_channelId(channelId)
  , m_writeApiKey(writeApiKey)
{ }

void
ThingSpeakLogger::begin()
{
  ThingSpeak.begin(m_client);
}

bool
ThingSpeakLogger::log(float humidity, unsigned long pumpSeconds)
{
  if (m_channelId) {
    ThingSpeak.setField((unsigned int)1, humidity);
    ThingSpeak.setField((unsigned int)2, static_cast<long>(pumpSeconds));
    ThingSpeak.writeFields(m_channelId, m_writeApiKey);

    return true;
  }
  return false;
}

