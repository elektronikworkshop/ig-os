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

/**
 * Constructor makes sure some things are set.
 */
StreamCmd::StreamCmd(Stream& stream,
                             char eolChar)
  : m_stream(stream)
  , commandList(NULL)
  , commandCount(0)
  , m_defaultCallback(NULL)
  , delim({' ', 0})
  , term(eolChar)           // default terminator for commands, newline character
  , buffer({0})
  , bufPos(0)
  , last(NULL)
{
//  strcpy(delim, " "); // strtok_r needs a null-terminated string
//  clearBuffer();
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void StreamCmd::addCommand(const char *command, CommandCallback commandCallback)
{
  #ifdef SERIALCOMMAND_DEBUG
    m_stream.print("Adding command (");
    m_stream.print(commandCount);
    m_stream.print("): ");
    m_stream.println(command);
  #endif

  commandList = (StreamCmdCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCmdCallback));
  strncpy(commandList[commandCount].command, command, SERIALCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].commandCallback = commandCallback;
  commandCount++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void StreamCmd::setDefaultHandler(DefaultCallback defaultCallback)
{
  m_defaultCallback = defaultCallback;
}


/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void StreamCmd::readSerial()
{
  while (m_stream.available() > 0) {

    int data = m_stream.read();
    if (data == -1) {
      break;
    }
    char inChar = data;

    #ifdef SERIALCOMMAND_DEBUG
      m_stream.print(inChar);   // Echo back to serial stream
    #endif
        
    if (inChar == term) {     // Check for the terminator (default '\r') meaning end of command
      #ifdef SERIALCOMMAND_DEBUG
        m_stream.print("Received: ");
        m_stream.println(buffer);
      #endif

      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
      if (command != NULL) {

        /* we ignore empty commands */
        if (strlen(command) == 0) {
          break;
        }
        
        boolean matched = false;
        for (int i = 0; i < commandCount; i++) {
          #ifdef SERIALCOMMAND_DEBUG
            m_stream.print("Comparing [");
            m_stream.print(command);
            m_stream.print("] to [");
            m_stream.print(commandList[i].command);
            m_stream.println("]");
          #endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, SERIALCOMMAND_MAXCOMMANDLENGTH) == 0) {
            #ifdef SERIALCOMMAND_DEBUG
              m_stream.print("Matched Command: ");
              m_stream.println(command);
            #endif

            // Execute the stored handler function for the command
            (this->*commandList[i].commandCallback)();
            matched = true;
            break;
          }
        }
        if (!matched && (m_defaultCallback != NULL)) {
          (this->*m_defaultCallback)(command);
        }
      }
      clearBuffer();
    }
    else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < SERIALCOMMAND_BUFFER) {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      } else {
        #ifdef SERIALCOMMAND_DEBUG
          m_stream.println("Line buffer is full - increase SERIALCOMMAND_BUFFER");
        #endif
      }
    }
  }
}

/*
 * Clear the input buffer.
 */
void StreamCmd::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *StreamCmd::next() {
  return strtok_r(NULL, delim, &last);
}