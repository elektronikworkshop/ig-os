#ifndef EW_IG_CONFIG_H
#define EW_IG_CONFIG_H

#include <Arduino.h>

const unsigned int SpiSckPin   = D5;
const unsigned int SpiMosiPin  = D7;
const unsigned int SpiLatchPin = D8;

/** Pin to enable the analog power supply */
const unsigned int SensorPowerPin = D4;
const unsigned int SensorAdcPin   = A0;

#define DefaultHostName "ew-intelliguss"

#define WelcomeMessage(what)                          \
  "Welcome to the Intelli-Güss " what " interface!\n" \
  "Copyright (c) 2017 Elektronik Workshop\n"          \
  "Type \"help\" for available commands\n"



template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

template <typename T>
inline unsigned int countBits(T value)
{
  T mask = 0x1;
  unsigned int count = 0;
  for (unsigned int i = 0; i < sizeof(T) * 8; i++, mask <<= 1) {
    count += (value & mask) != 0 ? 1 : 0;
  }
  return count;
}

template<unsigned char shift, unsigned char mask, typename T>
inline void setBitfields(T& target, T value)
{
  value <<= shift;
  value &= mask;
  target &= ~mask;
  target |= value;
}

template<unsigned char shift, unsigned char mask, typename T>
inline T getBitfields(T& target)
{
  T value = target & mask;
  value >>= shift;
  return value;
}


class LogProxy
  : public Print
{
public:
  static const unsigned int MaxStreams = 1;
  LogProxy(bool enabled = true)
    : m_streams({0})
    , m_n(0)
    , m_enabled(enabled)
  { }
  bool addClient(Print& stream)
  {
    for (uint8_t i = 0; i < MaxStreams; i++) {
      if (m_streams[i] == &stream) {
        return true;
      }
    }
    for (uint8_t i = 0; i < MaxStreams; i++) {
      if (not m_streams[i]) {
        m_streams[i] = &stream;
        m_n++;
        return true;
      }
    }
    return false;
  }
  bool removeClient(Print& stream)
  {
    for (uint8_t i = 0; i < MaxStreams; i++) {
      if (m_streams[i] == &stream) {
        m_streams[i] = NULL;
        m_n--;
        return true;
      }
    }
    return false;
  }
  void enable(bool enable)
  {
    m_enabled = enable;
  }
protected:
  virtual size_t write(uint8_t c)
  {
    if (not m_enabled) {
      return 1;
    }
    
    size_t ret = Serial.write(c);
    
    if (m_n) {
      for (uint8_t i = 0; i < MaxStreams; i++) {
        if (m_streams[i]) {
          m_streams[i]->write(c);
        }
      }
    }
    return ret;
  }
private:
  Print* m_streams[MaxStreams];
  uint8_t m_n;
  bool m_enabled;
};

extern LogProxy Log;
extern LogProxy Debug;
extern LogProxy Error;

#endif  /* EW_IG_CONFIG_H */

