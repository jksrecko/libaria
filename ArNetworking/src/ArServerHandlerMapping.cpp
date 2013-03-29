#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerMapping.h"

/**
   @param server the server to add the handlers too

   @param robot the robot to map from

   @param laser the laser to map with

   @param baseDirectory the directory to put the map file into when
   its done, NULL or an empty string means into the working directory

   @param tempDirectory the directory to put the map file into while
   its being created, if this is NULL or empty it'll use the base directory
 **/
AREXPORT ArServerHandlerMapping::ArServerHandlerMapping(
	ArServerBase *server, ArRobot *robot, ArLaser *laser,
	const char *baseDirectory, const char *tempDirectory,
	bool useReflectorValues, ArLaser *laser2, 
	const char *suffix, const char *suffix2) :
  myMappingStartCB(this, &ArServerHandlerMapping::serverMappingStart),
  myMappingEndCB(this, &ArServerHandlerMapping::serverMappingEnd),
  myMappingStatusCB(this, &ArServerHandlerMapping::serverMappingStatus),
  myPacketHandlerCB(this, &ArServerHandlerMapping::packetHandler),
  myLoopStartCB(this, &ArServerHandlerMapping::simpleLoopStart),
  myLoopEndCB(this, &ArServerHandlerMapping::simpleLoopEnd)
{
  myServer = server;
  myRobot = robot;
  myLaser = laser;
  myUseReflectorValues = useReflectorValues;

  myLaser2 = laser2;
  if (suffix != NULL)
    mySuffix = suffix;
  if (suffix2 != NULL)
    mySuffix2 = suffix2;

  if (baseDirectory != NULL && strlen(baseDirectory) > 0)
    myBaseDirectory = baseDirectory;
  else
    myBaseDirectory = "";

  if (tempDirectory != NULL && strlen(tempDirectory) > 0)
    myTempDirectory = tempDirectory;
  else
    myTempDirectory = myBaseDirectory;

  if (myServer != NULL)
  {
    myServer->addData("mappingStart", "Starts making a map",
		      &myMappingStartCB, "string: name of map file",
		      "byte: 0 (started mapping), 1 (already mapping), 2 (could not start map)",
		      "Mapping", "RETURN_SINGLE");
    myServer->addData("mappingEnd", "Stops making a map",
		      &myMappingEndCB, "none",
		      "byte: 0 (ended mapping), 1 (no mapping to end), 2 (could not move temporary file to permanent file)", 
		      "Mapping", "RETURN_SINGLE");
    myServer->addData("mappingStatus", "Gets the status of mapping",
		      &myMappingStatusCB, "none",
	      "string: mapName if mapping, empty string if not mapping", 
		      "RobotInfo", "RETURN_SINGLE");
    myServer->addData("mappingStatusBroadcast", "Gets the status of mapping, also sent when mapping starts or ends (this is a new/better mappingStatus)",
		      &myMappingStatusCB, "none",
	      "string: mapName if mapping, empty string if not mapping", 
		      "RobotInfo", "RETURN_SINGLE");
  }
  myLaserLogger = NULL;
  myLaserLogger2 = NULL;
  myMapName = "";
  myFileName = "";
  myHandlerCommands = NULL;
  
  myPacketHandlerCB.setName("ArServerHandlerMapping");
  if (myRobot != NULL)
    myRobot->addPacketHandler(&myPacketHandlerCB);
}

AREXPORT ArServerHandlerMapping::~ArServerHandlerMapping()
{
  if (myLaserLogger != NULL)
  {
    delete myLaserLogger;
    myLaserLogger = NULL;
  }

  if (myLaserLogger2 != NULL)
  {
    delete myLaserLogger;
    myLaserLogger = NULL;
  }
}

