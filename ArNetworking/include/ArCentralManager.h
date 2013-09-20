#ifndef ARCENTRALMANAGER
#define ARCENTRALMANAGER

#include "Aria.h"
#include "ArServerBase.h"
#include "ArCentralForwarder.h"

class ArCentralManager : public ArASyncTask
{
public:
  /// Constructor
  AREXPORT ArCentralManager(ArServerBase *robotServer, ArServerBase *clientServer);
  /// Destructor
  AREXPORT virtual ~ArCentralManager();
  /// Logs all the connection information
  void logConnections(void);
  /// Enforces that everything is using this protocol version
  AREXPORT void enforceProtocolVersion(const char *protocolVersion);
  /// Enforces that the robots that connect are this type
  AREXPORT void enforceType(ArServerCommands::Type type);
  /// Adds a callback for when a new forwarder is added
  AREXPORT void addForwarderAddedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor, int priority = 0);
  /// Removes a callback for when a new forwarder is added
  AREXPORT void remForwarderAddedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor);
  /// Adds a callback for when a new forwarder is destroyed
  AREXPORT void addForwarderRemovedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor, int priority = 0);
  /// Removes a callback for when a new forwarder is destroyed
  AREXPORT void remForwarderRemovedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor);	  
  /// Networking command to get the list of clients
  AREXPORT void netClientList(ArServerClient *client, ArNetPacket *packet);
  /// A callback so we can tell the main connection happened when a
  /// client is removed
  AREXPORT void forwarderServerClientRemovedCallback(
	  ArCentralForwarder *forwarder, ArServerClient *client);  
  /// A callback so we can close down other connetions when a main
  /// client loses connection
  AREXPORT void mainServerClientRemovedCallback(ArServerClient *client);  
  /// Networking command to switch the direction of a connection
  AREXPORT void netServerSwitch(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void *runThread(void *arg);
protected:
  void close(void);
  bool processFile(void);

  bool removePendingDuplicateConnections(const char *robotName);

  ArServerBase *myRobotServer;
  ArServerBase *myClientServer;
  double myHeartbeatTimeout;
  double myUdpHeartbeatTimeout;
  double myRobotBackupTimeout;
  double myClientBackupTimeout;

  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  int myMostForwarders;
  int myMostClients;

  ArTypes::UByte4 myClosingConnectionID;
  std::list<ArSocket *> myClientSockets;
  std::list<std::string> myClientNames;
  std::list<ArCentralForwarder *> myForwarders;
  std::map<int, ArTime *> myUsedPorts;
  ArMutex myCallbackMutex;
  std::multimap<int, 
      ArFunctor1<ArCentralForwarder *> *> myForwarderAddedCBList;
  std::multimap<int, 
      ArFunctor1<ArCentralForwarder *> *> myForwarderRemovedCBList;
  ArMutex myDataMutex;
  int myOnSocket;
  ArFunctor2C<ArCentralManager, ArServerClient *, 
      ArNetPacket *> myNetSwitchCB;
  ArFunctor2C<ArCentralManager, ArServerClient *, 
      ArNetPacket *> myNetClientListCB;
  ArFunctorC<ArCentralManager> myAriaExitCB;
  ArRetFunctorC<bool, ArCentralManager> myProcessFileCB;
  ArFunctor2C<ArCentralManager, ArCentralForwarder *, 
	      ArServerClient *> myForwarderServerClientRemovedCB;
  ArFunctor1C<ArCentralManager, ArServerClient *> myMainServerClientRemovedCB;
};


#endif // ARSERVERSWITCHMANAGER
