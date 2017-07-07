 
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

#if 0
/** std::mutex isn't currently available, so we have to cook something on our own. */
// #include <mutex>


#include <c_types.h>
/* Stolen from
 *  https://github.com/raburton/esp8266/tree/master/mutex
 */
class Mutex
{
private:
  typedef int32_t mutex_t;
  
  mutex_t m_mutex;

public:
  Mutex()
  {
    m_mutex = 1;
  }

  // try to get a mutex
  // returns true if successful, false if mutex not free
  // as the esp8266 doesn't support the atomic S32C1I instruction
  // we have to make the code uninterruptable to produce the
  // same overall effect
  bool acquire()
  {
    mutex_t *mutex = &m_mutex;
    
    int iOld = 1, iNew = 0;
  
    asm volatile (
      "rsil a15, 1\n"    // read and set interrupt level to 1
      "l32i %0, %1, 0\n" // load value of mutex
      "bne %0, %2, 1f\n" // compare with iOld, branch if not equal
      "s32i %3, %1, 0\n" // store iNew in mutex
      "1:\n"             // branch target
      "wsr.ps a15\n"     // restore program state
      "rsync\n"
      : "=&r" (iOld)
      : "r" (mutex), "r" (iOld), "r" (iNew)
      : "a15", "memory"
    );
  
    return (bool)iOld;
  }

  // release a mutex
  void release(mutex_t *mutex)
  {
    m_mutex = 1;
  }
};

#endif

#endif

