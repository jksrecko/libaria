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

/** @example robotSyncTaskExample.cpp  Shows how to add a task callback to ArRobot's synchronization/processing cycle

  A sensor interpretation task callback is invoked
  by the ArRobot object every cycle as it runs, which records the robot's current 
  pose and velocity.

  Note that tasks must take a small amount of time to execute, to avoid delaying the
  robot cycle.
*/

class PrintingTask
{
public:
  // Constructor. Adds our 'user task' to the given robot object.
  PrintingTask(ArRobot *robot);

  // Destructor. Removes our user task from the robot
  ~PrintingTask(void);
  
  // This method will be called by the callback functor
  void doTask(void);
protected:
  ArRobot *myRobot;

  // The functor to add to the robot for our 'user task'.
  ArFunctorC<PrintingTask> myTaskCB;
};


// the constructor (note how it uses chaining to initialize myTaskCB)
PrintingTask::PrintingTask(ArRobot *robot) :
  myTaskCB(this, &PrintingTask::doTask)
{
  myRobot = robot;
  // just add it to the robot
  myRobot->addSensorInterpTask("PrintingTask", 50, &myTaskCB);
}

PrintingTask::~PrintingTask()
{
  myRobot->remSensorInterpTask(&myTaskCB);
}

void PrintingTask::doTask(void)
{
  // print out some info about the robot
  printf("\rx %6.1f  y %6.1f  th  %6.1f vel %7.1f mpacs %3d", myRobot->getX(),
	 myRobot->getY(), myRobot->getTh(), myRobot->getVel(), 
	 myRobot->getMotorPacCount());
  fflush(stdout);

  // Need sensor readings? Try myRobot->getRangeDevices() to get all 
  // range devices, then for each device in the list, call lockDevice(), 
  // getCurrentBuffer() to get a list of recent sensor reading positions, then
  // unlockDevice().
}

int main(int argc, char** argv)
{
  // the connection
  ArSimpleConnector con(&argc, argv);
  if(!con.parseArgs())
  {
    con.logOptions();
    return 1;
  }

  // robot
  ArRobot robot;

  // sonar array range device
  ArSonarDevice sonar;

  // This object encapsulates the task we want to do every cycle. 
  // Upon creation, it puts a callback functor in the ArRobot object
  // as a 'user task'.
  PrintingTask pt(&robot);

  // initialize aria
  Aria::init();

  // add the sonar object to the robot
  robot.addRangeDevice(&sonar);

  // open the connection to the robot; if this fails exit
  if(!con.connectRobot(&robot))
  {
    printf("Could not connect to the robot.\n");
    return 2;
  }
  printf("Connected to the robot. (Press Ctrl-C to exit)\n");
  
  
  // Start the robot process cycle running. Each cycle, it calls the robot's
  // tasks. When the PrintingTask was created above, it added a new
  // task to the robot. 'true' means that if the robot connection
  // is lost, then ArRobot's processing cycle ends and this call returns.
  robot.run(true);

  printf("Disconnected. Goodbye.\n");
  
  return 0;
}
