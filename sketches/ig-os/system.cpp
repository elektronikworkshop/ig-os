#include "system.h"

#include <WiFiUdp.h>

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP,
                     "europe.pool.ntp.org",
                     2 * 60 * 60, /* offset seconds (zurich) */
                     60000        /* update interval millis  */
                     );



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
  virtual uint8_t read()
  {
    unsigned int v = analogRead(A0) / 4;
        
    Serial.print("ProtoSensor::read ");
    Serial.println(v);

    /* Clip and invert */
    v = v > 255 ? 255 : v;
    v = 255 - v;

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


/* Humidity read:
 *  
 * 63.15
 * 67.55
 */

ProtoPump pump;

ProtoSensor sensor0;
ProtoSensor sensor1;
ProtoSensor sensor2;
ProtoSensor sensor3;

Valve valve0;
Valve valve1;
Valve valve2;
Valve valve3;

WaterCircuit circuit0(0, sensor0, valve0, pump);
WaterCircuit circuit1(1, sensor0, valve0, pump);
WaterCircuit circuit2(2, sensor0, valve0, pump);
WaterCircuit circuit3(3, sensor0, valve0, pump);

WaterCircuit* circuits[NumWaterCircuits + 1] = {&circuit0, &circuit1, &circuit2, &circuit3, NULL};


SchedulerTime schedulerTime0;
SchedulerTime schedulerTime1;
SchedulerTime schedulerTime2;
SchedulerTime schedulerTime3;
SchedulerTime schedulerTime4;
SchedulerTime schedulerTime5;
SchedulerTime schedulerTime6;
SchedulerTime schedulerTime7;

SchedulerTime* schedulerTimes[NumSchedulerTimes + 1] =
{
  &schedulerTime0,
  &schedulerTime1,
  &schedulerTime2,
  &schedulerTime3,
  &schedulerTime4,
  &schedulerTime5,
  &schedulerTime6,
  &schedulerTime7,
  NULL
};

bool wateringDue()
{
  for (SchedulerTime** t = schedulerTimes; *t; t++) {
    if ((*t)->isDue()) {
      return true;
    }
  }
  return false;
}


