#ifndef EW_IG_LOG_H
#define EW_IG_LOG_H

#include "water_circuit.h"

/* TODO: we should trigger on absolute time to achieve a more regular logging interval */

class Logger
{
public:
  struct Settings
  {
    unsigned int m_intervalMinutes;
  };
  
  typedef enum
  {
    StateIdle,
    StateWaitSensor,
    StateSetupSensor,
    StateWaitReservoir,
    StateSetupReservoir,
  } State;

  Logger(WaterCircuit& circuit, Settings& settings);
  virtual void begin() = 0;
  virtual bool log(uint8_t humidity, uint8_t reservoir, unsigned long pumpSeconds) = 0;
  void run();
  void trigger();
  bool isDue() const;

  Settings& getSettings()
  {
    return m_settings;
  }
private:
  WaterCircuit& m_circuit;
  State m_state;
  Settings& m_settings;
  unsigned long m_previousLogTime;

  uint8_t m_humidity;
  uint8_t m_reservoir;
};

#include <ESP8266WiFi.h>

class ThingSpeakLogger
  : public Logger
{
public:
  static const int MaxWriteApiKeyLen = 31;
  struct Settings
  {
    Logger::Settings m_settings;
    unsigned long m_channelId;
    char m_writeApiKey[MaxWriteApiKeyLen + 1];
  };
  /**
   * @param channelId
   * ThingSpeak channel ID, if set to zero (0) this logger is disabled.
   */
  ThingSpeakLogger(WaterCircuit& circuit,
                   Settings& settings);
  virtual void begin();
  virtual bool log(uint8_t humidity, uint8_t reservoir, unsigned long pumpSeconds);

  Settings& getSettings()
  {
    /* Danger, danger! Struct address hack: address of first struct member is the address of the enclosing struct. */
    return *reinterpret_cast<Settings*>(&Logger::getSettings());
  }
private:
  WiFiClient  m_client;
  const char* m_writeApiKey;
};

#endif /* EW_IG_LOG_H */

