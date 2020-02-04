#ifndef EW_IG_CLI_H
#define EW_IG_CLI_H

#include "system.h"
#include "log.h"
#include "spi.h"
#include "network.h"
#include "settings.h"

#include <StreamCmd.h>
#include <OneWire.h>
#include <Wire.h>

OneWire oneWireBus(OneWirePin);

const char* helpGeneral = 
  "help\n"
  "  print this help\n"
  "time\n"
  "  display current time\n"
  "mode <off, auto, man>\n"
  "  switch the intelligüss mode\n"
  "hist [a] [b]\n"
  "  print error history. with no arguments the first few entries are displayed\n"  // TODO: we should rather print the last few...
  "    hist [a] prints the first [a] entries, [a] == -1 prints the whole history\n"
  "    hist [a] [b] prints the history between entries [a] and [b]\n"
  "      if [b] is -1 all entries from [a] up to the end are printed\n"
  "debug [on|off]\n"
  "  no argument: show if debug logging is enabled\n"
  "     on  enable debug logging\n"
  "    off  disable debug logging\n"
  "version\n"
  "  print IG-OS version\n"
;
const char* helpCircuit = 
  "c.trig [id]\n"
  "  manually trigger watering cycle. if [id] is provided only\n"
  "  circuit with [id] is triggered\n"
  "c.read <id>\n"
  "  read sensor of circuit with ID <id>\n"
  "c.res <id>\n"
  "  read reservoir of circuit with ID <id>\n"
  "c.pump <id> <seconds>\n"
  "  run pump of circuit with ID <id> for <seconds> seconds\n"
  "c.valve <id> <open|close>\n"
  "  open or close the valve with ID <id>\n"
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
  "    maxit <count>\n"
  "      set maximum number of iterations, range 0 .. 255\n"
  "c.stop <id>\n"
  "  stop any watering/measuring activity on circuit <id> and return it to idle\n"
;

const char* helpLogger = 
  "l.info [id]\n"
  "  display logging configuration of circuit with ID [id]\n"
  "  if no [id] is provided the configuration for for all circuits is printed\n"
  "l.trig [id]\n"
  "  trigger transmission of current data to remote logger.\n"
  "  if no ID is provided, the data of all circuits is transmitted\n"
  "l.set <id> <param>\n"
  "  configure the logging backend for circuit with ID <id>\n"
  "  <param> must be one of:\n"
  "    interval <minutes>\n"
  "      the logging interval in minutes\n"
  "    chid <id>\n"
  "      the ThingSpeak channel ID (integer number)\n"
  "    key <key>\n"
  "      the ThingSpeak channel write API key (character string)\n"
;

const char* helpScheduler = 
  "s.info\n"
  "  print scheduler configuration\n"
  "s.set <index> <time>\n"
  "  configure scheduler whereas\n"
  "    <index> is the entry number between 1 and "  /*<< NumSchedulerTimes << */ "8\n"
  "    <time> is the time formatted \"hh:mm\" or \"off\"\n"
;

const char* helpNetwork = 
  "n.rssi\n"
  "  display the current connected network strength (RSSI)\n"
  "n.list\n"
  "  list the visible networks\n"
  "n.ssid [ssid]\n"
  "  with argument: set wifi SSID\n"
  "  without: show current wifi SSID\n"
  "n.pass [password]\n"
  "  with argument: set wifi password\n"
  "  without: show current wifi password\n"
  "n.host [hostname]\n"
  "  shows the current host name when invoked without argument\n"
  "  sets a new host name when called with argument\n"
  "n.connect\n"
  "  connect to configured wifi network\n"
  "n.telnet [params]\n"
  "  without [params] this prints the telnet configuration\n"
  "  with [params] the telnet server can be configured as follows\n"
  "    on\n"
  "      enables the telnet server\n"
  "    off\n"
  "      disables the telnet server\n"
  "    pass <pass>\n"
  "      sets the telnet login password to <password>\n"
;
   
