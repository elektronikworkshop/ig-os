#ifndef EW_IG_SPI_H
#define EW_IG_SPI_H

/**
 * https://arduino.stackexchange.com/questions/16348/how-do-you-use-spi-on-an-arduino
 * https://www.arduino.cc/en/Reference/SPI
 * https://www.arduino.cc/en/Reference/SPITransfer
 */
 
#include <SPI.h>
#include "config.h"

class Spi
{
public:
  static const unsigned char AdcMask    = 0b11100000;
  static const unsigned char AdcShift   = 5;
  
  static const unsigned char PumpMask   = 0b00000001;
  static const unsigned char PumpShift  = 0;
  
  static const unsigned char ValveMask  = 0b00011110;
  static const unsigned char ValveShift = 1;
  
  Spi()
    : m_register(0)
  { }
  void begin()
  {
    pinMode(SpiLatchPin, OUTPUT);

    SPI.begin();
    SPI.setBitOrder (MSBFIRST);
    
    m_register = 0;
    transmit();
  }
  void run()
  { }
  void setAdcChannel(unsigned char channel)
  {
    setBitfields<AdcShift, AdcMask>(m_register, channel);
    transmit();
  }
  unsigned char getAdcChannel()
  {
    return getBitfields<AdcShift, AdcMask>(m_register);
  }
  
  void setPump(bool enable)
  {
    unsigned char ena =  enable ? 1 : 0;
    setBitfields<PumpShift, PumpMask>(m_register,ena);
    transmit();
  }
  bool getPump()
  {
    return getBitfields<PumpShift, PumpMask>(m_register) != 0;
  }
  
  typedef enum
  {
    ValveNone = 0,
    Valve1 = 0b0001,
    Valve2 = 0b0010,
    Valve3 = 0b0100,
    Valve4 = 0b1000,
    ValveCount = 4,
  } Valve;
  /**
   * Note: only one bitfield can be active at once.
   */
  bool setValve(Valve valve)
  {
    unsigned char val = static_cast<unsigned char>(valve);
    
    /* We allow only one valve to be active at once */
    if (countBits(val) > 1) {
      Error << "TOO MANY BITS FOR VALVE\n";
      return false;
    }
    
    setBitfields<ValveShift, ValveMask>(m_register, val);
    transmit();

    return true;
  }
  Valve getValve()
  {
    return static_cast<Spi::Valve>(getBitfields<ValveShift, ValveMask>(m_register));
  }

private:

  void transmit()
  {
    digitalWrite(SpiLatchPin, LOW);    
    SPI.transfer(m_register);
    digitalWrite(SpiLatchPin, HIGH);
  }

  /** Local copy of the shift register */
  unsigned char m_register;
};

extern Spi spi;

class Adc
{
public:
  typedef enum
  {
    StateIdle = 0,
    StatePoweringUp,
    StatePowerUpIdle,
    StateAdcSetup,
    StateReady,
  } State;
  typedef enum
  {
    ChSensor1 = 0,
    ChSensor2,
    ChSensor3,
    ChSensor4,
    ChReservoir,
  } Channel;

  static const unsigned int msPowerUp   =  2000;
  static const unsigned int msAdcSetup  =  1000;
  static const unsigned int msPowerDown =  2000;

  Adc()
    : m_state(StateIdle)
  { }
  
  void begin();  
  void run();  
  bool request(Channel channel);

  State getState() const
  {
    return m_state;
  }
  bool isReady() const
  {
    return m_state == StateReady;
  }
  uint16_t read() const;
  void reset();
private:
  void changeState(State newState);
  
  State m_state;
  Channel m_channel;
  unsigned long m_lastStateChangeMs;
};

extern Adc adc;

#endif /* EW_IG_SPI_H */

