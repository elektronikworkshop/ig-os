/*
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://developers.google.com/chart/
 * https://diyprojects.io/esp8266-web-server-part-5-add-google-charts-gauges-and-charts/#Add_Google_Charts_to_a_Web_Interface_ESP8266
 * https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
 * 
 */

#include "cli.h"
#include "adc.h"
#include "spi.h"
#include "flash.h"
#include "network.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("");

  flashMemory.begin();
  
  network.begin();
  timeClient.begin();

  cliInit();

  spi.begin();
  adc.begin();

  for (WaterCircuit** w = circuits; *w; w++) {
    (*w)->begin();
  }

  initSettings();

  loggerBegin();

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop()
{
  cliRun();

  network.run();

  if (network.isConnected()) {
    timeClient.update();
  }

  spi.run();
  adc.run();

  switch (systemMode.getMode()) {
    
    case SystemMode::Off:
      break;
    
    case SystemMode::Auto:
    {
      bool trigger = wateringDue() or cliTrigger;
      cliTrigger = false;
      for (WaterCircuit** c = circuits; *c; c++) {
        if ((*c)->isEnabled()) {
          if (trigger or cliTrigger) {
            (*c)->trigger();
          }
          (*c)->run();
        }
      }

      loggerRun();

      break;
    }
    
    case SystemMode::Manual:
      break;
  }

  unsigned long t = millis();
  if (t & 0x0000200UL) {
    static unsigned int led = LOW;
    led = led == HIGH ? LOW : HIGH;
    digitalWrite(LED_BUILTIN, led);
  }  
}


void initSettings()
{
  /* load watering cuircuit settings from flash */
  for (int i = 0; i < NumWaterCircuits; i++) {
      WaterCircuit* w = circuits[i];
      w->setSettings(flashDataSet.waterCircuitSettings[i]);
  }

  /* load scheduler settings from flash */
  for (int i = 0; i < NumSchedulerTimes; i++) {
    *schedulerTimes[i] = SchedulerTime(flashDataSet.schedulerTimes[i]);
  }
}

