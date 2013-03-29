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
#include "ArLaserFilter.h"
#include "ArRobot.h"
#include "ArConfig.h"

AREXPORT ArLaserFilter::ArLaserFilter(
	ArLaser *laser, const char *name) :
  ArLaser(laser->getLaserNumber(),
	  name != NULL && name[0] != '\0' ? name : laser->getName(), 
	  laser->getAbsoluteMaxRange(),
	  laser->isLocationDependent(),
	  false),
  myProcessCB(this, &ArLaserFilter::processReadings)
{
  myLaser = laser;

  if (name == NULL || name[0] == '\0')
  {
    std::string filteredName;
    filteredName = "filtered_";
    filteredName += laser->getName();
    laserSetName(filteredName.c_str());
  }

  myRawReadings = new std::list<ArSensorReading *>;

  char buf[1024];
  sprintf(buf, "%sProcessCB", getName());
  myProcessCB.setName(buf);

  myAngleToCheck = 1;
  myAnyFactor = -1;
  myAllFactor = -1;
  myMaxRange = -1;

  setCurrentDrawingData(
	  new ArDrawingData(*(myLaser->getCurrentDrawingData())),
	  true);

  setCumulativeDrawingData(
	  new ArDrawingData(*(myLaser->getCumulativeDrawingData())),
	  true);

  // laser parameters
  setInfoLogLevel(myLaser->getInfoLogLevel());
  setConnectionTimeoutSeconds(myLaser->getConnectionTimeoutSeconds());
  setCumulativeCleanDist(myLaser->getCumulativeCleanDist());
  setCumulativeCleanInterval(myLaser->getCumulativeCleanInterval());
  setCumulativeCleanOffset(myLaser->getCumulativeCleanOffset());

  setSensorPosition(myLaser->getSensorPosition());
  laserSetAbsoluteMaxRange(myLaser->getAbsoluteMaxRange());
  setMaxRange(myLaser->getMaxRange());
  
  // base range device parameters
  setMaxSecondsToKeepCurrent(myLaser->getMaxSecondsToKeepCurrent());
  setMinDistBetweenCurrent(getMinDistBetweenCurrent());
  setMaxSecondsToKeepCumulative(myLaser->getMaxSecondsToKeepCumulative());
  setMaxDistToKeepCumulative(myLaser->getMaxDistToKeepCumulative());
  setMinDistBetweenCumulative(myLaser->getMinDistBetweenCumulative());
  setMaxInsertDistCumulative(myLaser->getMaxInsertDistCumulative());
  setCurrentDrawingData(myLaser->getCurrentDrawingData(), false);
  setCumulativeDrawingData(myLaser->getCumulativeDrawingData(), false);

  // now all the specific laser settings (this should already be taken
  // care of when this is created, but the code existed for the
  // simulated laser so I put it here too)
  if (myLaser->canSetDegrees())
    laserAllowSetDegrees(
	    myLaser->getStartDegrees(), myLaser->getStartDegreesMin(), 
	    myLaser->getStartDegreesMax(), myLaser->getEndDegrees(), 
	    myLaser->getEndDegreesMin(), myLaser->getEndDegreesMax());

  if (myLaser->canChooseDegrees())
    laserAllowDegreesChoices(myLaser->getDegreesChoice(), 
			myLaser->getDegreesChoicesMap());

  if (myLaser->canSetIncrement())
    laserAllowSetIncrement(myLaser->getIncrement(), 
			   myLaser->getIncrementMin(), 
			   myLaser->getIncrementMax());

  if (myLaser->canChooseIncrement())
    laserAllowIncrementChoices(myLaser->getIncrementChoice(), 
			  myLaser->getIncrementChoicesMap());

  if (myLaser->canChooseUnits())
    laserAllowUnitsChoices(myLaser->getUnitsChoice(), 
			   myLaser->getUnitsChoices());

  if (myLaser->canChooseReflectorBits())
    laserAllowReflectorBitsChoices(myLaser->getReflectorBitsChoice(), 
			      myLaser->getReflectorBitsChoices());
  
  if (canSetPowerControlled())
    laserAllowSetPowerControlled(myLaser->getPowerControlled());

  if (myLaser->canChooseStartingBaud())
    laserAllowStartingBaudChoices(myLaser->getStartingBaudChoice(), 
			      myLaser->getStartingBaudChoices());

  if (myLaser->canChooseAutoBaud())
    laserAllowAutoBaudChoices(myLaser->getAutoBaudChoice(), 
			      myLaser->getAutoBaudChoices());
  
  laserSetDefaultTcpPort(myLaser->getDefaultTcpPort());
  laserSetDefaultPortType(myLaser->getDefaultPortType());


}

