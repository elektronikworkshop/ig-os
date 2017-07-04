/**
 * https://arduino.stackexchange.com/questions/16348/how-do-you-use-spi-on-an-arduino
 * https://www.arduino.cc/en/Reference/SPI
 * https://www.arduino.cc/en/Reference/SPITransfer
 */
 
#include <SPI.h>
#include "name.h"

/* it's ridiculous - the arduino IDE destroys my template if template and
 *  function declaration are not on the same line!
 *  
 */
template<unsigned char shift, unsigned char mask, class T> void setBitfields(T& target, T value)
{
  value <<= shift;
  value &= mask;
  target &= ~mask;
  target |= value;
}

const unsigned int SpiSckPin   = D5;
const unsigned int SpiMosiPin  = D7;
const unsigned int SpiLatchPin = D8;

static const unsigned char AdcMask    = 0b11100000;
static const unsigned char AdcShift   = 5;

static const unsigned char PumpMask   = 0b00000001;
static const unsigned char PumpShift  = 0;

static const unsigned char ValveMask  = 0b00011110;
static const unsigned char ValveShift = 1;

void transmit(uint8_t v)
{
  digitalWrite(SpiLatchPin, LOW);
  /* shift */
  SPI.transfer(v);
  /* latch */
  digitalWrite(SpiLatchPin, HIGH);
}

uint8_t valveValue(Valve valve)
{
  uint8_t v = 0;
  setBitfields<ValveShift, ValveMask>(v, static_cast<unsigned char>(valve));
  Serial.println(v, BIN);
  return v;
}

void setup() {

  Serial.begin(115200);
  
  pinMode(SpiLatchPin, OUTPUT);

  SPI.begin();

  /* MSBFIRST is default, for convenience we switch to LSBFIRST
   *  since our R_SENSE_MUX is at lowest in the local byte and
   *  highest in the register byte.
   */
  SPI.setBitOrder (MSBFIRST);
}


void loop()
{
    transmit(PumpMask);
    delay(1000);
    transmit(valveValue(Valve1));
    delay(1000);
    transmit(valveValue(Valve2));
    delay(1000);
    transmit(valveValue(Valve3));
    delay(1000);
    transmit(valveValue(Valve4));
    delay(1000);
}
