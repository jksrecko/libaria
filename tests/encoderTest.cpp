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
#include <time.h>

ArRobot *robot;

bool encoderPrinter(ArRobotPacket *packet)
{
  long int left;
  long int right;
  printf("0x%X %s\n", packet->getID(), 
    (packet->getID() == 0x90 ? "[ENCODERpac]" : 
      ( (packet->getID() == 0x32 || packet->getID() == 0x33) ? "[SIP]" : "" ) 
    ) 
  );
  if (packet->getID() != 0x90)
    return false;
  left = packet->bufToByte4();
  right = packet->bufToByte4();
  printf("### %ld %ld\n", left, right);
  return true;
}
  

int main(int argc, char **argv) 
{
  std::string str;
  int ret;
  
  ArGlobalRetFunctor1<bool, ArRobotPacket *> encoderPrinterCB(&encoderPrinter);
  ArSerialConnection con;
  Aria::init();
  
  robot = new ArRobot;

  robot->addPacketHandler(&encoderPrinterCB, ArListPos::FIRST);

  ArArgumentParser args(&argc, argv);
  char* portName = args.checkParameterArgument("-rp");  // might return NULL but ArSerialConnection::open accepts that for default port.
  if ((ret = con.open(portName)) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    exit(0);
  }

  robot->setDeviceConnection(&con);
  if (!robot->blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    exit(0);
  }

  robot->requestEncoderPackets();
  //robot->comInt(ArCommands::ENCODER, 1);

  robot->run(true);
  Aria::shutdown();
  
}

