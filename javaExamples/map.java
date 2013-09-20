
/* A simple example of connecting to and driving the robot with direct
 * motion commands.
 */

import com.mobilerobots.Aria.*;

public class map {

  static {
    try {
        System.loadLibrary("AriaJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library libAriaJava failed to load. Make sure that its directory is in your library path; See javaExamples/README.txt and the chapter on Dynamic Linking Problems in the SWIG Java documentation (http://www.swig.org) for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    System.out.println("Starting Java Map Test");

    Aria.init();

    //System.out.println("Will be able to load maps from the c++ maps directory:
    //../maps.");
    //ArMap map = new ArMap("../maps/");
    ArMap map = new ArMap();
    System.out.println("loading map: ../maps/columbia.map");
    if(!map.readFile("../maps/columbia.map"))
    {
        System.out.println("Error loading map.");
    }
    else
    {
        //System.out.println("writing map: out.map in ../maps directory");
        System.out.println("writing map: out.map");
        if(!map.writeFile("out.map"))
        {
            System.out.println("error writing map");
        }
    }

    Aria.exit(0);
  }
}
