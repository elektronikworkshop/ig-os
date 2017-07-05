
#include <ESP8266WiFi.h>

/* https://github.com/kroimon/Arduino-SerialCommand
 * https://github.com/kroimon/Arduino-SerialCommand/blob/master/examples/SerialCommandExample/SerialCommandExample.pde
 * https://playground.arduino.cc/Main/StreamingOutput
 */
#include <SerialCommand.h>
#include "system.h"
#include "log.h"
#include "spi.h"
#include "adc.h"
#include "network.h"
#include "flash.h"

SerialCommand sCmd;

/* helper functions */

void prtTwoDigit(int num)
{
  if (num < 10) {
    Serial << "0";
  }
  Serial << num;
}

bool cmdParseInt(int& num, int min, int max)
{
  char* arg = sCmd.next();
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
  char* arg = sCmd.next();
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
  char* arg = sCmd.next();
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

void cmdHelp()
{
  Serial.print(F(
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
  Serial << "current time: " << timeClient.getFormattedTime() << "\n";
}

void cmdMode()
{
  char* arg = sCmd.next();
  if (arg == NULL) {
    Serial.println(F("mode argument missing"));
    return;
  }
  
  SystemMode::Mode newMode = SystemMode::str2Mode(arg);
  if (newMode == SystemMode::Invalid) {
    Serial << F("invalid mode argument \"") << arg << F("\" -- allowed are: <off, auto, man>\n");
    return;
  }

  if (newMode != systemMode.getMode()) {
      for (unsigned int i = 0; i < NumWaterCircuits; i++) {
        circuits[i]->reset();
      }
      Serial << "system mode switched to \"" << arg << "\"\n";
      systemMode.setMode(newMode);
  }
}

/* TODO: we can combine both.
 * NumWaterCircuits triggers all, below single circuits are triggered
 */
bool cliTrigger = false;
int cliTriggerId = -1;

void cmdCircuitTrigger()
{
  cliTrigger = true;
  Serial << "watering triggered manually\n";  
}

void cmdCircuitRead()
{
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    Serial << "invalid index\n";
    return;
  }
  
  Sensor& s = w->getSensor();
  
  if (s.getState() != Sensor::StateIdle) {
    Serial << "sensor is currently active, try again later\n";
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

  Serial << "humidity of " << id << " is " << h << "/255\n";
}

// TODO: mostly same code as "read" -> combine somehow
void cmdCircuitReservoir()
{
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    Serial << "invalid index\n";
    return;
  }
  
  Sensor& r = w->getReservoir();
  
  if (r.getState() != Sensor::StateIdle) {
    Serial << "reservoir sensor is currently busy, try again later\n";
    return;
  }
  
  r.enable();
  while (r.getState() != Sensor::StateReady) {
    delay(500);
    r.run();
    spi.run();
    adc.run();
    Serial<<"state: "<<r.getState()<<"\n";
  }
  Serial << "reading adc result... \n";
  auto f = r.read();
  r.disable();

  Serial << "reservoir fill is " << f << "/255\n";
}

void cmdCircuitPump()
{
  if (systemMode.getMode() != SystemMode::Manual) {
    Serial << "you need to be in manual mode to do this\n";
    return;
  }
  
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    Serial << "invalid index\n";
    return;
  }

  float s;
  if (not cmdParseFloat(s, 0, 60 * 10)) {
    Serial << "manual pump seconds must be between 0.0 and 60.0\n";
    return;
  }

  Serial << "pump enabled " << timeClient.getFormattedTime() << "\n";

  auto& p = w->getPump();
  p.enable();
  delay(s * 1000);
  p.disable();

  Serial << "pump disabled " << timeClient.getFormattedTime() << "\n";
}

void cmdCircuitValve()
{
    if (systemMode.getMode() != SystemMode::Manual) {
    Serial << "you need to be in manual mode to do this\n";
    return;
  }
  
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    id = 1;
    for (WaterCircuit** w = circuits; *w; w++, id++) {
      Serial << "  valve " << id << ((*w)->getValve().isOpen() ? " open" : " closed") << "\n";
    }
    return;
  }

  const char* arg = sCmd.next();
  
  if (not arg) {
    Serial << "  valve " << id << (w->getValve().isOpen() ? " open" : " closed") << "\n";
    return;
  }
  bool open;
  if (strcmp(arg, "open") == 0) {
    open = true;
  } else if (strcmp(arg, "close") == 0) {
    open = false;
  } else {
    Serial << "no valve argument supplied. valid arguments are \"open\" and \"close\"\n";
    return;
  }

  // TODO: make sure that the valve is not used (currently not supported by valve)
  auto& v = w->getValve();
  if (open) {
    v.open();
    Serial << "valve " << id << " opened " << timeClient.getFormattedTime() << "\n";
  } else {
    v.close();
    Serial << "valve " << id << " closed " << timeClient.getFormattedTime() << "\n";
  }
}

