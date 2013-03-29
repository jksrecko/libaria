#include "Aria.h"
#include "ArExport.h"
#include "ArClientHandlerConfig.h"
#include "ArClientArgUtils.h"

//#define ARDEBUG_CLIENTHANDLERCONFIG

#if (defined(_DEBUG) && defined(ARDEBUG_CLIENTHANDLERCONFIG))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

/**
   @param client the client base to attach to
   @param ignoreBounds whether the ArConfig we have should ignore bounds or not, this should only be used for debugging
   @param robotName a name or identifier for the robot the server is controlling, used for logging etc.
*/
AREXPORT ArClientHandlerConfig::ArClientHandlerConfig(ArClientBase *client,
						                                          bool ignoreBounds,
                                                      const char *robotName) :
  myRobotName((robotName != NULL) ? robotName : ""),
  myLogPrefix(""),

  myConfig(NULL, false, ignoreBounds),
  myDefaultConfig(NULL),

  myHandleGetConfigBySectionsCB(this, &ArClientHandlerConfig::handleGetConfigBySections),
  myHandleGetConfigCB(this, &ArClientHandlerConfig::handleGetConfig),
  myHandleSetConfigCB(this, &ArClientHandlerConfig::handleSetConfig),
  myHandleGetConfigDefaultsCB(this, 
			      &ArClientHandlerConfig::handleGetConfigDefaults),
  myHandleGetConfigSectionFlagsCB(this, 
				  &ArClientHandlerConfig::handleGetConfigSectionFlags)
{
  myDataMutex.setLogName("ArClientConfigHandler::myDataMutex");
  myCallbackMutex.setLogName("ArClientConfigHandler::myCallbackMutex");

  myClient = client;
  myHaveGottenConfig = false;
  myHaveRequestedDefaults = false;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myIsQuiet = false;

  if (!myRobotName.empty()) {
    myLogPrefix = myRobotName + ": ";
  }

  myConfig.setConfigName("Server", myRobotName.c_str());

}

AREXPORT ArClientHandlerConfig::~ArClientHandlerConfig()
{
  if (myDefaultConfig != NULL)
    delete myDefaultConfig;
}

/**
   This requests a config from the server and resets it so we haven't
   gotten a config.
 **/
