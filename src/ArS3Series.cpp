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
#include "ArS3Series.h"
#include "ArRobot.h"
#include "ArSerialConnection.h"
#include "ariaInternal.h"
#include <time.h>

AREXPORT ArS3SeriesPacket::ArS3SeriesPacket() :
ArBasePacket(10000, 1, NULL, 1) {

}

AREXPORT ArS3SeriesPacket::~ArS3SeriesPacket() {

}

AREXPORT ArTime ArS3SeriesPacket::getTimeReceived(void) {
	return myTimeReceived;
}

AREXPORT void ArS3SeriesPacket::setTimeReceived(ArTime timeReceived) {
	myTimeReceived = timeReceived;
}

AREXPORT void ArS3SeriesPacket::duplicatePacket(ArS3SeriesPacket *packet) {
	myLength = packet->getLength();
	myReadLength = packet->getReadLength();
	myTimeReceived = packet->getTimeReceived();
	myDataLength = packet->myDataLength;
	myNumReadings = packet->myNumReadings;
	myStatusByte = packet->myStatusByte;
	myTimeStampByte1 = packet->myTimeStampByte1;
	myTimeStampByte2 = packet->myTimeStampByte2;
	myTimeStampByte3 = packet->myTimeStampByte3;
	myTimeStampByte4 = packet->myTimeStampByte4;
	myTelegramNumByte1 = packet->myTelegramNumByte1;
	myTelegramNumByte2 = packet->myTelegramNumByte2;
	myCrcByte1 = packet->myCrcByte1;
	myCrcByte2 = packet->myCrcByte2;

	memcpy(myBuf, packet->getBuf(), myLength);
}

AREXPORT void ArS3SeriesPacket::empty(void) {
	myLength = 0;
	myReadLength = 0;
}

AREXPORT ArS3SeriesPacketReceiver::ArS3SeriesPacketReceiver() {

}

AREXPORT ArS3SeriesPacketReceiver::~ArS3SeriesPacketReceiver() {

}

AREXPORT void ArS3SeriesPacketReceiver::setDeviceConnection(
		ArDeviceConnection *conn) {
	myConn = conn;
}

AREXPORT ArDeviceConnection *ArS3SeriesPacketReceiver::getDeviceConnection(void) {
	return myConn;
}

