#include "Aria.h"
#include "ArExport.h"
#include "ArClientSwitchManager.h"

AREXPORT ArClientSwitchManager::ArClientSwitchManager(
	ArServerBase *serverBase, ArArgumentParser *parser) :
  myParseArgsCB(this, &ArClientSwitchManager::parseArgs),
  myLogOptionsCB(this, &ArClientSwitchManager::logOptions),
  mySocketClosedCB(this, &ArClientSwitchManager::socketClosed),
  mySwitchCB(this, &ArClientSwitchManager::clientSwitch),
  myNetCentralHeartbeatCB(this, &ArClientSwitchManager::netCentralHeartbeat),
  myNetCentralServerHeartbeatCB(this, 
			   &ArClientSwitchManager::netCentralServerHeartbeat),
  myFileUserCB(this, &ArClientSwitchManager::fileUserCallback),
  myFilePasswordCB(this, &ArClientSwitchManager::filePasswordCallback),
  myFileServerKeyCB(this, &ArClientSwitchManager::fileServerKeyCallback),
  myProcessFileCB(this, &ArClientSwitchManager::processFile)
{
  myMutex.setLogName("ArClientSwitchManager::myDataMutex");
  myServer = serverBase;
  myParser = parser;
  
  setThreadName("ArClientSwitchManager");

  mySwitchCB.setName("ArClientSwitchManager");

  myParseArgsCB.setName("ArClientSwitchManager");
  Aria::addParseArgsCB(&myParseArgsCB, 49);
  myLogOptionsCB.setName("ArClientSwitchManager");
  Aria::addLogOptionsCB(&myLogOptionsCB, 49);

  myNetCentralHeartbeatCB.setName("ArClientSwitchManager");
  myServer->addData("centralHeartbeat", 
		    "a packet that is requested and used by the central server to make sure there is still a connection, it should not be used by anyone or anything else, note that this sends a response over tcp and udp at the same time",
		    &myNetCentralHeartbeatCB, "none", "none", "RobotInfo", 
		    "RETURN_SINGLE");

  myServer->addData("centralServerHeartbeat", 
		    "a packet that is sent by central server to make sure there is still a connection to the robot, it should not be used by anyone or anything else",
		    &myNetCentralServerHeartbeatCB, "none", "none", "RobotInfo", 
		    "RETURN_SINGLE");

  myServerBackupTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotToCentralServerTimeoutInMins", 
		      &myServerBackupTimeout,
		      "The amount of time the robot can go without sending a packet to the central server successfully (when there are packets to send).  A number less than 0 means this won't happen.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", 
		      -1),
	  "Connection timeouts", ArPriority::DETAILED);

  myServerHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotFromCentralServerTimeoutInMins", 
		      &myServerHeartbeatTimeout,
		      "The amount of time a robot can go without getting the heartbeat from the central server before disconnecting it.  A number less than 0 means that the central server will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);
  
  myServerUdpHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotFromCentralServerUdpTimeoutInMins", 
		      &myServerUdpHeartbeatTimeout,
		      "The amount of time a robot can go without getting the udp heartbeat from the central server before disconnecting it.  A number less than 0 means that the central server will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);


  myProcessFileCB.setName("ArClientSwitchManager");
  Aria::getConfig()->addProcessFileCB(&myProcessFileCB, -1000);
  

  
  switchState(IDLE);
  myClient = NULL;
  myServerClient = NULL;
  myCentralServerPort = 5000;

  myFileUserCB.setName("ArClientSwitchManager::user");
  myFileParser.addHandler("user", &myFileUserCB);
  myFilePasswordCB.setName("ArClientSwitchManager::password");
  myFileParser.addHandler("password", &myFilePasswordCB);
  myFileServerKeyCB.setName("ArClientSwitchManager::serverKey");
  myFileParser.addHandler("serverKey", &myFileServerKeyCB);
}
 
AREXPORT ArClientSwitchManager::~ArClientSwitchManager()
{
}

