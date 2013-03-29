#ifndef ARCENTRALFORWARDER_H
#define ARCENTRALFORWARDER_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArClientBase.h"

/**
   Class 
**/
class ArCentralForwarder 
{
public:
  AREXPORT ArCentralForwarder(
	  ArServerBase *mainServer, ArSocket *socket,
	  const char *robotName, int startingPort, std::set<int> *usedPorts,
	  ArFunctor2<ArCentralForwarder *,
	  ArServerClient *> *notifyServerClientRemovedCB);
  AREXPORT ~ArCentralForwarder();
  ArServerBase *getServer(void) { return myServer; }
  ArClientBase *getClient(void) { return myClient; }
  int getPort(void) { return myPort; }
  const char *getRobotName(void) { return myRobotName.c_str(); }
  AREXPORT void netCentralHeartbeat(ArNetPacket *packet);
  AREXPORT bool callOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool isConnected(void) { return myState == STATE_CONNECTED; }
protected:
  void robotServerClientRemoved(ArServerClient *client);
  void clientServerClientRemoved(ArServerClient *client);
  void receiveData(ArNetPacket *packet);
  void requestChanged(long interval, unsigned int command);
  void requestOnce(ArServerClient *client, ArNetPacket *packet);

  AREXPORT bool startingCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool connectingCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool gatheringCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool connectedCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);

  ArServerBase *myMainServer;
  ArSocket *mySocket;
  std::string myRobotName;
  std::string myPrefix;
  int myStartingPort;
  std::set<int> *myUsedPorts;
  ArFunctor2<ArCentralForwarder *,
	     ArServerClient *> *myForwarderServerClientRemovedCB;

  enum State
  {
    STATE_STARTING,
    STATE_CONNECTING,
    STATE_GATHERING,
    STATE_CONNECTED
  };

  ArServerBase *myServer;
  ArClientBase *myClient;
  State myState;
  int myPort;
  ArServerBase *server;
  ArClientBase *client;

  bool myRobotHasCentralServerHeartbeat;
  ArTime myLastSentCentralServerHeartbeat;

  enum ReturnType
  {
    RETURN_NONE,
    RETURN_SINGLE,
    RETURN_VIDEO,
    RETURN_UNTIL_EMPTY,
    RETURN_COMPLEX,
    RETURN_VIDEO_OPTIM,
  };

  std::map<unsigned int, ReturnType> myReturnTypes;
  std::map<unsigned int, std::list<ArServerClient *> *> myRequestOnces;
  std::map<unsigned int, ArTime *> myLastRequest;
  std::map<unsigned int, ArTime *> myLastBroadcast;

  ArTime myLastTcpHeartbeat;
  ArTime myLastUdpHeartbeat;

  ArFunctor1C<ArCentralForwarder, ArNetPacket *> myReceiveDataFunctor;
  ArFunctor2C<ArCentralForwarder, 
	      long, unsigned int> myRequestChangedFunctor;
  ArFunctor2C<ArCentralForwarder, 
	      ArServerClient *, ArNetPacket *> myRequestOnceFunctor;
  ArFunctor1C<ArCentralForwarder, 
      ArServerClient *> myRobotServerClientRemovedCB;
  ArFunctor1C<ArCentralForwarder, 
      ArNetPacket *> myNetCentralHeartbeatCB;
  ArFunctor1C<ArCentralForwarder, 
	      ArServerClient *> myClientServerClientRemovedCB;
  
};


#endif // ARSERVERSWITCHFORWARDER