ArS3SeriesPacket *ArS3SeriesPacketReceiver::receivePacket(unsigned int msWait,
		bool startMode) {

	ArS3SeriesPacket *packet;
	unsigned char c;
	long timeToRunFor;
	ArTime timeDone;
	ArTime lastDataRead;
	ArTime packetReceived;
	int i;

	if (myConn == NULL || myConn->getStatus()
			!= ArDeviceConnection::STATUS_OPEN) {
		return NULL;
	}

	timeDone.setToNow();
	if (!timeDone.addMSec(msWait)) {
		ArLog::log(ArLog::Verbose, "%s::receivePacket() error adding msecs (%i)",
				myName, msWait);
	}
	msWait = 10;

	do {
		timeToRunFor = timeDone.mSecTo();
		if (timeToRunFor < 0)
			timeToRunFor = 0;
		/*
		 ArLog::log(ArLog::Terse,
		 "%s::receivePacket() timeToRunFor = %d",
		 myName, timeToRunFor);
		 */

		myPacket.empty();
		myPacket.setLength(0);
		myReadCount = 0;

		unsigned char firstbytelen;
		unsigned char secondbytelen;
		unsigned char temp[4];

		unsigned char crcbuf[10000];
		int n = 0;

		// look for initial sequence 0x00 0x00 0x00 0x00
		for (i = 0; i < 4; i++) {
			if ((myConn->read((char *) &c, 1, msWait)) > 0) {
				if (c != 0x00) {
					//ArLog::log(ArLog::Terse,
					//                 "ArS3Series::receivePacket() error reading first 4 bytes of header");
					break;
				}
				if (i == 0) {
					packetReceived = myConn->getTimeRead(0);
					myPacket.setTimeReceived(packetReceived);
				}
			} else {
				/* don't log this if we are in starting mode, means laser is not connecting */
				if (startMode)
					ArLog::log(ArLog::Terse,
							"%s::receivePacket() myConn->read error (header)",
							myName);
				return NULL;
			}
		} // end for

		if (c != 0x00)
			continue;

		// next 2 bytes = 0x00 0x00 - data block number
		for (i = 0; i < 2; i++) {
			if ((myConn->read((char *) &c, 1, msWait)) > 0) {
				if (c != 0x00) {
					//ArLog::log(ArLog::Terse,
					//                 "ArS3Series::receivePacket() error data block number in header");
					break;
				}
			} else {
				ArLog::log(
						ArLog::Terse,
						"%s::receivePacket() myConn->read error (data block number)",
						myName);
				return NULL;
			}
		} // end for

		if (c != 0x00)
			continue;

		crcbuf[n++] = 0;
		crcbuf[n++] = 0;

		// next 2 bytes are length, i think they are swapped so we need to mess with them

		for (i = 0; i < 2; i++) {
			if ((myConn->read((char *) &c, 1, msWait)) > 0) {
				temp[i] = c;
				crcbuf[n++] = c;
			} else {
				ArLog::log(ArLog::Terse,
						"%s::receivePacket() myConn->read error (length)",
						myName);
				return NULL;
			}
		} // end for

		firstbytelen = temp[0];
		secondbytelen = temp[1];

		// do we need to validate byte length
		int datalen = secondbytelen | (firstbytelen << 8);

		// double it as this is 2 byte pairs of readings
		// and take of the header of 17 bytes
		myPacket.setDataLength((datalen * 2) - 17);
		//printf("datalength = %d \n",myPacket.getDataLength());

		// the number of reading is going to be 4 bytes less as there's
		// a bb bb and 11 11 (which are ID for measurement data and ID
		// for measured values from angular range 1
		myPacket.setNumReadings(((myPacket.getDataLength() - 5) / 2) - 1);

		/*
		ArLog::Terse,
		"%s::receivePacket() Number of readings = %d", myName, myPacket.getNumReadings());
		*/

		// next 2 bytes need to be 0xff & 0x07
		for (i = 0; i < 2; i++) {
			if ((myConn->read((char *) &c, 1, msWait)) > 0) {
				temp[i] = c;
				crcbuf[n++] = c;
			} else {
				ArLog::log(
						ArLog::Terse,
						"%s::receivePacket() myConn->read error (coordination flag and device code)",
						myName);
				return NULL;
			}
		} // end for

		if (temp[0] != 0xff || temp[1] != 0x07)
		{
			/*
			ArLog::log(ArLog::Terse,
			     "ArS3Series::receivePacket() co-oridination flag and device code error");
			 */
			continue;
		}

		// next 2 bytes are protocol version to be 0x02 & 0x01

		for (i = 0; i < 2; i++) {
			if ((myConn->read((char *) &c, 1, msWait)) > 0)
			{
				temp[i] = c;
				crcbuf[n++] = c;
			}
			else
			{
				ArLog::log(
						ArLog::Terse,
						"%s::receivePacket() myConn->read error (protocol version?)",
						myName);
				return NULL;
			}
		} // end for

		// we have an old S3000 who's protocol is 00 01, later versions are 02 01
		if ((temp[0] == 0x00 || temp[1] == 0x01) ||
				(temp[0] == 0x02 || temp[1] == 0x01))
		{
		}
		else
		{
			ArLog::log(ArLog::Terse,
					"%s::receivePacket() protocol version error (0x%x 0x%x)",
					myName, temp[0], temp[1]);
			continue;
		}

		// next 1 byte is status flag

		for (i = 0; i < 1; i++)
		{
			if ((myConn->read((char *) &c, 1, msWait)) > 0)
			{
				temp[i] = c;
				crcbuf[n++] = c;
			}
			else
			{
				ArLog::log(ArLog::Terse,
						"%s::receivePacket() myConn->read error (status flag?)",
						myName);
				return NULL;
			}
		} // end for

		myPacket.setStatusByte(temp[0]);

		// next 4 bytes are timestamp

		for (i = 0; i < 4; i++)
		{
			if ((myConn->read((char *) &c, 1, msWait)) > 0)
			{
				temp[i] = c;
				crcbuf[n++] = c;
			}
			else
			{
				ArLog::log(ArLog::Terse,
						"%s::receivePacket() myConn->read error (time stamp)",
						myName);
				return NULL;
			}
		} // end for

		myPacket.setTimeStampByte1(temp[0]);
		myPacket.setTimeStampByte2(temp[1]);
		myPacket.setTimeStampByte3(temp[2]);
		myPacket.setTimeStampByte4(temp[3]);

		/*
		ArLog::log(
				ArLog::Terse,
				"%s::receivePacket() Time stamp = %x %x %x %x ",
				myName, temp[0],temp[1],temp[2],temp[3]);
		 */

		// next 2 bytes are telegram number

		for (i = 0; i < 2; i++)
		{
			if ((myConn->read((char *) &c, 1, msWait)) > 0)
			{
				temp[i] = c;
				crcbuf[n++] = c;
			}
			else
			{
				ArLog::log(ArLog::Terse,
						"%s::receivePacket() myConn->read error (telegram number)",
						myName);
				return NULL;
			}
		} // end for

		myPacket.setTelegramNumByte1(temp[0]);
		myPacket.setTelegramNumByte2(temp[1]);

		/*
		ArLog::log(
				ArLog::Terse,
				"%s::receivePacket() Telegram number =  %d  ",
				myName,  myPacket.getTelegramNumByte2());
		 */

		int numRead = myConn->read((char *) &myReadBuf[0],
				myPacket.getDataLength(), 5000);

		// trap if we failed the read
		if (numRead < 0) {
			ArLog::log(ArLog::Terse, "%s::receivePacket() Failed read (%d)",
					myName, numRead);
			return NULL;
		}

		/*
		ArLog::log(ArLog::Terse,
				"%s::receivePacket() Number of bytes read = %d", myName,
				numRead);
		 */

		//printf("\nhere's the data from packetrecieve\n ");
		//for (i = 0; i < myPacket.getDataLength(); i++) {
		//strip out first 3 bytes
		//if ((i > 5) && ((i % 2 == 0)))
		//myReadBuf[i] = myReadBuf[i] && 0x1f;

		//printf("%x ", myReadBuf[i]);
		//}

		//printf("\n");
		// and finally the crc

		// start after the 00 bb bb 11 11 and put the
		// raw readings into myReadBuf
		//printf("\nhere's the raw readings\n ");
		//for (i=5; i<(myNumReadings * 2); i+2)
		//  {
		//	  // this my be backwards
		//  myReadBuf[i] = myReadBuf[i] && 0x1f;
		//  printf("%x %x",myReadBuf[i],myReadBuf[i+1]);
		// }

		for (i = 0; i < 2; i++)
		{
			if ((myConn->read((char *) &c, 1, msWait)) > 0)
			{
				temp[i] = c;
			}
			else
			{
				ArLog::log(ArLog::Terse,
						"%s::receivePacket() myConn->read error (crc)", myName);
				return NULL;
			}
		} // end for
		myPacket.setCrcByte1(temp[0]);
		myPacket.setCrcByte2(temp[1]);

#if 0
		// Matt put this check in to look for 0's in the data - not sure why
		int numZeros = 0;
		for (i = 5; i < myPacket.getDataLength(); i++) {
			if (myReadBuf[i] == 0)
				numZeros++;
			else
				numZeros = 0;

			if (numZeros >= 4) {
				ArLog::log(
						ArLog::Terse,
						"%s::receivePacket() myConn->read error (got 4 zeros in data)",
						myName);
				return NULL;
			}
		}
#endif

		memcpy(&crcbuf[n], &myReadBuf[0], myPacket.getDataLength());

		// now go validate the crc

		unsigned short crc = CRC16(crcbuf, myPacket.getDataLength() + n);

		unsigned short incrc = (temp[1] << 8) | temp[0];

		if (incrc != crc)
		{
			ArLog::log(ArLog::Terse,
					"%s::receivePacket() CRC error (in = 0x%02x calculated = 0x%02x) ",
					myName, incrc, crc);
			return NULL;
		}

		// PS 7/5/11 - there are 5 bytes of 00 bb bb 11 11 at the start of
		// the buffer - so skip over those so that we have the begining of
		// the readings
		myPacket.dataToBuf(&myReadBuf[5], myPacket.getNumReadings() * 2);
		myPacket.resetRead();
		packet = new ArS3SeriesPacket;
		packet->duplicatePacket(&myPacket);

		/*
		ArLog::log(ArLog::Normal,
		           "%s::receivePacket() returning packet %d %d", myName, packet->getNumReadings(), myPacket.getNumReadings());
		*/

		myPacket.empty();

		return packet;

	} while (timeDone.mSecTo() >= 0); // || !myStarting)

	ArLog::log(ArLog::Terse, "%s::receivePacket() Timeout on read", myName);
	return NULL;
}

