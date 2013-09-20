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

/** @example lineFinderExample.cpp Simple example of Aria's line-finder utility,
 * which uses data from a laser rangefinder to detect a continues line of
 * points.
 *
 * This example program will constantly search for a line-like pattern in the
 * sensor readings of a laser rangefinder. If you press the 'f' key, those
 * points will be saved in 'points' and 'lines' files. Use arrow keys or
 * joystick to teleoperate the robot.
 */

int main(int argc, char **argv)
{
  Aria::init();

  ArSimpleConnector connector(&argc, argv);
  ArRobot robot;
  ArSick sick;

  if (!Aria::parseArgs() || argc > 1)
  {
    Aria::logOptions();
    Aria::exit(1); // exit program with error code 1
    return 1;
  }
  
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);
  robot.attachKeyHandler(&keyHandler);


  robot.addRangeDevice(&sick);

  // Create the ArLineFinder object. Set it to log lots of information about its
  // processing.
  ArLineFinder lineFinder(&sick);
  lineFinder.setVerbose(true);

  // Add key callbacks that simply call the ArLineFinder::getLinesAndSaveThem()
  // function, which searches for lines in the current set of laser sensor
  // readings, and saves them in files with the names 'points' and 'lines'.
  ArFunctorC<ArLineFinder> findLineCB(&lineFinder, 
				  &ArLineFinder::getLinesAndSaveThem);
  keyHandler.addKeyHandler('f', &findLineCB);
  keyHandler.addKeyHandler('F', &findLineCB);

  
  ArLog::log(ArLog::Normal, "lineFinderExample: connecting to robot...");
  if (!connector.connectRobot(&robot))
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);  // exit program with error code 1
    return 1;
  }
  robot.runAsync(true);

  // now set up the laser
  ArLog::log(ArLog::Normal, "lineFinderExample: connecting to SICK laser...");
  connector.setupLaser(&sick);
  sick.runAsync();
  if (!sick.blockingConnect())
  {
    printf("Could not connect to SICK laser... exiting\n");
    Aria::exit(1);
    return 1;
  }

  printf("If you press the 'f' key the points and lines found will be saved\n");
  printf("Into the 'points' and 'lines' file in the current working directory\n");

  robot.waitForRunExit();
  Aria::exit(0);
  return 0;
}



