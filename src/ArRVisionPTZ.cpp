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
#include "ArExport.h"
#include "ariaOSDef.h"
#include "ArRVisionPTZ.h"
#include "ArRobot.h"
#include "ArCommands.h"

AREXPORT ArRVisionPacket::ArRVisionPacket(ArTypes::UByte2 bufferSize) :
  ArBasePacket(bufferSize)
{

}

AREXPORT ArRVisionPacket::~ArRVisionPacket()
{

}

AREXPORT void ArRVisionPacket::uByteToBuf(ArTypes::UByte val)
{
  if (myLength + 1 > myMaxLength)
  {
    ArLog::log(ArLog::Terse, "ArRVisionPacket::uByteToBuf: Trying to add beyond length of buffer.");
    return;
  }
  myBuf[myLength] = val;
  ++myLength;
}

AREXPORT void ArRVisionPacket::byte2ToBuf(ArTypes::Byte2 val)
{
  if ((myLength + 4) > myMaxLength)
  {
    ArLog::log(ArLog::Terse, "ArRVisionPacket::Byte2ToBuf: Trying to add beyond length of buffer.");
    return;
  }
  myBuf[myLength] = (val & 0xf000) >> 12;
  ++myLength;
  myBuf[myLength] = (val & 0x0f00) >> 8;
  ++myLength;
  myBuf[myLength] = (val & 0x00f0) >> 4;
  ++myLength;
  myBuf[myLength] = (val & 0x000f) >> 0;
  ++myLength;
}

/**
   This function is my concession to not rebuilding a packet from scratch
   for every command, basicaly this is to not lose all speed over just using
   a character array.  This is used by the default rvision commands, unless
   you have a deep understanding of how the packets are working and what
   the packet structure looks like you should not play with this function, 
   it also isn't worth it unless you'll be sending commands frequently.
   @param val the Byte2 to put into the packet
   @param pose the position in the packets array to put the value
*/
AREXPORT void ArRVisionPacket::byte2ToBufAtPos(ArTypes::Byte2 val,
					    ArTypes::UByte2 pose)
{
  ArTypes::Byte2 prevLength = myLength;

  if ((pose + 4) > myMaxLength)
  {
    ArLog::log(ArLog::Terse, "ArRVisionPacket::Byte2ToBuf: Trying to add beyond length of buffer.");
    return;
  }
  myLength = pose;
  byte2ToBuf(val);
  myLength = prevLength;
}


AREXPORT ArRVisionPTZ::ArRVisionPTZ(ArRobot *robot) :
  ArPTZ(NULL),
  myPacket(255), 
  myZoomPacket(9)
{
  //myRobot = robot;
  initializePackets();
  
  // these ticks were derived emperically.  Ticks / real_degrees
  myDegToTilt = 2880 / 180;
  myDegToPan =  960 / 60;

  myTiltOffsetInDegrees = TILT_OFFSET_IN_DEGREES; 
  myPanOffsetInDegrees = PAN_OFFSET_IN_DEGREES; 

  myConn = NULL;

}

AREXPORT ArRVisionPTZ::~ArRVisionPTZ()
{
}

void ArRVisionPTZ::initializePackets(void)
{
  myZoomPacket.empty();
  myZoomPacket.uByteToBuf(0x81);
  myZoomPacket.uByteToBuf(0x01);
  myZoomPacket.uByteToBuf(0x04);
  myZoomPacket.uByteToBuf(0x47);
  myZoomPacket.uByteToBuf(0x00);
  myZoomPacket.uByteToBuf(0x00);
  myZoomPacket.uByteToBuf(0x00);
  myZoomPacket.uByteToBuf(0x00);
  myZoomPacket.uByteToBuf(0xff);

  myPanTiltPacket.empty();
  myPanTiltPacket.uByteToBuf(0x81);
  myPanTiltPacket.uByteToBuf(0x01);
  myPanTiltPacket.uByteToBuf(0x06);
  myPanTiltPacket.uByteToBuf(0x02);
  myPanTiltPacket.uByteToBuf(0x18);
  myPanTiltPacket.uByteToBuf(0x14);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0x00);
  myPanTiltPacket.uByteToBuf(0xff);
}