#include <stdarg.h>
void p(char *fmt, ... )
{
  char buf[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 128, fmt, args);
  va_end (args);
  Serial.print(buf);
}

void prtCircuitInfo(WaterCircuit* w, int id)
{
  Serial
    <<  "on-board circuit [" << id << "]:"
    << (w->getPumpSeconds() == 0 ?
        " off\n" :  " active\n")    
    <<  "           pump time  "; p("%3u s\n", w->getPumpSeconds());   Serial
    <<  "          dry thresh  "; p("%3u\n", w->getThreshDry());       Serial
    <<  "          wet thresh  "; p("%3u\n",  w->getThreshWet());      Serial
    <<  "           soak time  "; p("%3u m\n", w->getSoakMinutes());   Serial
    <<  "    reservoir thresh  "; w->getThreshReservoir() == 0 ? p("off\n") : p("%3u\n", w->getThreshReservoir()); Serial
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
          Serial << "\n";
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
    Serial << "invalid index\n";
    return;
  }
  
  char* arg = sCmd.next();
  if (arg == NULL) {
    Serial << "no parameter\n";
    return;
  }

  if (strcmp(arg, "pump") == 0) {
    int s;
    if (not cmdParseInt(s, 0, 255)) {
      Serial << "pump seconds must be between 0 and 255\n";
      return;
    }
    w->setPumpSeconds(s);
    Serial << "pump time set to " << s << " seconds\n";
  } else if (strcmp(arg, "soak") == 0) {
    int m;
    if (not cmdParseInt(m, 0, 255)) {
      Serial << "soak minutes must be between 0 and 255\n";
      return;
    }
    w->setSoakMinutes(m);
    Serial << "soak time set to " << m << " minutes\n";
  } else if (strcmp(arg, "dry") == 0) {
    int t;
    if (not cmdParseInt(t, 0, 255)) {
      Serial << "dry threshold must be between 0 and 255\n";
      return;
    }
    w->setThreshDry(t);
    Serial << "dry threshold set to " << t << "\n";
  } else if (strcmp(arg, "wet") == 0) {
    int t;
    if (not cmdParseInt(t, 0, 255)) {
      Serial << "wet threshold must be between 0 and 255\n";
      return;
    }
    w->setThreshWet(t);
    Serial << "wet threshold set to " << t << "\n";
  } else if (strcmp(arg, "res") == 0) {
    int t;
    if (not cmdParseInt(t, 0, 255)) {
      Serial << "reservoir threshold must be between 0 and 255\n";
      return;
    }
    w->setThreshReservoir(t);
    Serial << "reservoir threshold set to " << t << "\n";
  } else {
    Serial << "invalid parameter \"" << arg << "\"\n";
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
  
  Serial
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
        Serial << "\n";
      }
    }
    return;
  }

  printLoggerInfo(id);
}

#include <climits>

void cmdLogSet()
{
  int id;

  if (not cmdParseInt(id, 1, NumWaterCircuits)) {
    Serial << "invalid index\n";
    return;
  }

    /* Attention: as soon as we have different loggers we have to make sure that we use the right types! */
  ThingSpeakLogger* tsl = static_cast<ThingSpeakLogger*>(loggers[id - 1]);

  char* arg = sCmd.next();
  if (arg == NULL) {
    Serial << "no parameter\n";
    return;
  }

  if (strcmp(arg, "interval") == 0) {
    int m;
    if (not cmdParseInt(m, 0, 1440)) {
      Serial << "interval minutes must be between 0 and 1440 (24h)\n";
      return;
    }
    tsl->setIntervalMinutes(m);
    Serial << "log interval set to " << m << " minutes\n";
  } else if (strcmp(arg, "chid") == 0) {
    long chid;
    if (not cmdParseLong(chid, 0, LONG_MAX)) {
      Serial << "channel ID must be unsigned\n";
      return;
    }
    tsl->setChannelId(chid);
    Serial << "channel ID set to " << chid << "\n";
  } else if (strcmp(arg, "key") == 0) {
    char* karg = sCmd.next();
      if (karg == NULL) {
        tsl->setWriteApiKey("");
        Serial << "write API key set to \"\"\n";
      } else {
        tsl->setWriteApiKey(karg);
        Serial << "write API key set to \"" << karg << "\"\n";
      }
  } else {
    Serial << "invalid parameter \"" << arg << "\"\n";
    return;
  }

  flashMemory.update();
}

