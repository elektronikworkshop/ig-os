#include "network.h"
#include "flash.h"

#include <ESP8266mDNS.h>

Network network;

Network::Network()
  : m_state(StateDisconnected)
  , m_connectStartMs(0)
  , m_connectTimeoutMs(10000)
{ }

void
Network::begin()
{
  connect();
}

void
Network::startMdns()
{
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network

  const char* hostName = flashDataSet.hostName;
  if (strlen(hostName) == 0) {
    Serial << "host name with length zero detected. defaulting to \"" << DefaultHostName << "\"\n";
    hostName = DefaultHostName;
  }
  if (!MDNS.begin(hostName)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    MDNS.addService("telnet", "tcp", 23);
    Serial << "published host name: " << hostName << "\n";
  }
}

void
Network::run()
{
  // TODO: check if connection lost and adjust state and call notifier/event system
  // TODO: if connection lost try to reconnect every now and then
  
  switch (m_state) {
    case StateDisconnected:
      break;
    case StateConnecting:
      if (WiFi.status() == WL_CONNECTED) {
        Serial
          << "wifi connected to " << flashDataSet.wifiSsid << "\n"
          << "signal strength: " << WiFi.RSSI() << " dB\n"
          << "IP:              " << WiFi.localIP() << "\n";
        m_state = StateConnected;

        startMdns();
        
      } else if (millis() - m_connectStartMs > m_connectTimeoutMs) {
        // Stop any pending request
        WiFi.disconnect();
        m_state = StateDisconnected;
        Serial << "wifi failed to connect to SSID \"" << flashDataSet.wifiSsid << "\" -- timeout\n";
      }
      break;
    case StateConnected:
      if (WiFi.status() != WL_CONNECTED) {
        Serial << "wifi connection lost\n";
        // event...
        m_state = StateDisconnected;
      }
      break;
  }
}

void
Network::connect()
{
  if (m_state != StateDisconnected) {
    return;
  }

  if (strlen(flashDataSet.wifiSsid) == 0 or strlen(flashDataSet.wifiPass) == 0) {
    Serial << "wifi SSID (\"" << flashDataSet.wifiSsid << "\") or password (\"" << flashDataSet.wifiPass << "\") not set -- can not connect to network, please set up your SSID and password\n";

    printVisibleNetworks();
    
    return;
  }

  m_connectStartMs = millis();

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);
  WiFi.begin(flashDataSet.wifiSsid, flashDataSet.wifiPass);

  m_state = StateConnecting;
}

void
Network::disconnect()
{
  if (m_state == StateDisconnected) {
    return;
  }

  WiFi.disconnect();
  m_state = StateDisconnected;
}

void
Network::printVisibleNetworks(Stream& stream)
{
  byte n = WiFi.scanNetworks();
  if (n) {
    stream << "visible network SSIDs:\n";     
    for (int i = 0; i < n; i++) {
      stream << "  " << WiFi.SSID(i) << " (" << WiFi.RSSI(i) << " dB)\n";
    }
  }
}