AREXPORT bool ArRVisionPTZ::init(void)
{
  // send command to power on camera on seekur
  if(myRobot)
  {
	  ArLog::log(ArLog::Normal, "ArRVisionPTZ: turning camer power on ...");
	  myRobot->com2Bytes(116, 12, 1);
  }
  
  myConn = getDeviceConnection();
  if(!myConn)
  {
     ArLog::log(ArLog::Normal, "ArRVisionPTZ: opening connectino to camera on COM3...");
     ArSerialConnection *ser = new ArSerialConnection();
     if(ser->open(ArUtil::COM3) != 0)
     {
	ArLog::log(ArLog::Terse, "ArRVisionPTZ: error opening COM3 for camera PTZ control, initialization failed.");
        myConn = NULL;
        return false;
     }
     myConn = ser;
  }
     
  myPacket.empty();
  myPacket.uByteToBuf(0x88);
  myPacket.uByteToBuf(0x01);
  myPacket.uByteToBuf(0x00);
  myPacket.uByteToBuf(0x01);
  myPacket.uByteToBuf(0xff);
  myPacket.uByteToBuf(0x88);
  myPacket.uByteToBuf(0x30);
  myPacket.uByteToBuf(0x01);
  myPacket.uByteToBuf(0xff);

  if (!sendPacket(&myPacket))
  {
    ArLog::log(ArLog::Terse, "ArRVisionPTZ: Error sending initialization packet to RVision camera!");
    return false;
  }
  if (!panTilt(0, 0))
    return false;
  if (!zoom(0))
    return false;
  return true;
}

AREXPORT bool ArRVisionPTZ::panTilt(double degreesPan, double degreesTilt)
{
  if (degreesPan > MAX_PAN)
    degreesPan = MAX_PAN;
  if (degreesPan < MIN_PAN)
    degreesPan = MIN_PAN;
  myPan = degreesPan;

  if (degreesTilt > MAX_TILT)
    degreesTilt = MAX_TILT;
  if (degreesTilt < MIN_TILT)
    degreesTilt = MIN_TILT;
  myTilt = degreesTilt;

  myPanTiltPacket.byte2ToBufAtPos(ArMath::roundInt((myPan+myPanOffsetInDegrees) * myDegToPan), 6);
  myPanTiltPacket.byte2ToBufAtPos(ArMath::roundInt((myTilt+myTiltOffsetInDegrees) * myDegToTilt), 10);
  return sendPacket(&myPanTiltPacket);
}

AREXPORT bool ArRVisionPTZ::panTiltRel(double degreesPan, double degreesTilt)
{
  return panTilt(myPan + degreesPan, myTilt + degreesTilt);
}

AREXPORT bool ArRVisionPTZ::pan(double degrees)
{
  return panTilt(degrees, myTilt);
}

AREXPORT bool ArRVisionPTZ::panRel(double degrees)
{
  return panTiltRel(degrees, 0);
}

AREXPORT bool ArRVisionPTZ::tilt(double degrees)
{
  return panTilt(myPan, degrees);
}

AREXPORT bool ArRVisionPTZ::tiltRel(double degrees)
{
  return panTiltRel(0, degrees);
}

AREXPORT bool ArRVisionPTZ::zoom(int zoomValue)
{
  //printf("ArRVision::zoom(%d)\n", zoomValue);
  if (zoomValue > MAX_ZOOM)
    zoomValue = MAX_ZOOM;
  if (zoomValue < MIN_ZOOM)
    zoomValue = MIN_ZOOM;
  myZoom = zoomValue;
    
  myZoomPacket.byte2ToBufAtPos(ArMath::roundInt(myZoom), 4);
  return sendPacket(&myZoomPacket);
}

AREXPORT bool ArRVisionPTZ::zoomRel(int zoomValue)
{
  return zoom(myZoom + zoomValue);
}

/*
AREXPORT bool ArRVisionPTZ::packetHandler(ArRobotPacket *packet)
{
  if (packet->getID() != 0xE0)
    return false;

  return true;
}
*/

#define MAX_RESPONSE_BYTES 16
//AREXPORT bool ArRVisionPTZ::packetHandler(ArBasePacket *packet)
ArBasePacket * ArRVisionPTZ::readPacket(void)
{
  unsigned char data[MAX_RESPONSE_BYTES];
  unsigned char byte;
  int num;
  memset(data, MAX_RESPONSE_BYTES, 0);
  for (num=0; num <= MAX_RESPONSE_BYTES+1; num++) {
    if (myConn->read((char *) &byte, 1,1) <= 0 ||
	num == MAX_RESPONSE_BYTES+1) {
      return NULL;
    }
    else if (byte == 0x90) {
      data[0] = byte;
      //printf("ArRVisionPTZ::packetHandler:  got 0x%x, expecting packet\n", byte);
      break;
    }
    else {
      //printf("ArRVisionPTZ::packetHandler:  got 0x%x, skipping\n", byte);
    }
  }
  // we got the header
  for (num=1; num <= MAX_RESPONSE_BYTES; num++) {
    if (myConn->read((char *) &byte, 1, 1) <= 0) {
      // there are no more bytes, so check the last byte for the footer
      if (data[num-1] != 0xFF) {
	//printf("ArRVisionPTZ::packetHandler: should have gotten 0xFF, got 0x%x\n", data[num-1]);
	return NULL;
      }
      else {
	break;
      }
    }
    else {
      // add the byte to the array
      data[num] = byte;
    }
  }
  // print the data for now
  //printf("ArRVisionPTZ::packetHandler: got packet!\n");
  //for (int i=0; i <= num; i++) {
  //printf("\t[%d]: 0x%x\n", i, data[i]);
  //}
  return NULL;
}