AREXPORT ArS3Series::ArS3Series(int laserNumber, const char *name) :
			ArLaser(laserNumber, name, 20000),
			mySensorInterpTask(this, &ArS3Series::sensorInterp),
			myAriaExitCB(this, &ArS3Series::disconnect) {

	//ArLog::log(ArLog::Normal, "%s: Sucessfully created", getName());

	clear();
	myRawReadings = new std::list<ArSensorReading *>;

	Aria::addExitCallback(&myAriaExitCB, -10);

	setInfoLogLevel(ArLog::Normal);
	//setInfoLogLevel(ArLog::Terse);

	laserSetName( getName());

	laserAllowSetPowerControlled(false);

	/* taking this out for now - PS 6/29/11
	 laserAllowSetDegrees(-135, -135, 135, // start degrees
	 135, -135, 135); // end degrees

	 std::map<std::string, double> incrementChoices;
	 incrementChoices["half"] = .5;
	 incrementChoices["quarter"] = .25;
	 laserAllowIncrementChoices("half", incrementChoices);

	 laserSetDefaultTcpPort(2111);

	 */

	// PS 6/29/11 - changing to serial laserSetDefaultPortType("tcp");
	laserSetDefaultPortType("serial422");

	// PS 6/29/11 - added baud
	std::list < std::string > baudChoices;

	baudChoices.push_back("9600");
	baudChoices.push_back("19200");
	baudChoices.push_back("38400");
	baudChoices.push_back("57600");
	baudChoices.push_back("115200");
	//baudChoices.push_back("125000");
	baudChoices.push_back("230400");
	//baudChoices.push_back("460800");

	laserAllowStartingBaudChoices("38400", baudChoices);

	// PS 7/1/11
	//laserAllowAutoBaudChoices("38400", baudChoices);

	//myLogLevel = ArLog::Verbose;
	//myLogLevel = ArLog::Terse;
	myLogLevel = ArLog::Normal;

	setMinDistBetweenCurrent(0);
	setMaxDistToKeepCumulative(4000);
	setMinDistBetweenCumulative(200);
	setMaxSecondsToKeepCumulative(30);
	setMaxInsertDistCumulative(3000);

	setCumulativeCleanDist(75);
	setCumulativeCleanInterval(1000);
	setCumulativeCleanOffset(600);

	resetLastCumulativeCleanTime();

#if 0
	setCurrentDrawingData(
			new ArDrawingData("polyDots",
					ArColor(0, 0, 255),
					80, // mm diameter of dots
					75), // layer above sonar
					true);

	setCumulativeDrawingData(
			new ArDrawingData("polyDots",
					ArColor(125, 125, 125),
					100, // mm diameter of dots
					60), // layer below current range devices
					true);

#endif
	// PS make theses a different color, etc
	setCurrentDrawingData(
			new ArDrawingData("polyDots", ArColor(223, 223, 0), 75, // mm diameter of dots
					76), // layer above sonar
					true);

	setCumulativeDrawingData(
			new ArDrawingData("polyDots", ArColor(128, 128, 0), 95, // mm diameter of dots
					61), // layer below current range devices
					true);

}

