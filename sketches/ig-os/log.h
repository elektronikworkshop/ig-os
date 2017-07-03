
#include "system.h"

/* TODO: we should trigger on absolute time to achieve a more regular logging interval */

class Logger
{
public:
  Logger(WaterCircuit& circuit, unsigned int intervalMinutes)
    : m_circuit(circuit)
    , m_intervalMinutes(intervalMinutes)
    , m_previousLogTime(0)
  { }
  virtual void begin() = 0;
  virtual bool log(float humidity, unsigned long pumpSeconds) = 0;
  void run()
  {
    if (millis() - m_previousLogTime > (unsigned long)m_intervalMinutes * 60UL * 1000UL) {
  
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
      
      m_previousLogTime = millis();
    }
  }
  void trigger()
  {
    m_previousLogTime = millis() - m_intervalMinutes * 1000UL;
  }
//protected:
//  WaterCircuit& getCircuit() { return m_circuit; }
private:
  WaterCircuit& m_circuit;
  unsigned int m_intervalMinutes;
  unsigned long m_previousLogTime;
};

#include "ThingSpeak.h"

class ThingSpeakLogger
  : public Logger
{
public:
  ThingSpeakLogger(WaterCircuit& circuit,
                   unsigned int intervalMinutes,
                   const unsigned long channelId,
                   const char* writeApiKey)
    : Logger(circuit, intervalMinutes)
    , m_channelId(channelId)
    , m_writeApiKey(writeApiKey)
  { }
  virtual void begin()
  {
    ThingSpeak.begin(m_client);
  }
  virtual bool log(float humidity, unsigned long pumpSeconds)
  {
    ThingSpeak.setField((unsigned int)1, humidity);
    ThingSpeak.setField((unsigned int)2, static_cast<long>(pumpSeconds));
    ThingSpeak.writeFields(m_channelId, m_writeApiKey);

    Serial << "logged data at " << timeClient.getFormattedTime() << "\n";
    
    return true;
  }

private:
  WiFiClient  m_client;
  unsigned long m_channelId;
  const char* m_writeApiKey;
};

const unsigned long thingspeakChannelId = 291734;
const char* thingspeakWriteApiKey = "GJESUISMQX7ZDJCP";

//ThingSpeakLogger thingSpeakLogger(circuit, 60, thingspeakChannelId, thingspeakWriteApiKey);

