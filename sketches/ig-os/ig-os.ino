/*
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://developers.google.com/chart/
 * https://diyprojects.io/esp8266-web-server-part-5-add-google-charts-gauges-and-charts/#Add_Google_Charts_to_a_Web_Interface_ESP8266
 * https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
 * 
 */

#include "water_circuit.h"
#include "cli.h"
#include "adc.h"
#include "flash.h"

FlashMemory flashMemory;

const char* WiFiSSID = "TokyoElectricPowerCompany";
const char* WiFiPSK  = "g3tR1d0ff1t";

void setup()
{
  Serial.begin(115200);
  Serial.println("");

  connectWiFi(WiFiSSID, WiFiPSK);

  timeClient.begin();

  cliInit();

  for (WaterCircuit** w = circuits; *w; w++) {
    (*w)->begin();
  }
//  thingSpeakLogger.begin();
  spi.begin();
  adc.begin();

  flashMemory.load();

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop()
{
  cliRun();
  timeClient.update();
  
  switch (systemMode.getMode()) {
    
    case SystemMode::Off:
      break;
    
    case SystemMode::Auto:
    {
      bool trigger = wateringDue();
      for (WaterCircuit** c = circuits; *c; c++) {
        if ((trigger or cliTrigger) and (*c)->isEnabled()) {
          (*c)->trigger();
        }
        (*c)->run();
      }
      
      cliTrigger = false;

//      thingSpeakLogger.run();
      
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

void connectWiFi(const char* ssid, const char* pass)
{
  byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  while (true) {

    WiFi.begin(ssid, pass);
  
    const int MillisecondsToTry = 5000;
    const int IntervalMs = 200;

    for (int i = 0; i < MillisecondsToTry/IntervalMs; i++) {
    
      if (WiFi.status() == WL_CONNECTED) {
        Serial
          << "connected\n"
          << "signal strength: " << WiFi.RSSI() << " dB\n"
          << "IP:              " << WiFi.localIP() << "\n"
          ;
        return;
      }
      
      delay(IntervalMs);
      
      Serial << ".";
    }
    Serial
      << "failed to connect to " << ssid << " after " << MillisecondsToTry/1000 << "seconds\n";
    WiFi.disconnect();
  }
}