AREXPORT void ArServerHandlerMapping::serverMappingStart(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  char buf[512];
  packet->bufToStr(buf, sizeof(buf));

  // see if we're already mapping
  if (myLaserLogger != NULL)
  {
    ArLog::log(ArLog::Normal, "MappingStart: Map already being made");
    sendPacket.byteToBuf(1);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }


  myRobot->lock();

  // lower case everything (to avoid case conflicts)
  ArUtil::lower(buf, buf, sizeof(buf));
  myMapName = buf;

  myFileName = myMapName;
  if (!mySuffix.empty())
    myFileName += mySuffix;
  if (strstr(myMapName.c_str(), ".2d") == NULL)
    myFileName += ".2d";

  if (myLaser2 != NULL)
  {
    myFileName2 = myMapName;
    if (!mySuffix2.empty())
      myFileName2 += mySuffix2;
    if (strstr(myMapName.c_str(), ".2d") == NULL)
      myFileName2 += ".2d";
  }

  // call our mapping started callbacks
  std::list<ArFunctor *>::iterator fit;
  ArFunctor *functor;
  for (fit = myMappingStartCallbacks.begin(); 
       fit != myMappingStartCallbacks.end(); 
       fit++)
  {
    functor = (*fit);
    functor->invoke();
  }


  myLaserLogger = new ArLaserLogger(myRobot, myLaser, 300, 25, myFileName.c_str(),
				  true, Aria::getJoyHandler(), 
				  myTempDirectory.c_str(), 
				  myUseReflectorValues,
				  Aria::getRobotJoyHandler(),
				  &myLocationDataMap);
  if (myLaserLogger == NULL)
  {
    ArLog::log(ArLog::Normal, "MappingStart: myLaserLogger == NULL");
    sendPacket.byteToBuf(2);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    myMapName = "";
    myFileName = "";
    myRobot->unlock();
    return;
  }
  if (!myLaserLogger->wasFileOpenedSuccessfully())
  {
    ArLog::log(ArLog::Normal, "MappingStart: Cannot open map file %s", 
	       myFileName.c_str());
    sendPacket.byteToBuf(2);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    myMapName = "";
    myFileName = "";
    delete myLaserLogger;
    myLaserLogger = NULL;
    myRobot->unlock();
    return;
  }

  if (myLaser2 != NULL)
  {
    myLaserLogger2 = new ArLaserLogger(myRobot, myLaser2, 300, 25, 
				     myFileName2.c_str(),
				     true, Aria::getJoyHandler(), 
				     myTempDirectory.c_str(), 
				     myUseReflectorValues,
				     Aria::getRobotJoyHandler(),
				     &myLocationDataMap);
  }

  // toss our strings for the start on there
  std::list<std::string>::iterator it;
  
  for (it = myStringsForStartOfLog.begin(); 
       it != myStringsForStartOfLog.end(); 
       it++)
  {
    myLaserLogger->addInfoToLogPlain((*it).c_str());
    if (myLaserLogger2 != NULL)
      myLaserLogger2->addInfoToLogPlain((*it).c_str());
  }
  myRobot->unlock();
  
  
  ArLog::log(ArLog::Normal, "MappingStart: Map %s started", 
	     myMapName.c_str());
  sendPacket.byteToBuf(0);
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);
  
  ArNetPacket broadcastPacket;
  broadcastPacket.strToBuf(myFileName.c_str());
  myServer->broadcastPacketTcp(&broadcastPacket, "mappingStatusBroadcast");
}