AREXPORT bool ArClientSwitchManager::isConnected(void)
{
  return myState == CONNECTED;
}

AREXPORT void ArClientSwitchManager::switchState(State state)
{
  myState = state;
  myStartedState.setToNow();
  //myGaveTimeWarning = false;
}

AREXPORT bool ArClientSwitchManager::parseArgs(void)
{
  const char *centralServer = NULL;
  const char *identifier = NULL;

  if (!myParser->checkParameterArgumentString("-centralServer", 
					      &centralServer) || 
      !myParser->checkParameterArgumentString("-cs", &centralServer) || 
      !myParser->checkParameterArgumentInteger("-centralServerPort", 
				     &myCentralServerPort) || 
      !myParser->checkParameterArgumentInteger("-csp", 
					       &myCentralServerPort) || 
      !myParser->checkParameterArgumentString("-identifier", &identifier) || 
      !myParser->checkParameterArgumentString("-id", &identifier))
    return false;


  bool wasReallySetOnlyTrue = myParser->getWasReallySetOnlyTrue();
  myParser->setWasReallySetOnlyTrue(true);
  
  bool wasReallySet = false;
  const char *centralServerInfoFile = NULL;
  while (myParser->checkParameterArgumentString(
		 "-centralServerInfoFile", &centralServerInfoFile, 
		 &wasReallySet, true) && 
	 wasReallySet)
  {
    if (centralServerInfoFile != NULL && !parseFile(centralServerInfoFile))
    {
      myParser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
      return false;
    }
    wasReallySet = false;
  }
  
  myParser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);

  myDataMutex.lock();
  if (centralServer != NULL && centralServer[0] != '\0')
  {
    myCentralServer = centralServer;
    myState = TRYING_CONNECTION;
  }

  if (identifier != NULL && identifier[0] != '\0')
  {
    myIdentifier = identifier;
  }
  /* Just don't set it so the server can just set it to the IP
  else 
  {
    int nameLen;
    char name[512];
    int i;
    
    nameLen = ArMath::random() % 15 + 1;
    for (i = 0; i < nameLen; i++)
      name[i] = 'a' + (ArMath::random() % ('z' - 'a'));
    name[nameLen] = '\0';
    
    myIdentifier = name;
  }
  */
  myDataMutex.unlock();

  return true;
}

AREXPORT void ArClientSwitchManager::logOptions(void) const
{
  ArLog::log(ArLog::Terse, "ArClientSwitchManager options:");
//  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "-centralServer <host>");
  ArLog::log(ArLog::Terse, "-cs <host>");
  ArLog::log(ArLog::Terse, "-centralServerPort <port>");
  ArLog::log(ArLog::Terse, "-csp <port>");
  ArLog::log(ArLog::Terse, "-identifier <identifier>");
  ArLog::log(ArLog::Terse, "-id <identifier>");
  /*
  ArLog::log(ArLog::Terse, "-centralServerUser <user>");
  ArLog::log(ArLog::Terse, "-csu <user>");
  ArLog::log(ArLog::Terse, "-centralServerPassword <password>");
  ArLog::log(ArLog::Terse, "-csp <password>");
  ArLog::log(ArLog::Terse, "-centralServerKey <serverKey>");
  ArLog::log(ArLog::Terse, "-csk <password>");
  */
  ArLog::log(ArLog::Terse, "-centralServerInfoFile <fileName>");
}

AREXPORT void ArClientSwitchManager::clientSwitch(ArNetPacket *packet)
{
  //myDataMutex.lock();
  ArLog::log(ArLog::Normal, "Switch acknowledged, switching");
  
  myServerClient = myServer->makeNewServerClientFromSocket(
	  myClient->getTcpSocket(), true);
  myServerClient->getTcpSocket()->setCloseCallback(&mySocketClosedCB);
  myServerClient->setBackupTimeout(myServerBackupTimeout);

  ArSocket emptySocket;
  myClient->getTcpSocket()->transfer(&emptySocket);

  myLastTcpHeartbeat.setToNow();
  myLastUdpHeartbeat.setToNow();
  switchState(CONNECTED);
  //myDataMutex.unlock();
}

