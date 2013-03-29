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
#include <stdarg.h>

#include "ArExport.h"
#include "ariaOSDef.h"
#include "ArLaserLogger.h"
#include "ArRobot.h"
#include "ArLaser.h"
#include "ArJoyHandler.h"
#include "ArRobotJoyHandler.h"
#include "ariaInternal.h"


/** @page LaserLogFileFormat Laser Scan Log File Format 
 *
 *  A log of raw laser and robot data can be created using an ArLaserLogger
 *  object and driving the robot manually with a joystick or other means. 
 *  This log file can then be futher processed, for example by loading it into
 *  Mapper3 or MobilePlanner, which corrects errors and creates a more accurate
 *  map suitable for navigation and localization.
 *  Laser log file names conventionally end in ".2d" and are sometimes referred
 *  to as ".2d files" or "2d files".
 *
 *  The log file is a text file, and can be opened in any text editor. The
 *  format is as follows. First, general information about the scan appears:
 *
 *  The file starts with the word "LaserOdometryLog", followed on each line by
 *  either a comment, or a tag and a space separated list of data values.
 *  The tag that starts a line of data is followed by a colon character
 *  (<code>:</code>). A comment line 
 *  starts with a <code>#</code> character, and should be ignored.
 *  A program should also ignore any lines that start with tags not described here.
 *
 *  <pre>
 *  LaserOdometryLog
 *  \#Created by ARIA's %ArLaserLogger
 *  version: 3
 *  useEncoderPose: <i>1|0</i>
 *  sick1pose: <i>X</i> <i>Y</i> <i>Theta</i>
 *  sick1conf: <i>LeftFOV</i> <i>RightFOV</i> <i>Samples</i> 
 *  locationTypes: <i>robot</i> <i>robotGlobal</i> <i>robotRaw</i>
 *  </pre>
 *
 *  If <code>useEncoderPose:</code> is followed by <code>1</code>, then only the
 *  robot's encoder-based position was used when scanning. If <code>0</code>,
 *  then other sources such as gyroscopic correction may have been used.
 *
 *  <code>sick1Pose:</code> is followed by the position of the laser device in
 *  the horizontal plane, relative to the center of the robot. This provides an
 *  offset from laser readings' origin to robot position.  <i>X</i> is the
 *  forward-back position (positive forward, negative backward), <i>Y</i> is the
 *  left-right position (positive right, negative left), <i>Theta</i> is the
 *  angle offset (counter-clockwise, usually it is 0).  <code>sick1conf:</code>
 *  is followed by some operating parameters of the laser device itself. <i>Left
 *  FOV</i> and <i>Right FOV</i> indicate the total field of view or angle of
 *  sweep of the laser in degrees (typically they are equivalent in magnitude, since the
 *  field of view of the SICK LMS-200 is always centered).  <i>Samples</i> is
 *  the number of samples taken per sweep within that field of view. The
 *  <code>locationTypes</code> line, if present, lists what kinds of robot position data will be 
 *  available in the log. Zero or more of the possible types may be listed.
 *  (If <code>locationTypes</code> is not present, you may assume that
 *  <code>robot</code> is available, or just check for all types when parsing
 *  the file.)
 *
 *  If you are generating a laser scan log file with software other than
 *  ArSickLogger, then you may replace the message in the comment 
 *  in the second line, for example: <code>\#Created by my custom laser scan
 *  logger</code>.  This comment is for informational purposes only.
 *
 *  It is also possible for other metadata to appear following this initial
 *  block, with different initial tags.  You can ignore these.
 *
 *
 *  Then, as a scan is being recorded, the following lines are written
 *  when triggered by the robot having moved or turned by a certain amount:
 *
 *  <pre>
 *  scan1Id: <i>N</i>
 *  time: <i>t.tt</i>
 *  velocities: <i>Vel</i> <i>RotVel</i> <i>LatVel</i>
 *  robot: <i>X</i> <i>Y</i> <i>Theta</i>
 *  ...
 *  scan1: <i>readings...</i>
 *  </pre>
 *
 *  Where <i>t.tt</i> is the time in seconds since the start of scanning when
 *  this block was added to the scan log;
 *  <i>X</i>, <i>Y</i> and <i>Theta</i> is the position of the robot and
 *  <i>Vel</i> and <i>RotVel</i> are the velocities when these laser
 *  readings were taken;
 *  <i>readings...</i> is a list of space separated
 *  point pairs, a pair for the last point detected by each reading in the last 
 *  sweep of the laser. Each pair defines a point relative to the robot,
 *  where a reading detected an obstacle.  (Note, In old versions of the log format,
 *  <code>sick1</code> was used instead of <code>scan1</code>.)
 *
 *  In addition to <code>robot:</code>, 
 *  additional lines providing other measures of robot position may be present
 *  (these will be listed in the <code>locationTypes:</code> header.)
 *  <code>robot:</code> provides the encoder pose, <code>robotGlobal:</code>
 *  is a corrected global pose, 
 *
 *  If use of special SICK laser reflectors is enabled (rare), then the following line
 *  is written before the <code>scan1:</code> line:
 *  
 *  <pre>
 *  reflector1: <i>levels...</i>
 *  </pre>
 *
 *  Where <i>levels...</i> is a reflectance value for each reading in the
 *  <code>scan1:</code> line to follow.
 *
 *
 *  If during the run, the 'g' key or the second joystick button is pressed, then the following
 *  lines are added:
 *
 *  <pre>
 *  time: <i>t.tt</i>
 *  robot: <i>X</i> <i>Y</i> <i>Theta</i> <i>Vel</i> <i>RotVel</i>
 *  cairn: GoalWithHeading "" ICON_GOALWITHHEADING "goal<i>N</i>"
 *  </pre>
 *
 *
 *  Where the values after <code>time:</code>, <code>\#rawRobot:</code> and
 *  <code>robot:</code> are as above, 
 *  and <i>N</i> is incremented with each goal (i.e. goal0, goal1, goal2,
 *  etc.).  This goal will be added to the final map at the position of the
 *  robot to define a goal or other point of interest in the map.
 *
 */