AREXPORT ArLaserFilter::~ArLaserFilter()
{
  if (myRobot != NULL)
  {
    myRobot->remSensorInterpTask(&myProcessCB);
    myRobot->remLaser(this);
  }
}

AREXPORT void ArLaserFilter::addToConfig(ArConfig *config, 
					       const char *sectionName,
					       const char *prefix)
{
  std::string name;
  
  config->addParam(ArConfigArg(ArConfigArg::SEPARATOR), sectionName,
		     ArPriority::TRIVIAL);
  name = prefix;
  name += "AngleSpread";
  config->addParam(
	  ArConfigArg(name.c_str(), &myAngleToCheck,
	      "The angle spread to check on either side of each reading",
		      0),
	  sectionName, ArPriority::TRIVIAL);

  name = prefix;
  name += "AnyNeighborFactor";
  config->addParam(
	  ArConfigArg(name.c_str(), &myAnyFactor,
	      "If a reading (decided by the anglespread) is further than any of its neighbor reading times this factor, it is ignored... so a value between 0 and 1 will check if they're all closer, a value greater than 1 will see if they're all further, negative values means this factor won't be used",
		      -1),
	  sectionName, ArPriority::TRIVIAL);

  name = prefix;
  name += "AllNeighborFactor";
  config->addParam(
	  ArConfigArg(name.c_str(), &myAllFactor,
	      "If a reading (decided by the anglespread) is further than all of its neighbor reading times this factor, it is ignored... so a value between 0 and 1 will check if they're all closer, a value greater than 1 will see if they're all further, negative values means this factor won't be used",
		      -1),
	  sectionName, ArPriority::TRIVIAL);

  name = prefix;
  name += "MaxRange";
  config->addParam(
	  ArConfigArg(name.c_str(), &myMaxRange,
		      "If a reading is further than this max range it will be ignored, -1 will use the base range device's max range",
		      -1),
	  sectionName, ArPriority::TRIVIAL);

  name = prefix;
  name += "CumulativeKeepDist";
  config->addParam(
	  ArConfigArg(name.c_str(), &myMaxDistToKeepCumulative,
		      "Distance cumulative readings can be from current pose",
		      -1),
	  sectionName, ArPriority::TRIVIAL);

  config->addParam(ArConfigArg(ArConfigArg::SEPARATOR), sectionName,
		   ArPriority::TRIVIAL);
}

AREXPORT void ArLaserFilter::setRobot(ArRobot *robot)
{
  myRobot = robot;
  if (myRobot != NULL)
  {
    myRobot->remSensorInterpTask(&myProcessCB);
    myRobot->addSensorInterpTask(myName.c_str(), 51, &myProcessCB);
  }
  ArLaser::setRobot(robot);
}