AREXPORT void ArServerHandlerMapping::serverMappingEnd(
	ArServerClient *client, ArNetPacket *packet)
{
  std::list<ArFunctor *>::iterator fit;

  ArNetPacket sendPacket;
  if (myLaserLogger == NULL)
  {
    ArLog::log(ArLog::Normal, "MappingEnd: No map being made");
    sendPacket.byteToBuf(1);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }

  myRobot->lock();
  delete myLaserLogger;
  myLaserLogger = NULL;

  bool haveFile2 = false;

  if (myLaserLogger2 != NULL)
  {
    haveFile2 = true;
    delete myLaserLogger2;
    myLaserLogger2 = NULL;
  }
    
  // now, if our temp directory and base directory are different we
  // need to move it and put the result in the packet, otherwise put
  // in we're okay
  if (ArUtil::strcasecmp(myBaseDirectory, myTempDirectory) != 0)
  {
#ifndef WIN32
    char *mvName = "mv";
#else
    char *mvName = "move";
#endif
    char systemBuf[6400];
    char fromBuf[1024];
    char toBuf[1024];

    char systemBuf2[6400];
    char fromBuf2[1024];
    char toBuf2[1024];

    if (myTempDirectory.size() > 0)
      snprintf(fromBuf, sizeof(fromBuf), "%s%s", 
	       myTempDirectory.c_str(), myFileName.c_str());
    else
      snprintf(fromBuf, sizeof(fromBuf), "%s", myFileName.c_str());
    ArUtil::fixSlashes(fromBuf, sizeof(fromBuf));

    if (myTempDirectory.size() > 0)
      snprintf(toBuf, sizeof(toBuf), "%s%s", 
	       myBaseDirectory.c_str(), myFileName.c_str());
    else
      snprintf(toBuf, sizeof(toBuf), "%s", myFileName.c_str());
    ArUtil::fixSlashes(toBuf, sizeof(toBuf));

    sprintf(systemBuf, "%s \"%s\" \"%s\"", mvName, fromBuf, toBuf);




    ArLog::log(ArLog::Verbose, "Moving with '%s'", systemBuf);
    // call our pre move callbacks
    for (fit = myPreMoveCallbacks.begin(); 
	 fit != myPreMoveCallbacks.end(); 
	 fit++)
      (*fit)->invoke();

    // move file
    int ret;
    if ((ret = system(systemBuf)) == 0)
    {
      ArLog::log(ArLog::Verbose, "ArServerHandlerMapping: Moved file %s (with %s)", 
		 myFileName.c_str(), systemBuf);
      sendPacket.byteToBuf(0);
    }
    else
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerHandlerMapping: Couldn't move file for %s (ret of '%s' is %d) removing file", 
		 myFileName.c_str(), systemBuf, ret);
      unlink(fromBuf);
      sendPacket.uByte2ToBuf(2);
    }

    if (haveFile2)
    {
      if (myTempDirectory.size() > 0)
	snprintf(fromBuf2, sizeof(fromBuf2), "%s%s", 
		 myTempDirectory.c_str(), myFileName2.c_str());
      else
	snprintf(fromBuf2, sizeof(fromBuf2), "%s", myFileName2.c_str());
      ArUtil::fixSlashes(fromBuf2, sizeof(fromBuf2));
      
      if (myTempDirectory.size() > 0)
	snprintf(toBuf2, sizeof(toBuf2), "%s%s", 
		 myBaseDirectory.c_str(), myFileName2.c_str());
      else
	snprintf(toBuf2, sizeof(toBuf2), "%s", myFileName2.c_str());
      ArUtil::fixSlashes(toBuf2, sizeof(toBuf2));
      
      sprintf(systemBuf2, "%s \"%s\" \"%s\"", mvName, fromBuf2, toBuf2);
      
      if ((ret = system(systemBuf2)) == 0)
      {
	ArLog::log(ArLog::Verbose, "ArServerHandlerMapping: Moved file2 %s (with %s)", 
		   myFileName2.c_str(), systemBuf2);
      }
      else
      {
	ArLog::log(ArLog::Normal, 
		   "ArServerHandlerMapping: Couldn't move file2 for %s (ret of '%s' is %d) removing file", 
		   myFileName2.c_str(), systemBuf2, ret);
	unlink(fromBuf2);
      }
    }
    
    // call our post move callbacks
    for (fit = myPostMoveCallbacks.begin(); 
	 fit != myPostMoveCallbacks.end(); 
	 fit++)
      (*fit)->invoke();

  }
  else
  {
    // just put in things are okay, it'll get sent below
    sendPacket.byteToBuf(0);
  }


  ArLog::log(ArLog::Normal, "MappingEnd: Stopped mapping %s", 
	     myFileName.c_str());

  // call our mapping started callbacks
  for (fit = myMappingEndCallbacks.begin(); 
       fit != myMappingEndCallbacks.end(); 
       fit++)
    (*fit)->invoke();

  myMapName = "";
  myFileName = "";

  myRobot->unlock();
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);

  ArNetPacket broadcastPacket;
  broadcastPacket.strToBuf(myFileName.c_str());
  myServer->broadcastPacketTcp(&broadcastPacket, "mappingStatusBroadcast");
}


