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


/** @example sonyPTZDemo.cpp Example using a joystick to operate a Sony PTZ
 * camera
 *
  This is basically just a demo of how to use the sony, but made fancy so
  you can drive around the robot and look at stuff with the camera.  Press 
  down button 1 to drive the robot, button 2 to move the camera, and both
  buttons to zoom the camera.
*/

/*
  This class is the core of this demo, it adds itself to the robot given
  as a user task, then drives the robot and camera from the joystick
*/
class Joydrive
{
public:
  // constructor
  Joydrive(ArRobot *robot, int LRAmount = 15, int UDAmount = 10, 
	   int zoomAmount = 50);
  // destructor, its just empty
  ~Joydrive(void) {}

  // the callback function
  void drive(void);

  // callbacks for key presses
  void up(void);
  void down(void);
  void left(void);
  void right(void);
  void in(void);
  void out(void);
  void center(void);
protected:
  // the rotational max for the robot
  int myRotValRobot;
  // the translational max for the robot
  int myTransValRobot;
  // the pan max for the camera
  int myPanValCamera;
  // the tilt max for the camera
  int myTiltValCamera;
  // the zoom max for the camera
  int myZoomValCamera;
  
  // joystick handler
  ArJoyHandler myJoyHandler;

  // the camera
  ArSonyPTZ myCam;
  // the robot pointer
  ArRobot *myRobot;
  // callback for the drive function
  ArFunctorC<Joydrive> myDriveCB;
  ArFunctorC<Joydrive> myUpCB;
  ArFunctorC<Joydrive> myDownCB;
  ArFunctorC<Joydrive> myLeftCB;
  ArFunctorC<Joydrive> myRightCB;
  ArFunctorC<Joydrive> myInCB;
  ArFunctorC<Joydrive> myOutCB;
  ArFunctorC<Joydrive> myCenterCB;
  int myLRAmount;
  int myUDAmount;
  int myZoomAmount;
};

/*
  Constructor, sets the robot pointer, and some initial values, also note the
  use of constructor chaining on myCam and myDriveCB.
*/
Joydrive::Joydrive(ArRobot *robot, int LRAmount, int UDAmount, int zoomAmount) :
  myCam(robot),
  myDriveCB(this, &Joydrive::drive),
  myUpCB(this, &Joydrive::up),
  myDownCB(this, &Joydrive::down),
  myLeftCB(this, &Joydrive::left),
  myRightCB(this, &Joydrive::right),
  myInCB(this, &Joydrive::in),
  myOutCB(this, &Joydrive::out),
  myCenterCB(this, &Joydrive::center)
    
{
  // set the robot pointer and add the joydrive as a user task
  myRobot = robot;
  myRobot->addUserTask("joydrive", 50, &myDriveCB);
  



  // initalize some variables
  myRotValRobot = 100;
  myTransValRobot = 700;
  myPanValCamera = 8;
  myTiltValCamera = 3;
  myZoomValCamera = 50;
  myLRAmount = LRAmount;
  myUDAmount = UDAmount;
  myZoomAmount = zoomAmount;

  ArKeyHandler *keyHandler;
  myRobot = robot;
  // see if there is already a keyhandler, if not make one for ourselves
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    myRobot->attachKeyHandler(keyHandler);
  }
  // now that we have one, add our keys as callbacks, print out big
  // warning messages if they fail
  if (!keyHandler->addKeyHandler(ArKeyHandler::UP, &myUpCB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for up, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::DOWN, &myDownCB))
    ArLog::log(ArLog::Terse, "The key handler already has a key for down, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::LEFT, &myLeftCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for left, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler(ArKeyHandler::RIGHT, &myRightCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for right, keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('z', &myInCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'z', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('Z', &myInCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'Z', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('x', &myOutCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'x', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('X', &myOutCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'X', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('c', &myCenterCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'c', keydrive will not work correctly.");
  if (!keyHandler->addKeyHandler('C', &myCenterCB))
    ArLog::log(ArLog::Terse,  
"The key handler already has a key for 'C', keydrive will not work correctly.");


  // initialize the joystick
  myJoyHandler.init();
  // see if we have a joystick, and let the usre know the results
  if (myJoyHandler.haveJoystick())
  {
    printf("Have a joystick\n\n");
  }
  // we don't have a joystic, so get out
  else
  {
    printf("Do not have a joystick, only the keyboard will work.\n");
  }
}

void Joydrive::left(void)
{
  myCam.panTiltRel(-myLRAmount, 0);
}

void Joydrive::right(void)
{
  myCam.panTiltRel(myLRAmount, 0);
}

void Joydrive::up(void)
{
  myCam.panTiltRel(0, myUDAmount);
}

void Joydrive::down(void)
{
  myCam.panTiltRel(0, -myUDAmount);
}

void Joydrive::in(void)
{
  myCam.zoomRel(myZoomAmount);
}

void Joydrive::out(void)
{
  myCam.zoomRel(-myZoomAmount);
}

void Joydrive::center(void)
{
  myCam.panTilt(0,0);
  myCam.zoom(0);
}

void Joydrive::drive(void)
{
  int trans, rot;
  int pan, tilt;
  int zoom, nothing;

  // if both buttons are pushed, zoom the joystick
  if (myJoyHandler.haveJoystick() && myJoyHandler.getButton(1) && 
      myJoyHandler.getButton(2))
  {
    // set its speed so we get desired value range, we only care about y
    myJoyHandler.setSpeeds(0, myZoomValCamera);
    // get the values
    myJoyHandler.getAdjusted(&nothing, &zoom);
    // zoom the camera
    myCam.zoomRel(zoom);
  }
  // if both buttons aren't pushed, see if one button is pushed
  else 
  {
    // if button one is pushed, drive the robot
    if (myJoyHandler.haveJoystick() && myJoyHandler.getButton(1))
    {
      // set the speed on the joystick so we get the values we want
      myJoyHandler.setSpeeds(myRotValRobot, myTransValRobot);
      // get the values
      myJoyHandler.getAdjusted(&rot, &trans);
      // set the robots speed
      myRobot->setVel(trans);
      myRobot->setRotVel(-rot);
    }
    // if button one isn't pushed, stop the robot
    else
    {
      myRobot->setVel(0);
      myRobot->setRotVel(0);
    }
    // if button two is pushed, move the camera
    if (myJoyHandler.haveJoystick() && myJoyHandler.getButton(2))
    {
      // set the speeds on the joystick so we get desired value range
      myJoyHandler.setSpeeds(myPanValCamera, myTiltValCamera);
      // get the values
      myJoyHandler.getAdjusted(&pan, &tilt);
      // drive the camera
      myCam.panTiltRel(pan, tilt);
    } 
  }

}

int main(int argc, char **argv) 
{
  // just some stuff for returns
  std::string str;
  int ret;
  
  // robots connection
  ArSerialConnection con;
  // the robot, this turns state reflection off
  ArRobot robot(NULL, false);
  // the joydrive as defined above, this also adds itself as a user task
  Joydrive joyd(&robot);

  // mandatory init
  Aria::init();

  // open the connection, if it fails, exit
  if ((ret = con.open()) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    Aria::exit(1);
    return 1;
  }

  // set the connection o nthe robot
  robot.setDeviceConnection(&con);
  // connect, if we fail, exit
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);
    return 1;
  }

  // turn off the sonar, enable the motors, turn off amigobot sounds
  robot.comInt(ArCommands::SONAR, 0);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);

  // run, if we lose connection to the robot, exit
  robot.run(true);
  
  // exit program
  Aria::exit(0);
  return 0;
}

