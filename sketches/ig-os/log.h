#ifndef EW_IG_LOG_H
#define EW_IG_LOG_H

#include "water_circuit.h"

/* TODO: we should trigger on absolute time to achieve a more regular logging interval */

class Logger
{
public:
  Logger(WaterCircuit& circuit, unsigned int intervalMinutes);
  virtual void begin() = 0;
  virtual bool log(float humidity, unsigned long pumpSeconds) = 0;
  void run();
  void trigger();
  bool isDue() const;

private:
  WaterCircuit& m_circuit;
  unsigned int m_intervalMinutes;
  unsigned long m_previousLogTime;
};

#include <ESP8266WiFi.h>

class ThingSpeakLogger
  : public Logger
{
public:
  /**
   * @param channelId
   * ThingSpeak channel ID, if set to zero (0) this logger is disabled.
   */
  ThingSpeakLogger(WaterCircuit& circuit,
                   unsigned int intervalMinutes,
                   const unsigned long channelId = 0,
                   const char* writeApiKey = NULL);
  virtual void begin();
  virtual bool log(float humidity, unsigned long pumpSeconds);
private:
  WiFiClient  m_client;
  unsigned long m_channelId;
  const char* m_writeApiKey;
};

#endif /* EW_IG_LOG_H */