AREXPORT void ArClientSwitchManager::socketClosed(void)
{
  myDataMutex.lock();
  if (myState == CONNECTED)
  {
    myServerClient = NULL;
    ArLog::log(ArLog::Normal, "ArClientSwitchManager: Lost connection to central server");
    switchState(LOST_CONNECTION);
  }
  myDataMutex.unlock();
}

AREXPORT void *ArClientSwitchManager::runThread(void *arg)
{
  threadStarted();
  
  while (getRunning())
  {
    myDataMutex.lock();
    if (myState == IDLE)
    {
    }
    else if (myState == TRYING_CONNECTION)
    {
      myLastConnectionAttempt.setToNow();
      ArLog::log(ArLog::Normal, "Trying to connect to central server");
      myClient = new ArClientBase;
      myClient->setRobotName("ClientSwitch", myDebugLogging);
      myClient->setServerKey(myServerKey.c_str(), false);
      myLastTcpHeartbeat.setToNow();
      myLastUdpHeartbeat.setToNow();
      if (!myClient->blockingConnect(myCentralServer.c_str(), 
				     myCentralServerPort, false,
				     myUser.c_str(), myPassword.c_str(),
				     myServer->getOpenOnIP()))
      {
	if (myClient->wasRejected())
	  ArLog::log(ArLog::Normal, 
		     "Could not connect to %s because it rejected connection (bad user, password, or serverkey)",
		     myCentralServer.c_str());
	else
	  ArLog::log(ArLog::Verbose, 
	     "Could not connect to %s to switch with, not doing anything",
		     myCentralServer.c_str());
	myClient->getTcpSocket()->close();
	delete myClient;
	myClient = NULL;
	switchState(LOST_CONNECTION);
	myDataMutex.unlock();
	continue;
      }
      
      if (!myClient->dataExists("switch"))
      {
	ArLog::log(ArLog::Normal, 
		   "ArClientSwitchManager: Connected to central server %s but it isn't a central server, going to idle");
	myClient->disconnect();
	delete myClient;
	myClient = NULL;
	switchState(IDLE);
	myDataMutex.unlock();
	continue;
      }

      ArNetPacket sendPacket;
      ArLog::log(ArLog::Verbose, "Putting in %s\n", myIdentifier.c_str());
      sendPacket.strToBuf(myIdentifier.c_str());

      if (myClient->dataExists("centralServerHeartbeat"))
      {
	ArLog::log(ArLog::Normal, "Requesting switch (have heartbeat)");
	myServerHasHeartbeat = true;
      }
      else
      {
	ArLog::log(ArLog::Normal, "Requesting switch (no heartbeat)");
	myServerHasHeartbeat = false;
      }
      
      myClient->addHandler("switch", &mySwitchCB);
      myClient->requestOnce("switch", &sendPacket);
      switchState(CONNECTING);
    }
    else if (myState == CONNECTING)
    {
      /* old behavior that just warned
      if (!myGaveTimeWarning && myStartedState.secSince() > 10)
      {
	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Connecting has taken over 10 seconds, probably a problem");
	myGaveTimeWarning = true;
      }
      myClient->loopOnce();
      myDataMutex.unlock();
      ArUtil::sleep(1);
      continue;      
      */
      // new behavior that starts over
      myClient->loopOnce();
      if (myStartedState.secSince() >= 15 && 
	  myLastTcpHeartbeat.secSince() / 60.0 >= myServerHeartbeatTimeout)
      {
	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Connecting to central server has taken %.2f minutes, restarting connection", myStartedState.secSince() / 4.0);
	/// added this to try and eliminate the occasional duplicates
	myClient->getTcpSocket()->close();
	delete myClient;
	myClient = NULL;
	switchState(LOST_CONNECTION);
	myDataMutex.unlock();
	continue;
      }
      myDataMutex.unlock();
      ArUtil::sleep(1);
      continue;
    }
    else if (myState == CONNECTED)
    {
      if (myClient != NULL)
      {
	delete myClient;
	myClient = NULL;
      }
      // if we have a heartbeat timeout make sure we've heard the
      // heartbeat within that range
      if (myServerHasHeartbeat && myServerHeartbeatTimeout >= -.00000001 && 
	  myLastTcpHeartbeat.secSince() >= 5 && 
	  myLastTcpHeartbeat.secSince() / 60.0 >= myServerHeartbeatTimeout)
      {
	ArLog::log(ArLog::Normal, 
		   "ArClientSwitchManager: Dropping connection since haven't heard from central server in %g minutes", 
		   myServerHeartbeatTimeout);
	myServerClient->forceDisconnect(false);
	myServerClient = NULL;
	switchState(LOST_CONNECTION);
	myDataMutex.unlock();
	continue;
      }
      else if 
	(myServerHasHeartbeat && !myServerClient->isTcpOnly() && 
	 myServerUdpHeartbeatTimeout >= -.00000001 && 
	 myLastUdpHeartbeat.secSince() >= 5 && 
	 myLastUdpHeartbeat.secSince() / 60.0 >= myServerUdpHeartbeatTimeout)
      {
	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Switching to TCP only since gotten UDP from central server in %g minutes", 
		   myServerUdpHeartbeatTimeout);
	myServerClient->useTcpOnly();
      }
      
    }
    else if (myState == LOST_CONNECTION)
    {
      if (myLastConnectionAttempt.secSince() > 10)
      {
	switchState(TRYING_CONNECTION);
      }
    }
    myDataMutex.unlock();
    ArUtil::sleep(100);
  }
  threadFinished();
  return NULL;
}

