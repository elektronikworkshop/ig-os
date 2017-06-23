

#include "water_circuit.h"
#include "cli.h"

const char* WiFiSSID = "TokyoElectricPowerCompany";
const char* WiFiPSK  = "g3tR1d0ff1t";

/*
const int RESERVOIR_POWER_PIN = D1;
const int RESERVOIR_SENSE_PIN = D2;
*/

void setup()
{
  Serial.begin(115200);
  Serial.println("");

  connectWiFi(WiFiSSID, WiFiPSK);

  systemTimeInit();

  cliInit();

  circuit.begin();
  thingSpeakLogger.begin();

  pinMode(LED_BUILTIN, OUTPUT);

/*
  pinMode(RESERVOIR_POWER_PIN, OUTPUT);
  digitalWrite(RESERVOIR_POWER_PIN, LOW);
  pinMode(RESERVOIR_SENSE_PIN, INPUT);
  */
}


void loop()
{
  cliRun();
  systemTimeRun();
  
  switch (systemMode.getMode()) {
    
    case SystemMode::Off:
      break;
    
    case SystemMode::Auto:
    {
      bool trigger = wateringDue();
      for (WaterCircuit** c = circuits; *c; c++) {
        if (trigger || cliTrigger) {
          (*c)->trigger();
        }
        (*c)->run();
      }
      
      cliTrigger = false;

      thingSpeakLogger.run();
      
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

/*
  digitalWrite(RESERVOIR_POWER_PIN, HIGH);
  delay(500);
  int r = digitalRead(RESERVOIR_SENSE_PIN);
  Serial << "reservoir: " << r << "\n";
  Serial << "reservoir: " << r << "\n";
  digitalWrite(RESERVOIR_POWER_PIN, LOW);
  */
  
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
      << "failed to connect to " << ssid << "\n";
    WiFi.disconnect();

#if 0

    {
      SerialCommand sc;
      bool entered = false;

      auto f = [&](const char* _ssid) {
        ssid = _ssid;
        entered = true;
      };
      
      void (decltype(f)::*ptr)(const char*)const = &decltype(f)::operator();
      
      sc.setDefaultHandler(std::bind(ptr, f));
      while (not entered) {
        sc.readSerial();
        delay(200);
      }
    }
    /*
    {    
      SerialCommand sc;
      bool entered = false;
      sc.setDefaultHandler([&](const char* _pass){
        pass = _pass;
        entered = true;
      });
      while (not entered) {
        sc.readSerial();
        delay(200);
      }
    }
    */
#endif
  }
}

