
  ARIA
  Adept MobileRobots Advanced Robotics Interface for Applications

  Version 2.7.5.2
  
  Linux 

Copyright 2002, 2003, 2004, 2005 
ActivMedia Robotics, LLC. All rights reserved.

Copyright 2006, 2007, 2008, 2009
MobileRobots Inc. All rights reserved.

Copyright 2010, 2011, 2012
Adept Technology. All rights reserved.

See LICENSE.txt for full license information about ARIA.

Please read this document for important details about using ARIA.

   Contents
   ========
*  Introduction
*  Documentation
*  Licenses and Sharing
*  ARIA Package
*  Files of Note
*  Compiling programs that use ARIA
   *  Windows
   *  Linux
      *  Using ARIA's Makefiles
      *  Using your own Makefile or other build system
      *  Learning more about using Linux
   *  Learning more about C++
*  Using ARIA from Python and Java
*  Simulator

Welcome to the Adept MobileRobots Advanced Robotics Interface for Applications (ARIA).
ARIA is an object-oriented, application programming interface (API) for
Adept MobileRobots (and ActivMedia) line of intelligent mobile robots, including
Pioneer 2/3 DX and AT, PeopleBot, PowerBot, AmigoBot, PatrolBot/Guiabot,
Seekur and SeekurJr mobile robots.

Written in the C++ language, ARIA is client-side software for easy,
high-performance access to and management of the robot server, as well
to the many accessory robot sensors and effectors, and useful utilities. 
   
ARIA can be run multi- or single-threaded, using its own wrapper
around Linux pthreads and WIN32 threads.  Use ARIA in many different
ways, from simple command-control of the robot server for direct-drive
navigation, to development of higher-level intelligent actions (aka
behaviors).   Add your own features and modifications to ARIA: It's 
open-source!

The ARIA package includes both source code and pre-build libraries
and example programs. These libraries and programs were build with
GCC 3.4 if on Linux, and MS Visual Studio .NET 2003 (7.1), Visual C++ 
Express 2008 (9.0) or Visual C++ Express 2010 (10.0) if on Windows.  
Using these compilers for development is recommended. 

NOTE: If you use a different compiler or compiler version, you must 
rebuild the ARIA libraries to ensure link compatability.

See below for more information about building programs with ARIA
on Windows and Linux and using the Windows and Linux development tools.


Documentation and Help
======================

Follow the INSTALL text instructions to install ARIA on your Linux or 
Windows workstation or robot computer. System requirements are included 
in the INSTALL.txt file.

ARIA includes a full API Reference Manual in HTML format. This manual,
Aria-Reference.html (and in the docs/ directory), includes documentation 
about each of the classes and methods in ARIA, as well as a comprehensive 
overview describing how to get stated understanding and using ARIA.  In 
addition, ARIA includes several example programs in the examples/ 
(start with simpleConnect.cpp, then explore the others) and 
advanced/ directories, and the header files and source code are included
in include/ and src/ as well.  

The ArNetworking library has its own reference manual,
ArNetworking-Reference.html in the ArNetworking subdirectory, and examples
in ArNetworking/examples.

If you plan on using the Java or Python wrapper libraries, see the 
javaExamples, pythonExamples, ArNetworking/javaExamples and
ArNetworking/pythonExamples directories for important information in
README files, and example programs. You should also read the Aria Reference
manual for general information about Aria -- the API in the wrapper libraries
are almost identical to the C++ API.

If you have any problems or questions using ARIA or your robot, the 
MobileRobots support site provides:
 
  * A FAQ (Frequently Asked Questions) list, at <http://robots.mobilerobots.com/FAQ.html>
  * A knowlege base of information on robot hardware and software, at <http://robots.mobilerobots.com>
  * All robot and device manuals
  * The Aria-Users mailing list, where you can discuss Aria with other users and 
    MobileRobots software developers:
      * Search the archives at <http://robots.mobilerobots.com/archives/aria-users/threads.html>
      * Join the list at <http://robots.mobilerobots.com/archives/aria-info.html>
  * Information on contacting MobileRobots technical support.

License and Sharing
===================

