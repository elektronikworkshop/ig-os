#ifndef EW_IG_NETWORK_H
#define EW_IG_NETWORK_H

#include <Arduino.h>

/* TODO: make use of wifi callbacks!
 * mDisconnectHandler = WiFi.onStationModeDisconnected(&onDisconnected);
 *
 * void onDisconnected(const WiFiEventStationModeDisconnected& event)
 *  {
 *      bla bla
 *  }
 */
 
class Network
{
public:
  typedef enum
  {
    StateDisconnected,
    StateConnecting,
    StateConnected,
  } State;

  Network();
  
  void begin();
  void run();

  void connect();
  void disconnect();
  
  State getState() const { return m_state; }

  bool isConnected() const { return m_state == StateConnected; }

  static void printVisibleNetworks(Stream& stream = Serial);
private:
  void startMdns();

  State m_state;
  unsigned long m_connectStartMs;
  unsigned long m_connectTimeoutMs;
};

extern Network network;

#endif /* EW_IG_NETWORK_H */

