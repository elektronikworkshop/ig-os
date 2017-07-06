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
  Serial << WelcomeMessage("serial");

  flashMemory.begin();
  
  network.begin();
  timeClient.begin();

  spi.begin();
  adc.begin();

  for (WaterCircuit** w = circuits; *w; w++) {
    (*w)->begin();
  }

  loggerBegin();
  telnetBegin();

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop()
{
  uartCli.readSerial();

  network.run();
  telnetRun();

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
      bool trigger = wateringDue() or uartCli.isWateringTriggered();

      /* Add trigger from telnet clients */
      for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
        trigger = trigger or telnetClis[i].isWateringTriggered();
      }
      
      for (WaterCircuit** c = circuits; *c; c++) {
        if ((*c)->isEnabled()) {
          if (trigger) {
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