AREXPORT ArS3Series::~ArS3Series() {
	Aria::remExitCallback(&myAriaExitCB);
	if (myRobot != NULL) {
		myRobot->remRangeDevice(this);
		myRobot->remLaser(this);
		myRobot->remSensorInterpTask(&mySensorInterpTask);
	}
	if (myRawReadings != NULL) {
		ArUtil::deleteSet(myRawReadings->begin(), myRawReadings->end());
		myRawReadings->clear();
		delete myRawReadings;
		myRawReadings = NULL;
	}
	lockDevice();
	if (isConnected())
		disconnect();
	unlockDevice();
}

void ArS3Series::clear(void) {
	myIsConnected = false;
	myTryingToConnect = false;
	myStartConnect = false;

	myNumChans = 0;
}

AREXPORT void ArS3Series::laserSetName(const char *name) {
	myName = name;

	myConnMutex.setLogNameVar("%s::myConnMutex", getName());
	myPacketsMutex.setLogNameVar("%s::myPacketsMutex", getName());
	myDataMutex.setLogNameVar("%s::myDataMutex", getName());
	myAriaExitCB.setNameVar("%s::exitCallback", getName());

	ArLaser::laserSetName( getName());
}

AREXPORT void ArS3Series::setRobot(ArRobot *robot) {
	myRobot = robot;

	if (myRobot != NULL) {
		myRobot->remSensorInterpTask(&mySensorInterpTask);
		myRobot->addSensorInterpTask("S3Series", 90, &mySensorInterpTask);
	}
	ArLaser::setRobot(robot);
}

