/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004, 2005 ActivMedia Robotics LLC
Copyright (C) 2006, 2007, 2008, 2009, 2010 MobileRobots Inc.
Copyright (C) 2011, 2012, 2013 Adept Technology

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
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
#include "ArExport.h"
#include "ariaOSDef.h"
#include "ArDeviceConnection.h"
#include "ArRobotPacketReceiver.h"
#include "ArLogFileConnection.h"
#include "ArLog.h"
#include "ariaUtil.h"


/**
   @param allocatePackets whether to allocate memory for the packets before
   returning them (true) or to just return a pointer to an internal 
   packet (false)... most everything should use false as this will help prevent
   many memory leaks or corruptions
   @param sync1 first byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
   @param sync2 second byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
*/
AREXPORT ArRobotPacketReceiver::ArRobotPacketReceiver(bool allocatePackets,
						      unsigned char sync1,
						      unsigned char sync2) : 
  myPacket(sync1, sync2)
{
  myAllocatePackets = allocatePackets;
	myTracking = false;
	myTrackingLogName.clear();
  myDeviceConn = NULL;
  mySync1 = sync1;
  mySync2 = sync2;
  myPacketReceivedCallback = NULL;
}

/**
   @param deviceConnection the connection which the receiver will use
   @param allocatePackets whether to allocate memory for the packets before
   returning them (true) or to just return a pointer to an internal 
   packet (false)... most everything should use false as this will help prevent
   many memory leaks or corruptions
   @param sync1 first byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
   @param sync2 second byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
*/
AREXPORT ArRobotPacketReceiver::ArRobotPacketReceiver(
	ArDeviceConnection *deviceConnection, bool allocatePackets,
	unsigned char sync1, unsigned char sync2) :
  myPacket(sync1, sync2)
{
  myDeviceConn = deviceConnection;
	myTracking = false;
	myTrackingLogName.clear();
  myAllocatePackets = allocatePackets;
  mySync1 = sync1;
  mySync2 = sync2;
  myPacketReceivedCallback = NULL;
}

/**
   @param deviceConnection the connection which the receiver will use
   @param allocatePackets whether to allocate memory for the packets before
   returning them (true) or to just return a pointer to an internal 
   packet (false)... most everything should use false as this will help prevent
   many memory leaks or corruptions
   @param sync1 first byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
   @param sync2 second byte of the header this receiver will receive, this 
   should be left as the default in nearly all cases, ie don't mess with it
   @param tracking if true write log messages for packets received
   @param trackingLogName name to include for packets with tracking log messages
*/
AREXPORT ArRobotPacketReceiver::ArRobotPacketReceiver(
	ArDeviceConnection *deviceConnection, bool allocatePackets,
	unsigned char sync1, unsigned char sync2, bool tracking,
	const char *trackingLogName) :
  myPacket(sync1, sync2),
	myTracking(tracking),
	myTrackingLogName(trackingLogName)
{
  myDeviceConn = deviceConnection;
  myAllocatePackets = allocatePackets;
  mySync1 = sync1;
  mySync2 = sync2;
  myPacketReceivedCallback = NULL;
}

AREXPORT ArRobotPacketReceiver::~ArRobotPacketReceiver() 
{
  
}

AREXPORT void ArRobotPacketReceiver::setDeviceConnection(
	ArDeviceConnection *deviceConnection)
{
  myDeviceConn = deviceConnection;
}

AREXPORT ArDeviceConnection *ArRobotPacketReceiver::getDeviceConnection(void)
{
  return myDeviceConn;
}

