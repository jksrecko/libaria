#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerConfig.h"

#include "ArClientArgUtils.h"

/**
@param server the server to add data to

@param config the config to serve up

@param defaultFile if this is given the config will try to copy the
config given and then load the default file into that config and
then serve data so clients can get those defaults

@param defaultFileBaseDirectory base directory for the default file
**/
AREXPORT ArServerHandlerConfig::ArServerHandlerConfig(ArServerBase *server, 
                                                      ArConfig *config, 
                                                      const char *defaultFile,
                                                      const char *defaultFileBaseDirectory) :
  myServer(server),
  myConfig(config),
  myDefault(NULL),
  myPreWriteCallbacks(),
  myPostWriteCallbacks(),
  myGetConfigBySectionsCB(this, &ArServerHandlerConfig::getConfigBySections),
  myGetConfigCB(this, &ArServerHandlerConfig::getConfig),
  mySetConfigCB(this, &ArServerHandlerConfig::setConfig),
  myReloadConfigCB(this, &ArServerHandlerConfig::reloadConfig),
  myGetConfigDefaultsCB(this, &ArServerHandlerConfig::getConfigDefaults),
  myGetConfigSectionFlagsCB(this, 
			    &ArServerHandlerConfig::getConfigSectionFlags)
{
  myDefaultConfigMutex.setLogName(
	  "ArServerHandlerConfig::myDefaultConfigMutex");
  myConfigMutex.setLogName("ArServerHandlerConfig::myConfigMutex");
  myAddedDefaultServerCommands = false;

  myServer->addData("getConfigBySections", 
                    "Gets the configuration information from the server", 
                    &myGetConfigBySectionsCB, 
                    "none", 
                    "A single packet is sent for each config section.  Too complex to describe here.  Use ArClientHandlerConfig if desired.", 
                    "ConfigEditing", "RETURN_UNTIL_EMPTY");


  myServer->addData("getConfig", 
    "gets the configuration information from the server", 
    &myGetConfigCB, 
    "none", 
    "deprecated (getConfigBySections is preferred).  Too complex to describe here.  Use ArClientHandlerConfig if desired.", 
    "ConfigEditing", "RETURN_SINGLE");

  myServer->addData("setConfig", 
    "takes a config back from the client to use",
    &mySetConfigCB,
    "Repeating pairs of strings which are parameter name and value to parse",
    "string: if empty setConfig worked, if the string isn't empty then it is the first error that occured (all non-error parameters are parsed, and only the first error is reported)",
    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");

  myServer->addData("reloadConfig", 
    "reloads the configuration file last loaded",
    &myReloadConfigCB, "none", "none",
    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");

  myServer->addData("configUpdated", 
    "gets sent when the config is updated",
    NULL, "none", "none",
    "ConfigEditing", "RETURN_SINGLE");

  myServer->addData("getConfigSectionFlags", 
    "gets the flags for each section of the config", 
    &myGetConfigSectionFlagsCB, 
    "none", 
    "byte4: number of sections; repeating for number of sections (string: section; string: flags (separated by |))",
    "ConfigEditing", "RETURN_SINGLE");

  if (defaultFile != NULL)
    myDefaultFile = defaultFile;
  if (defaultFileBaseDirectory != NULL)
    myDefaultFileBaseDir = defaultFileBaseDirectory;
  loadDefaultsFromFile();
}

AREXPORT ArServerHandlerConfig::~ArServerHandlerConfig()
{
  if (myDefault != NULL)
    delete myDefault;
}

AREXPORT bool ArServerHandlerConfig::loadDefaultsFromFile(void)
{
  bool ret = true;

  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  if (!myDefaultFile.empty())
  {
    createDefaultConfig(myDefaultFileBaseDir.c_str());
    myDefault->clearAllValueSet();
    // now fill in that copy
    if (myDefault->parseFile(myDefaultFile.c_str()))
    {
      addDefaultServerCommands();
    }
    else
    {
      ret = false;
      ArLog::log(ArLog::Normal, "Did not load default file '%s' successfully, not allowing getDefault", myDefaultFile.c_str());
      delete myDefault;
      myDefault = NULL;
    }
    if (myDefault != NULL)
      myDefault->removeAllUnsetValues();
  }
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
  return ret;
}

AREXPORT bool ArServerHandlerConfig::loadDefaultsFromPacket(
	ArNetPacket *packet)
{
  bool ret = true;

  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  createDefaultConfig(NULL);
  myDefault->clearAllValueSet();
  // now fill in that copy
  if (internalSetConfig(NULL, packet))
  {
      addDefaultServerCommands();    
  }
  else
  {
    ArLog::log(ArLog::Normal, "Did not load default from packet successfully, not allowing getDefault");
    delete myDefault;
    myDefault = NULL;
  }
  if (myDefault != NULL)
    myDefault->removeAllUnsetValues();
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
  return ret;
}

AREXPORT void ArServerHandlerConfig::createEmptyConfigDefaults(void)
{
  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  addDefaultServerCommands();    
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
}

/** 
    @internal

    doesn't delete the old one, do that if you're going to call this
    yourself and make sure you lock around all that (okay, it deletes
    it now, but the stuff that calls it should still take care of it)
**/
void ArServerHandlerConfig::createDefaultConfig(const char *defaultFileBaseDir)
{
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;
  }
  // copy that config (basedir will be NULL if we're not loading from
  // a file)... don't have the default save unknown values
  myDefault = new ArConfig(defaultFileBaseDir, false, false, false, false);

  std::list<ArConfigSection *>::iterator sectionIt;
  std::list<ArConfigArg>::iterator paramIt;
  ArConfigSection *section = NULL;
  std::list<ArConfigArg> *params = NULL;
  ArConfigArg param;
  for (sectionIt = myConfig->getSections()->begin(); 
       sectionIt != myConfig->getSections()->end(); 
       sectionIt++)
  {
    section = (*sectionIt);
    params = section->getParams();
    for (paramIt = params->begin(); paramIt != params->end(); paramIt++)
    {
      param = (*paramIt);
      switch (param.getType()) {
      case ArConfigArg::INT:
	myDefault->addParam(
		ArConfigArg(param.getName(), param.getInt(), 
			    param.getDescription(), 
			    param.getMinInt(), param.getMaxInt()), 
		section->getName(), 
		param.getConfigPriority(),
		param.getDisplayHint());
	break;
      case ArConfigArg::DOUBLE:
	myDefault->addParam(
		ArConfigArg(param.getName(), param.getDouble(), 
			    param.getDescription(),
			    param.getMinDouble(), param.getMaxDouble()), 
		section->getName(), 
		param.getConfigPriority(),
		param.getDisplayHint());
	break;
	
      case ArConfigArg::BOOL:
	myDefault->addParam(
		ArConfigArg(param.getName(), param.getBool(), 
			    param.getDescription()),
		section->getName(), 
		param.getConfigPriority(),
		param.getDisplayHint());
	break;
	
      case ArConfigArg::STRING:
	myDefault->addParam(
		ArConfigArg(param.getName(), (char *)param.getString(), 
			    param.getDescription(), 0),
		section->getName(), 
		param.getConfigPriority(),
		param.getDisplayHint());
	break;
	
      case ArConfigArg::SEPARATOR:
	myDefault->addParam(
		ArConfigArg(ArConfigArg::SEPARATOR),
		section->getName(), 
		param.getConfigPriority(),
		param.getDisplayHint());
	break;
      default:
	break;
      } // end switch param type
    } // end for each param
  } // end for each section
}

void ArServerHandlerConfig::addDefaultServerCommands(void)
{
  if (myAddedDefaultServerCommands)
    return;

  myServer->addData("getConfigDefaults", 
		    "Gets the config default values ",
		    &myGetConfigDefaultsCB, 
		    "string: section to load, empty string means get the whole thing", 
		    "repeating strings that are the parameters and arguments to parse, but use ArClientHandlerConfig to handle this if you want",
		    "ConfigEditing");
  myServer->addData("configDefaultsUpdated", 
		    "Gets sent when the config defaults are updated",
		    NULL, "none", "none",
		    "ConfigEditing", "RETURN_SINGLE");
  myAddedDefaultServerCommands = true;
}

/**
 * @param client the ArServerClient * to which to send the config
 * @param packet the ArNetPacket * which accompanied the client's request
 * @param isMultiplePackets a bool set to true if the server should send a
 * packet for each config section followed by the empty packet; false if 
 * the server should send the entire config in one packet (i.e. the old style)
 * @param lastPriority the last ArPriority::Priority that should be sent 
 * to the client (this is the greatest numerical value and the least 
 * semantic priority).
**/
AREXPORT void ArServerHandlerConfig::handleGetConfig(ArServerClient *client, 
                                                     ArNetPacket *packet,
                                                     bool isMultiplePackets,
                                                     ArPriority::Priority lastPriority)
{

  ArConfigArg param;

 
  // The multiple packets method also sends display hints with the parameters;
  // the old single packet method does not.
  ArClientArg clientArg(isMultiplePackets,
                        lastPriority);

  std::set<std::string> sent;

  ArNetPacket sending;
  ArLog::log(ArLog::Normal, "Config requested.");

  std::list<ArConfigSection *> *sections = myConfig->getSections();
  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       sIt != sections->end(); 
       sIt++)
  {
    // Clear the packet...
    if (isMultiplePackets) {
      sending.empty();
    }

    // clear out the sent list between sections
    sent.clear();

    ArConfigSection *section = (*sIt);
    if (section == NULL) {
      continue;
    }

    sending.byteToBuf('S');
    sending.strToBuf(section->getName());
    sending.strToBuf(section->getComment());

    ArLog::log(ArLog::Verbose, "Sending config section %s...", section->getName());

    //printf("S %s %s\n", section->getName(), section->getComment());
    std::list<ArConfigArg> *params = section->getParams();
    for (std::list<ArConfigArg>::iterator pIt = params->begin(); 
         pIt != params->end(); 
         pIt++)
    {
      param = (*pIt);

      bool isCheckableName = 
      (param.getType() != ArConfigArg::DESCRIPTION_HOLDER && 
       param.getType() != ArConfigArg::SEPARATOR &&
       param.getType() != ArConfigArg::STRING_HOLDER);

      // if we've already sent it don't send it again
      if (isCheckableName &&
          sent.find(param.getName()) != sent.end()) {
        continue;
      }
      else if (isCheckableName) {
        sent.insert(param.getName());
      }

      if (clientArg.isSendableParamType(param))
      {
        sending.byteToBuf('P');

        bool isSuccess = clientArg.createPacket(param,
                                                &sending);

      }
    } // end for each parameter

    if (!sending.isValid()) {

      ArLog::log(ArLog::Terse, "Config section %s cannot be sent; packet size exceeded",
                 section->getName());

    } // end if length exceeded...
    else if (isMultiplePackets) {

      client->sendPacketTcp(&sending);

    } // end else send in chunks...

  } // end for each section

  // If sending each section in individual packets, then send an empty packet 
  // to indicate the end of the config data.
  if (isMultiplePackets) {

    sending.empty();
    client->sendPacketTcp(&sending);
  }
  else { //  send the entire config in one packet

    // If the config is too big to fit in the packet, then just send an empty
    // packet (to try to prevent an older client from crashing)
    // TODO: Is there any better way to notify the user of an error....
    if (!sending.isValid()) {
      ArLog::log(ArLog::Terse, "Error sending config; packet size exceeded");
      sending.empty();
    }

    client->sendPacketTcp(&sending);

  } // end else send the entire packet

} // end method getConfigBySections


