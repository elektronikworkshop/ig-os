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
  static const unsigned char AdcMask    = 0b00000111;
  static const unsigned char AdcShift   = 0;

  static const unsigned char PumpMask   = 0b00001000;
  static const unsigned char PumpShift  = 3;
  
  static const unsigned char ValveMask  = 0b11110000;
  static const unsigned char ValveShift = 4;
  
  Spi()
    : m_register(0)
  {
    
  }
  void begin()
  {
    pinMode(SpiLatchPin, OUTPUT);

    SPI.begin();

    /* MSBFIRST is default, for convenience we switch to LSBFIRST
     *  since our R_SENSE_MUX is at lowest in the local byte and
     *  highest in the register byte.
     */
    SPI.setBitOrder (LSBFIRST);
    
    m_register = 0;
    transmit();
  }
  void run()
  {
    
  }
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
  } Valve;
  /**
   * Note: only one bitfield can be active at once.
   */
  bool setValve(Valve valve)
  {
    unsigned char val = static_cast<unsigned char>(valve);
    
    /* We allow only one valve to be active at once */
    if (countBits(val) > 1) {
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
    /* shift */
    SPI.transfer(m_register);
    /* latch */
    digitalWrite(SpiLatchPin, HIGH);
  }

  /** Local copy of the shift register */
  unsigned char m_register;
};

extern Spi spi;

#endif /* EW_IG_SPI_H */

