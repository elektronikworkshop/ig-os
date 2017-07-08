#include "water_circuit.h"

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

WaterCircuit::WaterCircuit(const unsigned int& id,
                           Sensor& sensor,
                           Valve& valve,
                           Pump& pump,
                           Sensor& reservoir,
                           Settings& settings)
    : m_id(id)
    , m_settings(settings)
    
    , m_sensor(sensor)
    , m_valve(valve)
    , m_pump(pump)
    , m_reservoir(reservoir)

    , m_state(StateIdle)
    , m_iterations(0)
    , m_currentHumidity(0)
  { }

const char*
WaterCircuit::getStateString(State state)
{
  switch (state) {
    case StateIdle:           return "idle";
    case StateWaitSensor:     return "wait sensor";
    case StateSense:          return "sense sensor";
    case StateWaitReservoir:  return "wait reservoir";
    case StateSenseReservoir: return "sense reservoir";
    case StateWaitPump:       return "wait pump";
    case StatePump:           return "pump";
    case StateSoak:           return "soak";
    case StateReservoirEmpty: return "reservoir empty";
    
    default:                  return "unknown";
  }
}

void
WaterCircuit::begin()
{
  m_sensor.begin();
  m_valve.begin();
  m_pump.begin();
}

void
WaterCircuit::trigger()
{
  if (m_state != StateIdle) {
    return;
  }
  m_state = StateWaitSensor;
  m_iterations = 0;
  dbg() << "state: " << getStateString(m_state) << "\n";
}

void
WaterCircuit::run()
{
  switch (m_state) {
    case StateIdle:
      break;
    case StateWaitSensor:
      if (m_sensor.getState() == Sensor::StateIdle) {
        m_sensor.enable();
        m_state = StateSense;
        dbg() << "state: " << getStateString(m_state) << "\n";
      }
      break;
    case StateSense:
      m_sensor.run();
      if (m_sensor.getState() == Sensor::StateReady) {
        m_currentHumidity = m_sensor.read();
        m_sensor.disable();
        
        dbg() << "humidity: " << m_currentHumidity << "\n";
        
        /* The first time we check if the soil is dry.
         * After watering we check if it's wet -- so we
         * have a little hysteresis here.
         */
        if ((m_iterations == 0 and m_currentHumidity <= m_settings.m_threshDry) or
            (m_iterations  > 0 and m_currentHumidity <= m_settings.m_threshWet))
        {
          if (m_settings.m_threshReservoir == 0) {
            m_state = StateWaitPump;
            dbg() << "reservoir threshold disabled, state: " << getStateString(m_state) << "\n";
          } else {
            m_state = StateWaitReservoir;
            dbg() << "state: " << getStateString(m_state) << "\n";
          }
        } else {
          m_state = StateIdle;
          dbg() << "soil not dry enough for watering or already wet, state: " << getStateString(m_state) << "\n";
        }
      }
      break;
    case StateWaitReservoir:
      if (m_reservoir.getState() == Sensor::StateIdle) {
        m_reservoir.enable();
        m_state = StateSenseReservoir;
        dbg() << "state: sense reservoir\n";
      }
      break;
    case StateSenseReservoir:
      m_reservoir.run();
      if (m_reservoir.getState() == Sensor::StateReady) {
        
        auto fill = m_reservoir.read();
        m_reservoir.disable();
        
        if (fill < m_settings.m_threshReservoir) {
          
          m_reservoirEmptyMillis = millis();
          m_state = StateReservoirEmpty;
          
          err() << "reservoir empty, read: " << fill << ", thresh: " << m_settings.m_threshReservoir << ". state: " << getStateString(m_state) << "\n";

          // TODO: this is the point to generate an alarm

        } else {
          m_state = StateWaitPump;
          dbg() << "reservoir ok, read: " << fill << ", thresh: " << m_settings.m_threshReservoir << ". state: wait pump\n";
        }
      }
      break;
    case StateWaitPump:
      if (not m_pump.isEnabled()) {
        m_valve.open();
        m_pump.enable();
        m_state = StatePump;
        dbg() << "state: " << getStateString(m_state) << "\n";
      }
      break;
    case StatePump:
      if (m_pump.secondsEnabled() >= m_settings.m_pumpSeconds) {
        m_pump.disable();
        m_valve.close();
        m_soakStartMillis = millis();
        m_state = StateSoak;
        dbg() << "state: " << getStateString(m_state) << "\n";
      }
      break;
    case StateSoak:
      if ((millis() - m_soakStartMillis) / (60 * 1000) > m_settings.m_soakMinutes) {
        m_iterations++;
        if (m_iterations >= m_settings.m_maxIterations) {
          m_state = StateIdle;
          err() << "maximum iterations (" << m_settings.m_maxIterations << ") reached -- forcing circuit into state: " << getStateString(m_state) << "\n";
          dbg() << "keeping your plants from being overflowed :)\n";
        } else {
          m_state = StateWaitSensor;
          dbg() << "state: " << getStateString(m_state) << "\n";
        }
      }
      break;

    case StateReservoirEmpty:
      if (millis() - m_reservoirEmptyMillis > RecheckReservoirMs) {
        m_state = StateWaitReservoir;
        dbg() << "rechecking reservoir after waiting for " << RecheckReservoirMs / 60UL / 1000UL << " minutes, state: " << getStateString(m_state) << "\n";
      }
      break;
  }
}

void
WaterCircuit::reset()
{
  if (m_state != StateIdle) {
    if (m_state == StateSense) {
      m_sensor.disable();
    }
    if (m_state == StateSenseReservoir) {
      m_reservoir.disable();
    }
    if (m_state == StatePump) {
      m_pump.disable();
      m_valve.close();
    }
    m_state = StateIdle;
    dbg() << "state: " << getStateString(m_state) << " (by reset)\n";
  }
}

/* duplicating this here since we'd like to move this code into a library */
inline Print&
prtFmt(Print& prt, const char *fmt, ... )
{
  char buf[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 128, fmt, args);
  va_end (args);
  prt.print(buf);
  return prt;
}

Print&
WaterCircuit::prt(Print& p) const
{
  p
    <<  (m_settings.m_pumpSeconds == 0 ? " off\n" :  "   on\n")
    <<  "            pump time  "; prtFmt(p, "%3u s\n", m_settings.m_pumpSeconds)
    <<  "           dry thresh  "; prtFmt(p, "%3u\n", m_settings.m_threshDry)
    <<  "           wet thresh  "; prtFmt(p, "%3u\n",  m_settings.m_threshWet)
    <<  "            soak time  "; prtFmt(p, "%3u m\n", m_settings.m_soakMinutes)
    <<  "     reservoir thresh  "; (m_settings.m_threshReservoir == 0 ? p << "off\n" : prtFmt(p, "%3u\n", m_settings.m_threshReservoir))
    <<  "----------------------------\n"
    <<  "   last read humidity  " << m_currentHumidity << "\n"
    <<  "accumulated pump time  " << m_pump.getTotalEnabledSeconds() << " s\n"
    <<  "                state  " << getStateString() << "\n"
    <<  "           iterations  " << m_iterations << "\n"
    ;
  return p;
}

