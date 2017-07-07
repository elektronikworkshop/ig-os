 
#ifndef EW_IG_WEBSERVER
#define EW_IG_WEBSERVER

/* https://github.com/me-no-dev/ESPAsyncWebServer
 */
#include <ESPAsyncWebServer.h>

class Webserver
{
public:
  Webserver(uint16_t port,
            const char* eventPath = "/events",
            const char* webSocketPath = "/ws");
  void begin();
  void run();
private:
  void onEvents(AsyncEventSourceClient *client);
  void onWsEvent(AsyncWebSocket * server,
                 AsyncWebSocketClient * client,
                 AwsEventType type,
                 void * arg,
                 uint8_t *data,
                 size_t len);

  AsyncWebServer m_server;
  AsyncEventSource m_events;
  AsyncWebSocket m_websocket;
};

extern Webserver webserver;

#endif

