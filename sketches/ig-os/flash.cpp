#include "flash.h"
#include <EEPROM.h>

FlashDataSet flashDataSet =
{
  sizeof(FlashDataSet),
  0xAAAA,
  
  "",  // router SSID
  "",  // router password

  /* WaterCircuit::Settings waterCircuitSettings[NumWaterCircuits]; */
  {
   /* circuit 1 */
   { 30,  /* pump seconds                            */
    180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
    230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
      5,  /* soak minutes                            */
      0,  /* reservoir threshold                     */
     10}, /* maximum iterations                      */
   /* circuit 2 */
   {  0,  /* pump seconds                            */
    180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
    230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
      5,  /* soak minutes                            */
      0,  /* reservoir threshold                     */
     10}, /* maximum iterations                      */
   /* circuit 3 */
   {  0,  /* pump seconds                            */
    180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
    230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
      5,  /* soak minutes                            */
      0,  /* reservoir threshold                     */
     10}, /* maximum iterations                      */
   /* circuit 4 */
   {  0,  /* pump seconds                            */
    180,  /* dry 0.6 * 255 (150) -- 0.7 * 255 (180)  */
    230,  /* wet 0.8 * 255 (200) -- 0.9 * 255 (230)  */
      5,  /* soak minutes                            */
      0,  /* reservoir threshold                     */
     10}, /* maximum iterations                      */
  },

  {{6, 0},
   {8, 0},
   {SchedulerTime::InvalidHour, 0},
   {SchedulerTime::InvalidHour, 0},
   {SchedulerTime::InvalidHour, 0},
   {SchedulerTime::InvalidHour, 0},
   {20, 0},
   {22, 0},
  },

  /* interval (minutes), channel ID, write API key */
  {{{0}, 0, ""},
   {{0}, 0, ""},
   {{0}, 0, ""},
   {{0}, 0, ""},
  },

  /* hostName */
  DefaultHostName,

  /* telnetEnabled */
  true,

  /* telnetPass */
  "h4ckm3",
  
//  {0} /* reserved */
};

FlashMemoryManager flashMemory;

void
FlashMemoryManager::begin()
{

  /* TODO: C++ verify if sizeof struct is not bigger than the flash partition */
  
  EEPROM.begin(512);

  uint8_t data[sizeof(FlashDataSet)];
  uint16_t *pwTemp = (uint16_t *)data;

  for (unsigned int i = 0; i < sizeof(FlashDataSet); i++) {
    data[i] = EEPROM.read(i);
  }

  Debug << "flash: size: " << pwTemp[0] << " (stored), " << sizeof(FlashDataSet) << " (expected)\n";
  
  if (pwTemp[0] != sizeof(FlashDataSet)) {
    Debug << "flash: reverting to defaults\n";
    return;
  }
  
  uint16_t sum = pwTemp[1];
  pwTemp[1] = 0;  // reset sum in data before computing new checksum
  pwTemp[1] = fletcher16(data, sizeof(FlashDataSet));

  Debug << "flash: checksum: " << sum << " (stored), " << pwTemp[1] << " (expected)\n";

  if(pwTemp[1] != sum) {
    Debug << "flash: reverting to defaults\n";
    return;
  }
  
  Debug << "flash: loading settings\n";
  
  memcpy(&flashDataSet, data, sizeof(FlashDataSet) );
}

void FlashMemoryManager::update() // write the settings if changed
{
  uint16_t old_sum = flashDataSet.sum;
  flashDataSet.sum = 0; // reset sum in data before computing new checksum
  flashDataSet.sum = fletcher16((uint8_t*)&flashDataSet, sizeof(FlashDataSet));

  if (old_sum == flashDataSet.sum) {
    /* Nothing has changed -- exit */
    return;
  }

  uint8_t *pData = (uint8_t *)&flashDataSet;
  for (unsigned int i = 0; i < sizeof(FlashDataSet); i++) {
    EEPROM.write(i, pData[i]);
  }
  EEPROM.commit();
}

uint16_t FlashMemoryManager::fletcher16( uint8_t* data, int count)
{
   uint16_t sum1 = 0;
   uint16_t sum2 = 0;

   for ( int index = 0; index < count; ++index ) {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }

   return (sum2 << 8) | sum1;
}