AREXPORT void ArClientSwitchManager::netCentralHeartbeat(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sending;
  client->sendPacketTcp(&sending);
  client->sendPacketUdp(&sending);
}

AREXPORT void ArClientSwitchManager::netCentralServerHeartbeat(
	ArServerClient *client, ArNetPacket *packet)
{
  myDataMutex.lock();
  if (client != myServerClient)
  {
    ArLog::log(ArLog::Normal, "Got a central server heartbeat packet from someone that isn't the central server %s", client->getIPString());
    myDataMutex.unlock();
    return;
  }
  if (packet->getPacketSource() == ArNetPacket::TCP)
    myLastTcpHeartbeat.setToNow();
  else if (packet->getPacketSource() == ArNetPacket::UDP)
    myLastUdpHeartbeat.setToNow();
  else
    ArLog::log(ArLog::Normal, 
       "Got unknown packet source for central server heartbeat packet");
  myDataMutex.unlock();
}

AREXPORT bool ArClientSwitchManager::parseFile(const char *fileName)
{
  ArLog::log(ArLog::Normal, "Loading central server user/password from %s", 
	     fileName);
  if (!myFileParser.parseFile(fileName))
  {
    
    ArLog::log(ArLog::Normal, "Failed parsing central server user/password file %s", 
	     fileName);
    return false;
  }
  return true;
}

bool ArClientSwitchManager::fileUserCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad user line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myUser = "";
  else
    myUser = arg->getArg(0);
  return true;
}


bool ArClientSwitchManager::filePasswordCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad password line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myPassword = "";
  else
    myPassword = arg->getArg(0);
  return true;
}

bool ArClientSwitchManager::fileServerKeyCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad serverKey line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myServerKey = "";
  else
    myServerKey = arg->getArg(0);
  return true;
}

AREXPORT const char *ArClientSwitchManager::getCentralServerHostName(void)
{
  if (myCentralServer.size() <= 0)
    return NULL;
  else
    return myCentralServer.c_str();
}

bool ArClientSwitchManager::processFile(void)
{
  myDataMutex.lock();
  if (myServerClient != NULL)
    myServerClient->setBackupTimeout(myServerBackupTimeout);
  myDataMutex.unlock();
  return true;
}
