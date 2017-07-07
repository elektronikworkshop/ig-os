/** Required libraries:
 *  
 * Github:
 *   ESPAsyncTCP         https://github.com/me-no-dev/ESPAsyncTCP
 *   ESPAsyncWebServer   https://github.com/me-no-dev/ESPAsyncWebServer

 * Library manager:
 *   OneWire
 * 
 * 
 * Notes for HUF/EW
 * 
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://developers.google.com/chart/
 * https://diyprojects.io/esp8266-web-server-part-5-add-google-charts-gauges-and-charts/#Add_Google_Charts_to_a_Web_Interface_ESP8266
 * https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
 * 
 * https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/eeMem.h
 *  
 * consider for later incorporation:
 * https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/WiFiManager.h
 * 
 */

#include "cli.h"
#include "adc.h"
#include "spi.h"
#include "flash.h"
#include "network.h"
#include "webserver.h"

LogProxy<MaxTelnetClients> Log;
LogProxy<MaxTelnetClients> Debug;//(false);
ErrorLogProxy Error;

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial << "build: " << __DATE__ << " " << __TIME__ << "\n";

  flashMemory.begin();
  
  network.begin();
  timeClient.begin();
  webserver.begin();

  spi.begin();
  adc.begin();

  for (WaterCircuit** w = circuits; *w; w++) {
    (*w)->begin();
  }

  loggerBegin();

  pinMode(LED_BUILTIN, OUTPUT);

  // perhaps we print this on network connect/connect-failure? would be more elegant
  Serial << WelcomeMessage("serial");
}


void loop()
{
  uartCli.readSerial();

  network.run();
  telnetRun();

  timeClient.update();
  webserver.run();
  spi.run();
  adc.run();

  switch (systemMode.getMode()) {
    
    case SystemMode::Off:
    case SystemMode::Manual:
    case SystemMode::Invalid:
      break;
    
    case SystemMode::Auto:
    {
      bool trigger = wateringDue() or uartCli.isWateringTriggered();

      /* Add trigger from telnet clients */
      for (unsigned int i = 0; i < MaxTelnetClients; i++) {
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
  }

  unsigned long t = millis();
  if (t & 0x0000200UL) {
    static unsigned int led = LOW;
    led = led == HIGH ? LOW : HIGH;
    digitalWrite(LED_BUILTIN, led);
  }  
}

