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

/** @example triangleDriveToActionExample.cpp Example/demonstration of
 * ArActionTriangleDriveTo, which drives the robot towards a specially shaped
 * triangular target
 *
   Press g or G to use ArActionTriangleDriveTo to detect and drive towards
   a triangular target shape.  Press s or S to stop.
   See ArActionTriangleDriveTo for more information about the triangular target and what the action does and its parameters.
 **/

#include "Aria.h"


int main(int argc, char **argv)
{
  Aria::init();

  // parse our args and make sure they were all accounted for
  ArSimpleConnector connector(&argc, argv);

  ArRobot robot;

  // the laser. ArActionTriangleDriveTo will use this laser object since it is
  // named "laser" when added to the ArRobot.
  ArSick sick;

  if (!connector.parseArgs() || argc > 1)
  {
    connector.logOptions();
    Aria::exit(1);
    return 1;
  }
  
  // a key handler so we can do our key handling
  ArKeyHandler keyHandler;
  // let the global aria stuff know about it
  Aria::setKeyHandler(&keyHandler);
  // toss it on the robot
  robot.attachKeyHandler(&keyHandler);

  // add the laser to the robot
  robot.addRangeDevice(&sick);

  ArSonarDevice sonar;
  robot.addRangeDevice(&sonar);
  
  ArActionTriangleDriveTo triangleDriveTo;
  ArFunctorC<ArActionTriangleDriveTo> lineGoCB(&triangleDriveTo, 
				      &ArActionTriangleDriveTo::activate);
  keyHandler.addKeyHandler('g', &lineGoCB);
  keyHandler.addKeyHandler('G', &lineGoCB);
  ArFunctorC<ArActionTriangleDriveTo> lineStopCB(&triangleDriveTo, 
					&ArActionTriangleDriveTo::deactivate);
  keyHandler.addKeyHandler('s', &lineStopCB);
  keyHandler.addKeyHandler('S', &lineStopCB);

  ArActionLimiterForwards limiter("limiter", 150, 0, 0, 1.3);
  robot.addAction(&limiter, 70);
  ArActionLimiterBackwards limiterBackwards;
  robot.addAction(&limiterBackwards, 69);

  robot.addAction(&triangleDriveTo, 60);

  ArActionKeydrive keydrive;
  robot.addAction(&keydrive, 55);


  ArActionStop stopAction;
  robot.addAction(&stopAction, 50);
  
  // try to connect, if we fail exit
  if (!connector.connectRobot(&robot))
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);
    return 1;
  }

  robot.comInt(ArCommands::SONAR, 1);
  robot.comInt(ArCommands::ENABLE, 1);
  
  // start the robot running, true so that if we lose connection the run stops
  robot.runAsync(true);

  // now set up the laser
  connector.setupLaser(&sick);

  sick.runAsync();

  if (!sick.blockingConnect())
  {
    printf("Could not connect to SICK laser... exiting\n");
    Aria::exit(1);
    return 1;
  }

  printf("If you press the 'g' key it'll go find a triangle, if you press 's' it'll stop.\n");

  robot.waitForRunExit();
  Aria::exit(0);
  return 0;
}