class Cli
  : public StreamCmd< 2, /* _NumCommandSets    */
                     48, /* _MaxCommands       */
                     128, /* _CommandBufferSize */
                      8> /* _MaxCommandSize    */
{
private:
  bool m_cliTrigger;
  
public:
  Cli(Stream& stream,
      char eolChar = '\n',
      const char* prompt = NULL)
    : StreamCmd(stream, eolChar, prompt)
    , m_cliTrigger(false)
  {
    /* WARNING: Due to the static nature of StreamCmd any overflow of the command list will go unnoticed, since this object is initialized in global scope.
     */
    addCommand("help",      &Cli::cmdHelp);
    addCommand(".",         &Cli::cmdHelp);
    addCommand("c.",        &Cli::cmdHelp);
    addCommand("l.",        &Cli::cmdHelp);
    addCommand("s.",        &Cli::cmdHelp);
    addCommand("n.",        &Cli::cmdHelp);
    
    addCommand("time",      &Cli::cmdTime);
    addCommand("mode",      &Cli::cmdMode);
    addCommand("hist",      &Cli::cmdHist);
    addCommand("debug",     &Cli::cmdDebug);
    addCommand("version",   &Cli::cmdVersion);
    
    addCommand("c.trig",    &Cli::cmdCircuitTrigger);
    addCommand("c.read",    &Cli::cmdCircuitRead);
    addCommand("c.res",     &Cli::cmdCircuitReservoir);
    addCommand("c.pump",    &Cli::cmdCircuitPump);
    addCommand("c.valve",   &Cli::cmdCircuitValve);
    addCommand("c.info",    &Cli::cmdCircuitInfo);
    addCommand("c.set",     &Cli::cmdCircuitSet);
    addCommand("c.stop",    &Cli::cmdCircuitStop);

    addCommand("l.trig",    &Cli::cmdLogTrigger);
    addCommand("l.info",    &Cli::cmdLogInfo);
    addCommand("l.set",     &Cli::cmdLogSet);
  
    addCommand("s.info",    &Cli::cmdSchedulerInfo);
    addCommand("s.set",     &Cli::cmdSchedulerSet);
  
    addCommand("n.rssi",    &Cli::cmdNetworkRssi);
    addCommand("n.list",    &Cli::cmdNetworkList);
    addCommand("n.ssid",    &Cli::cmdNetworkSsid);
    addCommand("n.ssidr",    &Cli::cmdNetworkSsid); /* undocumented wifi SSID reset */
    addCommand("n.pass",    &Cli::cmdNetworkPass);
    addCommand("n.passr",    &Cli::cmdNetworkPass); /* undocumented wifi pass reset */
    addCommand("n.connect", &Cli::cmdNetworkConnect);
    addCommand("n.host",    &Cli::cmdNetworkHostName);
    addCommand("n.telnet",  &Cli::cmdNetworkTelnet);

    addCommand("ow",        &Cli::cmdOneWireScan);
    addCommand("i2c",       &Cli::cmdI2cScan);
    addCommand("ee",       &Cli::cmdEe);
  
    setDefaultHandler(&Cli::cmdInvalid);
  }

  /* 
   *  http://playground.arduino.cc/Learning/OneWire
   *  https://www.pjrc.com/teensy/td_libs_OneWire.html
   */

  /** 
   *  https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
   *  Scratch pad layout:
   *    BYTE 0  TEMPERATURE LSB
   *    BYTE 1  TEMPERATURE MSB
   *    BYTE 2  TH REGISTER OR USER BYTE 1*
   *    BYTE 3  TL REGISTER OR USER BYTE 2*
   *    BYTE 4  CONFIGURATION REGISTER
   *    BYTE 5  RESERVED 
   *    BYTE 6  RESERVED
   *    BYTE 7  RESERVED (10h)
   *    BYTE 8  CRC*
   */
  void readDS18X20(uint8_t* addr)
  {
    union DS18X20ScratchPad {
      uint8_t data[12];
      struct {
        uint16_t temp;
        uint8_t th;
        uint8_t tl;
        uint8_t config;
        uint8_t reserved[3];
        uint8_t crc;
      } m;
    } data;
    
    oneWireBus.reset();
    oneWireBus.select(addr);
    oneWireBus.write(0x44,1);         // start conversion, with parasite power on at the end

    delay(1000);     // maybe 750ms is enough, maybe not -- we might do a ds.depower() here, but the reset will take care of it.

    /* uint8_t present = */
    oneWireBus.reset();
    oneWireBus.select(addr);    
    oneWireBus.write(0xBE);         // Read Scratchpad

    /* stream() << "P = " << present << "\n"; */
    stream() << "   scratch: ";
    
    for (int i = 0; i < 9; i++) {           // we need 9 bytes
      data.data[i] = oneWireBus.read();
      char buf[4] = {0};
      sprintf(buf, "%02x", data.data[i]);
      stream() << buf << " ";
    }
    stream() << "\n";
    uint8_t crc = OneWire::crc8(data.data, 8);
    if (crc != data.m.crc) {
      char buf[4] = {0};
      sprintf(buf, "%02x", crc);
      stream() << "crc failed: " << buf << "\n";
    } else {
      float t = data.m.temp;
      t /= 16.;
      stream() << "      temp: " << t << " °C\n";
    }
  }
  void cmdOneWireScan()
  {
    oneWireBus.reset_search();
    while (true) {
      uint8_t addr[8] = {0};
      if (not oneWireBus.search(addr)) {
        stream() << "search done\n";        
        break;
      }
      stream() << "   address: ";
      for (unsigned int i = 0; i < sizeof(addr); i++) {
        char buf[4] = {0};
        sprintf(buf, "%02x", addr[i]);
        stream() << buf << " ";
      }
      stream() << "\n";

      if (OneWire::crc8(addr, 7) != addr[7]) {
        stream() << "CRC of device address failed!\n";
        continue;
      }

      stream() << "    family: ";
      
      switch (addr[0]) {
        case 0x10:
          stream() << "DS18S20\n";
          readDS18X20(addr);
          break;
        case 0x28:
        case 0x20:
          stream() << "DS18B20\n";
          readDS18X20(addr);
          break;
        default:
          stream() << "unknown\n";
          break;        
      }
      stream() << "----\n";
    }
  }

  void cmdI2cScan()
  {
    byte error, address;
    int nDevices;
   
    stream() << "scanning for i2c devices ...\n";
   
    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
   
      if (not error) {
        stream() << "I2C device found at address ";
        prtFmt(stream(), "0x%2x\n", address);
   
        nDevices++;
      }
      else if (error==4) {
        stream() << "unknown error at address ";
        prtFmt(stream(), "0x%2x\n", address);
      }    
    }
    if (nDevices == 0) {
      stream() << "No I2C devices found\n";
    } else {
      stream() << "done\n";
    }
  }

  void cmdEe()
  {
    stream() << "sizeof(size_t): " << sizeof(size_t) << "\n";
    
    size_t rw(0);
    if (getOpt(rw, "r", "w") != ArgOk) {
      stream() << "read (r) or write (r)\n";
      return;
    }
    unsigned int address(0);
    if (getUInt(address, 0, UINT_MAX, 16) != ArgOk) {
      stream() << "no valid address argument\n";
      return;
    }

    const uint8_t deviceAddress = 0x50;

    if (rw) {
      const char* arg = next();
      if (not arg or not strlen(arg)) {
        stream() << "you must provide a string to write\n";
        return;
      }
      I2cAt24Cxx ee;
      ee.begin(deviceAddress);
      ee.write(address, (const uint8_t*)arg, strlen(arg));
      stream() << "\"" << arg << "\" written to ";
      prtFmt(stream(), "0x%04x\n", address);
    } else {
      unsigned int count(0);
      if (getUInt(count, 0, UINT_MAX, 16) != ArgOk) {
        stream() << "no valid count argument\n";
        return;
      }
      stream() << "reading " << count << " bytes from ";
      prtFmt(stream(), "0x%04x:\n", address);
      I2cAt24Cxx ee;
      ee.begin(deviceAddress);
      while (count) {
        const size_t N = 32;
        char buf[N + 1] = {0};
        size_t toread = std::min(N, count);
        ee.read(address, (uint8_t*)buf, toread);
        stream() << buf;
        address += toread;
        count -= toread;
      }
      stream() << "\n";
    }   
  }
  
  bool isWateringTriggered()
  {
    bool ret = m_cliTrigger;
    m_cliTrigger = false;
    return ret;
  }
  
