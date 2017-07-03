
#ifndef EW_WATER_CIRCUIT
#define EW_WATER_CIRCUIT

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
    StateReady,
  } State;

  Sensor()
    : m_state(StateIdle)
  {
  }
  virtual void begin() = 0;

  State getState() const
  {
    return m_state;
  }
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual uint8_t read() = 0;
  virtual void run() = 0;
protected:
  void setState(State newState)
  {
    if (newState != m_state) {
      m_state = newState;
      Serial.print("New sensor state: ");
      Serial.println((m_state == StateIdle    ? "Idle" :
                     (m_state == StatePrepare ? "Prepare" :
                                                "Ready")));
    }
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
  virtual void open() {}
  virtual void close() {}
private:
};


/* TODO: remember when run previously
 *
 * TODO: rename StateTriggered to StateSensorWait
 * and add new state StatePumpWait - this way we can interleave
 * sensing, watering and soaking of all circuits and do not have
 * to run them sequentially
 *
 *
 */

class WaterCircuit
{
public:
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
  } Settings;
  WaterCircuit(const unsigned int& id,
               Sensor& sensor,
               Valve& valve,
               Pump& pump,
               const Settings& settings = {0})
    : m_id(id)
    , m_sensor(sensor)
    , m_valve(valve)
    , m_pump(pump)
    , m_settings(settings)

    , m_state(StateIdle)
    , m_firstIteration(true)
    , m_currentHumidity(0)
  { }
  typedef enum
  {
    StateIdle = 0,
    /** Circuit is triggered but is waiting until busy hardware gets ready, e.g. sensors */
    StateTriggered,
    StateSense,
    StateWater,
    StateSoak
  } State;

  State getState() const
  {
    return m_state;
  }
  void begin()
  {
    m_sensor.begin();
    m_valve.begin();
    m_pump.begin();
  }

  void trigger()
  {
    if (m_state != StateIdle) {
      return;
    }
    m_state = StateTriggered;
    m_firstIteration = true;
    Serial.println("Water state: Triggered");
  }
  void run()
  {
    switch (m_state) {
      case StateIdle:
        break;
      case StateTriggered:
        if (m_sensor.getState() == Sensor::StateIdle) {
          m_state = StateSense;
          Serial.println("Water state: Sensing");
        }
        break;
      case StateSense:
        m_sensor.run();
        switch (m_sensor.getState()) {
          case Sensor::StateIdle:
            m_sensor.enable();
            break;
          case Sensor::StatePrepare:
            break;
          case Sensor::StateReady:
            m_currentHumidity = m_sensor.read();
            m_sensor.disable();
            Serial.print("Humidity: ");
            Serial.println(m_currentHumidity);
            /* The first time we check if the soil is dry.
             * After watering we check if it's wet -- so we
             * have a little hysteresis here.
             */
            if ((    m_firstIteration and m_currentHumidity <= m_settings.m_threshDry) or
                (not m_firstIteration and m_currentHumidity <= m_settings.m_threshWet))
            {
              m_valve.open();
              m_pump.enable();
              m_state = StateWater;
              Serial.println("Water state: Watering");
            } else {
              m_state = StateIdle;
              Serial.println("Water state: Idle");
            }
            break;
        }
        break;
      case StateWater:
        if (m_pump.secondsEnabled() >= m_settings.m_pumpSeconds) {
          m_pump.disable();
          m_valve.close();
          m_soakStartMillis = millis();
          m_state = StateSoak;
          Serial.println("Water state: Soaking");
        }
        break;
      case StateSoak:
        if ((millis() - m_soakStartMillis) / (60 * 1000) > m_settings.m_soakMinutes) {
          /* We must return to triggered state to avoid sensor collisions */
          m_state = StateTriggered;
          m_firstIteration = false;
          Serial.println("Water state: Triggered");
        }
        break;
    }
  }
  float getCurrentHumidity() const
  {
    return m_currentHumidity;
  }
  void reset()
  {
    if (m_state != StateIdle) {
      if (m_state != StateTriggered) {
        m_sensor.disable();
        m_pump.disable();
        m_valve.close();
      }
      m_state = StateIdle;
      Serial.println("Water state: Idle by reset");
    }
  }

  unsigned int getId() const {return m_id;}

  uint8_t getPumpSeconds() const { return m_settings.m_pumpSeconds; }
  uint8_t getThreshDry()   const { return m_settings.m_threshDry; }
  uint8_t getThreshWet()   const { return m_settings.m_threshWet; }
  uint8_t getSoakMinutes() const { return m_settings.m_soakMinutes; }
  uint8_t getHumidity()    const { return m_currentHumidity; }

  void setPumpSeconds(uint8_t s) { m_settings.m_pumpSeconds = s; }
  void setThreshDry(uint8_t t)   { m_settings.m_threshDry = t; }
  void setThreshWet(uint8_t t)   { m_settings.m_threshWet = t; }
  void setSoakMinutes(uint8_t m) { m_settings.m_soakMinutes = m; }

  const Settings& getSettings() const { return m_settings; }
  void setSettings(const Settings& settings) { m_settings = settings; }

  bool isEnabled() const
  {
    return m_settings.m_pumpSeconds > 0;
  }

  Sensor& getSensor()
  {
    return m_sensor;
  }
  Valve& getValve()
  {
    return m_valve;
  }
  Pump& getPump()
  {
    return m_pump;
  }

private:
  /** Watering circuit ID */
  unsigned int m_id;
  Settings m_settings;

  Sensor& m_sensor;
  Valve& m_valve;
  Pump& m_pump;

  /* State variables */
  State m_state;
  /* TODO: we sould count iterations here instead of detecting the first iteration only and set a maximum number such that we could detect issues when a circuits waters forever */
  bool m_firstIteration;
  uint8_t m_currentHumidity;
  unsigned long m_soakStartMillis;

  /* TODO: statistics */
};

/** Reservoir base class.
 *
 */
class Reservoir
  : public Sensor
{
public:
private:
};


#endif  /* #ifndef EW_WATER_CIRCUIT */
