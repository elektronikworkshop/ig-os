
#include <ESP8266WiFi.h>

/* https://github.com/kroimon/Arduino-SerialCommand
 * https://github.com/kroimon/Arduino-SerialCommand/blob/master/examples/SerialCommandExample/SerialCommandExample.pde
 * https://playground.arduino.cc/Main/StreamingOutput
 */
#include "StreamCmd.h"
#include "system.h"
#include "log.h"
#include "spi.h"
#include "adc.h"
#include "network.h"
#include "flash.h"


#include <climits>
#include <stdarg.h>

class Cli
  : public StreamCmd
{
private:
  bool m_cliTrigger;
  
public:
  Cli(Stream& stream,
      char eolChar = '\n')
    : StreamCmd(stream, eolChar)
    , m_cliTrigger(false)
  {
    addCommand("help",   static_cast<CommandCallback>(&Cli::cmdHelp));
    addCommand("time",   static_cast<CommandCallback>(&Cli::cmdTime));
    addCommand("mode",   static_cast<CommandCallback>(&Cli::cmdMode));
    
    addCommand("c.trig",  static_cast<CommandCallback>(&Cli::cmdCircuitTrigger));
    addCommand("c.read",  static_cast<CommandCallback>(&Cli::cmdCircuitRead));
    addCommand("c.res",   static_cast<CommandCallback>(&Cli::cmdCircuitReservoir));
    addCommand("c.pump",  static_cast<CommandCallback>(&Cli::cmdCircuitPump));
    addCommand("c.valve", static_cast<CommandCallback>(&Cli::cmdCircuitValve));
    addCommand("c.info",  static_cast<CommandCallback>(&Cli::cmdCircuitInfo));
    addCommand("c.set",   static_cast<CommandCallback>(&Cli::cmdCircuitSet));
  
    addCommand("l.trig",  static_cast<CommandCallback>(&Cli::cmdLogTrigger));
    addCommand("l.info",  static_cast<CommandCallback>(&Cli::cmdLogInfo));
    addCommand("l.set",   static_cast<CommandCallback>(&Cli::cmdLogSet));
  
    addCommand("s.info",  static_cast<CommandCallback>(&Cli::cmdSchedulerInfo));
    addCommand("s.set",   static_cast<CommandCallback>(&Cli::cmdSchedulerSet));
  
    addCommand("n.rssi",    static_cast<CommandCallback>(&Cli::cmdNetworkRssi));
    addCommand("n.list",    static_cast<CommandCallback>(&Cli::cmdNetworkList));
    addCommand("n.ssid",    static_cast<CommandCallback>(&Cli::cmdNetworkSsid));
    addCommand("n.pass",    static_cast<CommandCallback>(&Cli::cmdNetworkPass));
    addCommand("n.connect", static_cast<CommandCallback>(&Cli::cmdNetworkConnect));
  
    setDefaultHandler(static_cast<DefaultCallback>(&Cli::cmdInvalid));
  }
  bool isWateringTriggered()
  {
    bool ret = m_cliTrigger;
    m_cliTrigger = false;
    return ret;
  }
  
protected:  
/* helper functions */

  void prtTwoDigit(int num)
  {
    if (num < 10) {
      m_stream << "0";
    }
    m_stream << num;
  }
  
  bool cmdParseInt(int& num, int min, int max)
  {
    char* arg = next();
    if (arg == NULL) {
      return false;
    }
    int _num = atoi(arg);
    if (_num < min or _num > max) {
      return false;
    }
    num = _num;
    return true;
  }
  
  bool cmdParseLong(long& num, long min, long max)
  {
    char* arg = next();
    if (arg == NULL) {
      return false;
    }
    long _num = atol(arg);
    if (_num < min or _num > max) {
      return false;
    }
    num = _num;
    return true;
  }
  
  bool cmdParseFloat(float& num, float min, float max)
  {
    char* arg = next();
    if (arg == NULL) {
      return false;
    }
    float _num = atof(arg);
    if (_num < min or _num > max) {
      return false;
    }
    num = _num;
    return true;
  }
  
  bool cmdParseId(int& id, WaterCircuit*& w)
  {
    if (not cmdParseInt(id, 1, NumWaterCircuits)) {
      return false;
    }
    
    w = circuits[id - 1];
  
    return true;
  }
  
  /* Serial command interface */
  
  virtual void cmdHelp()
  {
    m_stream.print(F(
      "----------------\n"
      "help\n"
      "  print this help\n"
      "time\n"
      "  display current time\n"
      "mode <off, auto, man>\n"
      "  switch the intelligüss mode\n"
      "WATERING CIRCUITS:\n"
      "c.trig [id]\n"
      "  manually trigger watering cycle. if [id] is provided only\n"
      "  circuit with [id] is triggered\n"
      "c.read <id>\n"
      "  read sensor of circuit with ID <id>\n"
      "c.pump <id> <seconds>\n"
      "  run pump of circuit with ID <id> for <seconds> seconds\n"
      "c.info [id]\n"
      "  display settings and state of circuit with ID [id]\n"
      "  if no [id] is provided the info for all circuits is printed\n"
      "c.set <id> <param>\n"
      "  change settings and state of circuit with ID <id>\n"
      "  <param> must be one of:\n"
      "    pump <seconds>\n"
      "      set pump time in seconds (0: watering circuit off)\n"
      "    soak <minutes>\n"
      "      set soak time in minutes\n"
      "    dry <thresh>\n"
      "      set dry threshold, range 0 .. 255\n"
      "    wet <thresh>\n"
      "      set wet threshold, range 0 .. 255\n"
      "    res <thresh>\n"
      "      set reservoir threshold, range 0 .. 255 (0: reservoir off)\n"
      "c.log [id]\n"
      "LOGGER\n"
      "l.info [id]\n"
      "  display logging configuration of circuit with ID [id]\n"
      "  if no [id] is provided the configuration for for all circuits is printed\n"
      "l.trig [id]\n"
      "  trigger transmission of current data to remote logger. if no ID is provided, the data of all circuits is transmitted\n"
      "l.set <id> <param>\n"
      "  configure the logging backend for circuit with ID <id>\n"
      "  <param> must be one of:\n"
      "    interval <minutes>\n"
      "      the logging interval in minutes\n"
      "    chid <id>\n"
      "      the ThingSpeak channel ID (integer number)\n"
      "    key <key>\n"
      "      the ThingSpeak channel write API key (character string)\n"
      "SCHEDULER\n"
      "s.info\n"
      "  print scheduler configuration\n"
      "s.set <index> <time>\n"
      "  configure scheduler whereas\n"
      "    <index> is the entry number between 1 and "  /*<< NumSchedulerTimes << */ "8\n"
      "    <time> is the time formatted \"hh:mm\" or \"off\"\n"
      "RESERVOIR\n"
      "r.read\n"
      "  read reservoir level\n"
      "NETWORK\n"
      "n.rssi\n"
      "  display the current connected network strength (RSSI)\n"
      "n.list\n"
      "  list the visible networks\n"
      "n.ssid <ssid>\n"
      "  set wifi SSID\n"
      "n.pass <password>\n"
      "  set wifi password\n"
      "n.connect\n"
      "  connect to configured wifi network\n"
      "----------------\n"
    ));
  }
  
  void cmdTime()
  {
    m_stream << "current time: " << timeClient.getFormattedTime() << "\n";
  }
  
  void cmdMode()
  {
    char* arg = next();
    if (arg == NULL) {
      m_stream << F("mode argument missing\n");
      return;
    }
    
    SystemMode::Mode newMode = SystemMode::str2Mode(arg);
    if (newMode == SystemMode::Invalid) {
      m_stream << F("invalid mode argument \"") << arg << F("\" -- allowed are: <off, auto, man>\n");
      return;
    }
  
    if (newMode != systemMode.getMode()) {
        for (unsigned int i = 0; i < NumWaterCircuits; i++) {
          circuits[i]->reset();
        }
        m_stream << "system mode switched to \"" << arg << "\"\n";
        systemMode.setMode(newMode);
    }
  }
    
  void cmdCircuitTrigger()
  {
    m_cliTrigger = true;
    m_stream << "watering triggered manually\n";  
  }
  
  void cmdCircuitRead()
  {
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      m_stream << "invalid index\n";
      return;
    }
    
    Sensor& s = w->getSensor();
    
    if (s.getState() != Sensor::StateIdle) {
      m_stream << "sensor is currently active, try again later\n";
      return;
    }
    
    s.enable();
    while (s.getState() != Sensor::StateReady) {
      delay(500);
      s.run();
      spi.run();
      adc.run();
    }
    auto h = s.read();
    s.disable();
  
    m_stream << "humidity of " << id << " is " << h << "/255\n";
  }
  
  // TODO: mostly same code as "read" -> combine somehow
  void cmdCircuitReservoir()
  {
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      m_stream << "invalid index\n";
      return;
    }
    
    Sensor& r = w->getReservoir();
    
    if (r.getState() != Sensor::StateIdle) {
      m_stream << "reservoir sensor is currently busy, try again later\n";
      return;
    }
    
    r.enable();
    while (r.getState() != Sensor::StateReady) {
      delay(500);
      r.run();
      spi.run();
      adc.run();
      m_stream << "state: " << r.getState() << "\n";
    }
    m_stream << "reading adc result... \n";
    auto f = r.read();
    r.disable();
  
    m_stream << "reservoir fill is " << f << "/255\n";
  }
  
  void cmdCircuitPump()
  {
    if (systemMode.getMode() != SystemMode::Manual) {
      m_stream << "you need to be in manual mode to do this\n";
      return;
    }
    
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      m_stream << "invalid index\n";
      return;
    }
  
    float s;
    if (not cmdParseFloat(s, 0, 60 * 10)) {
      m_stream << "manual pump seconds must be between 0.0 and 60.0\n";
      return;
    }
  
    m_stream << "pump enabled " << timeClient.getFormattedTime() << "\n";
  
    auto& p = w->getPump();
    p.enable();
    delay(s * 1000);
    p.disable();
  
    m_stream << "pump disabled " << timeClient.getFormattedTime() << "\n";
  }
  
  void cmdCircuitValve()
  {
      if (systemMode.getMode() != SystemMode::Manual) {
      m_stream << "you need to be in manual mode to do this\n";
      return;
    }
    
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      id = 1;
      for (WaterCircuit** w = circuits; *w; w++, id++) {
        m_stream << "  valve " << id << ((*w)->getValve().isOpen() ? " open" : " closed") << "\n";
      }
      return;
    }
  
    const char* arg = next();
    
    if (not arg) {
      m_stream << "  valve " << id << (w->getValve().isOpen() ? " open" : " closed") << "\n";
      return;
    }
    bool open;
    if (strcmp(arg, "open") == 0) {
      open = true;
    } else if (strcmp(arg, "close") == 0) {
      open = false;
    } else {
      m_stream << "no valve argument supplied. valid arguments are \"open\" and \"close\"\n";
      return;
    }
  
    // TODO: make sure that the valve is not used (currently not supported by valve)
    auto& v = w->getValve();
    if (open) {
      v.open();
      m_stream << "valve " << id << " opened " << timeClient.getFormattedTime() << "\n";
    } else {
      v.close();
      m_stream << "valve " << id << " closed " << timeClient.getFormattedTime() << "\n";
    }
  }
  
  void p(char *fmt, ... )
  {
    char buf[128]; // resulting string limited to 128 chars
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    m_stream.print(buf);
  }
  
  void prtCircuitInfo(WaterCircuit* w, int id)
  {
    m_stream
      <<  "on-board circuit [" << id << "]:"
      << (w->getPumpSeconds() == 0 ?
          " off\n" :  " active\n")    
      <<  "           pump time  "; p("%3u s\n", w->getPumpSeconds());   m_stream
      <<  "          dry thresh  "; p("%3u\n", w->getThreshDry());       m_stream
      <<  "          wet thresh  "; p("%3u\n",  w->getThreshWet());      m_stream
      <<  "           soak time  "; p("%3u m\n", w->getSoakMinutes());   m_stream
      <<  "    reservoir thresh  "; w->getThreshReservoir() == 0 ? p("off\n") : p("%3u\n", w->getThreshReservoir()); m_stream
      <<  "  last read humidity  "; p("%3u\n", w->getHumidity());
  }
  
  void cmdCircuitInfo()
  {
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      id = 1;
      for (WaterCircuit** c = circuits; *c; c++, id++) {
          prtCircuitInfo(*c, id);
          if (id != NumWaterCircuits) {
            m_stream << "\n";
          }
        }
      return;
    }
    prtCircuitInfo(w, id);
  }
  
  void cmdCircuitSet()
  { 
    int id;
    WaterCircuit* w;
    if (not cmdParseId(id, w)) {
      m_stream << "invalid index\n";
      return;
    }
    
    char* arg = next();
    if (arg == NULL) {
      m_stream << "no parameter\n";
      return;
    }
  
    if (strcmp(arg, "pump") == 0) {
      int s;
      if (not cmdParseInt(s, 0, 255)) {
        m_stream << "pump seconds must be between 0 and 255\n";
        return;
      }
      w->setPumpSeconds(s);
      m_stream << "pump time set to " << s << " seconds\n";
    } else if (strcmp(arg, "soak") == 0) {
      int m;
      if (not cmdParseInt(m, 0, 255)) {
        m_stream << "soak minutes must be between 0 and 255\n";
        return;
      }
      w->setSoakMinutes(m);
      m_stream << "soak time set to " << m << " minutes\n";
    } else if (strcmp(arg, "dry") == 0) {
      int t;
      if (not cmdParseInt(t, 0, 255)) {
        m_stream << "dry threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshDry(t);
      m_stream << "dry threshold set to " << t << "\n";
    } else if (strcmp(arg, "wet") == 0) {
      int t;
      if (not cmdParseInt(t, 0, 255)) {
        m_stream << "wet threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshWet(t);
      m_stream << "wet threshold set to " << t << "\n";
    } else if (strcmp(arg, "res") == 0) {
      int t;
      if (not cmdParseInt(t, 0, 255)) {
        m_stream << "reservoir threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshReservoir(t);
      m_stream << "reservoir threshold set to " << t << "\n";
    } else {
      m_stream << "invalid parameter \"" << arg << "\"\n";
      return;
    }
  
    flashMemory.update();
  }
  
  void cmdLogTrigger()
  {
    int id;
  
    if (not cmdParseInt(id, 1, NumWaterCircuits)) {
      for (Logger** l = loggers; *l; l++) {
        (*l)->trigger();
      }
      return;
    }
  
    Logger* l = loggers[id];
    l->trigger();
  }
  
  void printLoggerInfo(int id)
  {
    /* Attention: as soon as we have different loggers we have to make sure that we use the right types! */
    ThingSpeakLogger* tsl = static_cast<ThingSpeakLogger*>(loggers[id - 1]);
    auto& s = tsl->getTslSettings();
    
    m_stream
      << "ThingSpeak Logger [" << id << "]: "
      << (s.m_settings.m_intervalMinutes == 0 or s.m_channelId == 0 ?
          " off\n" :  " active\n")
      << "       interval  " << s.m_settings.m_intervalMinutes << " m\n"
      << "     channel ID  " << s.m_channelId << "\n"
      << "  write API key  \"" << s.m_writeApiKey << "\"\n";
  }
  
  void cmdLogInfo()
  {
    int id;
  
    if (not cmdParseInt(id, 1, NumWaterCircuits)) {
      for (id = 1; id <= NumWaterCircuits; id++) {
        printLoggerInfo(id);
        if (id != NumWaterCircuits) {
          m_stream << "\n";
        }
      }
      return;
    }
  
    printLoggerInfo(id);
  }
  
  
  void cmdLogSet()
  {
    int id;
  
    if (not cmdParseInt(id, 1, NumWaterCircuits)) {
      m_stream << "invalid index\n";
      return;
    }
  
      /* Attention: as soon as we have different loggers we have to make sure that we use the right types! */
    ThingSpeakLogger* tsl = static_cast<ThingSpeakLogger*>(loggers[id - 1]);
  
    char* arg = next();
    if (arg == NULL) {
      m_stream << "no parameter\n";
      return;
    }
  
    if (strcmp(arg, "interval") == 0) {
      int m;
      if (not cmdParseInt(m, 0, 1440)) {
        m_stream << "interval minutes must be between 0 and 1440 (24h)\n";
        return;
      }
      tsl->setIntervalMinutes(m);
      m_stream << "log interval set to " << m << " minutes\n";
    } else if (strcmp(arg, "chid") == 0) {
      long chid;
      if (not cmdParseLong(chid, 0, LONG_MAX)) {
        m_stream << "channel ID must be unsigned\n";
        return;
      }
      tsl->setChannelId(chid);
      m_stream << "channel ID set to " << chid << "\n";
    } else if (strcmp(arg, "key") == 0) {
      char* karg = next();
        if (karg == NULL) {
          tsl->setWriteApiKey("");
          m_stream << "write API key set to \"\"\n";
        } else {
          tsl->setWriteApiKey(karg);
          m_stream << "write API key set to \"" << karg << "\"\n";
        }
    } else {
      m_stream << "invalid parameter \"" << arg << "\"\n";
      return;
    }
  
    flashMemory.update();
  }
  
  void cmdSchedulerInfo()
  {
    m_stream << "scheduler entries:\n";
    
    int i = 1;  
    for (SchedulerTime **t = schedulerTimes; *t; t++, i++) {
      m_stream << "  Entry [" << i << "]: ";
      if ((*t)->isValid()) {
        prtTwoDigit((*t)->getHour());
        m_stream << ":";
        prtTwoDigit((*t)->getMinute());
        m_stream << "\n";
      } else {
        m_stream << "off\n";
      }
    }
  }
  
  void cmdSchedulerSet()
  {
    int index;
    if (not cmdParseInt(index, 1, NumSchedulerTimes)) {
      m_stream << "index must be in the range 1 .. " << NumSchedulerTimes << "\n";
      return;
    }
    
    char* arg = next();
    if (arg == NULL             or
        strcmp(arg, "off") != 0 and
        not (strlen(arg) == 5   and
             isDigit(arg[0])    and
             isDigit(arg[1])    and
             arg[2] == ':'      and
             isDigit(arg[3])    and
             isDigit(arg[4])))
    {
      m_stream << "time must be of format \"hh:mm\" or \"off\"\n";
      return;
    }
  
    if (strcmp(arg, "off") == 0) {
      m_stream << "configuring scheduler entry [" << index << "] to \"off\"\n";
      schedulerTimes[index - 1]->setHour(SchedulerTime::InvalidHour);
      schedulerTimes[index - 1]->setMinute(0);
  
      flashMemory.update();
  
      return;
    }
  
    int h, m;
    if (arg[0] == '0') {
      h = atoi(arg+1);
    } else {
      h = atoi(arg);
    }
    if (arg[3] == '0') {
      m = atoi(arg+4);
    } else {
      m = atoi(arg+3);
    }
  
    if (h < 0 or h > 23) {
      m_stream << "hour must be in the range 0 .. 23\n";
      return;
    }
  
    if (m < 0 or m > 59) {
      m_stream << "minute must be in the range 0 .. 59\n";
      return;
    }
  
    m_stream << "configuring scheduler entry [" << index << "] to ";
    prtTwoDigit(h);
    m_stream << ":";
    prtTwoDigit(m);
    m_stream << "\n";
    
    schedulerTimes[index - 1]->setHour(h);
    schedulerTimes[index - 1]->setMinute(m);
  
    flashMemory.update();
  }
  
  void cmdNetworkRssi()
  {
    m_stream << "current RSSI: " << WiFi.RSSI() << " dB\n";
  }
  
  
  void cmdNetworkSsid()
  {
    const char* arg = next();
  
    if (arg == NULL or strlen(arg) == 0) {
      m_stream << "no wifi SSID argument\n";
      return;
    }
    strncpy(flashDataSet.wifiSsid, arg, MaxSsidNameLen);
    flashMemory.update();
  
    m_stream << "wifi SSID \"" << arg << "\" stored to flash\n";
  }
  
  void cmdNetworkPass()
  {
    const char* arg = next();
  
    if (arg == NULL or strlen(arg) == 0) {
      m_stream << "no wifi password argument\n";
      return;
    }
    strncpy(flashDataSet.wifiPass, arg, MaxSsidPassLen);
    flashMemory.update();
  
    m_stream << "wifi pass \"" << arg << "\" stored to flash\n";
  }
  
  void cmdNetworkConnect()
  {
    network.disconnect();
    network.connect();
  }
  
  void cmdNetworkList()
  {
    Network::printVisibleNetworks(m_stream);
  }
  
  void cmdInvalid(const char *command)
  {
    m_stream
      << "what do you mean by \"" << command << "\"? try the \"help\" command\n";
  }

};

Cli uartCli(Serial);


/*
 * https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiTelnetToSerial/WiFiTelnetToSerial.ino
 */
const unsigned int MAX_SRV_CLIENTS = 1;

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];


class TelnetStreamProxy
  : public Stream
{
private:
  Stream& m_stream;
  uint8_t m_negotiationSequenceIndex;
  bool m_lastWasCarriageReturn;
  
public:
  TelnetStreamProxy(Stream& stream)
    : m_stream(stream)
    , m_negotiationSequenceIndex(0)
    , m_lastWasCarriageReturn(false)
  { }
  virtual size_t write(uint8_t c)
  {
    size_t ret = 0;

    if ((char)c == '\n' and not m_lastWasCarriageReturn) {
      ret += m_stream.write('\r');
    }
    
    m_lastWasCarriageReturn = (char)c == '\r';

    ret += m_stream.write(c);
    return ret;
  }
  virtual int available()
  {
    return m_stream.available();
  }

  virtual int read()
  {
    /* http://mud-dev.wikidot.com/telnet:negotiation
     * https://github.com/lukemalcolm/TelnetServLib/tree/master/TelnetServLib
     */
    
    while (true) {
      
      int d = m_stream.read();
      char c = d;
      
      if (d == -1) {
        return d;
      }
      
      /* echo it back */
//      m_stream.write(d);

      /* strip negotiation sequences */
      if (c == 0xFF and m_negotiationSequenceIndex == 0) {
        m_negotiationSequenceIndex = 1;
        continue;
      }      
      if (m_negotiationSequenceIndex) {
        if (m_negotiationSequenceIndex == 2) {
          m_negotiationSequenceIndex = 0;
        } else {
          m_negotiationSequenceIndex++;
        }
        continue;
      }

      // TODO: strip other unwanted stuff here

      return d;      
    }
  }
  virtual int peek()
  {
    return m_stream.peek();
  }
  virtual void flush()
  {
    m_negotiationSequenceIndex = 0;
    m_lastWasCarriageReturn = false;
    m_stream.flush();
  }
};


class TelnetCli
  : public Cli
{
public:
  TelnetCli(WiFiClient& client)
    : m_client(client)
    , m_proxyStream(client)
    , Cli(m_proxyStream, '\r')
  {
    addCommand("quit",   static_cast<CommandCallback>(&TelnetCli::cmdQuit));
  }
  virtual void cmdHelp()
  {
    Cli::cmdHelp();
    m_proxyStream
      << "quit\n"
      << "  quit telnet session and close connection\n";
  }
private:
  void cmdQuit()
  {
    m_client.stop();
  }
  WiFiClient& m_client;
  TelnetStreamProxy m_proxyStream;
};

TelnetCli telnetClis[MAX_SRV_CLIENTS] = {serverClients[0]};

void telnetBegin()
{
  server.begin();
  server.setNoDelay(true);
}

void telnetRun()
{
  /* check if there are any new clients
   *  
   */
  if (server.hasClient()) {
    for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
      /* find free/disconnected slot
       *  
       */
      if (!serverClients[i] || !serverClients[i].connected()) {
        if(serverClients[i]) {
          serverClients[i].stop();
          // empty buffers and reset state machines
          serverClients[i].flush();
        }
        serverClients[i] = server.available();
        Serial << "New telnet client at slot " << Serial.println(i);
        serverClients[i] << "Welcome to the Intelli-Güss telnet interface!\n";
        continue;
      }
    }
    /* We stop the server client object - if it hasn't been assigned to a slot it will be rejected here.
     *  
     */
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  /* check clients for data */
  for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if (serverClients[i].available()) {
        /* as long as telnet data is avaible we read it and stream it to the CLI */
        while(serverClients[i].available()) {
          telnetClis[i].readSerial();
        }
      }
    }
  }
}

