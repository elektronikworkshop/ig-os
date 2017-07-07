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
#ifndef StreamCmd_h
#define StreamCmd_h

#if defined(WIRING) && WIRING >= 100
  #include <Wiring.h>
#elif defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif
#include <string.h>

// Size of the input buffer in bytes (maximum length of one command plus arguments)
#define SERIALCOMMAND_BUFFER 32
// Maximum length of a command excluding the terminating null
#define SERIALCOMMAND_MAXCOMMANDLENGTH 8

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define SERIALCOMMAND_DEBUG


class StreamCmd
{
public:
  StreamCmd(Stream& stream,
            char eolChar = '\n',
            const char* prompt = NULL);

  void readSerial();    // Main entry point.
  void clearBuffer();   // Clears the input buffer.
  char *next();         // Returns pointer to next token found in command buffer (for getting arguments to commands).

protected:
  typedef void(StreamCmd::*CommandCallback)(void);
  typedef void(StreamCmd::*DefaultCallback)(const char*);

  void addCommand(const char *command, CommandCallback commandCallback);  // Add a command to the processing dictionary.
  void setDefaultHandler(DefaultCallback defaultCallback);   // A handler to call when no valid command received.

  Stream& m_stream;

private:
  // Command/handler dictionary
  struct StreamCmdCallback {
    char command[SERIALCOMMAND_MAXCOMMANDLENGTH + 1];
    CommandCallback commandCallback;
  };                                    // Data structure to hold Command/Handler function key-value pairs
  StreamCmdCallback *commandList;   // Actual definition for command/handler array
  byte commandCount;

  // Pointer to the default handler function
  DefaultCallback m_defaultCallback;


  char delim[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
  char term;     // Character that signals end of command (default '\n')
  const char* m_prompt;

  char buffer[SERIALCOMMAND_BUFFER + 1]; // Buffer of stored characters while waiting for terminator character
  byte bufPos;                        // Current position in the buffer
  char *last;                         // State variable used by strtok_r during processing
};

#endif //StreamCmd_h
