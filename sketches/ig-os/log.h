#ifndef EW_IG_LOG_H
#define EW_IG_LOG_H

#include "circuit.h"

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

  const Settings& getSettings() const
  {
    return m_settings;
  }
  void setIntervalMinutes(unsigned int intervalMinutes)
  {
    m_settings.m_intervalMinutes = intervalMinutes;
  }
protected:
  Settings& m_settings;

private:
  WaterCircuit& m_circuit;
  State m_state;
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
  struct TslSettings
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
                   TslSettings& settings);
  virtual void begin();
  virtual bool log(uint8_t humidity, uint8_t reservoir, unsigned long pumpSeconds);

  const TslSettings& getTslSettings() const
  {
    /* Danger, danger! Struct address hack: address of first struct member is the address of the enclosing struct. */
    return *reinterpret_cast<const TslSettings*>(&m_settings);
  }

  void setChannelId(unsigned long channelId)
  {
    TslSettings& s = *reinterpret_cast<TslSettings*>(&m_settings);
    s.m_channelId = channelId;
  }
  void setWriteApiKey(const char* key)
  {
    TslSettings& s = *reinterpret_cast<TslSettings*>(&m_settings);
    
    strncpy(s.m_writeApiKey, key, MaxWriteApiKeyLen);
  }
private:
  WiFiClient  m_client;
  const char* m_writeApiKey;
};

#endif /* EW_IG_LOG_H */

