#ifndef EW_IG_SYSTEM
#define EW_IG_SYSTEM

#include "config.h"
#include "water_circuit.h"
#include "log.h"

#include <climits>

/* https://github.com/arduino-libraries/NTPClient
 * https://github.com/arduino-libraries/NTPClient/blob/master/NTPClient.h
 */
#include <NTPClient.h>

extern NTPClient timeClient;


class SystemMode
{
public:
  typedef enum
  {
    Invalid = -1,
    Off = 0,
    Auto,
    Manual,
  } Mode;

  SystemMode()
    : m_mode(Auto)
  { }

  Mode getMode() const
  {
    return m_mode;
  }
  void setMode(Mode mode)
  {
    m_mode = mode;
  }
  static Mode str2Mode(const char* modeStr)
  {
    if (strcmp(modeStr, "off") == 0) {
      return Off;
    } else if (strcmp(modeStr, "auto") == 0) {
      return Auto;
    } else if (strcmp(modeStr, "man") == 0) {
      return Manual;
    } else {
      return Invalid;
    }
  }
private:
  Mode m_mode;
};

extern SystemMode systemMode;


const unsigned int NumWaterCircuits = 4;

extern WaterCircuit* circuits[NumWaterCircuits + 1];

class SchedulerTime
{
public:
  static const uint8_t InvalidDay = UINT8_MAX;
  static const uint8_t InvalidHour = UINT8_MAX;
  
  struct Time
  {
    uint8_t  m_hour;
    uint8_t  m_minute;
  };
/*  
  SchedulerTime()
    : m_dayDone(InvalidDay)
    , m_time({InvalidHour, 0})
  {}
  SchedulerTime(uint8_t hour,
                uint8_t minute)
    : m_dayDone(InvalidDay)
    , m_time({hour, minute})
  { }
*/
  SchedulerTime(Time& t)
    : m_dayDone(InvalidDay)
    , m_time(t)
  { }
  uint8_t getHour() const { return m_time.m_hour; }
  uint8_t getMinute() const { return m_time.m_minute; }

  void setHour(uint8_t hour) { m_time.m_hour = hour; }
  void setMinute(uint8_t minute) { m_time.m_minute = minute; }

  /** Checks if due. If due it marks it with the day done and returns true. */
  bool isDue()
  {
    if (not isValid()) {
      return false;
    }
    
    /* Check if we processed it already today */
    if (m_dayDone == timeClient.getDay()) {
      return false;
    }

    bool due =
      timeClient.getDay()     != m_dayDone        and
      timeClient.getHours()   == m_time.m_hour    and
      timeClient.getMinutes() == m_time.m_minute;
    
    if (due) {
      m_dayDone = timeClient.getDay();
      Serial << "detected watering due at " << timeClient.getFormattedTime() << "\n";
    }
    
    return due;
  }
  void markDone()
  {
    m_dayDone = timeClient.getDay();
  }
  bool isValid() const
  {
    return m_time.m_hour <= 23;
  }
private:
  /** 0 Sunday, 1 Monday, ... */
  uint8_t  m_dayDone;
  Time& m_time;
};

const unsigned int NumSchedulerTimes = 8;

extern SchedulerTime* schedulerTimes[NumSchedulerTimes + 1];

bool wateringDue();


extern Logger* loggers[NumWaterCircuits + 1];
void loggerBegin();
void loggerRun();

namespace history {

  void skipLines(unsigned int& skip, unsigned int& i, const char* buf, unsigned int count);
  void prtLines(Print& prt, unsigned int& numLines, const char* buf, unsigned int count);
  bool prt(Print& prt, int start = -1, int end = 0);
  
} /* namespace history */

#endif /* #ifndef EW_IG_SYSTEM */

