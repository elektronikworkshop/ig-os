/**
 * StreamCmd - A Wiring/Arduino library to tokenize and parse commands
 * received over a serial port.
 * 
 * Copyright (C) 2012 Stefan Rado
 * Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
 *                    http://husks.wordpress.com
 * 
 * 
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the m_eols of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef StreamCmd_h
#define StreamCmd_h

#if defined(WIRING) && WIRING >= 100
  #include <Wiring.h>
#elif defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

class StreamCmd
{
public:
  /** Command buffer size */
  static const unsigned int CommandBufferSize = 32;
  /** Maximum length of a command excluding the m_eolinating null*/
  static const unsigned int MaxCommandSize = 8;
  /** */
  static const unsigned int NumCommandSets = 2;
  
  StreamCmd(Stream& stream,
            char eol = '\n',
            const char* prompt = NULL);

  /** Read the stream and run the CLI engine */
  virtual void run();
  
  /** Clear input buffer. */
  void clearBuffer()
  {
    m_commandLine[0] = '\0';
    m_pos = 0;
  }
  
protected:
  typedef void(StreamCmd::*CommandCallback)(void);
  typedef void(StreamCmd::*DefaultCallback)(const char*);

  /** Add a command to the current command set.
   */
  void addCommand(const char *command, CommandCallback commandCallback);

  template <typename T>
  void addCommand(const char *command, void(T::*m)(void))
  {
    addCommand(command, static_cast<CommandCallback>(m));
  }

  /** Set the default handler of the current command set.
   */
  void setDefaultHandler(DefaultCallback defaultCallback);
  template <typename T>
  void setDefaultHandler(void(T::*m)(const char*))
  {
    setDefaultHandler(static_cast<DefaultCallback>(m));
  }

  /** Get next command token */
  const char *next();

  /** The stream object on which StreamCmd should operate on.
   */
  Stream& m_stream;

  /** Switch the command set.
   */
  bool switchCommandSet(uint8_t set)
  {
    if (set >= NumCommandSets) {
      return false;
    }
    m_currentCommandSet = set;
    return true;
  }

  /** Get the current active command set.
   */
  uint8_t getCommandSet() const
  {
    return m_currentCommandSet;
  }

private:

  /** Command dictionary entry. */
  struct CommandEntry
  {
    char command[MaxCommandSize + 1];
    CommandCallback commandCallback;
  };

  /** Struct representing a command set. Each set features a
   *  regular command dictionary plus a default command handler. 
   */
  struct CommandSet
  {
    CommandEntry* m_commandList;
    uint8_t m_commandCount;
    DefaultCallback m_defaultCallback;
  };

  CommandSet& set()
  {
    return m_commandSets[m_currentCommandSet];    
  }

  CommandSet m_commandSets[NumCommandSets];
  uint8_t m_currentCommandSet;


  /** Delimiter for tokenizing the command line. Defaults to a single space. */
  char m_delimiter[2];
  /** The end of line (EOL) character. */
  char m_eol;
  /** A user configurable command promt name (host name for instance). */
  const char* m_prompt;

  /** The command line buffer. */
  char m_commandLine[CommandBufferSize + 1];
  /** The current write position in the command line buffer. */
  uint8_t m_pos;
  /** strtok_r state variable (stores the previous token position) */
  char *m_last;
};

#endif //StreamCmd_h
