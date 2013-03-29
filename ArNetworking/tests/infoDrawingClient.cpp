#include "Aria.h"
#include "ArNetworking.h"

ArClientBase client;

void drawingData(ArNetPacket *packet)
{
  int x, y;
  int numReadings;
  int i;

  numReadings = packet->bufToByte4();

  if (numReadings == 0)
  {
    printf("No readings for sensor %s\n", client.getName(packet));
  }
  printf("Readings for %s:", client.getName(packet));
  for (i = 0; i < numReadings; i++)
  {
    x = packet->bufToByte4();
    y = packet->bufToByte4();
    printf(" (%d %d)", x, y);
  }
  printf("\n\n");
}

ArGlobalFunctor1<ArNetPacket *> drawingDataCB(&drawingData);

void drawing(ArNetPacket *packet)
{
  int numDrawings;
  int i;
  char name[512];
  char shape[512];
  long primary, size, layer, secondary;
  unsigned long refresh;
  numDrawings = packet->bufToByte4();
  ArLog::log(ArLog::Normal, "There are %d drawings", numDrawings);
  for (i = 0; i < numDrawings; i++)
  {
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(shape, sizeof(shape));
    primary = packet->bufToByte4();
    size = packet->bufToByte4();
    layer = packet->bufToByte4();
    refresh = packet->bufToByte4();
    secondary = packet->bufToByte4();
    ArLog::log(ArLog::Normal, "name %-20s shape %-20s", name, shape);
    ArLog::log(ArLog::Normal, 
	       "\tprimary %08x size %2d layer %2d refresh %4u secondary %08x",
	       primary, size, layer, refresh, secondary);
    client.addHandler(name, &drawingDataCB);
    client.request(name, refresh);
  }
  
}

int main(int argc, char **argv)
{

  std::string hostname;
  ArGlobalFunctor1<ArNetPacket *> drawingCB(&drawing);
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);



  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  
  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");

  client.addHandler("listDrawings", &drawingCB);
  client.requestOnce("listDrawings");
  
  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}
