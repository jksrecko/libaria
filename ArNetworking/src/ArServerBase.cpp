#include "Aria.h"
#include "ArExport.h"
#include "ArServerBase.h"
#include "ArServerCommands.h"
#include "ArClientCommands.h"
#include "ArServerMode.h"

/**
   @param addAriaExitCB whether to add an exit callback to aria or not
   @param serverName the name for logging
   @param addBackupTimeoutToConfig whether to add the backup timeout
   (to clients of this server) as a config option, if this is true it
   sets a default value for the timeout of 2 minutes, if its false it
   has no timeout (but you can set one, @see
   ArServerBase::setBackupTimeout)
   @param backupTimeoutName the name of backup timeout
   @param backupTimeoutName the description of backup timeout
   @param masterServer If this is a master server (ie a central manager)
   @param slaveServer If this is a slave server (ie a central forwarder)
   @param logPasswordFailureVerbosely Whether we log password failure verbosely or not
   @param allowSlowPackets Whether we allow slow packets (that go in
   their own thread) or not... if we don't then they're processed in
   the main thread like they have always been
   @param allowIdlePackets Whether we allow idle packets (that go in
   their own thread and are processed only when the robot is idle) or
   not... if we don't then they're processed in the main thread like
   they have always been
**/

AREXPORT ArServerBase::ArServerBase(bool addAriaExitCB, 
				    const char * serverName,
				    bool addBackupTimeoutToConfig,
				    const char *backupTimeoutName,
				    const char *backupTimeoutDesc,
				    bool masterServer, bool slaveServer,
				    bool logPasswordFailureVerbosely,
				    bool allowSlowPackets,
				    bool allowIdlePackets) :
  myProcessPacketCB(this, &ArServerBase::processPacket),
  mySendUdpCB(this, &ArServerBase::sendUdp),
  myAriaExitCB(this, &ArServerBase::close),
  myGetFrequencyCB(this, &ArServerBase::getFrequency, 0, true),
  myProcessFileCB(this, &ArServerBase::processFile),
  myIdentGetConnectionIDCB(this, &ArServerBase::identGetConnectionID),
  myIdentSetConnectionIDCB(this, &ArServerBase::identSetConnectionID),
  myIdentSetSelfIdentifierCB(this, &ArServerBase::identSetSelfIdentifier),
  myIdentSetHereGoalCB(this, &ArServerBase::identSetHereGoal),
  myIdleProcessingPendingCB(this, &ArServerBase::netIdleProcessingPending)
{



  myDataMutex.setLogName("ArServerBase::myDataMutex");
  myClientsMutex.setLogName("ArServerBase::myClientsMutex");
  myCycleCallbacksMutex.setLogName("ArServerBase::myCycleCallbacksMutex");
  myAddListMutex.setLogName("ArServerBase::myAddListMutex");
  myRemoveSetMutex.setLogName("ArServerBase::myRemoveSetMutex");
  myProcessingSlowIdleMutex.setLogName(
	  "ArServerBase::myProcessingSlowIdleMutex");
  myIdleCallbacksMutex.setLogName(
	  "ArServerBase::myIdleCallbacksMutex");

  if (serverName != NULL && serverName[0] > 0)
    myServerName = serverName;
  else
    myServerName = "ArServer";

  myLogPrefix = myServerName + "Base: ";
  myDebugLogging = false;
  myVerboseLogLevel = ArLog::Verbose;

  setThreadName(myServerName.c_str());
  myTcpPort = 0;
  myUdpPort = 0;
  myRejecting = 0;
  myOpened = false;
  myUdpReceiver.setProcessPacketCB(&myProcessPacketCB);
  if (slaveServer)
    myNextDataNumber = 40000;
  else
    myNextDataNumber = 256;
  myUserInfo = NULL;
  myAriaExitCB.setName("ArServerBaseExit");
  if (addAriaExitCB)
    Aria::addExitCallback(&myAriaExitCB, 20);
  
  myLogPasswordFailureVerbosely = logPasswordFailureVerbosely;
  myConnectionNumber = 1;
  
  myMostClients = 0;
  
  myLoopMSecs = 1;

  if (slaveServer)
  {
    myAllowSlowPackets = false;
    myAllowIdlePackets = false;
  }
  else
  {
    myAllowSlowPackets = allowSlowPackets;
    myAllowIdlePackets = allowIdlePackets;
  }
  mySlowIdleThread = NULL;

  if (myAllowSlowPackets || myAllowIdlePackets)
    mySlowIdleThread = new SlowIdleThread(this);
  
  myHaveSlowPackets = false;
  myHaveIdlePackets = false;
  myHaveIdleCallbacks = false;

  if (addBackupTimeoutToConfig)
  {
    myBackupTimeout = 10;
    Aria::getConfig()->addParam(
	    ArConfigArg(backupTimeoutName,
			&myBackupTimeout,
			backupTimeoutDesc, -1),
	    "Connection timeouts", 
	    ArPriority::DETAILED);
    myProcessFileCB.setName("ArServerBase");
    Aria::getConfig()->addProcessFileCB(&myProcessFileCB, -100);
  }
  else
    myBackupTimeout = -1;

  if (masterServer)
  {
    addData("identGetConnectionID", 
	    "gets an id for this connection, this id should then be set on any connections to the other servers associated with this one (ie the ones for individual robots) so that all the connections to one client can be tied together",
	    &myIdentGetConnectionIDCB, "none", "ubyte4: id",
	    "RobotInfo", "RETURN_SINGLE");
  }

  if (slaveServer)
  {
    addData("identSetConnectionID", 
	    "Sets an id for this connection, this id should then be from the getID on the main connection for this server, this is so that all the connections to one client can be tied together",
	    &myIdentSetConnectionIDCB, "ubyte4: id", "none",
	    "RobotInfo", "RETURN_SINGLE");
  }

  addData("identSetSelfIdentifier", 
	  "Sets an arbitrary self identifier for this connection, this can be used by programs to send information back to the same client (if this and the identSetHereGoal)",
	  &myIdentSetSelfIdentifierCB, "string: selfID", "none",
	  "RobotInfo", "RETURN_SINGLE");
  
  addData("identSetHereGoal", 
	  "Sets an string for the here goal for this connection, this can be used by programs to send information back to the same client (if this and the identSetSelfID match)",
	  &myIdentSetHereGoalCB, "string: hereGoal", "none",
	  "RobotInfo", "RETURN_SINGLE");

  addData("requestOnceReturnIsAlwaysTcp", 
	  "The presence of this command means that any requestOnce commands will have the data sent back to them as tcp... any requestOnceUdp will be returned either as tcp or udp (that is up to the callback sending the data)",
	  &myIdentSetHereGoalCB, "none", "none",
	  "RobotInfo", "RETURN_NONE");

  if (myAllowIdlePackets)
    addData("idleProcessingPending",
	    "The presence of this command means that the server will process packets flagged IDLE_PACKET only when the robot is idle.  This packet will be broadcast when the state of idle processing changes.",
	    &myIdleProcessingPendingCB, "none", "byte: 1 for idle processing pending, 0 for idle processing not pending",
	    "RobotInfo", "RETURN_SINGLE");

}

