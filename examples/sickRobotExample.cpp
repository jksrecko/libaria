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

/*
  This program connects to a robot, then to a laser, then prints out
  some readings, and finally exits.
*/

int main(int argc, char **argv)
{
  // parse our args and make sure they were all accounted for
  ArSimpleConnector connector(&argc, argv);

  // the robot
  ArRobot robot;
  // the laser
  ArSick sick;
  // all the information for our printing out
  double dist, angle;
  std::list<ArPoseWithTime *> *readings;
  std::list<ArPoseWithTime *>::iterator it;
  double farDist, farAngle;
  bool found;

  if (!connector.parseArgs() || argc > 1)
  {
    connector.logOptions();
    Aria::exit(1);
    return 1;
  }

  // add the laser to the robot
  robot.addRangeDevice(&sick);

  // try to connect, if we fail exit
  if (!connector.connectRobot(&robot))
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);
    return 1;
  }

  // start the robot running, true so that if we lose connection the run stops
  robot.runAsync(true);

  // now set up the laser
  connector.setupLaser(&sick);
  printf("filter threshold (distance to other readings) for current buffer is %0.2f mm\n", sick.getFilterNearDist());
  printf("timeout on current readings is %d sec. (0 means disabled).\n", sick.getMaxSecondsToKeepCurrent());

  sick.runAsync();

  if (!sick.blockingConnect())
  {
    printf("Could not connect to SICK laser... exiting\n");
    Aria::shutdown();
    return 1;
  }

  printf("Connected\n");
  ArUtil::sleep(500);
  
  int times = 0;
  while (times++ < 3)
  {
    //dist = sick.getCurrentBuffer().getClosestPolar(-90, 90, ArPose(0, 0), 30000, &angle);
    sick.lockDevice();

    /* Current closest reading within a degree range */
    dist = sick.currentReadingPolar(-90, 90, &angle);
    if (dist < sick.getMaxRange())
      printf("Closest reading %.2f mm away at %.2f degrees\n", dist, angle);
    else
      printf("No close reading.\n");

    /* Print current buffer of reading positions (maybe filtered) */
    readings = sick.getCurrentBuffer();
    int i = 0;
    for (it = readings->begin(), found = false; it != readings->end(); it++)
    {
      i++;
      dist = (*it)->findDistanceTo(ArPose(0, 0));
      angle = (*it)->findAngleTo(ArPose(0, 0));
      if (!found || dist > farDist)
      {
        found = true;
        farDist = dist;
        farAngle = angle;
      }
    }
    printf("%d readings in current buffer\n", i);
    if (found)
      printf("Furthest reading %.2f mm away at %.2f degrees\n", 
	     farDist, farAngle);
    else
      printf("No far reading found.\n");

    /* Print set of raw, unfiltered readings */
    int nign = 0;
    printf("%lu raw readings.\n", sick.getRawReadings()->size());
    for(std::list<ArSensorReading*>::const_iterator ri = sick.getRawReadings()->begin(); ri != sick.getRawReadings()->end(); ++ri)
    {
      if((*ri)->getIgnoreThisReading()) {
        ++nign;
      }
    }
    printf("%d readings are being ignored (nothing detected at those angles)\n", nign);
    
    sick.unlockDevice();
    ArUtil::sleep(100);

    puts("");
  }

  Aria::exit(0);
  return 0;
}




