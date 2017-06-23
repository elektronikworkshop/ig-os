#ifndef EW_IG_SYSTEM
#define EW_IG_SYSTEM

/* https://github.com/arduino-libraries/NTPClient
 * https://github.com/arduino-libraries/NTPClient/blob/master/NTPClient.h
 */
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP,
                     "europe.pool.ntp.org",
                     2 * 60 * 60, /* offset seconds (zurich) */
                     60000        /* update interval millis  */
                     );

inline void systemTimeInit()
{
    timeClient.begin();
}

inline void systemTimeRun()
{
    timeClient.update();
}

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

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

SystemMode systemMode;

class ProtoSensor:
  public Sensor
{
public:
  const int PinVddSensor = D7;
  
  ProtoSensor(const unsigned int millisPrepare = 2000)
    : m_millisPrepare(millisPrepare)
  {}
  virtual void begin()
  {
    pinMode(PinVddSensor, OUTPUT);
    digitalWrite(PinVddSensor, LOW);
  }
  virtual ~ProtoSensor()
  {
    digitalWrite(PinVddSensor, LOW);
  }
  virtual void enable()
  {
    switch (getState()) {
      case StateIdle:
        digitalWrite(PinVddSensor, HIGH);
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
        digitalWrite(PinVddSensor, LOW);
        setState(StateIdle);
        break;
    }
  }
  virtual float read()
  {
    unsigned int v = analogRead(A0);
        
    Serial.print("ProtoSensor::read ");
    Serial.println(v);

    /* Clip and invert */
    v = v > 1023 ? 1023 : v;
    v = 1024 - v;

    /* Convert to 0 .. 1 scale */
    return (float)v * 0.00097751710654936461;
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

class ProtoPump:
  public Pump
{
public:
  const int PinPump = D6;
  virtual void begin()
  {
    pinMode(PinPump, OUTPUT);
    digitalWrite(PinPump, LOW);
  }
  virtual void enable()
  {
    if (isEnabled()) {
      return;
    }
    Serial.println("ProtoPump: On");
    pinMode(PinPump, INPUT);
    Pump::enable();
  }
  virtual void disable()
  {
    if (not isEnabled()) {
      return;
    }
    Serial.println("ProtoPump: Off");
    pinMode(PinPump, OUTPUT);
    digitalWrite(PinPump, LOW);
    Pump::disable();
  }
};

ProtoSensor sensor;
ProtoPump pump;
Valve valve;

/* Humidity read:
 *  
 * 63.15
 * 67.55
 */
WaterCircuit circuit(0,
                     sensor,
                     valve,
                     pump,
                     30 * 1000, /* pumpMillis */
                     0.6, /* threshDry */
                     0.8, /* threshWet */
                     5 * 60 *  1000 /* soakMillis */
                     );

const unsigned int NumWaterCircuits = 1;

WaterCircuit* circuits[NumWaterCircuits + 1] = {&circuit, NULL};


class PollTime
{
public:
  PollTime()
    : m_dayDone(-1)
    , m_hour(-1)
    , m_minute(0)
  {}
  PollTime(unsigned int hour,
           unsigned int minute)
    : m_dayDone(-1)
    , m_hour(hour)
    , m_minute(minute)
  { }  
  int getHour() const { return m_hour; }
  int getMinute() const { return m_minute; }

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
      timeClient.getDay()     != m_dayDone and
      timeClient.getHours()   == m_hour    and
      timeClient.getMinutes() == m_minute;
    
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
    return m_hour >= 0;
  }
private:
  /** 0 Sunday, 1 Monday, ... */
  int m_dayDone;
  int m_hour;
  int m_minute;
};

PollTime pollTime0( 6, 0);
PollTime pollTime1( 8, 0);
PollTime pollTime2;
PollTime pollTime3;
PollTime pollTime4;
PollTime pollTime5;
PollTime pollTime6(20, 0);
PollTime pollTime7(22, 0);

const unsigned int NumPollTimes = 8;

PollTime* pollTimes[NumPollTimes + 1] =
{
  &pollTime0,
  &pollTime1,
  &pollTime2,
  &pollTime3,
  &pollTime4,
  &pollTime5,
  &pollTime6,
  &pollTime7,
  NULL
};

bool wateringDue()
{
  bool due = false;
  
  for (PollTime** t = pollTimes; *t; t++) {
    due = due or (*t)->isDue();
  }
  return due;
}

#endif /* #ifndef EW_IG_SYSTEM */

