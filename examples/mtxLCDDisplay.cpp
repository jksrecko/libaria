/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004, 2005 ActivMedia Robotics LLC
Copyright (C) 2006, 2007, 2008, 2009, 2010 MobileRobots Inc.
Copyright (C) 2011, 2012, 2013 Adept Technology

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
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
#include "Aria.h"

/** @example mtxLCDDisplay.cpp Display text on the MTX LCD display */

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();
  ArRobot robot;

  ArRobotConnector robotConnector(&parser, &robot);
  if(!robotConnector.connectRobot())
  {
    ArLog::log(ArLog::Terse, "mtxLCDDisplay: Could not connect to the robot.");
    if(parser.checkHelpAndWarnUnparsed())
    {
        // -help not given
        Aria::logOptions();
        Aria::exit(1);
    }
  }

  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    Aria::exit(1);
  }

  robot.comInt(ArCommands::JOYINFO, 0);

  robot.runAsync(true);   // ArLCDMTX uses an ArRobot task to communicate with the LCD.

  ArLCDMTX* lcd = robot.findLCD(1);
  if(!lcd) 
  {
    ArLog::log(ArLog::Normal, "Not connected to an LCD display. Use --help for options to customize nonstandard LCD connections.");
    Aria::exit(2);
  }

  lcd->setMainStatus("Hello, world.");
  lcd->setTextStatus("text status");

#if 0
  for(unsigned char i = 0; i < 100; i += 10)
  {
    ArUtil::sleep(200);
    lcd->setSystemMeters(i, i);
  }
  for(unsigned char i = 100; i >= 0; i -= 10)
  {
    ArUtil::sleep(200);
    lcd->setSystemMeters(i, i);
  }
#endif

  robot.waitForRunExit();

  Aria::exit(0);
  return 0;
}
