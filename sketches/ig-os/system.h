#ifndef EW_IG_SYSTEM
#define EW_IG_SYSTEM

#include "config.h"
#include "circuit.h"
#include "log.h"

#include <climits>

/* For more info see:
 *  
 * https://github.com/arduino-libraries/NTPClient
 * https://github.com/arduino-libraries/NTPClient/blob/master/NTPClient.h
 */
#include <NTPClient.h>

extern NTPClient timeClient;

/**
 * TODO:
 *  * When switching from Auto to Manual: make sure that any watering process is terminated
 *  * Remove "Off" mode since it isn't used
 */
class SystemMode
{
public:
  typedef enum
  {
    Invalid = -1,
    Off = 0,
    Auto,
    Manual,
  } Mode;

  SystemMode()
    : m_mode(Auto)
  { }

  Mode getMode() const
  {
    return m_mode;
  }
  void setMode(Mode mode)
  {
    m_mode = mode;
  }
  static Mode str2Mode(const char* modeStr)
  {
    if (strcmp(modeStr, "off") == 0) {
      return Off;
    } else if (strcmp(modeStr, "auto") == 0) {
      return Auto;
    } else if (strcmp(modeStr, "man") == 0) {
      return Manual;
    } else {
      return Invalid;
    }
  }
private:
  Mode m_mode;
};

extern SystemMode systemMode;



extern WaterCircuit* circuits[NumWaterCircuits + 1];

class SchedulerTime
{
public:
  static const uint8_t InvalidDay = UINT8_MAX;
  static const uint8_t InvalidHour = UINT8_MAX;
  
  struct Time
  {
    uint8_t  m_hour;
    uint8_t  m_minute;
  };
/*  
  SchedulerTime()
    : m_dayDone(InvalidDay)
    , m_time({InvalidHour, 0})
  {}
  SchedulerTime(uint8_t hour,
                uint8_t minute)
    : m_dayDone(InvalidDay)
    , m_time({hour, minute})
  { }
*/
  SchedulerTime(Time& t)
    : m_dayDone(InvalidDay)
    , m_time(t)
  { }
  uint8_t getHour() const { return m_time.m_hour; }
  uint8_t getMinute() const { return m_time.m_minute; }

  void setHour(uint8_t hour) { m_time.m_hour = hour; }
  void setMinute(uint8_t minute) { m_time.m_minute = minute; }

  /** Checks if due. If due it marks it with the day done and returns true. */
  bool isDue()
  {
    if (not isValid()) {
      return false;
    }
    
    /* Check if we processed it already today */
    if (m_dayDone == timeClient.getDay()) {
      return false;
    }

    bool due =
      timeClient.getDay()     != m_dayDone        and
      timeClient.getHours()   == m_time.m_hour    and
      timeClient.getMinutes() == m_time.m_minute;
    
    if (due) {
      m_dayDone = timeClient.getDay();
      Serial << "detected watering due at " << timeClient.getFormattedTime() << "\n";
    }
    
    return due;
  }
  void markDone()
  {
    m_dayDone = timeClient.getDay();
  }
  bool isValid() const
  {
    return m_time.m_hour <= 23;
  }
private:
  /** 0 Sunday, 1 Monday, ... */
  uint8_t  m_dayDone;
  Time& m_time;
};


extern SchedulerTime* schedulerTimes[NumSchedulerTimes + 1];

bool wateringDue();


extern Logger* loggers[NumWaterCircuits + 1];
void loggerBegin();
void loggerRun();

namespace history {

  void skipLines(unsigned int& skip, unsigned int& i, const char* buf, unsigned int count);
  void prtLines(Print& prt, unsigned int& numLines, const char* buf, unsigned int count);
  bool prt(Print& prt, int start = -1, int end = 0);
  
} /* namespace history */


#include <Wire.h>

class I2cDevice
{
public:
  struct Config
  {
    uint8_t m_type;
    uint8_t m_address;
  };
  void begin(uint8_t address)
  {
    m_address = address;
    Wire.begin();
  }