AREXPORT ArServerBase::~ArServerBase()
{
  close();

  if (mySlowIdleThread != NULL)
  {
    delete mySlowIdleThread;
    mySlowIdleThread = NULL;
  }
}

/**
   This opens the server up on the given port, you will then need to
   run the server for it to actually listen and respond to client connections.

   @param port the port to open the server on 
   @param openOnIP If not null, only listen on the local network interface with this address
   @param tcpOnly If true, only accept TCP connections. If false, use both TCP
   and UDP communications with clients.

   @return true if the server could open the port, false otherwise
**/
AREXPORT bool ArServerBase::open(unsigned int port, const char *openOnIP,
				 bool tcpOnly)
{
  myDataMutex.lock();
  myTcpPort = port;
  myTcpOnly = tcpOnly;
  if (openOnIP != NULL)
    myOpenOnIP = openOnIP;
  else
    myOpenOnIP = "";


  if (myTcpSocket.open(myTcpPort, ArSocket::TCP, openOnIP))
  {
    // This can be taken out since the open does this now
    //myTcpSocket.setLinger(0);
    myTcpSocket.setNonBlock();
  }
  else
  {
    myOpened = false;
    myDataMutex.unlock();
    return false;
  }

  if (myTcpOnly)
  {
    if (openOnIP != NULL)
      ArLog::log(ArLog::Normal, 
		 "%sStarted on tcp port %d on ip %s.", 
		 myLogPrefix.c_str(), myTcpPort, openOnIP);
    else
      ArLog::log(ArLog::Normal, "%sStarted on tcp port %d.",
		 myLogPrefix.c_str(), myTcpPort);
    myOpened = true;
    myUdpPort = 0;
    myDataMutex.unlock();
    return true;
  }
  
  if (myUdpSocket.create(ArSocket::UDP) &&
      myUdpSocket.findValidPort(myTcpPort, openOnIP) &&
      myUdpSocket.setNonBlock())
  {
 // not possible on udp?   myUdpSocket.setLinger(0);
    myUdpPort = ArSocket::netToHostOrder(myUdpSocket.inPort());
    myUdpReceiver.setSocket(&myUdpSocket);
    if (openOnIP != NULL)
      ArLog::log(ArLog::Normal, 
		 "%sStarted on tcp port %d and udp port %d on ip %s.", 
		 myLogPrefix.c_str(), myTcpPort, myUdpPort, openOnIP);
    else
      ArLog::log(ArLog::Normal, "%sStarted on tcp port %d and udp port %d.", 
		 myLogPrefix.c_str(), myTcpPort, myUdpPort);
  
  }
  else
  {
    myOpened = false;
    myTcpSocket.close();
    myDataMutex.unlock();
    return false;
  }
  myOpened = true;
  myDataMutex.unlock();
  return true;
}

AREXPORT void ArServerBase::close(void)
{
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  myClientsMutex.lock();
  if (!myOpened)
  {
    myClientsMutex.unlock();
    return;
  }

  ArLog::log(ArLog::Normal, "Server shutting down.");
  myOpened = false;
  // now send off the client packets to shut down
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    client->shutdown();
    printf("Sending shutdown\n");
  }

  ArUtil::sleep(10);

  while ((it = myClients.begin()) != myClients.end())
  {
    client = (*it);
    myClients.pop_front();
    delete client;
  }
  myTcpSocket.close();
  if (!myTcpOnly)
    myUdpSocket.close();
  myClientsMutex.unlock();
  
  /// MPL adding this since it looks like its leaked
  myDataMutex.lock();
  ArUtil::deleteSetPairs(myDataMap.begin(), myDataMap.end());
  myDataMap.clear();
  myDataMutex.unlock();
}

void ArServerBase::acceptTcpSockets(void)
{
  //ArSocket socket;

  //myClientsMutex.lock();

  // loop while we have new connections to make... we need to make a
  // udp auth key for any accepted sockets and send our introduction
  // to the client
  while (myTcpSocket.accept(&myAcceptingSocket) && 
	 myAcceptingSocket.getFD() >= 0)
  {
    finishAcceptingSocket(&myAcceptingSocket, false);
  }
  //myClientsMutex.unlock();
}

AREXPORT ArServerClient *ArServerBase::makeNewServerClientFromSocket(
	ArSocket *socket, bool doNotReject)
{
  ArServerClient *serverClient = NULL;
  //myDataMutex.lock();
  serverClient = finishAcceptingSocket(socket, true, doNotReject);
  //myDataMutex.unlock();
  return serverClient;
}

AREXPORT ArServerClient *ArServerBase::finishAcceptingSocket(
	ArSocket *socket, bool skipPassword, bool doNotReject)
{
  ArServerClient *client;
  bool foundAuthKey;
  std::list<ArServerClient *>::iterator it;
  bool thisKeyBad;
  long authKey;
  long introKey;
  ArNetPacket packet;
  int iterations;

  ArLog::log(ArLog::Normal, "%sNew connection from %s", 
	     myLogPrefix.c_str(), socket->getIPString());

  myClientsMutex.lock();
  // okay, now loop until we find a key or until we've gone 10000
  // times (huge error but not worth locking up on)... basically
  // we're making sure that none of the current clients already have
  // this key... they should never have it if random is working
  // right... but...
  for (foundAuthKey = false, authKey = ArMath::random(), iterations = 0; 
       !foundAuthKey && iterations < 10000;
       iterations++, authKey = ArMath::random())
  {
    // see if any of the current clients have this authKey
    for (thisKeyBad = false, it = myClients.begin(); 
	 !thisKeyBad && it != myClients.end();
	 ++it)
    {
      if (authKey == (*it)->getAuthKey())
	thisKeyBad = true;
    }
    // if the key wasn't found above it means its clean, so we found
    // a good one and are done
    if (!thisKeyBad)
      foundAuthKey = true;
  }
  myClientsMutex.unlock();
  if (!foundAuthKey)
    authKey = 0;
  // now we pick an introKey which introduces the server to the client
  // this one isn't critical that it be unique though, so we just
  // set it straight out
  introKey = ArMath::random();
  std::string passwordKey;
  int numInPasswordKey = ArMath::random() % 100;
  int i;
  passwordKey = "";
  if (!skipPassword)
  {
    for (i = 0; i < numInPasswordKey; i++)
      passwordKey += '0' + ArMath::random() % ('z' - '0');
  }
  int reject;
  const char *rejectString;
  if (doNotReject)
  {
    reject = 0;
    rejectString = "";
  }
  else
  {
    reject = myRejecting;
    rejectString = myRejectingString.c_str();
  }
  std::string serverClientName = myServerName + "Client_" + socket->getIPString();
  ArLog::log(myVerboseLogLevel, "%sHanding off connection to %s to %s",
	     myLogPrefix.c_str(), socket->getIPString(), 
	     serverClientName.c_str());
  client = new ArServerClient(socket, myUdpPort, authKey, introKey,
			      &mySendUdpCB, &myDataMap, 
			      passwordKey.c_str(), myServerKey.c_str(),
			      myUserInfo, reject, rejectString,
			      myDebugLogging, serverClientName.c_str(),
			      myLogPasswordFailureVerbosely,
			      myAllowSlowPackets, myAllowIdlePackets);
  //client->setUdpAddress(socket->sockAddrIn());
  // put the client onto our list of clients...
  //myClients.push_front(client);
  myAddListMutex.lock();
  myAddList.push_front(client);
  myAddListMutex.unlock();
  return client;
}