AREXPORT void ArClientHandlerConfig::requestConfigFromServer(void)
{

  char *getConfigPacketName = "getConfigBySections";
  bool isInsertPriority = true;

  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> *getConfigCB = &myHandleGetConfigBySectionsCB;

  if (!myClient->dataExists(getConfigPacketName)) {
    getConfigPacketName = "getConfig";
    isInsertPriority = false;

    getConfigCB = &myHandleGetConfigCB;
  }

  myDataMutex.lock();
  ArLog::log(ArLog::Verbose, "%sRequesting config from server, and clearing sections...", 
              myLogPrefix.c_str());
  myConfig.clearSections();
  myDataMutex.unlock();

  myClient->remHandler(getConfigPacketName, getConfigCB);
  myClient->addHandler(getConfigPacketName, getConfigCB);
  myClient->remHandler("setConfig", &myHandleSetConfigCB);
  myClient->addHandler("setConfig", &myHandleSetConfigCB);

  if (myClient->dataExists("getConfigDefaults")) {
    myClient->remHandler("getConfigDefaults", &myHandleGetConfigDefaultsCB);
    myClient->addHandler("getConfigDefaults", &myHandleGetConfigDefaultsCB);
  }
  if (myClient->dataExists("getConfigSectionFlags"))
  {
    myClient->remHandler("getConfigSectionFlags", 
			 &myHandleGetConfigSectionFlagsCB);
    myClient->addHandler("getConfigSectionFlags", 
			 &myHandleGetConfigSectionFlagsCB);
    myClient->requestOnce("getConfigSectionFlags");
  }

  if (isInsertPriority) {
    ArLog::log(ArLog::Verbose,
               "%sRequesting that config has last priority value %i",
               myLogPrefix.c_str(),
               ArPriority::LAST_PRIORITY);

    ArNetPacket packet;
    packet.byteToBuf(ArPriority::LAST_PRIORITY);
    myClient->requestOnce(getConfigPacketName, &packet);
  }
  else { // don't insert priority
    myClient->requestOnce(getConfigPacketName);
  } // end else don't insert priority

  myDataMutex.lock();
  myHaveGottenConfig = false;  
  myDataMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::handleGetConfigBySections(ArNetPacket *packet)
{
  handleGetConfigData(packet, true);
}

AREXPORT void ArClientHandlerConfig::handleGetConfig(ArNetPacket *packet)
{
  handleGetConfigData(packet, false);
}

AREXPORT void ArClientHandlerConfig::handleGetConfigData(ArNetPacket *packet,
                                                         bool isMultiplePackets)
{
  char name[32000];
  char comment[32000];
  char type;
  std::string section;
  
  // The multiple packets method also sends display hints with the parameters;
  // the old single packet method does not.
  ArClientArg clientArg(isMultiplePackets, 
                        ArPriority::LAST_PRIORITY);
  bool isEmptyPacket = true;

  myDataMutex.lock();

  while (packet->getDataReadLength() < packet->getDataLength())
  {
    isEmptyPacket = false;

    type = packet->bufToByte();

    if (type == 'S')
    {
      packet->bufToStr(name, sizeof(name));
      packet->bufToStr(comment, sizeof(name));
      //printf("%c %s %s\n", type, name, comment);

      ArLog::log(ArLog::Verbose, "%sReceiving config section %s...", 
                 myLogPrefix.c_str(), name);
      
      section = name;
      myConfig.setSectionComment(name, comment);
    }
    else if (type == 'P')
    {
		  ArConfigArg configArg;

		  bool isSuccess = clientArg.createArg(packet,
											                     configArg);

		  if (isSuccess) {
        
			  myConfig.addParam(configArg,
							            section.c_str(),
							            configArg.getConfigPriority(),
                          configArg.getDisplayHint());
		  }
		  else
		  {
			  ArLog::log(ArLog::Terse, "ArClientHandlerConfig unknown param type");
		  }
    }
    else // unrecognized header
    {
      ArLog::log(ArLog::Terse, "ArClientHandlerConfig unknown type");
    }
  
  } // end while more to read

  if (!isMultiplePackets || isEmptyPacket) {

    ArLog::log(ArLog::Normal, "%sGot config from server.", myLogPrefix.c_str());
    IFDEBUG(myConfig.log());

    myHaveGottenConfig = true;
  }

  myDataMutex.unlock();



  if (myHaveGottenConfig) {

    myCallbackMutex.lock();

    for (std::list<ArFunctor *>::iterator it = myGotConfigCBList.begin(); 
        it != myGotConfigCBList.end(); 
        it++) {
      (*it)->invoke();
    }
    myCallbackMutex.unlock();

  } // end if config received

} // end method handleGetConfigData

AREXPORT void ArClientHandlerConfig::handleGetConfigSectionFlags(
	ArNetPacket *packet) 
{
  int numSections = packet->bufToByte4();
  
  int i;

  char section[32000];
  char flags[32000];

  myDataMutex.lock();
  for (i = 0; i < numSections; i++)
  {
    packet->bufToStr(section, sizeof(section));
    packet->bufToStr(flags, sizeof(flags));
    myConfig.addSectionFlags(section, flags);
  }
  myDataMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::saveConfigToServer(void)
{
  saveConfigToServer(&myConfig);
}

AREXPORT void ArClientHandlerConfig::saveConfigToServer(
	  ArConfig *config, 
	  const std::set<std::string, ArStrCaseCmpOp> *ignoreTheseSections)
{
  //ArConfigArg param;
  ArClientArg clientArg;

  ArNetPacket sending;
  ArLog::log(ArLog::Normal, "%sSaving config to server", myLogPrefix.c_str());

  myDataMutex.lock();
  std::list<ArConfigSection *> *sections = config->getSections();
  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       sIt != sections->end(); 
       sIt++)
  {
    ArConfigSection *section = (*sIt);
    // if we're ignoring sections and we're ignoring this one, then
    // don't send it
    if (ignoreTheseSections != NULL && 
	(ignoreTheseSections->find(section->getName()) != 
	 ignoreTheseSections->end()))
    {
      ArLog::log(ArLog::Verbose, "Not sending section %s", 
		 section->getName());
      continue;
    }
    sending.strToBuf("Section");
    sending.strToBuf(section->getName());
    std::list<ArConfigArg> *params = section->getParams();

    for (std::list<ArConfigArg>::iterator pIt = params->begin(); 
         pIt != params->end(); 
         pIt++)
    {
      ArConfigArg &param = (*pIt);

      if (!clientArg.isSendableParamType(param)) {
        continue;
      }

      sending.strToBuf(param.getName());

      clientArg.argTextToBuf(param, &sending);

    } // end for each param
  } // end for each section

  myDataMutex.unlock();
  myClient->requestOnce("setConfig", &sending);
}

AREXPORT void ArClientHandlerConfig::handleSetConfig(ArNetPacket *packet)
{
  
  char buffer[1024];
  packet->bufToStr(buffer, sizeof(buffer));

  myCallbackMutex.lock();
  if (buffer[0] == '\0')
  {
    ArLog::log(ArLog::Normal, "%sSaved config to server successfully", myLogPrefix.c_str());
    std::list<ArFunctor *>::iterator it;
    for (it = mySaveConfigSucceededCBList.begin(); 
	       it != mySaveConfigSucceededCBList.end(); 
	       it++)
      (*it)->invoke();
  }
  else
  {
    ArLog::log(ArLog::Normal, "%sSaving config to server had error: %s", myLogPrefix.c_str(), buffer);
    std::list<ArFunctor1<const char *> *>::iterator it;
    for (it = mySaveConfigFailedCBList.begin(); 
	       it != mySaveConfigFailedCBList.end(); 
	       it++)
      (*it)->invoke(buffer);
  }
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::reloadConfigOnServer(void)
{
  myClient->requestOnce("reloadConfig");
}

/**
   This will get the config, note, it gets the config we're using, so
   if you change something while data comes in things will get broken
 **/
AREXPORT ArConfig *ArClientHandlerConfig::getConfig(void)
{
  return &myConfig;
}

/**
 * Returns a pointer to the robot server's default configuration, if
 * canRequestDefaults() is true.  Note that both requestConfigFromServer() 
 * and then requestDefaultConfigFromServer() must be called before a 
 * valid default configuration is available on the client.
 * If there is no default configuration, then NULL is returned.
**/
AREXPORT ArConfig *ArClientHandlerConfig::getDefaultConfig(void)
{
  return myDefaultConfig;
}

/**
   This will get a copy of the config, you can use this with the
   safeConfigToServer that takes an argument
 **/
AREXPORT ArConfig ArClientHandlerConfig::getConfigCopy(void)
{
  ArConfig copy;
  myDataMutex.lock();
  copy = myConfig;
  myDataMutex.unlock();
  return copy;
}

AREXPORT int ArClientHandlerConfig::lock(void)
{
  return myDataMutex.lock();
}

AREXPORT int ArClientHandlerConfig::tryLock(void)
{
  return myDataMutex.tryLock();
}

AREXPORT int ArClientHandlerConfig::unlock(void)
{
  return myDataMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::setQuiet(bool isQuiet)
{
  myIsQuiet = isQuiet;
  myConfig.setQuiet(isQuiet);
  if (myDefaultConfig != NULL) {
    myDefaultConfig->setQuiet(isQuiet);
  }

}

AREXPORT bool ArClientHandlerConfig::haveGottenConfig(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveGottenConfig;
  myDataMutex.unlock();
  return ret;
}

AREXPORT void ArClientHandlerConfig::addGotConfigCB(ArFunctor *functor,
						    ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myGotConfigCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myGotConfigCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addGotConfigCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remGotConfigCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myGotConfigCBList.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::addSaveConfigSucceededCB(
	ArFunctor *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    mySaveConfigSucceededCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    mySaveConfigSucceededCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addSaveConfigSucceededCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remSaveConfigSucceededCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  mySaveConfigSucceededCBList.remove(functor);
  myCallbackMutex.unlock();
}


AREXPORT void ArClientHandlerConfig::addSaveConfigFailedCB(
	ArFunctor1<const char *> *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    mySaveConfigFailedCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    mySaveConfigFailedCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addSaveConfigFailedCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remSaveConfigFailedCB(ArFunctor1<const char *> *functor)
{
  myCallbackMutex.lock();
  mySaveConfigFailedCBList.remove(functor);
  myCallbackMutex.unlock();
}


AREXPORT void ArClientHandlerConfig::addGotConfigDefaultsCB(
	ArFunctor *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myGotConfigDefaultsCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myGotConfigDefaultsCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addGotConfigDefaultsCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remGotConfigDefaultsCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myGotConfigDefaultsCBList.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT bool ArClientHandlerConfig::haveRequestedDefaults(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveRequestedDefaults || myHaveRequestedDefaultCopy;
  myDataMutex.unlock();
  return ret;
}

AREXPORT bool ArClientHandlerConfig::haveGottenDefaults(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveGottenDefaults;
  myDataMutex.unlock();
  return ret;
}

AREXPORT bool ArClientHandlerConfig::canRequestDefaults(void)
{
  return myClient->dataExists("getConfigDefaults");
}

AREXPORT bool ArClientHandlerConfig::requestConfigDefaults(void)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Cannot request defaults as there are already some being requested",
         myLogPrefix.c_str());
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }

  ArLog::log(ArLog::Normal, 
	           "%sRequesting config reset to default",
             myLogPrefix.c_str());

  myDataMutex.lock();
  myHaveRequestedDefaults = true;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myDataMutex.unlock();

  myClient->requestOnce("getConfigDefaults");
  return true;
}

AREXPORT bool ArClientHandlerConfig::requestDefaultConfigFromServer(void)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%requestDefaultConfigFromServer: Cannot request defaults as there are already some being requested",
         myLogPrefix.c_str());
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }

  
  myDataMutex.lock();

  myHaveRequestedDefaults = false;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = true;
  myDataMutex.unlock();

  myClient->requestOnce("getConfigDefaults");
  return true;
}