void cmdSchedulerInfo()
{
  Serial << "scheduler entries:\n";
  
  int i = 1;  
  for (SchedulerTime **t = schedulerTimes; *t; t++, i++) {
    Serial << "  Entry [" << i << "]: ";
    if ((*t)->isValid()) {
      prtTwoDigit((*t)->getHour());
      Serial << ":";
      prtTwoDigit((*t)->getMinute());
      Serial << "\n";
    } else {
      Serial << "off\n";
    }
  }
}

void cmdSchedulerSet()
{
  int index;
  if (not cmdParseInt(index, 1, NumSchedulerTimes)) {
    Serial << "index must be in the range 1 .. " << NumSchedulerTimes << "\n";
    return;
  }
  
  char* arg = sCmd.next();
  if (arg == NULL             or
      strcmp(arg, "off") != 0 and
      not (strlen(arg) == 5   and
           isDigit(arg[0])    and
           isDigit(arg[1])    and
           arg[2] == ':'      and
           isDigit(arg[3])    and
           isDigit(arg[4])))
  {
    Serial << "time must be of format \"hh:mm\" or \"off\"\n";
    return;
  }

  if (strcmp(arg, "off") == 0) {
    Serial << "configuring scheduler entry [" << index << "] to \"off\"\n";
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
    Serial << "hour must be in the range 0 .. 23\n";
    return;
  }

  if (m < 0 or m > 59) {
    Serial << "minute must be in the range 0 .. 59\n";
    return;
  }

  Serial << "configuring scheduler entry [" << index << "] to ";
  prtTwoDigit(h);
  Serial << ":";
  prtTwoDigit(m);
  Serial << "\n";
  
  schedulerTimes[index - 1]->setHour(h);
  schedulerTimes[index - 1]->setMinute(m);

  flashMemory.update();
}

void cmdNetworkRssi()
{
  Serial << "current RSSI: " << WiFi.RSSI() << " dB\n";
}


void cmdNetworkSsid()
{
  const char* arg = sCmd.next();

  if (arg == NULL or strlen(arg) == 0) {
    Serial << "no wifi SSID argument\n";
    return;
  }
  strncpy(flashDataSet.wifiSsid, arg, MaxSsidNameLen);
  flashMemory.update();

  Serial << "wifi SSID \"" << arg << "\" stored to flash\n";
}

void cmdNetworkPass()
{
  const char* arg = sCmd.next();

  if (arg == NULL or strlen(arg) == 0) {
    Serial << "no wifi password argument\n";
    return;
  }
  strncpy(flashDataSet.wifiPass, arg, MaxSsidPassLen);
  flashMemory.update();

  Serial << "wifi pass \"" << arg << "\" stored to flash\n";
}

void cmdNetworkConnect()
{
  network.disconnect();
  network.connect();
}

void cmdNetworkList()
{
  Network::printVisibleNetworks();
}

void cmdInvalid(const char *command)
{
  Serial
    << "what do you mean by \"" << command << "\"?\n"
    << "valid commands are:\n"
    ;
    cmdHelp();
}

void cliInit()
{
  sCmd.addCommand("help",   cmdHelp);
  sCmd.addCommand("time",   cmdTime);
  sCmd.addCommand("mode",   cmdMode);
  
  sCmd.addCommand("c.trig",  cmdCircuitTrigger);
  sCmd.addCommand("c.read",  cmdCircuitRead);
  sCmd.addCommand("c.res",   cmdCircuitReservoir);
  sCmd.addCommand("c.pump",  cmdCircuitPump);
  sCmd.addCommand("c.valve", cmdCircuitValve);
  sCmd.addCommand("c.info",  cmdCircuitInfo);
  sCmd.addCommand("c.set",   cmdCircuitSet);

  sCmd.addCommand("l.trig",  cmdLogTrigger);
  sCmd.addCommand("l.info",  cmdLogInfo);
  sCmd.addCommand("l.set",   cmdLogSet);

  sCmd.addCommand("s.info",  cmdSchedulerInfo);
  sCmd.addCommand("s.set",   cmdSchedulerSet);

  sCmd.addCommand("n.rssi",    cmdNetworkRssi);
  sCmd.addCommand("n.list",    Network::printVisibleNetworks);
  sCmd.addCommand("n.ssid",    cmdNetworkSsid);
  sCmd.addCommand("n.pass",    cmdNetworkPass);
  sCmd.addCommand("n.connect", cmdNetworkConnect);

  sCmd.setDefaultHandler(cmdInvalid);
}

void cliRun()
{
  sCmd.readSerial();
}

