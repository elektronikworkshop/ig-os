#include "system.h"
#include "spi.h"
#include "settings.h"

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
  
  static const unsigned int NumMeasurements = 8;
  static const unsigned int MeasurementIntervalMs = 1000UL;
  
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
        m_index = 0;
        if (adc.request(m_adcChannel)) {
          setState(StateConvert);
        } else {
          setState(StatePrepare);
        }
        break;
      case StatePrepare:
      case StateConvert:
      case StateReady:
        break;
    }
  }
  virtual void disable()
  {
    switch (getState()) {
      case StateIdle:
      case StatePrepare:
        break;
      case StateConvert:
      case StateReady:
        adc.reset();
        setState(StateIdle);
        break;
    }
  }
  virtual uint8_t read()
  {
    uint16_t v = m_result >> 2;

    /* Clip and invert */
    v = v > 255 ? 255 : v;
    v = 255 - v;

    return v;
  }
  virtual void run()
  {
    switch (getState()) {
      case StateIdle:
        break;
      case StatePrepare:
        if (adc.request(m_adcChannel)) {
          setState(StateConvert);
        } else {
          setState(StatePrepare);
        }
        break;
      case StateConvert:
          if (adc.isReady()) {
            
            if (m_index == 0) {
              
              /* nothing */
              
            } else if (m_index < NumMeasurements) {
            
              if (millis() - m_millsPrevMeasurement < MeasurementIntervalMs) {
                break;
              }
              
            } else {
              m_result /= NumMeasurements;
              setState(StateReady);
              break;
            }

            Debug << "sensor " << m_adcChannel << ", iteration " << m_index << ": ";
            
            m_millsPrevMeasurement = millis();
            auto v = adc.read();
            m_result += v;
            m_index++;

            Debug << v << ", " << m_result / m_index << "\n";

          }
        break;
      case StateReady:
        break;
    }
  }
private:
  Adc::Channel m_adcChannel;
  uint32_t m_result;
  uint8_t m_index;
  unsigned long m_millsPrevMeasurement;
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

class TheWaterCircuit
  : public WaterCircuit
{
public:
  using WaterCircuit::WaterCircuit;

protected:
  virtual Print& dbg() const
  {
    Debug << "circuit ["<< getId() + 1 << "]: ";
    return Debug;
  }
  virtual Print& err() const
  {
    Debug << "circuit ["<< getId() + 1 << "]: ";
    return Debug;
  }
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

TheWaterCircuit circuit0(0, sensor0, valve0, pump, reservoir, flashSettings.waterCircuitSettings[0]);
TheWaterCircuit circuit1(1, sensor1, valve1, pump, reservoir, flashSettings.waterCircuitSettings[1]);
TheWaterCircuit circuit2(2, sensor2, valve2, pump, reservoir, flashSettings.waterCircuitSettings[2]);
TheWaterCircuit circuit3(3, sensor3, valve3, pump, reservoir, flashSettings.waterCircuitSettings[3]);

WaterCircuit* circuits[NumWaterCircuits + 1] = {&circuit0, &circuit1, &circuit2, &circuit3, NULL};


SchedulerTime schedulerTime0(flashSettings.schedulerTimes[0]);
SchedulerTime schedulerTime1(flashSettings.schedulerTimes[1]);
SchedulerTime schedulerTime2(flashSettings.schedulerTimes[2]);
SchedulerTime schedulerTime3(flashSettings.schedulerTimes[3]);
SchedulerTime schedulerTime4(flashSettings.schedulerTimes[4]);
SchedulerTime schedulerTime5(flashSettings.schedulerTimes[5]);
SchedulerTime schedulerTime6(flashSettings.schedulerTimes[6]);
SchedulerTime schedulerTime7(flashSettings.schedulerTimes[7]);

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


ThingSpeakLogger thingSpeakLogger0(circuit0, flashSettings.thingSpeakLoggerSettings[0]);
ThingSpeakLogger thingSpeakLogger1(circuit1, flashSettings.thingSpeakLoggerSettings[1]);
ThingSpeakLogger thingSpeakLogger2(circuit2, flashSettings.thingSpeakLoggerSettings[2]);
ThingSpeakLogger thingSpeakLogger3(circuit3, flashSettings.thingSpeakLoggerSettings[3]);

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


namespace history {
    void skipLines(unsigned int& skip, unsigned int& i, const char* buf, unsigned int count)
  {
    for (; i < count and skip;) {
      char c = *(buf + i);
      if (c == '\n') {
        skip--;
      }
      if (not c) {
        return;
      }
      i++;
    }
  }
  void prtLines(Print& prt, unsigned int& numLines, const char* buf, unsigned int count)
  {
    for (unsigned int i = 0; i < count and numLines; i++) {
      char c = *(buf + i);
      if (not c) {
        return;
      }
      prt << c;
      if (c == '\n') {
        --numLines;
        if (numLines == 0) {
          return;
        }
      }
    }
  }
  bool
  prt(Print& prt, int start, int end)
  {
    unsigned int numLinesRequested = 10, skip = 0, n;

    /* only count given */
    if (start and not end) {
      if (start > 0) {
        numLinesRequested = start;
      } else {
        numLinesRequested = UINT_MAX;
      }
    } else if (start and end) {
      if (start < 0) {
        prt << "<start> can not be negative when requesting a range\n";
        return false;
      }
      if (end < 0) {
        numLinesRequested = UINT_MAX;
      } else {
        if (end <= start) {
          prt << "<start> must be smaller than <end>\n";
          return false;
        }
        numLinesRequested = end - start + 1;
      }
      skip = start;
    }
    
    ErrorLogProxy::BufNfo nfo;
    Error.getBuffer(nfo);

    n = numLinesRequested;

    prt << "----\n";
    
    unsigned int i = 0;
    skipLines(skip, i, nfo.a, nfo.na);
    if (not skip) {
      prtLines(prt, n, nfo.a + i, nfo.na - i);
    }
    i = 0;
    if (skip) {
      skipLines(skip, i, nfo.b, nfo.nb);
    }
    if (not skip) {
      prtLines(prt, n, nfo.b + i, nfo.nb - i);
    }
    if (numLinesRequested - n == 0) {
      prt << "history clean\n";
    } else {
      prt
        << "----\n"
        << numLinesRequested - n << " lines\n"
        ;
    }
    return true;
  }
} /* namespace history */

