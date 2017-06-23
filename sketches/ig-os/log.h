
#include "system.h"

class Logger
{
public:
  Logger(WaterCircuit& circuit, unsigned int intervalMinutes)
    : m_circuit(circuit)
    , m_intervalMinutes(intervalMinutes)
    , m_previousLogTime(0)
  { }
  virtual void begin() = 0;
  virtual bool log() = 0;
  void run()
  {
    if (millis() - m_previousLogTime > (unsigned long)m_intervalMinutes * 60UL * 1000UL) {
  
      // todo: perform extra measurements here
      //        (check if sensor active and postpone logging by not writing m_previousLogTime)

      // todo: check if data from circuit is valid

      log();
      
      m_previousLogTime = millis();
    }
  }
protected:
  WaterCircuit& getCircuit() { return m_circuit; }
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
  virtual bool log()
  {
    ThingSpeak.setField((unsigned int)1, getCircuit().getHumidity());
    ThingSpeak.setField((unsigned int)2, static_cast<long>(getCircuit().getPump().getTotalEnabledTimeMs()/1000));
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

ThingSpeakLogger thingSpeakLogger(circuit, 60, thingspeakChannelId, thingspeakWriteApiKey);

