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
#ifndef ARIAINTERNAL_H
#define ARIAINTERNAL_H


#include "ArMutex.h"
#include "ArFunctor.h"
#include "ArConfig.h"

#ifndef ARINTERFACE
#include "ArStringInfoGroup.h"
class ArRobot;
class ArRobotJoyHandler;
#endif // ARINTERFACE

class ArKeyHandler;
class ArJoyHandler;


/// This class performs global initialization and deinitialization
class Aria
{
public:

  typedef enum {
    SIGHANDLE_SINGLE, ///< Setup signal handlers in a global, non-thread way
    SIGHANDLE_THREAD, ///< Setup a dedicated signal handling thread
    SIGHANDLE_NONE ///< Do no signal handling
  } SigHandleMethod;

  /// Initialize Aria global data struture and perform OS-specific initialization, including adding OS signal handlers on Linux, initializing sockets library on Windows, etc.
  AREXPORT static void init(SigHandleMethod method = SIGHANDLE_THREAD,
			    bool initSockets = true, 
			    bool sigHandleExitNotShutdown = true);

  /// Performs OS-specific deinitialization, used by shutdown() and exit().
  AREXPORT static void uninit();

  /// Adds a callback to call when Aria is initialized using init()
  AREXPORT static void addInitCallBack(ArFunctor *cb, ArListPos::Pos position);

  /// Adds a callback to call when Aria is uninititialized using uninit()
  AREXPORT static void addUninitCallBack(ArFunctor *cb,
					 ArListPos::Pos position);

  /// Shutdown all Aria processes/threads
  AREXPORT static void shutdown();

  /// Shutdown all Aria processes/threads, call exit callbacks, and exit the program
  AREXPORT static void exit(int exitCode = 0);

  /// Sees if Aria is still running (mostly for the thread in main)
  AREXPORT static bool getRunning(void);

  /// Sets the directory that ARIA resides in, to override default
  AREXPORT static void setDirectory(const char * directory);

  /// Gets the directory that ARIA resides in
  AREXPORT static const char *getDirectory(void);

  /// Parses the arguments for the program (calls all the callbacks added with addParseArgsCB())
  AREXPORT static bool parseArgs(void);

  /// Logs all the options for the program (Calls all the callbacks added with addLogOptionsCB())
  AREXPORT static void logOptions(void);

  /// Sets the key handler, so that other classes can find it using getKeyHandler()
  AREXPORT static void setKeyHandler(ArKeyHandler *keyHandler);

  /// Gets a pointer to the global key handler, if one has been set with setKeyHandler()
  AREXPORT static ArKeyHandler *getKeyHandler(void);

  /// Sets the joystick handler, so that other classes can find it using getJoyHandler()
  AREXPORT static void setJoyHandler(ArJoyHandler *joyHandler);

  /// Get a pointer to the joystick handler if one has been set with setJoyHandler()
  AREXPORT static ArJoyHandler *getJoyHandler(void);

  /// Adds a functor to by called before program exit by Aria::exit()
  AREXPORT static void addExitCallback(ArFunctor *functor, int position = 50);

  /// Removes a functor to by called before program exit by Aria::exit()
  AREXPORT static void remExitCallback(ArFunctor *functor);

  /// Sets the log level for the exit callbacks
  AREXPORT static void setExitCallbacksLogLevel(ArLog::LogLevel level);

  /// Force an exit of all Aria processes/threads (the old way)
  AREXPORT static void exitOld(int exitCode = 0);

  /// Internal, the callback for the signal handling
  AREXPORT static void signalHandlerCB(int sig);

  /// Internal, calls the exit callbacks
  AREXPORT static void callExitCallbacks(void);

  /// Adds a callback for when we parse arguments 
  AREXPORT static void addParseArgsCB(ArRetFunctor<bool> *functor, 
				      int position = 50);

  /// Sets the log level for the parsing function
  AREXPORT static void setParseArgLogLevel(ArLog::LogLevel level);

  /// Adds a callback for when we log options
  AREXPORT static void addLogOptionsCB(ArFunctor *functor, int position = 50);

