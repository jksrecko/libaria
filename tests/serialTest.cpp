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
#include "Aria.h"

int main(void)
{
  int ret;
  char bufWrite[1024];
  char bufRead[1024];
  int i, n;
  for (i = 0; i < 1024; i++)
    bufWrite[i] = 0x66;

  srand(time(NULL));
  
  int bytes1 = 0;
  int bytes2 = 0;
  //int numToWrite = 1;

  ArTime lastPrint;

  ArSerialConnection ser1;
  ArSerialConnection ser2;
  ser1.setPort(ArUtil::COM1);
//  ser1.setBaud(38400);
  ser2.setPort(ArUtil::COM2);
//  ser2.setBaud(38400);
  if (!ser1.openSimple())
    {
      printf("Exiting since open failed\n");
      exit(0);
    }
  if (!ser2.openSimple())
    {
      printf("Exiting since open failed\n");
      exit(0);
    }
  printf("Ports opened");
  lastPrint.setToNow();
  while (1)
    {
      if (ser1.write(bufWrite, rand() % 1024) < 0)
	printf("Failed write\n");
      n = rand() % 1024;
      if ((ret = ser2.read(bufRead, n)) < 0)
	printf("Failed read\n");
      else if (ret > 0) 
	{
	  for (i = 0; i < ret; i++)
	    if (bufRead[i] != 0x66)
	      {
		printf("Failed\n");
		break;
	      }
	  bytes2 += ret;
	} 
       
      if (ser2.write(bufWrite, rand() % 1024) < 0)
	printf("Failed2 write\n");
      n = rand() % 1024;
      if ((ret = ser1.read(bufRead, n)) < 0)
	printf("Failed2 read\n");
      else if (ret > 0)
	{
	  for (i = 0; i < ret; i++)
	    if (bufRead[i] != 0x66)
	      {
		printf("Failed2 0x%x %d/%d\n", bufRead[i], i, n);
		break;
	      }
	  bytes1 += ret;
	}

      if (lastPrint.mSecSince() > 1000)
	{
	  printf("%d %d\n", bytes1, bytes2);
	  lastPrint.setToNow();
	}
    }
	       
}
