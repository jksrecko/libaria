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
// ArThread.cc -- Thread classes


#include "ariaOSDef.h"
#include <errno.h>
#include <list>
#include "ArThread.h"
#include "ArLog.h"


ArMutex ArThread::ourThreadsMutex;
ArThread::MapType ArThread::ourThreads;
#ifdef WIN32
std::map<HANDLE, ArThread *> ArThread::ourThreadHandles;
#endif
AREXPORT ArLog::LogLevel ArThread::ourLogLevel = ArLog::Verbose; // todo, instead of AREXPORT move accessors into .cpp?
std::string ArThread::ourUnknownThreadName = "unknown";

AREXPORT void ArThread::stopAll()
{
  MapType::iterator iter;

  ourThreadsMutex.lock();
  for (iter=ourThreads.begin(); iter != ourThreads.end(); ++iter)
    (*iter).second->stopRunning();
  ourThreadsMutex.unlock();
}

AREXPORT void ArThread::joinAll()
{
  MapType::iterator iter;
  ArThread *thread;

  thread=self();
  ourThreadsMutex.lock();
  for (iter=ourThreads.begin(); iter != ourThreads.end(); ++iter)
  {
    if ((*iter).second->getJoinable() && thread && (thread != (*iter).second))
    {
      (*iter).second->doJoin();
    }
  }
  ourThreads.clear();
  // MPL BUG I'm not to sure why this insert was here, as far as I can
  // tell all it would do is make it so you could join the threads
  // then start them all up again, but I don't see much utility in
  // that so I'm not going to worry about it now
 
  //ourThreads.insert(MapType::value_type(thread->myThread, thread));
  ourThreadsMutex.unlock();
}

AREXPORT ArThread::ArThread(bool blockAllSignals) :
  myRunning(false),
  myJoinable(false),
  myBlockAllSignals(blockAllSignals),
  myStarted(false),
  myFinished(false),
  myFunc(0),
  myThread(),
  myStrMap()
{
}

AREXPORT ArThread::ArThread(ThreadType thread, bool joinable,
			    bool blockAllSignals) :
  myRunning(false),
  myJoinable(joinable),
  myBlockAllSignals(blockAllSignals),
  myStarted(false),
  myFinished(false),
  myFunc(0),
  myThread(thread),
  myStrMap()
{
}

AREXPORT ArThread::ArThread(ArFunctor *func, bool joinable,
			    bool blockAllSignals) :
  myRunning(false),
  myJoinable(false),
  myBlockAllSignals(blockAllSignals),
  myStarted(false),
  myFinished(false),
  myFunc(func),
  myThread(),
  myStrMap()
{
  create(func, joinable);
}

#ifndef WIN32
AREXPORT ArThread::~ArThread()
{
  // Just make sure the thread is no longer in the map.
  ourThreadsMutex.lock();
  MapType::iterator iter = ourThreads.find(myThread);
  if (iter != ourThreads.end()) {
    ourThreads.erase(iter);
  }
  ourThreadsMutex.unlock();
}
#endif 

AREXPORT int ArThread::join(void **iret)
{
  int ret;
  ret=doJoin(iret);
  if (ret)
    return(ret);

  ourThreadsMutex.lock();
  ourThreads.erase(myThread);
  ourThreadsMutex.unlock();

  return(0);
}

AREXPORT void ArThread::setThreadName(const char *name)
{ 
  myName = name; 
  std::string mutexLogName;
  mutexLogName = name;
  mutexLogName += "ThreadMutex";
  myMutex.setLogName(mutexLogName.c_str());
}

AREXPORT bool ArThread::isThreadStarted() const
{
  return myStarted;
}

AREXPORT bool ArThread::isThreadFinished() const
{
  return myFinished;
}

AREXPORT const char *ArThread::getThisThreadName(void) 
{
  ArThread *self;
  if ((self = ArThread::self()) != NULL)
    return self->getThreadName();
  else
    return ourUnknownThreadName.c_str();
}

AREXPORT const ArThread::ThreadType * ArThread::getThisThread(void)
{
  ArThread *self;
  if ((self = ArThread::self()) != NULL)
    return self->getThread();
  else
    return NULL;
}

AREXPORT ArThread::ThreadType ArThread::getThisOSThread(void)
{
  ArThread *self;
  if ((self = ArThread::self()) != NULL)
    return self->getOSThread();
  else
    return 0;
}
