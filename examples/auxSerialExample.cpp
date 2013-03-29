/*
MobileRobots Advanced Robotics Interface for Applications (ARIA)
Copyright (C) 2004, 2005 ActivMedia Robotics LLC
Copyright (C) 2006, 2007, 2008, 2009, 2010 MobileRobots Inc.
Copyright (C) 2011, 2012 Adept Technology

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
MobileRobots Inc, 10 Columbia Drive, Amherst, NH 03031; 800-639-9481
*/
#include "Aria.h"

/** @example auxSerialExample.cpp
 * Demonstrates the use of a robot packet handler to recieve data from a device 
 * attached to the robot microcontroller's auxilliary serial port.
 
  This example shows how to use the GETAUX command and how to recieve SERAUX
  serial port data.  
  To use this example, you must have the auxilliary serial port AUX1 on the
  microcontroller (inside the robot) connected to a device that is sending text data.

  You can connect AUX1 to a computer through an RS-232 cable with a NULL modem adapter, 
  and run a program such as minicom (Linux) or HyperTerminal (Windows). Then just
  type to send data to the AUX1 port.  Configure minicom or HyperTerminal to have 
  the following serial port settings:
    9600 baud
    8 data bits, 1 stop bit, no parity
    No flow control (not hardware, not software)!
  
  This program creates a packet handler function (getAuxPrinter)
  and adds it to the ArRobot object. All packets recieved from the robot
  are passed to all packet handlers. getAuxPrinter() checks whether the packet
  is an SERAUX packet, and if so, prints out the data contained within the packet.
  If a newline or carriage return character is recieved, then it sends a command
  to send data back out through the AUX serial port.  The packet handler then sends 
  a GETAUX command to the robot to request more data.
*/

// our robot object
ArRobot *robot;

/** 
   Packet handler that simply prints out each byte of a packet as a
   character, then sends another request for more data (using GETAUX command).
   When it receives a newline or carriage return, it also replies 
   using the TTY2 command with a string ("Hello, world!\n\r").
*/
bool getAuxPrinter(ArRobotPacket *packet)
{
  char c;
  
  // If this is not an aux. serial data packet, then return false to allow other
  // packet handlers to recieve the packet. The packet type ID numbers are found
  // in the description of the GETAUX commands in the robot manual.
  if (packet->getID() != 0xb0) // 0xB0 is SERAUXpac. SERAUX2pac is 0xB8, SERAUX3pac is 0xC8.
    return false;

  // Get bytes out of the packet buffer and print them.
  while (packet->getReadLength () < packet->getLength() - 2)
  {
    c = packet->bufToUByte();
    if (c == '\r' || c == '\n')
    {
      putchar('\n');

      // How to send data to the serial port. See robot manual
      // (but note that TTY2 is for the AUX1 port, TTY3 for AUX2, etc.)
      robot->comStr(ArCommands::TTY2, "Hello, World!\n\r");
    }
    else
      putchar(c);
    fflush(stdout);
  }

  
  // Request more data:
  robot->comInt(ArCommands::GETAUX, 1);

  // To request 12 bytes at a time, specify that instead:
  //robot->comInt(ArCommands::GETAUX, 12);

  // If you wanted to recieve information from the second aux. serial port, use
  // the GETAUX2 command instead; but the packet returned will also have a
  // different type ID.
  //robot->comInt(ArCommands::GETAUX2, 1);
  

  // Return true to indicate to ArRobot that we have handled this packet.
  return true;
}
  

int main(int argc, char **argv) 
{
  Aria::init();

  ArArgumentParser argParser(&argc, argv);
  ArSimpleConnector conn(&argParser);
  argParser.loadDefaultArguments();
  if(!Aria::parseArgs() || !argParser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    return 1;
  }
  
  // This is a global pointer so the global functions can use it.
  robot = new ArRobot;

  // functor for the packet handler
  ArGlobalRetFunctor1<bool, ArRobotPacket *> getAuxCB(&getAuxPrinter);
  // add our packet handler as the first one in the list
  robot->addPacketHandler(&getAuxCB, ArListPos::FIRST);

  // Connect to the robot
  if(!conn.connectRobot(robot))
  {
      ArLog::log(ArLog::Terse, "getAuxExample: Error connecting to the robot.");
      return 2;
  }

  ArLog::log(ArLog::Normal, "getAuxExample: Connected to the robot. Sending command to change AUX1 baud rate to 9600...");
  robot->comInt(ArCommands::AUX1BAUD, 0); // See robot manual

  // Send the first GETAUX request
  robot->comInt(ArCommands::GETAUX, 1);

  // If you wanted to recieve information from the second aux. serial port, use
  // the GETAUX2 command instead; but the packet returned will also have a
  // different type ID.
  //robot->comInt(ArCommands::GETAUX2, 1);

  // run the robot until disconnect, then shutdown and exit.
  robot->run(true);
  Aria::exit(0);
  return 0;  
}