AREXPORT bool ArS3Series::asyncConnect(void) {
	myStartConnect = true;
	if (!getRunning())
		runAsync();
	return true;
}

AREXPORT bool ArS3Series::disconnect(void) {
	if (!isConnected())
		return true;

	ArLog::log(ArLog::Normal, "%s: Disconnecting", getName());

	laserDisconnectNormally();
	return true;
}

void ArS3Series::failedToConnect(void) {
	lockDevice();
	myTryingToConnect = true;
	unlockDevice();
	laserFailedConnect();
}

void ArS3Series::sensorInterp(void) {
	ArS3SeriesPacket *packet;

	while (1) {
		myPacketsMutex.lock();
		if (myPackets.empty()) {
			myPacketsMutex.unlock();
			return;
		}
		packet = myPackets.front();
		myPackets.pop_front();
		myPacketsMutex.unlock();

		//set up the times and poses

		ArTime time = packet->getTimeReceived();
		ArPose pose;
		int ret;
		int retEncoder;
		ArPose encoderPose;
		int dist;
		int j;

		// Packet will already be offset by 5 bytes to the start
		// of the readings - for S3000 there should be 381 readings (190 degrees)
		// for S300 541 readings (270 degrees)
		unsigned char *buf = (unsigned char *) packet->getBuf();

		// this value should be found more empirically... but we used 1/75
		// hz for the lms2xx and it was fine, so here we'll use 1/50 hz for now
		// PS 7/9/11 - not sure what this is doing????
		if (!time.addMSec(-30)) {
			ArLog::log(ArLog::Verbose,
					"%s::sensorInterp() error adding msecs (-30)", getName());
		}

		if (myRobot == NULL || !myRobot->isConnected())
		{
			pose.setPose(0, 0, 0);
			encoderPose.setPose(0, 0, 0);
		}
		else if ((ret = myRobot->getPoseInterpPosition(time, &pose)) < 0
				|| (retEncoder = myRobot->getEncoderPoseInterpPosition(time,
						&encoderPose)) < 0)
		{
			ArLog::log(ArLog::Normal,
					"%s::sensorInterp(): Warning: reading too old to process", getName());
			delete packet;
			continue;
		}

		ArTransform transform;
		transform.setTransform(pose);

		unsigned int counter = 0;
		if (myRobot != NULL)
			counter = myRobot->getCounter();

		lockDevice();
		myDataMutex.lock();

		//std::list<ArSensorReading *>::reverse_iterator it;
		ArSensorReading *reading;

		myNumChans = packet->getNumReadings();

		double eachAngularStepWidth;
		int eachNumberData;

		// PS - test for both S3000 (190 degrees) and S300 (270 degrees)
		if ((packet->getNumReadings() == 381) || (packet->getNumReadings()
				== 540))
		{
			// PS 7/5/11 - grab the number of raw readings from the receive packet
			eachNumberData = packet->getNumReadings();
		}
		else
		{
			ArLog::log(ArLog::Normal,
					"%s::sensorInterp(): Warning: The number of readings is not correct = %d",
					getName(), myNumChans);
			delete packet;
			continue;
		}

		// If we don't have any sensor readings created at all, make 'em all
		if (myRawReadings->size() == 0)
			for (j = 0; j < eachNumberData; j++)
				myRawReadings->push_back(new ArSensorReading);

		if (eachNumberData > myRawReadings->size())
		{
			ArLog::log(ArLog::Terse,
					"%s::sensorInterp() Bad data, in theory have %d readings but can only have 541... skipping this packet",
					getName(), eachNumberData);
			continue;
		}

		std::list<ArSensorReading *>::iterator it;
		double atDeg;
		int onReading;

		double start;
		double increment;

		//eachStartingAngle = -5;
		eachAngularStepWidth = .5;

		// from the number of readings, calculate the start

		if (myFlipped) {
			//start = mySensorPose.getTh() + eachStartingAngle - 90.0 + eachAngularStepWidth * eachNumberData;
			start = mySensorPose.getTh() + (packet->getNumReadings() / 4);
			increment = -eachAngularStepWidth;
		} else {
			//start = mySensorPose.getTh() + eachStartingAngle - 90.0;
			start = -(mySensorPose.getTh() + (packet->getNumReadings() / 4));
			increment = eachAngularStepWidth;
		}

		int readingIndex;
		bool ignore = false;

		for (atDeg = start,
				it = myRawReadings->begin(),
				readingIndex = 0,
				onReading = 0;

				onReading < eachNumberData;

				atDeg += increment,
				it++,
				readingIndex++,
				onReading++)
		{


			reading = (*it);

			dist = ((buf[(readingIndex * 2) + 1] & 0x8f) << 8)
							| buf[readingIndex * 2];
			dist = dist * 10; // convert to mm


			reading->resetSensorPosition(ArMath::roundInt(mySensorPose.getX()),
					ArMath::roundInt(mySensorPose.getY()), atDeg);
			reading->newData(dist, pose, encoderPose, transform, counter, time,
					ignore, 0); // no reflector yet

			//printf("dist = %d, pose = %d, encoderPose = %d, transform = %d, counter = %d, time = %d, igore = %d",
			//		dist, pose, encoderPose, transform, counter,
			//					 time, ignore);
		}
		/*
		 ArLog::log(ArLog::Normal,
		 "Received: %s %s scan %d numReadings %d", 
		 packet->getCommandType(), packet->getCommandName(), 
		 myScanCounter, onReading);
		 */

		myDataMutex.unlock();

		/*
		ArLog::log(
				ArLog::Terse,
				"%s::sensorInterp() Telegram number =  %d  ",
				getName(),  packet->getTelegramNumByte2());
		 */

		laserProcessReadings();
		unlockDevice();
		delete packet;
	}
}

