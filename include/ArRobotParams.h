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
#ifndef ARROBOTPARAMS_H
#define ARROBOTPARAMS_H

#include "ariaTypedefs.h"
#include "ArConfig.h"

///Stores parameters read from the robot's parameter files
/** 
    Use ArRobot::getRobotParams() after a successful robot 
    connection to obtain a pointer to an ArRobotParams instance containing the
    values read from the files.  

    See @ref ParamFiles for a description of the robot parameter files.

    ArRobotParams is a subclass of ArConfig which contains some useful methods
    and features.
*/
class ArRobotParams : public ArConfig
{
public:
  /// Constructor
  AREXPORT ArRobotParams();
  /// Destructor
  AREXPORT virtual ~ArRobotParams();
  /// Returns the class from the parameter file
  const char *getClassName(void) const { return myClass; }
  /// Returns the subclass from the parameter file
  const char *getSubClassName(void) const { return mySubClass; }
  /// Returns the robot's radius
  double getRobotRadius(void) const { return myRobotRadius; }
  /// Returns the robot diagonal (half-height to diagonal of octagon)
  double getRobotDiagonal(void) const { return myRobotDiagonal; }
  /// Returns the robot's width
  double getRobotWidth(void) const { return myRobotWidth; }
  /// Returns the robot's length
  double getRobotLength(void) const { return myRobotLength; }
  /// Returns the robot's length to the front of the robot
  double getRobotLengthFront(void) const { return myRobotLengthFront; }
  /// Returns the robot's length to the rear of the robot
  double getRobotLengthRear(void) const { return myRobotLengthRear; }
  /// Returns whether the robot is holonomic or not
  bool isHolonomic(void) const { return myHolonomic; }
  /// Returns if the robot has a built in move command
  bool hasMoveCommand(void) const { return myHaveMoveCommand; }
  /// Returns the max velocity of the robot
  int getAbsoluteMaxVelocity(void) const { return myAbsoluteMaxVelocity; }
  /// Returns the max rotational velocity of the robot
  int getAbsoluteMaxRotVelocity(void) const { return myAbsoluteMaxRVelocity; }
  /// Returns the max lateral velocity of the robot
  int getAbsoluteMaxLatVelocity(void) const 
    { return myAbsoluteMaxLatVelocity; }
  /// Returns true if IO packets are automatically requested upon connection to the robot.
  bool getRequestIOPackets(void) const { return myRequestIOPackets; }
  /// Returns true if encoder packets are automatically requested upon connection to the robot.
  bool getRequestEncoderPackets(void) const { return myRequestEncoderPackets; }
  /// Returns the baud rate set in the param to talk to the robot at
  int getSwitchToBaudRate(void) const { return mySwitchToBaudRate; }
  /// Returns the angle conversion factor 
  double getAngleConvFactor(void) const { return myAngleConvFactor; }
  /// Returns the distance conversion factor
  double getDistConvFactor(void) const { return myDistConvFactor; }
  /// Returns the velocity conversion factor
  double getVelConvFactor(void) const { return myVelConvFactor; }
  /// Returns the sonar range conversion factor
  double getRangeConvFactor(void) const { return myRangeConvFactor; }
  /// Returns the wheel velocity difference to angular velocity conv factor
  double getDiffConvFactor(void) const { return myDiffConvFactor; }
  /// Returns the multiplier for VEL2 commands
  double getVel2Divisor(void) const { return myVel2Divisor; }
  /// Returns the multiplier for the Analog Gyro
  double getGyroScaler(void) const { return myGyroScaler; }
  /// Returns true if the robot has table sensing IR
  bool haveTableSensingIR(void) const { return myTableSensingIR; }
  /// Returns true if the robot's table sensing IR bits are sent in the 4th-byte of the IO packet
  bool haveNewTableSensingIR(void) const { return myNewTableSensingIR; }
  /// Returns true if the robot has front bumpers
  bool haveFrontBumpers(void) const { return myFrontBumpers; }
  /// Returns the number of front bumpers
  int numFrontBumpers(void) const { return myNumFrontBumpers; }
  /// Returns true if the robot has rear bumpers
  bool haveRearBumpers(void) const { return myRearBumpers; }
  /// Returns the number of rear bumpers
  int numRearBumpers(void) const { return myNumRearBumpers; }
  /// Returns the number of IRs
  int getNumIR(void) const { return myNumIR; }
  /// Returns if the IR of the given number is valid
  bool haveIR(int number) const
    {
      if (myIRMap.find(number) != myIRMap.end())
	return true;
      else
	return false;
    }
  /// Returns the X location of the given numbered IR
  int getIRX(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = myIRMap.find(number)) == myIRMap.end())
	return 0;
      if ((it2 = (*it).second.find(IR_X)) == (*it).second.end())

	return 0;
      return (*it2).second;
    }
  /// Returns the Y location of the given numbered IR
  int getIRY(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = myIRMap.find(number)) == myIRMap.end())
	return 0;
      if ((it2 = (*it).second.find(IR_Y)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  int getIRType(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = myIRMap.find(number)) == myIRMap.end())
	return 0;
      if ((it2 = (*it).second.find(IR_TYPE)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  int getIRCycles(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = myIRMap.find(number)) == myIRMap.end())
	return 0;
      if ((it2 = (*it).second.find(IR_CYCLES)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  /// Returns the number of sonar
  int getNumSonar(void) const { return myNumSonar; }
  /// Returns if the sonar of the given number is valid
  bool haveSonar(int number) const
    {
      if (mySonarMap.find(number) != mySonarMap.end())
	return true;
      else
	return false;
    }
  /// Returns the X location of the given numbered sonar disc
  int getSonarX(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = mySonarMap.find(number)) == mySonarMap.end())
	return 0;
      if ((it2 = (*it).second.find(SONAR_X)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  /// Returns the Y location of the given numbered sonar disc
  int getSonarY(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = mySonarMap.find(number)) == mySonarMap.end())
	return 0;
      if ((it2 = (*it).second.find(SONAR_Y)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  /// Returns the heading of the given numbered sonar disc
  int getSonarTh(int number) const
    {
      std::map<int, std::map<int, int> >::const_iterator it;
      std::map<int, int>::const_iterator it2;
      if ((it = mySonarMap.find(number)) == mySonarMap.end())
	return 0;
      if ((it2 = (*it).second.find(SONAR_TH)) == (*it).second.end())
	return 0;
      return (*it2).second;
    }
  /// Returns if the robot has a laser (according to param file)
  /**
     @deprecated
  **/
  bool getLaserPossessed(void) const 
    { 
      ArLog::log(ArLog::Normal, "Something called ArRobotParams::getLaserPossessed, but this is obsolete and doesn't mean anything.");
      return false; 
    }

  /// What type of laser this is
  const char *getLaserType(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserType; 
      else
	return NULL;
    }
  /// What type of port the laser is on
  const char *getLaserPortType(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserPortType; 
      else
	return NULL;
    }
  /// What port the laser is on
  const char *getLaserPort(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserPort; 
      else
	return NULL;
    }
  /// If the laser should be auto connected
  bool getConnectLaser(int laserNumber = 1) const 
    {       
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserAutoConnect; 
      else
	return false;
    }
  /// If the laser is flipped on the robot
  bool getLaserFlipped(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserFlipped; 
      else
	return false;
    }
  /// If the laser power is controlled by the serial port lines
  bool getLaserPowerControlled(int laserNumber = 1) const 
    {       
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserPowerControlled; 
      else
	return false;
    }
  /// The max range to use the laser
  int getLaserMaxRange(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserMaxRange; 
      else
	return 0;
    }
  /// The cumulative buffer size to use for the laser
  int getLaserCumulativeBufferSize(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserCumulativeBufferSize; 
      else
	return 0;
    }
  /// The X location of the laser
  int getLaserX(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserX; 
      else
	return 0;
    }
  /// The Y location of the laser 
  int getLaserY(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserY; 
      else
	return 0;
    }
  /// The rotation of the laser on the robot
  double getLaserTh(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserTh; 
      else
	return 0;
    }
  /// The height of the laser off of the ground (0 means unknown)
  int getLaserZ(int laserNumber = 1) const 
    { 
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserZ; 
      else
	return 0;
    }
  /// Gets the string that is the readings the laser should ignore
  const char *getLaserIgnore(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserIgnore; 
      else
	return NULL;
    }
  /// Gets the string that is the degrees the laser should start on
  const char *getLaserStartDegrees(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserStartDegrees; 
      else
	return NULL;
    }
  /// Gets the string that is the degrees the laser should end on
  const char *getLaserEndDegrees(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserEndDegrees; 
      else
	return NULL;
    }
  /// Gets the string that is choice for the number of degrees the laser should use
  const char *getLaserDegreesChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserDegreesChoice; 
      else
	return NULL;
    }
  /// Gets the string that is choice for the increment the laser should use
  const char *getLaserIncrement(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserIncrement; 
      else
	return NULL;
    }
  /// Gets the string that is choice for increment the laser should use
  const char *getLaserIncrementChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserIncrementChoice; 
      else
	return NULL;
    }
  /// Gets the string that is choice for units the laser should use
  const char *getLaserUnitsChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserUnitsChoice; 
      else
	return NULL;
    }
  /// Gets the string that is choice for reflectorBits the laser should use
  const char *getLaserReflectorBitsChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserReflectorBitsChoice; 
      else
	return NULL;
    }
  /// Gets the string that is choice for starting baud the laser should use
  const char *getLaserStartingBaudChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserStartingBaudChoice; 
      else
	return NULL;
    }
  /// Gets the string that is choice for auto baud the laser should use
  const char *getLaserAutoBaudChoice(int laserNumber = 1) const 
    {
      if (getLaserData(laserNumber) != NULL)
	return getLaserData(laserNumber)->myLaserAutoBaudChoice; 
      else
	return NULL;
    }


  /// Gets whether the VelMax values are settable or not
  bool hasSettableVelMaxes(void) const { return mySettableVelMaxes; }
  /// Gets the max trans vel from param file (0 uses microcontroller param)
  int getTransVelMax(void) const { return myTransVelMax; }
  /// Gets the max rot vel from param file (0 uses microcontroller param)
  int getRotVelMax(void) const { return myRotVelMax; }
  /// Whether the accelerations and decelerations are settable or not
  bool hasSettableAccsDecs(void) const { return mySettableAccsDecs; }
  /// Gets the trans accel from param file (0 uses microcontroller param)
  int getTransAccel(void) const { return myTransAccel; }
  /// Gets the trans decel from param file (0 uses microcontroller param)
  int getTransDecel(void) const { return myTransDecel; }
  /// Gets the rot accel from param file (0 uses microcontroller param)
  int getRotAccel(void) const { return myRotAccel; }
  /// Gets the rot decel from param file (0 uses microcontroller param)
  int getRotDecel(void) const { return myRotDecel; }
  /// Whether we have lateral control or not
  bool hasLatVel(void) const { return myHasLatVel; }
  /// Gets the max lat vel from param file (0 uses microcontroller param)
  int getLatVelMax(void) const { return myTransVelMax; }
  /// Gets the lat accel from param file (0 uses microcontroller param)
  int getLatAccel(void) const { return myTransAccel; }
  /// Gets the lat decel from param file (0 uses microcontroller param)
  int getLatDecel(void) const { return myTransDecel; }
  /// Saves it to the subtype.p in Aria::getDirectory/params
  AREXPORT bool save(void);

  /// The X (forward-back) location of the GPS (antenna) on the robot
  int getGPSX() const { return myGPSX; }
  /// The Y (left-right) location of the GPS (antenna) on the robot
  int getGPSY() const { return myGPSY; }
  /// The Baud rate to use when connecting to the GPS
  int getGPSBaud() const { return myGPSBaud; }
  /// The serial port the GPS is on
  const char *getGPSPort() const { return myGPSPort; }
  const char *getGPSType() const { return myGPSType; }

  // What kind of compass the robot has
  const char *getCompassType() const { return myCompassType; }
  const char *getCompassPort() const { return myCompassPort; }

protected:
  char myClass[1024];
  char mySubClass[1024];
  double myRobotRadius;
  double myRobotDiagonal;
  double myRobotWidth;
  double myRobotLength;
  double myRobotLengthFront;
  double myRobotLengthRear;
  bool myHolonomic;
  int myAbsoluteMaxRVelocity;
  int myAbsoluteMaxVelocity;
  bool myHaveMoveCommand;
  bool myRequestIOPackets;
  bool myRequestEncoderPackets;
  int mySwitchToBaudRate;
  double myAngleConvFactor;
  double myDistConvFactor;
  double myVelConvFactor;
  double myRangeConvFactor;
  double myDiffConvFactor;
  double myVel2Divisor;
  double myGyroScaler;
  bool myTableSensingIR;
  bool myNewTableSensingIR;
  bool myFrontBumpers;
  int myNumFrontBumpers;
  bool myRearBumpers;
  int myNumRearBumpers;

  class LaserData
  {
  public:
    LaserData()
      {
	myLaserType[0] = '\0';
	myLaserPortType[0] = '\0';
	myLaserPort[0] = '\0';
	myLaserAutoConnect = false;
	myLaserFlipped = false;
	myLaserPowerControlled = true;
	myLaserMaxRange = 0;
	myLaserCumulativeBufferSize = 0;
	myLaserX = 0;
	myLaserY = 0;
	myLaserTh = 0.0;
	myLaserZ = 0;
	myLaserIgnore[0] = '\0';
	myLaserStartDegrees[0] = '\0';
	myLaserEndDegrees[0] = '\0';
	myLaserDegreesChoice[0] = '\0';
	myLaserIncrement[0] = '\0';
	myLaserIncrementChoice[0] = '\0';
	myLaserUnitsChoice[0] = '\0';
	myLaserReflectorBitsChoice[0] = '\0';
	myLaserStartingBaudChoice[0] = '\0';
	myLaserAutoBaudChoice[0] = '\0';
      }
    virtual ~LaserData() {}
  
    char myLaserType[256];
    char myLaserPortType[256];
    char myLaserPort[256];
    bool myLaserAutoConnect;
    bool myLaserFlipped;
    bool myLaserPowerControlled;
    unsigned int myLaserMaxRange;
    unsigned int myLaserCumulativeBufferSize;
    int myLaserX;
    int myLaserY;
    double myLaserTh;
    int myLaserZ;
    char myLaserIgnore[256];
    char myLaserStartDegrees[256];
    char myLaserEndDegrees[256];
    char myLaserDegreesChoice[256];
    char myLaserIncrement[256];
    char myLaserIncrementChoice[256];
    char myLaserUnitsChoice[256];
    char myLaserReflectorBitsChoice[256];
    char myLaserStartingBaudChoice[256];
    char myLaserAutoBaudChoice[256];
  };
  std::map<int, LaserData *> myLasers;

  const LaserData *getLaserData(int laserNumber) const
    {
      std::map<int, LaserData *>::const_iterator it;
      if ((it = myLasers.find(laserNumber)) != myLasers.end())
	return (*it).second;
      else
	return NULL;
    }

  LaserData *getLaserData(int laserNumber) 
    {
      std::map<int, LaserData *>::const_iterator it;
      if ((it = myLasers.find(laserNumber)) != myLasers.end())
	return (*it).second;
      else
	return NULL;
    }

  bool mySettableVelMaxes;
  int myTransVelMax;
  int myRotVelMax;
  bool mySettableAccsDecs;
  int myTransAccel;
  int myTransDecel;
  int myRotAccel;
  int myRotDecel;

  bool myHasLatVel;
  int myLatVelMax;
  int myLatAccel;
  int myLatDecel;
  int myAbsoluteMaxLatVelocity;


  // Sonar
  int myNumSonar;
  std::map<int, std::map<int, int> > mySonarMap;
  enum SonarInfo 
  { 
    SONAR_X, 
    SONAR_Y, 
    SONAR_TH
  };
  AREXPORT void internalSetSonar(int num, int x, int y, int th);
  AREXPORT bool parseSonarUnit(ArArgumentBuilder *builder);
  AREXPORT const std::list<ArArgumentBuilder *> *getSonarUnits(void);
  std::list<ArArgumentBuilder *> myGetSonarUnitList;
  ArRetFunctorC<const std::list<ArArgumentBuilder *> *, ArRobotParams> mySonarUnitGetFunctor;
  ArRetFunctor1C<bool, ArRobotParams, ArArgumentBuilder *> mySonarUnitSetFunctor;

  // IRs
  int myNumIR;
  std::map<int, std::map<int, int> > myIRMap;
  enum IRInfo 
  { 
    IR_X, 
    IR_Y,
    IR_TYPE,
    IR_CYCLES
  };
  AREXPORT void internalSetIR(int num, int type, int cycles, int x, int y);
  AREXPORT bool parseIRUnit(ArArgumentBuilder *builder);
  AREXPORT const std::list<ArArgumentBuilder *> *getIRUnits(void);
  std::list<ArArgumentBuilder *> myGetIRUnitList;
  ArRetFunctorC<const std::list<ArArgumentBuilder *> *, ArRobotParams> myIRUnitGetFunctor;
  ArRetFunctor1C<bool, ArRobotParams, ArArgumentBuilder *> myIRUnitSetFunctor;

  // GPS
  bool myGPSPossessed;
  int myGPSX;
  int myGPSY;
  char myGPSPort[256];
  char myGPSType[256];
  int myGPSBaud;

  // Compass
  char myCompassType[256];
  char myCompassPort[256];

};

#endif // ARROBOTPARAMS_H
