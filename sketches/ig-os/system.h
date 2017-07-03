#ifndef EW_IG_SYSTEM
#define EW_IG_SYSTEM

#include "config.h"
#include "water_circuit.h"

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
  
  typedef struct
  {
    uint8_t  m_hour;
    uint8_t  m_minute;
  } SchedulerTimeStruct;
  
  SchedulerTime()
    : m_dayDone(InvalidDay)
    , m_time({InvalidHour, 0})
  {}
  SchedulerTime(uint8_t hour,
                uint8_t minute)
    : m_dayDone(InvalidDay)
    , m_time({hour, minute})
  { }
  SchedulerTime(const SchedulerTimeStruct& t)
    : m_dayDone(InvalidDay)
    , m_time(t)
  { }
  uint8_t getHour() const { return m_time.m_hour; }
  uint8_t getMinute() const { return m_time.m_minute; }

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
  SchedulerTimeStruct m_time;
};

const unsigned int NumSchedulerTimes = 8;

extern SchedulerTime* schedulerTimes[NumSchedulerTimes + 1];

bool wateringDue();

class ProtoReservoir
  : public Reservoir
{
public:
//  const int PinVddSensor = D7;
  
  ProtoReservoir(const unsigned int millisPrepare = 2000)
    : m_millisPrepare(millisPrepare)
  {}
  virtual void begin()
  {
//    pinMode(PinVddSensor, OUTPUT);
//    digitalWrite(PinVddSensor, LOW);
  }
  virtual ~ProtoReservoir()
  {
//    digitalWrite(PinVddSensor, LOW);
  }
  virtual void enable()
  {
    switch (getState()) {
      case StateIdle:
//        digitalWrite(PinVddSensor, HIGH);
        m_millisEnabled = millis();
        setState(StatePrepare);
        break;
    }
  }
  virtual void disable()
  {
    switch (getState()) {
      case StatePrepare:
      case StateReady:
//        digitalWrite(PinVddSensor, LOW);
        setState(StateIdle);
        break;
    }
  }
  virtual uint8_t read()
  {
    unsigned int v;
//  v = analogRead(A0) / 4;
        
    Serial.print("ProtoReservoir::read ");
    Serial.println(v);

    /* Clip and invert */
    v = v > 255 ? 255 : v;
    v = 255 - v;

    /* Convert to 0 .. 1 scale */
    return v;
  }
  virtual void run()
  {
    switch (getState()) {
      case StatePrepare:
        if (millis() - m_millisEnabled > m_millisPrepare) {
          setState(StateReady);
        }
      break;
    }
  }
private:
  unsigned long m_millisEnabled;
  unsigned int m_millisPrepare;
};

#endif /* #ifndef EW_IG_SYSTEM */

