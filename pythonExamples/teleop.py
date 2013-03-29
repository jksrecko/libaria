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

# This is an example of how to use the limiting behaviors.
#  
#  The way it works is that it has a limiting behavior higher priority
#  than the joydrive action behavior.  So the joydrive action can try
#  to do whatever it wants, but it won't work.

Aria_init()

# Robot and device objects
robot = ArRobot()
sonar = ArSonarDevice()
sick = ArSick()
robot.addRangeDevice(sonar)

if 1:
  robot.addRangeDevice(sick)

# the joydrive action
jdAct = ArActionJoydrive()

# a keyboard drive action
kdAct = ArActionKeydrive()

# limiter for close obstacles
limiter = ArActionLimiterForwards("speed limiter near", 300, 600, 250)

# limiter for far away obstacles
limiterFar = ArActionLimiterForwards("speed limiter far", 300, 1100, 400)

# if the robot has upward facing IR sensors ("under the table" sensors), this 
# stops us too
tableLimiter = ArActionLimiterTableSensor()

# limiter so we don't bump things backwards
backwardsLimiter = ArActionLimiterBackwards()



connector = ArSimpleConnector(sys.argv)

if not connector.parseArgs():
  connector.logOptions()
  Aria_exit(1)

# try to connect to the robot, if we fail exit
print "Connecting to the robot..."
if not connector.connectRobot(robot):
  print "Could not connect to robot. exiting"
  Aria_shutdown()
  Aria_exit(1)

# try to connect to the laser, if we succeed, add as range device to robot
print "setup and connect sick"
sick.runAsync()
connector.setupLaser(sick)
if sick.blockingConnect():
  print "Connected to SICK laser"
else:
  print "Note, could not connect to SICK laser."

print "Connected to the robot.  Use arrow keys or joystick to move. Spacebar to stop."

# add the actions, put the limiters on top, then have the joydrive action,
# this will keep the action from being able to drive too fast and hit
# something
robot.addAction(tableLimiter, 100)
robot.addAction(limiter, 95)
robot.addAction(limiterFar, 90)
robot.addAction(backwardsLimiter, 85)
robot.addAction(kdAct, 51)
robot.addAction(jdAct, 50)

# enable motors and run the robot until connection is lost
robot.enableMotors()
robot.run(1)
Aria_shutdown()
