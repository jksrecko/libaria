#include "Aria.h"
#include "ArExport.h"
#include "ArCentralManager.h"


ArCentralManager::ArCentralManager(ArServerBase *robotServer, 
			       ArServerBase *clientServer) :
  myNetSwitchCB(this, &ArCentralManager::netServerSwitch),
  myNetClientListCB(this, &ArCentralManager::netClientList),
  myAriaExitCB(this, &ArCentralManager::close),
  myProcessFileCB(this, &ArCentralManager::processFile),
  myForwarderServerClientRemovedCB(
	  this, &ArCentralManager::forwarderServerClientRemovedCallback),
  myMainServerClientRemovedCB(
	  this, &ArCentralManager::mainServerClientRemovedCallback)
{
  myMutex.setLogName("ArCentralManager::myCallbackMutex");
  myDataMutex.setLogName("ArCentralManager::myDataMutex");
  setThreadName("ArCentralManager");

  myRobotServer = robotServer;
  myClientServer = clientServer;
  
  myAriaExitCB.setName("ArCentralManager");
  Aria::addExitCallback(&myAriaExitCB, 25);

  myClientBackupTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("CentralServerToClientTimeoutInMins", 
		      &myClientBackupTimeout,
		      "The amount of time the central server can go without sending a packet to the robot successfully (when there are packets to send).  A number less than 0 means this won't happen.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);

  myRobotBackupTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("CentralServerToRobotTimeoutInMins", 
		      &myRobotBackupTimeout,
		      "The amount of time the central server can go without sending a packet to the robot successfully (when there are packets to send).  A number less than 0 means this won't happen.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);
  
  myHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("CentralServerFromRobotTimeoutInMins", 
		      &myHeartbeatTimeout,
		      "The amount of time the central server can go without hearing a robot's heartbeat without disconnecting it.  A number less than 0 means that the robots will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);

  myUdpHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("CentralServerFromRobotUdpTimeoutInMins", 
		      &myUdpHeartbeatTimeout,
		      "The amount of time the central server can go without hearing a robot's udp heartbeat without disconnecting it (this fails it over to tcp only).  A number less than 0 means that the robots will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);


  myProcessFileCB.setName("ArCentralManager");
  Aria::getConfig()->addProcessFileCB(&myProcessFileCB, -999);

  myRobotServer->addData("switch", "switches the direction of the connection, after this is requested it sends an empty packet denoting acceptance of the switch, then switches this to a client connection",
			 &myNetSwitchCB, "string: robotName", "empty packet", "RobotInfo", 
			 "RETURN_SINGLE");

  myRobotServer->addData("centralServerHeartbeat", "Just a data to let the robot's know that this server has the centralServerHeartbeat feature (nothing is actually done with this command)",
			 NULL, "none", "none", "RobotInfo", 
			 "RETURN_NONE");

  myClientServer->addData("clientList", "Lists the clients that are connected",
			  &myNetClientListCB, "none", 
			  "ubyte2: numClients; repeating for <numClients> [string: hostname (empty means this host); ubyte2: port; string: robot name; string: flags; string: robot ip address]",
			  "RobotInfo", "RETURN_SINGLE");
  myClientServer->addData("clientAdded", "Broadcast when a client is added",
			  NULL, "none", 
			  "string: hostname (empty means this host); ubyte2: port; string: robot name; string: flags; string: robot ip address",
			  "RobotInfo", "RETURN_SINGLE");
  myClientServer->addData("clientRemoved", "Broadcast when a client is removed",
			  NULL, "none", 
			  "string: hostname (empty means this host); ubyte2: port; string: robot name; string: flags; string: robot ip address",
			  "RobotInfo", "RETURN_SINGLE");

  myClientServer = clientServer;

  myClosingConnectionID = 0;

  myMostForwarders = 0;
  myMostClients = 0;

  myForwarderServerClientRemovedCB.setName("ArCentralManager");


  myMainServerClientRemovedCB.setName("ArCentralManager");
  myClientServer->addClientRemovedCallback(&myMainServerClientRemovedCB);

  runAsync();
}

ArCentralManager::~ArCentralManager()
{
}

void ArCentralManager::close(void)
{
  std::list<ArCentralForwarder *>::iterator fIt;
  ArCentralForwarder *forwarder;

  ArLog::log(ArLog::Normal, "ArCentralManager closing");
  //myDataMutex.lock();
  while ((fIt = myForwarders.begin()) != myForwarders.end())
  {
    forwarder = (*fIt);
    std::multimap<int, ArFunctor1<ArCentralForwarder *> *>::iterator it;
    for (it = myForwarderRemovedCBList.begin();
	 it != myForwarderRemovedCBList.end();
	 it++)
      (*it).second->invoke(forwarder);

    myForwarders.pop_front();
    delete forwarder;

  }
  //myDataMutex.unlock();
  ArLog::log(ArLog::Normal, "ArCentralManager closed");
}

void ArCentralManager::netServerSwitch(ArServerClient *client, ArNetPacket *packet)
{
  char robotName[512];
  std::string uniqueName;
  
  robotName[0] = '\0';

  //printf("robotName before '%s'\n", robotName);

  packet->bufToStr(robotName, sizeof(robotName));

  ArNetPacket sendPacket;
  /// acknowledge the switch
  client->sendPacketTcp(&sendPacket);

  ArSocket *clientSocket = new ArSocket;
  clientSocket->transfer(client->getTcpSocket());
  
  client->tcpCallback();
  client->forceDisconnect(true);

  
  // make the basis of the unique name
  if (robotName[0] != '\0')
    uniqueName = robotName;
  else
    uniqueName = client->getIPString();

  //printf("Starting with '%s'\n", uniqueName.c_str());

  myDataMutex.lock();
  // walk through and make the name unique
  bool nameIsUnique = false;
  std::list<ArCentralForwarder *>::iterator fIt;
  ArCentralForwarder *forwarder;
  while (!nameIsUnique)
  {
    nameIsUnique = true;
    for (fIt = myForwarders.begin(); 
	 fIt != myForwarders.end() && nameIsUnique; 
	 fIt++)
    {
      forwarder = (*fIt);
      if (strcasecmp(forwarder->getRobotName(), uniqueName.c_str()) == 0)
	nameIsUnique = false;
    }
    if (!nameIsUnique)
      uniqueName += "_";
  }
  myClientSockets.push_back(clientSocket);
  myClientNames.push_back(uniqueName);

  //printf("robotName after '%s'\n", robotName);
  // just print out the switch request name if its the same as it came in as
  if (strcmp(uniqueName.c_str(), robotName) == 0)
    ArLog::log(ArLog::Normal, "Got switch request from %s from %s", 
	       client->getIPString(), uniqueName.c_str());
  // otherwise let them know what the unique name is
  if (strcmp(uniqueName.c_str(), robotName) != 0)
    ArLog::log(ArLog::Normal, 
	       "Got switch request from %s from %s that started as %s", 
	       client->getIPString(), uniqueName.c_str(), robotName);
  //printf("Ended with '%s'\n", uniqueName.c_str());
  myDataMutex.unlock();
}

void ArCentralManager::netClientList(ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  std::list<ArCentralForwarder *>::iterator fIt;
  ArCentralForwarder *forwarder;
  ArTypes::UByte2 sizeLen;
  ArTypes::UByte2 realLen;
  unsigned int numConnected = 0;

  myDataMutex.lock();
  sizeLen = sendPacket.getLength();
  sendPacket.uByte2ToBuf(0);
  for (fIt = myForwarders.begin(); fIt != myForwarders.end(); fIt++)
  {
    forwarder = (*fIt);

    if (!forwarder->isConnected())
      continue;

    numConnected++;
    sendPacket.strToBuf("");
    sendPacket.uByte2ToBuf(forwarder->getPort());
    sendPacket.strToBuf(forwarder->getRobotName());
    sendPacket.strToBuf("");
    sendPacket.strToBuf(
	    forwarder->getClient()->getTcpSocket()->getIPString());
  }

  realLen = sendPacket.getLength();
  sendPacket.setLength(sizeLen);
  sendPacket.uByte2ToBuf(numConnected);
  sendPacket.setLength(realLen);

  myDataMutex.unlock();
  client->sendPacketTcp(&sendPacket);
}

/// This should be its own thread here
void *ArCentralManager::runThread(void *arg)
{
  std::list<ArSocket *>::iterator sIt;
  std::list<std::string>::iterator nIt;
  std::list<ArCentralForwarder *>::iterator fIt;
  ArSocket *socket;
  std::string robotName;
  ArCentralForwarder *forwarder;

  ArNetPacket *packet;
  std::list<ArNetPacket *> addPackets;
  std::list<ArNetPacket *> remPackets;


  threadStarted();
  while (getRunning())
  {
    int numForwarders = 0;
    int numClients = 0;
    
    myDataMutex.lock();
    while ((sIt = myClientSockets.begin()) != myClientSockets.end() && 
	   (nIt = myClientNames.begin()) != myClientNames.end())
    {
      socket = (*sIt);
      robotName = (*nIt);

      myClientSockets.pop_front();
      myClientNames.pop_front();

      ArLog::log(ArLog::Normal, 
		 "New forwarder for socket from %s with name %s", 
		 socket->getIPString(), robotName.c_str());

      forwarder = new ArCentralForwarder(myClientServer, socket, 
					 robotName.c_str(), 
					 myClientServer->getTcpPort() + 1, 
					 &myUsedPorts, 
					 &myForwarderServerClientRemovedCB);
      myForwarders.push_back(forwarder);
    }

    numClients += myRobotServer->getNumClients();
    if (myRobotServer != myClientServer)
      numClients += myClientServer->getNumClients();

    std::list<ArCentralForwarder *> connectedRemoveList;
    std::list<ArCentralForwarder *> unconnectedRemoveList;
    for (fIt = myForwarders.begin(); fIt != myForwarders.end(); fIt++)
    {
      forwarder = (*fIt);

      numForwarders++;
      if (forwarder->getServer() != NULL)
	numClients += forwarder->getServer()->getNumClients();
      bool connected = forwarder->isConnected();
      bool removed = false;
      if (!forwarder->callOnce(myHeartbeatTimeout, myUdpHeartbeatTimeout,
			       myRobotBackupTimeout, myClientBackupTimeout))
      {
	if (connected)
	{
	  ArLog::log(ArLog::Normal, "Will remove forwarder from %s", 
		     forwarder->getRobotName());
	  connectedRemoveList.push_back(forwarder);
	  removed = true;
	}
	else
	{
	  ArLog::log(ArLog::Normal, "Failed to connect to forwarder from %s", 
		     forwarder->getRobotName());
	  unconnectedRemoveList.push_back(forwarder);
	  removed = true;
	}
      }
      if (!connected && !removed && forwarder->isConnected())
      {
	ArLog::log(ArLog::Normal, "Adding forwarder %s", 
		   forwarder->getRobotName());
	myUsedPorts.insert(forwarder->getPort());
	
	std::multimap<int, ArFunctor1<ArCentralForwarder *> *>::iterator it;
	for (it = myForwarderAddedCBList.begin();
	     it != myForwarderAddedCBList.end();
	     it++)
	{
	  if ((*it).second->getName() == NULL || 
	      (*it).second->getName()[0] == '\0')
	    ArLog::log(ArLog::Normal, "Calling unnamed add functor at %d",
		       -(*it).first);
	  else
	    ArLog::log(ArLog::Normal, "Calling %s add functor at %d",
		       (*it).second->getName(), -(*it).first);
	  (*it).second->invoke(forwarder);
	}
	ArLog::log(ArLog::Normal, "Added forwarder %s", 
		   forwarder->getRobotName());      
	ArNetPacket *sendPacket = new ArNetPacket;
	sendPacket->strToBuf("");
	sendPacket->uByte2ToBuf(forwarder->getPort());
	sendPacket->strToBuf(forwarder->getRobotName());
	sendPacket->strToBuf("");
	sendPacket->strToBuf(
		forwarder->getClient()->getTcpSocket()->getIPString());
	addPackets.push_back(sendPacket);
	// MPL added this at the same time as the changes for the deadlock that happened down below
	//myClientServer->broadcastPacketTcp(&sendPacket, "clientAdded");
      }
    }

    while ((fIt = connectedRemoveList.begin()) != connectedRemoveList.end())
    {
      forwarder = (*fIt);

      ArLog::log(ArLog::Normal, "Removing forwarder %s", 
		 forwarder->getRobotName());
      std::multimap<int, ArFunctor1<ArCentralForwarder *> *>::iterator it;
      for (it = myForwarderRemovedCBList.begin();
	   it != myForwarderRemovedCBList.end();
	   it++)
      {
	if ((*it).second->getName() == NULL || 
	    (*it).second->getName()[0] == '\0')
	  ArLog::log(ArLog::Normal, "Calling unnamed remove functor at %d",
		     -(*it).first);
	else
	  ArLog::log(ArLog::Normal, "Calling %s remove functor at %d",
		     (*it).second->getName(), -(*it).first);
	(*it).second->invoke(forwarder);
      }

      ArLog::log(ArLog::Normal, "Called forwarder removed for %s", 
		 forwarder->getRobotName());
      ArNetPacket *sendPacket = new ArNetPacket;
      sendPacket->strToBuf("");
      sendPacket->uByte2ToBuf(forwarder->getPort());
      sendPacket->strToBuf(forwarder->getRobotName());
      sendPacket->strToBuf("");
      sendPacket->strToBuf(
	      forwarder->getClient()->getTcpSocket()->getIPString());
      // MPL making this just push packets into a list to broadcast at the end since this was deadlocking
      //myClientServer->broadcastPacketTcp(&sendPacket, "clientRemoved");
      remPackets.push_back(sendPacket);

      myUsedPorts.erase(forwarder->getPort());
      myForwarders.remove(forwarder);
      delete forwarder;
      connectedRemoveList.pop_front();
      ArLog::log(ArLog::Normal, "Removed forwarder");

    }

    while ((fIt = unconnectedRemoveList.begin()) != 
	   unconnectedRemoveList.end())
    {
      forwarder = (*fIt);
      ArLog::log(ArLog::Normal, "Removing unconnected forwarder %s", 
		 forwarder->getRobotName());
      myUsedPorts.erase(forwarder->getPort());
      myForwarders.remove(forwarder);
      delete forwarder;
      unconnectedRemoveList.pop_front();
      ArLog::log(ArLog::Normal, "Removed unconnected forwarder");
    }


    if (numForwarders > myMostForwarders)
      myMostForwarders = numForwarders;
    if (numClients > myMostClients)
      myMostClients = numClients;

    myDataMutex.unlock();

    while (addPackets.begin() != addPackets.end())
    {
      packet = addPackets.front();
      myClientServer->broadcastPacketTcp(packet, "clientAdded");
      addPackets.pop_front();
      delete packet;
    }

    while (remPackets.begin() != remPackets.end())
    {
      packet = remPackets.front();
      myClientServer->broadcastPacketTcp(packet, "clientRemoved");
      remPackets.pop_front();
      delete packet;
    }

    ArUtil::sleep(1);
  }

  threadFinished();
  return NULL;
}


AREXPORT void ArCentralManager::addForwarderAddedCallback(
	ArFunctor1<ArCentralForwarder *> *functor, int priority)
{
  myCallbackMutex.lock();
  myForwarderAddedCBList.insert(
	  std::pair<int, ArFunctor1<ArCentralForwarder *> *>(-priority, 
								  functor));
  myCallbackMutex.unlock();
}

AREXPORT void ArCentralManager::remForwarderAddedCallback(
	ArFunctor1<ArCentralForwarder *> *functor)
{
  myCallbackMutex.lock();
  
  std::multimap<int, ArFunctor1<ArCentralForwarder *> *>::iterator it;

  for (it = myForwarderAddedCBList.begin(); 
       it != myForwarderAddedCBList.end(); 
       ++it)
  {
    if ((*it).second == functor)
    {
      myForwarderAddedCBList.erase(it);
      myCallbackMutex.unlock();
      remForwarderAddedCallback(functor);
      return;
    }
  }
  myCallbackMutex.unlock();
}

AREXPORT void ArCentralManager::addForwarderRemovedCallback(
	ArFunctor1<ArCentralForwarder *> *functor, int priority)
{
  myCallbackMutex.lock();
  myForwarderRemovedCBList.insert(
	  std::pair<int, ArFunctor1<ArCentralForwarder *> *>(-priority, 
								  functor));
  myCallbackMutex.unlock();
}

AREXPORT void ArCentralManager::remForwarderRemovedCallback(
	ArFunctor1<ArCentralForwarder *> *functor)
{
  myCallbackMutex.lock();
  std::multimap<int, ArFunctor1<ArCentralForwarder *> *>::iterator it;

  for (it = myForwarderRemovedCBList.begin(); 
       it != myForwarderRemovedCBList.end(); 
       ++it)
  {
    if ((*it).second == functor)
    {
      myForwarderRemovedCBList.erase(it);
      myCallbackMutex.unlock();
      remForwarderRemovedCallback(functor);
      return;
    }
  }
  myCallbackMutex.unlock();
}

bool ArCentralManager::processFile(void)
{
  // this should be okay if it isn't locked since it just sets an int,
  //and the client server can't go away, and its client tcp sender
  //can't either...  and if it locks here when the config changes from
  //inside of it calling the clients, then its a deadlock
  
  //myDataMutex.lock();
  if (myClientServer != NULL)
    myClientServer->setBackupTimeout(myClientBackupTimeout);
  //myDataMutex.unlock();
  return true;
}

AREXPORT void ArCentralManager::forwarderServerClientRemovedCallback(
	ArCentralForwarder *forwarder, ArServerClient *client)
{
  // if this matches we're already closing this so don't do/print anything
  if (myClosingConnectionID != 0 && 
      myClosingConnectionID == client->getIdentifier().getConnectionID())
    return;

  ArLog::log(ArLog::Normal, 
	     "Notifying main server of removal of serverClient for %s %s", 
	     forwarder->getRobotName(), client->getIPString());

  ArNetPacket sendPacket;
  sendPacket.strToBuf("");
  sendPacket.uByte2ToBuf(forwarder->getPort());
  sendPacket.strToBuf(forwarder->getRobotName());
  sendPacket.strToBuf("");
  sendPacket.strToBuf(
	  forwarder->getClient()->getTcpSocket()->getIPString());
  myClientServer->broadcastPacketTcpToMatching(&sendPacket, "clientRemoved",
					       client->getIdentifier(), true);
  myClientServer->broadcastPacketTcpToMatching(&sendPacket, "clientAdded",
					       client->getIdentifier(), true);
}

AREXPORT void ArCentralManager::mainServerClientRemovedCallback(
	ArServerClient *client)
{
  ArTypes::UByte4 idNum;
  if ((idNum = client->getIdentifier().getConnectionID()) == 0)
    return;

  myDataMutex.lock();
  myClosingConnectionID = idNum;
  ArLog::log(ArLog::Normal, "ArCentralManager: Lost main connection to %s, closing robot connections to it", client->getIPString());
  std::list<ArCentralForwarder *>::iterator fIt;  
  ArCentralForwarder *forwarder;
  for (fIt = myForwarders.begin(); fIt != myForwarders.end(); fIt++)
  {
    forwarder = (*fIt);
    if (forwarder->getServer() != NULL)
      forwarder->getServer()->closeConnectionID(myClosingConnectionID);
  }
  myClosingConnectionID = 0;
  myDataMutex.unlock();
}


void ArCentralManager::logConnections(void)
{
  int numServerClients = 0;
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, "Logging connection information:");
  ArLog::log(ArLog::Normal, "Main client server:");
  myClientServer->logConnections("\t");
  ArLog::log(ArLog::Normal, "");
  numServerClients += myClientServer->getNumClients();
  myDataMutex.lock();
  std::list<ArCentralForwarder *>::iterator fIt;  
  ArCentralForwarder *forwarder;
  for (fIt = myForwarders.begin(); fIt != myForwarders.end(); fIt++)
  {
    forwarder = (*fIt);
    if (forwarder->isConnected())
    {
      ArLog::log(ArLog::Normal, "Forwarded connection to %s:", 
		 forwarder->getRobotName());
      forwarder->getServer()->logConnections("\t");
      numServerClients += forwarder->getServer()->getNumClients();
      ArLog::log(ArLog::Normal, "");
    }
    else
    {
      ArLog::log(ArLog::Normal, "Connecting forwarded connection to %s:", 
		 forwarder->getRobotName());
      ArLog::log(ArLog::Normal, "");
    }
  }
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, "Forwarders: %d now %d max",
	     myForwarders.size(), myMostForwarders);
  ArLog::log(ArLog::Normal, "Clients: %d now %d max", 
	     numServerClients, myMostClients);
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, "");
  myDataMutex.unlock();

}
