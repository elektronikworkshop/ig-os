#ifndef EW_IG_CONFIG_H
#define EW_IG_CONFIG_H

#include <Arduino.h>

const unsigned int SpiSckPin   = D5;
const unsigned int SpiMosiPin  = D7;
const unsigned int SpiLatchPin = D8;

const unsigned int OneWirePin  = D4;

/** Pin to enable the analog power supply */
const unsigned int SensorPowerPin = D6;
const unsigned int SensorAdcPin   = A0;

const unsigned int TelnetPort = 23;
const unsigned int MaxTelnetClients = 1;
const unsigned int SizeErrLogBuffer = 2048;

const unsigned int NumWaterCircuits = 4;
const unsigned int NumSchedulerTimes = 8;

#define DefaultHostName "ew-intelliguss"

#define WelcomeMessage(what)                          \
  "Welcome to the Intelli-Güss " what " interface!\n" \
  "Copyright (c) 2017 Elektronik Workshop\n"          \
  "Type \"help\" for available commands\n"


/* global functions which look for a better home later in the development cycle ... */

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

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

inline const char*
prtFmt(char* buf, size_t buflen, const char *fmt, ... )
{
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, buflen, fmt, args);
  va_end (args);
  return buf;
}

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

template<unsigned int _MaxStreams>
class LogProxy
  : public Print
{
public:
  static const unsigned int MaxStreams = _MaxStreams;
  LogProxy(bool enabled = true)
    : m_streams{0}
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
  bool isEnabled() const
  {
    return m_enabled;
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

template<unsigned int _BufferSize, unsigned int _MaxStreams>
class BufferedLogProxy
  : public LogProxy<_MaxStreams>
{
public:
  static const unsigned int BufferSize = _BufferSize;
  
  BufferedLogProxy(bool enabled = true)
    : LogProxy<_MaxStreams>(enabled)
    , m_buffer{0}
    , m_writeIndex(0)
  { }
  struct BufNfo
  {
    const char* a;
    unsigned int na;
    const char* b;
    unsigned int nb;
  };
  void getBuffer(BufNfo& nfo) const
  {
    nfo.a = m_buffer + m_writeIndex;
    nfo.na = BufferSize - m_writeIndex;
    nfo.b = m_buffer;
    nfo.nb = m_writeIndex;
  }
protected:
  virtual size_t write(uint8_t c)
  {
    if (not this->isEnabled()) {
      return 1;
    }

    /* write to buffer */
    
    m_buffer[m_writeIndex++] = c;
    if (m_writeIndex == BufferSize) {
      m_writeIndex = 0;
    }

    return LogProxy<_MaxStreams>::write(c);
  }
private:
  char m_buffer[BufferSize];
  unsigned int m_writeIndex;
};

/* we could back up the log buffers to flash, but that's probably overkill */

extern LogProxy<MaxTelnetClients> Log;
extern LogProxy<MaxTelnetClients> Debug;
typedef BufferedLogProxy<SizeErrLogBuffer, MaxTelnetClients> ErrorLogProxy;
extern ErrorLogProxy Error;

#endif  /* EW_IG_CONFIG_H */