AREXPORT int ArServerBase::getNumClients(void)
{
  return myNumClients;
}

AREXPORT void ArServerBase::run(void)
{
  runInThisThread();
}

AREXPORT void ArServerBase::runAsync(void)
{
  create();
}

AREXPORT void *ArServerBase::runThread(void *arg)
{
  threadStarted();
  while (myRunning)
  {
    loopOnce();
    ArUtil::sleep(myLoopMSecs);
  }
  close();
  threadFinished();
  return NULL;
}


/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketTcpWithExclusion for that if you really want it...
   
   @param packet the packet to send
   @param name the type of data to send
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcp(ArNetPacket *packet, 
					       const char *name)
{
  return broadcastPacketTcpWithExclusion(packet, name, NULL);
}


/**
   This will broadcast this packet to any client that wants this
   data.... 
   
   @param packet the packet to send
   @param name the type of data to send
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpToMatching(
	ArNetPacket *packet, const char *name, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  return broadcastPacketTcpWithExclusion(packet, name, NULL, true, identifier, 
					 matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this data.
   
   @param packet the packet to send
   @param name the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpWithExclusion(
	ArNetPacket *packet, const char *name, ArServerClient *excludeClient,
	bool match, ArServerClientIdentifier identifier, 
	bool matchConnectionID)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int command = 0;
  
  myDataMutex.lock();  
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      command = (*it).second->getCommand();
  }  
  if (command == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	       "ArServerBase::broadcastPacket: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return false;
  }
  myDataMutex.unlock();  
  return broadcastPacketTcpByCommandWithExclusion(
	  packet, command, excludeClient, match, identifier, 
	  matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketTcpByCommandWithExclusion for that...
   
   @param packet the packet to send
   @param command the command number of the data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpByCommand(
	ArNetPacket *packet, unsigned int command)
{
  return broadcastPacketTcpByCommandWithExclusion(packet, command, NULL);
}

/**
   This will broadcast this packet to any client that wants this data.
   
   @param packet the packet to send
   @param command the command number of the data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpByCommandWithExclusion(
	ArNetPacket *packet, unsigned int command, 
	ArServerClient *excludeClient, bool match, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  std::list<ArServerClient *>::iterator lit;
  ArServerClient *serverClient;
  ArNetPacket emptyPacket;

  myClientsMutex.lock();  
  emptyPacket.empty();

  if (!myOpened)
  {
    ArLog::log(ArLog::Verbose, "ArServerBase::broadcastPacket: server not open to send packet.");
    myClientsMutex.unlock();  
    return false;
  }

  if (packet == NULL)
    packet = &emptyPacket;

  packet->setCommand(command);
  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
  {
    serverClient = (*lit);
    if (excludeClient != NULL && serverClient == excludeClient)
      continue;
    if (match && 
	!serverClient->getIdentifier().matches(identifier, matchConnectionID))
      continue;
    serverClient->broadcastPacketTcp(packet);
  }

  myClientsMutex.unlock();  
  return true;
}


/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketUdpWithExclusion for that if you really want to

   @param packet the packet to send
   @param name the type of data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdp(ArNetPacket *packet, 
					       const char *name)
{
  return broadcastPacketUdpWithExclusion(packet, name, NULL);
}

/**
   This will broadcast this packet to any client that wants this
   data.... 

   @param packet the packet to send
   @param name the type of data to send
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpToMatching(
	ArNetPacket *packet, const char *name,
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  return broadcastPacketUdpWithExclusion(packet, name, NULL, true, 
					 identifier, matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this data.

   @param packet the packet to send
   @param name the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/

AREXPORT bool ArServerBase::broadcastPacketUdpWithExclusion(
	ArNetPacket *packet, const char *name, ArServerClient *excludeClient,
	bool match, ArServerClientIdentifier identifier, 
	bool matchConnectionID)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int command = 0;

  myDataMutex.lock();  
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      command = (*it).second->getCommand();
  }  
  if (command == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	       "ArServerBase::broadcastPacket: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return false;
  }
  myDataMutex.unlock();  
  return broadcastPacketUdpByCommandWithExclusion(
	  packet, command, excludeClient, match, identifier, 
	  matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this
   data.... 
   
   (To restrict what clients the command is sent to, use the
   broadcastPacketUdpByCommandWithExclusion() method.)

   @param packet the packet to send
   @param command the type of data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpByCommand(
	ArNetPacket *packet, unsigned int command)
{
  return broadcastPacketUdpByCommandWithExclusion(packet, command, NULL);
}

/**
   This will broadcast this packet to any client that wants this data.

   @param packet the packet to send
   @param command the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpByCommandWithExclusion(
	ArNetPacket *packet, unsigned int command,
	ArServerClient *excludeClient, bool match, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  std::list<ArServerClient *>::iterator lit;
  ArServerClient *serverClient;
  ArNetPacket emptyPacket;

  myClientsMutex.lock();    
  emptyPacket.empty();

  if (!myOpened)
  {
    ArLog::log(ArLog::Verbose, "ArServerBase::broadcastPacket: server not open to send packet.");
    myClientsMutex.unlock();  
    return false;
  }

  if (packet == NULL)
    packet = &emptyPacket;

  packet->setCommand(command);
  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
  {
    serverClient = (*lit);
    if (excludeClient != NULL && serverClient == excludeClient)
      continue;
    if (match && 
	!serverClient->getIdentifier().matches(identifier, matchConnectionID))
      continue;
    serverClient->broadcastPacketUdp(packet);
  }
  myClientsMutex.unlock();  
  return true;
}

/**
   @param name Name is the name of the command number to be found

   @return the number of the command or 0 if no command has that name
**/

AREXPORT unsigned int ArServerBase::findCommandFromName(const char *name)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int num = 0;
  myDataMutex.lock();    

  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      num = (*it).second->getCommand();
  }  
  if (num == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	    "ArServerBase::findCommandFromName: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return 0;
  }
  myDataMutex.unlock();  
  return num;
}

/**
   This runs the server loop once, normally you'll probably just use
   run() or runAsync(), but if you want to drive this server from within your own
   loop just call this repeatedly, but do note it takes up no time
   at all, so you'll want to put in your own millisecond sleep so it
   doesn't monopolize the CPU.
 **/
