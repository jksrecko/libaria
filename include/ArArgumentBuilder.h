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
#ifndef ARARGUMENTBUILDER_H
#define ARARGUMENTBUILDER_H

#include "ariaTypedefs.h"

/// This class is to build arguments for things that require argc and argv
class ArArgumentBuilder
{
public:
  /// Constructor
  AREXPORT ArArgumentBuilder(size_t argvLen = 512, char extraSpaceChar = '\0');
      /// Copy Constructor
  AREXPORT ArArgumentBuilder(const ArArgumentBuilder &builder);
  /// Destructor
  AREXPORT virtual ~ArArgumentBuilder();
#ifndef SWIG
  /** @brief Adds the given string, with varargs, separates if there are spaces
   *  @swignote Not available
   */
  AREXPORT void add(const char *str, ...);
#endif
  /// Adds the given string, without varargs (wrapper for java)
  AREXPORT void addPlain(const char *str, int position = -1);
  /// Adds the given string, without varargs and without touching the str
  AREXPORT void addPlainAsIs(const char *str, int position = -1);
  /// Adds the given string thats divided
  AREXPORT void addStrings(char **argv, int argc, int position = -1);
  /// Adds the given string thats divided
  AREXPORT void addStrings(int argc, char **argv, int position = -1);
  /// Adds the given string thats divided (but doesn't touch the strings)
  AREXPORT void addStringsAsIs(int argc, char **argv, int position = -1);
  /// Gets the original string of the input
  AREXPORT const char *getFullString(void) const;
  /// Sets the full string (this is so you can have a more raw full string)
  AREXPORT void setFullString(const char *str);
  /// Gets the extra string of the input, used differently by different things
  AREXPORT const char *getExtraString(void) const;
  /// Sets the extra string of the input, used differently by different things
  AREXPORT void setExtraString(const char *str);
  /// Prints out the arguments
  AREXPORT void log(void) const;
  /// Gets the argc
  AREXPORT size_t getArgc(void) const;
  /// Gets the argv
  AREXPORT char** getArgv(void) const;
  /// Gets a specific argument as a string
  AREXPORT const char* getArg(size_t whichArg) const;

  /// Sees if an argument is a bool
  AREXPORT bool isArgBool(size_t whichArg) const;

  /// Gets the value of an argument as a boolean 
  /**
   * Valid boolean values are "true" and "false", and "1" and "0".
   * There are two ways to to verify that the specified argument is a bool.
   * Either call isArgBool() before calling this method, or specify a non-NULL
   * ok parameter value. The latter is somewhat more efficient since the 
   * argument string is checked only once.
   * 
   * @param whichArg the size_t index of the arg to retrieve; must be >= 0
   * and less than getArgc()
   * @param ok an optional pointer to a bool that will be set to true if the 
   * arg was successfully retrieved or to false if an error occurred
   * @return bool the retrieved argument value; valid only if ok or isArgBool
   * is true
  **/
  AREXPORT bool getArgBool(size_t whichArg,
                           bool *ok = NULL) const;

  /// Sees if an argument is an int
  AREXPORT bool isArgInt(size_t whichArg) const;

  /// Gets the value of an argument as an integer
  /**
   * There are two ways to to verify that the specified argument is an integer.
   * Either call isArgInt() before calling this method, or specify a non-NULL
   * ok parameter value. The latter is somewhat more efficient because the 
   * digit status of each character is checked only once.
   * 
   * @param whichArg the size_t index of the arg to retrieve; must be >= 0
   * and less than getArgc()
   * @param ok an optional pointer to a bool that will be set to true if the 
   * arg was successfully retrieved or to false if an error occurred
   * @return int the retrieved argument value; valid only if ok or isArgInt 
   * is true
  **/
  AREXPORT int getArgInt(size_t whichArg,
                         bool *ok = NULL) const;

  /// Sees if an argument is a double
  AREXPORT bool isArgDouble(size_t whichArg) const;

  /// Gets the value of an argument as a double
  /**
   * There are two ways to to verify that the specified argument is a double.
   * Either call isArgDouble() before calling this method, or specify a non-NULL
   * ok parameter value. The latter is somewhat more efficient because the 
   * digit status of each character is checked only once.
   * 
   * @param whichArg the size_t index of the arg to retrieve; must be >= 0
   * and less than getArgc()
   * @param ok an optional pointer to a bool that will be set to true if the 
   * arg was successfully retrieved or to false if an error occurred
   * @return double the retrieved argument value; valid only if ok or 
   * isArgDouble is true
  **/
  AREXPORT double getArgDouble(size_t whichArg,
                               bool *ok = NULL) const;

  /// Delete a particular arg, you MUST finish adding before you can remove
  AREXPORT void removeArg(size_t which,
									        bool isRebuildFullString = false);
  /// Combines quoted arguments into one
  AREXPORT void compressQuoted(bool stripQuotationMarks = false);

  /// Turn on this flag to reduce the number of verbose log messages.
  AREXPORT void setQuiet(bool isQuiet);

protected:
  AREXPORT void internalAdd(const char *str, int position = -1);
  AREXPORT void internalAddAsIs(const char *str, int position = -1);
	AREXPORT void rebuildFullString();

  size_t getArgvLen(void) const { return myArgvLen; }
  // how many arguments we had originally (so we can delete 'em)
  size_t myOrigArgc;
  // how many arguments we have
  size_t myArgc;
  // argument list
  char **myArgv;
  // argv length
  size_t myArgvLen;
  // the extra string (utility thing)
  std::string myExtraString;
  // the full string
  std::string myFullString;
  // whether this is our first add or not
  bool myFirstAdd;
  // a character to alternately treat as a space
  char myExtraSpace;

  bool myIsQuiet;
};

#endif // ARARGUMENTBUILDER_H