AREXPORT void ArServerHandlerConfig::getConfigBySections(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  // The default value of lastPriority is DETAILED because that was the 
  // original last value and some of the older clients cannot handle
  // anything greater.  
  ArPriority::Priority lastPriority = ArPriority::DETAILED;

  // Newer clients insert a priority value into the packet.  This is 
  // the last priority (i.e. greatest numerical value) that the client
  // can handle.
  if ((packet != NULL) &&
      (packet->getDataReadLength() < packet->getDataLength())) {

    char priorityVal = packet->bufToByte();
    ArLog::log(ArLog::Verbose,
               "Sending config with maximum priority value set to %i", 
               priorityVal);

    if ((priorityVal >= 0) && (priorityVal <= ArPriority::LAST_PRIORITY)) {
      lastPriority = (ArPriority::Priority) priorityVal;
    }
    else if (priorityVal > ArPriority::LAST_PRIORITY) {
      // This is just to handle the unlikely case that more priorities 
      // are added.  That is, both the server and client can handle more
      // than DETAILED -- and the client can handle even more than the
      // server.
      lastPriority = ArPriority::LAST_PRIORITY;
    }

  } // end if packet is not empty

  handleGetConfig(client, 
                  packet,
                  true, // multiple packets
                  lastPriority);

} // end method getConfigBySections


