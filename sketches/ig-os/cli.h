
#include <ESP8266WiFi.h>

/* https://github.com/kroimon/Arduino-SerialCommand
 * https://github.com/kroimon/Arduino-SerialCommand/blob/master/examples/SerialCommandExample/SerialCommandExample.pde
 * https://playground.arduino.cc/Main/StreamingOutput
 */
#include <SerialCommand.h>
#include "system.h"
#include "log.h"

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
  Serial
    << "----------------\n"
    << "help\n"
    << "  print this help\n"
    << "rssi\n"
    << "  display the current RSSI\n"
    << "time\n"
    << "  display current time\n"
    << "mode <off, auto, man>\n"
    << "  switch the intelligüss mode\n"
    << "WATERING CIRCUITS:\n"
    << "c.trig [id]\n"
    << "  manually trigger watering cycle. if [id] is provided only\n"
    << "  circuit with [id] is triggered\n"
    << "c.read <id>\n"
    << "  read sensor of circuit with ID <id>\n"
    << "c.pump <id> <seconds>\n"
    << "  run pump of circuit with ID <id> for <seconds> seconds\n"
    << "c.info [id]\n"
    << "  display settings and state of circuit with ID [id]\n"
    << "  if no [id] is provided the info is printed for all circuits\n"
    << "c.set <id> <param>\n"
    << "  change settings and state of circuit with ID <id>\n"
    << "  <param> must be one of:\n"
    << "    pump <seconds>\n"
    << "      set pump time in seconds\n"
    << "    soak <seconds>\n"
    << "      set soak time in seconds\n"
    << "    dry <thresh>\n"
    << "      set dry threshold, range 0 .. 1.0\n"
    << "    wet <thresh>\n"
    << "      set wet threshold, range 0 .. 1.0\n"
    << "c.log [id]\n"
    << "  test remote logging. if no ID is provided, all info of all circuits will be logged\n"
    << "SCHEDULER\n"
    << "s.info\n"
    << "  print scheduler configuration\n"
    << "s.set <index> <time>\n"
    << "  configure scheduler whereas\n"
    << "    <index> is the entry number between " << 1 << " and " << NumSchedulerTimes << "\n"
    << "    <time> is the time formatted \"hh:mm\" or \"off\"\n"
    << "RESERVOIR\n"
    << "r.read\n"
    << "  read reservoir level\n"
    << "----------------\n"
    ;
}

void cmdRssi()
{
  Serial << "current RSSI: " << WiFi.RSSI() << " dB\n";
}

void cmdTime()
{
  Serial << "current time: " << timeClient.getFormattedTime() << "\n";
}

void cmdMode()
{
  char* arg = sCmd.next();
  if (arg == NULL) {
    Serial.println("mode argument missing");
    return;
  }
  
  SystemMode::Mode newMode = SystemMode::str2Mode(arg);
  if (newMode == SystemMode::Invalid) {
    Serial << "invalid mode argument \"" << arg << "\" -- allowed are: <off, auto, man>\n";
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
  /* todo */
    
  cliTrigger = true;
  Serial << "watering triggered manually\n";  
}

void cmdCircuitRead()
{
/*
  if (systemMode.getMode() != SystemMode::Manual) {
    Serial << "you need to be in manual mode to do this\n";
    return;
  }
*/
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
  }
  float h = s.read();
  s.disable();

  Serial << "humidity of " << id << " is " << h * 100 << " %\n";
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

void prtCircuitInfo(WaterCircuit* w, int id)
{
  Serial
    <<  "info circuit " << id << ":\n"
    <<  "  pump time  " << w->getPumpSeconds() / 1000 << " s\n"
    <<  "  dry thresh " << w->getThreshDry() << "\n"
    <<  "  wet thresh " << w->getThreshWet() << "\n"
    <<  "  soak time  " << w->getSoakMinutes() / 1000 << " s\n"
    <<  "  last read humidity  " << int(w->getHumidity() * 100) << " %\n"
    ;
}

void cmdCircuitInfo()
{
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    id = 1;
    for (WaterCircuit** c = circuits; *c; c++, id++) {
        prtCircuitInfo(*c, id);
      }
    return;
  }
  prtCircuitInfo(w, id);
}

void cmdCircuitSet()
{
/*  if (systemMode.getMode() == SystemMode::Auto) {
    Serial << "you need to be in manual or off mode to do this\n";
    return;
  }
 */
 
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
    return;
  } else if (strcmp(arg, "soak") == 0) {
    int m;
    if (not cmdParseInt(m, 0, 255)) {
      Serial << "soak minutes must be between 0 and 255\n";
      return;
    }
    w->setSoakMinutes(m);
    Serial << "soak time set to " << m << " minutes\n";
    return;
  } else if (strcmp(arg, "dry") == 0) {
    int t;
    if (not cmdParseInt(t, 0, 255)) {
      Serial << "dry threshold must be between 0 and 255\n";
      return;
    }
    w->setThreshDry(t);
    Serial << "dry threshold set to " << t << "\n";
    return;
  } else if (strcmp(arg, "wet") == 0) {
    int t;
    if (not cmdParseInt(t, 0, 255)) {
      Serial << "wet threshold must be between 0 and 255\n";
      return;
    }
    w->setThreshWet(t);
    Serial << "wet threshold set to " << t << "\n";
    return;
  } else {
    Serial << "invalid parameter \"" << arg << "\"\n";
    return;
  }
}

void cmdCircuitLog()
{
  int id;
  WaterCircuit* w;
  if (not cmdParseId(id, w)) {
    Serial << "invalid index\n";
    return;
  }
//  thingSpeakLogger.trigger();
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
    *schedulerTimes[index - 1] = SchedulerTime();
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
  
  *schedulerTimes[index - 1] = SchedulerTime(h, m);
}

void cmdReservoirRead()
{
  Serial << "not implemented yet\n";
}

void cmdNetworkSsid()
{
  Serial << "not implemented yet\n";
}

void cmdNetworkPass()
{
  Serial << "not implemented yet\n";
}

void cmdNetworkConnect()
{
  Serial << "not implemented yet\n";
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
  sCmd.addCommand("rssi",   cmdRssi);
  sCmd.addCommand("time",   cmdTime);
  sCmd.addCommand("mode",   cmdMode);
  sCmd.addCommand("c.trig", cmdCircuitTrigger);
  sCmd.addCommand("c.read", cmdCircuitRead);
  sCmd.addCommand("c.pump", cmdCircuitPump);
  sCmd.addCommand("c.info", cmdCircuitInfo);
  sCmd.addCommand("c.set",  cmdCircuitSet);
  sCmd.addCommand("c.log",  cmdCircuitLog);

  sCmd.addCommand("s.info", cmdSchedulerInfo);
  sCmd.addCommand("s.set",  cmdSchedulerSet);

  sCmd.addCommand("r.read", cmdReservoirRead);

  sCmd.addCommand("n.ssid", cmdNetworkSsid);
  sCmd.addCommand("n.pass", cmdNetworkPass);
  sCmd.addCommand("n.connect", cmdNetworkPass);

  sCmd.setDefaultHandler(cmdInvalid);
}

void cliRun()
{
  sCmd.readSerial();
}