  uint8_t getDeviceAddress()
  {
    return m_address;
  }
private:
  uint8_t m_address;
};

/**
 * Code mostly stolen and then modified from:
 * https://github.com/jlesech/Eeprom24C32_64/blob/master/Eeprom24C32_64.cpp
 */
class I2cAt24Cxx
  : public I2cDevice
{
public:
  typedef uint16_t Address;
  /** EEPROM page size (see datasheet)
   */
  static const uint16_t PageSize = 32;

  /** Read buffer size in Wire library.
   */
  static const uint16_t WireReadBufferSize = BUFFER_LENGTH;

  /** Wire write buffer length, Two bytes are reserved for address.
   */
  static const uint16_t WireWriteBufferSize = BUFFER_LENGTH - 2;

  I2cAt24Cxx(size_t size = 4096)
    : m_size(size)
  { }
  uint8_t readByte(Address address)
  {
      Wire.beginTransmission(getDeviceAddress());
      Wire.write(address >> 8);
      Wire.write(address & 0xFF);
      Wire.endTransmission();
      Wire.requestFrom(getDeviceAddress(), (uint8_t)1);
      uint8_t data = 0;
      if (Wire.available()) {
          data = Wire.read();
      }
      return data;
  }
  void read(Address address, uint8_t* data, size_t count)
  {
    size_t nBuffers = count / WireReadBufferSize;
    
    for (; nBuffers; nBuffers--) {
      
        readBuffer(address, data, WireReadBufferSize);
        
        address += WireReadBufferSize;
        data    += WireReadBufferSize;
        count   -= WireReadBufferSize;
    }

    if (count) {
      readBuffer(address, data, count);
    }
  }
  void write(Address address, uint8_t data)
  {
    Wire.beginTransmission(getDeviceAddress());
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.write(data);
    Wire.endTransmission();
  }
  void write(Address address, const uint8_t* data, size_t count)
  {
    // Write first page if not aligned.
    size_t notAlignedLength = 0;
    size_t pageOffset = address % PageSize;
    
    if (pageOffset > 0) {
      
        notAlignedLength = std::min(count, PageSize - pageOffset);
        
        writePage(address, data, notAlignedLength);
 
        address += notAlignedLength;
        data    += notAlignedLength;
        count   -= notAlignedLength;
    }

    if (count) {

      size_t nPages = count / PageSize;
      for (; nPages; nPages--) {
        
          writePage(address, data, PageSize);

          address += PageSize;
          data    += PageSize;
          count   -= PageSize;
      }

      if (count) {
          writePage(address, data, count);
      }
    }
  }
private:
  void readBuffer(Address address, uint8_t* data, size_t count)
  {
    Wire.beginTransmission(getDeviceAddress());
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.endTransmission();
    Wire.requestFrom(getDeviceAddress(), count);
    for (; count; count--) {
      if (Wire.available())
      {
          *data++ = Wire.read();
      }
    }
  }
  void writePage(Address address, const uint8_t* data, size_t count)
  {
    size_t nBuffers = count / WireWriteBufferSize;
    for (; nBuffers; nBuffers--)
    {
        writeBuffer(address, data, WireWriteBufferSize);
        
        address += WireWriteBufferSize;
        data    += WireWriteBufferSize;
        count   -= WireWriteBufferSize;
    }
    if (count) {
      writeBuffer(address, data, count);
    }
  }
  void writeBuffer(Address address, const uint8_t* data, size_t count)
  {
      Wire.beginTransmission(getDeviceAddress());
      Wire.write(address >> 8);
      Wire.write(address & 0xFF);
      for (; count; count--)
      {
          Wire.write(*data++);
      }
      Wire.endTransmission();
      
      // Write cycle time (tWR). See EEPROM memory datasheet for more details.
      delay(10);
  }
  size_t m_size;
};

// DS1307RTC library
// http://www.makeuseof.com/tag/how-and-why-to-add-a-real-time-clock-to-arduino/
// use the library example!

#endif /* #ifndef EW_IG_SYSTEM */