AREXPORT void ArServerBase::loopOnce(void)
{
  std::list<ArServerClient *>::iterator it;
  std::set<ArServerClient *>::iterator setIt;
  // for speed we'd use a list of iterators and erase, but for clarity
  // this is easier and this won't happen that often
  //std::list<ArServerClient *> removeList;
  ArServerClient *client;

  myDataMutex.lock();  
  if (!myOpened)
  {
    myDataMutex.unlock();
    return;
  }
  myDataMutex.unlock();

  acceptTcpSockets();

  if (!myTcpOnly)
    myUdpReceiver.readData();

  if (myProcessingSlowIdleMutex.tryLock() == 0)
  {
    myClientsMutex.lock();
    myAddListMutex.lock();
    while (!myAddList.empty())
    {
      myClients.push_back(myAddList.front());
      myAddList.pop_front();
    }
    myAddListMutex.unlock();
    myClientsMutex.unlock();
    myProcessingSlowIdleMutex.unlock();
  }

  bool needIdleProcessing = idleProcessingPending();
  if (needIdleProcessing != myLastIdleProcessingPending)
  {
    ArNetPacket sendingPacket;
    if (needIdleProcessing)
      sendingPacket.byteToBuf(1);
    else
      sendingPacket.byteToBuf(0);
    broadcastPacketTcp(&sendingPacket, "idleProcessingPending");
    myLastIdleProcessingPending = needIdleProcessing;
  }

  bool haveSlowPackets = false;
  bool haveIdlePackets = false;
  int numClients = 0;


  // need a recursive lock before we can lock here but we should be
  //okay without a lock here (and have been for ages)
  //myClientsMutex.lock(); first let the clients handle new data
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    numClients++;
    if (!client->tcpCallback())
    {
      client->forceDisconnect(true);
      myRemoveSetMutex.lock();
      myRemoveSet.insert(client);
      myRemoveSetMutex.unlock();
    }
    else
    {
      if (client->hasSlowPackets())
	haveSlowPackets = true;
      if (client->hasIdlePackets())
	haveIdlePackets = true;
    }
      
  }

  myHaveSlowPackets = haveSlowPackets;
  myHaveIdlePackets = haveIdlePackets;

  // remember how many we've had at max so its easier to track down memory
  if (numClients > myMostClients)
    myMostClients = numClients;

  myNumClients = numClients;

  //printf("Before...\n");
  if (myProcessingSlowIdleMutex.tryLock() == 0)
  {
    //printf("Almost...\n");
    myClientsMutex.lock();
    myRemoveSetMutex.lock();
    //printf("In...\n");
    while ((setIt = myRemoveSet.begin()) != myRemoveSet.end())
    {
      client = (*setIt);
      myClients.remove(client);
      for (std::list<ArFunctor1<ArServerClient*> *>::iterator rci = myClientRemovedCallbacks.begin();
	   rci != myClientRemovedCallbacks.end();
	   rci++) {
	if (*rci) {
	  (*rci)->invoke(client);
	}
      }
      
      myRemoveSet.erase(client);
      delete client;
    }
    myRemoveSetMutex.unlock();
    myClientsMutex.unlock();
    //printf("out...\n");
    myProcessingSlowIdleMutex.unlock();
  }

  // now let the clients send off their packets
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    client->handleRequests();
  }
  // need a recursive lock before we can lock here but we should be
  //okay without a lock here (and have been for ages)
  //myClientsMutex.unlock();
  myCycleCallbacksMutex.lock();
  // call cycle callbacks
  for(std::list<ArFunctor*>::const_iterator f = myCycleCallbacks.begin();
          f != myCycleCallbacks.end(); f++) 
  {
    if(*f) (*f)->invoke();
  }
  myCycleCallbacksMutex.unlock();
}

AREXPORT void ArServerBase::processPacket(ArNetPacket *packet, struct sockaddr_in *sin)
{
  std::list<ArServerClient *>::iterator it;
  unsigned char *bytes = (unsigned char *)&sin->sin_addr.s_addr;
  ArServerClient *client;
  struct sockaddr_in *clientSin;

  myClientsMutex.lock();
  // if its a udp packet then see if we have its owner
  if (packet->getCommand() == ArClientCommands::UDP_INTRODUCTION)
  {
    long authKey;
    bool matched;

    authKey = packet->bufToUByte4();
    for (matched = false, it = myClients.begin();
	 !matched && it != myClients.end(); ++it)
    {
      client = (*it);
      if (client->getAuthKey() == authKey)
      {
	packet->resetRead();
	/*
	ArLog::log(ArLog::Normal, "Got process auth from %d.%d.%d.%d %d\n", 
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));	     
	*/
	client->processAuthPacket(packet, sin);
      }
    }
    myClientsMutex.unlock();
    return;
  }

  // it wasn't the introduction so handle it the normal way

  // walk through our list of clients and see if it was one of them,
  // if so return
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    clientSin = client->getUdpAddress();
    if (clientSin->sin_port == sin->sin_port &&
	clientSin->sin_addr.s_addr == sin->sin_addr.s_addr)
    {
      client->processPacket(packet, false);
      myClientsMutex.unlock();
      return;
    }
    /*
    else
    {
      bytes = (unsigned char *)&clientSin->sin_addr.s_addr;
      printf("Wanted %d.%d.%d.%d %d\n", bytes[0], 
	     bytes[1], bytes[2], bytes[3], 
	     ArSocket::netToHostOrder(clientSin->sin_port));
    }
    */
  }
  // if it wasn't one of our clients it was from somewhere bogus
  //bytes = (unsigned char *)&clientSin->sin_addr.s_addr;
  bytes = (unsigned char *)&sin->sin_addr.s_addr;
  if (myDataMap.find(packet->getCommand()) != myDataMap.end())
    ArLog::log(ArLog::Normal, 
	       "UDP Packet %s from bogus source %d.%d.%d.%d %d\n", 
	       myDataMap[packet->getCommand()]->getName(),
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));
  else
    ArLog::log(ArLog::Normal, 
	       "UDP Packet unknown from bogus source %d.%d.%d.%d %d\n", 
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));
  myClientsMutex.unlock();
}

AREXPORT bool ArServerBase::sendUdp(ArNetPacket *packet, struct sockaddr_in *sin)
{
  bool ret;
  // this doesn't lock since it should only be called from 
  ret = myUdpSocket.sendTo(packet->getBuf(), packet->getLength(), sin);
  return ret;
}

/**
 * @note A callback is called from the server run loop, and if this callback
 * does not return in a timely manner data reception will be delayed (blocked).
  
 
   @param name the name of the data 

   @param description the more verbose description of what the data is for

   @param functor the functor to call when this data is requested

   @param argumentDescription a description of the arguments expected

   @param returnDescription a description of what the data returns

   @param commandGroup the name of the group this command is in

   @param dataFlags Most people won't need this, its for some advanced
   server things... this is a list of data flags separated by |
   characters, the flags are listed in ArClientData docs 

   @pynote Pass the name of a function or a lambda expression for @arg functor.
   @javanote Use a subclass of ArFunctor_ServerData instead of the ArFunctor2 template @arg functor.
**/
AREXPORT bool ArServerBase::addData(
	const char *name, const char *description,
	ArFunctor2<ArServerClient *, ArNetPacket *> * functor,
	const char *argumentDescription, const char *returnDescription, 
	const char *commandGroup, const char *dataFlags)
{
  return addDataAdvanced(name, description, functor, argumentDescription, 
		  returnDescription, commandGroup, dataFlags);
}

