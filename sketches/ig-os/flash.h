/** Ripped from
 *  https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/eeMem.h
 *  
 *  consider for later incorporation:
 *  https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/WiFiManager.h
 */
#ifndef EW_IG_FLASH_H
#define EW_IG_FLASH_H

#include "system.h"

const unsigned int MaxWifiSsidLen = 63; // excluding zero termination
const unsigned int MaxWifiPassLen = 63; // excluding zero termination
const unsigned int MaxHostNameLen = 63; // excluding zero termination

struct FlashDataSet // FLASH backed data
{
  uint16_t                size;          // if size changes, use defaults
  uint16_t                sum;           // if sum is different from memory struct, write
  char                    wifiSsid[MaxWifiSsidLen + 1];
  char                    wifiPass[MaxWifiPassLen + 1];

  WaterCircuit::Settings        waterCircuitSettings[NumWaterCircuits];
  SchedulerTime::Time           schedulerTimes[NumSchedulerTimes];
  ThingSpeakLogger::TslSettings thingSpeakLoggerSettings[NumWaterCircuits];

  char hostName[MaxHostNameLen + 1];
  bool telnetEnabled;
  char telnetPass[MaxWifiPassLen + 1];

//  uint8_t  reserved[32];
};

extern FlashDataSet flashDataSet;

class FlashMemoryManager
{
public:
/*
  template<typename T>
  FlashMemoryManager(T& data)
    : m_data(&data)
    , m_len(sizeof(T)
  {}
*/
  void begin();
  void update(void);
private:
/*
  void* m_data;
  const uint16_t m_len;
*/
  static uint16_t fletcher16( uint8_t* data, int count);
};

extern FlashMemoryManager flashMemory;

#endif  /* #define EW_IG_FLASH_H */

