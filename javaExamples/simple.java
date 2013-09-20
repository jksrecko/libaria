
/* A simple example of connecting to and driving the robot with direct
 * motion commands.
 */

import com.mobilerobots.Aria.*;

public class simple {

  static {
    try {
        System.loadLibrary("AriaJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library libAriaJava failed to load. Make sure that its directory is in your library path; See javaExamples/README.txt and the chapter on Dynamic Linking Problems in the SWIG Java documentation (http://www.swig.org) for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    System.out.println("Starting Java Test");

    Aria.init();

    ArRobot robot = new ArRobot();
    ArSimpleConnector conn = new ArSimpleConnector(argv);
 
    if(!Aria.parseArgs())
    {
      Aria.logOptions();
      Aria.exit(1);
    }

    if (!conn.connectRobot(robot))
    {
      System.err.println("Could not connect to robot, exiting.\n");
      System.exit(1);
    }
    robot.runAsync(true);
    robot.lock();
    System.out.println("Sending command to move forward 1 meter...");
    robot.enableMotors();
    robot.move(1000);
    robot.unlock();
    System.out.println("Sleeping for 5 seconds...");
    ArUtil.sleep(5000);
    robot.lock();
    System.out.println("Sending command to rotate 90 degrees...");
    robot.setHeading(90);
    robot.unlock();
    System.out.println("Sleeping for 5 seconds...");
    ArUtil.sleep(5000);
    robot.lock();
    System.out.println("Robot coords: robot.getX()=" + robot.getX() + ", robot.getY()=" + robot.getY() + ", robot.getTh()=" + robot.getTh()); 
    ArPose p = robot.getPose();
    System.out.println("               pose.getX()=" + p.getX() +     ", pose.getY()="  + p.getY() +     ",  pose.getTh()=" + p.getTh());
    double[] xout = {0};
    double[] yout = {0};
    double[] thout = {0};
    p.getPose(xout, yout, thout);
    System.out.println("              pose.getPose(): x=" + xout[0] + ", y=" + yout[0] + ", th=" + thout[0]);
    robot.unlock();
    robot.lock();
    System.out.println("exiting.");
    robot.stopRunning(true);
    robot.unlock();
    robot.lock();
    robot.disconnect();
    robot.unlock();
    Aria.exit(0);
  }
}