/**
   @copydoc ArServerBase::addData() 

   @param advancedCommandNumber 0 for most people... it is the optional number
   of the command, this is a very advanced option and is made for some
   server forwarding things so basically no one should use it, 0 means
   use the auto way... anything else does it manually... the number
   needs to be the same or higher as the next number available

   @param requestChangedFunctor functor called with the lowest amount
   of time requested for this packet, called whenever the requests for
   the packet change, note that 0 and higher means thats how often it
   was asked for, -1 means something has requested the data but just
   when it is called, and -2 means that nothing wants the data
   
   @param requestOnceFunctor functor called if this was a request
   once... with the server and client just like the normal functor,
   but its only called if its a request once, note that both are
   called (as long as they aren't NULL of course)

   @sa addData()
**/

AREXPORT bool ArServerBase::addDataAdvanced(
	const char *name, const char *description,
	ArFunctor2<ArServerClient *, ArNetPacket *> * functor,
	const char *argumentDescription, const char *returnDescription, 
	const char *commandGroup, const char *dataFlags,
	unsigned int advancedCommandNumber,
	ArFunctor2<long, unsigned int> *requestChangedFunctor, 
	ArFunctor2<ArServerClient *, ArNetPacket *> *requestOnceFunctor)
{
  ArServerData *serverData;
  std::map<unsigned int, ArServerData *>::iterator it;

  myDataMutex.lock();

  //printf("%s %s\n", name, description);
  // if we already have one we can't do this
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
    {
      ArLog::log(ArLog::Verbose, "ArServerBase::addData: already have data for name \"%s\", could not add it.", name);
      myDataMutex.unlock();
      return false;
    }
  }
  ArLog::log(ArLog::Verbose, "ArServerBase::addData: name \"%s\" mapped to number %d", name, myNextDataNumber);
  if (advancedCommandNumber != 0)
  {
    // old check from unified command numbers
    /* if (advancedCommandNumber < myNextDataNumber)
    {
      ArLog::log(ArLog::Normal, "ArServerBase::addData: Advanced command number given for %s but the number is too low, it wasn't added", name);
      myDataMutex.unlock();
      return false;
    }
    */
    if (myDataMap.find(advancedCommandNumber) != myDataMap.end())
    {
      ArLog::log(ArLog::Normal, "ArServerBase::addData: Advanced command number given for %s is already used, it wasn't added", name);
      myDataMutex.unlock();
      return false;
    }
    serverData = new ArServerData(name, description, advancedCommandNumber,
				  functor, argumentDescription, 
				  returnDescription, commandGroup, dataFlags,
				  &myGetFrequencyCB, requestChangedFunctor, 
				  requestOnceFunctor);
    if (myAdditionalDataFlags.size() > 0)
      serverData->addDataFlags(myAdditionalDataFlags.c_str());
    myDataMap[advancedCommandNumber] = serverData;
    myDataMutex.unlock();
    return true;

  }
  else
  {
    serverData = new ArServerData(name, description, myNextDataNumber,
				  functor, argumentDescription, 
				  returnDescription, commandGroup, dataFlags,
				  &myGetFrequencyCB, requestChangedFunctor, 
				  requestOnceFunctor);
    if (myAdditionalDataFlags.size() > 0)
      serverData->addDataFlags(myAdditionalDataFlags.c_str());
    myDataMap[myNextDataNumber] = serverData;
    myNextDataNumber++;
    myDataMutex.unlock();
    return true;
  }
}


/** Cycle callbacks are called every cycle, in no particular order.
 * @param functor Functor to add.
 */
AREXPORT void ArServerBase::addCycleCallback(ArFunctor* functor)
{
  myCycleCallbacksMutex.lock();
  myCycleCallbacks.push_back(functor);
  myCycleCallbacksMutex.unlock();
}

/** Cycle callbacks are called every cycle, in no particular order.
 * @param functor Functor to remove. If it's not currently in the cycle callback
 * list, nothing is done.
 */
AREXPORT void ArServerBase::remCycleCallback(ArFunctor* functor)
{
  myCycleCallbacksMutex.lock();
  myCycleCallbacks.remove(functor);
  myCycleCallbacksMutex.unlock();
}

/**
 *  @javanote Use ArFunctor_ServerClient instead of ArFunctor1<ArServerClient*>
 *  for @a functor.
 */
AREXPORT void ArServerBase::addClientRemovedCallback(ArFunctor1<ArServerClient *> *functor)
{
  myDataMutex.lock();
  myClientRemovedCallbacks.push_back(functor);
  myDataMutex.unlock();
}

/**
 *  @javanote Use ArFunctor_ServerClient instead of ArFunctor1<ArServerClient*>
 *  for @a functor.
 */
AREXPORT void ArServerBase::remClientRemovedCallback(ArFunctor1<ArServerClient *> *functor)
{
  myDataMutex.lock();
  myClientRemovedCallbacks.remove(functor);
  myDataMutex.unlock();
}


AREXPORT bool ArServerBase::loadUserInfo(const char *fileName,
					 const char *baseDirectory)
{
  if (myUserInfo != NULL)
  {
    delete myUserInfo;
    myUserInfo = NULL;
  }
  ArServerUserInfo *userInfo = new ArServerUserInfo;
  userInfo->setBaseDirectory(baseDirectory);
  if (!userInfo->readFile(fileName))
  {
    ArLog::log(ArLog::Terse, "%sloadUserInfo: Could not load user info for %s", myLogPrefix.c_str());
    delete userInfo;
    return false;
  }
  if (!userInfo->doNotUse())
  {
    ArLog::log(ArLog::Normal, "%sLoaded user information",
	       myLogPrefix.c_str());
    myDataMutex.lock();
    myUserInfo = userInfo;
    myDataMutex.unlock();
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "%sLoaded user information but not using it",
	       myLogPrefix.c_str());
    delete userInfo;
  }
  return true;
}

AREXPORT void ArServerBase::logUserInfo(void)
{
  myDataMutex.lock();
  if (myUserInfo == NULL)
    ArLog::log(ArLog::Terse, 
	       "%sNo user name or password needed to connect",
	       myLogPrefix.c_str());
  else
    myUserInfo->logUsers();
  myDataMutex.unlock();
}

/**
   Logs the different groups of commands.  It logs the command names
   first along with the list of commands in that group.  Then it
   outputs a list of groups.  Useful for building the user/pass file.
**/
AREXPORT void ArServerBase::logCommandGroups(void)
{
  logCommandGroupsToFile(NULL);
}

