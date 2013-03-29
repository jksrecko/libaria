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
#include "ArActionDesired.h"

/* VS 2003 can't use export attribute here on const static variables */
#if (_MSC_VER <= 1310)
#define STATIC_CONST_AREXPORT // defined to nothing
#else
#define STATIC_CONST_AREXPORT AREXPORT
#endif

STATIC_CONST_AREXPORT const double ArActionDesired::NO_STRENGTH = 0.0;
STATIC_CONST_AREXPORT const double ArActionDesired::MIN_STRENGTH = .000001;
STATIC_CONST_AREXPORT const double ArActionDesired::MAX_STRENGTH = 1.0;

STATIC_CONST_AREXPORT const double ArActionDesiredChannel::NO_STRENGTH =
                                                 ArActionDesired::NO_STRENGTH;
STATIC_CONST_AREXPORT const double ArActionDesiredChannel::MIN_STRENGTH = 
                                                ArActionDesired::MIN_STRENGTH;
STATIC_CONST_AREXPORT const double ArActionDesiredChannel::MAX_STRENGTH = 
                                                ArActionDesired::MAX_STRENGTH;


AREXPORT void ArActionDesired::log(void) const
{
  // all those maxes and movement parameters
  if (getMaxVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "\tMaxTransVel %.0f", getMaxVel());
  if (getMaxNegVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "\tMaxTransNegVel %.0f", 
	       getMaxNegVel());
  if (getTransAccelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "\tTransAccel %.0f", getTransAccel());
  if (getTransDecelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "\tTransDecel %.0f", getTransDecel());
  if (getMaxRotVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tMaxRotVel %.0f", "",
	       getMaxRotVel());
  if (getRotAccelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tRotAccel %.0f", "",
	       getRotAccel());
  if (getRotDecelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tRotDecel %.0f", "",
	       getRotDecel());

  if (getMaxLeftLatVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%12s\tMaxLeftLatVel %.0f", "",
	       getMaxLeftLatVel());
  if (getMaxRightLatVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%12s\tMaxRightLatVel %.0f", "",
	       getMaxRightLatVel());
  if (getLatAccelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%12s\tLatAccel %.0f", "",
	       getLatAccel());
  if (getLatDecelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%12s\tLatDecel %.0f", "",
	       getLatDecel());
  
  // the actual movement part
  if (getVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "\tVel %.0f", getVel());
  if (getHeadingStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tHeading %.0f", "", 
	       getHeading());
  if (getDeltaHeadingStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tDeltaHeading %.0f", "", 
	       getDeltaHeading());
  if (getRotVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%25s\tRotVel %.0f", "", 
	       getRotVel());
  if (getLatVelStrength() >= ArActionDesired::MIN_STRENGTH)
    ArLog::log(ArLog::Normal, "%12s\tLatVel %.0f", "", 
	       getLatVel());
}


AREXPORT bool ArActionDesired::isAnythingDesired(void) const
{
  if (getMaxVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getMaxNegVelStrength() >= ArActionDesired::MIN_STRENGTH || 
      getTransAccelStrength() >= ArActionDesired::MIN_STRENGTH || 
      getTransDecelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getMaxRotVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getRotAccelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getRotDecelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getHeadingStrength() >= ArActionDesired::MIN_STRENGTH ||
      getDeltaHeadingStrength() >= ArActionDesired::MIN_STRENGTH ||
      getRotVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getMaxLeftLatVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getMaxRightLatVelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getLatAccelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getLatDecelStrength() >= ArActionDesired::MIN_STRENGTH ||
      getLatVelStrength() >= ArActionDesired::MIN_STRENGTH)
    return true;
  else
    return false;
}
