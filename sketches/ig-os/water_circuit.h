
#ifndef EW_WATER_CIRCUIT
#define EW_WATER_CIRCUIT

class Pump
{
public:
  Pump()
    : m_enabled(false)
    , m_startMillis(0)
  { }
  virtual void begin() = 0;
  virtual void enable()
  {
    m_startMillis = millis();
    m_enabled = true;
  }
  virtual void disable()
  {
    if (m_enabled) {
      m_enabled = false;
      m_totalEnabledTime += millis() - m_startMillis;
    }
  }
  bool isEnabled() const
  {
    return m_enabled;
  }
  unsigned int millisEnabled() const
  {
    if (not m_enabled) {
      return 0;
    }
    return millis() - m_startMillis;
  }
  unsigned long long getTotalEnabledTimeMs(bool clear = false)
  {
    unsigned long long ret = m_totalEnabledTime;
    if (clear) {
      m_totalEnabledTime = 0;
    }
    return ret;
  }
private:
  bool m_enabled;
  unsigned long m_startMillis;

  unsigned long long m_totalEnabledTime;
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
  virtual float read() = 0;
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
  WaterCircuit(const unsigned int& id,
               Sensor& sensor,
               Valve& valve,
               Pump& pump,
               const unsigned int& pumpMillis,
               const float& threshDry,
               const float& threshWet,
               const unsigned int& soakMillis)
    : m_id(id)
    , m_sensor(sensor)
    , m_valve(valve)
    , m_pump(pump)
    , m_pumpMillis(pumpMillis)
    , m_threshDry(threshDry)
    , m_threshWet(threshWet)
    , m_soakMillis(soakMillis)

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
            if ((    m_firstIteration and m_currentHumidity <= m_threshDry) or
                (not m_firstIteration and m_currentHumidity <= m_threshWet))
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
        if (m_pump.millisEnabled() >= m_pumpMillis) {
          m_pump.disable();
          m_valve.close();
          m_soakStartMillis = millis();
          m_state = StateSoak;
          Serial.println("Water state: Soaking");
        }
        break;
      case StateSoak:
        if (millis() - m_soakStartMillis > m_soakMillis) {
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

  unsigned int getPumpMillis() const { return m_pumpMillis; }
  float getThreshDry() const { return m_threshDry; }
  float getThreshWet() const { return m_threshWet; }
  unsigned int getSoakMillis() const { return m_soakMillis; }
  float getHumidity() const { return m_currentHumidity; }

  void setPumpMillis(unsigned int ms) { m_pumpMillis = ms; }
  void setThreshDry(float thresh) { m_threshDry = thresh; }
  void setThreshWet(float thresh) { m_threshWet = thresh; }
  void setSoakMillis(unsigned int ms) { m_soakMillis = ms; }

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
  /** How long the pump should be active between measurements. */
  unsigned int m_pumpMillis;
  /** The sensor threshold when the soil is considered dry */
  float m_threshDry;
  /** The sensor threshold when the soil is considered wet */
  float m_threshWet;
  /** How long the pumped in water should soak before measuring the humidity again */
  unsigned int m_soakMillis;

  Sensor& m_sensor;
  Valve& m_valve;
  Pump& m_pump;

  /* State variables */
  State m_state;
  bool m_firstIteration;
  float m_currentHumidity;
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
