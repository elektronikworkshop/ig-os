#include "flash.h"
#include <EEPROM.h>

FlashDataSet flashDataSet =
{
  sizeof(FlashDataSet),
  0xAAAA,
  "",  // saved SSID (place your SSID and password here)
  "",  // router password

  {{ 30,  /* pump seconds                       */
    180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)! */
    230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
      5,  /* soak minutes                       */
      0}, /* reservoir threshold                */
   {0},
   {0},
   {0}
  },// WaterCircuit::Settings waterCircuitSettings[NumWaterCircuits];

  {{6, 0},
   {8, 0},
   {SchedulerTime::InvalidHour, 0},
   {SchedulerTime::InvalidHour, 0},
   {SchedulerTime::InvalidHour, 0},
   {20, 0},
   {22, 0}
  },

  /* interval (minutes), channel ID, write API key */
  {{{0}, 0, ""},
   {{0}, 0, ""},
   {{0}, 0, ""},
   {{0}, 0, ""}
  },

  /* hostName */
  DefaultHostName,
  
  {0} /* reserved */

#if 0
  {820, 850},   // 79.0, 82.0 default cool temps
  {730, 750},   // default heat temps
  {30, 17},     // cycleThresh (cool 3.0, heat 1.7)
  0,            // Mode
  30,           // heatThresh (under 30F is gas)
  60*2,         // 2 mins minimum for a cycle
  60*25,        // 25 minutes maximun for a cycle
  60*5,         // idleMin 5 minutes minimum between cycles
  0,            // filterMinutes
  {60, 120},    // fanPostDelay {cool, HP}
  {60, 60},     // fanPreTime {cool, HP}
  60*10,        // 10 mins default for override
  0,            // heatMode
  -5,           // timeZone
  0,            // temp reading offset adjust
  0,            // humidMode
  {450, 550},   // rhLevel 45.0%, 55%
  {40, -40},    // awayDelta cool, heat
  9*60,         // awayTime (minutes)
  30*60,        // fanCycleTime 30 mins
  192 | (168<<8) | (105<<24), // hostIp 192.168.0.105
  85,           // host port
  "41042",      // zipCode
  "password",  // password for controlling thermostat
  false,        // bLock
  false,        // bNotLocal
  14543,        // price per KWH in cents / 10000 (0.145)
    700,        // nat gas cost per cubic foot in cents / 100 (0.70)
  46,           // forecast range for in mapping to out mix/max (5, but 3 can be better)
  46,           // forecast range for display (5 of 7 day max)
#endif
};

FlashMemory flashMemory;

void
FlashMemory::begin()
{
  EEPROM.begin(512);

  uint8_t data[sizeof(FlashDataSet)];
  uint16_t *pwTemp = (uint16_t *)data;

  for(int i = 0; i < sizeof(FlashDataSet); i++) {
    data[i] = EEPROM.read(i);
  }

  Serial.print("size: ");
  Serial.print(pwTemp[0]);
  Serial.print(" ");
  Serial.println(sizeof(FlashDataSet));
  
  if(pwTemp[0] != sizeof(FlashDataSet)) {
    return; // revert to defaults if struct size changes
  }
  
  uint16_t sum = pwTemp[1];
  pwTemp[1] = 0;  // reset sum in data before computing new checksum
  pwTemp[1] = fletcher16(data, sizeof(FlashDataSet));

  Serial.print("sum: ");
  Serial.print(sum);
  Serial.print(" ");
  Serial.println(pwTemp[1]);

  if(pwTemp[1] != sum) {
    return; // revert to defaults if sum fails
  }
  
  memcpy(&flashDataSet, data, sizeof(FlashDataSet) );
}

void FlashMemory::update() // write the settings if changed
{
  uint16_t old_sum = flashDataSet.sum;
  flashDataSet.sum = 0; // reset sum in data before computing new checksum
  flashDataSet.sum = fletcher16((uint8_t*)&flashDataSet, sizeof(FlashDataSet));

  if(old_sum == flashDataSet.sum) {
    /* Nothing has changed -- exit */
    return;
  }

  uint8_t *pData = (uint8_t *)&flashDataSet;
  for(int i = 0; i < sizeof(FlashDataSet); i++)
  {
    EEPROM.write(i, pData[i]);
  }
  EEPROM.commit();
}

uint16_t FlashMemory::fletcher16( uint8_t* data, int count)
{
   uint16_t sum1 = 0;
   uint16_t sum2 = 0;

   for( int index = 0; index < count; ++index )
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }

   return (sum2 << 8) | sum1;
}