/**
   For a description of what this outputs see logCommandGroups
**/
AREXPORT void ArServerBase::logCommandGroupsToFile(const char *fileName)
{
  std::map<unsigned int, ArServerData *>::iterator dit;

  std::multimap<std::string, std::string> groups;
  std::multimap<std::string, std::string>::iterator git;

  myDataMutex.lock();
  for (dit = myDataMap.begin(); dit != myDataMap.end(); dit++)
  {
    std::string group;
    std::string command;
    command = (*dit).second->getName();
    if ((*dit).second->getCommandGroup() == NULL)
      group = "";
    else
      group = (*dit).second->getCommandGroup();
    
    groups.insert(std::pair<std::string, std::string>(group, command));
  }

  FILE *file = NULL;
  if (fileName != NULL)
  {
    file = ArUtil::fopen(fileName, "w");
  }

  char descLine[10000];
  std::string line;
  std::string lastGroup;
  bool first = true;
  bool firstGroup = true;
  std::map<std::string, std::string>::iterator dIt; 
  std::string listOfGroups = "Groups";

  for (git = groups.begin(); git != groups.end(); git++)
  {
    if (ArUtil::strcasecmp((*git).first, lastGroup) != 0 || firstGroup)
    {
      if (!firstGroup)
      {
	if (file != NULL)
	  fprintf(file, "%s", line.c_str());
	else
	  ArLog::log(ArLog::Terse, "%s", line.c_str());
      }
      first = true;
      firstGroup = false;
      lastGroup = (*git).first;
      listOfGroups += " ";
      if (lastGroup.size() == 0)
	listOfGroups += "None";
      else
	listOfGroups += lastGroup;
    }
    if (first)
    {
      line = "CommandGroup ";
      if ((*git).first.size() == 0)
      {
	line += "None";
      }
      else
      {
	// output the groups description if therei s one
	if ((dIt = myGroupDescription.find((*git).first)) != 
	    myGroupDescription.end())
	  snprintf(descLine, sizeof(descLine), 
		   "CommandGroup %s has description %s", 
		   (*git).first.c_str(), (*dIt).second.c_str());
	else
	  snprintf(descLine, sizeof(descLine), 
		   "CommandGroup %s has no description", 
		   (*git).first.c_str());
	if (file != NULL)
	  fprintf(file, "%s\n", descLine);
	else
	  ArLog::log(ArLog::Terse, "%s", descLine);

	line += (*git).first.c_str();
      }
      line += " is";


      }
    line += " ";
    line += (*git).second.c_str();
    first = false;
  }
  if (!firstGroup && !first)
  {
    if (file != NULL)
      fprintf(file, "%s\n", line.c_str());
    else
      ArLog::log(ArLog::Terse, "%s", line.c_str());
  }
  if (file != NULL)
    fprintf(file, "%s\n", listOfGroups.c_str());
  else
    ArLog::log(ArLog::Terse, "%s", listOfGroups.c_str());
  myDataMutex.unlock();
}


///     
///=========================================================================///
///   Function: setGroupDescription                                         ///
///   Created:  17-Nov-2005                                                 ///
///   Purpose:  To permit the user to update the Command Group Descriptions.///
///   Notes:                                                                ///
///                                                                         ///
///   Revision History:                                                     ///
///      WHEN       WHO         WHAT and/or WHY                             ///
///    17-Nov-2005   NJ    Created this function.                           ///
///                                                                         ///
///=========================================================================///
///
AREXPORT void ArServerBase::setGroupDescription(const char *cmdGrpName, const char *cmdGrpDesc)
{
  myDataMutex.lock();
  myGroupDescription.insert(std::pair<std::string, std::string>(cmdGrpName, cmdGrpDesc));
  myDataMutex.unlock();
}





//   Created:  17-Nov-2005                                                 //
//   Purpose:  
//   Notes:                                                                //
//             Assumption: Command Group Name Map exists already.          //
//                                                                         //
//   Revision History:                                                     //
//      WHEN       WHO         WHAT and/or WHY                             //
//   17-Nov-2005   NJ    Created this function.                            //

AREXPORT void ArServerBase::logGroupDescriptions(void)
{
  // -- This wrapper insures that NULL is the only input passed to the
  //    logCommandGroupNamesToFile routine.   
  //    This is used if the log is desired over a filename.  
  logGroupDescriptionsToFile(NULL);	// NULL is the only value passed.
}

AREXPORT void ArServerBase::logGroupDescriptionsToFile(const char *fileName)
{ 
  myDataMutex.lock();
  std::map<std::string, std::string>::iterator d_itr;	// establish map iterator		
  char line[1024];
  FILE *file = NULL;
  if (fileName != NULL)
    file = ArUtil::fopen(fileName, "w");			// open file if exists

  // -- Assemble output strings and send to file or log
  for (d_itr = myGroupDescription.begin(); d_itr != myGroupDescription.end(); d_itr++)
  {
    snprintf(line, sizeof(line), "%-29s %s", (*d_itr).first.c_str(), (*d_itr).second.c_str());
    
    if (file != NULL)
      fprintf(file, "%s\n", line);		// send string to file
    else
      ArLog::log(ArLog::Terse, "%s", line);	// send string to log
  }
  if (fileName != NULL)			// task complete notification
    fclose (file);
  myDataMutex.unlock();
}	



/**
   Text string that is the key required when using user and password
   information... this is NOT used if there is no user and password
   information.
**/ 
AREXPORT void ArServerBase::setServerKey(const char *serverKey)
{
  myDataMutex.lock();
  // if this is setting it to empty and its already empty don't print
  // a message
  if ((serverKey == NULL || serverKey[0] == '\0') && myServerKey.size() > 0)
    ArLog::log(ArLog::Normal, "Clearing server key");
  if (serverKey != NULL && serverKey[0] != '\0')
    ArLog::log(ArLog::Normal, "Setting new server key");
  if (serverKey != NULL)
    myServerKey = serverKey;
  else
    myServerKey = "";
  myDataMutex.unlock();
}

AREXPORT void ArServerBase::rejectSinceUsingCentralServer(
	const char *centralServerIPString)
{
  myDataMutex.lock();
  myRejecting = 2;
  myRejectingString = centralServerIPString;
  myDataMutex.unlock();
}

/**
   Sets up the backup timeout, if there are packets to send to clients
   and they haven't been sent for longer than this then the connection
   is closed.  Less than 0 means this won't happen.  If this is
   positive but less than 5 seconds then 5 seconds is used.
**/
AREXPORT void ArServerBase::setBackupTimeout(double timeoutInMins)
{
  myBackupTimeout = timeoutInMins;

  std::list<ArServerClient *>::iterator it;
//  ArLog::log(ArLog::Normal, "Setting server base backup time to %g\n", myBackupTimeout);
  myClientsMutex.lock();
  for (it = myClients.begin(); it != myClients.end(); it++)
    (*it)->setBackupTimeout(myBackupTimeout);
  myClientsMutex.unlock();
}

