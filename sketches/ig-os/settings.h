#ifndef _EW_IG_FLASH_SETTINGS_H_
#define _EW_IG_FLASH_SETTINGS_H_

#include "config.h"
#include "circuit.h"
#include "system.h"

#include <FlashSettings.h>

const unsigned int MaxWifiSsidLen = 63; // excluding zero termination
const unsigned int MaxWifiPassLen = 63; // excluding zero termination
const unsigned int MaxHostNameLen = 63; // excluding zero termination
const unsigned int MaxTelnetPassLen = 63; // excluding zero termination
const unsigned int MinTelnetPassLen =  5;

struct FlashData
  : public FlashDataBase
{
  char                    wifiSsid[MaxWifiSsidLen + 1];
  char                    wifiPass[MaxWifiPassLen + 1];

  WaterCircuit::Settings        waterCircuitSettings[NumWaterCircuits];
  SchedulerTime::Time           schedulerTimes[NumSchedulerTimes];
  ThingSpeakLogger::TslSettings thingSpeakLoggerSettings[NumWaterCircuits];

  char hostName[MaxHostNameLen + 1];
  bool telnetEnabled;
  char telnetPass[MaxTelnetPassLen + 1];

  bool debug;
  
  FlashData()
    /* router SSID */
    : wifiSsid{""}
    /* router password */
    , wifiPass{""}

    /* WaterCircuit::Settings waterCircuitSettings[NumWaterCircuits]; */
    , waterCircuitSettings
    {
     /* circuit 1 */
     { 30,  /* pump seconds                            */
      180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
      230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
        5,  /* soak minutes                            */
        0,  /* reservoir threshold                     */
       20}, /* maximum iterations                      */
     /* circuit 2 */
     {  0,  /* pump seconds                            */
      180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
      230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
        5,  /* soak minutes                            */
        0,  /* reservoir threshold                     */
       20}, /* maximum iterations                      */
     /* circuit 3 */
     {  0,  /* pump seconds                            */
      180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
      230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
        5,  /* soak minutes                            */
        0,  /* reservoir threshold                     */
       20}, /* maximum iterations                      */
     /* circuit 4 */
     {  0,  /* pump seconds                            */
      180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
      230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
        5,  /* soak minutes                            */
        0,  /* reservoir threshold                     */
       20}, /* maximum iterations                      */
    }
    
    , schedulerTimes
    {{6, 0},
     {8, 0},
     {SchedulerTime::InvalidHour, 0},
     {SchedulerTime::InvalidHour, 0},
     {SchedulerTime::InvalidHour, 0},
     {SchedulerTime::InvalidHour, 0},
     {20, 0},
     {22, 0},
    }
    
    , thingSpeakLoggerSettings
      /* interval (minutes), channel ID, write API key */
      {{{0}, 0, ""},
       {{0}, 0, ""},
       {{0}, 0, ""},
       {{0}, 0, ""},
      }
      
    , hostName{DefaultHostName}
    , telnetEnabled(true)
    , telnetPass{"h4ckm3"}

    , debug(false)
  { }
};

extern FlashSettings<FlashData> flashSettings;

#endif /* _EW_IG_FLASH_SETTINGS_H_ */

