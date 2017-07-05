/** Ripped from
 *  https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/eeMem.h
 *  
 *  consider for later incorporation:
 *  https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/WiFiManager.h
 */
#ifndef EW_IG_FLASH_H
#define EW_IG_FLASH_H

#include "system.h"

const unsigned int MaxSsidNameLen = 64;
const unsigned int MaxSsidPassLen = 64;

struct FlashDataSet // FLASH backed data
{
  uint16_t                size;          // if size changes, use defaults
  uint16_t                sum;           // if sum is different from memory struct, write
  char                    wifiSsid[MaxSsidNameLen];
  char                    wifiPass[MaxSsidPassLen];

  /* TODO: instead of copying this stuff around and have it redundant in memory we could map it directly into those objects by reference */
  WaterCircuit::Settings     waterCircuitSettings[NumWaterCircuits];
  SchedulerTime::Time        schedulerTimes[NumSchedulerTimes];
  ThingSpeakLogger::Settings thingSpeakLoggerSettings[NumWaterCircuits];
  
  uint8_t  reserved[32];
  
#if 0
  uint16_t coolTemp[2]; // cool to temp *10 low/high
  uint16_t heatTemp[2]; // heat to temp *10 low/high
  int16_t  cycleThresh[2]; // temp range for cycle *10
  uint8_t  Mode;        // Off, Cool, Heat, Auto
  uint8_t  eHeatThresh; // degree threshold to switch to gas
  uint16_t cycleMin;    // min time to run
  uint16_t cycleMax;    // max time to run
  uint16_t idleMin;     // min time to not run
  uint16_t filterMinutes; // resettable minutes run timer (200 hours is standard change interval)
  uint16_t fanPostDelay[2]; // delay to run auto fan after [hp/cool] stops
  uint16_t fanPreTime[2]; // fan pre-run before [cool/heat]
  uint16_t overrideTime; // time used for an override
  uint8_t  heatMode;    // heating mode (gas, electric)
  int8_t   tz;          // current timezone and DST
  int8_t   adj;         // temp offset adjust
  uint8_t  humidMode;   // Humidifier modes
  uint16_t rhLevel[2];  // rh low/high
  int16_t  awayDelta[2]; // temp offset in away mode[cool][heat]
  uint16_t awayTime;    // time limit for away offset (in minutes)
  uint16_t fanCycleTime; // for user fan cycles
  unsigned long hostIp;
  uint16_t  hostPort;
  char     zipCode[8];  // Your zipcode
  char     password[32];
  bool     bLock;
  bool     bNotLocalFcst; // Use weather.gov server
  uint16_t ppkwh;
  uint16_t ccf;
  uint8_t  fcRange; // number in forecasts (3 hours)
  uint8_t  fcDisplay; // number in forecasts (3 hours)
#endif
};

extern FlashDataSet flashDataSet;

class FlashMemory
{
public:
  void begin();
  void update(void);
private:
  static uint16_t fletcher16( uint8_t* data, int count);
};

extern FlashMemory flashMemory;

#endif  /* #define EW_IG_FLASH_H */