/**
 * see @ref LaserLogFileFormat
   Make sure you have called ArSick::configure() or
   ArSick::configureShort() on the ArSick object used when creating an instance of this class.

   @param robot The robot to attach to

   @param laser the laser to log readings from. It must be
   initialized/configured/connected already.

   @param distDiff the distance traveled at which to take a new reading

   @param degDiff the degrees turned at which to take a new reading

   @param fileName the file name in which to put the log 

   @param addGoals whether to add goals automatically or... if true
   then the sick logger puts hooks into places it needs this to
   happen, into any keyhandler thats around (for a keypress of G), it
   pays attention to the flag bit of the robot, and it puts in a
   button press callback for the joyhandler passed in (if any)
**/
AREXPORT ArLaserLogger::ArLaserLogger(
	ArRobot *robot, ArLaser *laser, 
	double distDiff, double degDiff, 
	const char *fileName, bool addGoals, ArJoyHandler *joyHandler,
	const char *baseDirectory, bool useReflectorValues,
	ArRobotJoyHandler *robotJoyHandler,
	const std::map<std::string, ArRetFunctor2<int, ArTime, ArPose *> *, 
	ArStrCaseCmpOp> *extraLocationData) :
  mySectors(18), 
  myTaskCB(this, &ArLaserLogger::robotTask),
  myGoalKeyCB(this, &ArLaserLogger::goalKeyCallback), 
  myLoopPacketHandlerCB(this, &ArLaserLogger::loopPacketHandler)
{
  ArKeyHandler *keyHandler;
  

  myOldReadings = false;
  myNewReadings = true;
  myUseReflectorValues = useReflectorValues;
  myWrote = false;
  myRobot = robot;
  myLaser = laser;
  if (baseDirectory != NULL && strlen(baseDirectory) > 0)
    myBaseDirectory = baseDirectory;
  else
    myBaseDirectory = "";
  std::string realFileName;
  if (fileName[0] == '/' || fileName[0] == '\\')
  {
    realFileName = fileName;
  }
  else
  {
    realFileName = myBaseDirectory;
    realFileName += fileName;
  }
  myFileName = realFileName;

  if (myRobot->getEncoderCorrectionCallback() != NULL)
    myIncludeRawEncoderPose = true;
  else
    myIncludeRawEncoderPose = false;

  if (extraLocationData != NULL)
    myExtraLocationData = *extraLocationData;

  myFile = ArUtil::fopen(realFileName.c_str(), "w+");

  /*
  double deg, incr;

  ArSick::Degrees degrees;
  ArSick::Increment increment;
  degrees = myLaser->getDegrees();
  increment = myLaser->getIncrement();

  if (degrees == ArSick::DEGREES180)
    deg = 180;
  else 
    deg = 100;
  if (increment == ArSick::INCREMENT_ONE)
    incr = 1;
  else
    incr = .5;
  */

  const std::list<ArSensorReading *> *readings;

  readings = myLaser->getRawReadings();

  double minAngle = HUGE_VAL;
  double maxAngle = -HUGE_VAL;
  double firstAngle = HUGE_VAL;
  double lastAngle = HUGE_VAL;

  if (!readings->empty())
  {
    firstAngle = readings->front()->getSensorTh();
    lastAngle = readings->back()->getSensorTh();

    if (firstAngle < lastAngle)
      myFlipped = true;
    else
      myFlipped = false;

    minAngle = ArUtil::findMin(minAngle, firstAngle);
    minAngle = ArUtil::findMin(minAngle, lastAngle);

    maxAngle = ArUtil::findMax(maxAngle, firstAngle);
    maxAngle = ArUtil::findMax(maxAngle, lastAngle);
  }
  else
  {
    ArLog::log(ArLog::Normal, "ArLaserLogger: Apparently there are no readings...");
  }
      
  if (myFile != NULL)
  {
    //const ArRobotParams *params;
    //params = robot->getRobotParams();
    fprintf(myFile, "LaserOdometryLog\n");
    fprintf(myFile, "#Created by ARIA's ArLaserLogger\n");
    fprintf(myFile, "version: 3\n");
    //fprintf(myFile, "sick1pose: %d %d %.2f\n", params->getLaserX(), 
    //params->getLaserY(), params->getLaserTh());
    /*
    fprintf(myFile, "sick1pose: %.0f %.0f %.2f\n", 
	    myLaser->getSensorPositionX(),
	    myLaser->getSensorPositionY(),
	    myLaser->getSensorPositionTh());
    fprintf(myFile, "sick1conf: %d %d %d\n", 
	    ArMath::roundInt(0.0 - deg / 2.0),
	    ArMath::roundInt(deg / 2.0), ArMath::roundInt(deg / incr + 1.0));
    */
    fprintf(myFile, "sick1pose: %.0f %.0f %.2f\n", 
	    myLaser->getSensorPositionX(),
	    myLaser->getSensorPositionY(),
	    myLaser->getSensorPositionTh());
    fprintf(myFile, "sick1conf: %d %d %d\n", 
	    ArMath::roundInt(minAngle),
	    ArMath::roundInt(maxAngle), 
	    readings->size());
    std::string available;
    available = "robot robotGlobal";
    if (myIncludeRawEncoderPose)
      available += " robotRaw";

    std::map<std::string, ArRetFunctor2<int, ArTime, ArPose *> *, ArStrCaseCmpOp>::iterator it;
    for (it = myExtraLocationData.begin(); 
	 it != myExtraLocationData.end(); 
	 it++)
      available += " " + (*it).first;

    fprintf(myFile, "locationTypes: %s\n", available.c_str());
  }
  else
  {
    ArLog::log(ArLog::Terse, "ArLaserLogger cannot write to file %s", 
	       myFileName.c_str());
    return;
  }

  myDistDiff = distDiff;
  myDegDiff = degDiff;
  myFirstTaken = false;
  myScanNumber = 0;
  myLastVel = 0;
  myStartTime.setToNow();
  myRobot->addUserTask("Sick Logger", 1, &myTaskCB);

  char uCFileName[21];
  snprintf(uCFileName, 18, "map %s", fileName);
  uCFileName[18] = '\0';
  myRobot->comStr(94, uCFileName);

  myLoopPacketHandlerCB.setName("ArLaserLogger");
  myRobot->addPacketHandler(&myLoopPacketHandlerCB, ArListPos::FIRST);

  myAddGoals = addGoals;
  myJoyHandler = joyHandler;
  myRobotJoyHandler = robotJoyHandler;
  myTakeReadingExplicit = false;
  myAddGoalExplicit = false;
  myAddGoalKeyboard = false;
  myLastAddGoalKeyboard = false;
  myLastJoyButton = false;
  myLastRobotJoyButton = false;
  myFirstGoalTaken = false;
  myNumGoal = 1;
  myLastLoops = 0;
  // only add goals from the keyboard if there's already a keyboard handler
  if (myAddGoals && (keyHandler = Aria::getKeyHandler()) != NULL)
  {
    // now that we have a key handler, add our keys as callbacks, print out big
    // warning messages if they fail
    if (!keyHandler->addKeyHandler('g', &myGoalKeyCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for g, sick logger goal handling will not work correctly.");
    if (!keyHandler->addKeyHandler('G', &myGoalKeyCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for g, sick logger goal handling will not work correctly.");
  }

}

AREXPORT ArLaserLogger::~ArLaserLogger()
{
  myRobot->remUserTask(&myTaskCB);
  myRobot->remPacketHandler(&myLoopPacketHandlerCB);
  myRobot->comStr(94, "");
  if (myFile != NULL)
  {
    fprintf(myFile, "# End of log\n");
    fclose(myFile);
  }
}
  
AREXPORT bool ArLaserLogger::loopPacketHandler(ArRobotPacket *packet)
{
  unsigned char loops;
  if (packet->getID() != 0x96)
    return false;
  loops = packet->bufToUByte();
  unsigned char bit;
  int num;
  if (loops != myLastLoops)
  {
    for (bit = 1, num = 1; num <= 8; bit *= 2, num++)
    {
      if ((loops & bit) && !(myLastLoops & bit))
      {
	addTagToLog("loop: start %d", num);
	ArLog::log(ArLog::Normal, "Starting loop %d", num);
      }
      else if (!(loops & bit) && (myLastLoops & bit))
      {
	addTagToLog("loop: stop %d", num);
	ArLog::log(ArLog::Normal, "Stopping loop %d", num);
      }
    }
  }
  myLastLoops = loops;
  // we return this as false so multiple sick loggers can snag the
  // data... the ArServerHandlerMapping will return true for it so
  // that it doesn't get logged
  return false;
}

/**
   The robot MUST be locked before you call this function, so that
   this function is not adding to a list as the robotTask is using it.
   
   This function takes the given tag and puts it into the log file
   along with a tag as to where the robot was and when in the mapping
   it was
**/
AREXPORT void ArLaserLogger::addTagToLogPlain(const char *str)
{
  myTags.push_back(str);
}

#ifndef SWIG
/**
   The robot MUST be locked before you call this function, so that
   this function is not adding to a list as the robotTask is using it.
   
   This function takes the given tag and puts it into the log file
   along with a tag as to where the robot was and when in the mapping
   it was

   @swigomit
   @sa addTagToLogPlain()
**/
AREXPORT void ArLaserLogger::addTagToLog(const char *str, ...)
{
  char buf[2048];
  va_list ptr;
  va_start(ptr, str);
  vsprintf(buf, str, ptr);
  addTagToLogPlain(buf);
  va_end(ptr);
}
#endif


/**
   The robot MUST be locked before you call this function, so that
   this function is not adding to a list as the robotTask is using it.
   
   This function takes the given tag and puts it into the log file by
   itself
**/

AREXPORT void ArLaserLogger::addInfoToLogPlain(const char *str)
{
  myInfos.push_back(str);
}

/**
   The robot MUST be locked before you call this function, so that
   this function is not adding to a list as the robotTask is using it.
   
   This function takes the given tag and puts it into the log file by
   itself
**/
AREXPORT void ArLaserLogger::addInfoToLog(const char *str, ...)
{
  char buf[2048];
  va_list ptr;
  va_start(ptr, str);
  vsprintf(buf, str, ptr);
  addInfoToLogPlain(buf);
  va_end(ptr);
}

void ArLaserLogger::goalKeyCallback(void)
{
  myAddGoalKeyboard = true;
}

void ArLaserLogger::internalAddGoal(void)
{
  bool joyButton;
  bool robotJoyButton;

  // this check is for if we're not adding goals return... but if
  // we're not adding goals and one was requested explicitly then add
  // that one
  if (!myAddGoals && !myAddGoalExplicit) 
    return;
  
  if (myJoyHandler != NULL)
    joyButton = (myJoyHandler->getButton(2) || 
		   myJoyHandler->getButton(3) || 
		   myJoyHandler->getButton(4));
  else
    joyButton = (myRobot->getFlags() & ArUtil::BIT9);
  
  if (myRobotJoyHandler != NULL)
    robotJoyButton = myRobotJoyHandler->getButton2();
  else
    robotJoyButton = false;

  // see if we want to add a goal... note that if the button is pushed
  // it must have been unpushed at one point to cause the goal to
  // trigger
  if (myRobot->isConnected() && 
      (myAddGoalExplicit ||
       (myAddGoalKeyboard && !myLastAddGoalKeyboard) ||
       (joyButton && !myLastJoyButton) ||
       (robotJoyButton && !myLastRobotJoyButton)))
  {
    myFirstGoalTaken = true;
    myAddGoalExplicit = false;
    myLastGoalTakenTime.setToNow();
    myLastGoalTakenPose = myRobot->getEncoderPose();
    // call addTagToLog not do it directly so we get additional info
    // needed
    addTagToLog("cairn: GoalWithHeading \"\" ICON_GOALWITHHEADING \"goal%d\"", myNumGoal);
    ArLog::log(ArLog::Normal, "Goal %d taken", myNumGoal);
    myNumGoal++;
  }
  myLastAddGoalKeyboard = myAddGoalKeyboard;
  myLastJoyButton = joyButton;
  myLastRobotJoyButton = robotJoyButton;
      
  // reset this here for if they held the key down a little, so it
  // gets reset and doesn't hit multiple goals
  myAddGoalKeyboard = false;
}

void ArLaserLogger::internalWriteTags(void)
{
  time_t msec;

  // now put the tags into the file
  while (myInfos.size() > 0)
  {
    if (myFile != NULL)
    {
      myWrote = true;
      fprintf(myFile, "%s\n", (*myInfos.begin()).c_str());
    }
    myInfos.pop_front();
  }


  // now put the tags into the file
  while (myTags.size() > 0)
  {
    if (myFile != NULL)
    {
      myWrote = true;
      msec = myStartTime.mSecSince();
      fprintf(myFile, "time: %ld.%ld\n", msec / 1000, msec % 1000);
      internalPrintPos(myRobot->getEncoderPose(), myRobot->getPose(), 
		       myStartTime);
      fprintf(myFile, "%s\n", (*myTags.begin()).c_str());
    }
    myTags.pop_front();
  }
}

void ArLaserLogger::internalTakeReading(void)
{
  const std::list<ArSensorReading *> *readings;
  std::list<ArSensorReading *>::const_iterator it;
  std::list<ArSensorReading *>::const_reverse_iterator rit;
  ArPose encoderPoseTaken;
  ArPose globalPoseTaken;
  ArTime timeTaken;
  time_t msec;
  ArSensorReading *reading;
  bool usingAdjustedReadings;

  // we take readings in any of the following cases if we haven't
  // taken one yet or if we've been explicitly told to take one or if
  // we've gone further than myDistDiff if we've turned more than
  // myDegDiff if we've switched sign on velocity and gone more than
  // 50 mm (so it doesn't oscilate and cause us to trigger)

  if (myRobot->isConnected() && (!myFirstTaken || myTakeReadingExplicit || 
      myLast.findDistanceTo(myRobot->getEncoderPose()) > myDistDiff ||
      fabs(ArMath::subAngle(myLast.getTh(), 
			    myRobot->getEncoderPose().getTh())) > myDegDiff ||
      ((myLastVel < 0 && myRobot->getVel() > 0 ||
	myLastVel > 0 && myRobot->getVel() < 0) && 
       myLast.findDistanceTo(myRobot->getEncoderPose()) > 50)))
  {
    myWrote = true;
    myLaser->lockDevice();
    /// use the adjusted raw readings if we can, otherwise just use
    /// the raw readings like before
    if ((readings = myLaser->getAdjustedRawReadings()) != NULL)
    {
      usingAdjustedReadings = true;
    }
    else
    {
      usingAdjustedReadings = false;
      readings = myLaser->getRawReadings();
    }
    if (readings == NULL || (it = readings->begin()) == readings->end() ||
	myFile == NULL)
    {
      myLaser->unlockDevice();
      return;
    }
    myTakeReadingExplicit = false;
    myScanNumber++;
    if (usingAdjustedReadings)
      ArLog::log(ArLog::Normal, 
		 "Taking adjusted readings from the %d laser values", 
		 readings->size());
    else
      ArLog::log(ArLog::Normal, 
		 "Taking readings from the %d laser values", 
		 readings->size());
    myFirstTaken = true;
    myLast = myRobot->getEncoderPose();
    encoderPoseTaken = (*readings->begin())->getEncoderPoseTaken();
    globalPoseTaken = (*readings->begin())->getPoseTaken();
    timeTaken = (*readings->begin())->getTimeTaken();
    myLastVel = myRobot->getVel();
    msec = myStartTime.mSecSince();
    fprintf(myFile, "scan1Id: %d\n", myScanNumber);
    fprintf(myFile, "time: %ld.%ld\n", msec / 1000, msec % 1000);
    fprintf(myFile, "velocities: %.2f %.2f %.2f\n", 
	    myRobot->getVel(), myRobot->getRotVel(), myRobot->getLatVel());
    internalPrintPos(encoderPoseTaken, globalPoseTaken, timeTaken);

    if (myUseReflectorValues)
    {
      fprintf(myFile, "reflector1: ");
      
      if (!myFlipped) //myLaser->isLaserFlipped())
      {
	// make sure that the list is in increasing order
	for (it = readings->begin(); it != readings->end(); it++)
	{
	  reading = (*it);
	  if (!reading->getIgnoreThisReading())
	    fprintf(myFile, "%d ", reading->getExtraInt());
	  else
	    fprintf(myFile, "0 ");
	}
      }
      else
      {
	for (rit = readings->rbegin(); rit != readings->rend(); rit++)
	{
	  reading = (*rit);
	  if (!reading->getIgnoreThisReading())
	    fprintf(myFile, "%d ", reading->getExtraInt());
	  else
	    fprintf(myFile, "0 ");
	}
      }
      fprintf(myFile, "\n");
    }
    /**
       Note that the the sick1: or scan1: must be the last thing in
       that timestamp, ie that you should put any other data before
       it.
     **/
    if (myOldReadings)
    {
      fprintf(myFile, "sick1: ");
      
      if (!myFlipped) //myLaser->isLaserFlipped())
      {
	// make sure that the list is in increasing order
	for (it = readings->begin(); it != readings->end(); it++)
	{
	  reading = (*it);
	  fprintf(myFile, "%d ", reading->getRange());
	}
      }
      else
      {
	for (rit = readings->rbegin(); rit != readings->rend(); rit++)
	{
	  reading = (*rit);
	  fprintf(myFile, "%d ", reading->getRange());
	}
      }
      fprintf(myFile, "\n");
    }
    if (myNewReadings)
    {
      fprintf(myFile, "scan1: ");
      
      if (!myFlipped) //myLaser->isLaserFlipped())
      {
	// make sure that the list is in increasing order
	for (it = readings->begin(); it != readings->end(); it++)
	{
	  reading = (*it);
	  if (!reading->getIgnoreThisReading())
	    fprintf(myFile, "%.0f %.0f  ", 
		    reading->getLocalX() - myLaser->getSensorPositionX(), 
		    reading->getLocalY() - myLaser->getSensorPositionY());
	  else
	    fprintf(myFile, "0 0  ");
	}
      }
      else
      {
	for (rit = readings->rbegin(); rit != readings->rend(); rit++)
	{
	  reading = (*rit);
	  if (!reading->getIgnoreThisReading())
	    fprintf(myFile, "%.0f %.0f  ", 
		    reading->getLocalX() - myLaser->getSensorPositionX(), 
		    reading->getLocalY() - myLaser->getSensorPositionY());
	  else
	    fprintf(myFile, "0 0  ");
	}
      }
      fprintf(myFile, "\n");
    }
    myLaser->unlockDevice();
  }
}

void ArLaserLogger::internalPrintPos(ArPose encoderPoseTaken, 
				    ArPose globalPoseTaken, ArTime timeTaken)
{
  if (myFile == NULL)
    return;

  fprintf(myFile, "robot: %.0f %.0f %.2f\n", 
	  encoderPoseTaken.getX(), 
	  encoderPoseTaken.getY(), 
	  encoderPoseTaken.getTh());

  fprintf(myFile, "robotGlobal: %.0f %.0f %.2f\n", 
	  globalPoseTaken.getX(), 
	  globalPoseTaken.getY(), 
	  globalPoseTaken.getTh());

  if (myIncludeRawEncoderPose)
  {
    ArPose encoderPose = myRobot->getEncoderPose();
    ArPose rawEncoderPose = myRobot->getRawEncoderPose();
    ArTransform normalToRaw(rawEncoderPose, encoderPose);
    
    ArPose rawPose;
    rawPose = normalToRaw.doInvTransform(encoderPoseTaken);
    fprintf(myFile, "robotRaw: %.0f %.0f %.2f\n", 
	    rawPose.getX(), 
	    rawPose.getY(), 
	    rawPose.getTh());
  }
  
  std::map<std::string, ArRetFunctor2<int, ArTime, ArPose *> *, 
	   ArStrCaseCmpOp>::iterator it;
  for (it = myExtraLocationData.begin(); it != myExtraLocationData.end(); it++)
  {
    ArPose pose;
    int ret;
    if ((ret = (*it).second->invokeR(timeTaken, &pose)) >= 0)
    {
      fprintf(myFile, "%s: %.0f %.0f %.2f\n", 
	      (*it).first.c_str(),
	      pose.getX(), 
	      pose.getY(), 
	      pose.getTh());
    }
    else
    {
      ArLog::log(ArLog::Verbose, "Could not use %s it returned %d",
		 (*it).first.c_str(), ret);
      fprintf(myFile, "%s: \n", 
	      (*it).first.c_str());
    }
  }
}

AREXPORT void ArLaserLogger::robotTask(void)
{

  // call our function to check goals
  internalAddGoal();
  
  // call our function to dump tags
  internalWriteTags();
  
  // call our function to take a reading
  internalTakeReading();

  // now make sure the files all out to disk
  if (myWrote)
  {
    fflush(myFile);
#ifndef WIN32
    fsync(fileno(myFile));
#endif
  }
  myWrote = false;
}