protected:  
  
  GetResult getId(int& id, WaterCircuit*& w)
  {
    auto r = getInt(id, 1, NumWaterCircuits);
    if (r != ArgOk) {
      return r;
    }
    
    w = circuits[id - 1];
  
    return r;
  }
  
  /* Serial command interface */
  
  virtual void cmdHelp()
  {
    const char* arg = current();

           if (strncmp(arg, ".", 1) == 0) {
      stream() << helpGeneral;
    } else if (strncmp(arg, "c.", 2) == 0) {
      stream() << helpCircuit;
    } else if (strncmp(arg, "l.", 2) == 0) {
      stream() << helpLogger;
    } else if (strncmp(arg, "s.", 2) == 0) {
      stream() << helpScheduler;
    } else if (strncmp(arg, "n.", 2) == 0) {
      stream() << helpNetwork;
    } else {
      stream()
        << "----------------\n"
        << helpGeneral
        << "WATERING CIRCUITS:\n"
        << helpCircuit
        << "LOGGER\n"
        << helpLogger
        << "SCHEDULER\n"
        << helpScheduler
        << "NETWORK\n"
        << helpNetwork
        << "----------------\n"
        ;
    }
  }
  
  void cmdTime()
  {
    stream() << "current time: " << systemTime.getTimeStr() << "\n";
  }
  
  void cmdMode()
  {
    const char* arg = next();
    if (arg == NULL) {
      stream() << F("mode argument missing\n");
      return;
    }
    
    SystemMode::Mode newMode = SystemMode::str2Mode(arg);
    if (newMode == SystemMode::Invalid) {
      stream() << F("invalid mode argument \"") << arg << F("\" -- allowed are: <off, auto, man>\n");
      return;
    }
  
    if (newMode != systemMode.getMode()) {
        for (unsigned int i = 0; i < NumWaterCircuits; i++) {
          circuits[i]->reset();
        }
        stream() << "system mode switched to \"" << arg << "\"\n";
        systemMode.setMode(newMode);
    }
  }

  void cmdHist()
  {
    int start = 0, end = 0;
    getInt(start, -1, INT_MAX);
    getInt(end, -1, INT_MAX);

    history::prt(stream(), start, end);
  }

  void cmdDebug()
  {
    enum {OFF = 0, ON};
    size_t idx(0);
    switch (getOpt(idx, "off", "on")) {
      case ArgOk:
        switch (idx) {
          case ON:
            if (flashSettings.debug) {
              stream() << "debug logging already enabled\n";
              return;
            }
            stream() << "enabling debug logging\n";
            flashSettings.debug = true;
            break;
          case OFF:
            if (not flashSettings.debug) {
              stream() << "debug logging already disabled\n";
              return;
            }
            stream() << "disabling debug logging\n";
            flashSettings.debug = false;
            break;
        }
        break;
      case ArgNone:
        stream() << "debug logging " << (flashSettings.debug ? "en" : "dis") << "abled\n";
        return;
      default:
        stream() << "invalid arguments\n";
        return;
    }
    
    Debug.enable(flashSettings.debug);
    flashSettings.update();
  }

  void cmdVersion()
  {
    PrintVersion(stream());
  }
  
  void cmdCircuitTrigger()
  {
    m_cliTrigger = true;
  }
  
  void cmdCircuitRead()
  {
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }
    
    Sensor& s = w->getSensor();
    
    if (s.getState() != Sensor::StateIdle) {
      stream() << "sensor is currently active, try again later\n";
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
  
    stream() << "humidity of " << id << " is " << h << "/255\n";
  }
  
  // TODO: mostly same code as "read" -> combine somehow
  void cmdCircuitReservoir()
  {
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }
    
    Sensor& r = w->getReservoir();
    
    if (r.getState() != Sensor::StateIdle) {
      stream() << "reservoir sensor is currently busy, try again later\n";
      return;
    }
    
    r.enable();
    while (r.getState() != Sensor::StateReady) {
      delay(500);
      r.run();
      spi.run();
      adc.run();
    }
    auto f = r.read();
    r.disable();
  
    stream() << "reservoir fill is " << f << "/255\n";
  }
  
  void cmdCircuitPump()
  {
    if (systemMode.getMode() != SystemMode::Manual) {
      stream() << "you need to be in manual mode to do this\n";
      return;
    }
    
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }
  
    float s;
    if (getFloat(s, 0, 60 * 10) != ArgOk) {
      stream() << "manual pump seconds must be between 0.0 and 60.0\n";
      return;
    }
  
    stream() << "pump enabled " << systemTime.getTimeStr() << "\n";
  
    auto& p = w->getPump();
    p.enable();
    delay(s * 1000);
    p.disable();
  
    stream() << "pump disabled " << systemTime.getTimeStr() << "\n";
  }
  
  void cmdCircuitValve()
  {
      if (systemMode.getMode() != SystemMode::Manual) {
      stream() << "you need to be in manual mode to do this\n";
      return;
    }
    
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      id = 1;
      for (WaterCircuit** w = circuits; *w; w++, id++) {
        stream() << "  valve " << id << ((*w)->getValve().isOpen() ? " open" : " closed") << "\n";
      }
      return;
    }
  
    const char* arg = next();
    
    if (not arg) {
      stream() << "  valve " << id << (w->getValve().isOpen() ? " open" : " closed") << "\n";
      return;
    }
    bool open;
    if (strcmp(arg, "open") == 0) {
      open = true;
    } else if (strcmp(arg, "close") == 0) {
      open = false;
    } else {
      stream() << "no valve argument supplied. valid arguments are \"open\" and \"close\"\n";
      return;
    }
  
    // TODO: make sure that the valve is not used (currently not supported by valve)
    auto& v = w->getValve();
    if (open) {
      v.open();
      stream() << "valve " << id << " opened " << systemTime.getTimeStr() << "\n";
    } else {
      v.close();
      stream() << "valve " << id << " closed " << systemTime.getTimeStr() << "\n";
    }
  }
  
  void prtCircuitInfo(WaterCircuit* w, int id)
  {
    stream() <<  "on-board circuit [" << id << "]:";
    w->prt(stream());
  }
  
  void cmdCircuitInfo()
  {
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      id = 1;
      for (WaterCircuit** c = circuits; *c; c++, id++) {
        prtCircuitInfo(*c, id);
        if (id != NumWaterCircuits) {
          stream() << "\n";
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
    if (getId(id, w) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }
    
    const char* arg = next();
    if (arg == NULL) {
      stream() << "no parameter\n";
      return;
    }
  
    if (strcmp(arg, "pump") == 0) {
      int s;
      if (getInt(s, 0, 255) != ArgOk) {
        stream() << "pump seconds must be between 0 and 255\n";
        return;
      }
      w->setPumpSeconds(s);
      stream() << "pump time set to " << s << " seconds\n";
    } else if (strcmp(arg, "soak") == 0) {
      int m;
      if (getInt(m, 0, 255) != ArgOk) {
        stream() << "soak minutes must be between 0 and 255\n";
        return;
      }
      w->setSoakMinutes(m);
      stream() << "soak time set to " << m << " minutes\n";
    } else if (strcmp(arg, "dry") == 0) {
      int t;
      if (getInt(t, 0, 255) != ArgOk) {
        stream() << "dry threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshDry(t);
      stream() << "dry threshold set to " << t << "\n";
    } else if (strcmp(arg, "wet") == 0) {
      int t;
      if (getInt(t, 0, 255) != ArgOk) {
        stream() << "wet threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshWet(t);
      stream() << "wet threshold set to " << t << "\n";
    } else if (strcmp(arg, "res") == 0) {
      int t;
      if (getInt(t, 0, 255) != ArgOk) {
        stream() << "reservoir threshold must be between 0 and 255\n";
        return;
      }
      w->setThreshReservoir(t);
      stream() << "reservoir threshold set to " << t << "\n";
    } else if (strcmp(arg, "maxit") == 0) {
      int i;
      if (getInt(i, 0, 255) != ArgOk) {
        stream() << "maximum iterations must be between 0 and 255\n";
        return;
      }
      w->setMaxIterations(i);
      stream() << "maximum iterations set to " << i << "\n";
    } else {
      stream() << "invalid parameter \"" << arg << "\"\n";
      return;
    }
  
    flashSettings.update();
  }

  void cmdCircuitStop()
  { 
    int id;
    WaterCircuit* w;
    if (getId(id, w) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }

    w->reset();
  }
  
  void cmdLogTrigger()
  {
    int id;
  
    if (getInt(id, 1, NumWaterCircuits) != ArgOk) {
      for (Logger** l = loggers; *l; l++) {
        (*l)->trigger();
      }
      return;
    }
  
    Logger* l = loggers[id - 1];
    l->trigger();
  }
  
  void printLoggerInfo(int id)
  {
    /* Attention: as soon as we have different loggers we have to make sure that we use the right types! */
    ThingSpeakLogger* tsl = static_cast<ThingSpeakLogger*>(loggers[id - 1]);
    auto& s = tsl->getTslSettings();
    
    stream()
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
  
    if (getInt(id, 1, NumWaterCircuits) != ArgOk) {
      for (id = 1; id <= (int)NumWaterCircuits; id++) {
        printLoggerInfo(id);
        if (id != NumWaterCircuits) {
          stream() << "\n";
        }
      }
      return;
    }
  
    printLoggerInfo(id);
  }
  
  
  void cmdLogSet()
  {
    int id;
  
    if (getInt(id, 1, NumWaterCircuits) != ArgOk) {
      stream() << "invalid index\n";
      return;
    }
  
      /* Attention: as soon as we have different loggers we have to make sure that we use the right types! */
    ThingSpeakLogger* tsl = static_cast<ThingSpeakLogger*>(loggers[id - 1]);
  
    const char* arg = next();
    if (arg == NULL) {
      stream() << "no parameter\n";
      return;
    }
  
    if (strcmp(arg, "interval") == 0) {
      int m;
      if (getInt(m, 0, 1440) != ArgOk) {
        stream() << "interval minutes must be between 0 and 1440 (24h)\n";
        return;
      }
      tsl->setIntervalMinutes(m);
      stream() << "log interval set to " << m << " minutes\n";
    } else if (strcmp(arg, "chid") == 0) {
      long chid;
      if (getLong(chid, 0, LONG_MAX) != ArgOk) {
        stream() << "channel ID must be unsigned\n";
        return;
      }
      tsl->setChannelId(chid);
      stream() << "channel ID set to " << chid << "\n";
    } else if (strcmp(arg, "key") == 0) {
      const char* karg = next();
      if (karg == NULL) {
        tsl->setWriteApiKey("");
        stream() << "write API key set to \"\"\n";
      } else {
        tsl->setWriteApiKey(karg);
        stream() << "write API key set to \"" << karg << "\"\n";
      }
    } else {
      stream() << "invalid parameter \"" << arg << "\"\n";
      return;
    }
  
    flashSettings.update();
  }
  
  void cmdSchedulerInfo()
  {
    stream() << "scheduler entries:\n";
    
    int i = 1;  
    for (SchedulerTime **t = schedulerTimes; *t; t++, i++) {
      stream() << "  Entry [" << i << "]: ";
      if ((*t)->isValid()) {
        prtFmt(stream(), "%02u:%02u\n", (*t)->getHour(), (*t)->getMinute());
      } else {
        stream() << "off\n";
      }
    }
  }
  
  void cmdSchedulerSet()
  {
    int index;
    auto res = getInt(index, 1, NumSchedulerTimes);
    if (res != ArgOk) {
      stream() << "index must be in the range 1 .. " << NumSchedulerTimes << "\n";
      return;
    }
    
    const char* arg = next();
    if (arg == NULL             or
        (strcmp(arg, "off") != 0 and
         not (strlen(arg) == 5   and
              isDigit(arg[0])    and
              isDigit(arg[1])    and
              arg[2] == ':'      and
              isDigit(arg[3])    and
              isDigit(arg[4]))))
    {
      stream() << "time must be of format \"hh:mm\" or \"off\"\n";
      return;
    }
  
    if (strcmp(arg, "off") == 0) {
      stream() << "configuring scheduler entry [" << index << "] to \"off\"\n";
      schedulerTimes[index - 1]->setHour(SchedulerTime::InvalidHour);
      schedulerTimes[index - 1]->setMinute(0);
  
      flashSettings.update();
  
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
//      m_proxyStream << flashDataSet.hostName << "> ";

      m = atoi(arg+3);
    }
  
    if (h < 0 or h > 23) {
      stream() << "hour must be in the range 0 .. 23\n";
      return;
    }
  
    if (m < 0 or m > 59) {
      stream() << "minute must be in the range 0 .. 59\n";
      return;
    }
  
    stream() << "configuring scheduler entry [" << index << "] to ";
    prtFmt(stream(), "%02u:%02u\n", h, m);
    
    schedulerTimes[index - 1]->setHour(h);
    schedulerTimes[index - 1]->setMinute(m);
  
    flashSettings.update();
  }
  
  void cmdNetworkRssi()
  {
    stream() << "RSSI: " << WiFi.RSSI() << " dB\n";
  }
  
  
  void cmdNetworkSsid()
  {
    const char* arg;
    if (strcmp(current(), "n.ssidr") == 0) {
      arg = "";
    } else {
      arg = next();

      /* revert all strtok effects to get the plain password with spaces etc.
       * "arg" now points to the start of the password and the password contains
       * all spaces.
       */
      reset();

      if (not arg) {
        stream() << "SSID: " << flashSettings.wifiSsid << "\n";
        return;
      }
    }
    strncpy(flashSettings.wifiSsid, arg, MaxWifiSsidLen);
    flashSettings.update();
  
    stream() << "SSID \"" << arg << "\" stored to flash\n";
  }
  
  void cmdNetworkPass()
  {
    const char* arg;
    if (strcmp(current(), "n.passr") == 0) {
      arg = "";
    } else {
      arg = next();

      /* revert all strtok effects to get the plain password with spaces etc.
       * "arg" now points to the start of the password and the password contains
       * all spaces.
       */
      reset();

      if (not arg) {
        /* this is perhaps not a good idea, but can come in handy for now */
        stream() << "wifi password: " << flashSettings.wifiPass << "\n";
        return;
      }
    }
    strncpy(flashSettings.wifiPass, arg, MaxWifiPassLen);
    flashSettings.update();
  
    stream() << "wifi pass \"" << arg << "\" stored to flash\n";
  }
  
  void cmdNetworkConnect()
  {
    network.disconnect();
    network.connect();
  }
  
  void cmdNetworkList()
  {
    Network::printVisibleNetworks(stream());
  }

  void cmdNetworkHostName()
  {
    const char* arg = next();
    if (not arg) {
      stream() << "host name: " << flashSettings.hostName << "\n";
      return;
    }

    if (strlen(arg) == 0) {
      stream() << "the host name can not be empty\n";
      return;
    }
    
    strncpy(flashSettings.hostName, arg, MaxHostNameLen);
    flashSettings.update();
  
    stream() << "new host name \"" << arg << "\" stored to flash. restarting network...\n";

    network.disconnect();
    network.connect();
  }

  void cmdNetworkTelnet()
  {
    size_t idx(0);
    enum {ON = 0, OFF, PASS};
    switch (getOpt(idx, "on", "off", "pass")) {
      case ArgOk:
        switch (idx) {
          case ON:
            stream() << "telnet server ";
            if (not flashSettings.telnetEnabled) {
              flashSettings.telnetEnabled = true;
              stream() << " now enabled\n";
            } else {
              stream() << "already enabled\n";
              return;
            }
            break;
          case OFF:
            stream() << "telnet server ";
            if (flashSettings.telnetEnabled) {
              flashSettings.telnetEnabled = false;
              stream() << " now disabled\n";
            } else {
              stream() << "already disabled\n";
              return;
            }
            break;
          case PASS:
          {
            const char* pass = next();
            if (not pass or strlen(pass) == 0) {
              stream() << "password must have at least a length of " << MinTelnetPassLen << "\n";
              return;
            }
            strncpy(flashSettings.telnetPass, pass, MaxTelnetPassLen);
            break;
          }
        }
        break;
      case ArgNone:
        stream() << "the telnet server is " << (flashSettings.telnetEnabled ? "on" : "off") << ", the login password is \"" << flashSettings.telnetPass << "\"\n"; 
        return;
      default:
        stream() << "invalid argument \"" << current() << "\", see \"help\" for proper use\n";
        return;
    }

    flashSettings.update();
  }
  void cmdInvalid(const char *command)
  {
    if (strlen(command)) {
      stream() << "what do you mean by \"" << command << "\"? try the \"help\" command\n";
    }
  }

};

Cli uartCli(Serial);


#include <TelnetServer.h>

class TelnetCli
  : public TelnetClient
  , public Cli
{
public:
  typedef StreamCmd<2> CliBase;
  enum CommandSet
  {
    Authenticated = 0,
    NotAuthenticated,
  };
  TelnetCli()
    : Cli(getStream(), '\r', flashSettings.hostName)
  {
    switchCommandSet(Authenticated);
    
    /* Add quit command to command set 0 */
    addCommand("quit",   &TelnetCli::cmdQuit);

    switchCommandSet(NotAuthenticated);

    setDefaultHandler(&TelnetCli::auth);
  }

private:
  void auth(const char* password)
  {
    if (strcmp(flashSettings.telnetPass, password) == 0) {
      switchCommandSet(0);
      return;
    }
    getStream() << "authentication failed -- wrong password\n";
    reset();
  }
  virtual void cmdHelp()
  {
    Cli::cmdHelp();
    getStream()
      << "quit\n"
      << "  quit telnet session and close connection\n";
  }
  void cmdQuit()
  {
    reset();
  }
  void begin(const WiFiClient& client)
  {
    if (isConnected()) {
      return;
    }

    TelnetClient::begin(client);

    /* Log before we add client to the logger proxies */
    Debug << "telnet client connection (" << getClient().remoteIP().toString() << ")\n";

    auto prterr = [](const char* who)
    {
      Error << "failed to add telnet stream proxy to " << who << " logger proxy\n";
    };
    if (not Log.addClient(getStream())) {
      prterr("default");
    }
    if (not Debug.addClient(getStream())) {
      prterr("debug");
    }
    if (not Error.addClient(getStream())) {
      prterr("error");
    }
    
    /* make sure the new client must authenticate first */
    switchCommandSet(NotAuthenticated);

    getStream()
      << WelcomeMessage("telnet")
      << "Please enter password for \"" << flashSettings.hostName << "\": ";
      ;
  }
  virtual void reset()
  {
    if (not isConnected()) {
      return;
    }

    getStream() << "bye\n";
    
    auto prterr = [](const char* who)
    {
      Error << "failed to remove telnet stream proxy from " << who << " logger proxy\n";
    };
    if (not Log.removeClient(getStream())) {
      prterr("default");
    }
    if (not Debug.removeClient(getStream())) {
      prterr("debug");
    }
    if (not Error.removeClient(getStream())) {
      prterr("error");
    }

    if (getClient()) {
      Debug << "telnet connection closed (" << getClient().remoteIP().toString() << ")\n";
    } else {
      Debug<< "telnet connection closed (unknown IP)\n";
    }

    TelnetClient::reset();
  }
  virtual void processStreamData()
  {
    Cli::run();
  }
};

TelnetCli telnetClis[MaxTelnetClients];
TelnetServer telnetServer(telnetClis, MaxTelnetClients);

#endif /* EW_IG_CLI_H */

