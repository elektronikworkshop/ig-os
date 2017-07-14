
#ifndef EW_WATER_CIRCUIT
#define EW_WATER_CIRCUIT

#include <Arduino.h>

/**
 * Note that pumps valves sensors that are part of multiple watering circuits get their begin() member function called once for each circuit. 
 * 
 * 
 * TODO: Should migrate everything to seconds.
 */
class Pump
{
public:
  Pump()
    : m_enabled(false)
    , m_startMillis(0)
    , m_totalEnabledMs(0)
  { }
  virtual void begin() {};
  virtual void enable()
  {
    m_startMillis = millis();
    m_enabled = true;
  }
  virtual void disable()
  {
    if (m_enabled) {
      m_enabled = false;
      m_totalEnabledMs += millis() - m_startMillis;
    }
  }
  bool isEnabled() const
  {
    return m_enabled;
  }
  unsigned int secondsEnabled() const
  {
    if (not m_enabled) {
      return 0;
    }
    return (millis() - m_startMillis) / 1000;
  }
  unsigned int getTotalEnabledSeconds(bool clear = false)
  {
    unsigned long long ret = m_totalEnabledMs;
    if (clear) {
      m_totalEnabledMs = 0;
    }
    return ret / 1000;
  }
private:
  bool m_enabled;
  unsigned long m_startMillis;
  unsigned long long m_totalEnabledMs;
};

/** Humidity sensor base class, all sensor types should derive from it.
 *
 */
class Sensor
{
public:
  typedef enum
  {
    StateIdle = 0,
    StatePrepare,
    StateConvert,
    StateReady,
  } State;

  Sensor()
    : m_state(StateIdle)
  {
  }
  virtual void begin() = 0;

  virtual State getState() const
  {
    return m_state;
  }
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual uint8_t read() = 0;
  virtual void run() = 0;
  static const char* getStateString(State state)
  {
    return (state == StateIdle    ? "Idle"    :
           (state == StatePrepare ? "Prepare" :
           (state == StateConvert ? "Convert" :
                                      "Ready")));
  }
protected:
  void setState(State newState)
  {
    m_state = newState;
  }
private:
  State m_state;
};

/** Water valve base class, all valve types should derive from it.
 *
 */
class Valve
{
public:
  virtual void begin() {}
  virtual void open() = 0;
  virtual void close() = 0;
  virtual bool isOpen() const = 0;
private:
};


/* TODO: remember when run previously
 *
 * TODO: define something like max-iterations to avoid the pump to run infinitely because of an empty reservoir without reservoir sensor
 *
 */

class WaterCircuit
{
public:
  /** When reservoir is empty, recheck its state every 30 minutes.
   *  If the reservoir is refilled in the meanwhile, watering will continue.
   */
  static const unsigned long RecheckReservoirMs = 30UL * 60UL * 1000UL;

  /**
   * 
   */
  typedef struct
  {
    /** How long the pump should be active between measurements.
     *  If m_pumpSeconds is zero (0), the watering circuit is disabled.
     */
    uint8_t m_pumpSeconds;
    /** The sensor threshold when the soil is considered dry. Range 0 .. 255. */
    uint8_t m_threshDry;
    /** The sensor threshold when the soil is considered wet. Range 0 .. 255. */
    uint8_t m_threshWet;
    /** How long the pumped in water should soak before measuring the humidity again. */
    uint8_t m_soakMinutes;
    /** Threshold below which the reservoir should be considered empty. Range 0 .. 255, whereas 0 turns reservoir checking off. */
    uint8_t m_threshReservoir;
    /** Maximum number of watering cycles after which the watering should return to idle even if we read a dry sensor value.
     *  This can be useful in case there should be an error with the humidity sensor which would drain the reservoir and
     *  flood your plant.
     */
    uint8_t m_maxIterations;
  } Settings;

  typedef enum
  {
    StateIdle = 0,
    /** Circuit is triggered but is waiting until busy hardware gets ready, e.g. sensors */
    StateWaitSensor,
    StateSense,
    StateWaitReservoir,
    StateSenseReservoir,
    StateWaitPump,
    StatePump,
    StateSoak,

    StateReservoirEmpty,
  } State;

  WaterCircuit(const unsigned int& id,
               Sensor& sensor,
               Valve& valve,
               Pump& pump,
               Sensor& reservoir,
               Settings& settings);

  State getState() const
  {
    return m_state;
  }

  static const char* getStateString(State state);
  
  const char* getStateString() const
  {
    return getStateString(m_state);
  }

  void begin();
  void trigger();
  void run();
  void reset();

  unsigned int getId() const {return m_id;}
  const Settings& getSettings() const { return m_settings; }

  uint8_t getPumpSeconds() const { return m_settings.m_pumpSeconds; }
  uint8_t getThreshDry()   const { return m_settings.m_threshDry; }
  uint8_t getThreshWet()   const { return m_settings.m_threshWet; }
  uint8_t getSoakMinutes() const { return m_settings.m_soakMinutes; }
  uint8_t getThreshReservoir() const { return m_settings.m_threshReservoir; }
  uint8_t getMaxIterations() const { return m_settings.m_maxIterations; }

  void setPumpSeconds(uint8_t s) { m_settings.m_pumpSeconds = s; }
  void setThreshDry(uint8_t t)   { m_settings.m_threshDry = t; }
  void setThreshWet(uint8_t t)   { m_settings.m_threshWet = t; }
  void setSoakMinutes(uint8_t m) { m_settings.m_soakMinutes = m; }
  void setThreshReservoir(uint8_t t) { m_settings.m_threshReservoir = t; }
  void setMaxIterations(uint8_t i) const { m_settings.m_maxIterations = i; }

  uint8_t getHumidity()    const { return m_currentHumidity; }
  uint8_t getNumIterations() const { return m_iterations; }

  bool isEnabled() const
  {
    return m_settings.m_pumpSeconds > 0;
  }
  
  Sensor& getSensor() { return m_sensor; }
  Valve& getValve() { return m_valve; }
  Pump& getPump() { return m_pump; }
  Sensor& getReservoir() { return m_reservoir; }

  Print& prt(Print& p) const;

protected:
  virtual Print& dbg() const { return Serial; }
  virtual Print& err() const { return Serial; }
//  virtual Time& time() const {}

private:
  /** Watering circuit ID */
  unsigned int m_id;
  Settings& m_settings;

  Sensor& m_sensor;
  Valve& m_valve;
  Pump& m_pump;
  Sensor& m_reservoir;
  
  /* State variables */
  State m_state;
  /** detect issues when a circuits waters forever */
  uint8_t m_iterations;
  uint8_t m_currentHumidity;
  unsigned long m_soakStartMillis;
  unsigned long m_reservoirEmptyMillis;

  /* TODO: statistics */
};


#endif  /* #ifndef EW_WATER_CIRCUIT */
