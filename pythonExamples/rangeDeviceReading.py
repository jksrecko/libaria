"""
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
"""
from AriaPy import *
import sys

# This Python script connects to the robot and prints out the current
# Sonar and Laser range readings.

Aria_init()
argparser = ArArgumentParser(sys.argv)
argparser.loadDefaultArguments()
robot = ArRobot()
conn = ArSimpleConnector(argparser)

if not Aria_parseArgs():
  Aria_logOptions()
  Aria_exit(1)

sonar = ArSonarDevice()
robot.addRangeDevice(sonar)

laser = ArSick()
laser.configureShort(1)
robot.addRangeDevice(laser)

if (not conn.connectRobot(robot)):
    print "Could not connect to robot, exiting"
    Aria_exit(1)




robot.runAsync(1)
laser.runAsync()
print "Connecting to laser and waiting 1 sec..."
laser.blockingConnect()
ArUtil_sleep(1000)
robot.lock()
poses = sonar.getCurrentBufferAsVector()
print "Sonar readings (%d) (Point coordinates in space):" % (len(poses))
for p in poses:
    print "    sonar reading at ", p
robot.unlock()

print ""
print ""

laser.lockDevice()
readings = laser.getRawReadingsAsVector()
print "Laser readings (%d):" % (len(readings))
for r in readings:
  print "   from laser placed at pose (X:%g, Y:%g) reading point pos is %s, distance is %d" % (r.getSensorX(), r.getSensorY(), r.getPose(), r.getRange())
laser.unlockDevice()


laser.disconnect()
print "goodbye."
Aria_exit(0)
