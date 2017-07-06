#include "system.h"
#include "adc.h"
#include "spi.h"
#include "flash.h"

#include <WiFiUdp.h>

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP,
                     "europe.pool.ntp.org",
                     2 * 60 * 60, /* offset seconds (zurich) */
                     60000        /* update interval millis  */
                     );



SystemMode systemMode;

class OnboardSensor:
  public virtual Sensor
{
public:
  
  OnboardSensor(Adc::Channel channel)
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
        setState(StateIdle);
        break;
    }
  }
  virtual uint8_t read()
  {
    unsigned int v = adc.getResult() / 4;

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
class OnboardPump:
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
    Debug << "onboard pump enabled\n";
  }
  virtual void disable()
  {
    if (not isEnabled()) {
      return;
    }
    spi.setPump(false);
    Pump::disable();
    Debug << "onboard pump disabled\n";
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
  virtual bool isOpen() const
  {
    return (static_cast<uint8_t>(spi.getValve()) & static_cast<uint8_t>(m_valve)) != 0;
  }

private:
  Spi::Valve m_valve;
};

/* Humidity read:
 *  
 * 63.15
 * 67.55
 */

OnboardPump pump;

OnboardSensor sensor0(Adc::ChSensor1);
OnboardSensor sensor1(Adc::ChSensor2);
OnboardSensor sensor2(Adc::ChSensor3);
OnboardSensor sensor3(Adc::ChSensor4);
OnboardSensor reservoir(Adc::ChReservoir);

OnboardValve valve0(Spi::Valve1);
OnboardValve valve1(Spi::Valve2);
OnboardValve valve2(Spi::Valve3);
OnboardValve valve3(Spi::Valve4);

WaterCircuit circuit0(0, sensor0, valve0, pump, reservoir, flashDataSet.waterCircuitSettings[0]);
WaterCircuit circuit1(1, sensor1, valve1, pump, reservoir, flashDataSet.waterCircuitSettings[1]);
WaterCircuit circuit2(2, sensor2, valve2, pump, reservoir, flashDataSet.waterCircuitSettings[2]);
WaterCircuit circuit3(3, sensor3, valve3, pump, reservoir, flashDataSet.waterCircuitSettings[3]);

WaterCircuit* circuits[NumWaterCircuits + 1] = {&circuit0, &circuit1, &circuit2, &circuit3, NULL};


SchedulerTime schedulerTime0(flashDataSet.schedulerTimes[0]);
SchedulerTime schedulerTime1(flashDataSet.schedulerTimes[1]);
SchedulerTime schedulerTime2(flashDataSet.schedulerTimes[2]);
SchedulerTime schedulerTime3(flashDataSet.schedulerTimes[3]);
SchedulerTime schedulerTime4(flashDataSet.schedulerTimes[4]);
SchedulerTime schedulerTime5(flashDataSet.schedulerTimes[5]);
SchedulerTime schedulerTime6(flashDataSet.schedulerTimes[6]);
SchedulerTime schedulerTime7(flashDataSet.schedulerTimes[7]);

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


ThingSpeakLogger thingSpeakLogger0(circuit0, flashDataSet.thingSpeakLoggerSettings[0]);
ThingSpeakLogger thingSpeakLogger1(circuit1, flashDataSet.thingSpeakLoggerSettings[1]);
ThingSpeakLogger thingSpeakLogger2(circuit2, flashDataSet.thingSpeakLoggerSettings[2]);
ThingSpeakLogger thingSpeakLogger3(circuit3, flashDataSet.thingSpeakLoggerSettings[3]);

Logger* loggers[NumWaterCircuits + 1] =
{
  &thingSpeakLogger0,
  &thingSpeakLogger1,
  &thingSpeakLogger2,
  &thingSpeakLogger3,
  NULL
};

void loggerBegin()
{
  for (Logger** l = loggers; *l; l++) {
    (*l)->begin();
  }
}
void loggerRun()
{ 
  for (Logger** l = loggers; *l; l++) {
    (*l)->run();
  }
}