AREXPORT bool ArClientHandlerConfig::requestSectionDefaults(
	const char *section)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "RequestSectionDefaults: Cannot request defaults as there are already some being requested");
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestSectionDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }
  myDataMutex.lock();
  if (myConfig.findSection(section) == NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestSectionDefaults: Section '%s' requested but doesn't exist", 
         myLogPrefix.c_str(), section);
    myDataMutex.unlock();
    return false;
  }

  ArLog::log(ArLog::Normal, 
	           "%sRequesting config section %s reset to default",
             myLogPrefix.c_str(),
             section);

  myHaveRequestedDefaults = true;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myDataMutex.unlock();
  myClient->requestOnceWithString("getConfigDefaults", section);
  return true;
}



AREXPORT void ArClientHandlerConfig::handleGetConfigDefaults(
	ArNetPacket *packet)
{
  ArLog::log(ArLog::Normal, "%sreceived default config %s", 
		             myLogPrefix.c_str(), 
                 ((myHaveRequestedDefaultCopy) ? "(copy)" : "(reset)"));

  char param[1024];
  char argument[1024];
  char errorBuffer[1024];
  
  myDataMutex.lock();

  ArConfig *config = NULL;

  // If the config (or a section) is being reset to its default values,
  // then we don't want to remove any parameters that are not set -- i.e.
  // any parameters that are not contained in the default config.
  bool isClearUnsetValues = false;

  if (myHaveRequestedDefaults) {

    config = &myConfig;
  }
  else if (myHaveRequestedDefaultCopy) {

    // If we have requested a copy of the default configuration, then we
    // will want to remove any parameters that haven't been explicitly set.
    // (This is because of the next line, which copies the current config 
    // to the default config.)
    isClearUnsetValues = true;

    // The default config is transmitted in an "abbreviated" form -- just 
    // the section/param names and values.  Copy the current config to the
    // default before processing the packet so that the parameter types, etc.
    // can be preserved.
    if (myDefaultConfig == NULL) {
      myDefaultConfig = new ArConfig(myConfig);
      myDefaultConfig->setConfigName("Default", myRobotName.c_str());
      myDefaultConfig->setQuiet(myIsQuiet);
    }
    else {
      *myDefaultConfig = myConfig;
    }


    config = myDefaultConfig;
  }
  // if we didn't ask for any of these, then just return since the
  // data is for someone else
  else
  {
    myDataMutex.unlock();
    return;
  }

  if (config == NULL) {
    ArLog::log(ArLog::Normal,
               "%serror determining config to populate with default values",
               myLogPrefix.c_str());
  myDataMutex.unlock();
    return;
  }

  ArArgumentBuilder *builder = NULL;
  ArLog::log(ArLog::Normal, "%sGot defaults", myLogPrefix.c_str());
  errorBuffer[0] = '\0';
  
  //myDataMutex.lock();
  if (isClearUnsetValues) {
    config->clearAllValueSet();
  }

  while (packet->getDataReadLength() < packet->getDataLength())
  {
    packet->bufToStr(param, sizeof(param));  
    packet->bufToStr(argument, sizeof(argument));  


    builder = new ArArgumentBuilder;
    builder->setQuiet(myIsQuiet);
    builder->setExtraString(param);
    builder->add(argument);

    if ((strcasecmp(param, "Section") == 0 && 
        !config->parseSection(builder, errorBuffer, sizeof(errorBuffer))) ||
        (strcasecmp(param, "Section") != 0 &&
        !config->parseArgument(builder, errorBuffer, sizeof(errorBuffer))))
    {
      ArLog::log(ArLog::Terse, "%shandleGetConfigDefaults: Hideous problem getting defaults, couldn't parse '%s %s'", 
		             myLogPrefix.c_str(), param, argument);
    }
    else {
      IFDEBUG(if (strlen(param) > 0) {
                 ArLog::log(ArLog::Normal, "%shandleGetConfigDefaults: added default '%s %s'", 
                 myLogPrefix.c_str(), param, argument); } );
    }
    delete builder;
    builder = NULL;
  }
  myHaveRequestedDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myHaveGottenDefaults = true;

  if (isClearUnsetValues) {
    config->removeAllUnsetValues();
  }

  IFDEBUG(config->log());

  myDataMutex.unlock();

  myCallbackMutex.lock();
  std::list<ArFunctor *>::iterator it;
  for (it = myGotConfigDefaultsCBList.begin(); 
       it != myGotConfigDefaultsCBList.end(); 
       it++)
    (*it)->invoke();
  myCallbackMutex.unlock();
}

