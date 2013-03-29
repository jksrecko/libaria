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
#include "ArSoundPlayer.h"
#include "ArLog.h"
#include "ariaUtil.h"

int ArSoundPlayer::myPlayChildPID = -1;
ArGlobalRetFunctor2<bool, const char*, const char*> ArSoundPlayer::ourPlayWavFileCB(&ArSoundPlayer::playWavFile);
ArGlobalFunctor ArSoundPlayer::ourStopPlayingCB(&ArSoundPlayer::stopPlaying);


AREXPORT ArRetFunctor2<bool, const char*, const char*>* ArSoundPlayer::getPlayWavFileCallback() 
{
  return &ourPlayWavFileCB;
}


AREXPORT ArFunctor* ArSoundPlayer::getStopPlayingCallback()
{
  return &ourStopPlayingCB;
}


#ifdef WIN32

      /* Windows: */

#include <assert.h>

AREXPORT bool ArSoundPlayer::playWavFile(const char* filename, const char* params) 
{
  return (PlaySound(filename, NULL, SND_FILENAME) == TRUE);
}

AREXPORT bool ArSoundPlayer::playNativeFile(const char* filename, const char* params)
{
  /* WAV is the Windows native format */
  return playWavFile(filename, 0);
}

AREXPORT void ArSoundPlayer::stopPlaying()
{
  PlaySound(NULL, NULL, NULL);
}


AREXPORT bool ArSoundPlayer::playSoundPCM16(char* data, int numSamples)
{
  ArLog::log(ArLog::Terse, "INTERNAL ERROR: ArSoundPlayer::playSoundPCM16() is not implemented for Windows yet! Bug reed@activmedia.com about it!");
  assert(false);

  return false;
}

#else

      /* Linux: */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/soundcard.h>
//#include <linux/soundcard.h>
#include <unistd.h>
#include <errno.h>



bool ArSoundPlayer::playNativeFile(const char* filename, const char* params)
{
  int snd_fd = ArUtil::open("/dev/dsp", O_WRONLY); // | O_NONBLOCK);
  if(snd_fd < 0) {
    return false;
  }
  int file_fd = ArUtil::open(filename, O_RDONLY);
  if(file_fd < 0)
  {
    perror("ArSoundPlayer::playNativeFile");
    return false;
  }
  int len;
  const int buflen = 512;
  char buf[buflen];
  while((len = read(file_fd, buf, buflen)) > 0)
  {
    if (write(snd_fd, buf, len) != len) {
      perror("ArSoundPlayer::playNativeFile");
    }
  }
  close(file_fd);
  close(snd_fd);
  return true;
}

bool ArSoundPlayer::playWavFile(const char* filename, const char* params)
{
  char* prog = NULL;
  prog = getenv("PLAY_WAV");
  if(prog == NULL)
    prog = "play";
  myPlayChildPID = fork();
  if(myPlayChildPID == -1) 
  {
    ArLog::log(ArLog::Terse, "ArSoundPlayer: error forking!");
    perror("Aria: ArSoundPlayer");
    return false;
  }
  if(myPlayChildPID == 0)
  {
    // child process: execute sox
    ArLog::log(ArLog::Verbose, "ArSoundPlayer: executing \"%s\" with argument \"%s\" in child process...\n", prog, filename);
    if(execlp(prog, prog, filename, (char*)0) < 0) 
    {
      perror("Aria: ArSoundPlayer: Error executing Wav file playback program");
      exit(-1);
    }
  } 
  // parent process: wait for child to finish
  ArLog::log(ArLog::Verbose, "ArSoundPlayer: created child process %d to play wav file \"%s\".", 
      myPlayChildPID, filename);
  int status;
  waitpid(myPlayChildPID, &status, 0);
  if(WEXITSTATUS(status) != 0) {
    ArLog::log(ArLog::Terse, "ArSoundPlayer: Error: Wav file playback program \"%s\" exited with error code %d.", prog, WEXITSTATUS(status));
    myPlayChildPID = -1;
    return false;
  }
  myPlayChildPID = -1;
  return true;
}



void ArSoundPlayer::stopPlaying()
{
  // Kill a child processes (created by playWavFile) if it exists.
  if(myPlayChildPID > 0)
  {
    ArLog::log(ArLog::Verbose, "ArSoundPlayer: Sending SIGTERM to child process %d.", myPlayChildPID);
    kill(myPlayChildPID, SIGTERM);
  }
}


bool ArSoundPlayer::playSoundPCM16(char* data, int numSamples)
{
  //ArLog::log(ArLog::Normal, "ArSoundPlayer::playSoundPCM16[linux]: opening sound device.");
  int fd = ArUtil::open("/dev/dsp", O_WRONLY); // | O_NONBLOCK);
  if(fd < 0)
    return false;
  int arg = AFMT_S16_LE;
  if(ioctl(fd, SNDCTL_DSP_SETFMT, &arg) != 0)
  {
    close(fd);
    return false;
  }
  arg = 0;
  if(ioctl(fd, SNDCTL_DSP_STEREO, &arg) != 0)
  {
    close(fd);
    return false;
  }
  arg = 16000;
  if(ioctl(fd, SNDCTL_DSP_SPEED, &arg) != 0)
  {
    close(fd);
    return false;
  }
  //ArLog::log(ArLog::Normal, "ArSoundPlayer::playSoundPCM16[linux]: writing %d bytes to sound device.", 2*numSamples);
  int r;
  if((r = write(fd, data, 2*numSamples) < 0))
  {
    close(fd);
    return false;
  }
  close(fd);
  ArLog::log(ArLog::Verbose, "ArSoundPlayer::playSoundPCM16[linux]: finished playing sound. (wrote %d bytes of 16-bit monaural signed sound data to /dev/dsp)", r);
  return true;
}



#endif  // ifdef WIN32

