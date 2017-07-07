/**
 * StreamCmd - A Wiring/Arduino library to tokenize and parse commands
 * received over a serial port.
 * 
 * Copyright (C) 2012 Stefan Rado
 * Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
 *                    http://husks.wordpress.com
 * 
 * Version 20120522
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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
#include "StreamCmd.h"

#include <string.h>

StreamCmd::StreamCmd(Stream& stream,
                     char eol,
                     const char* prompt)
  : m_stream(stream)
  , m_commandSets{
    {NULL, 0, NULL},
    {NULL, 0, NULL}}
  , m_currentCommandSet(0)
  , m_delimiter{' ', 0}
  , m_eol(eol)
  , m_prompt(prompt)
  , m_commandLine{0}
  , m_pos(0)
  , m_last(NULL)
{ }

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void
StreamCmd::addCommand(const char *command,
                      CommandCallback commandCallback)
{
  auto& count = set().m_commandCount;
  
  set().m_commandList = (CommandEntry*) realloc(set().m_commandList, (count + 1) * sizeof(CommandEntry));
  strncpy(set().m_commandList[count].command, command, MaxCommandSize);
  set().m_commandList[count].commandCallback = commandCallback;
  count++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void StreamCmd::setDefaultHandler(DefaultCallback defaultCallback)
{
  set().m_defaultCallback = defaultCallback;
}


/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void StreamCmd::readSerial()
{
  while (m_stream.available() > 0) {

    int raw = m_stream.read();
    if (raw == -1) {
      break;
    }
    char ch = raw;
        
    if (ch == m_eol) {

      bool executed = false;

      /* tokenize command line */
      
      char *command = strtok_r(m_commandLine, m_delimiter, &m_last);
      
      if (command) {
      
        for (unsigned int i = 0; i < set().m_commandCount; i++) {

          auto& entry = set().m_commandList[i];

          if (strncmp(command, entry.command, MaxCommandSize) == 0) {
            (this->*entry.commandCallback)();
            executed = true;
            break;
          }
        }
      }

      if (not executed and set().m_defaultCallback) {
        (this->*set().m_defaultCallback)(command ? command : "");
      }

      if (m_prompt) {
        m_stream.print(m_prompt);
        m_stream.print("> ");
      }
      
      clearBuffer();
      
    } else if (isprint(ch)) {     // Only printable characters into the buffer
      if (m_pos < CommandBufferSize) {
        m_commandLine[m_pos++] = ch;  // Put character into buffer
        m_commandLine[m_pos  ] = '\0';    // Null terminate
      } else {
        m_stream.println("StreamCmd line buffer full - increase StreamCmd::CommandBufferSize");
      }
    }
  }
}

const char*
StreamCmd::next()
{
  return strtok_r(NULL, m_delimiter, &m_last);
}

