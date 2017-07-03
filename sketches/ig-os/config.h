#ifndef EW_IG_CONFIG_H
#define EW_IG_CONFIG_H

#include <Arduino.h>

const unsigned int SpiSckPin   = D5;
const unsigned int SpiMosiPin  = D7;
const unsigned int SpiLatchPin = D8;

/** Pin to enable the analog power supply */
const unsigned int SensorPowerPin = D4;
const unsigned int SensorAdcPin   = A0;


template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

template <typename T>
inline unsigned int countBits(T value)
{
  T mask = 0x1;
  unsigned int count = 0;
  for (unsigned int i = 0; i < sizeof(T) * 8; i++, mask <<= 1) {
    count += value & mask != 0 ? 1 : 0;
  }
  return count;
}

template<unsigned char shift, unsigned char mask, typename T>
inline void setBitfields(T& target, T value)
{
  value <<= shift;
  value &= mask;
  target &= mask;
  target |= value;
}

template<unsigned char shift, unsigned char mask, typename T>
inline T getBitfields(T& target)
{
  T value = target & mask;
  value >>= shift;
  return value;
}

#endif  /* EW_IG_CONFIG_H */