AREXPORT void ArServerHandlerConfig::getConfig(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  handleGetConfig(client, 
                  packet,
                  false,  // single packet
                  ArPriority::DETAILED); // older clients can't handle greater

}



AREXPORT void ArServerHandlerConfig::setConfig(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  internalSetConfig(client, packet);
}

/**
   @param client If client is NULL it means use the default config
   @param packet request packet containing config options
**/

bool ArServerHandlerConfig::internalSetConfig(ArServerClient *client, 
					      ArNetPacket *packet)
{
  char param[1024];
  char argument[1024];
  char errorBuffer[1024];
  char firstError[1024];
  ArNetPacket retPacket;
  ArConfig *config;
  bool ret = true;

  if (client != NULL)
    config = myConfig;
  else
    config = myDefault;

  if (client != NULL)
    lockConfig();
  ArArgumentBuilder *builder = NULL;
  if (client != NULL)
    ArLog::log(ArLog::Normal, "Got new config from client %s", client->getIPString());
  else
    ArLog::log(ArLog::Verbose, "New default config");
  errorBuffer[0] = '\0';
  firstError[0] = '\0';

  while (packet->getDataReadLength() < packet->getDataLength())
  {
    packet->bufToStr(param, sizeof(param));  
    packet->bufToStr(argument, sizeof(argument));  

    builder = new ArArgumentBuilder;
    builder->setExtraString(param);
    builder->add(argument);
    ArLog::log(ArLog::Verbose, "Config: %s %s", param, argument);
    // if the param name here is "Section" we need to parse sections,
    // otherwise we parse the argument
    if ((strcasecmp(param, "Section") == 0 && 
        !config->parseSection(builder, errorBuffer, sizeof(errorBuffer))) ||
        (strcasecmp(param, "Section") != 0 &&
        !config->parseArgument(builder, errorBuffer, sizeof(errorBuffer))))
    {
      if (firstError[0] == '\0')
        strcpy(firstError, errorBuffer);
    }
    delete builder;
    builder = NULL;
  }
  if (firstError[0] == '\0')
  {
    if (config->callProcessFileCallBacks(true, 
                                           errorBuffer, 
                                           sizeof(errorBuffer)))
    {
      if (client != NULL)
	ArLog::log(ArLog::Normal, "New config from client %s was fine.",
		   client->getIPString());
      else
	ArLog::log(ArLog::Verbose, "New default config was fine.");
      retPacket.strToBuf("");
      writeConfig();
    }
    else // error processing config callbacks
    {
      ret = false;
      if (firstError[0] == '\0')
        strcpy(firstError, errorBuffer);
      // if its still empty it means we didn't have anything good in the errorBuffer
      if (firstError[0] == '\0')
        strcpy(firstError, "Error processing");

      if (client != NULL)
	ArLog::log(ArLog::Normal, 
		   "New config from client %s had errors processing ('%s').",
		   client->getIPString(), firstError);
      else
	ArLog::log(ArLog::Normal, 
		   "New default config had errors processing ('%s').",
		   firstError);
      retPacket.strToBuf(firstError);
    }
  }
  else
  {
    ret = false;
    if (client != NULL)
      ArLog::log(ArLog::Normal, 
		 "New config from client %s had at least this problem: %s", 
		 client->getIPString(), firstError);
    else
      ArLog::log(ArLog::Normal, 
		 "New default config had at least this problem: %s", 
		 firstError);
    retPacket.strToBuf(firstError);
  }
  //printf("Sending ");
  //retPacket.log();
  if (client != NULL)
    client->sendPacketTcp(&retPacket);
  if (client != NULL)
    unlockConfig();
  if (client != NULL)
    configUpdated(client);

  return ret;
}

