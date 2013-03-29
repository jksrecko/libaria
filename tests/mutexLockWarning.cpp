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

int main(int argc, char **argv) 
{
  Aria::init();
  ArMutex mutex;
  mutex.setLogName("test mutex");
  ArLog::log(ArLog::Normal, "This test succeeds if three (and only three) mutex lock/unlock time warning follow.");
  mutex.setUnlockWarningTime(1); // 1 sec
  mutex.lock();
  mutex.unlock(); // should not warn
  mutex.lock();
  ArUtil::sleep(2000); // 2 sec
  mutex.unlock(); // should warn
  mutex.lock();
  ArUtil::sleep(500); // 0.5 sec
  mutex.unlock();	// should not warn
  mutex.setUnlockWarningTime(0.5); // 0.5 sec
  mutex.lock();
  ArUtil::sleep(600); // 0.6 sec
  mutex.unlock(); // should warn
  mutex.lock();
  ArUtil::sleep(200); // 0.2 sec
  mutex.unlock(); // should not warn
  mutex.lock();
  mutex.unlock(); // should not warn
  mutex.setUnlockWarningTime(0.1);  // 0.1 sec
  mutex.lock();
  ArUtil::sleep(200); // 0.2 sec
  mutex.unlock(); // should warn
  mutex.setUnlockWarningTime(0.0); // off
  mutex.lock();
  ArUtil::sleep(100); // should not warn
  mutex.unlock();
  Aria::exit(0);
}