AREXPORT void ArServerHandlerMapping::serverMappingStatus(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  sendPacket.strToBuf(myMapName.c_str());
  client->sendPacketTcp(&sendPacket);
}

AREXPORT void ArServerHandlerMapping::addStringForStartOfLogs(
	const char *str, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myStringsForStartOfLog.push_front(str);
  else if (position == ArListPos::LAST)
    myStringsForStartOfLog.push_back(str);
  else 
    ArLog::log(ArLog::Terse, "ArServerHandlerMapping::addStringForStartOfLogs: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remStringForStartOfLogs(
	const char *str)
{
  myStringsForStartOfLog.remove(str);
}

AREXPORT void ArServerHandlerMapping::addMappingStartCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingStartCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingStartCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addMappingStartCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingStartCallback(
	ArFunctor *functor)
{
  myMappingStartCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addMappingEndCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingEndCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingEndCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addMappingEndCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingEndCallback(
	ArFunctor *functor)
{
  myMappingEndCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addPreMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addPreMoveCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remPreMoveCallback(
	ArFunctor *functor)
{
  myPreMoveCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addPostMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addPostMoveCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remPostMoveCallback(
	ArFunctor *functor)
{
  myPostMoveCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addSimpleCommands(
	ArServerHandlerCommands *handlerCommands)
{
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addStringCommand(
	  "mappingLoopStart", 
	  "If mapping is happening it starts a new loop with the tag of the given string", 
	  &myLoopStartCB);

  myHandlerCommands->addStringCommand(
	  "mappingLoopEnd", 
	  "If mapping is happening it ends a loop with the tag of the given string", 
	  &myLoopEndCB);
}

AREXPORT void ArServerHandlerMapping::simpleLoopStart(ArArgumentBuilder *arg)
{
  if (myLaserLogger != NULL) 
    myLaserLogger->addTagToLog("loop start %s", arg->getFullString());
  if (myLaserLogger2 != NULL) 
    myLaserLogger2->addTagToLog("loop start %s", arg->getFullString());
}

AREXPORT void ArServerHandlerMapping::simpleLoopEnd(ArArgumentBuilder *arg)
{
  if (myLaserLogger != NULL) 
    myLaserLogger->addTagToLog("loop stop %s", arg->getFullString());
  if (myLaserLogger2 != NULL) 
    myLaserLogger2->addTagToLog("loop stop %s", arg->getFullString());
}

/// The packet handler for starting/stopping scans from the lcd
AREXPORT bool ArServerHandlerMapping::packetHandler(ArRobotPacket *packet)
{
  // we return these as processed to help out the ArLaserLogger class
  if (packet->getID() == 0x96)
    return true;

  if (packet->getID() != 0x97)
    return false;

  myRobot->unlock();
  char argument = packet->bufToByte();
  if (argument == 1)
  {
    // see if we're already mapping
    if (myLaserLogger != NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerHandlerMapping::packetHandler: Start scan requested when already mapping");
    }
    else
    {
      // build a name
      time_t now;
      struct tm nowStruct;
      now = time(NULL);
      char buf[1024];

	  if (ArUtil::localtime(&now, &nowStruct))
      {
	sprintf(buf, "%02d%02d%02d_%02d%02d%02d", 
		(nowStruct.tm_year%100), nowStruct.tm_mon+1, nowStruct.tm_mday, 
		nowStruct.tm_hour, nowStruct.tm_min, nowStruct.tm_sec);
      }
      else
      {
	ArLog::log(ArLog::Normal, 
		   "ArServerHandlerMapping::packetHandler: Could not make good packet name (error getting time), so just naming it \"lcd\"");
	sprintf(buf, "lcd");
      }

      ArLog::log(ArLog::Normal, "Starting scan '%s' from LCD", buf);
      ArNetPacket fakePacket;
      fakePacket.strToBuf(buf);
      fakePacket.finalizePacket();
      serverMappingStart(NULL, &fakePacket);
    }
  }
  else if (argument == 0)
  {
    if (myLaserLogger == NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerHandlerMapping::packetHandler: Stop scan requested when not mapping");
    }
    else
    {
      ArLog::log(ArLog::Normal, "Stopping scan from LCD");
      serverMappingEnd(NULL, NULL);
    }
  }
  else
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerMapping::packetHandler: Unknown scan argument %d", argument);
  }
  myRobot->lock();
  return true;
}

AREXPORT void ArServerHandlerMapping::addTagToLog(const char *str)
{
  if (myLaserLogger != NULL)
    myLaserLogger->addTagToLogPlain(str);
  if (myLaserLogger2 != NULL)
    myLaserLogger2->addTagToLogPlain(str);
}

AREXPORT void ArServerHandlerMapping::addInfoToLog(const char *str)
{
  if (myLaserLogger != NULL)
    myLaserLogger->addInfoToLogPlain(str);
  if (myLaserLogger2 != NULL)
    myLaserLogger2->addInfoToLogPlain(str);
}

AREXPORT const char * ArServerHandlerMapping::getFileName(void)
{
  return myFileName.c_str();
}

AREXPORT const char * ArServerHandlerMapping::getMapName(void)
{
  return myMapName.c_str();
}

AREXPORT bool ArServerHandlerMapping::isMapping(void)
{
  if (myLaserLogger != NULL)
    return true;
  else
    return false;
}

AREXPORT bool ArServerHandlerMapping::addLocationData(
	const char *name, 
	ArRetFunctor2<int, ArTime, ArPose *> *functor)
{
  if (myLocationDataMap.find(name) != myLocationDataMap.end())
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerMapping::addLocationData: Already have location data for %s", name);
    return false;
  }
  ArLog::log(ArLog::Normal, "ArServerHandlerMapping: Added location data %s", 
	     name);
  myLocationDataMap[name] = functor;
  return true;
}

  /// Get location data map (mostly for internal things)
AREXPORT const std::map<std::string, 
			ArRetFunctor2<int, ArTime, ArPose *> *, 
			ArStrCaseCmpOp> *
ArServerHandlerMapping::getLocationDataMap(void)
{
  return &myLocationDataMap;
}


AREXPORT void ArServerHandlerMapping::addStringsForStartOfLogToMap(
	ArMap *arMap)
{
  std::list<std::string>::iterator it;
  std::string str;
  ArArgumentBuilder *builder;

  for (it = myStringsForStartOfLog.begin(); 
       it != myStringsForStartOfLog.end(); 
       it++)
  {
    str = (*it);
    builder = new ArArgumentBuilder;
    builder->add(str.c_str());
    if (strcasecmp(builder->getArg(0), "LogInfoStrip:") == 0)
    {
      builder->removeArg(0, true);
      printf("lis %s\n", builder->getFullString());

    }
    std::list<std::string> infoNames = arMap->getInfoNames();
    for (std::list<std::string>::iterator iter = infoNames.begin();
         iter != infoNames.end();
         iter++) 
    {
      const char *curName = (*iter).c_str();

      if (strcasecmp(builder->getArg(0), curName) == 0)
      {
	      builder->removeArg(0, true);
	      arMap->getInfo(curName)->push_back(builder);
	      builder = NULL;
	      break;
      }
    }
    if (builder != NULL)
      delete builder;
  }
}