AREXPORT void ArServerBase::logTracking(bool terse)
{
  std::list<ArServerClient *>::iterator lit;

  myClientsMutex.lock();  

  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
    (*lit)->logTracking(terse);

  ArLog::log(ArLog::Terse, "");
  
  if (!terse)
  {
    ArLog::log(ArLog::Terse, "%-85s %7ld udp rcvs %10ld udp B",
	       "Low Level UDP Received (all conns)", myUdpSocket.getRecvs(), 
	       myUdpSocket.getBytesRecvd());
    ArLog::log(ArLog::Terse, "%-85s %7ld udp snds %10ld udp B",
	       "Low Level UDP Sent (all conns)", myUdpSocket.getSends(), 
	       myUdpSocket.getBytesSent());

    ArLog::log(ArLog::Terse, "");
  }
  myClientsMutex.unlock();  
}

AREXPORT void ArServerBase::resetTracking(void)
{
  std::list<ArServerClient *>::iterator lit;

  myClientsMutex.lock();  

  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
    (*lit)->resetTracking();
  myClientsMutex.unlock();  
  myDataMutex.lock();
  myUdpSocket.resetTracking();
  myDataMutex.unlock();
}

AREXPORT const ArServerUserInfo* ArServerBase::getUserInfo(void) const
{
  return myUserInfo;
}

AREXPORT void ArServerBase::setUserInfo(const ArServerUserInfo *userInfo)
{
  myDataMutex.lock();
  myUserInfo = userInfo;
  myDataMutex.unlock();
}
/**
   @param command the command number, you can use findCommandFromName
   
   @param internalCall whether its an internal call or not (whether to
   lock or not)
   
   @return returns lowest amount of time requested for this packet,
   note that 0 and higher means thats how often it was asked for, -1
   means nothing requested the data at an interval but wants it when
   its been pushed, and -2 means that nothing wants the data
**/

AREXPORT long ArServerBase::getFrequency(unsigned int command,
					 bool internalCall)
{
  std::list<ArServerClient *>::iterator it;
  long ret = -2;
  long clientFreq;

  if (!internalCall)
    myClientsMutex.lock();
  
  for (it = myClients.begin(); it != myClients.end(); it++)
  {
    clientFreq = (*it)->getFrequency(command);
    // if the ret is an interval and so is this client but this client
    // is a smalelr interval
    if (clientFreq >= 0 && (ret < 0 || (ret >= 0 && clientFreq < ret)))
      ret = clientFreq;
    // if this client just wants the data when pushed but ret is still
    // at never then set it to when pushed
    else if (clientFreq == -1 && ret == -2)
      ret = -1;
  }
  
  if (!internalCall)
    myClientsMutex.unlock();

  return ret;
}

/**
   Additional data flags to be added to addData calls (these are added
   in with what is there).  You can do multiple flags by separating
   them with a | character.
 **/
AREXPORT void ArServerBase::setAdditionalDataFlags(
	const char *additionalDataFlags)
{
  myDataMutex.lock();
  if (additionalDataFlags == NULL || additionalDataFlags[0] == '\0')
    myAdditionalDataFlags = "";
  else  
    myAdditionalDataFlags = additionalDataFlags;
  myDataMutex.unlock();
}


AREXPORT bool ArServerBase::dataHasFlag(const char *name, 
					const char *dataFlag)
{
  unsigned int command;
  if ((command = findCommandFromName(name)) == 0)
  {
    ArLog::log(ArLog::Verbose, 
	   "ArServerBase::dataHasFlag: %s is not data that is on the server", 
	       name);
    return false;
  }

  return dataHasFlagByCommand(findCommandFromName(name), dataFlag);
}

AREXPORT bool ArServerBase::dataHasFlagByCommand(unsigned int command, 
						 const char *dataFlag)
{
  std::map<unsigned int, ArServerData *>::iterator dIt;
  bool ret;

  myDataMutex.lock();
  if ((dIt = myDataMap.find(command)) == myDataMap.end())
    ret = false;
  else
    ret = (*dIt).second->hasDataFlag(dataFlag);
  myDataMutex.unlock();
  
  return ret;
}

AREXPORT unsigned int ArServerBase::getTcpPort(void)
{
  return myTcpPort;
}

AREXPORT unsigned int ArServerBase::getUdpPort(void)
{
  return myUdpPort;  
}

AREXPORT const char *ArServerBase::getOpenOnIP(void)
{
  if (myOpenOnIP.empty())
    return NULL;
  else
    return myOpenOnIP.c_str();
}

bool ArServerBase::processFile(void)
{
  setBackupTimeout(myBackupTimeout);
  return true;
}

AREXPORT void ArServerBase::identGetConnectionID(ArServerClient *client, 
						 ArNetPacket *packet)
{
  ArNetPacket sending;
  ArServerClientIdentifier identifier = client->getIdentifier();
  
  // if there is an id then just send that ConnectionID
  if (identifier.getConnectionID() != 0)
  {
    sending.uByte4ToBuf(identifier.getConnectionID());
    client->sendPacketTcp(&sending);
  }
  // otherwise we need to find an ConnectionID for it (don't lock while doing
  // this since the client bases can only be active from within the
  // server base)
  
  // we have the id number, but make sure it isn't already taken
  // (maybe we'll wrap a 4 byte number, but its unlikely, but better
  // safe than sorry)
  bool duplicate;
  std::list<ArServerClient *>::iterator it;
  bool hitZero;

  // this does both the stepping to find a good ID, the for also winds
  // up incrementing it after we find one, which I don't like since
  // its a side effect, but oh well
  for (duplicate = true, hitZero = false;
       duplicate;
       myConnectionNumber++)
  {
    if (myConnectionNumber == 0)
    {
      // if we hit zero once already and are back there, just use 0
      // for an id
      if (hitZero)
      {
	ArLog::log(ArLog::Terse, "ConnectionID hit zero again, giving up");
	break;
      }
      ArLog::log(ArLog::Terse, "ConnectionID lookup wrapped");
      // otherwise set that we hit zero and go to 1
      hitZero = true;
      myConnectionNumber++;
    }

    // see if we have any duplicates
    for (it = myClients.begin(); 
	 it != myClients.end();
	 it++)
    {
      if (myConnectionNumber != (*it)->getIdentifier().getConnectionID())
      {
	duplicate = false;
	break;
      }
    }
  }
  
  identifier.setConnectionID(myConnectionNumber);
  client->setIdentifier(identifier);

  sending.uByte4ToBuf(identifier.getConnectionID());
  client->sendPacketTcp(&sending);
}

AREXPORT void ArServerBase::identSetConnectionID(ArServerClient *client, ArNetPacket *packet)
{
  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setConnectionID(packet->bufToUByte4());
  client->setIdentifier(identifier);
}

AREXPORT void ArServerBase::identSetSelfIdentifier(ArServerClient *client, 
				     ArNetPacket *packet)
{
  char buf[32000];
  packet->bufToStr(buf, sizeof(buf));

  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setSelfIdentifier(buf);
  client->setIdentifier(identifier);
}

AREXPORT void ArServerBase::identSetHereGoal(ArServerClient *client, ArNetPacket *packet)
{
  char buf[32000];
  packet->bufToStr(buf, sizeof(buf));

  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setHereGoal(buf);
  client->setIdentifier(identifier);
}