AREXPORT bool ArS3Series::blockingConnect(void) {
	long timeToRunFor;

	if (!getRunning())
		runAsync();

	myConnMutex.lock();
	if (myConn == NULL) {
		ArLog::log(ArLog::Terse,
				"%s: Error: Could not connect because there is no connection defined",
				getName());
		myConnMutex.unlock();
		failedToConnect();
		return false;
	}

	// PS 9/9/11 - moved this here to fix issue with setting baud in mt400.p
	laserPullUnsetParamsFromRobot();
	laserCheckParams();

	// PS 9/9/11 - add setting baud
    ArSerialConnection *serConn = NULL;
	serConn = dynamic_cast<ArSerialConnection *>(myConn);
	if (serConn != NULL)
    	serConn->setBaud(atoi(getStartingBaudChoice()));

	if (myConn->getStatus() != ArDeviceConnection::STATUS_OPEN
			&& !myConn->openSimple()) {
		ArLog::log(
				ArLog::Terse,
				"%s: Could not connect because the connection was not open and could not open it",
				getName());
		myConnMutex.unlock();
		failedToConnect();
		return false;
	}

	// PS - set logging level and laser type in packet receiver class
	myReceiver.setmyInfoLogLevel(myInfoLogLevel);
	myReceiver.setmyName(getName());

	myReceiver.setDeviceConnection(myConn);
	myConnMutex.unlock();

	lockDevice();
	myTryingToConnect = true;
	unlockDevice();

	// PS 9/9/11 - moved up top
	//laserPullUnsetParamsFromRobot();
	//laserCheckParams();

	int size = ArMath::roundInt((270 / .25) + 1);
	ArLog::log(myInfoLogLevel,
			"%s::blockingConnect() Setting current buffer size to %d",
			getName(), size);
	setCurrentBufferSize(size);

	ArTime timeDone;
	if (myPowerControlled)
	{
		if (!timeDone.addMSec(60 * 1000))
		{
			ArLog::log(ArLog::Verbose,
					"%s::blockingConnect() error adding msecs (60 * 1000)",
					getName());
		}
	}
	else
	{
		if (!timeDone.addMSec(30 * 1000))
		{
			ArLog::log(ArLog::Verbose,
					"%s::blockingConnect() error adding msecs (30 * 1000)",
					getName());
		}
	}

	ArS3SeriesPacket *packet;

	bool startMode = true;
	do
	{
		timeToRunFor = timeDone.mSecTo();
		if (timeToRunFor < 0)
			timeToRunFor = 0;

		if ((packet = myReceiver.receivePacket(1000, startMode)) != NULL)
		{
			ArLog::log(ArLog::Verbose, "%s: got packet", getName());
			delete packet;
			packet = NULL;

			lockDevice();
			myIsConnected = true;
			myTryingToConnect = false;
			unlockDevice();
			ArLog::log(ArLog::Normal, "%s: Connected to laser", getName());
			laserConnect();
			return true;
		}
		else
		{
			packet = NULL;
			ArLog::log(ArLog::Verbose, "%s: MS left = %d", getName(), timeDone.mSecTo());
		}
		// this is only used for logging in receivePacket
		startMode = false;
	} while (timeDone.mSecTo() >= 0); // || !myStarting)

	ArLog::log(ArLog::Terse,
			"%s::blockingConnect()  Did not get scan data back from laser",
			getName());
	failedToConnect();
	return false;

}