AREXPORT void ArServerHandlerConfig::reloadConfig(ArServerClient *client, 
                                                  ArNetPacket *packet)
{
  myConfig->parseFile(myConfig->getFileName(), true);
  configUpdated();
}

AREXPORT void ArServerHandlerConfig::getConfigDefaults(ArServerClient *client, 
                                                       ArNetPacket *packet)
{
  char sectionRequested[512];
  sectionRequested[0] = '\0';
  ArNetPacket sending;

  if (myDefault == NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerConfig::getConfigDefaults: No default config to get");
    client->sendPacketTcp(&sending);
    return;
  }
  myConfigMutex.lock();
  // if we have a section name pick it up, otherwise we send everything
  if (packet->getDataReadLength() < packet->getDataLength())
    packet->bufToStr(sectionRequested, sizeof(sectionRequested));

  //ArConfigArg param;

  ArClientArg clientArg;

  if (sectionRequested[0] == '\0')
    ArLog::log(ArLog::Normal, "Sending all defaults to client");
  else
    ArLog::log(ArLog::Normal, "Sending defaults for section '%s' to client",
               sectionRequested);


  std::list<ArConfigSection *> *sections = myDefault->getSections();

  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       sIt != sections->end(); 
       sIt++)
  {
    ArConfigSection *section = (*sIt);

    if (section == NULL) {
      continue; // Should never happen...
    }

    // if we're not sending them all and not in the right section just cont
    if (sectionRequested[0] != '\0' &&
        ArUtil::strcasecmp(sectionRequested, section->getName()) != 0) {
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
  myConfigMutex.unlock();

  client->sendPacketTcp(&sending);
}


AREXPORT void ArServerHandlerConfig::addPreWriteCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreWriteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreWriteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
    "ArServerHandlerConfig::addPreWriteCallback: Invalid position.");
}

