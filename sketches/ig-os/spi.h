#ifndef EW_IG_SPI_H
#define EW_IG_SPI_H

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
    : m_register
  {
    
  }
  void begin()
  {
    transmit();
  }
  void run()
  {
    
  }
  void selectAdcChannel(unsigned char channel)
  {
    setBitfields<AdcShift, AdcMask>(channel);
    transmit();
  }
  void enablePump(bool enable)
  {
    setBitfields<PumpShift, PumpMask>(enable ? 1 : 0);
    transmit();
  }
  typedef enum
  {
    ValveNone = 0,
    Valve1 = 0b0001,
    Valve2 = 0b0010,
    Valve3 = 0b0100,
    Valve4 = 0b1000,
  } Valve;
  void selectValve(Valve valve)
  {
    setBitfields<ValveShift, ValveMask>(valve);
    transmit();
  }
  
private:
  template<unsigned char shift, unsigned char mask>
  inline void setBitfields(unsigned char value)
  {
    value <<= shift;
    value &= mask;
    m_register &= mask;
    m_register |= value;
  }
  void transmit()
  {
    /* shift */
    /* latch */
  }

  /** Local copy of the shift register */
  unsigned char m_register;
};

#endif /* EW_IG_SPI_H */