void ArLaserFilter::processReadings(void)
{
  myLaser->lockDevice();
  selfLockDevice();

  const std::list<ArSensorReading *> *rdRawReadings;
  std::list<ArSensorReading *>::const_iterator rdIt;
  
  if ((rdRawReadings = myLaser->getRawReadings()) == NULL)
  {
    selfUnlockDevice();
    myLaser->unlockDevice();
    return;
  }

  size_t rawSize = myRawReadings->size();
  size_t rdRawSize = myLaser->getRawReadings()->size();
  
  while (rawSize < rdRawSize)
  {
    myRawReadings->push_back(new ArSensorReading);
    rawSize++;
  }

  // set where the pose was taken
  myCurrentBuffer.setPoseTaken(
	  myLaser->getCurrentRangeBuffer()->getPoseTaken());
  myCurrentBuffer.setEncoderPoseTaken(
	  myLaser->getCurrentRangeBuffer()->getEncoderPoseTaken());


  std::list<ArSensorReading *>::iterator it;
  ArSensorReading *rdReading;
  ArSensorReading *reading;

#ifdef DEBUGRANGEFILTER
  FILE *file = NULL;
  file = ArUtil::fopen("/mnt/rdsys/tmp/filter", "w");
#endif

  std::map<int, ArSensorReading *> readingMap;
  int numReadings = 0;

  // first pass to copy the readings and put them into a map
  for (rdIt = rdRawReadings->begin(), it = myRawReadings->begin();
       rdIt != rdRawReadings->end() && it != myRawReadings->end();
       rdIt++, it++)
  {
    rdReading = (*rdIt);
    reading = (*it);
    *reading = *rdReading;
    
    readingMap[numReadings] = reading;
    numReadings++;
  }
  
  char buf[1024];
  int i;
  int j;
  ArSensorReading *lastAddedReading = NULL;
  
  // now walk through the readings to filter them
  for (i = 0; i < numReadings; i++)
  {
    reading = readingMap[i];

    // if we're ignoring this reading then just get on with life
    if (reading->getIgnoreThisReading())
      continue;

    if (myMaxRange >= 0 && reading->getRange() > myMaxRange)
    {
      reading->setIgnoreThisReading(true);
      continue;
    }
      
    if (lastAddedReading != NULL)
    {

      if (lastAddedReading->getPose().findDistanceTo(reading->getPose()) < 50)
      {
#ifdef DEBUGRANGEFILTER
	if (file != NULL)
	  fprintf(file, "%.1f too close from last %6.0f\n", 
		  reading->getSensorTh(),
		  lastAddedReading->getPose().findDistanceTo(
			  reading->getPose()));
#endif
	reading->setIgnoreThisReading(true);
	continue;
      }
#ifdef DEBUGRANGEFILTER
      else if (file != NULL)
	fprintf(file, "%.1f from last %6.0f\n", 
		reading->getSensorTh(),
		lastAddedReading->getPose().findDistanceTo(
			reading->getPose()));
#endif
    }

    buf[0] = '\0';
    bool goodAll = true;
    bool goodAny = false;
    if (myAnyFactor <= 0)
      goodAny = true;
    for (j = i - 1; 
	 (j >= 0 && //good && 
	  fabs(ArMath::subAngle(readingMap[j]->getSensorTh(),
				reading->getSensorTh())) <= myAngleToCheck);
	 j--)
    {
      if (readingMap[j]->getIgnoreThisReading())
      {
#ifdef DEBUGRANGEFILTER
	sprintf(buf, "%s %6s", buf, "i");
#endif
	continue;
      }
#ifdef DEBUGRANGEFILTER
      sprintf(buf, "%s %6d", buf, readingMap[j]->getRange());
#endif
      if (myAllFactor > 0 && 
	  !checkRanges(reading->getRange(), 
		       readingMap[j]->getRange(), myAllFactor))
	goodAll = false;
      if (myAnyFactor > 0 &&
	  checkRanges(reading->getRange(), 
		      readingMap[j]->getRange(), myAnyFactor))
	goodAny = true;
    }
#ifdef DEBUGRANGEFILTER
    sprintf(buf, "%s %6d*", buf, reading->getRange());
#endif 
    for (j = i + 1; 
	 (j < numReadings && //good &&
	  fabs(ArMath::subAngle(readingMap[j]->getSensorTh(),
				reading->getSensorTh())) <= myAngleToCheck);
	 j++)
    {
      if (readingMap[j]->getIgnoreThisReading())
      {
#ifdef DEBUGRANGEFILTER
	sprintf(buf, "%s %6s", buf, "i");
#endif
	continue;
      }
#ifdef DEBUGRANGEFILTER
      sprintf(buf, "%s %6d", buf, readingMap[j]->getRange());
#endif
      if (myAllFactor > 0 && 
	  !checkRanges(reading->getRange(), 
		       readingMap[j]->getRange(), myAllFactor))
	goodAll = false;
      if (myAnyFactor > 0 &&
	  checkRanges(reading->getRange(), 
		       readingMap[j]->getRange(), myAnyFactor))
	goodAny = true;
    }
    
    if (!goodAll || !goodAny)
      reading->setIgnoreThisReading(true);
    else
      lastAddedReading = reading;

#ifdef DEBUGRANGEFILTER
    if (file != NULL)
      fprintf(file, 
	      "%5.1f %6d %c\t%s\n", reading->getSensorTh(), reading->getRange(),
	      good ? 'g' : 'b', buf);
#endif
	    
  }


#ifdef DEBUGRANGEFILTER
  if (file != NULL)
    fclose(file);
#endif

  laserProcessReadings();

  selfUnlockDevice();
  myLaser->unlockDevice();
}

/**
   @return Return true if the reading is good, false if the reading is bad
**/
bool ArLaserFilter::checkRanges(int thisReading, int otherReading,
				double factor)
{
  if (thisReading == otherReading || factor <= 0)
    return true;

  if ((factor >= 1 && thisReading > otherReading * factor) || 
      (factor < 1 && thisReading < otherReading * factor))
    return false;
  else
    return true;
}


AREXPORT int ArLaserFilter::selfLockDevice(void)
{
  return lockDevice();
}

AREXPORT int ArLaserFilter::selfTryLockDevice(void)
{
  return tryLockDevice();
}

AREXPORT int ArLaserFilter::selfUnlockDevice(void)
{
  return unlockDevice();
}