ARIA is released under the GNU Public License, which means that if you
distribute any work which uses ARIA, you must distribute the entire 
source code to that work under the GPS as well.  Read the included 
LICENSE text for details.  We open-sourced ARIA under GPL with full
source code not only for your convenience, but also so that you can 
share your enhancements to the software.  If you wish your enhancements 
to make it into our future ARIA versions, you will need to assign the 
copyright on those changes to Adept. Contact lafary@mobilerobots.com 
with these changes or with questions about this.

Accordingly, please do share your work, and please sign up for the 
exclusive aria-users@mobilerobots.com newslist so that you can benefit 
from others' work, too.

ARIA may instead be relicensed for proprietary, closed source applications. 
Contact sales@mobilerobots.com for details.

For answers to frequently asked questions about what the GPL allows
and requires, see <http://www.gnu.org/licenses/gpl-faq.html>


The ARIA Package
================

Aria/:
  docs      The API Reference Manual: Extensive documentation of all of ARIA
  examples  ARIA examples -- a good place to start; see examples/README.txt
  include   ARIA header (*.h) files
  src       ARIA source code (*.cpp) files
  lib       Libraries (.lib files for Windows, .so files for Linux) 
  bin       Contains Windows binaries and DLLs.
  params    Robot definition (parameter) files ("p3dx.p", for example)
  tests     Test files, somewhat esoteric but useful during ARIA development
  utils     Utility commands, not generally needed
  advanced  Advanced demos, not for the faint of heart (or ARIA novice)
  python    Python wrapper package
  java      Java wrapper package

Aria/ArNetworking/:  (A library used to facilitate network communication)
  docs      API Reference Manual for ArNetworking 
  examples  ArNetworking examples
  include   ArNetworking header (*.h) files, of course
  src       ArNetworking source (*.cpp) files
  python    Python wrapper package
  java      Java wrapper package
  

Other ARIA Files of Note
========================

LICENSE.txt        GPL license; agree to this to use ARIA
Aria-vc2003.sln    MSVC++ 2003 solution for building ARIA and 'demo'.
Aria-vc2008.sln    MSVC++ 2008 solution file for building ARIA and demo.
Aria-VC2010.sln    MSVC++ 2010 solution file forbuilding Aria and demo.
examples/All_examples-vc2003.sln 
                  MSVC++ 2003 solution for building all of the examples
examples/All_examples-vc2008.sln 
                  MSVC++ 2008 solution for building all of the examples
examples/All_examples-vc2010.sln 
                  MSVC++ 2010 solution for building all of the examples
Makefile          Linux makefile (GNU Make) for building ARIA and examples
Makefile.dep      Linux dependency file
INSTALL.txt       More information about installing ARIA
README.txt        This file; also see READMEs in advanced/, examples/, and tests/
Changes.txt       Summary of changes in each version of ARIA


Building programs that use ARIA
===============================

Windows
-------

On Windows, use Microsoft Visual Studio .NET 2003 (MS Visual C++ 7.1),
Visual C++ 2008 (VC9), or Visual C++ 2010 (VC10).

Note: If you use a different version of Visual C++, you must rebuild 
the ARIA libraries. We recommand and support only these versions; other
versions may or may not work.

You can add your new project the Visual Studio solution files provided with ARIA if you
wish, or create a new solution and add your new project as well as projects
for the ARIA libraries you use.

Now you must configure your project to find ARIA. These instructions
assume a default installation; if you installed ARIA elsewhere you
must use different paths. If you keep your Visual Studo project within
Aria's directory, you can also use relative paths (e.g. "..\lib" for the
library path).

Aria libraries are provided in two variants, "Debug" libraries that use 
the "Multithreaded Debug DLL" runtime library, and non-Debug (or "Release") libraries
that use the plain "Multithreaded DLL" runtime library.  The ARIA solutions
use a "Debug" and a "Release" configuration to select these variants; you
can also use these configuration names as well if you wish.

1. Create new configuration in the solution: Release and Debug.

2. From the Project Menu, choose to edit your projects Properties. Or
  right-click on the project in the Solution Explorer and choose Properties.