/**
    @param msWait how long to block for the start of a packet, nonblocking if 0
    @return NULL if there are no packets in alloted time, otherwise a pointer
    to the packet received, if allocatePackets is true than the place that 
    called this function owns the packet and should delete the packet when 
    done... if allocatePackets is false then nothing must store a pointer to
    this packet, the packet must be used and done with by the time this 
    method is called again
*/
AREXPORT ArRobotPacket *ArRobotPacketReceiver::receivePacket(
	unsigned int msWait)
{
  ArRobotPacket *packet;
  unsigned char c;
  char buf[256];
  int count = 0;
  // state can be one of the STATE_ enums in the class
  int state = STATE_SYNC1;
  //unsigned int timeDone;
  //unsigned int curTime;
  long timeToRunFor;
  ArTime timeDone;
  ArTime lastDataRead;
  ArTime packetReceived;
  int numRead;

  if (myAllocatePackets)
    packet = new ArRobotPacket(mySync1, mySync2);
  else
    packet = &myPacket;

  if (packet == NULL || myDeviceConn == NULL || 
      myDeviceConn->getStatus() != ArDeviceConnection::STATUS_OPEN)
  {
    myDeviceConn->debugEndPacket(false, -10);
    if (myAllocatePackets)
      delete packet;
    return NULL;
  }
  
  timeDone.setToNow();
  if (!timeDone.addMSec(msWait)) {
    ArLog::log(ArLog::Normal,
               "ArRobotPacketReceiver::receivePacket() error adding msecs (%i)",
               msWait);
  }

  // check for log file connection, return assembled packet
  if (dynamic_cast<ArLogFileConnection *>(myDeviceConn))
  {
    packet->empty();
    packet->setLength(0);
    packetReceived = myDeviceConn->getTimeRead(0);
    packet->setTimeReceived(packetReceived);
    numRead = myDeviceConn->read(buf, 255, 0);
    if (numRead > 0)
    {
      packet->dataToBuf(buf, numRead);
      packet->resetRead();
      myDeviceConn->debugEndPacket(true, packet->getID());
      if (myPacketReceivedCallback != NULL)
				myPacketReceivedCallback->invoke(packet);

			// if tracking is on - log packet - also make sure
			// buffer length is in range
			
			if ((myTracking) && (packet->getLength() < 10000)) {

				unsigned char *buf = (unsigned char *) packet->getBuf();
		
				char obuf[10000];
				obuf[0] = '\0';
				int j = 0;
				for (int i = 0; i < packet->getLength(); i++) {
					sprintf (&obuf[j], "_%02x", buf[i]);
					j= j+3;
				}
				ArLog::log (ArLog::Normal,
				            "Recv Packet: %s packet = %s", 
										myTrackingLogName.c_str(), obuf);


			}  // end tracking		

      return packet;
    }
    else
    {
      myDeviceConn->debugEndPacket(false, -20);
      if (myAllocatePackets)
	delete packet;
      return NULL;
    }
  }      
  

  do
    {
      timeToRunFor = timeDone.mSecTo();
      if (timeToRunFor < 0)
        timeToRunFor = 0;

      if (state == STATE_SYNC1)
	myDeviceConn->debugStartPacket();

      if (myDeviceConn->read((char *)&c, 1, timeToRunFor) == 0) 
        {
	  myDeviceConn->debugBytesRead(0);
          if (state == STATE_SYNC1)
            {
	      myDeviceConn->debugEndPacket(false, -30);
              if (myAllocatePackets)
                delete packet;
              return NULL;
            }
          else
            {
              //ArUtil::sleep(1);
              continue;
            }
        }

      myDeviceConn->debugBytesRead(1);

      switch (state) {
      case STATE_SYNC1:


        if (c == mySync1) // move on, resetting packet
          {
            state = STATE_SYNC2;
            packet->empty();
            packet->setLength(0);
            packet->uByteToBuf(c);
            packetReceived = myDeviceConn->getTimeRead(0);
            packet->setTimeReceived(packetReceived);
          }
        else
	{
          //printf("Bad sync1 %d\n", c);
	}
        break;
      case STATE_SYNC2:
 
       if (c == mySync2) // move on, adding this byte
          {
            state = STATE_ACQUIRE_DATA;
            packet->uByteToBuf(c);
          }
        else // go back to beginning, packet hosed
          {
	    //printf("Bad sync2 %d\n", c);
            state = STATE_SYNC1;
          }
        break;
      case STATE_ACQUIRE_DATA:

        // the character c is the count of the packets remianing at this point
        // so we'll just put it into the packet then get the rest of the data
        packet->uByteToBuf(c);
        // if c > 200 than there is a problem, spec says packet max size is 200
        count = 0;
	/** this case can't happen since c can't be over that so taking it out
        if (c > 255) 
          {
            ArLog::log(ArLog::Normal, "ArRobotPacketReceiver::receivePacket: bad packet, more than 255 bytes");
            state = STATE_SYNC1;
            break;
          }
	*/
        // here we read until we get as much as we want, OR until
        // we go 100 ms without data... its arbitrary but it doesn't happen often
        // and it'll mean a bad packet anyways
        lastDataRead.setToNow();
        while (count < c)
          {
            numRead = myDeviceConn->read(buf + count, c - count, 1);
            if (numRead > 0)
	    {
	      myDeviceConn->debugBytesRead(numRead);
              lastDataRead.setToNow();
	    }
	    else
	    {
	      myDeviceConn->debugBytesRead(0);
	    }
            if (lastDataRead.mSecTo() < -100)
              {
		myDeviceConn->debugEndPacket(false, -40);
                if (myAllocatePackets)
                  delete packet;
		//printf("Bad time taken reading\n");
                return NULL;
              }
            count += numRead;
          }
        packet->dataToBuf(buf, c);
        if (packet->verifyCheckSum()) 
          {
	
            packet->resetRead();
	    /* put this in if you want to see the packets received
	       printf("Input ");
               packet->printHex();
	       */

	    // you can also do this next line if you only care about type
	    //printf("Input %x\n", packet->getID());
	    myDeviceConn->debugEndPacket(true, packet->getID());
	    if (myPacketReceivedCallback != NULL)
	      myPacketReceivedCallback->invoke(packet);

			// if tracking is on - log packet - also make sure
			// buffer length is in range

			if ((myTracking) && (packet->getLength() < 10000)) {

				unsigned char *buf = (unsigned char *) packet->getBuf();
		
				char obuf[10000];
				obuf[0] = '\0';
				int j = 0;
				for (int i = 0; i < packet->getLength(); i++) {
					sprintf (&obuf[j], "_%02x", buf[i]);
					j= j+3;
				}
				ArLog::log (ArLog::Normal,
				            "Recv Packet: %s packet = %s", 
										myTrackingLogName.c_str(), obuf);


			}  // end tracking		

			return packet;
          }
        else 
          {
	    /* put this in if you want to see bad checksum packets 
               printf("Bad Input ");
               packet->printHex();
	       */
            ArLog::log(ArLog::Normal, 
                       "ArRobotPacketReceiver::receivePacket: bad packet, bad checksum");
            state = STATE_SYNC1;
	    myDeviceConn->debugEndPacket(false, -50);
            break;
          }
        break;
      default:
        break;
      }
    } while (timeDone.mSecTo() >= 0 || state != STATE_SYNC1);

  myDeviceConn->debugEndPacket(false, -60);
  //printf("finished the loop...\n");
  if (myAllocatePackets)
    delete packet;
  return NULL;

}

AREXPORT void ArRobotPacketReceiver::setPacketReceivedCallback(
	ArFunctor1<ArRobotPacket *> *functor)
{
  myPacketReceivedCallback = functor;
}
