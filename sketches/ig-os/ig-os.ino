/** Required libraries:
 *  
 * Github:
 *   ESPAsyncTCP         https://github.com/me-no-dev/ESPAsyncTCP
 *   ESPAsyncWebServer   https://github.com/me-no-dev/ESPAsyncWebServer

 * Library manager:
 *   OneWire
 *   DS1307RTC (optional for I2C real time clock)
 * 
 * 
 * Notes for HUF/EW
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://developers.google.com/chart/
 * https://diyprojects.io/esp8266-web-server-part-5-add-google-charts-gauges-and-charts/#Add_Google_Charts_to_a_Web_Interface_ESP8266
 * https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
 * 
 * // nice flash management, configuration AP
 * https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/eeMem.h
 *  
 * http://www.makeuseof.com/tag/how-and-why-to-add-a-real-time-clock-to-arduino/
 *  
 * OTA
 * http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html
 * https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
 *  
 * consider for later incorporation:
 * https://github.com/CuriousTech/ESP8266-HVAC/blob/master/Arduino/WiFiManager.h
 * 
 */

#include "cli.h"
#include "spi.h"
#include "flash.h"
#include "network.h"
#include "webserver.h"

LogProxy<MaxTelnetClients> Log;
LogProxy<MaxTelnetClients> Debug;//(false);
ErrorLogProxy Error;

//Webserver webserver(80);

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial << "build: " << __DATE__ << " " << __TIME__ << "\n";

  Wire.begin();

  flashMemory.begin();
  
  network.begin();
  timeClient.begin();
//  webserver.begin();

  spi.begin();
  adc.begin();

  for (WaterCircuit** w = circuits; *w; w++) {
    (*w)->begin();
  }

  loggerBegin();

  pinMode(LED_BUILTIN, OUTPUT);

  Debug << "number of registered commands: " << uartCli.getNumCommandsRegistered(0) << "\n";
}


void loop()
{
  uartCli.run();

  network.run();
  telnetServer.run();

  timeClient.update();
//  webserver.run();
  spi.run();
  adc.run();

  switch (systemMode.getMode()) {
    
    case SystemMode::Off:
    case SystemMode::Manual:
    case SystemMode::Invalid:
      break;
    
    case SystemMode::Auto:
    {
      bool trigger = false;
      if (wateringDue()) {
        trigger = true;
        Log << "watering triggered by scheduler at " << timeClient.getFormattedTime() << "\n";
      }
      
      if (uartCli.isWateringTriggered()) {
        trigger = true;
        Log << "watering triggered by CLI at " << timeClient.getFormattedTime() << "\n";
      }

      /* Add trigger from telnet clients */
      bool tnt = false;
      for (unsigned int i = 0; i < MaxTelnetClients; i++) {
        tnt = tnt or telnetClis[i].isWateringTriggered();
      }
      if (tnt) {
        trigger = true;
        Log << "watering triggered by telnet CLI at " << timeClient.getFormattedTime() << "\n";
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