AREXPORT void ArServerHandlerConfig::remPreWriteCallback(
  ArFunctor *functor)
{
  myPreWriteCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerConfig::addPostWriteCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostWriteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostWriteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
    "ArServerHandlerConfig::addPostWriteCallback: Invalid position.");
}

AREXPORT void ArServerHandlerConfig::remPostWriteCallback(
  ArFunctor *functor)
{
  myPostWriteCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerConfig::addConfigUpdatedCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myConfigUpdatedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myConfigUpdatedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
    "ArServerHandlerConfig::addConfigUpdatedCallback: Invalid position.");
}

AREXPORT void ArServerHandlerConfig::remConfigUpdatedCallback(
  ArFunctor *functor)
{
  myConfigUpdatedCallbacks.remove(functor);
}

AREXPORT bool ArServerHandlerConfig::writeConfig(void)
{
  bool ret;
  std::list<ArFunctor *>::iterator fit;

  if (myConfig->getFileName() != NULL && 
      strlen(myConfig->getFileName()) > 0)
  {
    // call our pre write callbacks
    for (fit = myPreWriteCallbacks.begin(); 
	 fit != myPreWriteCallbacks.end(); 
	 fit++) 
    {
      (*fit)->invoke();
    }
    
    // write it 
    ArLog::log(ArLog::Normal, "Writing config file %s", 
	       myConfig->getFileName());
    ret = myConfig->writeFile(myConfig->getFileName());
    
    
    // call our post write callbacks
    for (fit = myPostWriteCallbacks.begin(); 
	 fit != myPostWriteCallbacks.end(); 
	 fit++) 
    {
      (*fit)->invoke();
    }
  }
  return ret;
}

AREXPORT bool ArServerHandlerConfig::configUpdated(ArServerClient *client)
{
  ArNetPacket emptyPacket;

  std::list<ArFunctor *>::iterator fit;

  // call our post write callbacks
  for (fit = myConfigUpdatedCallbacks.begin(); 
       fit != myConfigUpdatedCallbacks.end(); 
       fit++) 
  {
    (*fit)->invoke();
  }
  // this one is okay to exclude, because if the central server is
  // managing the config then its handling those updates and what not
  return myServer->broadcastPacketTcpWithExclusion(&emptyPacket, 
						   "configUpdated", client);
}

AREXPORT void ArServerHandlerConfig::getConfigSectionFlags(
	ArServerClient *client, ArNetPacket *packet)
{
  ArLog::log(ArLog::Normal, "Config section flags requested.");

  ArNetPacket sending;

  std::list<ArConfigSection *> *sections = myConfig->getSections();
  std::list<ArConfigSection *>::iterator sIt;
  sending.byte4ToBuf(sections->size());

  for (sIt = sections->begin(); sIt != sections->end(); sIt++)
  {

    ArConfigSection *section = (*sIt);
    if (section == NULL) 
    {
      sending.strToBuf("");
      sending.strToBuf("");
    }
    else
    {
      sending.strToBuf(section->getName());
      sending.strToBuf(section->getFlags());
    }
  }
  client->sendPacketTcp(&sending);
}
