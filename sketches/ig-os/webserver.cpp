#include "webserver.h"


Webserver webserver(80, "/events", "/ws");


Webserver::Webserver(uint16_t port,
                     const char* eventPath,
                     const char* webSocketPath)
  : m_server(port)
  , m_events(eventPath)
  , m_websocket(webSocketPath)
{
  
}

// Handle event stream
void
Webserver::onEvents(AsyncEventSourceClient *client)
{
  static bool rebooted = true;
  if(rebooted) {
    rebooted = false;
    m_events.send("Restarted", "alert");
  }
//  m_events.send(dataJson().c_str(), "state");
}

void
Webserver::onWsEvent(AsyncWebSocket* server,
                     AsyncWebSocketClient* client,
                     AwsEventType type,
                     void* arg,
                     uint8_t* data,
                     size_t len)
{  //Handle WebSocket event
  static bool rebooted = true;
  switch(type)
  {
    case WS_EVT_CONNECT:      //client connected
      if(rebooted)
      {
        rebooted = false;
        client->printf("alert;Restarted");
      }
/*
      client->printf("settings;%s", hvac.settingsJson().c_str()); // update everything on start
      client->printf("state;%s", dataJson().c_str());
*/
      client->ping();
      break;
    case WS_EVT_DISCONNECT:    //client disconnected
    case WS_EVT_ERROR:    //error was received from the other end
/*
      if(hvac.m_bRemoteStream && client->id() == WsRemoteID) // stop remote
      {
         hvac.m_bRemoteStream = false;
         hvac.m_bLocalTempDisplay = !hvac.m_bRemoteStream; // switch to showing local/remote color
         hvac.m_notif = Note_RemoteOff;
      }
*/
      break;
    case WS_EVT_PONG:    //pong message was received (in response to a ping request maybe)
      break;
    case WS_EVT_DATA:  //data packet
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len){
        //the whole message is in a single frame and we got all of it's data
        if(info->opcode == WS_TEXT){
          data[len] = 0;
          char *pCmd = strtok((char *)data, ";"); // assume format is "name;{json:x}"
          char *pData = strtok(NULL, "");
          if(pCmd == NULL || pData == NULL) {
            break;
          }
//          bKeyGood = false; // for callback (all commands need a key)
//          WsClientID = client->id();
//          remoteParse.process(pCmd, pData);
        }
      }
      break;
  }
}

#include "system.h"

void
Webserver::begin()
{

  m_events.onConnect(std::bind(&Webserver::onEvents, this, std::placeholders::_1));
  m_server.addHandler(&m_events);

  // attach AsyncWebSocket
  m_websocket.onEvent(std::bind(&Webserver::onWsEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  m_server.addHandler(&m_websocket);

  m_server.on("/",
              HTTP_GET | HTTP_POST,
              [](AsyncWebServerRequest *request)
  {
    /*
    if(wifi.isCfg()){
      request->send(200, "text/html", wifi.page());
    }
    */
    request->send(200, "text/plain", "hello world\n");
  });

  m_server.on("/circuit/info",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  m_server.begin();


// This should be in main ino or in network
#ifdef OTA_ENABLE
  ArduinoOTA.begin();
#endif


#if 0

#ifdef USE_SPIFFS
  SPIFFS.begin();
  server.addHandler(new SPIFFSEditor("admin", ee.password));
#endif

  // attach AsyncEventSource
  events.onConnect(onEvents);
  server.addHandler(&events);

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on ( "/", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    if(wifi.isCfg())
      request->send( 200, "text/html", wifi.page() );
  });
  server.on ( "/iot", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    parseParams(request);
#ifdef USE_SPIFFS
    request->send(SPIFFS, "/index.html");
#else
    request->send_P(200, "text/html", page1);
#endif
  });

  server.on ( "/s", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    parseParams(request);
    request->send ( 200, "text/html", "OK" );
  });

  server.on ( "/json", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    String s = hvac.settingsJson();
    request->send ( 200, "text/json", s);
  });

  server.on ( "/settings", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    parseParams(request);
#ifdef USE_SPIFFS
    request->send(SPIFFS, "/settings.html");
#else
    request->send_P(200, "text/html", page2);
#endif
  });
  server.on ( "/chart.html", HTTP_GET, [](AsyncWebServerRequest *request){
    parseParams(request);
#ifdef USE_SPIFFS
    request->send(SPIFFS, "/chart.html");
#else
    request->send_P(200, "text/html", chart);
#endif
  });
  server.on ( "/data", HTTP_GET, dataPage);  // history for chart
  server.on ( "/forecast", HTTP_GET, fcPage); // forecast data for remote unit

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(404);
  });
  server.onNotFound([](AsyncWebServerRequest *request){
//    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.begin();

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", serverPort);

  remoteParse.addList(jsonList1);
  remoteParse.addList(cmdList);
  remoteParse.addList(jsonList3);

#ifdef OTA_ENABLE
  ArduinoOTA.begin();
#endif

  fc_client.onConnect([](void* obj, AsyncClient* c) { fc_onConnect(c); });
  fc_client.onData([](void* obj, AsyncClient* c, void* data, size_t len){fc_onData(c, static_cast<char*>(data), len); });
  fc_client.onDisconnect([](void* obj, AsyncClient* c) { fc_onDisconnect(c); });
fc_client.onTimeout([](void* obj, AsyncClient* c, uint32_t time) { fc_onTimeout(c, time); });
#endif
}

void
Webserver::run()
{
  
}

