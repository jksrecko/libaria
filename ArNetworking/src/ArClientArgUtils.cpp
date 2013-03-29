
#include "Aria.h"
#include "ArExport.h"

#include "ArClientArgUtils.h"

#include <ArConfigArg.h>

#include "ArNetPacket.h"

//#define ARDEBUG_CLIENTARGUTILS

#if (defined(_DEBUG) && defined(ARDEBUG_CLIENTARGUTILS))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

AREXPORT ArClientArg::ArClientArg(bool isDisplayHintParsed,
                                  ArPriority::Priority lastPriority) :
  myIsDisplayHintParsed(isDisplayHintParsed),
  myLastPriority(lastPriority),
  myBuffer(),
  myDisplayBuffer()
{}

AREXPORT ArClientArg::~ArClientArg()
{}

AREXPORT bool ArClientArg::isSendableParamType(const ArConfigArg &arg)
{
  switch (arg.getType()) {
  case ArConfigArg::INT:
  case ArConfigArg::DOUBLE:
  case ArConfigArg::BOOL:
  case ArConfigArg::STRING:
  case ArConfigArg::SEPARATOR:
    return true;

  default:
    return false;
  }
}


AREXPORT bool ArClientArg::createArg(ArNetPacket *packet, 
							                       ArConfigArg &argOut)
{
	if (packet == NULL) {
    ArLog::log(ArLog::Verbose, "ArClientArg::createArg() cannot unpack NULL packet");
		return false;
	}

	bool isSuccess = true;

	char name[32000];
	char description[32000];
  myDisplayBuffer[0] = '\0';

	packet->bufToStr(name, sizeof(name));
	packet->bufToStr(description, sizeof(description));


	char priorityVal = packet->bufToByte();
 
	ArPriority::Priority priority = myLastPriority;
  if ((priorityVal >= 0) && (priorityVal <= myLastPriority)) {
    priority = (ArPriority::Priority) priorityVal;
  }

	char argType = packet->bufToByte();

  switch (argType) {
  case 'B':
	  {
		  bool boolVal = false;
		  if (packet->bufToByte()) {
			  boolVal = true;
		  }
		  argOut = ArConfigArg(name, boolVal, description);
	  }
    break;

  case 'b': // Lower case indicates display information contained in packet...
	  {
      if (myIsDisplayHintParsed) {

		    bool boolVal = false;
		    if (packet->bufToByte()) {
			    boolVal = true;
		    }
		    //packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
		    argOut = ArConfigArg(name, boolVal, description);
      }
      else {
        isSuccess = false;
      }
	  }
    break;

  case 'I':
	  {
		  int intVal = packet->bufToByte4();
		  int intMin = packet->bufToByte4();
		  int intMax = packet->bufToByte4();

		  argOut = ArConfigArg(name, intVal, description, intMin, intMax);
	  }
    break;

  case 'i':  // Lower case indicates display information contained in packet...
	  {
      if (myIsDisplayHintParsed) {

		    int intVal = packet->bufToByte4();
		    int intMin = packet->bufToByte4();
		    int intMax = packet->bufToByte4();

			  //packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
  	    argOut = ArConfigArg(name, intVal, description, intMin, intMax);
      }
      else {
        isSuccess = false;
      }
	  }
    break;


  case 'D':
	  {
		double doubleVal = packet->bufToDouble();
		double doubleMin = packet->bufToDouble();
		double doubleMax = packet->bufToDouble();

		argOut = ArConfigArg(name, doubleVal, description, doubleMin, doubleMax);
	  }
    break;

  case 'd': // Lower case indicates display information contained in packet...
	  {
      if (myIsDisplayHintParsed) {

		    double doubleVal = packet->bufToDouble();
		    double doubleMin = packet->bufToDouble();
		    double doubleMax = packet->bufToDouble();

			  //packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
		    argOut = ArConfigArg(name, 
                             doubleVal, 
                             description, 
                             doubleMin, 
                             doubleMax);
      }
      else {
        isSuccess = false;
      }
	  }
    break;

  case 'S':
	  {
		packet->bufToStr(myBuffer, BUFFER_LENGTH);
		argOut = ArConfigArg(name, myBuffer, description, 0);
	  }  
    break;

  case 's': // Lower case indicates display information contained in packet...
	  {
      if (myIsDisplayHintParsed) {

		    packet->bufToStr(myBuffer, BUFFER_LENGTH);
				
        //packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
  	    argOut = ArConfigArg(name, myBuffer, description, 0);
      }
      else {
        isSuccess = false;
      }
	  }  
    break;

  case '.':
    {
       //if (myIsDisplayHintParsed) {
			 //packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
       //}
       argOut = ArConfigArg(ArConfigArg::SEPARATOR);
    }
    break;

  default:

		isSuccess = false;
    ArLog::log(ArLog::Terse, "ArClientArg::createArg() unsupported param type %c",
               argType);
	}

  argOut.setConfigPriority(priority);
  if (myIsDisplayHintParsed) {

    if (isSuccess) {
		  packet->bufToStr(myDisplayBuffer, BUFFER_LENGTH);
    }

    IFDEBUG(
      if (strlen(myDisplayBuffer) > 0) {
        ArLog::log(ArLog::Verbose, "ArClientArg::createArg() arg %s has displayHint = %s",
                  argOut.getName(), myDisplayBuffer);
      }
    );

    argOut.setDisplayHint(myDisplayBuffer);
  }

	return isSuccess;

} // end method createArg