AREXPORT void ArServerBase::closeConnectionID(ArTypes::UByte4 idNum)
{
  if (idNum == 0)
  {
    ArLog::log(ArLog::Verbose, "%s: Cannot close connection to id 0", 
	       myLogPrefix.c_str());
    return;
  }

  std::list<ArServerClient *>::iterator it;
  // for speed we'd use a list of iterators and erase, but for clarity
  // this is easier and this won't happen that often
  //std::list<ArServerClient *> removeList;
  ArServerClient *client;

  myClientsMutex.lock(); 
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    if (client->getIdentifier().getConnectionID() == idNum)
    {
      client->forceDisconnect(true);
      myRemoveSetMutex.lock();
      myRemoveSet.insert(client);
      myRemoveSetMutex.unlock();
    }
  }
  /*
  while ((it = removeList.begin()) != removeList.end())
  {
    client = (*it);
    if (myDebugLogging)
      ArLog::log(ArLog::Normal,
		 "%sClosing (by connection id) connection to %s", 
		 myLogPrefix.c_str(), client->getIPString());
    myClients.remove(client);
    for (std::list<ArFunctor1<ArServerClient*> *>::iterator rci = myClientRemovedCallbacks.begin();
         rci != myClientRemovedCallbacks.end();
	 rci++) {
      if (*rci) {
        (*rci)->invoke(client);
      }
    }

    delete client;
    removeList.pop_front();
  }
  */
  myClientsMutex.unlock(); 
}

AREXPORT void ArServerBase::logConnections(const char *prefix)
{
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  myClientsMutex.lock(); 

  ArLog::log(ArLog::Normal, "%sConnections for %s (%d now, %d max)", prefix, 
	     myServerName.c_str(), myClients.size(), myMostClients);

  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    ArLog::log(ArLog::Normal, "%s\t%s", prefix, client->getIPString());
  }

  myClientsMutex.unlock(); 
}
/// Sets debug logging 
AREXPORT void ArServerBase::setDebugLogging(bool debugLogging)
{
  myDebugLogging = debugLogging; 
  if (myDebugLogging)
    myVerboseLogLevel = ArLog::Normal;
  else
    myVerboseLogLevel = ArLog::Verbose;
}

AREXPORT bool ArServerBase::getDebugLogging(void)
{
  return myDebugLogging; 
}

AREXPORT int ArServerBase::getMostClients(void)
{ 
  return myMostClients; 
}

void ArServerBase::slowIdleCallback(void)
{
  if (!myOpened)
  {
    ArUtil::sleep(myLoopMSecs);
    return;
  }

  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  std::list<ArFunctor *>::iterator cIt;
  ArFunctor *functor;

  if (myAllowSlowPackets)
  {
    myProcessingSlowIdleMutex.lock();
    //printf("SLOW...\n");
    for (it = myClients.begin(); it != myClients.end(); ++it)
    {
      client = (*it);
      if (!client->slowPacketCallback())
      {
	client->forceDisconnect(true);
	myRemoveSetMutex.lock();
	myRemoveSet.insert(client);
	myRemoveSetMutex.unlock();
      }
    }
    //printf("done slow...\n");
    myProcessingSlowIdleMutex.unlock();
  }

  int activityTime = ArServerMode::getActiveModeActivityTimeSecSince();
  if (myAllowIdlePackets && (activityTime < 0 || activityTime > 1))
  {
    /*
    if (idleProcessingPending() && ArServerMode::getActiveMode() != NULL)
      ArLog::log(myVerboseLogLevel, "Idle processing pending, idle %ld msecs", 
		 ArServerMode::getActiveMode()->getActivityTime().mSecSince());
    */

    //printf("IDLE...\n");

    myProcessingSlowIdleMutex.lock();

    myIdleCallbacksMutex.lock();
    if (myHaveIdleCallbacks)
    {
      while ((cIt = myIdleCallbacks.begin()) != myIdleCallbacks.end())
      {
	functor = (*cIt);
	myIdleCallbacks.pop_front();
	myIdleCallbacksMutex.unlock();
	if (functor->getName() != NULL && functor->getName()[0] != '\0')
	  ArLog::log(myVerboseLogLevel, "Calling idle callback functor %s", 
		     functor->getName());
	else
	  ArLog::log(myVerboseLogLevel, 
		     "Calling anonymous idle callback functor");
	functor->invoke();
	myIdleCallbacksMutex.lock();
      }
      myHaveIdleCallbacks = false;
    }
    myIdleCallbacksMutex.unlock();

    for (it = myClients.begin(); it != myClients.end(); ++it)
    {
      client = (*it);
      if (!client->idlePacketCallback())
      {
	client->forceDisconnect(true);
	myRemoveSetMutex.lock();
	myRemoveSet.insert(client);
	myRemoveSetMutex.unlock();
      }
    }
    //printf("done idle...\n");
    myProcessingSlowIdleMutex.unlock();
  }
  ArUtil::sleep(myLoopMSecs);
}

AREXPORT bool ArServerBase::hasSlowPackets(void)
{
  return myHaveSlowPackets; 
}

AREXPORT bool ArServerBase::hasIdlePackets(void)
{
  return myHaveIdlePackets; 
}

AREXPORT bool ArServerBase::hasIdleCallbacks(void)
{
  return myHaveIdleCallbacks;
}

AREXPORT bool ArServerBase::idleProcessingPending(void)
{
  return myHaveIdleCallbacks || myHaveIdlePackets;
}

AREXPORT void ArServerBase::netIdleProcessingPending(ArServerClient *client,
						     ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  
  if (idleProcessingPending())
    sendingPacket.byteToBuf(1);
  else
    sendingPacket.byteToBuf(0);

  client->sendPacketTcp(&sendingPacket);
}

AREXPORT bool ArServerBase::allowingIdlePackets(void)
{
  return myAllowIdlePackets;
}

/**
   Note that if idle packets are not allowed then idle callbacks will
   not work.
   
   @return true if the callback was added, false if not
**/
AREXPORT bool ArServerBase::addIdleSingleShotCallback(ArFunctor *functor)
{
  if (!myAllowIdlePackets)
    return false;

  myIdleCallbacksMutex.lock();
  myIdleCallbacks.push_back(functor);
  myHaveIdleCallbacks = true;
  if (functor->getName() != NULL && functor->getName()[0] != '\0')
    ArLog::log(myVerboseLogLevel, 
	       "Adding idle singleshot callback functor %s", 
	       functor->getName());
  else
    ArLog::log(myVerboseLogLevel, 
	       "Adding anonymous idle singleshot callback functor");
  myIdleCallbacksMutex.unlock();
  return true;
}

ArServerBase::SlowIdleThread::SlowIdleThread(ArServerBase *serverBase)
{
  setThreadName("ArServerBase::SlowIdleThread");
  myServerBase = serverBase;
  runAsync();
}

ArServerBase::SlowIdleThread::~SlowIdleThread()
{
  
}

void * ArServerBase::SlowIdleThread::runThread(void *arg)
{
  threadStarted();

  while (getRunning())
  {
    myServerBase->slowIdleCallback();
  }
  
  threadFinished();
  return NULL;
}