  /// Adds a type of deviceConnection for Aria to be able to create
  AREXPORT static bool deviceConnectionAddCreator(
	  const char *deviceConnectionType, 
	  ArRetFunctor3<ArDeviceConnection *, const char *, const char *, const char *> *creator);

  
  /// Gets a list of the possible deviceConnection types
  AREXPORT static const char *deviceConnectionGetTypes(void);
  
  /// Creates a deviceConnection of the given type
  AREXPORT static ArDeviceConnection *deviceConnectionCreate(
	  const char *deviceConnectionType, const char *port, 
	  const char *defaultInfo, 
	  const char *prefix = "Aria::deviceConnectionCreate");

#ifndef ARINTERFACE
  /// Sets the robot joystick handler, so that other classes can find it
  AREXPORT static void setRobotJoyHandler(ArRobotJoyHandler *robotJoyHandler);

  /// Gets the robot joystick handler if one has been set
  AREXPORT static ArRobotJoyHandler *getRobotJoyHandler(void);

  /// Gets the ArConfig for this program
  AREXPORT static ArConfig *getConfig(void);

  /// Gets the ArStringInfoGroup for this program
  AREXPORT static ArStringInfoGroup *getInfoGroup(void);

  /// Add a robot to the global list of robots
  AREXPORT static void addRobot(ArRobot *robot);

  /// Remove a robot from the global list of robots
  AREXPORT static void delRobot(ArRobot *robot);

  /// Finds a robot in the global list of robots, by name
  AREXPORT static ArRobot *findRobot(char *name);

  /// Get a copy of the global robot list
  AREXPORT static std::list<ArRobot*> * getRobotList();
  
  /// Gets the maximum number of lasers to use
  AREXPORT static int getMaxNumLasers(void);

  /// Sets the maximum number of lasers to use
  AREXPORT static void setMaxNumLasers(int maxNumLasers);

  /// Adds a type of laser for Aria to be able to create
  AREXPORT static bool laserAddCreator(
	  const char *laserType, 
	  ArRetFunctor2<ArLaser *, int, const char *> *creator);
  
  /// Gets a list of the possible laser types
  AREXPORT static const char *laserGetTypes(void);
  
  /// Creates a laser of the given type
  AREXPORT static ArLaser *laserCreate(
	  const char *laserType, int laserNumber,
	  const char *prefix = "Aria::laserCreate");
#endif // ARINTERFACE
protected:
  static bool ourInited;
  static ArGlobalFunctor1<int> ourSignalHandlerCB;
  static bool ourRunning;
  static ArMutex ourShuttingDownMutex;
  static bool ourShuttingDown;
  static bool ourExiting;
  static std::string ourDirectory;
  static std::list<ArFunctor*> ourInitCBs;
  static std::list<ArFunctor*> ourUninitCBs;
  static ArKeyHandler *ourKeyHandler;
  static ArJoyHandler *ourJoyHandler;
#ifndef ARINTERFACE
  static std::list<ArRobot*> ourRobots;
  static ArRobotJoyHandler *ourRobotJoyHandler;
  static ArConfig ourConfig;
  static ArStringInfoGroup ourInfoGroup;
  static int ourMaxNumLasers;
  static std::map<std::string, ArRetFunctor2<ArLaser *, int, const char *> *, ArStrCaseCmpOp> ourLaserCreatorMap;
  static std::string ourLaserTypes;
#endif // ARINTERFACE
  static ArMutex ourExitCallbacksMutex;
  static std::multimap<int, ArFunctor *> ourExitCallbacks;
  static bool ourSigHandleExitNotShutdown;
  static std::multimap<int, ArRetFunctor<bool> *> ourParseArgCBs;
  static ArLog::LogLevel ourParseArgsLogLevel;
  static std::multimap<int, ArFunctor *> ourLogOptionsCBs;
  static ArLog::LogLevel ourExitCallbacksLogLevel;
  static std::map<std::string, ArRetFunctor3<ArDeviceConnection *, const char *, const char *, const char *> *, ArStrCaseCmpOp> ourDeviceConnectionCreatorMap;
  static std::string ourDeviceConnectionTypes;
};


#endif // ARIAINTERNAL_H
