#include "system.h"
#include "adc.h"
#include "spi.h"

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
  
  ProtoSensor(Adc::Channel channel)
    : m_adcChannel(channel)
  {}
  virtual void begin()
  {
  }
  virtual void enable()
  {
    switch (getState()) {
      case StateIdle:
        m_result = 0;
        if (adc.request(m_adcChannel)) {
          setState(StateConvert);
        } else {
          setState(StatePrepare);
        }
        break;
    }
  }
  virtual void disable()
  {
    switch (getState()) {
      case StateConvert:
      case StateReady:
        adc.reset();
        break;
    }
  }
  virtual uint8_t read()
  {
    unsigned int v = adc.getResult() / 4;
        
    Serial << "Sensor " << m_adcChannel << ": " << v << "\n";

    /* Clip and invert */
    v = v > 255 ? 255 : v;
    v = 255 - v;

    return v;
  }
  virtual void run()
  {
    switch (getState()) {
      case StatePrepare:
        if (adc.request(m_adcChannel)) {
          setState(StateConvert);
        } else {
          setState(StatePrepare);
        }
        break;
      case StateConvert:
          if (adc.conversionDone()) {
            setState(StateReady);
          }
        break;
    }
  }
private:
  Adc::Channel m_adcChannel;
  uint8_t m_result;
};

/** 
 *  TODO: Note that pumps valves sensors that are part of multiple watering circuits get their begin() member function called once for each circuit. 
 */
class ProtoPump:
  public Pump
{
public:
  virtual void begin()
  {
  }
  virtual void enable()
  {
    if (isEnabled()) {
      return;
    }
    spi.setPump(true);
    Pump::enable();
    Serial.println("ProtoPump: On");
  }
  virtual void disable()
  {
    if (not isEnabled()) {
      return;
    }
    spi.setPump(false);
    Pump::disable();
    Serial.println("ProtoPump: Off");
  }
};

class OnboardValve
  : public Valve
{
public:
  OnboardValve(Spi::Valve valve)
    : m_valve(valve)
  {}
  virtual void begin() {}
  virtual void open()
  {
    spi.setValve(m_valve);
  }
  virtual void close()
  {
    spi.setValve(Spi::ValveNone);
  }
private:
  Spi::Valve m_valve;
};

/* Humidity read:
 *  
 * 63.15
 * 67.55
 */

ProtoPump pump;

ProtoSensor sensor0(Adc::ChSensor1);
ProtoSensor sensor1(Adc::ChSensor2);
ProtoSensor sensor2(Adc::ChSensor3);
ProtoSensor sensor3(Adc::ChSensor4);

OnboardValve valve0(Spi::Valve1);
OnboardValve valve1(Spi::Valve2);
OnboardValve valve2(Spi::Valve3);
OnboardValve valve3(Spi::Valve4);

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