AREXPORT bool ArClientArg::createPacket(const ArConfigArg &arg,
                                        ArNetPacket *packet)
{
  if (packet == NULL) {
    ArLog::log(ArLog::Verbose, 
               "ArClientArg::createPacket() cannot create NULL packet");
    return false;
  }

	bool isSuccess = true;

	packet->strToBuf(arg.getName());
	packet->strToBuf(arg.getDescription());
  
  ArPriority::Priority priority = arg.getConfigPriority(); 
  if (priority > myLastPriority) {
    priority = myLastPriority;
  }
 
	packet->byteToBuf(priority);

	char argType = '\0';
  
  switch (arg.getType()) {
  case ArConfigArg::BOOL:

    argType = (myIsDisplayHintParsed ? 'b' : 'B');

    packet->byteToBuf(argType);
    packet->byteToBuf(arg.getBool());
    break;
  
  case ArConfigArg::INT:

    argType = (myIsDisplayHintParsed ? 'i' : 'I');
    packet->byteToBuf(argType);

		packet->byte4ToBuf(arg.getInt());
		packet->byte4ToBuf(arg.getMinInt());
		packet->byte4ToBuf(arg.getMaxInt());

    break;

  case ArConfigArg::DOUBLE:

    argType = (myIsDisplayHintParsed ? 'd' : 'D');
    packet->byteToBuf(argType);

		packet->doubleToBuf(arg.getDouble());
		packet->doubleToBuf(arg.getMinDouble());
		packet->doubleToBuf(arg.getMaxDouble());
    break;

  case ArConfigArg::STRING:

    argType = (myIsDisplayHintParsed ? 's' : 'S');
    packet->byteToBuf(argType);

		packet->strToBuf(arg.getString());

    break;

  case ArConfigArg::SEPARATOR:

    argType = '.';
    packet->byteToBuf(argType);
    break;

  default:

		isSuccess = false;
    ArLog::log(ArLog::Terse, "ArClientArg::createPacket() unsupported param type %i", arg.getType());

  } // end switch type

  if (isSuccess && myIsDisplayHintParsed) {
    packet->strToBuf(arg.getDisplayHint());
  }

	return isSuccess;

} // end method createPacket



AREXPORT bool ArClientArg::bufToArgValue(ArNetPacket *packet,
                                ArConfigArg &arg)
{
  if (packet == NULL) {
    return false;
  }

  bool isSuccess = true;

  switch (arg.getType()) {
  case ArConfigArg::BOOL:
    {
		bool boolVal = false;
		if (packet->bufToByte()) {
			boolVal = true;
		}
    isSuccess = arg.setBool(boolVal);
    }
    break;
  
  case ArConfigArg::INT:
    {
		int intVal = packet->bufToByte4();
    isSuccess = arg.setInt(intVal);
    }
    break;

  case ArConfigArg::DOUBLE:
    {
    double doubleVal = packet->bufToDouble();
    isSuccess = arg.setDouble(doubleVal);
    }
    break;

  case ArConfigArg::STRING:
    {
		packet->bufToStr(myBuffer, BUFFER_LENGTH);
    isSuccess = arg.setString(myBuffer);
    }
    break;


  case ArConfigArg::SEPARATOR:
    // There is no value...
    break;

  default:

		isSuccess = false;
    ArLog::log(ArLog::Terse, "ArClientArg::createPacket() unsupported param type %i", arg.getType());

  } // end switch type

  return isSuccess;

} // end method bufToArgValue



AREXPORT bool ArClientArg::argValueToBuf(const ArConfigArg &arg,
                                ArNetPacket *packet)
{
  if (packet == NULL) {
    return false;
  }

  bool isSuccess = true;

  switch (arg.getType()) {
  case ArConfigArg::BOOL:

    packet->byteToBuf(arg.getBool());
    break;
  
  case ArConfigArg::INT:

		packet->byte4ToBuf(arg.getInt());
    break;

  case ArConfigArg::DOUBLE:

    packet->doubleToBuf(arg.getDouble());
    break;

  case ArConfigArg::STRING:

		packet->strToBuf(arg.getString());
    break;

  case ArConfigArg::SEPARATOR:
    // There is no value...
    break;

  default:

		isSuccess = false;
    ArLog::log(ArLog::Terse, "ArClientArg::createPacket() unsupported param type %i", arg.getType());

  } // end switch type

  return isSuccess;

} // end method argValueToBuf


AREXPORT bool ArClientArg::argTextToBuf(const ArConfigArg &arg,
                               ArNetPacket *packet)
{
  if (packet == NULL) {
    return false;
  }

  bool isSuccess = true;

  switch (arg.getType()) {
  case ArConfigArg::INT:
    snprintf(myBuffer, BUFFER_LENGTH, "%d", arg.getInt());
    break;
  case ArConfigArg::DOUBLE:
    snprintf(myBuffer, BUFFER_LENGTH, "%g", arg.getDouble());
    break;
  case ArConfigArg::BOOL:
    snprintf(myBuffer, BUFFER_LENGTH, "%s", 
             ArUtil::convertBool(arg.getBool()));
    break;
  case ArConfigArg::STRING:
    snprintf(myBuffer, BUFFER_LENGTH, "%s", arg.getString());
    break;
  case ArConfigArg::SEPARATOR:
    break;
  default:
    isSuccess = false;
    break;
  } // end switch type

  if (isSuccess) {
    packet->strToBuf(myBuffer);
  }
  return isSuccess;

} // end method argTextToBuf