To edit your new project's configuration properties, right click on the 
project in the solution explorer pane and choose Properties.

3. Change the "Configuration" (upper left of the dialog) menu to "All
  Configurations".

4. Click on the "General" section

  * Set "Output Files" to "C:\Program Files\MobileRobots\Aria\bin" 
    (Aria's bin directory, where Aria.dll etc. are).  Or, if you want your
    program saved in a different directory, you must put Aria's bin directory
    in your system PATH environment variable, or copy the DLLs to your own
    output directory.

  * You may want to change Intermediate Files. A useful value is
    "obj\$(ConfigurationName)".

  * (Visual Studio 2003) Set "Use Managed Extensions" to "no".

  * (Visual C++ 2008) Set "Common Language Runtime Support" to "No Common
    Language Runtime Support"

5. Click on the "Link" or "Linker" section.

  * Click on the "Input" subsection.

    * To "Additional Library Path" add "C:\Program Files\MobileRobots\Aria\lib".

    * Change the "Configuration" menu (upper left of the window) to "Debug"

    * (Visual Studio 2003) To "Additional Dependencies", add:
        AriaDebug.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebug.lib ws2_32.lib

    * (Visual C++ 2008) To "Additional Dependencies", add:
        AriaDebugVC9.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebugVC9.lib ws2_32.lib

    * Change the "Configuration" menu to "Release"

    * (Visual Studio 2003) To "Additional Dependencies", add:
        Aria.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworking.lib ws2_32.lib

    * (Visual C++ 2008) To "Additional Dependencies", add:
        AriaVC9.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingVC9.lib ws2_32.lib
  
    * Change the "Configuration" menu (upper left of the window) to "Debug"

    * If using Visual Studio 2003, To "Additional Dependencies", add:
        AriaDebug.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebug.lib ws2_32.lib

    * Or, if using Visual Studio 2008, To "Additional Dependencies", add:
        AriaDebugVC9.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebugVC9.lib ws2_32.lib

    * Change the "Configuration" menu to "Release"

    * If using Visual Studio 2003, To "Additional Dependencies", add:
        Aria.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworking.lib ws2_32.lib

    * Or, if using Visual Studio 2008, To "Additional Dependencies", add:
        AriaVC9.lib winmm.lib advapi32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingVC9.lib ws2_32.lib
  
    * Change the "Configuration" menu back to "All Configurations"

6. Click on the "C++" section.

  * Click on the "General" subsection

    * To "Additional Include Directories" add 
      "C:\Program Files\MobileRobots\Aria\include".

   * Click on the "Code Generation" subsection

     * Change the "Configuration" menu (upper left of the window) to "Debug".

       * Under the "Use run-time library" pulldown select "Debug
         Multithreaded DLL".

     * Change the "Configuration" menu (upper left of the screen) to "Release".

       * Under the "Use run-time library" pulldown select "Multithreaded DLL".

     * Change the "Configuration" menu back to "All Configurations"

Linux
-----

On GNU/Linux two tools are used: the compiler (GCC), which compiles
and/or links a single file, and Make, which is used to manage 
building multiple files and their dependencies.  If you need to
deal with multiple files, you can copy a Makefile and modify it, 
or create a new one.

Note: In packages for Debian, the shared libraries were built with the 
standard G++ version provided with the Debian system. You must use 
the same version of G++ or a compatible version to build programs that 
link against it, or rebuild ARIA using your preferrend G++ version. (On
Debian systems, the default g++ and gcc compiler versions may be changed
using the 'galternatives' program or the 'update-alternatives' command). 

When compiling ARIA or its examples, you may also temporarily override the 
C++ compiler command ('g++' by default) by setting the CXX environment 
variable before or while running make. E.g.: "make CXX=g++-3.4" or 
"export CXX=g++-3.4; make".

More information and documentation about GCC is available at 
<http://gcc.gnu.org>, and in the gcc 'man' page.  More information 
about Make is at <http://www.gnu.org/software/make/>. Several graphical 
IDE applications are available for use with GCC, though none are 
currently supported by MobileRobots.



# Using ARIA's Makefiles on Linux: #

The ARIA Makefile is able to build any program consisting 
of one source code file in the 'examples', 'tests', or 'advanced' 
directory.  For example, if your source code file is 
'examples/newProgram.cpp', then you can run 'make examples/newProgram' 
from the top-level Aria directory, or 'make newProgram' from within 
the 'examples' directory, to build it.   This makes it easy to copy
one of the example programs to a new name and modify it -- a good way
to learn how to use ARIA.



# Using ARIA from your own Makefile or other build system: #

If you want to keep your program in a different place than the Aria 
examples directory, and use your own Makefile or other build tool,
you need to use the following g++ options to compile the program:

 -fPIC -I/usr/local/Aria/include 

If you wish, you may also use -g and -Wall for debugging information
and useful warnings.   Aria does not use exceptions, so you may also
use -fno-exceptions if you wish; this will cause any use of exceptions
in your program to trigger a compile error.

For linking with libAria use these options:

  -L/usr/local/Aria/lib -lAria -lpthread -ldl -lrt

If you are also using ArNetworking, use the following compile option
in addition to the Aria options above:

  -I/usr/local/Aria/ArNetworking/include
  
And for linking to libArNetworking:

  -lArNetworking


A Makefile for a program called "program" with source file "program.cpp"
might look something like this:


all: program

CFLAGS=-fPIC -g -Wall
ARIA_INCLUDE=-I/usr/local/Aria/include
ARIA_LINK=-L/usr/local/Aria/lib -lAria -lpthread -ldl

%: %.cpp
	$(CXX) $(CFLAGS) $(ARIA_INCLUDE) $< -o $@ $(ARIA_LINK)


Refer to the GNU Make manual <http://www.gnu.org/software/make> or 
other books or documentation about Make to learn more.


# Learning more about using Linux: #

If you are new to using GNU/Linux, some guides on getting started can be 
found at the following sites:

 * If your robot is using RedHat (it displays "RedHat Linux" on the console), 
   start with
   <https://www.redhat.com/docs/manuals/linux/RHL-7.3-Manual/getting-started-guide/>.
   More at <https://www.redhat.com/support/resources/howto/rhl73.html>.

 * If your robot is using Debian, start with 
   <http://www.us.debian.org/doc/manuals/debian-tutorial>. More is at
   <http://www.debian.org/doc/>.

 * <http://www.tldp.org> is a repository of many different guides, FAQ and
   HOWTO documents about Linux in general, including various programming
   and system administration tasks.

For more depth, there are many books about using Linux, consult your
local computer bookseller. The ideal way to learn about Linux is to work 
with an experienced colleague who can demonstrate things and answer 
questions as they arise.



Learning C++
------------

If you are new to C++ programming, the definitive guide is Bjarne
Stroustrup's book "The C++ Programming Language".   The book
"C++ In a Nutshell", published by O'Reilly & Associates, is a
good quick guide and reference. There are also several websites
and many other books about C++.


Using ARIA from Java or Python
==============================

Even though ARIA was written in C++, it is possible to use ARIA from
the Java and Python programming languages as well. The directories
'python' and 'java' contain Python and Java packages automatically generated
by the 'swig' tool (http://www.swig.org) which mirror the ARIA API
closely, and are "wrappers" around the native ARIA library: When you call
an ARIA function in a Python or Java program, the wrapper re-marshalls the
function arguments and makes the equivalent call into the C++ ARIA library.
See the 'pythonExamples' and 'javaExamples' directories for more information
and example programs.  See the ARIA API Reference Manual in 'docs' for 
some more information.

More about the Python programming language is at <http://www.python.org>.
More about the Java programming language is at <http://java.sun.com>.


MobileSim Simulator
===================

SRIsim is no longer included with Aria.  There is now a seperately
downloadable MobileSim simulator at our support webpage 
<http://robots.mobilerobots.com>.  MobileSim is compatible with SRISim,
but adds many new features.




-------------------------
Adept MobileRobots
10 Columbia Drive
Amherst, NH, 03031. USA

support@mobilerobots.com
http://robots.mobilerobots.com
http://www.mobilerobots.com


