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
#include "ArExport.h"


#include "ariaOSDef.h"
#include "ArRobotParams.h"
#include "ariaInternal.h"

AREXPORT ArRobotParams::ArRobotParams() :
  ArConfig(NULL, true),
  mySonarUnitGetFunctor(this, &ArRobotParams::getSonarUnits),
  mySonarUnitSetFunctor(this, &ArRobotParams::parseSonarUnit),
  myIRUnitGetFunctor(this, &ArRobotParams::getIRUnits),
  myIRUnitSetFunctor(this, &ArRobotParams::parseIRUnit)
{
  sprintf(myClass, "Pioneer");
  mySubClass[0] = '\0';
  myRobotRadius = 250;
  myRobotDiagonal = 120;
  myRobotWidth = 400;
  myRobotLength = 500; 
  myRobotLengthFront = 0; 
  myRobotLengthRear = 0; 
  myHolonomic = true;
  myAbsoluteMaxVelocity = 0;
  myAbsoluteMaxRVelocity = 0;
  myHaveMoveCommand = true;
  myAngleConvFactor = 0.001534;
  myDistConvFactor = 1.0;
  myVelConvFactor = 1.0;
  myRangeConvFactor = 1.0;
  myVel2Divisor = 20;
  myGyroScaler = 1.626;
  myNumSonar = 0;
  myTableSensingIR = false;
  myNewTableSensingIR = false;
  myFrontBumpers = false;
  myNumFrontBumpers = 5;
  myRearBumpers = false;
  myNumRearBumpers = 5;

  myNumSonar = 0;
  mySonarMap.clear();

  myNumIR = 0;
  myIRMap.clear();


  
  myRequestIOPackets = false;
  myRequestEncoderPackets = false;
  mySwitchToBaudRate = 38400;

  mySettableVelMaxes = true;
  myTransVelMax = 0;
  myRotVelMax = 0;

  mySettableAccsDecs = true;
  myTransAccel = 0;
  myTransDecel = 0;
  myRotAccel = 0;
  myRotDecel = 0;

  myHasLatVel = false;
  myLatVelMax = 0;
  myLatAccel = 0;
  myLatDecel = 0;
  myAbsoluteMaxLatVelocity = 0;

  myGPSX = 0;
  myGPSY = 0;
  strcpy(myGPSPort, "COM2");
  strcpy(myGPSType, "standard");
  myGPSBaud = 9600;

  strcpy(myCompassType, "robot");
  strcpy(myCompassPort, "");

  addComment("Robot parameter file");
//  addComment("");
  //addComment("General settings");
  std::string section;
  section = "General settings";
  addParam(ArConfigArg("Class", myClass, "general type of robot", 
		 sizeof(myClass)), section.c_str(), ArPriority::TRIVIAL);
  addParam(ArConfigArg("Subclass", mySubClass, "specific type of robot", 
		       sizeof(mySubClass)), section.c_str(), 
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("RobotRadius", &myRobotRadius, "radius in mm"), 
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("RobotDiagonal", &myRobotDiagonal, 
		 "half-height to diagonal of octagon"), "General settings",
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("RobotWidth", &myRobotWidth, "width in mm"), 
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("RobotLength", &myRobotLength, "length in mm of the whole robot"),
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("RobotLengthFront", &myRobotLengthFront, "length in mm to the front of the robot (if this is 0 (or non existant) this value will be set to half of RobotLength)"),
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("RobotLengthRear", &myRobotLengthRear, "length in mm to the rear of the robot (if this is 0 (or non existant) this value will be set to half of RobotLength)"), 
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("Holonomic", &myHolonomic, "turns in own radius"), 
	   section.c_str(), ArPriority::TRIVIAL);
  addParam(ArConfigArg("MaxRVelocity", &myAbsoluteMaxRVelocity, 
		       "absolute maximum degrees / sec"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("MaxVelocity", &myAbsoluteMaxVelocity, 
		 "absolute maximum mm / sec"), section.c_str(), 
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("MaxLatVelocity", &myAbsoluteMaxLatVelocity, 
		 "absolute lateral maximum mm / sec"), section.c_str(), 
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("HasMoveCommand", &myHaveMoveCommand, 
		 "has built in move command"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("RequestIOPackets", &myRequestIOPackets,
		 "automatically request IO packets"), section.c_str(),
	   ArPriority::NORMAL);
  addParam(ArConfigArg("RequestEncoderPackets", &myRequestEncoderPackets,
		       "automatically request encoder packets"), 
	   section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("SwitchToBaudRate", &mySwitchToBaudRate, 
		 "switch to this baud if non-0 and supported on robot"), 
	   section.c_str(), ArPriority::IMPORTANT);
  
  section = "Conversion factors";
  addParam(ArConfigArg("AngleConvFactor", &myAngleConvFactor,
		     "radians per angular unit (2PI/4096)"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("DistConvFactor", &myDistConvFactor,
		       "multiplier to mm from robot units"), section.c_str(),
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("VelConvFactor", &myVelConvFactor,
		     "multiplier to mm/sec from robot units"), 
	   section.c_str(),
	   ArPriority::NORMAL);
  addParam(ArConfigArg("RangeConvFactor", &myRangeConvFactor, 
		       "multiplier to mm from sonar units"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("DiffConvFactor", &myDiffConvFactor, 
		     "ratio of angular velocity to wheel velocity (unused in newer firmware that calculates and returns this)"), 
	   section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("Vel2Divisor", &myVel2Divisor, 
		       "divisor for VEL2 commands"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("GyroScaler", &myGyroScaler, 
		     "Scaling factor for gyro readings"), section.c_str(),
	   ArPriority::IMPORTANT);

  section = "Accessories the robot has";
  addParam(ArConfigArg("TableSensingIR", &myTableSensingIR,
		       "if robot has upwards facing table sensing IR"), 
	   section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("NewTableSensingIR", &myNewTableSensingIR,
		 "if table sensing IR are sent in IO packet"), 
	   section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("FrontBumpers", &myFrontBumpers, 
		 "if robot has a front bump ring"), section.c_str(),
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("NumFrontBumpers", &myNumFrontBumpers,
		     "number of front bumpers on the robot"), 
	   section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("RearBumpers", &myRearBumpers,
		       "if the robot has a rear bump ring"), section.c_str(),
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("NumRearBumpers", &myNumRearBumpers,
		       "number of rear bumpers on the robot"), section.c_str(),
	   ArPriority::TRIVIAL);

  section = "Sonar parameters";
  addParam(ArConfigArg("SonarNum", &myNumSonar, 
		     "number of sonar on the robot"), section.c_str(),
	   ArPriority::NORMAL);
  addParam(ArConfigArg("SonarUnit", &mySonarUnitSetFunctor, 
		     &mySonarUnitGetFunctor,
		     "SonarUnit <sonarNumber> <x position, mm> <y position, mm> <heading of disc, degrees>"), section.c_str(), ArPriority::TRIVIAL);

  section = "IR parameters";
  addParam(ArConfigArg("IRNum", &myNumIR, "number of IRs on the robot"), section.c_str(), ArPriority::NORMAL);
   addParam(ArConfigArg("IRUnit", &myIRUnitSetFunctor, &myIRUnitGetFunctor,
			"IRUnit <IR Number> <IR Type> <Persistance, cycles> <x position, mm> <y position, mm>"), 
	    section.c_str(), ArPriority::TRIVIAL);


    
  section = "Movement control parameters";
  setSectionComment(section.c_str(), "if these are 0 the parameters from robot flash will be used, otherwise these values will be used");
  addParam(ArConfigArg("SettableVelMaxes", &mySettableVelMaxes, "if TransVelMax and RotVelMax can be set"), section.c_str(),
	   ArPriority::TRIVIAL);
  addParam(ArConfigArg("TransVelMax", &myTransVelMax, "maximum desired translational velocity for the robot"), section.c_str(), 
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("RotVelMax", &myRotVelMax, "maximum desired rotational velocity for the robot"), section.c_str(),
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("SettableAccsDecs", &mySettableAccsDecs, "if the accel and decel parameters can be set"), section.c_str(), ArPriority::TRIVIAL);
  addParam(ArConfigArg("TransAccel", &myTransAccel, "translational acceleration"), 
	   section.c_str(), ArPriority::IMPORTANT);
  addParam(ArConfigArg("TransDecel", &myTransDecel, "translational deceleration"), 
	   section.c_str(), ArPriority::IMPORTANT);
  addParam(ArConfigArg("RotAccel", &myRotAccel, "rotational acceleration"), 
	   section.c_str());
  addParam(ArConfigArg("RotDecel", &myRotDecel, "rotational deceleration"),
	   section.c_str(), ArPriority::IMPORTANT);

  addParam(ArConfigArg("HasLatVel", &myHasLatVel, "if the robot has lateral velocity"), section.c_str(), ArPriority::TRIVIAL);

  addParam(ArConfigArg("LatVelMax", &myLatVelMax, "maximum desired lateral velocity for the robot"), section.c_str(), 
	   ArPriority::IMPORTANT);
  addParam(ArConfigArg("LatAccel", &myLatAccel, "lateral acceleration"), 
	   section.c_str(), ArPriority::IMPORTANT);
  addParam(ArConfigArg("LatDecel", &myLatDecel, "lateral deceleration"), 
	   section.c_str(), ArPriority::IMPORTANT);

  section = "GPS parameters";
  addParam(ArConfigArg("GPSPX", &myGPSX, "x location of gps receiver antenna on robot, mm"), section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("GPSPY", &myGPSY, "y location of gps receiver antenna on robot, mm"), section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("GPSType", myGPSType, "type of gps receiver (trimble, novatel, standard)", sizeof(myGPSType)), section.c_str(), ArPriority::IMPORTANT);
  addParam(ArConfigArg("GPSPort", myGPSPort, "port the gps is on", sizeof(myGPSPort)), section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("GPSBaud", &myGPSBaud, "gps baud rate (9600, 19200, 38400, etc.)"), section.c_str(), ArPriority::NORMAL);

  section = "Compass parameters";
  addParam(ArConfigArg("CompassType", myCompassType, "type of compass: robot (typical configuration), or serialTCM (computer serial port)", sizeof(myCompassType)), section.c_str(), ArPriority::NORMAL);
  addParam(ArConfigArg("CompassPort", myCompassPort, "serial port name, if CompassType is serialTCM", sizeof(myCompassPort)), section.c_str(), ArPriority::NORMAL);

  int i;
  for (i = 1; i <= Aria::getMaxNumLasers(); i++)
  {
    LaserData *laserData = new LaserData;

    if (i == 1)
    {
      section = "Laser parameters";
    }
    else
    {
      char buf[1024];
      sprintf(buf, "Laser %d parameters", i);
      section = buf;
    }

    myLasers[i] = laserData;
    addParam(ArConfigArg("LaserType", laserData->myLaserType, 
			 "type of laser", 
			 sizeof(laserData->myLaserType)), section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserPortType", laserData->myLaserPortType, 
			 "type of port the laser is on", 
			 sizeof(laserData->myLaserPort)), section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserPort", laserData->myLaserPort, 
			 "port the laser is on", 
			 sizeof(laserData->myLaserPort)), section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserAutoConnect", &laserData->myLaserAutoConnect,
			 "if the laser connector should autoconnect this laser or not"), section.c_str(),
	     ArPriority::NORMAL);

    addParam(ArConfigArg("LaserFlipped", &laserData->myLaserFlipped,
			 "if the laser is upside-down or not"), section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserPowerControlled", 
			 &laserData->myLaserPowerControlled,
			 "if the power to the laser is controlled by serial"), 
	     section.c_str(),
	     ArPriority::TRIVIAL);
    addParam(ArConfigArg("LaserMaxRange", (int *)&laserData->myLaserMaxRange, 
			 "Max range to use for the laser, 0 to use default (only use if you want to shorten it from the default), mm"), 
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserCumulativeBufferSize", 
			 (int *)&laserData->myLaserCumulativeBufferSize, 
			 "Cumulative buffer size to use for the laser, 0 to use default"), 
			 section.c_str(),
			 ArPriority::NORMAL);
    addParam(ArConfigArg("LaserX", &laserData->myLaserX, 
			 "x location of laser, mm"), 
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserY", &laserData->myLaserY, 
			 "y location of laser, mm"), 
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserTh", &laserData->myLaserTh, 
			 "rotation of laser, deg"), 
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserZ", &laserData->myLaserZ, 
			 "height of the laser off the ground, mm (0 means unknown)"),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserIgnore", laserData->myLaserIgnore, 
			 "Readings within a degree of the listed degrees (separated by a space) will be ignored", sizeof(laserData->myLaserIgnore)), 
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserStartDegrees", laserData->myLaserStartDegrees, 
			 "start degrees for the sensor (leave blank for default, use this to constrain the range) (double)", sizeof(laserData->myLaserStartDegrees)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserEndDegrees", laserData->myLaserEndDegrees, 
			 "start degrees for the sensor (leave blank for default, use this to constrain the range) (double)", sizeof(laserData->myLaserEndDegrees)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserDegreesChoice", laserData->myLaserDegreesChoice, 
			 "degrees choice for the sensor (leave blank for default, use this to constrain the range)", sizeof(laserData->myLaserDegreesChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserIncrement", laserData->myLaserIncrement, 
			 "Increment for the sensor  (leave blank for default, use this to have a custom increment) (double)", sizeof(laserData->myLaserIncrement)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserIncrementChoice", laserData->myLaserIncrementChoice, 
			 "Increment for the sensor  (leave blank for default, use this to have a larger increment)", sizeof(laserData->myLaserIncrementChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserUnitsChoice", laserData->myLaserUnitsChoice, 
			 "Units for the sensor  (leave blank for default, use this to have a larger units)", sizeof(laserData->myLaserUnitsChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserReflectorBitsChoice", 
			 laserData->myLaserReflectorBitsChoice, 
			 "ReflectorBits for the sensor  (leave blank for default, use this to have a larger units)", 
			 sizeof(laserData->myLaserReflectorBitsChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserStartingBaudChoice", 
			 laserData->myLaserStartingBaudChoice, 
			 "StartingBaud for the sensor  (leave blank for default, use this to have a larger StartingBaud)", 
			 sizeof(laserData->myLaserStartingBaudChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
    addParam(ArConfigArg("LaserAutoBaudChoice", 
			 laserData->myLaserAutoBaudChoice, 
			 "AutoBaud for the sensor  (leave blank for default, use this to have a larger units)", 
			 sizeof(laserData->myLaserAutoBaudChoice)),
	     section.c_str(),
	     ArPriority::NORMAL);
  }
  

}

AREXPORT ArRobotParams::~ArRobotParams()
{

}



AREXPORT bool ArRobotParams::parseSonarUnit(ArArgumentBuilder *builder)
{
  if (builder->getArgc() != 4 || !builder->isArgInt(0) || 
      !builder->isArgInt(1) || !builder->isArgInt(2) ||
      !builder->isArgInt(3))
  {
    ArLog::log(ArLog::Terse, "ArRobotParams: SonarUnit parameters invalid");
    return false;
  }
  mySonarMap[builder->getArgInt(0)][SONAR_X] = builder->getArgInt(1);
  mySonarMap[builder->getArgInt(0)][SONAR_Y] = builder->getArgInt(2);
  mySonarMap[builder->getArgInt(0)][SONAR_TH] = builder->getArgInt(3);
  return true;
}


AREXPORT const std::list<ArArgumentBuilder *> *ArRobotParams::getSonarUnits(void)
{
  std::map<int, std::map<int, int> >::iterator it;
  int num, x, y, th;
  ArArgumentBuilder *builder;

  for (it = mySonarMap.begin(); it != mySonarMap.end(); it++)
  {
    num = (*it).first;
    x = (*it).second[SONAR_X];
    y = (*it).second[SONAR_Y];
    th = (*it).second[SONAR_TH];
    builder = new ArArgumentBuilder;
    builder->add("%d %d %d %d", num, x, y, th);
    myGetSonarUnitList.push_back(builder);
  }
  return &myGetSonarUnitList;
}

AREXPORT void ArRobotParams::internalSetSonar(int num, int x, 
					      int y, int th)
{
  mySonarMap[num][SONAR_X] = x;
  mySonarMap[num][SONAR_Y] = y;
  mySonarMap[num][SONAR_TH] = th;
}

AREXPORT bool ArRobotParams::parseIRUnit(ArArgumentBuilder *builder)
{
  if (builder->getArgc() != 5 || !builder->isArgInt(0) || 
      !builder->isArgInt(1) || !builder->isArgInt(2) || 
      !builder->isArgInt(3) || !builder->isArgInt(4))
  {
    ArLog::log(ArLog::Terse, "ArRobotParams: IRUnit parameters invalid");
    return false;
  }
  myIRMap[builder->getArgInt(0)][IR_TYPE] = builder->getArgInt(1);
  myIRMap[builder->getArgInt(0)][IR_CYCLES] = builder->getArgInt(2);
  myIRMap[builder->getArgInt(0)][IR_X] = builder->getArgInt(3);
  myIRMap[builder->getArgInt(0)][IR_Y] = builder->getArgInt(4);
  return true;
}

AREXPORT const std::list<ArArgumentBuilder *> *ArRobotParams::getIRUnits(void)
{
  std::map<int, std::map<int, int> >::iterator it;
  int num, type, cycles,  x, y;
  ArArgumentBuilder *builder;

  for (it = myIRMap.begin(); it != myIRMap.end(); it++)
  {
    num = (*it).first;
    type = (*it).second[IR_TYPE];
    cycles = (*it).second[IR_CYCLES];
    x = (*it).second[IR_X];
    y = (*it).second[IR_Y];
    builder = new ArArgumentBuilder;
    builder->add("%d %d %d %d %d", num, type, cycles, x, y);
    myGetIRUnitList.push_back(builder);
  }
  return &myGetIRUnitList;
}

AREXPORT void ArRobotParams::internalSetIR(int num, int type, int cycles, int x, int y)
{
  myIRMap[num][IR_TYPE] = type;
  myIRMap[num][IR_CYCLES] = cycles;
  myIRMap[num][IR_X] = x;
  myIRMap[num][IR_Y] = y;
}

AREXPORT bool ArRobotParams::save(void)
{
  char buf[10000];
  sprintf(buf, "%sparams/", Aria::getDirectory());
  setBaseDirectory(buf);
  sprintf(buf, "%s.p", getSubClassName());
  return writeFile(buf, false, NULL, false);
}