AREXPORT void * ArS3Series::runThread(void *arg) {
	//char buf[1024];
	ArS3SeriesPacket *packet;

while (getRunning() )
{
	lockDevice();
	
	if (myStartConnect) {
		myStartConnect = false;
		myTryingToConnect = true;
		unlockDevice();
		
		blockingConnect();
		
		lockDevice();
		myTryingToConnect = false;
		unlockDevice();
		continue;
	}
	
	unlockDevice();
	
	if (!myIsConnected) {
		ArUtil::sleep (100);
		continue;
	}
	
	
	while (getRunning() && myIsConnected &&
	       (packet = myReceiver.receivePacket (500, false) ) != NULL) {
		myPacketsMutex.lock();
		myPackets.push_back (packet);
		myPacketsMutex.unlock();
		
		if (myRobot == NULL)
			sensorInterp();
	}
	
	// if we have a robot but it isn't running yet then don't have a
	// connection failure
	if (getRunning() && myIsConnected && laserCheckLostConnection() ) {
		ArLog::log (ArLog::Terse,
		            "%s::runThread()  Lost connection to the laser because of error.  Nothing received for %g seconds (greater than the timeout of %g).", getName(),
		            myLastReading.mSecSince() / 1000.0,
		            getConnectionTimeoutSeconds() );
		myIsConnected = false;
		laserDisconnectOnError();
		continue;
	}
	
	/// MPL no sleep here so it'll get back into that while as soon as it can
	
	//ArUtil::sleep(1);
	//ArUtil::sleep(2000);
	//ArUtil::sleep(500);
	
#if 0 // PS 10/12/11 - fixing disconnects
	
	
	// PS 7/5/11 - change msWait from 50 to 5000
	
	// MPL 7/12/11 Changed mswait to 500 (which is bad enough,
	// especially since receive packet doesn't use it quite right at
	// this time)
	while (getRunning() && myIsConnected && (packet
	        = myReceiver.receivePacket (500, false) ) != NULL) {
	        
		myPacketsMutex.lock();
		myPackets.push_back (packet);
		myPacketsMutex.unlock();
		
		//ArLog::log(ArLog::Terse, "myRobot = %s",myRobot);
		
		//if (myRobot == NULL)
		//sensorInterp();
		
		/// MPL TODO see if this gets called if the laser goes
		/// away... it looks like it may not (since the receivePacket may just return nothing)
		
		// if we have a robot but it isn't running yet then don't have a
		// connection failure
		if (laserCheckLostConnection() ) {
			ArLog::log (ArLog::Terse,
			            "%s:  Lost connection to the laser because of error.  Nothing received for %g seconds (greater than the timeout of %g).",
			            getName(), myLastReading.mSecSince() / 1000.0,
			            getConnectionTimeoutSeconds() );
			myIsConnected = false;
			
			laserDisconnectOnError();
			continue;
		}
	}
	
	ArUtil::sleep (1);
	//ArUtil::sleep(2000);
	//ArUtil::sleep(500);
	
#endif // end PS 10/12/11
	
}
	return NULL;
}

static const unsigned short
crc_table[256] = { 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
		0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
		0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294,
		0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
		0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7,
		0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf,
		0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
		0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe,
		0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
		0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
		0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50,
		0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58,
		0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
		0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b,
		0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32,
		0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
		0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d,
		0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025,
		0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
		0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214,
		0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f,
		0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
		0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
		0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676,
		0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
		0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1,
		0x3882, 0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8,
		0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
		0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b,
		0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
		0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
		0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2,
		0x0ed1, 0x1ef0 };

unsigned short ArS3SeriesPacketReceiver::CRC16(unsigned char *Data, int length) {
	unsigned short CRC_16 = 0xFFFF;
	int i;
	for (i = 0; i < length; i++)
	{
		CRC_16 = (CRC_16 << 8) ^ (crc_table[(CRC_16 >> 8) ^ (Data[i])]);
	}
	return CRC_16;
}

