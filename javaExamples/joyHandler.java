
/* How to use ArJoyHandler in Java */

import com.mobilerobots.Aria.*;

public class joyHandler {

  static {
    try {
        System.loadLibrary("AriaJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library libAriaJava failed to load. Make sure that its directory is in your library path; See javaExamples/README.txt and the chapter on Dynamic Linking Problems in the SWIG Java documentation (http://www.swig.org) for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    Aria.init();
    ArJoyHandler joyHandler = new ArJoyHandler();
    if(!joyHandler.init()) {
      System.out.println("Error initializing joy handler. (No joystick or no joystick OS device installed?)");
      Aria.exit(1);
    }
    boolean havez = joyHandler.haveZAxis();
    System.out.printf("Initialized? %s\tHave Z? %s\tNum Axes %d\tNum Buttons %d\n", 
      joyHandler.haveJoystick()?"yes":"no", havez?"yes":"no", joyHandler.getNumAxes(), joyHandler.getNumButtons());
    while(true) {
      ArJoyVec3f pos = joyHandler.getDoubles();
      ArJoyVec3i adj = joyHandler.getAdjusted();
      ArJoyVec3i unf = joyHandler.getUnfiltered();
      ArJoyVec3i speed = joyHandler.getSpeeds();
      System.out.print("("+pos.getX()+", "+pos.getY()+", "+pos.getZ()+") "+ 
        "\tAdjusted: ("+adj.getX()+", "+adj.getY()+", "+adj.getZ()+") "+
        "\tUnfiltered: ("+unf.getX()+", "+unf.getY()+", "+unf.getZ()+") "+
        "\tSpeed: ("+speed.getX()+", "+speed.getY()+", "+speed.getZ()+")\tButtons: [");
      for(int i = 0; i < joyHandler.getNumButtons(); ++i) {
        if (joyHandler.getButton(i)) {
          System.out.print(i + " ");
        }
      }
      System.out.println("]");
      ArUtil.sleep(1000);
    }
    //Aria.exit(0);
  }
}
